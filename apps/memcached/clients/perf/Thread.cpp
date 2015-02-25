/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT memcached application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "Client.h"

#define MAX_KEY_SIZE	64
#define MAX_DATA_SIZE	3000

#include "Timer.h"

uint64_t GetTime() {
	struct timeval st;
	gettimeofday(&st, NULL);
	return st.tv_sec * 1000000ULL + (uint64_t)st.tv_usec;
}

void CThread::CreateThread() {
    pthread_attr_t  attr;
    int             ret;

    pthread_attr_init(&attr);

 	if ((ret = pthread_create(&m_threadId, &attr, CThread::StartThread_helper, this)) != 0) {
        fprintf(stderr, "Can't create thread: %s\n", strerror(ret));
        exit(1);
    }
}

void CThread::InitKey(int reqNum, int keyIdx, bool bMiss)
{
    int pos = 0;
    if (!g_bBinary) {
        m_key[keyIdx][0] = ' ';
        pos = 1;
    }
    strncpy(m_key[keyIdx]+pos, m_hostName, m_hostNameLen);
    pos+=m_hostNameLen;
    if (bMiss) {
	    sprintf(m_key[keyIdx]+pos, "Key_t%04d_s%08d_", m_tid + g_keyStart, reqNum);
	    memset(m_key[keyIdx]+pos+20, 'x', g_keyLen - 20-pos);
    } else {
	    sprintf(m_key[keyIdx]+pos, "Key_t%04d_s%08d_", m_tid + g_keyStart, reqNum);
	    memset(m_key[keyIdx]+pos+20, 'a', g_keyLen - 20-pos);
    }
    m_key[keyIdx][g_keyLen+pos] = '\0';
}

void CThread::InitValue(int reqNum, size_t len)
{
	if (len == 0)
		len = g_valueLen;

	sprintf(m_pValue, "Value_t%04d_s%08d_", m_tid + g_keyStart, reqNum);
	memset(m_pValue+22, 'a', len - 22);
}

void CThread::CloseSockets()
{
	if (m_udpFd) {
		closesocket(m_udpFd);
		m_udpFd = 0;
	}
	if (m_tcpFd) {
		closesocket(m_tcpFd);
		m_tcpFd = 0;
	}
}

void CThread::StartThread()
{
	m_pValue = new char [g_valueLen+1];
	m_opaque = m_tid << 24;
	m_getRespCnt = 0;
    m_getReqCnt = 0;
    m_multiGetReqCnt = 0;
    m_iovLen = 0;

	TcpSocket();

    if (g_bFlushAll)
	    FlushAll();

	__sync_fetch_and_sub(&g_flushAllThreads, 1);

	while (g_flushAllThreads > 0)
		usleep(1000);

	// fill memcached server with stores
	for(int reqNum = 0; reqNum < g_storeCnt; reqNum += 1) {
		StoreReq(reqNum);
		m_respQue.push(reqNum);

		if ((int)m_respQue.size() == g_overlapReqNum) {
			int respNum = (int)m_respQue.front();
			m_respQue.pop();

			if (!StoreResp(respNum))
				break;

            m_storeCnt = respNum+1;
		}
	}

	while (!m_respQue.empty()) {
		int respNum = (int)m_respQue.front();
		m_respQue.pop();
		StoreResp(respNum);
	}

	__sync_fetch_and_sub(&g_storeThreads, 1);

	while (g_storeThreads > 0 && g_numThreads.m_max <= m_tid)
		usleep(1000);

	// now perform get operations
    if (g_burstDelay.m_max > 0)
        PerformBurstTest();
    else
        PerformContinuousTest();
}

