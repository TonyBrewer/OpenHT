/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT memcached application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "MemCached.h"

#include "Timer.h"
#include "Powersave.h"
extern uint64_t pktRecvTime;
extern uint64_t blkSendTime;

void CConn::BuildUdpHeader(CXmitMsg * pXmitMsg)
{
	CUdpReqInfo & udpReqInfo = pXmitMsg->m_udpReqInfo;
    char *pHdr = pXmitMsg->m_udpHdr;

	*pHdr++ = udpReqInfo.m_udpReqId / 256;
	*pHdr++ = udpReqInfo.m_udpReqId % 256;
	*pHdr++ = pXmitMsg->m_msgNum / 256;
	*pHdr++ = pXmitMsg->m_msgNum % 256;
	*pHdr++ = pXmitMsg->m_msgCnt / 256;
	*pHdr++ = pXmitMsg->m_msgCnt % 256;
	*pHdr++ = 0;
	*pHdr++ = 0;
}

/*
 * Link the next chunk of data from our list of msgbuf structures.
 */
void CConn::LinkForTransmit()
{
    assert(!m_pXmitMsg->m_bCloseAfterXmit || m_pXmitMsg->m_pMsgHdrFront->m_msghdr.msg_iovlen == 0);

    // first check if a response is present
    if (!m_pXmitMsg->m_bCloseAfterXmit && !m_pXmitMsg->m_bFreeAfterXmit 
        && (m_pXmitMsg->m_pMsgHdrFront == 0 || m_pXmitMsg->m_pMsgHdrFront->m_msghdr.msg_iovlen == 0))
    {
		FreeXmitMsg(m_pXmitMsg, false);
        m_pXmitMsg = 0;
		return;
	}

    // insert time stamp
    m_pXmitMsg->m_xmitLinkTime = CTimer::Rdtsc();

	// Insert pXmitMsg to back of list
	if (m_pXmitMsgFront == 0)
		m_pXmitMsgFront = m_pXmitMsgBack = m_pXmitMsg;
	else
		m_pXmitMsgBack = m_pXmitMsgBack->m_pNext = m_pXmitMsg;

	m_xmitMsgCnt += 1;

	// if current xmit message is complete then set it as ready to transmit
	m_pXmitMsg = 0;

    // Link connection for transmission
    assert(m_bFree == false);
    m_pWorker->LinkConnForXmit(this);
}

void CConn::SendCloseToCoproc()
{
    m_pWorker->m_closeMsgQue.Push(m_connId);
    m_bCloseReqSent = true;
}

void CConn::CloseConn()
{
	if (m_sfd != 0) {
        // remove event to avoid getting future notifications, we free resources when coproc responds
        event_del(&m_event);
        m_bEventActive = false;

        if (g_settings.m_verbose > 1)
            fprintf(stderr, ">%d connection closed.\n", m_sfd);

		close_socket(m_sfd);
		m_sfd = 0;
	}
}

void CConn::FreeConn() {
    g_bAllowNewConns = true;

    assert(m_pItem == 0);
    assert(m_pXmitMsgFront == 0);
    assert(m_pXmitMsg == 0);

    /* if the connection has big buffers, just free it */
    m_pWorker->ConnAddToFreelist(this);

    __sync_fetch_and_sub(&g_stats.m_currConns, 1);

    return;
}

