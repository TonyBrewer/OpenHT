/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT memcached application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Thread.h"

void CThread::AsciiStore(ECmd cmd, int keyIdx, int flagsIdx, uint64_t value, uint64_t casUnique, ERsp reply)
{
	char buf[25];
	sprintf(buf, "%llu", (unsigned long long)value);
	string str = buf;

	AsciiStore(cmd, keyIdx, flagsIdx, str, casUnique, reply);
}

void CThread::AsciiStore(ECmd cmd, int keyIdx, int flagsIdx, int dataIdx, uint64_t casUnique, ERsp reply)
{
    static string	str("");

    if(args.test == 2) {
	m_asciiStoreIndex = dataIdx;
	AsciiStore(cmd, keyIdx, flagsIdx, str, casUnique, reply);
    } else if(cmd == ePrependCmd) {
	AsciiStore(cmd, keyIdx, flagsIdx, m_prependData[dataIdx], casUnique, reply);
    } else if(cmd == eAppendCmd) {
	AsciiStore(cmd, keyIdx, flagsIdx, m_appendData[dataIdx], casUnique, reply);
    } else {
	AsciiStore(cmd, keyIdx, flagsIdx, m_randData[dataIdx], casUnique, reply);
    }
}

void CThread::AsciiStore(ECmd cmd, int keyIdx, int flagsIdx, string & str, uint64_t casUnique, ERsp reply)
{
    struct msghdr	msghdr = {0};
    struct iovec	iov[6];
    int			dataSize;
    string		respString;
    string		expString;
    string		errMsg;
    char		udphdr[UDP_HEADER_SIZE];

    msghdr.msg_iov = iov;
    size_t & len = msghdr.msg_iovlen = 0;
    if (m_bUdpEnabled) {
	msghdr.msg_name = &m_udpAddr;
	msghdr.msg_namelen = m_udpAddrLen;
		
	CreateUdpHeader(udphdr);
	iov[len].iov_base = udphdr;
	iov[len].iov_len = UDP_HEADER_SIZE;
	len += 1;
    }
    switch (cmd) {
    case eSetCmd:
	iov[len].iov_base = (void *)"set ";
	iov[len].iov_len = 4;
	errMsg = "Ascii Set failed, ";
	break;
    case eAddCmd:
	iov[len].iov_base = (void *)"add ";
	iov[len].iov_len = 4;
	errMsg = "Ascii Add failed, ";
	break;
    case eReplaceCmd:
	iov[len].iov_base = (void *)"replace ";
	iov[len].iov_len = 8;
	errMsg = "Ascii Replace failed, ";
	break;
    case eAppendCmd:
	iov[len].iov_base = (void *)"append ";
	iov[len].iov_len = 7;
	errMsg = "Ascii Append failed, ";
	break;
    case ePrependCmd:
	iov[len].iov_base = (void *)"prepend ";
	iov[len].iov_len = 8;
	errMsg = "Ascii Prepend failed, ";
	break;
    case eCasCmd:
	iov[len].iov_base = (void *)"cas ";
	iov[len].iov_len = 4;
	errMsg = "Ascii Cas failed, ";
	break;
    default:
	assert(0);
    }
    len += 1;

    if(args.test != 2) {
	iov[len].iov_base = (char *)m_randKey[keyIdx].c_str();
	iov[len].iov_len = m_randKey[keyIdx].size();
	dataSize = str.size();
	len += 1;
    } else {			// use perf headers
	iov[len].iov_base = m_perfKeyTbl[keyIdx];
	iov[len].iov_len = args.perfKeyLength;
	dataSize = args.perfDataLength;
	len += 1;
    }
    uint32_t exptime;
    if(reply == eStored || reply == eNoreply) {
	exptime = StartKeyTimeout(keyIdx);
    } else {
	exptime = 0;
    }

    char argS[72];
    if (cmd != eCasCmd) {
	sprintf(argS, " %u %u %d%s\r\n",
	    m_randFlags[flagsIdx], exptime, dataSize,
	    reply == eNoreply ? " noreply" : "");
    } else {
	sprintf(argS, " %u %u %d %llu%s\r\n",
	    m_randFlags[flagsIdx], exptime, dataSize,
	    (unsigned long long)casUnique, reply == eNoreply ? " noreply" : "");
    }

    iov[len].iov_base = argS;
    iov[len].iov_len = strlen(argS);
    len += 1;

    if(args.test != 2) {
	iov[len].iov_base = (char *)str.c_str();
	iov[len].iov_len = str.size();
	len += 1;
    } else {
	iov[len].iov_base = m_perfDataTbl[m_asciiStoreIndex];
	iov[len].iov_len = m_dataHdrLength;
	len += 1;
	iov[len].iov_base = m_perfDataFixed;
	iov[len].iov_len = m_perfDataFixedLength;
	len += 1;
    }

    iov[len].iov_base = (void *)"\r\n";
    iov[len].iov_len = 2;
    len += 1;
    m_gCasState = C_INV;
    if(m_genErr == true) {
	AsciiForceError(keyIdx, msghdr);
	return;
    }
    SendMsg(msghdr);
    if(reply != eNoreply) {
	if(RespGetToEol(respString) == false) {
	    return;
	}
    }

    switch (reply) {
    case eStored:
	expString = "STORED\r\n";
	break;
    case eNotStored:
	expString = "NOT_STORED\r\n";
	break;
    case eNotFound:
	expString = "NOT_FOUND\r\n";
	break;
    case eExists:
	expString = "EXISTS\r\n";
	break;
    case eNoreply:
	break;
    default:
	assert(0);
    }
    if(reply != eNoreply && respString.compare(expString) != 0) {
	ErrorMsg(errMsg, &expString, &respString);
    }
}