void CThread::PerformBurstTest()
{
    int reqNum = 0;
	int respNum = 0;
    extern double g_missInterval;
    double nextMiss = g_missInterval;

    for (;;) {
        m_overlapReqCnt = 0;
        do {
		    m_respQue.push(GetTime());

            bool bMiss = false;
            bool bForceMultiGetEnd = m_overlapReqCnt+1 == g_overlapReqNum;

		    GetReq(reqNum, bMiss, bForceMultiGetEnd);

            reqNum = (reqNum + 1) % m_storeCnt;
            m_overlapReqCnt += 1;

            if (reqNum == 0)
                nextMiss = g_missInterval;

        } while (m_overlapReqCnt != g_overlapReqNum);
        
        m_overlapReqCnt = 0;
        while (m_respQue.size() > 0) {
			uint64_t reqTime = m_respQue.front();
			m_respQue.pop();

            if (reqTime > 0) {
                bool bForceMultiGetEnd = m_overlapReqCnt+1 == g_overlapReqNum;

			    if (!GetResp(respNum, bForceMultiGetEnd)) {
                    while (__sync_fetch_and_add(&g_errorReport, 1) != 1);

				    fprintf(stderr, "Get failure\n");
				    exit(1);
			    }

			    uint64_t respTime = GetTime();
			    uint64_t delta = respTime - reqTime;

			    m_getRespCnt += 1;
			    m_getRespTime += delta;
			    if (m_getRespMinTime > delta) m_getRespMinTime = delta;
			    if (m_getRespMaxTime < delta) m_getRespMaxTime = delta;
            }

			respNum = (respNum + 1) % m_storeCnt;
            m_getReqCnt += 1;
            m_overlapReqCnt += 1;
		}

        // delay between bursts
        usleep(g_burstDelay.m_cur);
	}
}

void CThread::PerformContinuousTest()
{

    struct CHist {
        uint64_t    m_reqNum;
        double      m_nextMiss;
        bool        m_bMiss;
        int64_t     m_reqTime;
        int64_t     m_delta;
    } m_hist[256];

	int respNum = 0;
    extern double g_missInterval;
    double nextMiss = g_missInterval;
	for(int reqNum = 0; ; reqNum = (reqNum + 1) % m_storeCnt) {

        if (reqNum == 0)
            nextMiss = g_missInterval;

        CHist *pHist = m_hist + (reqNum & 0xff);
        pHist->m_reqNum = reqNum;
        pHist->m_nextMiss = nextMiss;
        pHist->m_bMiss = reqNum > nextMiss;

        if (pHist->m_bMiss) {
            pHist->m_reqTime = GetTime();
            nextMiss += g_missInterval;
            m_respQue.push(0);
            GetReq(reqNum, true);
        } else {
            pHist->m_reqTime = GetTime();
		    m_respQue.push(GetTime());
		    GetReq(reqNum);
        }

        pHist = m_hist + (respNum & 0xff);

		if ((int)m_respQue.size() == g_overlapReqNum) {
			uint64_t reqTime = m_respQue.front();
			m_respQue.pop();

            if (reqTime > 0) {

                if (!GetResp(respNum, false)) {
                    while (__sync_fetch_and_add(&g_errorReport, 1) != 1);

				    fprintf(stderr, "Get failure\n");
				    exit(1);
			    }

			    uint64_t respTime = GetTime();
			    uint64_t delta = respTime - reqTime;

			    m_getRespCnt += 1;
			    m_getRespTime += delta;
			    if (m_getRespMinTime > delta) m_getRespMinTime = delta;
			    if (m_getRespMaxTime < delta) m_getRespMaxTime = delta;
            }

            pHist->m_delta = GetTime() - pHist->m_reqTime;
			respNum = (respNum + 1) % m_storeCnt;
            m_getReqCnt += 1;
		}
	}
}

void CThread::FlushAll()
{
    if (g_bBinary)
        FlushAllBinary();
    else
        FlushAllAscii();
}

void CThread::FlushAllAscii()
{
	struct msghdr msghdr = { 0 };
	struct iovec iov[5];

	msghdr.msg_iov = iov;
	size_t & len = msghdr.msg_iovlen = 0;

	iov[len].iov_base = (void *)"flush_all\r\n";
	iov[len].iov_len = 11;
	len += 1;

	SendMsgTcp(msghdr);

    char respBuf[10];
	RespReadTcp(&respBuf, 4);
    assert(memcmp(respBuf, "OK\r\n", 4) == 0);
}

