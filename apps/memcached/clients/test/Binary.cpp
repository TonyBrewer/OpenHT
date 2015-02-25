/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT memcached application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
//#include "Client.h"

#include "Thread.h"

uint64_t mc_swap64(uint64_t in) {
    /* Little endian, flip the bytes around until someone makes a faster/better
    * way to do this. */
    int64_t rv = 0;
    int i = 0;
     for(i = 0; i<8; i++) {
        rv = (rv << 8) | (in & 0xff);
        in >>= 8;
     }
    return rv;
}

uint64_t ntohll(uint64_t val) {
   return mc_swap64(val);
}

uint64_t htonll(uint64_t val) {
   return mc_swap64(val);
}

void
CThread::BinaryStore(ECmd cmd, int keyIdx, int flagsIdx, uint64_t value,
			uint64_t casValue, ERsp reply)
{
    char	buf[25];

    sprintf(buf, "%llu", (unsigned long long)value);
    string str = buf;

    BinaryStore(cmd, keyIdx, flagsIdx, str, casValue, reply);
}

void
CThread::BinaryStore(ECmd cmd, int keyIdx, int flagsIdx, int dataIdx,
			uint64_t casValue, ERsp reply)
{
    string	str;

    if(args.test == 2) {
	m_asciiStoreIndex = dataIdx;
	BinaryStore(cmd, keyIdx, flagsIdx, str, casValue, reply);
    } else if(cmd == ePrependCmd) {
	BinaryStore(cmd, keyIdx, flagsIdx, m_prependData[dataIdx],
				casValue, reply);
    } else if(cmd == eAppendCmd) {
	BinaryStore(cmd, keyIdx, flagsIdx, m_appendData[dataIdx],
				casValue, reply);
    } else {
	BinaryStore(cmd, keyIdx, flagsIdx, m_randData[dataIdx], casValue,reply);
    }
}

void
CThread::BinaryStore(ECmd cmd, int keyIdx, int flagsIdx, string & str,
			uint64_t casValue, ERsp reply)
{
    protocol_binary_request_header	binHdr;
    const char				*pErrStr;
    char				msgStr[128];
    const char				*instName;

    memset(&binHdr, 0, sizeof(binHdr));
    binHdr.request.magic = PROTOCOL_BINARY_REQ;

    switch (cmd) {
    case eSetCmd:
	binHdr.request.opcode = reply == eNoreply
			? PROTOCOL_BINARY_CMD_SETQ : PROTOCOL_BINARY_CMD_SET;
	binHdr.request.extlen = 8;
	instName = "Set ";
	break;
    case eAddCmd:
	binHdr.request.opcode = reply == eNoreply
			? PROTOCOL_BINARY_CMD_ADDQ : PROTOCOL_BINARY_CMD_ADD;
	binHdr.request.extlen = 8;
	instName = "Add ";
	break;
    case eReplaceCmd:
	binHdr.request.opcode = reply == eNoreply
		? PROTOCOL_BINARY_CMD_REPLACEQ : PROTOCOL_BINARY_CMD_REPLACE;
	binHdr.request.extlen = 8;
	instName = "Replace ";
	break;
    case eAppendCmd:
	binHdr.request.opcode = reply == eNoreply
		? PROTOCOL_BINARY_CMD_APPENDQ : PROTOCOL_BINARY_CMD_APPEND;
	binHdr.request.extlen = 0;
	instName = "Append ";
	break;
    case ePrependCmd:
	binHdr.request.opcode = reply == eNoreply
		? PROTOCOL_BINARY_CMD_PREPENDQ : PROTOCOL_BINARY_CMD_PREPEND;
	binHdr.request.extlen = 0;
	instName = "Prepend ";
	break;
    case eCasCmd:
	binHdr.request.opcode = reply == eNoreply
		? PROTOCOL_BINARY_CMD_SETQ : PROTOCOL_BINARY_CMD_SET;
	binHdr.request.extlen = 8;
	instName = "Set ";
	break;
    default:
	assert(0);
    }

    int	dataSize;
    int	keySize;
    if(args.test != 2) {
	dataSize = str.size();
	keySize = m_randKey[keyIdx].size();
    } else {
	dataSize = args.perfDataLength;
	keySize = args.perfKeyLength;
    }
    binHdr.request.keylen = htons(keySize);
    binHdr.request.bodylen = htonl(binHdr.request.extlen + keySize + dataSize);
    binHdr.request.opaque = htonl(m_opaque);
    binHdr.request.cas = htonll(casValue);

    struct msghdr msghdr = { 0 };
    struct iovec iov[6];

    msghdr.msg_iov = iov;
    size_t & len = msghdr.msg_iovlen = 0;
    char udphdr[UDP_HEADER_SIZE];
    if (m_bUdpEnabled) {
	msghdr.msg_name = &m_udpAddr;
	msghdr.msg_namelen = m_udpAddrLen;
		
	CreateUdpHeader(udphdr);
	iov[len].iov_base = (void *)udphdr;
	iov[len].iov_len = UDP_HEADER_SIZE;
	len += 1;
    }

    iov[len].iov_base = (void *)&binHdr;
    iov[len].iov_len = sizeof(binHdr);
    len += 1;

    uint32_t flags;
    uint32_t exptime;
    if (binHdr.request.extlen == 8) {
	flags = htonl(m_randFlags[flagsIdx]);
	iov[len].iov_base = (void *)&flags;
	iov[len].iov_len = 4;
	len += 1;

	if(reply == eStored || reply == eNoreply) {
	    exptime = StartKeyTimeout(keyIdx);
	} else {
	    exptime = 0;
	}
	exptime = htonl(exptime);
	iov[len].iov_base = (void *)&exptime;
	iov[len].iov_len = 4;
	len += 1;
    }

    if(args.test != 2) {
	iov[len].iov_base = (char *)m_randKey[keyIdx].c_str();
	iov[len].iov_len = m_randKey[keyIdx].size();
	len += 1;
    } else {                        // use perf headers
	iov[len].iov_base = m_perfKeyTbl[keyIdx];
	iov[len].iov_len = args.perfKeyLength;
	len += 1;
    }

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
    if(m_genErr == true) {
	BinaryForceError(keyIdx, msghdr);
	return;
    }

    SendMsg(msghdr);

    protocol_binary_response_header respHdr, respHdrE;
    respHdrE.response.magic = PROTOCOL_BINARY_RES;
    respHdrE.response.opcode = binHdr.request.opcode;
    respHdrE.response.keylen = 0;
    respHdrE.response.extlen = 0;
    respHdrE.response.datatype = 0;
    respHdrE.response.status = ntohs(PROTOCOL_BINARY_RESPONSE_SUCCESS);
    respHdrE.response.bodylen = htonl(0);
    respHdrE.response.opaque = binHdr.request.opaque;
    respHdrE.response.cas = binHdr.request.cas;

    if(reply == eNoreply) {
	m_gCasState = C_INV;		// value is unknown
	return;
    }
    if(BinaryRespRead(&respHdr,sizeof(protocol_binary_response_header))==false){
	m_gCasState = C_INV;
	return;
    }
    //
    //	The cas is not checked, the reply reflects the cas 
    //
    SkipPbrh skip(SkipPbrh::CAS);

    switch (reply) {
    case eStored:
	m_gCasState = C_VAL;
	m_gCas = ntohll(respHdr.response.cas);
	CheckHdrOk(instName, respHdr, respHdrE, skip);
	break;
    case eNotStored:
    case eExists:
    case eNotFound:
	m_gCasState = C_INV;
	m_gCas = ntohll(respHdr.response.cas);

	if(reply == eNotStored) {
	    pErrStr = "Not stored.";
	    respHdrE.response.status=ntohs(PROTOCOL_BINARY_RESPONSE_NOT_STORED);
	} else if(reply == eExists) {
	    pErrStr = "Data exists for key.";
	   respHdrE.response.status=ntohs(PROTOCOL_BINARY_RESPONSE_KEY_EEXISTS);
	} else if(reply == eNotFound) {
	    pErrStr = "Not found";
	    respHdrE.response.status=ntohs(PROTOCOL_BINARY_RESPONSE_KEY_ENOENT);
	} else {
	    assert(0);
	}
	respHdrE.response.bodylen = htonl(strlen(pErrStr));
	CheckHdrOk(instName, respHdr, respHdrE, skip);
	if(BinaryRespRead(msgStr, strlen(pErrStr)) == false) {
	    return;
	}
	CheckDataOk(instName, (uint8_t *)msgStr, (uint8_t *)pErrStr,
		strlen(pErrStr), true);
	break;
    case eNoreply:
	break;
    default:
	assert(0);
    }
}