uint64_t
CThread::AsciiGet(ECmd cmd, int keyIdx, int flagsIdx, int dataIdx, ERsp reply)
{
    string	dataString("");
    KeyState	*pKeyState;

    if(args.test == 2) {
	m_asciiGetIndex = dataIdx;
	return AsciiGet(cmd, keyIdx, flagsIdx, dataString, reply);
    }
    pKeyState = &m_keyState[keyIdx];
    if(pKeyState->m_state == INVALID) {
	dataString = m_randData[dataIdx];
    } else {
	if(pKeyState->m_preIdx != -1) {
	    dataString += m_prependData[pKeyState->m_preIdx];
	}
	dataString += m_randData[dataIdx];
	if(pKeyState->m_appIdx != -1) {
	    dataString += m_appendData[pKeyState->m_appIdx];
	}
    }
    return AsciiGet(cmd, keyIdx, flagsIdx, dataString, reply);
}

uint64_t
CThread::AsciiGet(ECmd cmd, int keyIdx, int flagsIdx,string &dataStr,ERsp reply)
{
    struct msghdr	msghdr = {0};
    struct iovec	iov[5];
    char		udphdr[UDP_HEADER_SIZE];
    uint64_t		rv = 0;
    string		respString;
    string		expString;
    string		errMsg;
    char		buf[256];
    size_t		pos;
    string		cas;

    msghdr.msg_iov = iov;
    size_t & len = msghdr.msg_iovlen = 0;

    if (m_bUdpEnabled) {
	msghdr.msg_name = &m_udpAddr;
	msghdr.msg_namelen = m_udpAddrLen;
		
	CreateUdpHeader(udphdr);
	iov[len].iov_base = udphdr;
	iov[len].iov_len = UDP_HEADER_SIZE;
	len += 1;
    }

    switch (cmd) {
    case eGetCmd:
	iov[len].iov_base = (void *)"get ";
	iov[len].iov_len = 4;
	errMsg = "Ascii Get failed, ";
	break;
    case eGetsCmd:
	iov[len].iov_base = (void *)"gets ";
	iov[len].iov_len = 5;
	errMsg = "Ascii Gets failed, ";
	break;
    default:
	assert(0);
    }
    len += 1;

    if(args.test != 2) {
	iov[len].iov_base = (char *)m_randKey[keyIdx].c_str();
	iov[len].iov_len = m_randKey[keyIdx].size();
	len += 1;
    } else {			// use perf headers
	iov[len].iov_base = m_perfKeyTbl[keyIdx];
	iov[len].iov_len = args.perfKeyLength;
	len += 1;
    }

    iov[len].iov_base = (void *)"\r\n";
    iov[len].iov_len = 2;
    len += 1;
    if(m_genErr == true) {
	AsciiForceError(keyIdx, msghdr);
	return(0);
    }
    m_gCasState = C_INV;
    SendMsg(msghdr);
    if(RespGetToEol(respString) == false) {
	return(0);
    }

    switch (reply) {
	case eGetHit:
	    expString = "VALUE ";
	    if(args.test != 2) {
		expString += m_randKey[keyIdx];
	    } else {
		expString += m_perfKeyTbl[keyIdx];
	    }
	    sprintf(buf, " %d ", m_randFlags[flagsIdx]);
	    expString += buf;
	    if(args.test != 2) {
		sprintf(buf, "%u", (unsigned int)dataStr.size());
	    } else {
		sprintf(buf, "%u", args.perfDataLength);
	    }
	    expString += buf;
	    if (cmd == eGetsCmd) {
		// the value is known only to the server
		// we must get it from the message
		pos = respString.find_last_of(" ");
		expString += " ";
		m_gCasState = C_VAL;
		cas = respString.substr(pos+1, respString.length() - pos-1 - 2);
		expString += cas;
		rv = RespGetUint64(cas);
		m_gCas = rv;
	    }
	    expString += "\r\n";
	    if(respString.compare(expString) != 0) {
		ErrorMsg(errMsg, &expString, &respString);
	    }
	    //
	    //	second line of reply
	    //
	    if(RespGetToEol(respString) == false) {
		return(0);
	    }
	    if(args.test != 2) {
		expString = dataStr;
	    } else {
		expString = m_perfDataTbl[m_asciiGetIndex];
		expString += m_perfDataFixed;
	    }
	    expString += "\r\n";
	    if(respString.compare(expString) != 0) {
		ErrorMsg(errMsg, &expString, &respString);
	    }
	    //
	    //	third line of reply, read and fall into end
	    //
	    if(RespGetToEol(respString) == false) {
		return(0);
	    }
	case eGetMiss:
	    expString = "END\r\n";
	    break;
	default:
	    assert(0);
    }
    if(respString.compare(expString) != 0) {
	ErrorMsg(errMsg, &expString, &respString);
    }
    return rv;
}