void CThread::FlushAllBinary()
{
	protocol_binary_request_header binHdr;

	memset(&binHdr, 0, sizeof(binHdr));

	binHdr.request.magic = PROTOCOL_BINARY_REQ;

	binHdr.request.opcode = PROTOCOL_BINARY_CMD_FLUSH;

	binHdr.request.keylen = 0;
	binHdr.request.extlen = 0;
	binHdr.request.bodylen = 0;
	binHdr.request.opaque = m_opaque++;
	binHdr.request.cas = 0;

	struct msghdr msghdr = { 0 };
	struct iovec iov[5];

	msghdr.msg_iov = iov;
	size_t & len = msghdr.msg_iovlen = 0;

	iov[len].iov_base = (void *)&binHdr;
	iov[len].iov_len = sizeof(binHdr);
	len += 1;

	SendMsgTcp(msghdr);

	protocol_binary_response_header respHdr;

	RespReadTcp(&respHdr, sizeof(protocol_binary_response_header));
	assert(respHdr.response.magic == PROTOCOL_BINARY_RES);
	assert(respHdr.response.opcode == binHdr.request.opcode);
	assert(respHdr.response.keylen == 0);
	assert(respHdr.response.extlen == 0);
	assert(respHdr.response.datatype == 0);
	assert(respHdr.response.bodylen == htonl(0));
	assert(respHdr.response.opaque == binHdr.request.opaque);
	//assert(respHdr.response.cas == 0);
	assert(ntohs(respHdr.response.status) == PROTOCOL_BINARY_RESPONSE_SUCCESS);
}

void CThread::StoreReq(int reqNum)
{
    if (g_bBinary)
        StoreReqBinary(reqNum);
    else
        StoreReqAscii(reqNum);
}

void CThread::StoreReqAscii(int reqNum)
{
	InitKey(reqNum, 0, false);
	InitValue(reqNum);

	// perform store with generated key/data
	struct msghdr msghdr = { 0 };
	struct iovec iov[8];

	msghdr.msg_iov = iov;
	size_t & len = msghdr.msg_iovlen = 0;

	iov[len].iov_base = (void *)"set";
	iov[len].iov_len = 3;
	len += 1;

	iov[len].iov_base = m_key[0];
	iov[len].iov_len = g_keyLen+1;
	len += 1;

	uint32_t flags = (m_tid << 24) | reqNum;
    char flagStr[32];

    iov[len].iov_base = flagStr;
    iov[len].iov_len = snprintf(flagStr, 32, " %d", flags);
    len += 1;

    iov[len].iov_base = (void *)" 0";   // expTime
    iov[len].iov_len = 2;
    len += 1;

    char lenStr[32];

	iov[len].iov_base = lenStr;
	iov[len].iov_len = snprintf(lenStr, 32, " %d\r\n", g_valueLen);
	len += 1;

	iov[len].iov_base = (void *)m_pValue;
	iov[len].iov_len = g_valueLen;
	len += 1;

	iov[len].iov_base = (void *)"\r\n";
	iov[len].iov_len = 2;
	len += 1;

	SendMsgTcp(msghdr);
}