void CConn::Transmit()
{
    if (m_sfd == 0 || m_pXmitMsgFront->m_bCloseAfterXmit || m_pXmitMsgFront->m_bFreeAfterXmit) {
        // handle special transmit operation when xmit message is head of xmit queue

        while (m_pXmitMsgFront) {

            if (m_pXmitMsgFront->m_bCloseAfterXmit) {
                FreeXmitMsg(m_pXmitMsgFront, true);
                CloseConn();

            } else if (m_pXmitMsgFront->m_bFreeAfterXmit) {
                FreeXmitMsg(m_pXmitMsgFront, true);
                FreeConn();

            } else
                FreeXmitMsg(m_pXmitMsgFront, true);
        }

        return;
    }

	CXmitMsg * pXmitMsg = m_pXmitMsgFront;

    uint64_t accumTime = CTimer::Rdtsc() - pXmitMsg->m_xmitLinkTime;
    if (g_maxXmitLinkTimeCpuClks > (int)accumTime)
        return;

    g_pTimers[11]->Start(m_pWorker->m_unitId);

#define IOV_LIST_SIZE   512
    struct iovec iov[IOV_LIST_SIZE];
    size_t iovCnt = 0;

    struct msghdr msghdr = pXmitMsg->m_pMsgHdrFront->m_msghdr;
    msghdr.msg_iov = iov;

    // build multi-message header
    int xmitMsgCnt = 0;
	for ( CXmitMsg * pXmitMsg = m_pXmitMsgFront; pXmitMsg && iovCnt < IOV_LIST_SIZE;
        pXmitMsg = pXmitMsg->m_pNext, xmitMsgCnt += 1) {

        if (pXmitMsg->m_bCloseAfterXmit || pXmitMsg->m_bFreeAfterXmit)
            break;

        if (pXmitMsg->m_pMsgHdrFront->m_msghdr.msg_iovlen == 0) {
            assert(0);
            continue;
        }

        CMsgHdr * pMsgHdr = pXmitMsg->m_pMsgHdrFront;

		if (IsUdp()) {
			CUdpReqInfo & udpReqInfo = pXmitMsg->m_udpReqInfo;
			msghdr.msg_name = (void *) &udpReqInfo.m_udpReqAddr;
			msghdr.msg_namelen = udpReqInfo.m_udpReqAddrSize;

			BuildUdpHeader(pXmitMsg);
            pMsgHdr->m_msghdr.msg_iov->iov_base = pXmitMsg->m_udpHdr;
            pMsgHdr->m_msghdr.msg_iov->iov_len = UDP_HEADER_SIZE;

            memcpy(iov + iovCnt, pMsgHdr->m_msghdr.msg_iov, pMsgHdr->m_msghdr.msg_iovlen * sizeof(iovec));
            iovCnt += pMsgHdr->m_msghdr.msg_iovlen;
            break;
        }

        for ( ; pMsgHdr && iovCnt < IOV_LIST_SIZE; pMsgHdr = pMsgHdr->m_pNext) {
            size_t cnt = min(IOV_LIST_SIZE - iovCnt, pMsgHdr->m_msghdr.msg_iovlen);
            memcpy(iov + iovCnt, pMsgHdr->m_msghdr.msg_iov, cnt * sizeof(iovec));
            iovCnt += cnt;
        }
    }

    msghdr.msg_iovlen = iovCnt;

    assert(iovCnt > 0);
	ssize_t res = sendmsg(m_sfd, &msghdr, MSG_DONTWAIT);

    int sampleXmitMsgCnt = 0;
    if (res > 0) {
        m_pWorker->m_stats.m_bytesWritten += res;

        // free completed commands
	    while (m_pXmitMsgFront && res > 0) {

            while (m_pXmitMsgFront->m_pMsgHdrFront && res > 0) {

                CMsgHdr * pMsgHdr = m_pXmitMsgFront->m_pMsgHdrFront;
                while (pMsgHdr->m_msghdr.msg_iovlen > 0 && (size_t)res >= pMsgHdr->m_msghdr.msg_iov->iov_len) {
                    res -= (ssize_t)pMsgHdr->m_msghdr.msg_iov->iov_len;

				    pMsgHdr->m_msghdr.msg_iovlen--;
				    pMsgHdr->m_msghdr.msg_iov++;
                }

                if (pMsgHdr->m_msghdr.msg_iovlen == 0) {
                    // cleanup msgHdr
                    if (pMsgHdr->m_pItem) {
				        item_remove(pMsgHdr->m_pItem);
				        pMsgHdr->m_pItem = 0;
			        }

                    xmitMsgCnt -= 1;
                    m_pXmitMsgFront->m_pMsgHdrFront = pMsgHdr->m_pNext;
			        m_pWorker->FreeMsgHdr(pMsgHdr);

                    m_pXmitMsgFront->m_msgNum += 1;
                } else
                    break;
            }

            if (m_pXmitMsgFront->m_pMsgHdrFront == 0) {

                FreeXmitMsg(m_pXmitMsgFront, true);

                sampleXmitMsgCnt += 1;
            } else
                break;
        }

        // adjust iov and len for partially transmitted iov entry
        if (res > 0 && m_pXmitMsgFront) {
			assert(!IsUdp());

            CMsgHdr * pMsgHdr = m_pXmitMsgFront->m_pMsgHdrFront;
            assert(pMsgHdr);

			pMsgHdr->m_msghdr.msg_iov->iov_base = (char *)pMsgHdr->m_msghdr.msg_iov->iov_base + res;
			pMsgHdr->m_msghdr.msg_iov->iov_len -= res;
		}

    } else if (res == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {

	    ;
	} else {
		/* if res == 0 or res == -1 and error is not EAGAIN or EWOULDBLOCK,
			we have a real error, on which we close the connection */
		if (errno < 0 && g_settings.m_verbose > 0)
			perror("Failed to write");

		// send socket closed message through personality
		if (!m_bCloseReqSent && !IsUdp()) {
            // remove event to avoid getting future notifications, we free resources when coproc responds
            CloseConn();
            m_caxCode = 2;

            // send close message through coproc to follow allow packets
            SendCloseToCoproc();
        }

        FreeXmitMsg(m_pXmitMsgFront, true);
	}

    g_pTimers[11]->Stop(m_pWorker->m_unitId);
    g_pSamples[0]->Sample(sampleXmitMsgCnt, m_pWorker->m_unitId);
}

bool CConn::UpdateEvent(const short newFlags) {

    if (m_eventFlags == newFlags || !m_bEventActive)
        return true;

	if (m_eventFlags != EV_PERSIST)
		if (event_del(&m_event) == -1) return false;

	if (newFlags != EV_PERSIST) {
		event_set(&m_event, m_sfd, newFlags, CWorker::EventHandler, (void *)this);

        if (m_pWorker)
		    event_base_set(m_pWorker->m_base, &m_event);
        else
            event_base_set(g_pMainEventLoop, &m_event);

		m_eventFlags = newFlags;
		if (event_add(&m_event, 0) == -1) return false;
	} else
		m_eventFlags = newFlags;

    return true;
}

void CConn::FreeXmitMsg(CXmitMsg * pXmitMsg, bool bInXmitList)
{
	/* Finished writing the current msg; advance to the next. */

	if (pXmitMsg->m_pStatsBuf) {
		free(pXmitMsg->m_pStatsBuf);
        pXmitMsg->m_pStatsBuf = 0;
    }

	// free any stranded message headers
	while (pXmitMsg->m_pMsgHdrFront) {
		CMsgHdr * pMsgHdr = pXmitMsg->m_pMsgHdrFront;
		if (pMsgHdr->m_pItem) {
			item_remove(pMsgHdr->m_pItem);
			pMsgHdr->m_pItem = 0;
		}
		pXmitMsg->m_pMsgHdrFront = pMsgHdr->m_pNext;
		m_pWorker->FreeMsgHdr(pMsgHdr);
	}

	// free xmit msg
    if (bInXmitList) {
	    assert(pXmitMsg == m_pXmitMsgFront);
	    m_pXmitMsgFront = pXmitMsg->m_pNext;
	    m_xmitMsgCnt -= 1;
    }

	m_pWorker->FreeXmitMsg(pXmitMsg);
}

/*
 * Adds a message header to a connection.
 *
 * Returns CXmitMsg* on success, 0 on out-of-memory.
 */
CXmitMsg * CConn::AddXmitMsg()
{
	assert(m_pXmitMsg == 0);
	m_pXmitMsg = m_pWorker->CallocXmitMsg();

	if (m_pXmitMsg == 0) {
		// TODO message without xmitMsg
		//OutString("SERVER_ERROR out of memory preparing response\r\n");
		return 0;
	}

	if (m_pRspUdpReqInfo)
		m_pXmitMsg->m_udpReqInfo = *m_pRspUdpReqInfo;

	AddMsgHdr();

	return m_pXmitMsg;
}

CMsgHdr * CConn::AddMsgHdr()
{
	if (m_pXmitMsg == 0)
		// if m_pXmitMsg == 0 then out of memory, just return 0
		return 0;

	// allocate first message header
	m_pMsgHdr = m_pWorker->AllocMsgHdr();

	if (m_pMsgHdr == 0) {
		OutString("SERVER_ERROR out of memory allocating message header\r\n");
		return 0;
	}

	memset(&m_pMsgHdr->m_msghdr, 0, sizeof(struct msghdr));
	m_pMsgHdr->m_pNext = 0;
	m_pMsgHdr->m_pItem = 0;

	m_pXmitMsg->m_msgCnt += 1;
	if (m_pXmitMsg->m_pMsgHdrFront == 0)
		m_pXmitMsg->m_pMsgHdrBack = m_pXmitMsg->m_pMsgHdrFront = m_pMsgHdr;
	else
		m_pXmitMsg->m_pMsgHdrBack = m_pXmitMsg->m_pMsgHdrBack->m_pNext = m_pMsgHdr;

	// must limit udp message to fit in single datagram
	m_msgbytes = 0;

	m_pIov = &m_pMsgHdr->m_iov[0];
	m_pMsgHdr->m_msghdr.msg_iov = &m_pMsgHdr->m_iov[0];

    return m_pMsgHdr;
}

/*
 * Adds data to the list of pending data that will be written out to a
 * connection.
 *
 * Returns 0 on success, -1 on out-of-memory.
 */

bool CConn::AddIov(const void *buf, size_t len) {
    size_t leftover;
    bool bLimitMtu;

    do {
        /*
         * Limit UDP packets, and the first payloads of TCP replies, to
         * UDP_MAX_PAYLOAD_SIZE bytes.
         */
        bLimitMtu = IsUdp();

        /* We may need to start a new msghdr if this one is full. */
        if (bLimitMtu && m_msgbytes >= UDP_MAX_PAYLOAD_SIZE) {

			AddMsgHdr();
            if (m_pMsgHdr == 0)
				return false;
        }

        /* If the fragment is too big to fit in a single datagram, split it up */
        if (bLimitMtu && len + m_msgbytes > UDP_MAX_PAYLOAD_SIZE) {
            leftover = len + m_msgbytes - UDP_MAX_PAYLOAD_SIZE;
            len -= leftover;
        } else {
            leftover = 0;
        }
        if (bLimitMtu && m_pIov == m_pMsgHdr->m_iov) {
            /* Leave room for the UDP header, which we'll fill in later. */
            m_msgbytes += UDP_HEADER_SIZE;
            m_pMsgHdr->m_msghdr.msg_iovlen++;
            m_pIov += 1;
        }
        assert(m_pMsgHdr->m_msghdr.msg_iovlen < MAX_MSG_IOV_ENTRIES);
		m_pIov->iov_base = (char *)buf;
        m_pIov->iov_len = len;
		m_pIov += 1;

        m_msgbytes += (int)len;
        m_pMsgHdr->m_msghdr.msg_iovlen++;

        buf = ((char *)buf) + len;
        len = leftover;
    } while (leftover > 0);

    return true;
}

void CConn::OutString(const char *str) {

    if (m_noreply) {
        if (g_settings.m_verbose > 1)
            fprintf(stderr, ">%d NOREPLY %s", m_sfd, str);
        m_noreply = false;
        return;
    }

    if (g_settings.m_verbose > 1)
        fprintf(stderr, ">%d %s", m_sfd, str);

	if (m_pXmitMsg == 0 && AddXmitMsg() == 0)
		return;

    AddIov(str, strlen(str));
}

/////////////////////////////////////////////////////////////////////////////
// Ascii command processing

bool CConn::ProcessCmdUnused()
{
#ifdef CHECK_USING_MODEL
    m_pWorker->m_checkUnit.OpenMiscompareFile();
    m_pWorker->m_checkUnit.PrintMiscompareInfo();
#endif
	assert(0);
	return true;
}

bool CConn::ProcessCmdStore()
{
	CRspCmdQw * pRsp;
	if (!(pRsp = m_pWorker->GetHifRspCmd(CNY_CMD_STORE_QW_CNT)))
		return false;

	CCnyCpCmd * pRspCmd = &pRsp->m_rspCmd;
    m_cmd = pRspCmd->m_cmd;
	uint32_t vlen = pRspCmd->m_valueLen;
	m_noreply = pRspCmd->m_noreply;
	m_ascii = pRspCmd->m_ascii;
    m_cas = 0;

	uint32_t & flags = pRsp->m_rspDw[CNY_CMD_SET_DW_CNT];
	uint32_t & exptime = pRsp->m_rspDw[CNY_CMD_SET_DW_CNT + 1];
	uint64_t & req_cas_id = pRsp->m_rspQw[(CNY_CMD_SET_DW_CNT + 2)>>1];

	char * pKey;
	int keyLen;
    FindRecvBufString(pKey, keyLen, pRspCmd->m_keyPos, pRspCmd->m_keyLen, pRspCmd->m_keyInit);

	AddXmitMsg();

    CItem *it = ItemAlloc(pKey, keyLen, pRspCmd->m_keyHash, flags, realtime(exptime), vlen);

    if (it == 0) {
        if (! item_size_ok(keyLen, flags, vlen))
			OutString("SERVER_ERROR object too large for cache\r\n");
        else
            OutString("SERVER_ERROR out of memory storing object\r\n");

		/* Avoid stale data persisting in cache because we failed alloc.
		 * Unacceptable for SET. Anywhere else too? */
        it = ItemGet(pKey, keyLen, pRspCmd->m_keyHash);
        if (it) {
            item_unlink(it);
            item_remove(it);
        }

		m_pItem = 0;
		m_pItemRd = 0;
		m_itemRdLen = vlen;

        return true;
    }

    ITEM_set_cas(it, req_cas_id);

    m_pItem = it;
    m_pItemRd = ITEM_data(it);
    m_itemRdLen = it->nbytes;

	return true;
}

bool CConn::ProcessCmdKey()
{
	CRspCmdQw * pRsp = m_pWorker->GetHifRspCmd(1);
	CCnyCpCmd * pRspCmd = &pRsp->m_rspCmd;

	if (pRspCmd->m_keyInit)
        m_partialLen = 0;

	if (pRspCmd->m_keyPos + pRspCmd->m_keyLen >= MCD_RECV_BUF_SIZE ||
		m_partialLen + pRspCmd->m_keyLen > KEY_MAX_LENGTH)
	{
        // the can occur when a binary command is parsed as ASCII due to a bad magic number
        if (g_settings.m_verbose) {
            fprintf(stderr, "<< Protocol Error - key size exceeded\n");
			//m_pWorker->m_checkUnit.OpenMiscompareFile();
			//m_pWorker->m_checkUnit.PrintMiscompareInfo();
        }
		return true;
	}

	char * pKey = m_pWorker->m_pRecvBufBase + pRspCmd->m_keyPos;

    int copyStrLen = min((int)pRspCmd->m_keyLen, KEY_MAX_LENGTH-m_partialLen);
	memcpy(m_partialStr + m_partialLen, pKey, copyStrLen);
	m_partialLen += copyStrLen;

    assert(m_partialLen <= KEY_MAX_LENGTH);

	return true;
}

bool CConn::ProcessCmdData()
{
	CRspCmdQw * pRsp = m_pWorker->GetHifRspCmd(1);
	CCnyCpCmd * pRspCmd = &pRsp->m_rspCmd;

	char * pData = m_pWorker->m_pRecvBufBase + pRspCmd->m_dataPos;

	if (m_pItem) {
		memcpy(m_pItemRd, pData, pRspCmd->m_dataLen);
		m_pItemRd += pRspCmd->m_dataLen;
		m_itemRdLen -= pRspCmd->m_dataLen;

		if (m_itemRdLen == 0) {
			CompleteCmdStore();
			LinkForTransmit();
		}
	} else {
		m_itemRdLen -= pRspCmd->m_dataLen;

		if (m_itemRdLen == 0)
			LinkForTransmit();
	}

	return true;
}

/*
 * we get here after reading the value in set/add/replace commands. The command
 * has been stored in c->m_cmd, and the item is ready in c->m_pItem.
 */
void CConn::CompleteCmdStore() {
	EStoreItemType ret;
	CItem *it;
	int comm;

	it = m_pItem;
	comm = m_cmd;

	//pthread_mutex_lock(&m_pWorker->m_stats.mutex);
	m_pWorker->m_stats.m_slabStats[it->slabs_clsid].m_setCmds++;
	//pthread_mutex_unlock(&m_pWorker->m_stats.mutex);

	if (strncmp(ITEM_data(it) + it->nbytes - 2, "\r\n", 2) != 0) {
		OutString("CLIENT_ERROR bad data chunk\r\n");
	} else {
		ret = store_item(it, comm, this);

#ifdef ENABLE_DTRACE
		uint64_t cas = ITEM_get_cas(it);
		switch (m_cmd) {
		case CNY_CMD_ADD:
			MEMCACHED_COMMAND_ADD(m_sfd, ITEM_key(it), it->nkey,
				(ret == 1) ? it->nbytes : -1, cas);
			break;
		case CNY_CMD_REPLACE:
			MEMCACHED_COMMAND_REPLACE(m_sfd, ITEM_key(it), it->nkey,
				(ret == 1) ? it->nbytes : -1, cas);
			break;
		case CNY_CMD_APPEND:
			MEMCACHED_COMMAND_APPEND(m_sfd, ITEM_key(it), it->nkey,
				(ret == 1) ? it->nbytes : -1, cas);
			break;
		case CNY_CMD_PREPEND:
			MEMCACHED_COMMAND_PREPEND(m_sfd, ITEM_key(it), it->nkey,
				(ret == 1) ? it->nbytes : -1, cas);
			break;
		case CNY_CMD_SET:
			MEMCACHED_COMMAND_SET(m_sfd, ITEM_key(it), it->nkey,
				(ret == 1) ? it->nbytes : -1, cas);
			break;
		case CNY_CMD_CAS:
			MEMCACHED_COMMAND_CAS(m_sfd, ITEM_key(it), it->nkey, it->nbytes,
				cas);
			break;
		}
#endif

		switch (ret) {
		case STORED:
			OutString("STORED\r\n");
			break;
		case EXISTS:
			OutString("EXISTS\r\n");
			break;
		case NOT_FOUND:
			OutString("NOT_FOUND\r\n");
			break;
		case NOT_STORED:
			OutString("NOT_STORED\r\n");
			break;
		default:
			OutString("SERVER_ERROR Unhandled storage type.\r\n");
		}

	}

	item_remove(m_pItem);       /* release the m_pItem reference */
	m_pItem = 0;
}

bool CConn::ProcessCmdGet()
{
	CRspCmdQw * pRsp;
	if (!(pRsp = m_pWorker->GetHifRspCmd(CNY_CMD_GET_QW_CNT)))
		return false;

	CCnyCpCmd * pRspCmd = &pRsp->m_rspCmd;
	m_ascii = pRspCmd->m_ascii;

	char * pKey;
	int keyLen;
    FindRecvBufString(pKey, keyLen, pRspCmd->m_keyPos, pRspCmd->m_keyLen, pRspCmd->m_keyInit);

    CItem *it = ItemGet(pKey, keyLen, pRspCmd->m_keyHash);
    if (g_settings.detail_enabled) {
        stats_prefix_record_get(pKey, keyLen, NULL != it);
    }

	if (it) {
		if (m_pXmitMsg == 0)
			AddXmitMsg();
		else
			AddMsgHdr();

        /*
            * Construct the response. Each hit adds three elements to the
            * outgoing data list:
            *   "VALUE "
            *   key
            *   " " + flags + " " + data length + "\r\n" + data (with \r\n)
            */
        if (pRspCmd->m_cmd == CNY_CMD_GETS)
        {
			int casStrLen = snprintf(m_pMsgHdr->m_casStr, CAS_STR_LEN,
                                    " %llu\r\n",
                                    (unsigned long long)ITEM_get_cas(it));

            AddIov("VALUE ", 6);
            AddIov(ITEM_key(it), it->nkey);
            AddIov(ITEM_suffix(it), it->nsuffix - 2);
            AddIov(m_pMsgHdr->m_casStr, casStrLen);
            AddIov(ITEM_data(it), it->nbytes);
        }
        else
        {
            AddIov("VALUE ", 6);
			AddIov(ITEM_key(it), it->nkey);
			AddIov(ITEM_suffix(it), it->nsuffix + it->nbytes);
        }

        if (g_settings.m_verbose > 1)
            fprintf(stderr, ">%d sending key %.*s\n", m_sfd, it->nkey, ITEM_key(it));

        /* item_get() has incremented it->refcount for us */
        m_pWorker->m_stats.m_slabStats[it->slabs_clsid].m_getHits += 1;
        m_pWorker->m_stats.m_getCmds += 1;
        item_update(it);

		m_pMsgHdr->m_pItem = it;

    } else {
        m_pWorker->m_stats.m_getMisses += 1;
        m_pWorker->m_stats.m_getCmds += 1;
    }

	return true;
}

bool CConn::ProcessCmdGetEnd()
{
	CRspCmdQw * pRsp;
	if (!(pRsp = m_pWorker->GetHifRspCmd(CNY_CMD_GET_END_QW_CNT)))
		return false;

    if (g_settings.m_verbose > 1)
        fprintf(stderr, ">%d END\n", m_sfd);

	if (m_pXmitMsg == 0)
		AddXmitMsg();

    AddIov("END\r\n", 5);

	LinkForTransmit();

	return true;
}

bool CConn::ProcessCmdDelete()
{
	CRspCmdQw * pRsp;
	if (!(pRsp = m_pWorker->GetHifRspCmd(CNY_CMD_DELETE_QW_CNT)))
		return false;

	CCnyCpCmd * pRspCmd = &pRsp->m_rspCmd;
	m_noreply = pRspCmd->m_noreply;
	m_ascii = pRspCmd->m_ascii;

	char * pKey;
	int keyLen;
    FindRecvBufString(pKey, keyLen, pRspCmd->m_keyPos, pRspCmd->m_keyLen, pRspCmd->m_keyInit);

	AddXmitMsg();

    if (g_settings.detail_enabled) {
        stats_prefix_record_delete(pKey, keyLen);
    }

    CItem * it = ItemGet(pKey, keyLen, pRspCmd->m_keyHash);
    if (it) {
        m_pWorker->m_stats.m_slabStats[it->slabs_clsid].m_deleteHits++;

        item_unlink(it);
        item_remove(it);      /* release our reference */
		OutString("DELETED\r\n");
    } else {
        m_pWorker->m_stats.m_deleteMisses += 1;

        OutString("NOT_FOUND\r\n");
    }

	LinkForTransmit();

	return true;
}

bool CConn::ProcessCmdTouch()
{
	CRspCmdQw * pRsp;
	if (!(pRsp = m_pWorker->GetHifRspCmd(CNY_CMD_TOUCH_QW_CNT)))
		return false;

	CCnyCpCmd * pRspCmd = &pRsp->m_rspCmd;
	m_noreply = pRspCmd->m_noreply;
	m_ascii = pRspCmd->m_ascii;

	char * pKey;
	int keyLen;
    FindRecvBufString(pKey, keyLen, pRspCmd->m_keyPos, pRspCmd->m_keyLen, pRspCmd->m_keyInit);

	int32_t exptime_int = pRsp->m_rspDw[CNY_CMD_TOUCH_DW_CNT];

	CXmitMsg * pXmitMsg = AddXmitMsg();
	if (pXmitMsg == 0) return true;

    CItem *it = ItemTouch(pKey, keyLen, pRspCmd->m_keyHash, realtime(exptime_int));
    if (it) {
        item_update(it);
        //pthread_mutex_lock(&m_pWorker->m_stats.mutex);
        m_pWorker->m_stats.m_touchCmds += 1;
        m_pWorker->m_stats.m_slabStats[it->slabs_clsid].m_touchHits++;
        //pthread_mutex_unlock(&m_pWorker->m_stats.mutex);

        OutString("TOUCHED\r\n");
        item_remove(it);
    } else {
        //pthread_mutex_lock(&m_pWorker->m_stats.mutex);
        m_pWorker->m_stats.m_touchCmds++;
        m_pWorker->m_stats.m_touchMisses += 1;
        //pthread_mutex_unlock(&m_pWorker->m_stats.mutex);

        OutString("NOT_FOUND\r\n");
    }

	LinkForTransmit();

	return true;
}

void CConn::FindRecvBufString(char * &pStr, int &strLen, uint32_t cmdStrPos, uint32_t cmdStrLen, bool keyInit)
{
    if (keyInit)
        m_partialLen = 0;
	if (m_partialLen > 0) {
        int copyStrLen = min((int)cmdStrLen, KEY_MAX_LENGTH-m_partialLen);
		pStr = m_partialStr;
		memcpy(m_partialStr + m_partialLen, m_pWorker->m_pRecvBufBase + cmdStrPos, copyStrLen);
		strLen = m_partialLen + copyStrLen;
		m_partialLen = 0;
        assert(strLen <= KEY_MAX_LENGTH);
	} else {
		pStr = m_pWorker->m_pRecvBufBase + cmdStrPos;
		strLen = cmdStrLen;
	}
}

bool CConn::ProcessCmdArith()
{
	CRspCmdQw * pRsp;
	if (!(pRsp = m_pWorker->GetHifRspCmd(CNY_CMD_ARITH_QW_CNT)))
		return false;

	CCnyCpCmd * pRspCmd = &pRsp->m_rspCmd;
    m_cmd = pRspCmd->m_cmd;
	m_noreply = pRspCmd->m_noreply;
	m_ascii = pRspCmd->m_ascii;

	char * pKey;
	int keyLen;
    FindRecvBufString(pKey, keyLen, pRspCmd->m_keyPos, pRspCmd->m_keyLen, pRspCmd->m_keyInit);

	uint64_t & delta = pRsp->m_rspQw[CNY_CMD_ARITH_DW_CNT>>1];

	CXmitMsg * pXmitMsg = AddXmitMsg();
	if (pXmitMsg == 0) return true;

	bool bIncr = pRspCmd->m_cmd == CNY_CMD_INCR;

    switch(add_delta(this, pKey, keyLen, pRspCmd->m_keyHash, bIncr, delta, pXmitMsg->m_wrBuf, NULL)) {
    case OK:
		strcat(pXmitMsg->m_wrBuf, "\r\n");
        OutString(pXmitMsg->m_wrBuf);
        break;
    case NON_NUMERIC:
        OutString("CLIENT_ERROR cannot increment or decrement non-numeric value\r\n");
        break;
    case EOM:
        OutString("SERVER_ERROR out of memory\r\n");
        break;
    case DELTA_ITEM_NOT_FOUND:
        //pthread_mutex_lock(&m_pWorker->m_stats.mutex);
        if (bIncr) {
            m_pWorker->m_stats.m_incrMisses++;
        } else {
            m_pWorker->m_stats.m_decrMisses++;
        }
        //pthread_mutex_unlock(&m_pWorker->m_stats.mutex);

        OutString("NOT_FOUND\r\n");
        break;
    case DELTA_ITEM_CAS_MISMATCH:
        break; /* Should never get here */
    }

	LinkForTransmit();

	return true;
}

bool CConn::ProcessCmdError()
{
    CRspCmdQw * pRsp;
    if (!(pRsp = m_pWorker->GetHifRspCmd(CNY_CMD_ERROR_QW_CNT)))
        return false;

    CCnyCpCmd * pRspCmd = &pRsp->m_rspCmd;

	if (m_pXmitMsg == 0 && AddXmitMsg() == 0)
		return true;

	m_noreply = false;
    m_cmd = pRspCmd->m_cmd & 0x7f;
    m_opcode = pRspCmd->m_reqCmd;
    m_opaque = pRspCmd->m_opaque;
    m_ascii = pRspCmd->m_ascii;
    m_cas = 0;

	switch (pRspCmd->m_errCode) {
	case CNY_ERR_INV_DELTA:
        OutString("CLIENT_ERROR invalid delta argument\r\n");
		break;
	case CNY_ERR_INV_FLAG:
        OutString("CLIENT_ERROR invalid flag argument\r\n");
		break;
	case CNY_ERR_INV_EXPTIME:
        OutString("CLIENT_ERROR invalid exptime argument\r\n");
		break;
	case CNY_ERR_INV_VLEN:
        OutString("CLIENT_ERROR invalid value length argument\r\n");
		break;
	case CNY_ERR_BAD_CMD_EOL:
        OutString("CLIENT_ERROR bad command end-of-line\r\n");
		break;
	case CNY_ERR_BAD_DATA_EOL:
        OutString("CLIENT_ERROR bad data end-of-line\r\n");
		break;
	case CNY_ERR_BAD_FORMAT:
	default:
		// catch all
        OutString("CLIENT_ERROR bad command line format\r\n");
		break;
	case CNY_ERR_BINARY_MAGIC:
        // close connection
        if (g_settings.m_verbose)
            fprintf(stderr, "<%d Invalid binary magic\n", m_sfd);

        // FIXME: stock server doesn't send any message
        //OutString("CLIENT_ERROR invalid binary magic\r\n");

 		// send socket closed message through personality
		if (!m_bCloseReqSent && !IsUdp()) {
		    // must close after all responses have been transmitted
		    m_pXmitMsg->m_bCloseAfterXmit = true;
            assert(m_caxCode == 0);
            m_caxCode = 2;

            SendCloseToCoproc();
		}
		break;

    case CNY_ERR_INV_ARG:
        // close connection

        // send socket closed message through personality
        if (!m_bCloseReqSent && !IsUdp()) {

            WriteBinError(eInvalid);
            LinkForTransmit();

            // must close after all responses have been transmitted
            AddXmitMsg();
            m_pXmitMsg->m_bCloseAfterXmit = true;
            assert(m_caxCode == 0);
            m_caxCode = 3;

            SendCloseToCoproc();
        }
        break;
    case CNY_ERR_CONN_CLOSED:
		// connection was closed or had an error, cleanup, no transmission is required
        m_pXmitMsg->m_bFreeAfterXmit = true;
        assert(m_caxCode > 0);
		break;
	}

	LinkForTransmit();

	return true;
}

bool CConn::ProcessCmdOther()
{
	CRspCmdQw * pRsp = m_pWorker->GetHifRspCmd(1);
	CCnyCpCmd * pRspCmd = &pRsp->m_rspCmd;

	m_protocol = ascii_prot;
    m_noreply = pRspCmd->m_noreply;
    m_ascii = pRspCmd->m_ascii;

	char * pLine;
	int lineLen;
    FindRecvBufString(pLine, lineLen, pRspCmd->m_keyPos, pRspCmd->m_keyLen, pRspCmd->m_keyInit);

	pLine[lineLen-2] = '\0';

	ProcessCommand(pLine);

	return true;
}

void CConn::ProcessCommand(char *command) {

    token_t tokens[MAX_TOKENS];
    size_t ntokens;

    assert(this != NULL);

    if (g_settings.m_verbose > 1)
        fprintf(stderr, "<%d %s\n", m_sfd, command);

    /*
     * for commands set/add/replace, we build an item and read the data
     * directly into it, then continue in nread_complete().
     */

	AddXmitMsg();

    ntokens = TokenizeCommand(command, tokens, MAX_TOKENS);

	if (ntokens >= 2 && (strcmp(tokens[COMMAND_TOKEN].value, "stats") == 0)) {

        ProcessStat(tokens, ntokens);

    } else if (ntokens >= 2 && ntokens <= 4 && (strcmp(tokens[COMMAND_TOKEN].value, "flush_all") == 0)) {
        time_t exptime = 0;

        SetNoreplyMaybe(tokens, ntokens);

        m_pWorker->m_stats.m_flushCmds++;

        if(ntokens == (m_noreply ? 3 : 2)) {
            g_settings.oldest_live = current_time - 1;
            item_flush_expired();
            OutString("OK\r\n");
			LinkForTransmit();
            return;
        }

        exptime = strtol(tokens[1].value, NULL, 10);
        if(errno == ERANGE) {
            OutString("CLIENT_ERROR bad command line format\r\n");
			LinkForTransmit();
            return;
        }

        /*
          If exptime is zero realtime() would return zero too, and
          realtime(exptime) - 1 would overflow to the max unsigned
          value.  So we process exptime == 0 the same way we do when
          no delay is given at all.
        */
        if (exptime > 0)
            g_settings.oldest_live = realtime(exptime) - 1;
        else /* exptime == 0 */
            g_settings.oldest_live = current_time - 1;
        item_flush_expired();
        OutString("OK\r\n");
		LinkForTransmit();
        return;

    } else if (ntokens == 2 && (strcmp(tokens[COMMAND_TOKEN].value, "version") == 0)) {

        OutString("VERSION " MC_VERSION "\r\n");

    } else if (ntokens == 2 && (strcmp(tokens[COMMAND_TOKEN].value, "quit") == 0)) {

 		// send socket closed message through personality
		if (!m_bCloseReqSent && !IsUdp()) {
		    // must close after all responses have been transmitted
		    m_pXmitMsg->m_bCloseAfterXmit = true;
            assert(m_caxCode == 0);
            m_caxCode = 5;

            SendCloseToCoproc();
		}

    } else if (ntokens > 1 && strcmp(tokens[COMMAND_TOKEN].value, "slabs") == 0) {
        if (ntokens == 5 && strcmp(tokens[COMMAND_TOKEN + 1].value, "reassign") == 0) {
            int src, dst, rv;

            if (g_settings.slab_reassign == false) {
                OutString("CLIENT_ERROR slab reassignment disabled\r\n");
				LinkForTransmit();
                return;
            }

            src = strtol(tokens[2].value, NULL, 10);
            dst = strtol(tokens[3].value, NULL, 10);

            if (errno == ERANGE) {
                OutString("CLIENT_ERROR bad command line format\r\n");
				LinkForTransmit();
                return;
            }

            rv = slabs_reassign(src, dst);
            switch (rv) {
            case REASSIGN_OK:
                OutString("OK\r\n");
                break;
            case REASSIGN_RUNNING:
                OutString("BUSY currently processing reassign request\r\n");
                break;
            case REASSIGN_BADCLASS:
                OutString("BADCLASS invalid src or dst class id\r\n");
                break;
            case REASSIGN_NOSPARE:
                OutString("NOSPARE source class has no spare pages\r\n");
                break;
            case REASSIGN_SRC_DST_SAME:
                OutString("SAME src and dst class are identical\r\n");
                break;
            }
			LinkForTransmit();
            return;
        } else if (ntokens == 4 &&
            (strcmp(tokens[COMMAND_TOKEN + 1].value, "automove") == 0)) {
            ProcessSlabsAutomoveCommand(tokens, ntokens);
        } else {
            OutString("ERROR\r\n");
        }
    } else if ((ntokens == 3 || ntokens == 4) && (strcmp(tokens[COMMAND_TOKEN].value, "verbosity") == 0)) {
        ProcessVerbosityCommand(tokens, ntokens);
    } else {
        OutString("ERROR\r\n");
    }

	LinkForTransmit();

    return;
}

/*
 * Tokenize the command string by replacing whitespace with '\0' and update
 * the token array tokens with pointer to start of each token and length.
 * Returns total number of tokens.  The last valid token is the terminal
 * token (value points to the first unprocessed character of the string and
 * length zero).
 *
 * Usage example:
 *
 *  while(tokenize_command(command, ncommand, tokens, max_tokens) > 0) {
 *      for(int ix = 0; tokens[ix].length != 0; ix++) {
 *          ...
 *      }
 *      ncommand = tokens[ix].value - command;
 *      command  = tokens[ix].value;
 *   }
 */
size_t CConn::TokenizeCommand(char *command, token_t *tokens, const size_t max_tokens)
{
    char *s, *e;
    size_t ntokens = 0;
    size_t len = strlen(command);
    unsigned int i = 0;

    assert(command != NULL && tokens != NULL && max_tokens > 1);

    s = e = command;
    for (i = 0; i < len; i++) {
        if (*e == ' ') {
            if (s != e) {
                tokens[ntokens].value = s;
                tokens[ntokens].length = e - s;
                ntokens++;
                *e = '\0';
                if (ntokens == max_tokens - 1) {
                    e++;
                    s = e; /* so we don't add an extra token */
                    break;
                }
            }
            s = e + 1;
        }
        e++;
    }

    if (s != e) {
        tokens[ntokens].value = s;
        tokens[ntokens].length = e - s;
        ntokens++;
    }

    /*
     * If we scanned the whole string, the terminal value pointer is null,
     * otherwise it is the first unprocessed character.
     */
    tokens[ntokens].value =  *e == '\0' ? NULL : e;
    tokens[ntokens].length = 0;
    ntokens++;

    return ntokens;
}

/* set up a connection to write a buffer then free it, used for stats */
void CConn::WriteAndFree(char *buf, size_t bytes) {
    if (buf) {
		AddIov(buf, bytes);

		m_pXmitMsg->m_pStatsBuf = buf;
    } else {
        OutString("SERVER_ERROR out of memory writing stats\r\n");
    }
}

void CConn::ProcessSlabsAutomoveCommand(token_t *tokens, const size_t ntokens)
{
    unsigned int level;

    SetNoreplyMaybe(tokens, ntokens);

    level = strtoul(tokens[2].value, NULL, 10);
    if (level == 0) {
        g_settings.slab_automove = 0;
    } else if (level == 1 || level == 2) {
        g_settings.slab_automove = level;
    } else {
        OutString("ERROR\r\n");
        return;
    }
    OutString("OK\r\n");
    return;
}

void CConn::ProcessVerbosityCommand(token_t *tokens, const size_t ntokens)
{
    unsigned int level;

    SetNoreplyMaybe(tokens, ntokens);

    level = strtoul(tokens[1].value, NULL, 10);
    g_settings.m_verbose = level > MAX_VERBOSITY_LEVEL ? MAX_VERBOSITY_LEVEL : level;
    OutString("OK\r\n");
    return;
}

bool CConn::SetNoreplyMaybe(token_t *tokens, size_t ntokens)
{
    size_t noreply_index = ntokens - 2;

    /*
      NOTE: this function is not the first place where we are going to
      send the reply.  We could send it instead from process_command()
      if the request line has wrong number of tokens.  However parsing
      malformed line for "noreply" option is not reliable anyway, so
      it can't be helped.
    */
    if (tokens[noreply_index].value
        && strcmp(tokens[noreply_index].value, "noreply") == 0) {
        m_noreply = true;
    }
    return m_noreply;
}

void CConn::ProcessStat(token_t *tokens, const size_t ntokens)
{
    const char *subcommand = tokens[SUBCOMMAND_TOKEN].value;
    assert(this != NULL);

    if (ntokens < 2) {
        OutString("CLIENT_ERROR bad command line\r\n");
        return;
    }

    if (ntokens == 2) {
        ProcessServerStats();
        (void)get_stats(NULL, 0, this);
    } else if (strcmp(subcommand, "reset") == 0) {
        stats_reset();
        OutString("RESET\r\n");
        return ;
    } else if (strcmp(subcommand, "detail") == 0) {
        /* NOTE: how to tackle detail with binary? */
        if (ntokens < 4)
            ProcessStatsDetail("");  /* outputs the error message */
        else
            ProcessStatsDetail(tokens[2].value);
        /* Output already generated */
        return ;
    } else if (strcmp(subcommand, "settings") == 0) {
        ProcessSettingsStats();
    } else if (strcmp(subcommand, "cachedump") == 0) {
        char *buf;
        unsigned int bytes, id, limit = 0;

        if (ntokens < 5) {
            OutString("CLIENT_ERROR bad command line\r\n");
            return;
        }

        if (!safe_strtoul(tokens[2].value, &id) ||
            !safe_strtoul(tokens[3].value, &limit)) {
            OutString("CLIENT_ERROR bad command line format\r\n");
            return;
        }

        if (id >= POWER_LARGEST) {
            OutString("CLIENT_ERROR Illegal slab id\r\n");
            return;
        }

        buf = item_cachedump(id, limit, &bytes);
        WriteAndFree(buf, bytes);
        return ;
    } else {
        /* getting here means that the subcommand is either engine specific or
           is invalid. query the engine and see. */
        if (get_stats(subcommand, strlen(subcommand), this)) {
            if (m_stats.m_pBuffer == NULL) {
                OutString("SERVER_ERROR out of memory writing stats\r\n");
            } else {
                WriteAndFree(m_stats.m_pBuffer, m_stats.m_offset);
                m_stats.m_pBuffer = NULL;
            }
        } else {
            OutString("ERROR\r\n");
        }
        return ;
    }

    /* append terminator and start the transfer */
    AddStats(NULL, 0, NULL, 0);

    if (m_stats.m_pBuffer == NULL) {
        OutString("SERVER_ERROR out of memory writing stats\r\n");
    } else {
        WriteAndFree(m_stats.m_pBuffer, m_stats.m_offset);
        m_stats.m_pBuffer = NULL;
    }
}

void CConn::ProcessStatsDetail(const char *command)
{
    if (strcmp(command, "on") == 0) {
        g_settings.detail_enabled = 1;
        OutString("OK\r\n");
    }
    else if (strcmp(command, "off") == 0) {
        g_settings.detail_enabled = 0;
        OutString("OK\r\n");
    }
    else if (strcmp(command, "dump") == 0) {
        int len;
        char *stats = stats_prefix_dump(&len);
        WriteAndFree(stats, len);
    }
    else {
        OutString("CLIENT_ERROR usage: stats detail on|off|dump\r\n");
    }
}

///////////////////////////////// Binary Commands /////////////////////////////////

void CConn::AddBinHeader(uint16_t err, size_t hdr_len, size_t key_len, size_t body_len) {

	if (m_pXmitMsg == 0)
		return;

    CMcdBinRspHdr* pBinRspHdr = (CMcdBinRspHdr*)m_pXmitMsg->m_wrBuf;

    pBinRspHdr->m_magic = (uint8_t)MCD_BIN_RSP_MAGIC;
    pBinRspHdr->m_opcode = m_opcode;
    pBinRspHdr->m_keyLen = (uint16_t)htons((uint16_t)key_len);

    pBinRspHdr->m_extraLen = (uint8_t)hdr_len;
    pBinRspHdr->m_dataType = (uint8_t)eRawBytes;
    pBinRspHdr->m_status = (uint16_t)htons(err);

    pBinRspHdr->m_bodyLen = htonl((uint32_t)body_len);
    pBinRspHdr->m_opaque = m_opaque;
    pBinRspHdr->m_uniqueId = htonll(m_cas);

    if (g_settings.m_verbose > 1) {
        size_t ii;
        fprintf(stderr, ">%d Writing bin response:", m_sfd);
        for (ii = 0; ii < sizeof(CMcdBinRspHdr); ++ii) {
            if (ii % 4 == 0) {
                fprintf(stderr, "\n>%d  ", m_sfd);
            }
            fprintf(stderr, " 0x%02x", pBinRspHdr->m_bytes[ii]);
        }
        fprintf(stderr, "\n");
    }

    AddIov(pBinRspHdr, sizeof(CMcdBinRspHdr));
}

bool CConn::ProcessCmdBinStore()
{
	CRspCmdQw * pRsp;
	if (!(pRsp = m_pWorker->GetHifRspCmd(CNY_CMD_BIN_STORE_QW_CNT)))
		return false;

	CCnyCpCmd * pRspCmd = &pRsp->m_rspCmd;

	char * pKey;
	int keyLen;
    FindRecvBufString(pKey, keyLen, pRspCmd->m_keyPos, pRspCmd->m_keyLen, pRspCmd->m_keyInit);

	m_cmd = pRspCmd->m_cmd & 0x7f;
    m_opcode = pRspCmd->m_cmd & 0x7f;
	m_opaque = pRspCmd->m_opaque;
	uint32_t vlen = pRspCmd->m_valueLen;
	m_noreply = pRspCmd->m_noreply;
	m_ascii = pRspCmd->m_ascii;

	uint32_t & flags = pRsp->m_rspDw[CNY_BIN_CMD_DW_CNT];
	uint32_t & exptime = pRsp->m_rspDw[CNY_BIN_CMD_DW_CNT + 1];
    m_cas = 0;
    uint64_t cmdCas = *(uint64_t*)(void*)&pRsp->m_rspDw[CNY_BIN_CMD_DW_CNT + 2];

    if (g_settings.m_verbose > 1) {
        int ii;
        if (m_cmd == eMcdBinCmdAdd) {
            fprintf(stderr, "<%d ADD ", m_sfd);
        } else if (m_cmd == eMcdBinCmdSet) {
            fprintf(stderr, "<%d SET ", m_sfd);
        } else {
            fprintf(stderr, "<%d REPLACE ", m_sfd);
        }
        for (ii = 0; ii < keyLen; ++ii) {
            fprintf(stderr, "%c", pKey[ii]);
        }

        fprintf(stderr, " Value len is %d", vlen);
        fprintf(stderr, "\n");
    }

    if (g_settings.detail_enabled) {
        stats_prefix_record_set(pKey, keyLen);
    }

	CItem *it = ItemAlloc(pKey, keyLen, pRspCmd->m_keyHash, flags, realtime(exptime), vlen+2);

	AddXmitMsg();

    if (it == 0) {
        if (! item_size_ok(keyLen, flags, vlen + 2)) {
            WriteBinError(eValueToBig);
        } else {
            WriteBinError(eNoMemory);
        }

		/* Avoid stale data persisting in cache because we failed alloc.
			* Unacceptable for SET. Anywhere else too? */
        it = ItemGet(pKey, keyLen, pRspCmd->m_keyHash);
        if (it) {
            item_unlink(it);
            item_remove(it);
        }

		m_pItem = 0;
		m_pItemRd = 0;
		m_itemRdLen = vlen;

        return true;
    }

    ITEM_set_cas(it, cmdCas);

	m_cmd &= 0xf;	// strip quiet bit
    //switch (m_cmd) {
    //    case PROTOCOL_BINARY_CMD_ADD:
    //        m_cmd = CNY_CMD_ADD;
    //        break;
    //    case PROTOCOL_BINARY_CMD_SET:
    //        m_cmd = CNY_CMD_SET;
    //        break;
    //    case PROTOCOL_BINARY_CMD_REPLACE:
    //        m_cmd = CNY_CMD_REPLACE;
    //        break;
    //    default:
    //        assert(0);
    //}

    if (ITEM_get_cas(it) != 0) {
        m_cmd = CNY_CMD_CAS;
    }

    m_pItem = it;
    m_pItemRd = ITEM_data(it);
    m_itemRdLen = it->nbytes-2;

	return true;
}

bool CConn::ProcessCmdBinData()
{
	CRspCmdQw * pRsp = m_pWorker->GetHifRspCmd(1);
	CCnyCpCmd * pRspCmd = &pRsp->m_rspCmd;

	char * pData = m_pWorker->m_pRecvBufBase + pRspCmd->m_dataPos;

	if (m_pItem) {
		memcpy(m_pItemRd, pData, pRspCmd->m_dataLen);
		m_pItemRd += pRspCmd->m_dataLen;
		m_itemRdLen -= pRspCmd->m_dataLen;

		if (m_itemRdLen == 0) {
			CompleteBinStore();
			LinkForTransmit();
		}
	} else {
		m_itemRdLen -= pRspCmd->m_dataLen;

		if (m_itemRdLen == 0)
			LinkForTransmit();
	}

	return true;
}

bool CConn::ProcessCmdBinGet()
{
    g_pTimers[8]->Start(m_pWorker->m_unitId);
    g_pTimers[7]->Start(m_pWorker->m_unitId);

    CRspCmdQw * pRsp;
	if (!(pRsp = m_pWorker->GetHifRspCmd(CNY_CMD_BIN_GET_QW_CNT)))
		return false;

    g_pTimers[7]->Stop(m_pWorker->m_unitId);

	CCnyCpCmd * pRspCmd = &pRsp->m_rspCmd;

	char * pKey;
	int keyLen;
    FindRecvBufString(pKey, keyLen, pRspCmd->m_keyPos, pRspCmd->m_keyLen, pRspCmd->m_keyInit);

	m_cmd = pRspCmd->m_cmd & 0x7f;
    m_opcode = pRspCmd->m_cmd & 0x7f;
	m_opaque = pRspCmd->m_opaque;
	m_noreply = pRspCmd->m_noreply;
	m_ascii = pRspCmd->m_ascii;
    m_cas = 0;

    if (g_settings.m_verbose > 1)
        fprintf(stderr, "<%d GET %.*s\n", m_sfd, keyLen, pKey);

	AddXmitMsg();
	CMcdBinRspGet* rsp = (CMcdBinRspGet*)m_pXmitMsg->m_wrBuf;

	bool bGetkCmd = m_opcode == eMcdBinCmdGetk || m_opcode == eMcdBinCmdGetkq;

    g_pTimers[9]->Start(m_pWorker->m_unitId);
    CItem * it = ItemGet(pKey, keyLen, pRspCmd->m_keyHash);
    g_pTimers[9]->Stop(m_pWorker->m_unitId);

    if (it) {
        /* the length has two unnecessary bytes ("\r\n") */
        uint16_t keylen = 0;
        uint32_t bodylen = sizeof(rsp->m_body) + (it->nbytes - 2);

        g_pTimers[10]->Start(m_pWorker->m_unitId);
        item_update(it);

        m_pWorker->m_stats.m_getCmds += 1;
        m_pWorker->m_stats.m_slabStats[it->slabs_clsid].m_getHits += 1;

        if (bGetkCmd) {
            bodylen += keyLen;
            keylen = keyLen;
        }
        
		AddBinHeader(0, sizeof(rsp->m_body), keylen, bodylen);
        rsp->m_hdr.m_uniqueId = htonll(ITEM_get_cas(it));

        // add the flags
        rsp->m_body.m_flags = htonl(strtoul(ITEM_suffix(it), NULL, 10));
        AddIov(&rsp->m_body, sizeof(rsp->m_body));

        if (bGetkCmd) {
            AddIov(ITEM_key(it), keyLen);
        }

        /* Add the data minus the CRLF */
        AddIov(ITEM_data(it), it->nbytes - 2);

        /* Remember this command so we can garbage collect it later */
		m_pMsgHdr->m_pItem = it;
        g_pTimers[10]->Stop(m_pWorker->m_unitId);

    } else {
        m_pWorker->m_stats.m_getCmds += 1;
        m_pWorker->m_stats.m_getMisses += 1;

        if (!pRspCmd->m_noreply) {
			if (bGetkCmd) {
				AddBinHeader(eKeyNotFound, 0, keyLen, keyLen);
				char *ofs = m_pXmitMsg->m_wrBuf + sizeof(CMcdBinRspHdr);
				memcpy(ofs, pKey, keyLen);
				AddIov(ofs, keyLen);
			} else {
				WriteBinError(eKeyNotFound);
			}
		}
    }

    if (g_settings.detail_enabled) {
        stats_prefix_record_get(pKey, keyLen, NULL != it);
    }

	LinkForTransmit();

    g_pTimers[8]->Stop(m_pWorker->m_unitId);
	return true;
}

bool CConn::ProcessCmdBinDelete()
{
	CRspCmdQw * pRsp;
	if (!(pRsp = m_pWorker->GetHifRspCmd(CNY_CMD_BIN_DELETE_QW_CNT)))
		return false;

	CCnyCpCmd * pRspCmd = &pRsp->m_rspCmd;

	char * pKey;
	int keyLen;
    FindRecvBufString(pKey, keyLen, pRspCmd->m_keyPos, pRspCmd->m_keyLen, pRspCmd->m_keyInit);

    m_cmd = pRspCmd->m_cmd & 0x7f;
    m_opcode = pRspCmd->m_cmd & 0x7f;
	m_opaque = pRspCmd->m_opaque;
	m_noreply = pRspCmd->m_noreply;
	m_ascii = pRspCmd->m_ascii;
    m_cas = 0;

    uint64_t cmdCas = *(uint64_t*)(void*)&pRsp->m_rspDw[CNY_BIN_CMD_DW_CNT];

    if (g_settings.m_verbose > 1)
        fprintf(stderr, "Deleting %.*s\n", keyLen, pKey);

    if (g_settings.detail_enabled) {
        stats_prefix_record_delete(pKey, keyLen);
    }

	AddXmitMsg();

    CItem *it = ItemGet(pKey, keyLen, pRspCmd->m_keyHash);
    if (it) {
       if (cmdCas == 0 || cmdCas == ITEM_get_cas(it)) {
            m_pWorker->m_stats.m_slabStats[it->slabs_clsid].m_deleteHits++;
            item_unlink(it);
            WriteBinResponse(NULL, 0, 0, 0);
        } else {
            WriteBinError(eKeyExists);
        }
        item_remove(it);      /* release our reference */
    } else {
        WriteBinError(eKeyNotFound);
        m_pWorker->m_stats.m_deleteMisses += 1;
    }

	LinkForTransmit();

	return true;
}

bool CConn::ProcessCmdBinArith()
{
	CRspCmdQw * pRsp;
	if (!(pRsp = m_pWorker->GetHifRspCmd(CNY_CMD_BIN_ARITH_QW_CNT)))
		return false;

	CCnyCpCmd * pRspCmd = &pRsp->m_rspCmd;

	char * pKey;
	int keyLen;
    FindRecvBufString(pKey, keyLen, pRspCmd->m_keyPos, pRspCmd->m_keyLen, pRspCmd->m_keyInit);

    m_cmd = pRspCmd->m_cmd & 0x7f;
    m_opcode = pRspCmd->m_cmd & 0x7f;
	m_opaque = pRspCmd->m_opaque;
	m_noreply = pRspCmd->m_noreply;
	m_ascii = pRspCmd->m_ascii;
    m_cas = 0;

	uint32_t exptime = pRsp->m_rspDw[CNY_BIN_CMD_DW_CNT];
	uint64_t initial = *(uint64_t*)(void*)&pRsp->m_rspDw[CNY_BIN_CMD_DW_CNT+1];
	uint64_t delta = *(uint64_t*)(void*)&pRsp->m_rspDw[CNY_BIN_CMD_DW_CNT+3];
	uint64_t cas = *(uint64_t*)(void*)&pRsp->m_rspDw[CNY_BIN_CMD_DW_CNT+5];

	if (g_settings.m_verbose > 1) {
		fprintf(stderr, "Incr/Decr %.*s %lld, %llu, %d\n",
			keyLen, pKey,
            (long long)delta,
			(long long)initial,
			exptime);
	}

	AddXmitMsg();

	CMcdBinRspIncr * rsp = (CMcdBinRspIncr*) m_pXmitMsg->m_wrBuf;
    char tmpbuf[INCR_MAX_STORAGE_LEN];

    switch(add_delta(this, pKey, keyLen, pRspCmd->m_keyHash, m_cmd == eMcdBinCmdIncr,
                     delta, tmpbuf,
                     &cas)) {
    case OK:
        rsp->m_body.m_value = htonll(strtoull(tmpbuf, NULL, 10));
        if (cas) {
            m_cas = cas;
        }
        WriteBinResponse(&rsp->m_body, 0, 0,
                           sizeof(rsp->m_body.m_value));
        break;
    case NON_NUMERIC:
        WriteBinError(eBadDelta);
        break;
    case EOM:
        WriteBinError(eNoMemory);
        break;
    case DELTA_ITEM_NOT_FOUND:
        if (exptime != 0xffffffff) {
            /* Save some room for the response */
            rsp->m_body.m_value = htonll(initial);
            CItem *it = ItemAlloc(pKey, keyLen, pRspCmd->m_keyHash, 0, realtime(exptime),
                            INCR_MAX_STORAGE_LEN);

            if (it != NULL) {
                snprintf(ITEM_data(it), INCR_MAX_STORAGE_LEN, "%llu",
                         (unsigned long long)initial);

                if (store_item(it, CNY_CMD_ADD, this)) {
                    m_cas = ITEM_get_cas(it);
                    WriteBinResponse(&rsp->m_body, 0, 0, sizeof(rsp->m_body.m_value));
                } else {
                    WriteBinError(eNotStored);
                }
                item_remove(it);         /* release our reference */
            } else {
                WriteBinError(eNoMemory);
            }
        } else {
            //pthread_mutex_lock(&m_pWorker->m_stats.mutex);
            if (m_cmd == eMcdBinCmdIncr) {
                m_pWorker->m_stats.m_incrMisses++;
            } else {
                m_pWorker->m_stats.m_decrMisses++;
            }
            //pthread_mutex_unlock(&m_pWorker->m_stats.mutex);

            WriteBinError(eKeyNotFound);
        }
        break;
    case DELTA_ITEM_CAS_MISMATCH:
        WriteBinError(eKeyExists);
        break;
    }

	LinkForTransmit();

	return true;
}

bool CConn::ProcessCmdBinTouch()
{
	CRspCmdQw * pRsp;
	if (!(pRsp = m_pWorker->GetHifRspCmd(CNY_CMD_BIN_TOUCH_QW_CNT)))
		return false;

	CCnyCpCmd * pRspCmd = &pRsp->m_rspCmd;

	char * pKey;
	int keyLen;
    FindRecvBufString(pKey, keyLen, pRspCmd->m_keyPos, pRspCmd->m_keyLen, pRspCmd->m_keyInit);

    m_cmd = pRspCmd->m_cmd & 0x7f;
    m_opcode = pRspCmd->m_cmd & 0x7f;
	m_opaque = pRspCmd->m_opaque;
	m_noreply = pRspCmd->m_noreply;
	m_ascii = pRspCmd->m_ascii;
    m_cas = 0;

	uint32_t exptime = pRsp->m_rspDw[CNY_BIN_CMD_DW_CNT];

	AddXmitMsg();

    CMcdBinRspGet * rsp = (CMcdBinRspGet*) m_pXmitMsg->m_wrBuf;

    if (g_settings.m_verbose > 1) {
        /* May be GAT/GATQ/etc */
        fprintf(stderr, "<%d TOUCH %.*s\n", m_sfd, keyLen, pKey);
    }

	bool bGatkCmd = m_cmd == eMcdBinCmdGatk || m_cmd == eMcdBinCmdGatkq;

    CItem * it = ItemTouch(pKey, keyLen, pRspCmd->m_keyHash, realtime(exptime));

    if (it) {
        /* the length has two unnecessary bytes ("\r\n") */
        uint16_t keylen = 0;
        uint32_t bodylen = sizeof(rsp->m_body) + (it->nbytes - 2);

        item_update(it);
        m_pWorker->m_stats.m_touchCmds += 1;
        m_pWorker->m_stats.m_slabStats[it->slabs_clsid].m_touchHits++;

        if (m_cmd == eMcdBinCmdTouch) {
            bodylen -= it->nbytes - 2;
        } else if (bGatkCmd) {
            bodylen += keyLen;
            keylen = keyLen;
        }

        AddBinHeader(0, sizeof(rsp->m_body), keylen, bodylen);
        rsp->m_hdr.m_uniqueId = htonll(ITEM_get_cas(it));

        // add the flags
        rsp->m_body.m_flags = htonl(strtoul(ITEM_suffix(it), NULL, 10));
        AddIov(&rsp->m_body, sizeof(rsp->m_body));

        if (bGatkCmd) {
            AddIov(ITEM_key(it), keyLen);
        }

        /* Add the data minus the CRLF */
        if (m_cmd != eMcdBinCmdTouch) {
            AddIov(ITEM_data(it), it->nbytes - 2);
        }

        /* Remember this item so we can garbage collect it later */
		m_pMsgHdr->m_pItem = it;

    } else {
        m_pWorker->m_stats.m_touchCmds += 1;
        m_pWorker->m_stats.m_touchMisses += 1;

        if (!pRspCmd->m_noreply) {
            if (bGatkCmd) {
                char *ofs = m_pXmitMsg->m_wrBuf + sizeof(CMcdBinRspHdr);
                AddBinHeader(eKeyNotFound, 0, keyLen, keyLen);
                memcpy(ofs, pKey, keyLen);
                AddIov(ofs, keyLen);
            } else {
                WriteBinError(eKeyNotFound);
            }
        }
    }

    if (g_settings.detail_enabled) {
        stats_prefix_record_get(pKey, keyLen, NULL != it);
    }

	LinkForTransmit();

	return true;
}

bool CConn::ProcessCmdBinAppendPrepend()
{
	CRspCmdQw * pRsp;
	if (!(pRsp = m_pWorker->GetHifRspCmd(CNY_CMD_BIN_STORE_QW_CNT)))
		return false;

	CCnyCpCmd * pRspCmd = &pRsp->m_rspCmd;

	char * pKey;
	int keyLen;
    FindRecvBufString(pKey, keyLen, pRspCmd->m_keyPos, pRspCmd->m_keyLen, pRspCmd->m_keyInit);

	m_cmd = pRspCmd->m_cmd & 0x7f;
    m_opcode = pRspCmd->m_cmd & 0x7f;
	m_opaque = pRspCmd->m_opaque;
	uint32_t vlen = pRspCmd->m_valueLen;
	m_noreply = pRspCmd->m_noreply;
	m_ascii = pRspCmd->m_ascii;

	//uint32_t & flags = pRsp->m_rspDw[CNY_BIN_CMD_DW_CNT];
	//uint32_t & exptime = pRsp->m_rspDw[CNY_BIN_CMD_DW_CNT + 1];
    m_cas = 0;
    uint64_t cmdCas = *(uint64_t*)(void*)&pRsp->m_rspDw[CNY_BIN_CMD_DW_CNT + 2];

    if (g_settings.m_verbose > 1) {
        fprintf(stderr, "Value len is %d\n", vlen);
    }

    if (g_settings.detail_enabled) {
        stats_prefix_record_set(pKey, keyLen);
    }

    CItem * it = ItemAlloc(pKey, keyLen, pRspCmd->m_keyHash, 0, 0, vlen+2);
    m_pItem = it;

	AddXmitMsg();

    if (it == 0) {
        if (! item_size_ok(keyLen, 0, vlen + 2)) {
            WriteBinError(eValueToBig);
        } else {
            WriteBinError(eNoMemory);
        }
        return true;
    }

    ITEM_set_cas(it, cmdCas);

    switch (m_cmd) {
        case eMcdBinCmdAppend:
        case eMcdBinCmdAppendq:
            m_cmd = CNY_CMD_APPEND;
            break;
        case eMcdBinCmdPrepend:
        case eMcdBinCmdPrependq:
            m_cmd = CNY_CMD_PREPEND;
            break;
        default:
            assert(0);
    }

    m_pItemRd = ITEM_data(it);
    m_itemRdLen = vlen;

	return true;
}

bool CConn::ProcessCmdBinOther()
{
	CRspCmdQw * pRsp;
	if (!(pRsp = m_pWorker->GetHifRspCmd(CNY_CMD_BIN_OTHER_QW_CNT)))
		return false;

	CCnyCpCmd * pRspCmd = &pRsp->m_rspCmd;

	m_protocol = binary_prot;

    m_cmd = pRspCmd->m_cmd & 0x7f;
    m_opcode = (pRspCmd->m_cmd & 0x7f) | (pRspCmd->m_cmdMs << 7);
	m_opaque = pRspCmd->m_opaque;
	m_noreply = pRspCmd->m_noreply;
	m_ascii = pRspCmd->m_ascii;
    m_cas = 0;

	char * pKey;
	int keyLen;
    FindRecvBufString(pKey, keyLen, pRspCmd->m_keyPos, pRspCmd->m_keyLen, pRspCmd->m_keyInit);

	AddXmitMsg();

	switch (m_cmd) {
	case eMcdBinCmdQuit:
	case eMcdBinCmdQuitq:
        if (keyLen == 0) {
	if (!m_noreply)
	  WriteBinResponse(NULL, 0, 0, 0);
 			// send socket closed message through personality
			if (!m_bCloseReqSent && !IsUdp()) {

			    if (!m_noreply) {
                    LinkForTransmit();
                    AddXmitMsg();
                }

			    // must close after all responses have been transmitted
			    m_pXmitMsg->m_bCloseAfterXmit = true;
                assert(m_caxCode == 0);
                m_caxCode = 6;

                SendCloseToCoproc();
			}
        } else {
            HandleBinaryProtocolError(eInvalid);
        }
		break;
    case eMcdBinCmdVersion:
        if (keyLen == 0) {
            WriteBinResponse(MC_VERSION, 0, 0, strlen(MC_VERSION));
        } else {
            HandleBinaryProtocolError(eInvalid);
        }
        break;
    case eMcdBinCmdNoop:
        if (keyLen == 0) {
            WriteBinResponse(NULL, 0, 0, 0);
        } else {
            HandleBinaryProtocolError(eInvalid);
        }
        break;
    case eMcdBinCmdFlush:
        if (keyLen == 0 || keyLen == 4) {
			time_t exptime = 0;

			if (keyLen == sizeof(uint32_t)) {
				// Fixme - this can't be right, already did this above
				char * pKey;
				int keyLen;
                FindRecvBufString(pKey, keyLen, pRspCmd->m_keyPos, pRspCmd->m_keyLen, pRspCmd->m_keyInit);

				exptime = ntohl(*(uint32_t*)pKey);
			}

			if (exptime > 0) {
				g_settings.oldest_live = realtime(exptime) - 1;
			} else {
				g_settings.oldest_live = current_time - 1;
			}
			item_flush_expired();

			//pthread_mutex_lock(&m_pWorker->m_stats.mutex);
			m_pWorker->m_stats.m_flushCmds++;
			//pthread_mutex_unlock(&m_pWorker->m_stats.mutex);

			WriteBinResponse(NULL, 0, 0, 0);
        } else {
            HandleBinaryProtocolError(eInvalid);
        }
        break;
    case eMcdBinCmdStat:
		ProcessBinStat(pKey, keyLen);
		break;
	default:
        HandleBinaryProtocolError(eUnknownCmd);
		break;
	}

	LinkForTransmit();
	return true;
}

void CConn::ProcessBinStat(char * pKey, int keyLen) {
    char *subcommand = pKey;

    if (g_settings.m_verbose > 1)
        fprintf(stderr, "<%d STATS %.*s\n", m_sfd, keyLen, pKey);

	m_protocol = binary_prot;

    if (keyLen == 0) {
        /* request all statistics */
        ProcessServerStats();
        (void)get_stats(NULL, 0, this);
    } else if (strncmp(subcommand, "reset", 5) == 0) {
        stats_reset();
    } else if (strncmp(subcommand, "settings", 8) == 0) {
        ProcessSettingsStats();
    } else if (strncmp(subcommand, "detail", 6) == 0) {
        char *subcmd_pos = subcommand + 6;
        if (strncmp(subcmd_pos, " dump", 5) == 0) {
            int len;
            char *dump_buf = stats_prefix_dump(&len);
            if (dump_buf == NULL || len <= 0) {
                WriteBinError(eNoMemory);
                return ;
            } else {
                AddStats("detailed", strlen("detailed"), dump_buf, len);
                free(dump_buf);
            }
        } else if (strncmp(subcmd_pos, " on", 3) == 0) {
            g_settings.detail_enabled = 1;
        } else if (strncmp(subcmd_pos, " off", 4) == 0) {
            g_settings.detail_enabled = 0;
        } else {
            WriteBinError(eKeyNotFound);
            return;
        }
    } else {
        if (get_stats(subcommand, keyLen, this)) {
            if (m_stats.m_pBuffer == NULL) {
                WriteBinError(eNoMemory);
            } else {
                WriteAndFree(m_stats.m_pBuffer, m_stats.m_offset);
                m_stats.m_pBuffer = NULL;
            }
        } else {
            WriteBinError(eKeyNotFound);
        }

        return;
    }

    /* Append termination package and start the transfer */
    AddStats(NULL, 0, NULL, 0);
    if (m_stats.m_pBuffer == NULL) {
        WriteBinError(eNoMemory);
    } else {
        WriteAndFree(m_stats.m_pBuffer, m_stats.m_offset);
        m_stats.m_pBuffer = NULL;
    }
}

void CConn::CompleteBinStore()
{
    EMcdBinRspStatus eno = eInvalid;
    EStoreItemType ret = NOT_STORED;
	CItem *it;

    it = m_pItem;

    //pthread_mutex_lock(&m_pWorker->m_stats.mutex);
    m_pWorker->m_stats.m_slabStats[it->slabs_clsid].m_setCmds++;
    //pthread_mutex_unlock(&m_pWorker->m_stats.mutex);

    /* We don't actually receive the trailing two characters in the bin
     * protocol, so we're going to just set them here */
    *(ITEM_data(it) + it->nbytes - 2) = '\r';
    *(ITEM_data(it) + it->nbytes - 1) = '\n';

    ret = store_item(it, m_cmd, this);

#ifdef ENABLE_DTRACE
    uint64_t cas = ITEM_get_cas(it);
    switch (m_cmd) {
    case CNY_CMD_ADD:
        MEMCACHED_COMMAND_ADD(m_sfd, ITEM_key(it), it->nkey,
                              (ret == STORED) ? it->nbytes : -1, cas);
        break;
    case CNY_CMD_REPLACE:
        MEMCACHED_COMMAND_REPLACE(m_sfd, ITEM_key(it), it->nkey,
                                  (ret == STORED) ? it->nbytes : -1, cas);
        break;
    case CNY_CMD_APPEND:
        MEMCACHED_COMMAND_APPEND(m_sfd, ITEM_key(it), it->nkey,
                                 (ret == STORED) ? it->nbytes : -1, cas);
        break;
    case CNY_CMD_PREPEND:
        MEMCACHED_COMMAND_PREPEND(m_sfd, ITEM_key(it), it->nkey,
                                 (ret == STORED) ? it->nbytes : -1, cas);
        break;
    case CNY_CMD_SET:
        MEMCACHED_COMMAND_SET(m_sfd, ITEM_key(it), it->nkey,
                              (ret == STORED) ? it->nbytes : -1, cas);
        break;
    }
#endif

    switch (ret) {
    case STORED:
        /* Stored */
        WriteBinResponse(NULL, 0, 0, 0);
        break;
    case EXISTS:
        WriteBinError(eKeyExists);
        break;
    case NOT_FOUND:
        WriteBinError(eKeyNotFound);
        break;
    case NOT_STORED:
        if (m_cmd == CNY_CMD_ADD) {
            eno = eKeyExists;
        } else if(m_cmd == CNY_CMD_REPLACE) {
            eno = eKeyNotFound;
        } else {
            eno = eNotStored;
        }
        WriteBinError(eno);
    }

    item_remove(m_pItem);       /* release the m_pItem reference */
    m_pItem = 0;
}

/* Just write an error message and disconnect the client */
void CConn::HandleBinaryProtocolError(EMcdBinRspStatus err) {
    WriteBinError(err);
    switch(err) {
    case eUnknownCmd:
        if (g_settings.m_verbose) {
            fprintf(stderr, "<%d Protocol error (opcode %02x)\n", m_sfd, m_opcode);
        }
        break;
    case eInvalid:
        if (g_settings.m_verbose) {
            fprintf(stderr, "<%d Protocol error (opcode %02x), close connection\n", m_sfd, m_opcode);
        }

        // send socket closed message through personality
        if (!m_bCloseReqSent && !IsUdp()) {
            // must close after all responses have been transmitted
            LinkForTransmit();

            AddXmitMsg();
            m_pXmitMsg->m_bCloseAfterXmit = true;
            assert(m_caxCode == 0);
            m_caxCode = 1;

            SendCloseToCoproc();
        }
        break;
    default:
        assert(0);
    }
}

void CConn::WriteBinError(EMcdBinRspStatus err) {
    const char *errstr = "Unknown error";
    uint32_t len;

    switch (err) {
    case eNoMemory:
        errstr = "Out of memory";
        break;
    case eUnknownCmd:
        errstr = "Unknown command";
        break;
    case eKeyNotFound:
        errstr = "Not found";
        break;
    case eInvalid:
        errstr = "Invalid arguments";
        break;
    case eKeyExists:
        errstr = "Data exists for key.";
        break;
    case eValueToBig:
        errstr = "Too large.";
        break;
    case eBadDelta:
        errstr = "Non-numeric server-side value for incr or decr";
        break;
    case eNotStored:
        errstr = "Not stored.";
        break;
    default:
        assert(false);
        errstr = "UNHANDLED ERROR";
        fprintf(stderr, ">%d UNHANDLED ERROR: %d\n", m_sfd, err);
    }

    if (g_settings.m_verbose > 1) {
        fprintf(stderr, ">%d Writing an error: %s\n", m_sfd, errstr);
    }

    len = (uint32_t)strlen(errstr);
    AddBinHeader(err, 0, 0, len);
    if (len > 0) {
        AddIov(errstr, len);
    }
}

void CConn::WriteBinResponse(const void *d, size_t hlen, size_t keylen, size_t dlen) {
    if (!m_noreply || m_cmd == eMcdBinCmdGet || m_cmd == eMcdBinCmdGetk) {
        AddBinHeader(0, hlen, keylen, dlen);
        if(dlen > 0) {
            AddIov(d, dlen);
        }
    }
}



///////////////////////////////// Routines common to ASCII and Binary ////////////////////

CConn * NewConn(const int m_sfd, bool bListening, const int event_flags,
                const int read_buffer_size, ETransport transport, event_base * pEventBase, CWorker *pWorker) {

    CConn *c = NULL;
	
	if (pWorker)
		c = pWorker->ConnFromFreelist();

    if (NULL == c) {
		if (pWorker && pWorker->m_connAllocCnt == CNY_MAX_UNIT_CONN_CNT) {
			// we reached the maximum unit connection count (hard limit).
			__sync_fetch_and_add(&g_stats.m_rejectedConns, 1);
			return NULL;
		}

		if (pWorker)
			c = (CConn*) pWorker->Calloc(1, sizeof(CConn));
		else
			c = (CConn*) calloc(1, sizeof(CConn));

		if (!c) {
			fprintf(stderr, "calloc()\n");
			return NULL;
		}

		if (pWorker) {
			c->m_connId = pWorker->m_connAllocCnt++;
		}

        __sync_fetch_and_add(&g_stats.m_connStructs, 1);
    }

    c->m_transport = transport;
    c->m_protocol = g_settings.binding_protocol;
	c->m_bListening = bListening;

	if (!bListening)
		pWorker->ProcessNewConn(c);

    /* unix socket mode doesn't need this, so zeroed out.  but why
     * is this done for every command?  presumably for UDP
     * mode.  */
 
    if (g_settings.m_verbose > 1) {
        if (bListening) {
            fprintf(stderr, "<%d server listening (%s)\n", m_sfd,
                ProtocolText(c->m_protocol));
        } else if (transport == udp_transport) {
            fprintf(stderr, "<%d server listening (udp)\n", m_sfd);
        } else if (c->m_protocol == negotiating_prot) {
            fprintf(stderr, "<%d new auto-negotiating client connection\n",
                    m_sfd);
        } else if (c->m_protocol == ascii_prot) {
            fprintf(stderr, "<%d new ascii client connection.\n", m_sfd);
        } else if (c->m_protocol == binary_prot) {
            fprintf(stderr, "<%d new binary client connection.\n", m_sfd);
        } else {
            fprintf(stderr, "<%d new unknown (%d) client connection\n",
                m_sfd, c->m_protocol);
            assert(false);
        }
    }

	assert(c->m_sfd == 0);
	assert(c->m_pXmitMsg == 0);

    c->m_sfd = m_sfd;
	c->m_bXmitActive = false;
    c->m_itemRdLen = 0;
    c->m_cmd = -1;
    c->m_pItemRd = 0;

    c->m_pItem = 0;

    c->m_noreply = false;
    c->m_partialLen = 0;

    if (event_flags != 0) {	
	event_set(&c->m_event, m_sfd, event_flags, CWorker::EventHandler, (void *)c);

	if (pWorker && g_settings.powersave) {
	    // Powersave mode - let whichever Worker is responsible for this
	    // connection update his own event_base the next time he runs.
	    pthread_mutex_lock(&pWorker->m_psPendLock);
	    c->m_psPendNext = pWorker->m_psPendHead;
	    pWorker->m_psPendHead = c;
	    pthread_mutex_unlock(&pWorker->m_psPendLock);

	    c->m_bEventActive = false;
	} else {
	    event_base_set(pEventBase, &c->m_event);

	    if (event_add(&c->m_event, 0) == -1) {
		if (pWorker)
		    pWorker->ConnAddToFreelist(c);
		perror("event_add");
		return NULL;
	    }

	    c->m_bEventActive = true;
	}
    } else {
	c->m_bEventActive = false;
    }

    c->m_eventFlags = event_flags;
	c->m_bFree = false;
	c->m_bCloseReqSent = false;

    c->m_caxCode = 0;

    __sync_fetch_and_add(&g_stats.m_currConns, 1);
    __sync_fetch_and_add(&g_stats.m_totalConns, 1);

    return c;
}

/*
 * Shrinks a connection's buffers if they're too big.  This prevents
 * periodic large "get" requests from permanently chewing lots of server
 * memory.
 *
 * This should only be called in between requests since it can wipe output
 * buffers!
 */
#ifdef NEVER	// unused
static void conn_shrink(CConn *c) {
    assert(c != NULL);

    if (c->IsUdp())
        return;
}
#endif

/*
 * Stores an item in the cache according to the semantics of one of the set
 * commands. In threaded mode, this is protected by the cache lock.
 *
 * Returns the state of storage.
 */
EStoreItemType CConn::DoStoreItem(CItem *it, int comm)
{
    char *key = ITEM_key(it);
    CItem *old_it = do_item_get(key, it->nkey, it->hv);
    EStoreItemType stored = NOT_STORED;

    CItem *new_it = NULL;
    int flags;

    if (old_it != NULL && comm == CNY_CMD_ADD) {
        /* add only adds a nonexistent item, but promote to head of LRU */
        do_item_update(old_it);
    } else if (!old_it && (comm == CNY_CMD_REPLACE
        || comm == CNY_CMD_APPEND || comm == CNY_CMD_PREPEND))
    {
        /* replace only replaces an existing value; don't store */
    } else if (comm == CNY_CMD_CAS) {
        /* validate cas operation */
        if(old_it == NULL) {
            // LRU expired
            stored = NOT_FOUND;
            //pthread_mutex_lock(&m_pWorker->m_stats.mutex);
            m_pWorker->m_stats.m_casMisses++;
            //pthread_mutex_unlock(&m_pWorker->m_stats.mutex);
        }
        else if (ITEM_get_cas(it) == ITEM_get_cas(old_it)) {
            // cas validates
            // it and old_it may belong to different classes.
            // I'm updating the stats for the one that's getting pushed out
            //pthread_mutex_lock(&m_pWorker->m_stats.mutex);
            m_pWorker->m_stats.m_slabStats[old_it->slabs_clsid].m_casHits++;
            //pthread_mutex_unlock(&m_pWorker->m_stats.mutex);

            item_replace(old_it, it);
            stored = STORED;
        } else {
            //pthread_mutex_lock(&m_pWorker->m_stats.mutex);
            m_pWorker->m_stats.m_slabStats[old_it->slabs_clsid].m_casBadVal++;
            //pthread_mutex_unlock(&m_pWorker->m_stats.mutex);

            if(g_settings.m_verbose > 1) {
                fprintf(stderr, "CAS:  failure: expected %llu, got %llu\n",
                        (unsigned long long)ITEM_get_cas(old_it),
                        (unsigned long long)ITEM_get_cas(it));
            }
            stored = EXISTS;
        }
    } else {
        /*
         * Append - combine new and old record into single one. Here it's
         * atomic and thread-safe.
         */
        if (comm == CNY_CMD_APPEND || comm == CNY_CMD_PREPEND) {
            /*
             * Validate CAS
             */
            if (ITEM_get_cas(it) != 0) {
                // CAS much be equal
                if (ITEM_get_cas(it) != ITEM_get_cas(old_it)) {
                    stored = EXISTS;
                }
            }

            if (stored == NOT_STORED) {
                /* we have it and old_it here - alloc memory to hold both */
                /* flags was already lost - so recover them from ITEM_suffix(it) */

                flags = (int) strtol(ITEM_suffix(old_it), (char **) NULL, 10);

                new_it = do_item_alloc(key, it->nkey, it->hv, flags, old_it->exptime, it->nbytes + old_it->nbytes - 2 /* CRLF */);

                if (new_it == NULL) {
                    /* SERVER_ERROR out of memory */
                    if (old_it != NULL)
                        do_item_remove(old_it);

                    return NOT_STORED;
                }

                /* copy data from it and old_it to new_it */

                if (comm == CNY_CMD_APPEND) {
                    memcpy(ITEM_data(new_it), ITEM_data(old_it), old_it->nbytes);
                    memcpy(ITEM_data(new_it) + old_it->nbytes - 2 /* CRLF */, ITEM_data(it), it->nbytes);
                } else {
                    /* CNY_CMD_PREPEND */
                    memcpy(ITEM_data(new_it), ITEM_data(it), it->nbytes);
                    memcpy(ITEM_data(new_it) + it->nbytes - 2 /* CRLF */, ITEM_data(old_it), old_it->nbytes);
                }

                it = new_it;
            }
        }

        if (stored == NOT_STORED) {
            if (old_it != NULL) {
                item_replace(old_it, it);
            } else {
                do_item_link(it);
            }

            //m_cas = ITEM_get_cas(it);

            stored = STORED;
        }
    }

    if (old_it != NULL)
        do_item_remove(old_it);         /* release our reference */
    if (new_it != NULL)
        do_item_remove(new_it);

    if (stored == STORED) {
        m_cas = ITEM_get_cas(it);
    }

    return stored;
}

/*
 * adds a delta value to a numeric item.
 *
 * c     connection requesting the operation
 * it    item to adjust
 * incr  true to increment value, false to decrement
 * delta amount to adjust value by
 * buf   buffer for response string
 *
 * returns a response string to send back to the client.
 */
EDeltaResultType CConn::DoAddDelta(const char *key, const size_t nkey,
                                    const uint32_t hv, const bool incr, const uint64_t delta,
                                    char *buf, uint64_t *cas)
{
    char *ptr;
    uint64_t value;
    size_t res;
    CItem *it;

    it = do_item_get(key, nkey, hv);
    if (!it) {
        return DELTA_ITEM_NOT_FOUND;
    }

    if (cas != NULL && *cas != 0 && ITEM_get_cas(it) != *cas) {
        do_item_remove(it);
        return DELTA_ITEM_CAS_MISMATCH;
    }

    ptr = ITEM_data(it);

    if (!safe_strtoull(ptr, &value)) {
        do_item_remove(it);
        return NON_NUMERIC;
    }

    if (incr) {
        value += delta;
    } else {
        if(delta > value) {
            value = 0;
        } else {
            value -= delta;
        }
    }
    if (incr) {
        m_pWorker->m_stats.m_slabStats[it->slabs_clsid].m_incrHits++;
    } else {
        m_pWorker->m_stats.m_slabStats[it->slabs_clsid].m_decrHits++;
    }

    snprintf(buf, INCR_MAX_STORAGE_LEN, "%llu", (unsigned long long)value);
    res = strlen(buf);
    if (res + 2 > (size_t)it->nbytes || it->refcount != 1) { /* need to realloc */
        CItem *new_it;
        new_it = do_item_alloc(ITEM_key(it), it->nkey, it->hv, atoi(ITEM_suffix(it) + 1), it->exptime, (int)(res + 2));
        if (new_it == 0) {
            do_item_remove(it);
            return EOM;
        }
		new_it->hv = hv;
        memcpy(ITEM_data(new_it), buf, res);
        memcpy(ITEM_data(new_it) + res, "\r\n", 2);
        item_replace(it, new_it);
        // Overwrite the older item's CAS with our new CAS since we're
        // returning the CAS of the old item below.
        ITEM_set_cas(it, (g_settings.use_cas) ? ITEM_get_cas(new_it) : 0);
        do_item_remove(new_it);       /* release our reference */
    } else { /* replace in-place */
        /* When changing the value without replacing the item, we
           need to update the CAS on the existing item. */
        mutex_lock(&cache_lock); /* FIXME */
        ITEM_set_cas(it, (g_settings.use_cas) ? get_cas_id() : 0);
        mutex_unlock(&cache_lock);

        memcpy(ITEM_data(it), buf, res);
        memset(ITEM_data(it) + res, ' ', it->nbytes - res - 2);
        do_item_update(it);
    }

    if (cas) {
        *cas = ITEM_get_cas(it);    /* swap the incoming CAS value */
    }
    do_item_remove(it);         /* release our reference */
    return OK;
}

///////////////////////////////// Stats routines ////////////////////

void CConn::ProcessSettingsStats() {
    AppendStat("maxbytes", "%u", (unsigned int)g_settings.maxbytes);
    AppendStat("maxconns", "%d", g_settings.m_maxConns);
    AppendStat("tcpport", "%d", g_settings.port);
    AppendStat("udpport", "%d", g_settings.udpport);
    AppendStat("inter", "%s", g_settings.inter ? g_settings.inter : "NULL");
    AppendStat("verbosity", "%d", g_settings.m_verbose);
    AppendStat("oldest", "%lu", (unsigned long)g_settings.oldest_live);
    AppendStat("evictions", "%s", g_settings.evict_to_free ? "on" : "off");
    AppendStat("domain_socket", "%s",
                g_settings.socketpath ? g_settings.socketpath : "NULL");
    AppendStat("umask", "%o", g_settings.access);
    AppendStat("growth_factor", "%.2f", g_settings.factor);
    AppendStat("chunk_size", "%d", g_settings.chunk_size);
    AppendStat("num_threads", "%d", g_settings.num_threads);
    AppendStat("num_threads_per_udp", "%d", g_settings.num_threads_per_udp);
    AppendStat("stat_key_prefix", "%c", g_settings.prefix_delimiter);
    AppendStat("detail_enabled", "%s",
                g_settings.detail_enabled ? "yes" : "no");
    AppendStat("reqs_per_event", "%d", g_settings.reqs_per_event);
    AppendStat("cas_enabled", "%s", g_settings.use_cas ? "yes" : "no");
    AppendStat("tcp_backlog", "%d", g_settings.backlog);
    AppendStat("binding_protocol", "%s",
                ProtocolText(g_settings.binding_protocol));
    AppendStat("item_size_max", "%d", g_settings.item_size_max);
    AppendStat("maxconns_fast", "%s", g_settings.maxconns_fast ? "yes" : "no");
    AppendStat("hashpower_init", "%d", g_settings.hashpower_init);
    AppendStat("slab_reassign", "%s", g_settings.slab_reassign ? "yes" : "no");
    AppendStat("slab_automove", "%d", g_settings.slab_automove);
}

/* return server specific stats only */
void CConn::ProcessServerStats() {
    pid_t pid = getpid();
    rel_time_t now = current_time;

    CThreadStats thread_stats;
    CSlabStats slab_stats;

    ThreadStatsAggregate(&thread_stats);
    SlabStatsAggregate(&thread_stats, &slab_stats);

#ifndef WIN32
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
#endif /* !WIN32 */

    STATS_LOCK();

	AppendStat("pid", "%lu", (long)pid);
    AppendStat("pid", "%lu", (long)pid);
    AppendStat("uptime", "%u", now);
    AppendStat("time", "%ld", now + (long)g_processStartTime);
    AppendStat("version", "%s", MC_VERSION);
    AppendStat("libevent", "%s", event_get_version());
    AppendStat("pointer_size", "%d", (int)(8 * sizeof(void *)));

#ifndef WIN32
    AppendStat("rusage_user", "%ld.%06ld",
                (long)usage.ru_utime.tv_sec,
                (long)usage.ru_utime.tv_usec);
    AppendStat("rusage_system", "%ld.%06ld",
                (long)usage.ru_stime.tv_sec,
                (long)usage.ru_stime.tv_usec);
#endif /* !WIN32 */

    AppendStat("curr_connections", "%u", g_stats.m_currConns - 1);
    AppendStat("total_connections", "%u", g_stats.m_totalConns);
    if (g_settings.maxconns_fast) {
        AppendStat("rejected_connections", "%llu", (unsigned long long)g_stats.m_rejectedConns);
    }
    AppendStat("connection_structures", "%u", g_stats.m_connStructs);
    AppendStat("reserved_fds", "%u", g_stats.m_reservedFds);
    AppendStat("cmd_get", "%llu", (unsigned long long)thread_stats.m_getCmds);
    AppendStat("cmd_set", "%llu", (unsigned long long)slab_stats.m_setCmds);
    AppendStat("cmd_flush", "%llu", (unsigned long long)thread_stats.m_flushCmds);
    AppendStat("cmd_touch", "%llu", (unsigned long long)thread_stats.m_touchCmds);
    AppendStat("get_hits", "%llu", (unsigned long long)slab_stats.m_getHits);
    AppendStat("get_misses", "%llu", (unsigned long long)thread_stats.m_getMisses);
    AppendStat("delete_misses", "%llu", (unsigned long long)thread_stats.m_deleteMisses);
    AppendStat("delete_hits", "%llu", (unsigned long long)slab_stats.m_deleteHits);
    AppendStat("incr_misses", "%llu", (unsigned long long)thread_stats.m_incrMisses);
    AppendStat("incr_hits", "%llu", (unsigned long long)slab_stats.m_incrHits);
    AppendStat("decr_misses", "%llu", (unsigned long long)thread_stats.m_decrMisses);
    AppendStat("decr_hits", "%llu", (unsigned long long)slab_stats.m_decrHits);
    AppendStat("cas_misses", "%llu", (unsigned long long)thread_stats.m_casMisses);
    AppendStat("cas_hits", "%llu", (unsigned long long)slab_stats.m_casHits);
    AppendStat("cas_badval", "%llu", (unsigned long long)slab_stats.m_casBadVal);
    AppendStat("touch_hits", "%llu", (unsigned long long)slab_stats.m_touchHits);
    AppendStat("touch_misses", "%llu", (unsigned long long)thread_stats.m_touchMisses);
    AppendStat("auth_cmds", "%llu", (unsigned long long)thread_stats.m_authCmds);
    AppendStat("auth_errors", "%llu", (unsigned long long)thread_stats.m_authErrors);
    AppendStat("bytes_read", "%llu", (unsigned long long)thread_stats.m_bytesRead);
    AppendStat("bytes_written", "%llu", (unsigned long long)thread_stats.m_bytesWritten);
    AppendStat("limit_maxbytes", "%llu", (unsigned long long)g_settings.maxbytes);
    AppendStat("accepting_conns", "%u", g_stats.m_bAcceptingConns);
    AppendStat("listen_disabled_num", "%llu", (unsigned long long)g_stats.m_listenDisabledNum);
    AppendStat("threads", "%d", g_settings.num_threads);
    AppendStat("conn_yields", "%llu", (unsigned long long)thread_stats.m_connYields);
    AppendStat("hash_power_level", "%u", g_stats.m_hashPowerLevel);
    AppendStat("hash_bytes", "%llu", (unsigned long long)g_stats.m_hashBytes);
    AppendStat("hash_is_expanding", "%u", g_stats.m_bHashIsExpanding);
    if (g_settings.slab_reassign) {
        AppendStat("slab_reassign_running", "%u", g_stats.m_bSlabReassignRunning);
        AppendStat("slabs_moved", "%llu", g_stats.m_slabsMoved);
    }
    STATS_UNLOCK();
}

void CConn::ThreadStatsAggregate(CThreadStats *stats) {
    int ii, sid;

    /* The struct has a mutex, but we can safely set the whole thing
     * to zero since it is unused when aggregating. */
    memset(stats, 0, sizeof(*stats));

    for (ii = 0; ii < g_settings.num_threads; ++ii) {
        //pthread_mutex_lock(&g_pWorkers[ii]->m_stats.mutex);

        stats->m_getCmds += g_pWorkers[ii]->m_stats.m_getCmds;
        stats->m_getMisses += g_pWorkers[ii]->m_stats.m_getMisses;
        stats->m_touchCmds += g_pWorkers[ii]->m_stats.m_touchCmds;
        stats->m_touchMisses += g_pWorkers[ii]->m_stats.m_touchMisses;
        stats->m_deleteMisses += g_pWorkers[ii]->m_stats.m_deleteMisses;
        stats->m_decrMisses += g_pWorkers[ii]->m_stats.m_decrMisses;
        stats->m_incrMisses += g_pWorkers[ii]->m_stats.m_incrMisses;
        stats->m_casMisses += g_pWorkers[ii]->m_stats.m_casMisses;
        stats->m_bytesRead += g_pWorkers[ii]->m_stats.m_bytesRead;
        stats->m_bytesWritten += g_pWorkers[ii]->m_stats.m_bytesWritten;
        stats->m_flushCmds += g_pWorkers[ii]->m_stats.m_flushCmds;
        stats->m_connYields += g_pWorkers[ii]->m_stats.m_connYields;
        stats->m_authCmds += g_pWorkers[ii]->m_stats.m_authCmds;
        stats->m_authErrors += g_pWorkers[ii]->m_stats.m_authErrors;

        for (sid = 0; sid < MAX_NUMBER_OF_SLAB_CLASSES; sid++) {
            stats->m_slabStats[sid].m_setCmds +=
                g_pWorkers[ii]->m_stats.m_slabStats[sid].m_setCmds;
            stats->m_slabStats[sid].m_getHits +=
                g_pWorkers[ii]->m_stats.m_slabStats[sid].m_getHits;
            stats->m_slabStats[sid].m_touchHits +=
                g_pWorkers[ii]->m_stats.m_slabStats[sid].m_touchHits;
            stats->m_slabStats[sid].m_deleteHits +=
                g_pWorkers[ii]->m_stats.m_slabStats[sid].m_deleteHits;
            stats->m_slabStats[sid].m_decrHits +=
                g_pWorkers[ii]->m_stats.m_slabStats[sid].m_decrHits;
            stats->m_slabStats[sid].m_incrHits +=
                g_pWorkers[ii]->m_stats.m_slabStats[sid].m_incrHits;
            stats->m_slabStats[sid].m_casHits +=
                g_pWorkers[ii]->m_stats.m_slabStats[sid].m_casHits;
            stats->m_slabStats[sid].m_casBadVal +=
                g_pWorkers[ii]->m_stats.m_slabStats[sid].m_casBadVal;
        }

        //pthread_mutex_unlock(&g_pWorkers[ii]->m_stats.mutex);
    }
}

void CConn::SlabStatsAggregate(CThreadStats *stats, CSlabStats *out)
{
    int sid;

    out->m_setCmds = 0;
    out->m_getHits = 0;
    out->m_touchHits = 0;
    out->m_deleteHits = 0;
    out->m_incrHits = 0;
    out->m_decrHits = 0;
    out->m_casHits = 0;
    out->m_casBadVal = 0;

    for (sid = 0; sid < MAX_NUMBER_OF_SLAB_CLASSES; sid++) {
        out->m_setCmds += stats->m_slabStats[sid].m_setCmds;
        out->m_getHits += stats->m_slabStats[sid].m_getHits;
        out->m_touchHits += stats->m_slabStats[sid].m_touchHits;
        out->m_deleteHits += stats->m_slabStats[sid].m_deleteHits;
        out->m_decrHits += stats->m_slabStats[sid].m_decrHits;
        out->m_incrHits += stats->m_slabStats[sid].m_incrHits;
        out->m_casHits += stats->m_slabStats[sid].m_casHits;
        out->m_casBadVal += stats->m_slabStats[sid].m_casBadVal;
    }
}

void CConn::AppendStat(const char *name, const char *fmt, ...)
{
    char val_str[STAT_VAL_LEN];
    int vlen;
    va_list ap;

    assert(name);
    assert(this);
    assert(fmt);

    va_start(ap, fmt);
    vlen = vsnprintf(val_str, sizeof(val_str) - 1, fmt, ap);
    va_end(ap);

    AddStats(name, strlen(name), val_str, vlen);
}

void CConn::AddStats(const char *key, const size_t klen,
                  const char *val, const size_t vlen)
{
    /* value without a key is invalid */
    if (klen == 0 && vlen > 0) {
        return ;
    }

    if (m_protocol == binary_prot) {
        size_t needed = vlen + klen + sizeof(CMcdBinRspHdr);
        if (!GrowStatsBuf(needed)) {
            return;
        }
        AppendBinStats(key, klen, val, vlen);
    } else {
        size_t needed = vlen + klen + 10; // 10 == "STAT = \r\n"
        if (!GrowStatsBuf(needed)) {
            return;
        }
        AppendAsciiStats(key, klen, val, vlen);
    }

    assert(m_stats.m_offset <= m_stats.m_size);
}

void CConn::AppendBinStats(const char *key, const size_t klen, const char *val, const size_t vlen)
{
    char *buf = m_stats.m_pBuffer + m_stats.m_offset;
    size_t bodylen = klen + vlen;
    CMcdBinRspHdr header;
    header.m_magic = (uint8_t)MCD_BIN_RSP_MAGIC;
    header.m_opcode = eMcdBinCmdStat;
	header.m_extraLen = 0;
    header.m_keyLen = (uint16_t)htons((uint16_t)klen);
    header.m_dataType = (uint8_t)eRawBytes;
    header.m_bodyLen = htonl((uint32_t)bodylen);
    header.m_opaque = m_opaque;
	header.m_status = 0;

    memcpy(buf, header.m_bytes, sizeof(CMcdBinRspHdr));
    buf += sizeof(CMcdBinRspHdr);

    if (klen > 0) {
        memcpy(buf, key, klen);
        buf += klen;

        if (vlen > 0) {
            memcpy(buf, val, vlen);
        }
    }

    m_stats.m_offset += sizeof(CMcdBinRspHdr) + bodylen;
}

void CConn::AppendAsciiStats(const char *key, const size_t klen, const char *val, const size_t vlen)
{
    char *pos = m_stats.m_pBuffer + m_stats.m_offset;
    size_t nbytes = 0;
    size_t remaining = m_stats.m_size - m_stats.m_offset;
    size_t room = remaining - 1;

    if (klen == 0 && vlen == 0) {
        nbytes = snprintf(pos, room, "END\r\n");
    } else if (vlen == 0) {
        nbytes = snprintf(pos, room, "STAT %s\r\n", key);
    } else {
        nbytes = snprintf(pos, room, "STAT %s %s\r\n", key, val);
    }

    m_stats.m_offset += nbytes;
}

bool CConn::GrowStatsBuf(size_t needed) {
    size_t nsize = m_stats.m_size;
    size_t available = nsize - m_stats.m_offset;
    bool rv = true;

    /* Special case: No buffer -- need to allocate fresh */
    if (m_stats.m_pBuffer == NULL) {
        nsize = 1024;
        available = m_stats.m_size = m_stats.m_offset = 0;
    }

    while (needed > available) {
        assert(nsize > 0);
        nsize = nsize << 1;
        available = nsize - m_stats.m_offset;
    }

    if (nsize != m_stats.m_size) {
        char *ptr = (char *)realloc(m_stats.m_pBuffer, nsize);
        if (ptr) {
            m_stats.m_pBuffer = ptr;
            m_stats.m_size = nsize;
        } else {
            rv = false;
        }
    }

    return rv;
}
