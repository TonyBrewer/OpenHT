/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT memcached application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "Ht.h"
#include "PersCtl.h"

void
CPersCtl::PersCtl()
{

    if (!RecvMsgBusy_blkFinish() && RecvMsg_blkFinish()) {
        assert(S_blkActCnt!=0);
		S_blkActCnt -= 1;
    }
    HostBlk2Qw blkInfo;
    blkInfo.m_qw = m_blkList.front();
    pkt_cnt_t blkPktCnt = (pkt_cnt_t)blkInfo.m_hb.m_recvBlkPktCnt;
    pkt_offset_t blkPktOffset = (pkt_offset_t)blkInfo.m_hb.m_recvBufBlkIdx;

    //m_rdPktHdr.read_addr((pkt_hdr_qw_t)(P_cnt >> 1));
    //uint64_t rdPktHdr = m_rdPktHdr.read_mem();
    uint64_t rdPktHdr = GR_rdPktHdr_qw();
	uint16_t pktLen = (P_cnt & 1) ?
			  (uint16_t)(rdPktHdr >> 48) :
			  (uint16_t)(rdPktHdr >> 16);
	uint16_t pktCid = (P_cnt & 1) ?
			  (uint16_t)(rdPktHdr >> 32) :
			  (uint16_t)(rdPktHdr >> 0);
    bool pktNoBytes = (pktCid & 0x8000) != 0 || pktLen == 0x0000;
    uint16_t tPktLen = pktLen + 7;
    ht_uint4 pktQWords = ((tPktLen & 0xffc0) != 0) ? (ht_uint4)8 :
                                                   (ht_uint4)((tPktLen>>3)&0x7);
    // The above should work, but causes a memory miscompare in pkt, not debuging for now
    pktQWords = 8;

    sch_thr_idx_t cidIdx = cid2que(pktCid);

    bool bSchQueFull = S_schQueCnt[cidIdx] == 0;

    bool bPrefetchBufFull = S_prefetchBufMask == 0;
    prefetch_idx_t nextPrefetchBuf;
    tzc64(S_prefetchBufMask, nextPrefetchBuf);
    bool bTakePrefetchBuf = false;
    m_prefetchIdx.write_addr((pkt_hdr_idx_t)P_cnt);
    m_prefetchIdx.read_addr((pkt_hdr_idx_t)P_cnt);

    if (!RecvMsgBusy_schPop()) {
        ht_uint4 idx = RecvMsg_schPop();
        S_schQueCnt[idx] += 1;
    }

    m_pktAddr.write_addr((pkt_hdr_idx_t)P_cnt);
	m_pktAddr.read_addr((pkt_hdr_idx_t)P_cnt);
    //m_rdPktDat0.read_addr((pkt_hdr_idx_t)P_cnt);
    //m_rdPktDat1.read_addr((pkt_hdr_idx_t)P_cnt);

    MemAddr_t c_pRecvBuffOffset = (MemAddr_t)(SR_pRecvBufBase & 0xfffffffffff00000ll);

	if (PR_htValid) {
#ifndef _HTV
		extern FILE *tfp;
        if (false && tfp && !m_blkList.empty()) {
            fprintf(tfp, "CTL: cmd=%d pRecvBufBase=0x%012llx blkPktCnt=%d blkPktOffset=0x%x cnt=%d pktNum=%d actCnt=%d @ %lld\n",
                    (int)PR_htInst,
                    (long long)SR_pRecvBufBase,
                    (int)blkPktCnt, (int)blkPktOffset,
                    (int)P_cnt,
				(int)P_pktNum, (int)S_blkActCnt,
				(long long)sc_time_stamp().value() / 10000);
			fflush(tfp);
		}
#endif

		switch (PR_htInst) {
		case CTL_ENTRY: {
            P_blkInfo.m_blkIdx = 0;
            P_pBlkBase = 0;
            P_pBlk = 0;
			P_cnt = 0;
            P_hdrAddr = 0;

			HtContinue(CTL_SPAWN);
		}
		break;
		case CTL_SPAWN: {
			if (SendCallBusy_sch() || SendCallBusy_acc()) {
				HtRetry();
				break;
			}

			SendCallFork_sch(CTL_JOIN);

			if (P_cnt < 8)
				SendCallFork_acc(CTL_JOIN);

			P_cnt += 1;

			if (P_cnt == SCH_THREAD_CNT) {
                HtContinue(CTL_BLK_START);
			} else
				HtContinue(CTL_SPAWN);
		}
		break;
        case CTL_BLK_START: {
			// wait for a block
			if (m_blkList.empty()) {
				HtRetry();
				break;
			}

            if (blkPktCnt == 0) {
				m_blkList.pop();

				HtContinue(CTL_RETURN);
				break;
            }

            // check that the buffer base is naturely alligned
            assert((SR_pRecvBufBase & CTL_PKT_OFFSET_MASK) == 0);
            assert((blkPktOffset & 0x1f) == 0);
            // P_pBlkBase = (MemAddr_t)SR_pRecvBufBase + (MemAddr_t)blkPktOffset;
            P_pBlkBase = blkPktOffset;
            P_pBlk = P_pBlkBase;


			P_bStart = true;
			S_blkActCnt += 1;

			P_pktNum = 0;
            P_cnt = 0;
            P_hdrAddr = 0;
            for (int i=0; i<16; i++)
                P_blkInfo.m_pktCnt[i]=0;

			HtContinue(CTL_PKT_HDR_RD);
		}
		break;
		case CTL_PKT_HDR_RD: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

            assert((P_pBlk - P_pBlkBase) <= CTL_BLK_SIZE);

            //fprintf(stderr,"CTL_PKT_HDR_RD: readAddr=0x%012llx\n",
            //        (long long)P_pBlk);
            // read all 8 even if remaining pktcnt is less
            ReadMem_rdPktHdr(c_pRecvBuffOffset | (MemAddr_t)P_pBlk, 0, 8);

            P_cnt = 0;
            P_hdrAddr = 0;
            P_pBlk = (P_pBlk + CNY_RECV_BUF_ALIGN) & ~(CNY_RECV_BUF_ALIGN-1);
            ReadMemPause(CTL_PKT_RD);
		}
		break;
        case CTL_PKT_RD: {
#ifndef _HTV
            extern FILE *tfp;
            if (false && tfp && bPrefetchBufFull) {
                fprintf(tfp, "CTL: CTL_PKT_RD blkIdx=%d bPrefetchBufFull=%d @ %lld\n",
                        (int)P_blkInfo.m_blkIdx,
                        (int)bPrefetchBufFull,
                    (long long)sc_time_stamp().value() / 10000);
                fflush(tfp);
            }
#endif
            bool blkActBusy = S_blkActCnt > 16;
            if (ReadMemBusy() || SendMsgBusy_blkStart() ||
                    bPrefetchBufFull || blkActBusy) {
#ifndef _HTV
                if (false && bPrefetchBufFull)
                    fprintf(stderr,"CTL: blkIdx=%d prefetch buffer full  @ %lld\n",
                            (int)P_blkInfo.m_blkIdx,
                            (long long)sc_time_stamp().value() / 10000);
                if (false && blkActBusy)
                    fprintf(stderr,"CTL: blkIdx=%d blkPktCnt=%d blkActCnt > 16  @ %lld\n",
                            (int)P_blkInfo.m_blkIdx,
                            (int)blkPktCnt,
                            (long long)sc_time_stamp().value() / 10000);
#endif
                HtRetry();
                break;
			}

			if (P_bStart) {
//				assert(CTL_PKT_CNT >= (uint32_t)S_rdBlkHdr);
//				assert((uint32_t)blkSize == (uint32_t)(S_rdBlkHdr >> 32));

                // first send the total count in pktCnt[0]
                CBlkInfo info;
                info.m_blkIdx = P_blkInfo.m_blkIdx;
                info.m_pktCnt[0] = blkPktCnt;
                for (int i=1; i<16; i++)
                    info.m_pktCnt[i] = 0;

                SendMsg_blkStart(info);

#ifndef _HTV
				extern FILE *tfp;
				if (tfp) {
					fprintf(tfp, "CTL: BLK_START blkIdx=%d pktCnt=%d @ %lld\n",
                            (int)P_blkInfo.m_blkIdx, (int)info.m_pktCnt[0],
						(long long)sc_time_stamp().value() / 10000);
					fflush(tfp);
				}
#endif

				P_bStart = false;
			}

			pkt_cnt_t pktNum = P_pktNum + P_cnt;

            if (P_cnt == CTL_PKT_HDR_CNT || pktNum == blkPktCnt) {
                P_cnt = 0;
                //fprintf(stderr,"CTL: CTL_PKT_RD: pause pktLen=%d pktCid=%04x blkPktCnt=%d\n",
                //        (int)pktLen, (int)pktCid, (int)blkPktCnt);
                P_hdrAddr = 0;
				ReadMemPause(CTL_QUEUE_PKT);
				break;
			}
            m_pktAddr.write_mem(c_pRecvBuffOffset | (MemAddr_t)P_pBlk);
			if (!pktNoBytes) {
                //fprintf(stderr,"CTL_PKT_RD0: readAddr=0x%012llx QWords=%d\n",
                //        (long long)P_pBlk, (int)pktQWords);
                ReadMem_prePktDat(c_pRecvBuffOffset | (MemAddr_t)P_pBlk, nextPrefetchBuf<<3, pktQWords);
                bTakePrefetchBuf = true;
            }

            m_prefetchIdx.write_mem(nextPrefetchBuf);

            P_cnt += 1;
            P_hdrAddr = (ht_uint3)(P_cnt>>1);
            if (!pktNoBytes) {
                P_pBlk = (P_pBlk + pktLen + CNY_RECV_BUF_ALIGN-1) & ~(CNY_RECV_BUF_ALIGN-1);
            }
            HtContinue(CTL_PKT_RD);

		}
        break;
		case CTL_QUEUE_PKT: {
            if (P_pktNum == blkPktCnt) {
                if (SendMsgBusy_blkStart()) {
                    HtRetry();
                } else {
                    // send the counts for each index to ACC
                    SendMsg_blkStart(P_blkInfo);
                    P_blkInfo.m_blkIdx += 1;
                    m_blkList.pop();

                    HtContinue(CTL_BLK_START);
                }
				break;
			}

			if (P_cnt == CTL_PKT_HDR_CNT) {
				P_cnt = 0;
                P_hdrAddr = 0;
				HtContinue(CTL_PKT_HDR_RD);
				break;
			}
#ifndef _HTV
            extern FILE *tfp;
            if ((true || tfp) && bSchQueFull) {
                fprintf(stderr, "CTL: CTL_QUE_PKT blkIdx=%d bSchQueFull=%d @ %lld\n",
                        (int)P_blkInfo.m_blkIdx,
                        (int)bSchQueFull,
                    (long long)sc_time_stamp().value() / 10000);
                fflush(tfp);
            }
#endif

			if (bSchQueFull || SendMsgBusy_schPush()) {
				HtRetry();
				break;
			}

			CPktInfo2 pktInfo;
            pktInfo.m_info.m_blkIndex = P_blkInfo.m_blkIdx;
			pktInfo.m_info.m_pPkt = m_pktAddr.read_mem();
            pktInfo.m_info.m_pktLen = (ht_uint12)pktLen;
            //pktInfo.m_info.m_pktWord1 = m_rdPktDat0.read_mem();
            //pktInfo.m_info.m_pktWord2 = m_rdPktDat1.read_mem();
            pktInfo.m_info.m_pktOffset = (ht_uint20)m_pktAddr.read_mem();
            pktInfo.m_info.m_prefetchIdx = m_prefetchIdx.read_mem();
			pktInfo.m_connId = pktCid;

#ifndef _HTV
			extern FILE *tfp;
			if (tfp) {
				fprintf(tfp, "CTL: cid=0x%04x blkIdx=%d",
					(int)pktInfo.m_connId, (int)pktInfo.m_info.m_blkIndex);
				if (pktCid & 0x8000)
					fprintf(tfp, " flags=0x%04x", (int)pktInfo.m_info.m_pktLen);
				else {
					fprintf(tfp, " len=%d", (int)pktInfo.m_info.m_pktLen);
					fprintf(tfp, " adr=0x%012llx", (long long)pktInfo.m_info.m_pPkt);
					fprintf(tfp, " off=%d", (int)pktInfo.m_info.m_pktOffset);
                    fprintf(tfp, " prefetchIdx=%d", (int)pktInfo.m_info.m_prefetchIdx);
				}
				fprintf(tfp, " @ %lld\n", (long long)sc_time_stamp().value() / 10000);
				fflush(tfp);
			}
#endif

			SendMsg_schPush(pktInfo);

			S_schQueCnt[cidIdx] -= 1;
            P_blkInfo.m_pktCnt[cidIdx] += 1;
            P_cnt += 1;
            P_hdrAddr = (ht_uint3)(P_cnt>>1);
			P_pktNum += 1;

			HtContinue(CTL_QUEUE_PKT);
		}
		break;
		case CTL_JOIN: {
			RecvReturnJoin_sch();
		}
		break;
		case CTL_RETURN: {
			if (SendReturnBusy_htmain() || S_blkActCnt != 0) {
				HtRetry();
				break;
			}

			SendReturn_htmain();
		}
		break;
		default:
			assert(0);
		}
	}

    if (!RecvMsgBusy_pktFinish()) {
        prefetch_buf_t prefetchSet = (prefetch_buf_t)(1ll<<RecvMsg_pktFinish());
        // don't set a bit that is already set
        assert((~S_prefetchBufMask & prefetchSet) != 0);
        S_prefetchBufMask |= prefetchSet;
    }
    if (bTakePrefetchBuf) {
        prefetch_buf_t prefetchClr = (prefetch_buf_t)(1ll<<nextPrefetchBuf);
        // don't clear a bit that is already clear
                assert((S_prefetchBufMask & prefetchClr) != 0);
        S_prefetchBufMask &= ~prefetchClr;
    }

	if (GR_htReset) {
        S_prefetchBufMask = 0xffffffffffffffffll;
		for (unsigned i = 0; i < SCH_THREAD_CNT; i++) {
			S_schQueCnt[i] = SCH_QUE_CNT;

		}
	}
}