void CThread::StoreReqBinary(int reqNum)
{
	InitKey(reqNum, 0, false);
	InitValue(reqNum);

	// perform store with generated key/data
	protocol_binary_request_header binHdr;

	memset(&binHdr, 0, sizeof(binHdr));

	binHdr.request.magic = PROTOCOL_BINARY_REQ;

	binHdr.request.opcode = PROTOCOL_BINARY_CMD_SET;
	binHdr.request.extlen = 8;
	binHdr.request.keylen = htons(g_keyLen);
	binHdr.request.bodylen = htonl(binHdr.request.extlen + g_keyLen + g_valueLen);
	binHdr.request.opaque = (m_tid << 24) | reqNum;
	binHdr.request.cas = 0;

	struct msghdr msghdr = { 0 };
	struct iovec iov[5];

	msghdr.msg_iov = iov;
	size_t & len = msghdr.msg_iovlen = 0;

	iov[len].iov_base = (void *)&binHdr;
	iov[len].iov_len = sizeof(binHdr);
	len += 1;

	uint32_t flags;
	uint32_t exptime;
	if (binHdr.request.extlen == 8) {
		flags = htonl((m_tid << 24) | reqNum);
		iov[len].iov_base = (void *)&flags;
		iov[len].iov_len = 4;
		len += 1;

		exptime = 0;
		iov[len].iov_base = (void *)&exptime;
		iov[len].iov_len = 4;
		len += 1;
	}

	iov[len].iov_base = (void *)m_key[0];
	iov[len].iov_len = g_keyLen;
	len += 1;

	iov[len].iov_base = (void *)m_pValue;
	iov[len].iov_len = g_valueLen;
	len += 1;

	SendMsgTcp(msghdr);
}

bool CThread::StoreResp(int respNum)
{
    if (g_bBinary)
        return StoreRespBinary(respNum);
    else
        return StoreRespAscii(respNum);
}

bool CThread::StoreRespAscii(int respNum)
{
    char respBuf[32];

	RespReadTcp(respBuf, 8);

	if (memcmp(respBuf, "STORED\r\n", 8) == 0)
		return true;
	else {
        RespReadTcp(respBuf+8, 4);
        if (memcmp(respBuf, "NOT_STORED\r\n", 8) == 0) {
            return false;
	    } else {
		    assert(0);
		    return false;
	    }
    }
}

bool CThread::StoreRespBinary(int respNum)
{
	protocol_binary_response_header respHdr;
	const char * pErrStr;

	RespReadTcp(&respHdr, sizeof(protocol_binary_response_header));
	assert(respHdr.response.magic == PROTOCOL_BINARY_RES);
	assert(respHdr.response.opcode == PROTOCOL_BINARY_CMD_SET);
	assert(respHdr.response.keylen == 0);
	assert(respHdr.response.extlen == 0);
	assert(respHdr.response.datatype == 0);
	assert(respHdr.response.opaque == (uint32_t)((m_tid << 24) | respNum));

	if (ntohs(respHdr.response.status) == PROTOCOL_BINARY_RESPONSE_SUCCESS)
		return true;
	else if (ntohs(respHdr.response.status) == PROTOCOL_BINARY_RESPONSE_ENOMEM) {
		pErrStr = "Out of memory";
		assert(respHdr.response.bodylen == htonl(strlen(pErrStr)));
		RespCheckTcp(pErrStr, strlen(pErrStr));
		return false;
	} else {
		assert(0);
		return false;
	}
}

void CThread::GetReq(int reqNum, bool bMiss, bool bForceMultiGetEnd)
{
    if (g_bBinary)
        GetReqBinary(reqNum, bMiss, bForceMultiGetEnd);
    else
        GetReqAscii(reqNum, bMiss, bForceMultiGetEnd);
}

void CThread::GetReqAscii(int reqNum, bool bMiss, bool bForceMultiGetEnd)
{
	InitKey(reqNum, m_overlapReqCnt, bMiss);

	//char udphdr[UDP_HEADER_SIZE];
	//if (m_bUdpEnabled) {
	//	msghdr.msg_name = &m_udpAddr;
	//	msghdr.msg_namelen = m_udpAddrLen;
	//	
	//	CreateUdpHeader(udphdr);
	//	iov[len].iov_base = (void *)udphdr;
	//	iov[len].iov_len = UDP_HEADER_SIZE;
	//	len += 1;
	//}

    if (m_multiGetReqCnt == 0) {
        m_iovLen = 0;

        m_iov[m_iovLen].iov_base = (void *)"get";
        m_iov[m_iovLen].iov_len = 3;
	    m_iovLen += 1;
    }

	m_iov[m_iovLen].iov_base = (void *)m_key[m_overlapReqCnt];
	m_iov[m_iovLen].iov_len = g_keyLen+1;
	m_iovLen += 1;

    m_multiGetReqCnt += 1;

    if (m_multiGetReqCnt == g_multiGetReqCnt || bForceMultiGetEnd) {

        m_iov[m_iovLen].iov_base = (void *)"\r\n";
        m_iov[m_iovLen].iov_len = 2;
	    m_iovLen += 1;

    	struct msghdr msghdr = { 0 };
    	msghdr.msg_iov = m_iov;
        msghdr.msg_iovlen = m_iovLen;

	    SendMsgTcp(msghdr);

        m_multiGetReqCnt = 0;
    }
}