void
CThread::AsciiMultiGet(vector<GetQ> &getQvect)
{
    struct msghdr	msghdr = {0};
    struct iovec	iov[25];
    char		udphdr[UDP_HEADER_SIZE];
    char                ws = ' ';
    size_t               i;

    msghdr.msg_iov = iov;
    size_t & len = msghdr.msg_iovlen = 0;

    if (m_bUdpEnabled) {
	msghdr.msg_name = &m_udpAddr;
	msghdr.msg_namelen = m_udpAddrLen;
		
	CreateUdpHeader(udphdr);
	iov[len].iov_base = udphdr;
	iov[len].iov_len = UDP_HEADER_SIZE;
	len += 1;
    }
    iov[len].iov_base = (void *)"get ";
    iov[len].iov_len = 4;
    len += 1;

    for(i=0; i<getQvect.size(); i++) {
        GetQ &gq = getQvect[i];

        iov[len].iov_base = (char *)m_randKey[gq.cmdKey].c_str();
        iov[len].iov_len = m_randKey[gq.cmdKey].size();
	len += 1;
        iov[len].iov_base = &ws;
        iov[len].iov_len = 1;
        len += 1;
    }
    iov[len].iov_base = (void *)"\r\n";
    iov[len].iov_len = 2;
    len += 1;
    SendMsg(msghdr);
}
//
//  NOTE: a leyIdx of -1 is expect END\r\n
//
uint64_t
CThread::AsciiGetReply(int keyIdx, int flagsIdx, int dataIdx, ERsp reply)
{
    string      respString;
    string      expString;
    KeyState    *pKeyState;
    string      dataString("");
    char        buf[256];

    if(reply == eNoreply && keyIdx != -1) {
        return(0);                          // invalid key
    }
    if(RespGetToEol(respString) == false) {
	return(0);
    }
    if(keyIdx == -1) {
        expString = "END\r\n";
    } else {
        pKeyState = &m_keyState[keyIdx];
        if(pKeyState->m_state == INVALID) {
            dataString = m_randData[dataIdx];
        } else {
            if(pKeyState->m_preIdx != -1) {
                dataString += m_prependData[pKeyState->m_preIdx];
            }
            dataString += m_randData[dataIdx];
            if(pKeyState->m_appIdx != -1) {
                dataString += m_appendData[pKeyState->m_appIdx];
            }
        }
        switch (reply) {
	    case eGetHit:
	        expString = "VALUE ";
	        expString += m_randKey[keyIdx];
	        sprintf(buf, " %d ", m_randFlags[flagsIdx]);
	        expString += buf;
	        sprintf(buf, "%u", (unsigned int)dataString.size());
	        expString += buf;
	        expString += "\r\n";
	        if(respString.compare(expString) != 0) {
		    ErrorMsg("Ascii get k,k,...", &expString, &respString);
	        }
	        //
	        //	second line of reply
	        //
	        if(RespGetToEol(respString) == false) {
		    return(0);
	        }
                expString = dataString;
	        expString += "\r\n";
	        if(respString.compare(expString) != 0) {
		    ErrorMsg("Ascii get k,k,...", &expString, &respString);
	        }
                break;
	    case eNoreply:                  // this key was not valid
                expString = "";
	        break;
	    default:
	        assert(0);
        }
    }
    if(expString.length() != 0) {
        if(respString.compare(expString) != 0) {
	    ErrorMsg("Ascii get k,k,...", &expString, &respString);
        }
    }
    return(0);
}