void
CPersCtl::tzc64(const uint64_t & m, ht_uint6 & cnt) {
    ht_uint4 cnt0, cnt1, cnt2, cnt3;
    tzc16((uint16_t)(m>>48), cnt3);
    tzc16((uint16_t)(m>>32), cnt2);
    tzc16((uint16_t)(m>>16), cnt1);
    tzc16((uint16_t)(m>>0), cnt0);

    //bool m3Zero = (m & 0xffff000000000000ll) == 0;
    bool m2Zero = (m & 0x0000ffff00000000ll) == 0;
    bool m1Zero = (m & 0x00000000ffff0000ll) == 0;
    bool m0Zero = (m & 0x000000000000ffffll) == 0;

    if (!m0Zero)
        cnt = (ht_uint6)((0<<4) | cnt0);
    else if (!m1Zero)
        cnt = (ht_uint6)((1<<4) | cnt1);
    else if (!m2Zero)
        cnt = (ht_uint6)((2<<4) | cnt2);
    else
        cnt = (ht_uint6)((3<<4) | cnt3);
}

void
CPersCtl::tzc16(const uint16_t & m, ht_uint4 & cnt) {
    sc_uint<16> m0 = m;
    sc_uint<4> m1;
    m1[0] = m0(3,0) != 0;
    m1[1] = m0(7,4) != 0;
    m1[2] = m0(11,8) != 0;
    m1[3] = m0(15,12) != 0;

    if (m1[0])
        cnt(3,2) = 0;
    else if (m1[1])
        cnt(3,2) = 1;
    else if (m1[2])
        cnt(3,2) = 2;
    else
        cnt(3,2) = 3;

    sc_uint<4> m2;
    switch (cnt(3,2)) {
    case 0: m2 = m0(3,0); break;
    case 1: m2 = m0(7,4); break;
    case 2: m2 = m0(11,8); break;
    case 3: m2 = m0(15,12); break;
    }

    if (m2[0])
        cnt(1,0) = 0;
    else if (m2[1])
        cnt(1,0) = 1;
    else if (m2[2])
        cnt(1,0) = 2;
    else
        cnt(1,0) = 3;

}