void CThread::GetReqBinary(int reqNum, bool bMiss, bool bForceMultiGetEnd)
{
	InitKey(reqNum, m_multiGetReqCnt, bMiss);

	// perform get with generated key
	memset(&m_binHdr[m_multiGetReqCnt], 0, sizeof(m_binHdr[m_multiGetReqCnt]));

	m_binHdr[m_multiGetReqCnt].request.magic = PROTOCOL_BINARY_REQ;

	m_binHdr[m_multiGetReqCnt].request.opcode = bMiss ? PROTOCOL_BINARY_CMD_GETQ : PROTOCOL_BINARY_CMD_GET;

	m_binHdr[m_multiGetReqCnt].request.keylen = htons(g_keyLen);
	m_binHdr[m_multiGetReqCnt].request.extlen = 0;
	m_binHdr[m_multiGetReqCnt].request.bodylen = htonl(g_keyLen);
	m_binHdr[m_multiGetReqCnt].request.opaque = (m_tid << 24) | reqNum;
	m_binHdr[m_multiGetReqCnt].request.cas = 0;

	//char udphdr[UDP_HEADER_SIZE];
	//if (m_bUdpEnabled) {
	//	msghdr.msg_name = &m_udpAddr;
	//	msghdr.msg_namelen = m_udpAddrLen;
	//	
	//	CreateUdpHeader(udphdr);
	//	iov[len].iov_base = (void *)udphdr;
	//	iov[len].iov_len = UDP_HEADER_SIZE;
	//	len += 1;
	//}

	m_iov[m_iovLen].iov_base = (void *)&m_binHdr[m_multiGetReqCnt];
	m_iov[m_iovLen].iov_len = sizeof(m_binHdr[m_multiGetReqCnt]);
	m_iovLen += 1;

	m_iov[m_iovLen].iov_base = (void *)m_key[m_multiGetReqCnt];
	m_iov[m_iovLen].iov_len = g_keyLen;
	m_iovLen += 1;

    m_multiGetReqCnt += 1;

    if (m_multiGetReqCnt == g_multiGetReqCnt || bForceMultiGetEnd) {

    	struct msghdr msghdr = { 0 };
    	msghdr.msg_iov = m_iov;
        msghdr.msg_iovlen = m_iovLen;

	    SendMsgTcp(msghdr);

        m_multiGetReqCnt = 0;
        m_iovLen = 0;
    }
}

bool CThread::GetResp(int respNum, bool bForceMultiGetEnd)
{
    if (g_bBinary)
        return GetRespBinary(respNum, bForceMultiGetEnd);
    else
        return GetRespAscii(respNum, bForceMultiGetEnd);
}

