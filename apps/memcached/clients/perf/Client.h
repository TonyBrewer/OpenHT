/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT memcached application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>

#include <pthread.h>

#include <queue>
#include <string>
using namespace std;

#ifdef _WIN32
#include <Winsock2.h>
#include "linux/linux.h"

#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>

#define WSACleanup()
#define WSAGetLastError()	errno
#define closesocket			close
#define INVALID_SOCKET		-1
#define SOCKET_ERROR		-1
#define WSAECONNRESET		-1
typedef int					SOCKET;
typedef struct sockaddr		SOCKADDR;
#endif

#ifdef _WIN32
#ifndef __sync_fetch_and_sub
#define __sync_fetch_and_sub(a, v)	HtAtomicSub(a, v)
inline uint32_t HtAtomicSub(volatile uint32_t * a, uint32_t v) { return InterlockedExchangeSubtract(a, v); }
#endif

#ifndef __sync_fetch_and_add
#define __sync_fetch_and_add(a, v)	HtAtomicAdd(a, v)
inline uint32_t HtAtomicAdd(volatile uint32_t * a, uint32_t v) { return InterlockedExchangeAdd(a, v); }
#endif
#endif // _WIN32

#include "BinaryProtocol.h"

struct CRangeInfo {
    void SetValues(const char *pCmd, const char *pArg);
    void Inc() {
        if (m_bMul)
            m_cur = (m_cur * (100 + m_inc)) / 100;
        else
            m_cur += m_inc;
    }
    int m_cur;
    int m_min;
    int m_max;
    int m_inc;
    bool m_bMul;
};

extern int g_keyStart;
extern int g_valueLen;
extern int g_storeCnt;
extern int g_overlapReqNum;
extern int g_keyLen;
extern string g_hostname;
extern int g_portnum;
extern int g_multiGetReqCnt;
extern bool g_bFlushAll;
extern CRangeInfo g_numThreads;
extern CRangeInfo g_burstDelay;
extern bool g_bBinary;

extern volatile uint32_t g_errorReport;
extern volatile uint32_t g_storeThreads;
extern volatile uint32_t g_remainingThreads;
extern volatile uint32_t g_flushAllThreads;

#define RESP_BUF_SIZE	4096

#define MAX_OVERLAP_GET_CNT   64

#define UDP_HEADER_SIZE	8

struct CUdpHdr {
    CUdpHdr() { m_pad = 0; }
    uint16_t	m_reqId;
    uint16_t	m_seqNum;
    uint16_t	m_pktCnt;
    uint16_t	m_pad;
};

struct CGetInfo {
};

class CThread {
public:
    CThread() {}

    void Test(bool bBinary, bool bUdp);

    void TcpSocket();
    void UdpSocket();

    void CloseSockets();

    void EnableUdp(bool bUdpEnabled) {
        m_bUdpEnabled = bUdpEnabled;
    }
    void EnableBinary(bool bBinaryEnabled) {
        m_bBinaryEnabled = bBinaryEnabled;
    }

    void FlushAll();
    void FlushAllAscii();
    void FlushAllBinary();
    void StoreReq(int reqNum);
    void StoreReqAscii(int reqNum);
    void StoreReqBinary(int reqNum);
    bool StoreResp(int respNum);
    bool StoreRespAscii(int respNum);
    bool StoreRespBinary(int respNum);
    void GetReq(int reqNum, bool bMiss=false, bool bForceMultiGetEnd=false);
    void GetReqAscii(int reqNum, bool bMiss=false, bool bForceMultiGetEnd=false);
    void GetReqBinary(int reqNum, bool bMiss=false, bool bForceMultiGetEnd=false);
    bool GetResp(int respNum, bool bForceMultiGetEnd);
    bool GetRespAscii(int respNum, bool bForceMultiGetEnd);
    bool GetRespBinary(int respNum, bool bForceMultiGetEnd);
    void InitKey(int reqNum, int keyIdx, bool bMiss);
    void InitValue(int reqNum, size_t len=0);

    void RespSkipToEolTcp();

    bool RespCheckTcp(const char *pRespStr, size_t respLen);

    void SendMsgTcp(msghdr & msghdr);

    void RespReadTcp(void * pBuf, size_t size);

    void RespSkipTcp(size_t len);

    void CreateUdpHeader(char * udphdr);
    void ParseUdpHeader(CUdpHdr * pHdr);

    void		SetTid(int tid) { m_tid = tid; }
    void                SetHostName(char *hostName, int len) {strncpy(m_hostName, hostName, len); m_hostNameLen=len;}

    void		CreateThread();
    void		StartThread();
    static void * StartThread_helper(void * arg) {
        CThread *pThread = (CThread  *) arg;
        pThread->StartThread();
        return 0;
    }

    void        PerformContinuousTest();
    void        PerformBurstTest();

public:
    uint64_t volatile	m_getReqCnt;
    uint64_t volatile	m_getRespCnt;
    uint64_t volatile	m_getRespTime;
    uint64_t volatile	m_getRespMinTime;
    uint64_t volatile	m_getRespMaxTime;

private:
    pthread_t			m_threadId;
    int					m_tid;
    char m_hostName[32];
    int m_hostNameLen;
    int					m_storeCnt;
    bool				m_bUdpEnabled;
    bool				m_bBinaryEnabled;
    int					m_udpFd;
    int					m_tcpFd;
    uint16_t			m_udpReqId;
    uint16_t			m_udpSeqNum;
    struct sockaddr_in	m_udpAddr;
    socklen_t			m_udpAddrLen;
    char *				m_pValue;
    uint32_t			m_opaque;

    queue<uint64_t>		m_respQue;

    int					m_tcpRespBufLen;
    char				m_tcpRespBuf[RESP_BUF_SIZE];
    char *				m_pTcpRespBuf;
    ssize_t				m_udpRespBufLen;
    char				m_udpRespBuf[RESP_BUF_SIZE];
    char *				m_pUdpRespBuf;

    int                 m_multiGetReqCnt;
    int                 m_overlapReqCnt;
    int                 m_iovLen;
    struct iovec        m_iov[4 * MAX_OVERLAP_GET_CNT];

	protocol_binary_request_header      m_binHdr[MAX_OVERLAP_GET_CNT];
    char				m_key[MAX_OVERLAP_GET_CNT][258];

};