void CThread::AsciiDelete(int keyIdx, ERsp reply)
{
    struct msghdr   msghdr = {0};
    struct	    iovec iov[5];
    string	    respString;
    string	    expString;
    char	    udphdr[UDP_HEADER_SIZE];

    msghdr.msg_iov = iov;
    size_t & len = msghdr.msg_iovlen = 0;

    if (m_bUdpEnabled) {
	msghdr.msg_name = &m_udpAddr;
	msghdr.msg_namelen = m_udpAddrLen;
		
	CreateUdpHeader(udphdr);
	iov[len].iov_base = udphdr;
	iov[len].iov_len = UDP_HEADER_SIZE;
	len += 1;
    }
    iov[len].iov_base = (void *)"delete ";
    iov[len++].iov_len = 7;

    iov[len].iov_base = (char *)m_randKey[keyIdx].c_str();
    iov[len++].iov_len = m_randKey[keyIdx].size();

    if (reply == eNoreply) {
	iov[len].iov_base = (void *)" noreply\r\n";
	iov[len++].iov_len = 10;
    } else {
	iov[len].iov_base = (void *)"\r\n";
	iov[len++].iov_len = 2;
    }
    if(m_genErr == true) {
	AsciiForceError(keyIdx, msghdr);
	return;
    }
    SendMsg(msghdr);
    if(reply != eNoreply) {
	if(RespGetToEol(respString) == false) {
	    return;
	}
    }

    switch (reply) {
    case eDeleted:
	expString = "DELETED\r\n";
	break;
    case eNotFound:
	expString = "NOT_FOUND\r\n";
	break;
    case eNoreply:
	break;
    default:
	assert(0);
    }
    if(reply != eNoreply && respString.compare(expString) != 0) {
	ErrorMsg("Ascii Delete failed, ", &expString, &respString);
    }
}