bool CThread::GetRespAscii(int respNum, bool bForceMultiGetEnd)
{
    char respBuf[260];
    char checkBuf[32];
    int checkLen;

    RespReadTcp(respBuf, 5);
    if (memcmp(respBuf, "VALUE", 5) != 0)
        return false;

    RespReadTcp(respBuf, g_keyLen+1);
    if (memcmp(respBuf, m_key[m_overlapReqCnt], g_keyLen+1) != 0)
        return false;

	uint32_t flags = (m_tid << 24) | respNum;

    checkLen = snprintf(checkBuf, 32, " %d", flags);
    RespReadTcp(respBuf, checkLen);
    if (memcmp(respBuf, checkBuf, checkLen) != 0)
        return false;

    checkLen = snprintf(checkBuf, 32, " %d\r\n", g_valueLen);
    RespReadTcp(respBuf, checkLen);
    if (memcmp(respBuf, checkBuf, checkLen) != 0)
        return false;

    RespSkipTcp(g_valueLen);

    RespReadTcp(respBuf, 2);
    if (memcmp(respBuf, "\r\n", 2) != 0)
        return false;

    m_multiGetReqCnt += 1;

    if (m_multiGetReqCnt == g_multiGetReqCnt || bForceMultiGetEnd) {
        RespReadTcp(respBuf, 5);
        if (memcmp(respBuf, "END\r\n", 5) != 0)
            return false;

        m_multiGetReqCnt = 0;
    }

    return true;
}

bool CThread::GetRespBinary(int respNum, bool bForceMultiGetEnd)
{
	protocol_binary_response_header respHdr;

	RespReadTcp(&respHdr, sizeof(protocol_binary_response_header));
	assert(respHdr.response.magic == PROTOCOL_BINARY_RES);
	assert(respHdr.response.opcode == PROTOCOL_BINARY_CMD_GET);
	assert(respHdr.response.keylen == 0);
	assert(respHdr.response.datatype == 0);

	if (ntohs(respHdr.response.status) == PROTOCOL_BINARY_RESPONSE_SUCCESS) {
		assert(respHdr.response.extlen == 4);
		uint32_t tmp;
		RespReadTcp(&tmp, 4);
		uint32_t flags = ntohl(tmp);
		if (flags != (uint32_t)((m_tid << 24) | respNum) ||
			respHdr.response.opaque != (uint32_t)((m_tid << 24) | respNum)) {
			printf("Error, rsp flags = %08x, expected %08x\n", flags, ((m_tid << 24) | respNum));
			printf("       rsp opaque= %08x, expected %08x\n", respHdr.response.opaque, ((m_tid << 24) | respNum));
			RespSkipTcp(ntohl(respHdr.response.bodylen) - 4);
			return true;
		}

		InitValue(respNum, 25);
		RespCheckTcp(m_pValue, 25);
		RespSkipTcp(ntohl(respHdr.response.bodylen) - 25 - 4);
		return true;
	} else
		return false;
}

void CThread::UdpSocket()
{
	//----------------------
	// Create a SOCKET for server
	m_udpFd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (m_udpFd < 0) {
        while (__sync_fetch_and_add(&g_errorReport, 1) != 1);

		printf("Error at socket()\n");
		exit(0);
	}

	memset((char *) &m_udpAddr, 0, sizeof(m_udpAddr));
	m_udpAddr.sin_family = AF_INET;
	m_udpAddr.sin_addr.s_addr = inet_addr( "127.0.0.1" );
	m_udpAddr.sin_port = htons(11211);

	m_udpAddrLen = sizeof(m_udpAddr);
}

void CThread::TcpSocket()
{
	//----------------------
	// Create a SOCKET for connecting to server
	m_tcpFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_tcpFd < 0) {
        while (__sync_fetch_and_add(&g_errorReport, 1) != 1);

		printf("Error at socket()\n");
		exit(0);
	}

	// Get the local host information
	hostent * localHost = gethostbyname(g_hostname.c_str());
	char * localIP = inet_ntoa (*(struct in_addr *)*localHost->h_addr_list);

	//----------------------
	// The sockaddr_in structure specifies the address family,
	// IP address, and port of the server to be connected to.
	sockaddr_in clientService; 
	clientService.sin_family = AF_INET;
	clientService.sin_addr.s_addr = inet_addr( localIP );//"127.0.0.1" );
	clientService.sin_port = htons( g_portnum );//11211 );

	//----------------------
	// Connect to server.
	if ( connect( m_tcpFd, (sockaddr *) &clientService, sizeof(clientService) ) < 0) {
        while (__sync_fetch_and_add(&g_errorReport, 1) != 1);

		printf( "Failed to connect.\n" );
		exit(0);
	}

	m_tcpRespBufLen = 0;
}