uint64_t
CThread::BinaryGet(ECmd cmd, int keyIdx, int flagsIdx, int dataIdx, ERsp reply,
			uint64_t casUnique)
{
    string          dataString("");
    KeyState        *pKeyState;

    if(args.test == 2) {
        m_binaryGetIndex = dataIdx;
        return BinaryGet(cmd, keyIdx, flagsIdx, dataString, reply, casUnique);
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
    return BinaryGet(cmd, keyIdx, flagsIdx, dataString, reply, casUnique);
}

uint64_t
CThread::BinaryGetReply(int keyIdx, int flagsIdx, int dataIdx, ERsp reply)
{
    int			    respLength;
    const char		    *pErrStr;
    char		    msgStr[256];
    uint64_t		    rtn = 0;
    string		    dataString("");
    KeyState		    *pKeyState;

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

    respLength = dataString.size();
 
    protocol_binary_response_header respHdr, respHdrE;
    static SkipPbrh skip(SkipPbrh::CAS, SkipPbrh::OPQ);
    respHdrE.response.magic = PROTOCOL_BINARY_RES;
    respHdrE.response.opcode = PROTOCOL_BINARY_CMD_GETQ;
    respHdrE.response.keylen = 0;
    respHdrE.response.extlen = 0;
    respHdrE.response.datatype = 0;
    respHdrE.response.status = ntohs(PROTOCOL_BINARY_RESPONSE_SUCCESS);
    respHdrE.response.bodylen = htonl(0);
    respHdrE.response.opaque = htonl(0xdeadbeef);	// not testes here
    if(reply != eNoreply) {
	if(BinaryRespRead(&respHdr, sizeof(protocol_binary_response_header)) == false) {
	    return(0);
	}
    }
    switch (reply) {
    case eGetHit:
	respHdrE.response.extlen = 4;			// flags returned
	respHdrE.response.bodylen = htonl(respLength + 4);
	CheckHdrOk("Get ", respHdr, respHdrE, skip);
	RespCheck(htonl(m_randFlags[flagsIdx]), (size_t)4);
	RespCheck(dataString);
	rtn = ntohll(respHdr.response.cas);
	break;
    case eGetMiss:
    case eNotFound:
	pErrStr = "Not found";
	respHdrE.response.bodylen = htonl(strlen(pErrStr));
	respHdrE.response.status = ntohs(PROTOCOL_BINARY_RESPONSE_KEY_ENOENT);
	CheckHdrOk("Get ", respHdr, respHdrE, skip);
	if(BinaryRespRead(msgStr, strlen(pErrStr)) == false) {
	    return(0);
	}
	CheckDataOk("Get ", (uint8_t *)msgStr, (uint8_t *)pErrStr, strlen(pErrStr), true);
	break;
    case eNoreply:
	break;
    default:
	assert(0);
    }
    return(rtn);
}


uint64_t
CThread::BinaryGet(ECmd cmd, int keyIdx, int flagsIdx, string &dataStr,
			ERsp reply, uint64_t casUnique)
{
    protocol_binary_request_header	binHdr;
    int					respLength;
    struct msghdr			msghdr = { 0 };
    struct iovec			iov[6];
    char				udphdr[UDP_HEADER_SIZE];
    const char				*pErrStr;
    char				msgStr[256];
    uint64_t				rtn = 0;
    size_t				kl;
    char				*kp;
    uint32_t				exptime;

    memset(&binHdr, 0, sizeof(binHdr));
    binHdr.request.magic = PROTOCOL_BINARY_REQ;
    switch (cmd) {
	case eGetCmd:
	case eGetsCmd:
	    binHdr.request.opcode = (reply == eNoreply)
			? PROTOCOL_BINARY_CMD_GETQ : PROTOCOL_BINARY_CMD_GET;
	    break;
	case eGetkCmd:
	    binHdr.request.opcode = (reply == eNoreply)
			? PROTOCOL_BINARY_CMD_GETKQ : PROTOCOL_BINARY_CMD_GETK;
	    break;
	case eGatCmd:
	    binHdr.request.opcode = (reply == eNoreply)
			? PROTOCOL_BINARY_CMD_GATQ : PROTOCOL_BINARY_CMD_GAT;
	    break;
	default:
	    assert(0);
    }

    if(args.test != 2) {
        binHdr.request.keylen = htons(m_randKey[keyIdx].size());
	binHdr.request.bodylen = htonl(m_randKey[keyIdx].size());
    } else {                        // use perf headers
        binHdr.request.keylen = htons(args.perfKeyLength);
	binHdr.request.bodylen = htonl(args.perfKeyLength);
    }
    binHdr.request.extlen = (cmd == eGatCmd) ? 4 : 0;
    binHdr.request.opaque = htonl(m_opaque);
    binHdr.request.cas = htonll(casUnique);

    msghdr.msg_iov = iov;
    size_t & len = msghdr.msg_iovlen = 0;

    if (m_bUdpEnabled) {
	msghdr.msg_name = &m_udpAddr;
	msghdr.msg_namelen = m_udpAddrLen;
		
	CreateUdpHeader(udphdr);
	iov[len].iov_base = (void *)udphdr;
	iov[len].iov_len = UDP_HEADER_SIZE;
	len += 1;
    }

    iov[len].iov_base = (void *)&binHdr;
    iov[len].iov_len = sizeof(binHdr);
    len += 1;

    if(cmd == eGatCmd) {
	if(reply == eGetHit) {
	    exptime = StartKeyTimeout(keyIdx);
	} else {
	    exptime = 0;
	}
	exptime = htonl(exptime);
        iov[len].iov_base = (void *)&exptime;
        iov[len].iov_len = 4;
        len += 1;
    }

    if(args.test != 2) {
        iov[len].iov_base = kp = (char *)m_randKey[keyIdx].c_str();
        iov[len].iov_len = kl = m_randKey[keyIdx].size();
        len += 1;
    } else {                        // use perf headers
        iov[len].iov_base = kp = m_perfKeyTbl[keyIdx];
        iov[len].iov_len = kl = args.perfKeyLength;
        len += 1;
    }
    if(m_genErr == true) {
	BinaryForceError(keyIdx, msghdr);
	return(0);
    }
    SendMsg(msghdr);

    respLength = (args.test != 2) ? dataStr.size()
				  : m_dataHdrLength + m_perfDataFixedLength;
 
    protocol_binary_response_header respHdr, respHdrE;
    static SkipPbrh skip(SkipPbrh::CAS);
    respHdrE.response.magic = PROTOCOL_BINARY_RES;
    respHdrE.response.opcode = binHdr.request.opcode;
    respHdrE.response.keylen = 0;
    if(cmd == eGetkCmd) {
	respHdrE.response.keylen = binHdr.request.keylen;
    }
    respHdrE.response.extlen = 0;
    respHdrE.response.datatype = 0;
    respHdrE.response.status = ntohs(PROTOCOL_BINARY_RESPONSE_SUCCESS);
    respHdrE.response.bodylen = htonl(0);
    respHdrE.response.opaque = binHdr.request.opaque;

    if(reply == eNoreply) {
        m_gCasState = C_INV;            // value is unknown
        return(0);
    }

    if(BinaryRespRead(&respHdr,sizeof(protocol_binary_response_header))==false){
        m_gCasState = C_INV;            // value is unknown
	return(0);
    }

    switch (reply) {
    case eGetHit:
	respHdrE.response.extlen = 4;			// flags returned
	respHdrE.response.bodylen = htonl(respLength + 4);
	if(cmd == eGetkCmd) {
	    respHdrE.response.bodylen = htonl(respLength + 4 + kl);
	}
	CheckHdrOk("Get ", respHdr, respHdrE, skip);
	RespCheck(htonl(m_randFlags[flagsIdx]), (size_t)4);
	if(cmd == eGetkCmd) {
	    if(BinaryRespRead(msgStr, kl) == false) {
		return(0);
	    }
	    CheckDataOk("Get ", (uint8_t *)msgStr, (uint8_t *)kp, kl, false);
	}
	if(args.test != 2) {
	    RespCheck(dataStr);
	} else {
	    RespCheck(m_perfDataTbl[m_binaryGetIndex]);
	    RespCheck(m_perfDataFixed);
	}
	m_gCasState = C_VAL;
	m_gCas = ntohll(respHdr.response.cas);
	rtn = m_gCas;
	break;
    case eGetMiss:
    case eNotFound:
	m_gCasState = C_INV;
	rtn = 0;
	if(cmd == eGetkCmd) {
	    pErrStr = "Not found";
	    respHdrE.response.bodylen = htonl(kl);
	    respHdrE.response.status =
				ntohs(PROTOCOL_BINARY_RESPONSE_KEY_ENOENT);
	    CheckHdrOk("Get ", respHdr, respHdrE, skip);
	    if(BinaryRespRead(msgStr, kl) == false) {
		return(0);
	    }
	    CheckDataOk("Get ", (uint8_t *)msgStr, (uint8_t *)kp, kl, false);
	} else {
	    pErrStr = "Not found";
	    respHdrE.response.bodylen = htonl(strlen(pErrStr));
	    respHdrE.response.status
				= ntohs(PROTOCOL_BINARY_RESPONSE_KEY_ENOENT);
	    CheckHdrOk("Get ", respHdr, respHdrE, skip);
	    if(BinaryRespRead(msgStr, strlen(pErrStr)) == false) {
		return(0);
	    }
	    CheckDataOk("Get ", (uint8_t *)msgStr, (uint8_t *)pErrStr,
			strlen(pErrStr), true);
	}
	break;
    default:
	break;
    }
    return(rtn);
}

void CThread::BinaryDelete(int keyIdx, ERsp reply, uint64_t casUnique)
{
    protocol_binary_request_header	binHdr;
    const char				*pErrStr;
    char				msgStr[256];

    memset(&binHdr, 0, sizeof(binHdr));

    binHdr.request.magic = PROTOCOL_BINARY_REQ;
    binHdr.request.opcode = reply == eNoreply ?
		    PROTOCOL_BINARY_CMD_DELETEQ : PROTOCOL_BINARY_CMD_DELETE;
    binHdr.request.keylen = htons(m_randKey[keyIdx].size());
    binHdr.request.extlen = 0;
    binHdr.request.bodylen = htonl(m_randKey[keyIdx].size());
    binHdr.request.opaque = htonl(m_opaque);
    binHdr.request.cas = htonll(casUnique);

    struct msghdr msghdr = { 0 };
    struct iovec iov[5];

    msghdr.msg_iov = iov;
    size_t & len = msghdr.msg_iovlen = 0;
    char udphdr[UDP_HEADER_SIZE];
    if (m_bUdpEnabled) {
	msghdr.msg_name = &m_udpAddr;
	msghdr.msg_namelen = m_udpAddrLen;
		
	CreateUdpHeader(udphdr);
	iov[len].iov_base = (void *)udphdr;
	iov[len].iov_len = UDP_HEADER_SIZE;
	len += 1;
    }

    iov[len].iov_base = (void *)&binHdr;
    iov[len].iov_len = sizeof(binHdr);
    len += 1;

    iov[len].iov_base = (void *)m_randKey[keyIdx].c_str();
    iov[len].iov_len = m_randKey[keyIdx].size();
    len += 1;
    if(m_genErr == true) {
	BinaryForceError(keyIdx, msghdr);
	return;
    }
    SendMsg(msghdr);

    protocol_binary_response_header respHdr, respHdrE;
    static SkipPbrh skip(SkipPbrh::CAS);
    respHdrE.response.magic = PROTOCOL_BINARY_RES;
    respHdrE.response.opcode = binHdr.request.opcode;
    respHdrE.response.keylen = 0;
    respHdrE.response.extlen = 0;
    respHdrE.response.datatype = 0;
    respHdrE.response.status = ntohs(PROTOCOL_BINARY_RESPONSE_SUCCESS);
    respHdrE.response.bodylen = htonl(0);
    respHdrE.response.opaque = binHdr.request.opaque;

    if(reply != eNoreply) {
	if(BinaryRespRead(&respHdr, sizeof(protocol_binary_response_header)) == false) {
	    return;
	}
    }
    if(reply == eSwallow) {
	reply = (respHdr.response.bodylen == 0) ? eDeleted : eNotFound;
    }
	
    switch (reply) {
	case eDeleted:
	    CheckHdrOk("Delete ", respHdr, respHdrE, skip);
	    break;
	case eNotFound:
	    pErrStr = "Not found";
	    respHdrE.response.bodylen = htonl(strlen(pErrStr));
	    respHdrE.response.status=ntohs(PROTOCOL_BINARY_RESPONSE_KEY_ENOENT);
	    CheckHdrOk("Delete ", respHdr, respHdrE, skip);
	    if(BinaryRespRead(msgStr, strlen(pErrStr)) == false) {
		return;
	    }
	    CheckDataOk("Delete ", (uint8_t *)msgStr, (uint8_t *)pErrStr,
			strlen(pErrStr), true);
	    break;
	case eExists:
	    pErrStr = "Data exists for key.";
	    respHdrE.response.bodylen = htonl(strlen(pErrStr));
	    respHdrE.response.status =
				ntohs(PROTOCOL_BINARY_RESPONSE_KEY_EEXISTS);
	    CheckHdrOk("Delete", respHdr, respHdrE, skip);
	    if(BinaryRespRead(msgStr, strlen(pErrStr)) == false) {
		return;
	    }
	    CheckDataOk("Delete", (uint8_t *)msgStr, (uint8_t *)pErrStr,
				strlen(pErrStr), false);
	    break;
	case eNoreply:
	    break;
	default:
	    assert(0);
    }
}

void CThread::BinaryArith(ECmd cmd, int keyIdx, uint64_t inV, uint64_t outV,
			uint32_t expTmr, ERsp reply, uint64_t casUnique)
{
    protocol_binary_request_header	binHdr;
    const char				*pErrStr;
    const char				*instName;
    char				msgStr[256];
    uint64_t				rspVal;

    memset(&binHdr, 0, sizeof(binHdr));

    binHdr.request.magic = PROTOCOL_BINARY_REQ;

    switch (cmd) {
	case eIncrCmd:
	    binHdr.request.opcode = reply == eNoreply ?
		PROTOCOL_BINARY_CMD_INCREMENTQ : PROTOCOL_BINARY_CMD_INCREMENT;
	    instName = "Incr ";
	    break;
	case eDecrCmd:
	    binHdr.request.opcode = reply == eNoreply ?
		PROTOCOL_BINARY_CMD_DECREMENTQ : PROTOCOL_BINARY_CMD_DECREMENT;
	    instName = "Decr ";
	    break;
	default:
	    assert(0);
    }

    binHdr.request.keylen = htons(m_randKey[keyIdx].size());
    binHdr.request.extlen = 20;
    binHdr.request.bodylen = htonl(20 + m_randKey[keyIdx].size());
    binHdr.request.opaque = htonl(m_opaque);
    binHdr.request.cas = htonll(casUnique);

    struct msghdr msghdr = { 0 };
    struct iovec iov[6];

    msghdr.msg_iov = iov;
    size_t & len = msghdr.msg_iovlen = 0;
    char udphdr[UDP_HEADER_SIZE];
    if (m_bUdpEnabled) {
	msghdr.msg_name = &m_udpAddr;
	msghdr.msg_namelen = m_udpAddrLen;
		
	CreateUdpHeader(udphdr);
	iov[len].iov_base = (void *)udphdr;
	iov[len].iov_len = UDP_HEADER_SIZE;
	len += 1;
    }

    iov[len].iov_base = (void *)&binHdr;
    iov[len].iov_len = sizeof(binHdr);
    len += 1;

    uint64_t delta = htonll(inV);
    iov[len].iov_base = (void *)&delta;
    iov[len].iov_len = 8;
    len += 1;

    uint64_t init = htonll(outV);
    iov[len].iov_base = (void *)&init;
    iov[len].iov_len = 8;
    len += 1;

    uint32_t exptime = expTmr;
    if(reply == eValue || reply == eNoreply) {
	exptime = StartKeyTimeout(keyIdx);
    } else {
	exptime = expTmr;
    }
    exptime = htonl(exptime);
    iov[len].iov_base = (void *)&exptime;
    iov[len].iov_len = 4;
    len += 1;

    iov[len].iov_base = (void *)m_randKey[keyIdx].c_str();
    iov[len].iov_len = m_randKey[keyIdx].size();
    len += 1;
    if(m_genErr == true) {
	BinaryForceError(keyIdx, msghdr);
	return;
    }
    SendMsg(msghdr);

    protocol_binary_response_header respHdr, respHdrE;
    static SkipPbrh skip(SkipPbrh::CAS);
    respHdrE.response.magic = PROTOCOL_BINARY_RES;
    respHdrE.response.opcode = binHdr.request.opcode;
    respHdrE.response.keylen = 0;
    respHdrE.response.extlen = 0;
    respHdrE.response.datatype = 0;
    respHdrE.response.status = ntohs(PROTOCOL_BINARY_RESPONSE_SUCCESS);
    respHdrE.response.opaque = binHdr.request.opaque;
    if(reply != eNoreply) {
	if(BinaryRespRead(&respHdr, sizeof(protocol_binary_response_header)) == false) {
	    return;
	}
    }

    switch (reply) {
	case eValue:
	    respHdrE.response.bodylen = htonl(8);
	    CheckHdrOk(instName, respHdr, respHdrE, skip);
	    if(BinaryRespRead(msgStr, 8) == false) {
		return;
	    }
	    rspVal = htonll(outV);
	    CheckDataOk(instName, (uint8_t *)msgStr,(uint8_t *)&rspVal,8,false);
	    break;
	case eNotFound:
	    respHdrE.response.bodylen = htonl(9);
	    respHdrE.response.status=ntohs(PROTOCOL_BINARY_RESPONSE_KEY_ENOENT);
	    CheckHdrOk(instName, respHdr, respHdrE, skip);
	    pErrStr = "Not found";
	    if(BinaryRespRead(msgStr, strlen(pErrStr)) == false) {
		return;
	    }
	    CheckDataOk(instName, (uint8_t *)msgStr, (uint8_t *)pErrStr,
				strlen(pErrStr), false);
	    break;
	case eNonNumeric:
	    pErrStr = "Non-numeric server-side value for incr or decr";
	    respHdrE.response.bodylen = htonl(strlen(pErrStr));
	    respHdrE.response.status =
				ntohs(PROTOCOL_BINARY_RESPONSE_DELTA_BADVAL);
	    CheckHdrOk(instName, respHdr, respHdrE, skip);
	    if(BinaryRespRead(msgStr, strlen(pErrStr)) == false) {
		return;
	    }
	    CheckDataOk(instName, (uint8_t *)msgStr, (uint8_t *)pErrStr,
				strlen(pErrStr), false);
	    break;
	case eExists:
	    pErrStr = "Data exists for key.";
	    respHdrE.response.bodylen = htonl(strlen(pErrStr));
	    respHdrE.response.status =
				ntohs(PROTOCOL_BINARY_RESPONSE_KEY_EEXISTS);
	    CheckHdrOk(instName, respHdr, respHdrE, skip);
	    if(BinaryRespRead(msgStr, strlen(pErrStr)) == false) {
		return;
	    }
	    CheckDataOk(instName, (uint8_t *)msgStr, (uint8_t *)pErrStr,
				strlen(pErrStr), false);
	    break;
	case eNoreply:
	    break;
	default:
	    assert(0);
    }
}

void
CThread::BinaryTouch(int keyIdx, ERsp reply, uint64_t casUnique)
{
    protocol_binary_request_header	binHdr;
    const char				*pErrStr;
    uint32_t				flags;

    memset(&binHdr, 0, sizeof(binHdr));

    binHdr.request.magic = PROTOCOL_BINARY_REQ;
    binHdr.request.opcode = PROTOCOL_BINARY_CMD_TOUCH;
    binHdr.request.keylen = htons(m_randKey[keyIdx].size());
    binHdr.request.extlen = 4;
    binHdr.request.bodylen = htonl(m_randKey[keyIdx].size());
    binHdr.request.opaque = htonl(m_opaque);
    binHdr.request.cas = htonll(casUnique);

    struct msghdr msghdr = { 0 };
    struct iovec iov[5];

    msghdr.msg_iov = iov;
    size_t & len = msghdr.msg_iovlen = 0;
    char udphdr[UDP_HEADER_SIZE];
    if (m_bUdpEnabled) {
	msghdr.msg_name = &m_udpAddr;
	msghdr.msg_namelen = m_udpAddrLen;
		
	CreateUdpHeader(udphdr);
	iov[len].iov_base = (void *)udphdr;
	iov[len].iov_len = UDP_HEADER_SIZE;
	len += 1;
    }

    iov[len].iov_base = (void *)&binHdr;
    iov[len].iov_len = sizeof(binHdr);
    len += 1;

    uint32_t exptime;
    if(reply == eTouched) {
	exptime = StartKeyTimeout(keyIdx);
    } else {
	exptime = 0;
    }
    exptime = htonl(exptime);
    iov[len].iov_base = (void *)&exptime;
    iov[len].iov_len = 4;
    len += 1;

    iov[len].iov_base = (void *)m_randKey[keyIdx].c_str();
    iov[len].iov_len = m_randKey[keyIdx].size();
    len += 1;
    if(m_genErr == true) {
	BinaryForceError(keyIdx, msghdr);
	return;
    }
    SendMsg(msghdr);

    protocol_binary_response_header respHdr, respHdrE;
    static SkipPbrh skip(SkipPbrh::CAS);
    respHdrE.response.magic = PROTOCOL_BINARY_RES;
    respHdrE.response.opcode = binHdr.request.opcode;
    respHdrE.response.keylen = 0;
    respHdrE.response.extlen = 0;
    respHdrE.response.datatype = 0;
    respHdrE.response.status = ntohs(PROTOCOL_BINARY_RESPONSE_SUCCESS);
    respHdrE.response.bodylen = htonl(0);
    respHdrE.response.opaque = binHdr.request.opaque;

    if(BinaryRespRead(&respHdr, sizeof(protocol_binary_response_header)) == false) {
	return;
    }

    switch (reply) {
	case eTouched:
	    respHdrE.response.extlen = sizeof(uint32_t);
	    respHdrE.response.bodylen = htonl(sizeof(uint32_t));
	    CheckHdrOk("Touch ", respHdr, respHdrE, skip);
	    if(BinaryRespRead(&flags, sizeof(uint32_t)) == false) {
		return;
	    }
	    break;
	case eNotFound:
	    pErrStr = "Not found";
	    respHdrE.response.bodylen = htonl(strlen(pErrStr));
	    respHdrE.response.status=htons(PROTOCOL_BINARY_RESPONSE_KEY_ENOENT);
	    CheckHdrOk("Touch ", respHdr, respHdrE, skip);
	    RespCheck(pErrStr);
	    break;
	case eNoreply:
	    break;
	default:
	    assert(0);
    }
}

void CThread::BinaryQuit(ERsp reply)
{
    protocol_binary_request_header binHdr;

    memset(&binHdr, 0, sizeof(binHdr));

    binHdr.request.magic = PROTOCOL_BINARY_REQ;
    binHdr.request.opcode = (reply != eNoreply)
			? PROTOCOL_BINARY_CMD_QUIT : PROTOCOL_BINARY_CMD_QUITQ;
    binHdr.request.keylen = 0;
    binHdr.request.extlen = 0;
    binHdr.request.bodylen = 0;
    binHdr.request.opaque = htonl(m_opaque);
    binHdr.request.cas = htonll(m_randCas);

    struct msghdr msghdr = { 0 };
    struct iovec iov[5];

    msghdr.msg_iov = iov;
    size_t & len = msghdr.msg_iovlen = 0;
    char udphdr[UDP_HEADER_SIZE];
    if (m_bUdpEnabled) {
	msghdr.msg_name = &m_udpAddr;
	msghdr.msg_namelen = m_udpAddrLen;
		
	CreateUdpHeader(udphdr);
	iov[len].iov_base = (void *)udphdr;
	iov[len].iov_len = UDP_HEADER_SIZE;
	len += 1;
    }

    iov[len].iov_base = (void *)&binHdr;
    iov[len].iov_len = sizeof(binHdr);
    len += 1;

    SendMsg(msghdr);

    protocol_binary_response_header respHdr, respHdrE;
    static SkipPbrh skip(SkipPbrh::CAS);
    respHdrE.response.magic = PROTOCOL_BINARY_RES;
    respHdrE.response.opcode = binHdr.request.opcode;
    respHdrE.response.keylen = 0;
    respHdrE.response.extlen = 0;
    respHdrE.response.datatype = 0;
    respHdrE.response.status = ntohs(PROTOCOL_BINARY_RESPONSE_SUCCESS);
    respHdrE.response.bodylen = htonl(0);
    respHdrE.response.opaque = binHdr.request.opaque;

    if(reply != eNoreply) {
	if(BinaryRespRead(&respHdr, sizeof(protocol_binary_response_header)) == false) {
	    return;
	}
	CheckHdrOk("Quit ", respHdr, respHdrE, skip);
    }
}

void CThread::BinaryClientError()
{
    struct msghdr msghdr = { 0 };
    struct iovec iov[5];

    msghdr.msg_iov = iov;
    size_t & len = msghdr.msg_iovlen = 0;

    iov[len].iov_base = (void *)"This string does not have the magic number";
    iov[len].iov_len = sizeof((char *)iov[len].iov_base);
    len += 1;

    SendMsgTcp(msghdr);

    BinaryRespReadTcp((void *)"", 0);

    // the only binary client error is miss compare of BINARY MAGIC number, which closes connection
}

void CThread::BinaryVersion()
{
    protocol_binary_request_header  binHdr;
    uint8_t			    msgBuf[256];

    memset(&binHdr, 0, sizeof(binHdr));

    binHdr.request.magic = PROTOCOL_BINARY_REQ;
    binHdr.request.opcode = PROTOCOL_BINARY_CMD_VERSION;
    binHdr.request.keylen = 0;
    binHdr.request.extlen = 0;
    binHdr.request.bodylen = 0;
    binHdr.request.opaque = htonl(m_opaque);
    binHdr.request.cas = htonll(m_randCas);

    struct msghdr msghdr = { 0 };
    struct iovec iov[5];

    msghdr.msg_iov = iov;
    size_t & len = msghdr.msg_iovlen = 0;
    char udphdr[UDP_HEADER_SIZE];
    if (m_bUdpEnabled) {
	msghdr.msg_name = &m_udpAddr;
	msghdr.msg_namelen = m_udpAddrLen;
		
	CreateUdpHeader(udphdr);
	iov[len].iov_base = (void *)udphdr;
	iov[len].iov_len = UDP_HEADER_SIZE;
	len += 1;
    }

    iov[len].iov_base = (void *)&binHdr;
    iov[len].iov_len = sizeof(binHdr);
    len += 1;
    if(m_genErr == true) {
	BinaryForceError(0, msghdr);
	return;
    }
    SendMsg(msghdr);

    protocol_binary_response_header respHdr, respHdrE;
    static SkipPbrh skip(SkipPbrh::CAS);
    respHdrE.response.magic = PROTOCOL_BINARY_RES;
    respHdrE.response.opcode = binHdr.request.opcode;
    respHdrE.response.keylen = 0;
    respHdrE.response.extlen = 0;
    respHdrE.response.datatype = 0;
    respHdrE.response.status = ntohs(PROTOCOL_BINARY_RESPONSE_SUCCESS);
    respHdrE.response.bodylen = htonl(0);
    respHdrE.response.opaque = binHdr.request.opaque;

    if(BinaryRespRead(&respHdr, sizeof(protocol_binary_response_header)) == false) {
	return;
    }

    const char *vs1 = "Convey 1.4.15";
    const char *vs2 = "1.4.15";
    const char *vsE = "";
    if(respHdr.response.bodylen == htonl(strlen(vs1))) {
	respHdrE.response.bodylen = respHdr.response.bodylen;		// make commpare pass
	vsE = vs1;
    } else if(respHdr.response.bodylen == htonl(strlen(vs2))) {
	respHdrE.response.bodylen = respHdr.response.bodylen;		// make commpare pass
	vsE = vs2;
    }
    CheckHdrOk("Version ", respHdr, respHdrE, skip);

    if(BinaryRespRead(msgBuf, ntohl(respHdr.response.bodylen)) == false) {
	return;
    }
    CheckDataOk("Version ", msgBuf, (uint8_t *)vsE, strlen(vsE), false);
}

void CThread::BinaryNoop()
{
    protocol_binary_request_header binHdr;

    memset(&binHdr, 0, sizeof(binHdr));

    binHdr.request.magic = PROTOCOL_BINARY_REQ;
    binHdr.request.opcode = PROTOCOL_BINARY_CMD_NOOP;
    binHdr.request.keylen = 0;
    binHdr.request.extlen = 0;
    binHdr.request.bodylen = 0;
    binHdr.request.opaque = htonl(m_opaque);
    binHdr.request.cas = htonll(m_randCas);

    struct msghdr msghdr = { 0 };
    struct iovec iov[5];

    msghdr.msg_iov = iov;
    size_t & len = msghdr.msg_iovlen = 0;
    char udphdr[UDP_HEADER_SIZE];
    if (m_bUdpEnabled) {
	msghdr.msg_name = &m_udpAddr;
	msghdr.msg_namelen = m_udpAddrLen;
		
	CreateUdpHeader(udphdr);
	iov[len].iov_base = (void *)udphdr;
	iov[len].iov_len = UDP_HEADER_SIZE;
	len += 1;
    }

    iov[len].iov_base = (void *)&binHdr;
    iov[len].iov_len = sizeof(binHdr);
    len += 1;
    if(m_genErr == true) {
	BinaryForceError(0, msghdr);
	return;
    }
    SendMsg(msghdr);

    protocol_binary_response_header respHdr, respHdrE;
    static SkipPbrh skip(SkipPbrh::CAS);
    respHdrE.response.magic = PROTOCOL_BINARY_RES;
    respHdrE.response.opcode = binHdr.request.opcode;
    respHdrE.response.keylen = 0;
    respHdrE.response.extlen = 0;
    respHdrE.response.datatype = 0;
    respHdrE.response.status = ntohs(PROTOCOL_BINARY_RESPONSE_SUCCESS);
    respHdrE.response.bodylen = htonl(0);
    respHdrE.response.opaque = binHdr.request.opaque;

    if(BinaryRespRead(&respHdr, sizeof(protocol_binary_response_header)) == false) {
	return;
    }
    CheckHdrOk("Noop ", respHdr, respHdrE, skip);
}

void CThread::BinaryFlushAll()
{
    protocol_binary_request_header binHdr;

    memset(&binHdr, 0, sizeof(binHdr));

    binHdr.request.magic = PROTOCOL_BINARY_REQ;
    binHdr.request.opcode = PROTOCOL_BINARY_CMD_FLUSH;
    binHdr.request.keylen = 0;
    binHdr.request.extlen = 0;
    binHdr.request.bodylen = 0;
    binHdr.request.opaque = htonl(m_opaque);
    binHdr.request.cas = htonll(m_randCas);

    struct msghdr msghdr = { 0 };
    struct iovec iov[5];

    msghdr.msg_iov = iov;
    size_t & len = msghdr.msg_iovlen = 0;

    char udphdr[UDP_HEADER_SIZE];
    if (m_bUdpEnabled) {
	msghdr.msg_name = &m_udpAddr;
	msghdr.msg_namelen = m_udpAddrLen;
		
	CreateUdpHeader(udphdr);
	iov[len].iov_base = (void *)udphdr;
	iov[len].iov_len = UDP_HEADER_SIZE;
	len += 1;
    }

    iov[len].iov_base = (void *)&binHdr;
    iov[len].iov_len = sizeof(binHdr);
    len += 1;

    SendMsg(msghdr);

    protocol_binary_response_header respHdr, respHdrE;
    static SkipPbrh skip(SkipPbrh::CAS);
    respHdrE.response.magic = PROTOCOL_BINARY_RES;
    respHdrE.response.opcode = binHdr.request.opcode;
    respHdrE.response.keylen = 0;
    respHdrE.response.extlen = 0;
    respHdrE.response.datatype = 0;
    respHdrE.response.status = ntohs(PROTOCOL_BINARY_RESPONSE_SUCCESS);
    respHdrE.response.bodylen = htonl(0);
    respHdrE.response.opaque = binHdr.request.opaque;


    if(BinaryRespRead(&respHdr, sizeof(protocol_binary_response_header)) == false) {
	return;
    }
    CheckHdrOk("Flush ", respHdr, respHdrE, skip);
}

void CThread::BinaryStats(const char * pSubStat, bool printData, const char *hdr)
{
    protocol_binary_request_header	binHdr;
    char				respData[1024];

    memset(&binHdr, 0, sizeof(binHdr));

    binHdr.request.magic = PROTOCOL_BINARY_REQ;
    binHdr.request.opcode = PROTOCOL_BINARY_CMD_STAT;
    binHdr.request.keylen = pSubStat ? htons(strlen(pSubStat)) : 0;
    binHdr.request.extlen = 0;
    binHdr.request.bodylen = 0;
    binHdr.request.opaque = htonl(m_opaque);
    binHdr.request.cas = htonll(m_randCas);

    struct msghdr msghdr = { 0 };
    struct iovec iov[5];

    msghdr.msg_iov = iov;
    size_t & len = msghdr.msg_iovlen = 0;
    char udphdr[UDP_HEADER_SIZE];
    if (m_bUdpEnabled) {
	msghdr.msg_name = &m_udpAddr;
	msghdr.msg_namelen = m_udpAddrLen;
		
	CreateUdpHeader(udphdr);
	iov[len].iov_base = (void *)udphdr;
	iov[len].iov_len = UDP_HEADER_SIZE;
	len += 1;
    }

    iov[len].iov_base = (void *)&binHdr;
    iov[len].iov_len = sizeof(binHdr);
    len += 1;

    if (pSubStat) {
	    iov[len].iov_base = (void *)pSubStat;
	    iov[len].iov_len = strlen(pSubStat);
	    len += 1;
    }
    if(m_genErr == true) {
	BinaryForceError(0, msghdr);
	return;
    }
    SendMsg(msghdr);
    SkipPbrh skip1(SkipPbrh::CAS, SkipPbrh::KL, SkipPbrh::BL, SkipPbrh::STAT);
    SkipPbrh skip(SkipPbrh::CAS, SkipPbrh::KL, SkipPbrh::BL);
    protocol_binary_response_header respHdr, respHdrE;
    if(hdr != 0 && strncmp(hdr, "ITEM", 4) == 0) {
	skip = skip1;
    }
    respHdrE.response.magic = PROTOCOL_BINARY_RES;
    respHdrE.response.opcode = binHdr.request.opcode;
    respHdrE.response.extlen = 0;
    respHdrE.response.datatype = 0;
    respHdrE.response.status = ntohs(PROTOCOL_BINARY_RESPONSE_SUCCESS);
    respHdrE.response.opaque = binHdr.request.opaque;

    for(;;) {
	if(BinaryRespRead(&respHdr, sizeof(protocol_binary_response_header)) == false) {
	    return;
	}
	CheckHdrOk("Stats ", respHdr, respHdrE, skip);
	if(BinaryRespRead(&respData, ntohl(respHdr.response.bodylen)) == false) {	// read the data
	    return;
	}
	if(respHdr.response.keylen == 0) {
	    if(printData == true) {
		printf("END\n");
	    }
	    break;
	}
	if(printData == true) {
	    int keyL = htons(respHdr.response.keylen);
	    int dataL = ntohl(respHdr.response.bodylen) - keyL;
	    printf("STAT %.*s %.*s\n", keyL, respData, dataL, &respData[keyL]);
	}
    }
}

bool
CThread::BinaryRespRead(void *pBuf, size_t len)
{
    bool    rtn;

    if (m_bUdpEnabled) {
	rtn = BinaryRespReadUdp(pBuf, len);
    } else {
	rtn = BinaryRespReadTcp(pBuf, len);
    }
    return(rtn);
}

bool
CThread::BinaryRespReadUdp(void *pBuf, size_t size)
{
    // first check what is in buffer
    char * pRespStr = (char *)pBuf;
    ssize_t respLen = size;

    while (respLen > 0) {
	if(FillUdpBuffer() == false) {
	    return(false);
	}
	assert(m_udpRespBufLen > 0);

	ssize_t checkLen = min(respLen, m_udpRespBufLen);
	memcpy(pRespStr, m_pUdpRespBuf, checkLen);

	m_pUdpRespBuf += checkLen;
	m_udpRespBufLen -= checkLen;

	pRespStr += checkLen;
	respLen -= checkLen;
    }
    return(true);
}

bool
CThread::BinaryRespReadTcp(void *pBuf, size_t size)
{
    // first check what is in buffer
    char * pRespStr = (char *)pBuf;
    ssize_t respLen = size;

    while (respLen > 0) {
	if (m_tcpRespBufLen == 0) {
	    m_pTcpRespBuf = m_tcpRespBuf;
	    m_tcpRespBufLen = m_recv(m_tcpFd, m_tcpRespBuf, RESP_BUF_SIZE, 0,
			"BinaryRespReadTcp");
	    if(m_tcpRespBufLen == 0) {
		ErrorMsg("BinaryRespReadTcp read failed", 0, 0);
		exit(0);
	    }
	}

	size_t checkLen = min(respLen, m_tcpRespBufLen);
	memcpy(pRespStr, m_pTcpRespBuf, checkLen);

	m_pTcpRespBuf += checkLen;
	m_tcpRespBufLen -= checkLen;

	pRespStr += checkLen;
	respLen -= checkLen;
    }
    return(true);
}

void CThread::BinaryRespSkip(ssize_t len)
{
    if (m_bUdpEnabled)
	BinaryRespSkipUdp(len);
    else
	BinaryRespSkipTcp(len);
}

void CThread::BinaryRespSkipUdp(ssize_t len)
{
    do {
	if (m_udpRespBufLen == 0) {
	    if(FillUdpBuffer() == false) {
		return;
	    }
	}
	assert(m_udpRespBufLen > 0);

	if (m_udpRespBufLen > len) {
	    m_pUdpRespBuf += len;
	    m_udpRespBufLen -= len;
	    len = 0;
	} else {
	    len -= m_udpRespBufLen;
	    m_udpRespBufLen = 0;
	}
    } while (len > 0);
}

void CThread::BinaryRespSkipTcp(ssize_t len)
{
    do {
	if (m_tcpRespBufLen == 0) {
	    m_pTcpRespBuf = m_tcpRespBuf;
	    m_tcpRespBufLen = m_recv(m_tcpFd, m_tcpRespBuf, RESP_BUF_SIZE, 0,
			"BinaryRespSkipTcp");
	}
	if(m_tcpRespBufLen == 0) {
	    ErrorMsg("BinaryRespSkipTcp read failed", 0, 0);
	    exit(0);
	}

	if (m_tcpRespBufLen > len) {
	    m_pTcpRespBuf += len;
	    m_tcpRespBufLen -= len;
	    len = 0;
	} else {
	    len -= m_tcpRespBufLen;
	    m_tcpRespBufLen = 0;
	}
    } while (len > 0);
}

bool
CThread::CheckHdrOk(const char *eMsg, protocol_binary_response_header &hdr,
				protocol_binary_response_header &exp, SkipPbrh skipPbrh)
{
    string	txt;
    string	expStr;
    string	gotStr;
    char	buf[256];
    bool	rtn = true;

    if(skipPbrh.magicT && hdr.response.magic != exp.response.magic) {
	txt = eMsg;
	txt += "Header magic compare failed, ";
	snprintf(buf, sizeof(buf), "0x%02x", exp.response.magic);
	expStr = buf;
	snprintf(buf, sizeof(buf), "0x%02x", hdr.response.magic);
	gotStr = buf;
	ErrorMsg(txt, &expStr, &gotStr);
	rtn = false;
    }
    if(skipPbrh.opcodeT && hdr.response.opcode != exp.response.opcode) {
	txt = eMsg;
	txt += "Header opcode compare failed, ";
	snprintf(buf, sizeof(buf), "0x%02x", exp.response.opcode);
	expStr = buf;
	snprintf(buf, sizeof(buf), "0x%02x", hdr.response.opcode);
	gotStr = buf;
	ErrorMsg(txt, &expStr, &gotStr);
	rtn = false;
    }
    if(skipPbrh.statusT && hdr.response.status != exp.response.status) {
	txt = eMsg;
	txt += "Header status compare failed, ";
	snprintf(buf, sizeof(buf), "0x%08x", exp.response.status);
	expStr = buf;
	snprintf(buf, sizeof(buf), "0x%08x", hdr.response.status);
	gotStr = buf;
	ErrorMsg(txt, &expStr, &gotStr);
	rtn = false;
    }
    if(skipPbrh.keylenT && hdr.response.keylen != exp.response.keylen) {
	txt = eMsg;
	txt += "Header keylen compare failed, ";
	snprintf(buf, sizeof(buf), "0x%04x", exp.response.keylen);
	expStr = buf;
	snprintf(buf, sizeof(buf), "0x%04x", hdr.response.keylen);
	gotStr = buf;
	ErrorMsg(txt, &expStr, &gotStr);
	rtn = false;
    }
    if(skipPbrh.extlenT && hdr.response.extlen != exp.response.extlen) {
	txt = eMsg;
	txt += "Header extlen compare failed, ";
	snprintf(buf, sizeof(buf), "0x%02x", exp.response.extlen);
	expStr = buf;
	snprintf(buf, sizeof(buf), "0x%02x", hdr.response.extlen);
	gotStr = buf;
	ErrorMsg(txt, &expStr, &gotStr);
	rtn = false;
    }
    if(skipPbrh.datatypeT && hdr.response.datatype != exp.response.datatype) {
	txt = eMsg;
	txt += "Header datatype compare failed, ";
	snprintf(buf, sizeof(buf), "0x%02x", exp.response.datatype);
	expStr = buf;
	snprintf(buf, sizeof(buf), "0x%02x", hdr.response.datatype);
	gotStr = buf;
	ErrorMsg(txt, &expStr, &gotStr);
	rtn = false;
    }
    if(skipPbrh.bodylenT && hdr.response.bodylen != exp.response.bodylen) {
	txt = eMsg;
	txt += "Header bodylen compare failed, ";
	snprintf(buf, sizeof(buf), "0x%08x", exp.response.bodylen);
	expStr = buf;
	snprintf(buf, sizeof(buf), "0x%08x", hdr.response.bodylen);
	gotStr = buf;
	ErrorMsg(txt, &expStr, &gotStr);
	rtn = false;
    }
    if(skipPbrh.opaqueT && hdr.response.opaque != exp.response.opaque) {
	txt = eMsg;
	txt += "Header opaque compare failed, ";
	snprintf(buf, sizeof(buf), "0x%08x", exp.response.opaque);
	expStr = buf;
	snprintf(buf, sizeof(buf), "0x%08x", hdr.response.opaque);
	gotStr = buf;
	ErrorMsg(txt, &expStr, &gotStr);
	rtn = false;
    }
    if(skipPbrh.casT && hdr.response.cas != exp.response.cas) {
	txt = eMsg;
	txt += "Header cas compare failed, ";
	snprintf(buf, sizeof(buf), "0x%016lx", exp.response.cas);
	expStr = buf;
	snprintf(buf, sizeof(buf), "0x%016lx", hdr.response.cas);
	gotStr = buf;
	ErrorMsg(txt, &expStr, &gotStr);
	rtn = false;
    }
    return(rtn);
}

bool
CThread::CheckDataOk(const char *eMsg, uint8_t *got, uint8_t *exp, int length, bool asciiData)
{
    string	txt;
    string	expStr;
    string	gotStr;
    char	buf[256];
    bool	rtn = true;
    int		i;

    if(memcmp(got, exp, length) != 0) {
	txt = eMsg;
	txt += "Data compare failed, ";
	if(asciiData) {
	    snprintf(buf, sizeof(buf), "%s", exp);
	    expStr = buf;
	    snprintf(buf, sizeof(buf), "%s", got);
	    gotStr = buf;
	} else {
	    for(i=0; i<min(8, length); i++) {
		sprintf(&buf[i*5], "0x%02x ", exp[i]);
	    }
	    expStr = buf;
	    for(i=0; i<min(8, length); i++) {
		sprintf(&buf[i*5], "0x%02x ", got[i]);
	    }
	    gotStr = buf;
	}
	ErrorMsg(txt, &expStr, &gotStr);
    }
    return(rtn);
}
//
//  force some error and handle the result
//
void
CThread::BinaryForceError(int key, msghdr &msghdr)
{
    ErrType			    et;
    protocol_binary_request_header  *pbrh;
    bool			    expDropConn;
    string			    expText;
    int				    keySize;
    static int			    eCnt = 0;
    Action			    act;

    //
    // any command that has data following the header should not be tested for aUnkCmd
    // the server reads the header, replies (24 bytes) and then reads expecting a header
    // but normally gets an invalid magic number.
    //
    //	the data type test is ignored because the server ignores the data type
    //
    static const Action action[eCmdCnt][eErrCnt] = {	    // must match ECmd order
	// magic	cmd	    key
	{ aDropConn,	aSkip,	    aSkip,	aSkip },    // set cmd
	{ aDropConn,	aSkip,	    aSkip,	aSkip },    // add cmd
	{ aDropConn,	aSkip,	    aSkip,	aSkip },    // rep cmd
	{ aDropConn,	aSkip,	    aSkip,	aSkip },    // app cmd
	{ aDropConn,	aSkip,	    aSkip,	aSkip },    // pre cmd
	{ aDropConn,	aSkip,	    aSkip,	aSkip },    // cas cmd
	{ aDropConn,	aSkip,	    aInvArg,	aSkip },    // get cmd
	{ aDropConn,	aSkip,	    aSkip,	aSkip },    // gets cmd
	{ aDropConn,	aSkip,	    aInvArg,	aSkip },    // getk cmd
	{ aSkip,	aSkip,	    aSkip,	aSkip },    // getr cmd
        { aDropConn,	aSkip,	    aSkip,	aSkip },    // gat cmd
	{ aDropConn,	aSkip,	    aInvArg,	aSkip },    // inc cmd
	{ aDropConn,	aSkip,	    aInvArg,	aSkip },    // dec cmd
	{ aDropConn,	aSkip,	    aInvArg,	aSkip },    // del cmd
	{ aDropConn,	aSkip,	    aSkip,	aSkip },    // touch cmd
	{ aDropConn,	aUnkCmd,    aNotFound,	aSkip },    // stat cmd
	{ aDropConn,	aUnkCmd,    aInvArg,	aSkip },    // ver cmd
	{ aDropConn,	aUnkCmd,    aInvArg,	aSkip },    // nop cmd

    };

    expText = "";
    pbrh = (protocol_binary_request_header *)msghdr.msg_iov->iov_base;

    protocol_binary_response_header respHdr, respHdrE;
    static SkipPbrh skip(0);		// 0 keeps compiler happy, test all
    respHdrE.response.magic = PROTOCOL_BINARY_RES;
    respHdrE.response.opcode = pbrh->request.opcode;
    respHdrE.response.keylen = 0;
    respHdrE.response.extlen = 0;
    respHdrE.response.datatype = 0;
    respHdrE.response.status = ntohs(PROTOCOL_BINARY_RESPONSE_SUCCESS);
    respHdrE.response.bodylen = htonl(0);
    respHdrE.response.opaque = pbrh->request.opaque;
    respHdrE.response.cas = 0;

    et = (ErrType)(m_rnd() % eErrCnt);
    act = action[m_cmdCmd][et];
    if(m_trace2) {
        printf("command %s   error %s   action %s\n",
	    cmdStr[m_cmdCmd], errStr[et], actStr[act]);
    }
    switch(et) {		// do the actual error
	case eMagic:
	    pbrh->request.magic = 0x90 + (eCnt++%10);
	    break;
	case eCmd:
	    pbrh->request.opaque = pbrh->request.opcode;
	    respHdrE.response.opaque = pbrh->request.opcode;
	    pbrh->request.opcode = 0xff;
	    respHdrE.response.opcode = 0xff;
	    break;
	case eKeyLen:
	    //
	    // if original keySize is 0 and total body length is 0
	    // add 1 to the msghdr.msg_iov->iov_len to prevent hanging the server
	    //
	    keySize = ntohs(pbrh->request.keylen);
	    if(keySize == 0 && pbrh->request.bodylen == 0) {
		msghdr.msg_iov->iov_len++;
	    }
	    pbrh->request.keylen = htons(keySize + 1);
	    break;
	case eDataType:
	    pbrh->request.datatype = 1;
	    respHdrE.response.datatype = 1;
	    break;
	default:
	    assert(0);
    }
    expDropConn = false;
    expText = "";
    switch(act) {
	case aTAS:			// test and stop
	    break;
	case aSkip:
	    if(m_trace2) {
		printf("\n");
	    }
	    return;			// legal case
	case aDropConn:
	    expDropConn = true;
	    break;
	case aUnkCmd:
	    respHdrE.response.opcode = pbrh->request.opcode;
	    respHdrE.response.status = ntohs(PROTOCOL_BINARY_RESPONSE_UNKNOWN_COMMAND);
	    expText = "Unknown command";
	    respHdrE.response.bodylen = htonl(expText.length());
	    break;
	case aInvArg:
	    respHdrE.response.status = ntohs(PROTOCOL_BINARY_RESPONSE_EINVAL);
	    expText = "Invalid arguments";
	    respHdrE.response.bodylen = htonl(expText.length());
	    expDropConn = true;
	    break;
	case aNotFound:
	    respHdrE.response.status = ntohs(PROTOCOL_BINARY_RESPONSE_KEY_ENOENT);
	    expText = "Not found";
	    respHdrE.response.bodylen = htonl(expText.length());
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
	m_tcpRespBufLen = m_recv(m_tcpFd, m_tcpRespBuf, RESP_BUF_SIZE, 0,
		"BinaryForceError");
	if(m_tcpRespBufLen == 0) {
	    ErrorMsg("BinaryForceError read failed", 0, 0);
	    exit(0);
	}
	if(m_trace2) {
	    printf("recieved %ld bytes   ", m_tcpRespBufLen);
	}
	if(m_tcpRespBufLen < (ssize_t)sizeof(protocol_binary_response_header)) {
	    printf("did not recieve header, expected %ld   got %ld bytes\n",
		sizeof(protocol_binary_response_header), m_tcpRespBufLen);
	    ErrorMsg("Invalid header length recieved", 0, 0);
	} else {
	    respHdr = *(protocol_binary_response_header *)(void *)m_tcpRespBuf;
	    CheckHdrOk("ErrChk ", respHdr, respHdrE, skip);
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
    if(m_trace2) {
	printf("\n\n");
    }
    m_tcpRespBufLen = 0;
}

bool
CThread::FillUdpBuffer()
{
    fd_set  read;
    int	    sel;

    if (m_udpRespBufLen == 0) {
	timeval tv = {20,1};		// wait 2 sec
	FD_ZERO(&read);
	FD_SET(m_udpFd, &read);
	sel = select(m_udpFd + 1, &read, 0, 0, &tv);
	if(sel != 1) {
	    if(m_trace2) {
		printf("%3d Udp timeout\n", m_tid);
		printf("***************************************************\n");
	    }
	    m_udpDropped++;
	    return(false);
	}
	m_pUdpRespBuf = m_udpRespBuf;
	m_udpRespBufLen = recvfrom(m_udpFd, m_udpRespBuf, RESP_BUF_SIZE, 0,
	    (sockaddr *)&m_udpAddr, &m_udpAddrLen);
	if(m_udpRespBufLen == -1) {
	    if(m_trace2) {
		m_udpRespBufLen = 0;
		m_udpDropped++;
		printf("errno %d\n", errno);
		return(false);
	    }
	}

	CUdpHdr udpHdr;
	if(ParseUdpHeader(&udpHdr) == false) {
	    return(false);
	}
    }
    return(true);
}

ssize_t
CThread::m_recv(int fd, void *buf, size_t len, int flags, char *routine)
{
    ssize_t	rtn;
    char	pbuf[256];

    rtn = recv(fd, buf, len, flags);
    if(rtn == -1) {
	sprintf(pbuf, "%3d  %s   Bad Rx", m_tid, routine);
	perror(pbuf);
	exit(0);
    }
    return(rtn);
}