void
CThread::AsciiArith(ECmd cmd, int keyIdx, uint64_t inV, uint64_t outV,
			uint64_t expTmr, ERsp reply)
{
    struct msghdr   msghdr = {0};
    struct iovec    iov[5];
    string	    respString;
    string	    expString;
    string	    errString;
    char	    udphdr[UDP_HEADER_SIZE];

    msghdr.msg_iov = iov;
    size_t & len = msghdr.msg_iovlen = 0;
    if (m_bUdpEnabled) {
	msghdr.msg_name = &m_udpAddr;
	msghdr.msg_namelen = m_udpAddrLen;
		
	CreateUdpHeader(udphdr);
	iov[len].iov_base = udphdr;
	iov[len].iov_len = UDP_HEADER_SIZE;
	len += 1;
    }
    switch (cmd) {
    case eIncrCmd:
	iov[len].iov_base = (void *)"incr ";
	iov[len++].iov_len = 5;
	errString = "Ascii Incr failed, ";
	break;
    case eDecrCmd:
	iov[len].iov_base = (void *)"decr ";
	iov[len++].iov_len = 5;
	errString = "Ascii Decr failed, ";
	break;
    default:
	assert(0);
    }

    iov[len].iov_base = (char *)m_randKey[keyIdx].c_str();
    iov[len++].iov_len = m_randKey[keyIdx].size();

    char buf[25];
    sprintf(buf, " %llu", (unsigned long long)inV);

    iov[len].iov_base = buf;
    iov[len++].iov_len = strlen(buf);

    if (reply == eNoreply) {
	iov[len].iov_base = (void *)" noreply\r\n";
	iov[len++].iov_len = 10;
    } else {
	iov[len].iov_base = (void *)"\r\n";
	iov[len++].iov_len = 2;
    }
    if(m_genErr == true) {
	AsciiForceError(keyIdx, msghdr);
	return;
    }
    SendMsg(msghdr);
    if(reply != eNoreply) {
	if(RespGetToEol(respString) == false) {
	    return;
	}
    }

    switch (reply) {
	case eValue:
	    sprintf(buf, "%llu\r\n", (unsigned long long)outV);
	    expString = buf;
	    break;
	case eNotFound:
	    expString = "NOT_FOUND\r\n";
	    break;
	case eNonNumeric:
	    expString = "CLIENT_ERROR cannot increment or decrement "
			"non-numeric value\r\n";
	    break;
	case eNoreply:
	    break;
	default:
	    assert(0);
    }
    if(reply != eNoreply && respString.compare(expString) != 0) {
	ErrorMsg(errString, &expString, &respString);
    }
}

void CThread::AsciiTouch(int keyIdx, ERsp reply)
{
    struct msghdr   msghdr = {0};
    struct iovec    iov[5];
    string	    respString;
    string	    expString;
    char	    udphdr[UDP_HEADER_SIZE];

    msghdr.msg_iov = iov;
    size_t & len = msghdr.msg_iovlen = 0;
    if (m_bUdpEnabled) {
	msghdr.msg_name = &m_udpAddr;
	msghdr.msg_namelen = m_udpAddrLen;
		
	CreateUdpHeader(udphdr);
	iov[len].iov_base = udphdr;
	iov[len].iov_len = UDP_HEADER_SIZE;
	len += 1;
    }
    iov[len].iov_base = (void *)"touch ";
    iov[len++].iov_len = 6;

    iov[len].iov_base = (char *)m_randKey[keyIdx].c_str();
    iov[len++].iov_len = m_randKey[keyIdx].size();

    uint32_t exptime;
    if(reply == eTouched || reply == eNoreply) {
	exptime = StartKeyTimeout(keyIdx);
    } else {
	exptime = 0;
    }
    char Sbuf[256];
    sprintf(Sbuf, " %u%s\r\n", exptime, (reply == eNoreply) ? " noreply" : "");
    iov[len].iov_base = (void *)Sbuf;
    iov[len++].iov_len = strlen(Sbuf);

    if(m_genErr == true) {
	AsciiForceError(keyIdx, msghdr);
	return;
    }
    SendMsg(msghdr);
    if(RespGetToEol(respString) == false) {
	return;
    }

    switch (reply) {
	case eTouched:
	    expString = "TOUCHED\r\n";
	    break;
	case eNotFound:
	    expString = "NOT_FOUND\r\n";
	    break;
	case eNoreply:
	    expString = "";
	    break;
	default:
	    assert(0);
    }
    if(respString.compare(expString) != 0) {
	ErrorMsg("Ascii Touch failed, ", &expString, &respString);
    }
}

void CThread::AsciiQuit(ERsp reply)
{
    struct msghdr   msghdr = {0};
    struct iovec    iov[5];
    char	    udphdr[UDP_HEADER_SIZE];

    msghdr.msg_iov = iov;
    size_t & len = msghdr.msg_iovlen = 0;
    if (m_bUdpEnabled) {
	msghdr.msg_name = &m_udpAddr;
	msghdr.msg_namelen = m_udpAddrLen;
		
	CreateUdpHeader(udphdr);
	iov[len].iov_base = udphdr;
	iov[len].iov_len = UDP_HEADER_SIZE;
	len += 1;
    }
    iov[len].iov_base = (void *)"quit\r\n";
    iov[len++].iov_len = 6;

    SendMsg(msghdr);

    RespCheckTcp("");
}