void CThread::SendMsgTcp(msghdr & msghdr)
{
	do {
		int res = sendmsg(m_tcpFd, &msghdr, 0);

		if (res < 0) {
            while (__sync_fetch_and_add(&g_errorReport, 1) != 1);

			fprintf(stderr, "sendmsg: errno=%d\n", WSAGetLastError());
			exit(1);
		}

		while (msghdr.msg_iovlen > 0 && msghdr.msg_iov->iov_len <= (size_t)res) {
			res -= msghdr.msg_iov->iov_len;
			msghdr.msg_iov += 1;
			msghdr.msg_iovlen -= 1;
		}

		if (res > 0) {
			msghdr.msg_iov->iov_base = (char *)msghdr.msg_iov->iov_base + res;
			msghdr.msg_iov->iov_len -= res;
		}
	} while (msghdr.msg_iovlen > 0);
}

bool CThread::RespCheckTcp(const char *pRespStr, size_t respLen)
{
	// first check what is in buffer
	do  {
		if (m_tcpRespBufLen == 0) {
			m_pTcpRespBuf = m_tcpRespBuf;
			m_tcpRespBufLen = recv(m_tcpFd, m_tcpRespBuf, RESP_BUF_SIZE, 0);
		}

		if (respLen == 0)
			assert(m_tcpRespBufLen == 0);

		else if (m_tcpRespBufLen < 0) {
            while (__sync_fetch_and_add(&g_errorReport, 1) != 1);

            fprintf(stderr, "recv failed\n");
            exit(1);
        }


		size_t checkLen = min((int)respLen, m_tcpRespBufLen);
		if (memcmp(m_pTcpRespBuf, pRespStr, checkLen) != 0) {
			assert(0);
		}

		m_pTcpRespBuf += checkLen;
		m_tcpRespBufLen -= checkLen;

		pRespStr += checkLen;
		respLen -= checkLen;
	} while (respLen > 0);

	return true;
}

void CThread::RespReadTcp(void *pBuf, size_t size)
{
	// first check what is in buffer
	char * pRespStr = (char *)pBuf;
	size_t respLen = size;

	while (respLen > 0) {
		if (m_tcpRespBufLen == 0) {
			m_pTcpRespBuf = m_tcpRespBuf;
			m_tcpRespBufLen = recv(m_tcpFd, m_tcpRespBuf, RESP_BUF_SIZE, 0);
		}
		//int err = errno;
        if (m_tcpRespBufLen < 0) {
            while (__sync_fetch_and_add(&g_errorReport, 1) != 1);

            fprintf(stderr, "recv failed\n");
            exit(1);
        }

		size_t checkLen = min((int)respLen, m_tcpRespBufLen);
		memcpy(pRespStr, m_pTcpRespBuf, checkLen);

		m_pTcpRespBuf += checkLen;
		m_tcpRespBufLen -= checkLen;

		pRespStr += checkLen;
		respLen -= checkLen;
	}
}

void CThread::RespSkipTcp(size_t len)
{
	do {
		if (m_tcpRespBufLen == 0) {
			m_pTcpRespBuf = m_tcpRespBuf;
			m_tcpRespBufLen = recv(m_tcpFd, m_tcpRespBuf, RESP_BUF_SIZE, 0);
		}
        if (m_tcpRespBufLen < 0) {
            while (__sync_fetch_and_add(&g_errorReport, 1) != 1);

            fprintf(stderr, "recv failed\n");
            exit(1);
        }

		if ((size_t)m_tcpRespBufLen > len) {
			m_pTcpRespBuf += len;
			m_tcpRespBufLen -= len;
			len = 0;
		} else {
			len -= m_tcpRespBufLen;
			m_tcpRespBufLen = 0;
		}
	} while (len > 0);
}