void CThread::AsciiVersion()
{
    struct msghdr   msghdr = {0};
    struct iovec    iov[5];
    string	    respString;
    char	    udphdr[UDP_HEADER_SIZE];

    msghdr.msg_iov = iov;
    size_t & len = msghdr.msg_iovlen = 0;
    if (m_bUdpEnabled) {
	msghdr.msg_name = &m_udpAddr;
	msghdr.msg_namelen = m_udpAddrLen;
		
	CreateUdpHeader(udphdr);
	iov[len].iov_base = udphdr;
	iov[len].iov_len = UDP_HEADER_SIZE;
	len += 1;
    }
    iov[len].iov_base = (void *)"version\r\n";
    iov[len++].iov_len = 9;
    if(m_genErr == true) {
	AsciiForceError(0, msghdr);
	return;
    }
    SendMsg(msghdr);

    if(RespGetToEol(respString) == false) {
	return;
    }
    if(respString.compare("VERSION Convey 1.4.15\r\n") != 0
		    && respString.compare("VERSION 1.4.15\r\n") != 0) {
	ErrorMsg("Ascii Version failed, ", 0, &respString);
    }
}

void CThread::AsciiFlushAll()
{
    struct msghdr   msghdr = {0};
    struct iovec    iov[5];
    string	    respString;
    string	    expString = "OK\r\n";
    char	    udphdr[UDP_HEADER_SIZE];

    msghdr.msg_iov = iov;
    size_t & len = msghdr.msg_iovlen = 0;
    if (m_bUdpEnabled) {
	msghdr.msg_name = &m_udpAddr;
	msghdr.msg_namelen = m_udpAddrLen;
		
	CreateUdpHeader(udphdr);
	iov[len].iov_base = udphdr;
	iov[len].iov_len = UDP_HEADER_SIZE;
	len += 1;
    }

    iov[len].iov_base = (void *)"flush_all\r\n";
    iov[len++].iov_len = 11;

    SendMsg(msghdr);
    if(RespGetToEol(respString) == false) {
	return;
    }
    if(respString.compare(expString) != 0) {
	ErrorMsg("Ascii flush failed, ", &expString, &respString);
    }
}

void
CThread::ErrorMsg(string msg, string *expected, string *got)
{
    size_t	pos;

    if(expected) {
	while(1) {
	    pos = expected->find_first_of("\n\r");
	    if(pos == string::npos) {
		break;
	    }
	    expected->erase(pos, 1);
	}
    }
    if(got) {
	while(1) {
	    pos = got->find_first_of("\n\r");
	    if(pos == string::npos) {
		break;
	    }
	    got->erase(pos, 1);
	}
    }
    if(expected == 0) {
	if(got == 0) {
	    printf("\n%2d %s\n", m_tid, msg.c_str());
	} else {
	    printf("\n%2d %s  got <%s>\n",
		m_tid, msg.c_str(), got->c_str());
	}
    } else if(got == 0) {
	printf("\n%2d %s  expected <%s>\n",
	    m_tid, msg.c_str(), expected->c_str());
    } else {
	printf("\n%2d %s  expected <%s>  got  <%s>\n",
	    m_tid, msg.c_str(), expected->c_str(), got->c_str());
    }
    threadFails++;
    if(args.stop != 0) {
	m_pauseThreads = true;
	 printf("%s test  seed %u   (thread seed %u) (tstate 0x%x)\n",
		(args.test == 0) ? "rand" : "dir", args.seed,
		args.seed + m_tid, m_tState);
	printf("Hit enter to continue  or ^C to exit ");
	fflush(stdout);
	getc(stdin);
	m_pauseThreads = false;
    }
}

void CThread::AsciiNoop()
{
	// Command doesn't exist for ASCII protocol
}

void CThread::AsciiStats(const char * pSubStat, bool printData, const char *hdr)
{
    struct msghdr   msghdr = {0};
    struct iovec    iov[5];
    string	    statString;
    char	    udphdr[UDP_HEADER_SIZE];
    string	    expHdr;

    expHdr = (hdr != 0) ? hdr : "STAT ";
    msghdr.msg_iov = iov;
    size_t & len = msghdr.msg_iovlen = 0;
    if (m_bUdpEnabled) {
	msghdr.msg_name = &m_udpAddr;
	msghdr.msg_namelen = m_udpAddrLen;
		
	CreateUdpHeader(udphdr);
	iov[len].iov_base = udphdr;
	iov[len].iov_len = UDP_HEADER_SIZE;
	len += 1;
    }
    iov[len].iov_base = (void *)"stats";
    iov[len++].iov_len = 5;

    if (pSubStat) {
	iov[len].iov_base = (void *)"  ";
	iov[len++].iov_len = 2;

	iov[len].iov_base = (void *)pSubStat;
	iov[len++].iov_len = strlen(pSubStat);
    }

    iov[len].iov_base = (void *)"\r\n";
    iov[len++].iov_len = 2;
    if(m_genErr == true) {
	AsciiForceError(0, msghdr);
	return;
    }
    SendMsg(msghdr);

    while(1) {
	if(RespGetToEol(statString) == false) {
	    return;
	}
	if(printData == true) {
	    printf("%s", statString.c_str());
	}
	if(statString.compare(0, expHdr.length(), expHdr) == 0) {
	    continue;
	}
	if(statString.compare("END\r\n") == 0) {
	    break;
	}
	if(statString.compare("OK\r\n") == 0) {	    // a stats command (detail on)
	    break;
	}
	if(statString.compare("RESET\r\n") == 0) {
	    break;
	}
	printf("Unexpected response to Stats command   got <%s>\n",
	    statString.c_str());
	break;
    }
}

void
CThread::AsciiForceError(int key, msghdr &msghdr)
{
    ErrType			    et;
    bool			    expDropConn;
    string			    expText;
    string			    gotText;
    Action			    act;
    struct iovec		    *iovP;
    char			    cp0[256];	    // iov0 replacement buffer
    bool			    doubleRx;
 
    //
    // any command that has data following the header should not be tested for aUnkCmd
    // the server reads the header, replies (24 bytes) and then reads expecting a header
    // but normally gets an invalid magic number.
    //
    //	the data type test is ignored because the server ignores the data type
    //
    static const Action action[eCmdCnt][eErrCnt] = {			// must match ECmd order
	// magic	cmd	    key
	{ aSkip,	aError2,    aSkip,	aSkip },		// set cmd
	{ aSkip,	aError2,    aSkip,	aSkip },		// add cmd
	{ aSkip,	aError2,    aSkip,	aSkip },		// rep cmd
	{ aSkip,	aError2,    aSkip,	aSkip },		// app cmd
	{ aSkip,	aError2,    aSkip,	aSkip },		// pre cmd
	{ aSkip,	aError2,    aSkip,	aSkip },		// cas cmd
	{ aSkip,	aError,	    aSkip,	aSkip },		// get cmd
	{ aSkip,	aSkip,	    aSkip,	aSkip },		// gets cmd
	{ aSkip,	aSkip,	    aSkip,	aSkip },		// getk cmd
	{ aSkip,	aSkip,	    aSkip,	aSkip },		// getr cmd
        { aSkip,	aError,	    aSkip,	aSkip },		// gat cmd
	{ aSkip,	aError,	    aSkip,	aSkip },		// inc cmd
	{ aSkip,	aError,	    aSkip,	aSkip },		// dec cmd
	{ aSkip,	aError,	    aSkip,	aSkip },		// del cmd
	{ aSkip,	aError,	    aSkip,	aSkip },		// touch cmd
	{ aSkip,	aError,	    aSkip,	aSkip },		// stat cmd
	{ aSkip,	aError,	    aSkip,	aSkip },		// ver cmd
	{ aSkip,	aSkip,	    aSkip,	aSkip },		// nop cmd

    };

    expText = "";
    do {
	et = (ErrType)(m_rnd() % eErrCnt);
    } while(et == eMagic || et == eDataType || et == eKeyLen);			    // no magic in ascii
    iovP = msghdr.msg_iov;
    memcpy(cp0, iovP[0].iov_base, iovP[0].iov_len);
    iovP[0].iov_base = cp0;
    act = action[m_cmdCmd][et];
    if(m_trace2) {
        printf("command %s   error %s   action %s\n",
	    cmdStr[m_cmdCmd], errStr[et], actStr[act]);
    }
    switch(et) {		// do the actual error
	case eMagic:
	    assert(0);
	    break;
	case eCmd:
	    cp0[0] = 'X';		    // no command starts with X
	    break;
	case eKeyLen:
	    iovP[1].iov_base = 0;	    // remove key
	    iovP[1].iov_len = 0;
	    break;
	case eDataType:			 // ascii has no datatype
	    break;

	default:
	    return;
	    assert(0);
    }
    expDropConn = false;
    doubleRx = false;
    expText = "";
    switch(act) {
	case aTAS:			// test and stop
	    break;
	case aSkip:
	    if(m_trace2) {
		printf("\n");
	    }
	    return;			// legal case
	case aError:
	    expText = "ERROR\r\n";
	    break;
	case aError2:
	    expText = "ERROR\r\n";
	    doubleRx = true;
	    break;
	case aDropConn:
	    expDropConn = true;
	    break;
	case aUnkCmd:
	    expText = "Unknown command";
	    break;
	case aInvArg:
	    expText = "Invalid arguments";
	    expDropConn = true;
	    break;
	case aNotFound:
	    expText = "Not found";
	    expDropConn = true;
	    break;
	default:
	    assert(0);
    }

    if(m_trace2) {
	printf("forcing %s error  ", errStr[et]);
    }
    SendMsg(msghdr);
    if(act == aTAS) {
	printf("\nTAS  Hit enter to continue  or ^C to exit ");
	fflush(stdout);
	getc(stdin);
    }
    if(expText.length() != 0) {
	if(m_trace2) {
	    printf("expText  ");
	}
	m_tcpRespBufLen = recv(m_tcpFd, m_tcpRespBuf, expText.length(), 0);
	if(m_trace2) {
	    printf("recieved %ld bytes   ", m_tcpRespBufLen);
	}
    }
    if(expDropConn == true) {		// expect connection to drop
	fd_set  read;
	timeval tv = {0,100000};		// wait 1 sec
	FD_ZERO(&read);
	FD_SET(m_tcpFd, &read);
	int sel = select(m_tcpFd + 1, &read, 0, 0, &tv);
	if(m_trace2) {
	    printf("sel %d ", sel);
	}
	if(sel > 0) {
	    m_tcpRespBufLen = recv(m_tcpFd, m_tcpRespBuf, RESP_BUF_SIZE, 0);
	    if(m_trace2) {
		printf("recieved %ld bytes   ", m_tcpRespBufLen);
	    }
	}
        closesocket(m_tcpFd);
	TcpSocket();		    // reopen
    }

    if(expText.length() != 0) {
	m_tcpRespBuf[m_tcpRespBufLen] = 0;
	if(expText.compare(m_tcpRespBuf) != 0) {
	    gotText = m_tcpRespBuf;
	    ErrorMsg("Error test failed, ", &expText, &gotText);
	}
    }
    if(doubleRx) {
	m_tcpRespBufLen = recv(m_tcpFd, m_tcpRespBuf, RESP_BUF_SIZE, 0);
    }
    if(1) {		// possible connection to drop
	fd_set  read;
	timeval tv = {0,10000};		// wait
	FD_ZERO(&read);
	FD_SET(m_tcpFd, &read);
	int sel = select(m_tcpFd + 1, &read, 0, 0, &tv);
	m_tcpRespBufLen = 1;
	if(sel > 0) {
	    m_tcpRespBufLen = recv(m_tcpFd, m_tcpRespBuf, RESP_BUF_SIZE, 0);
	    if(m_trace2) {
		printf("recieved %ld bytes   ", m_tcpRespBufLen);
	    }
	}
	if(sel < 0 || m_tcpRespBufLen == 0 || (int)m_tcpRespBufLen == -1) {
	    closesocket(m_tcpFd);
	    TcpSocket();		    // reopen
	}
    }
    if(m_trace2) {
	printf("\n\n");
    }
    m_tcpRespBufLen = 0;
}
