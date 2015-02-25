/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT memcached application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "Ht.h"
#include "PersAcc.h"

void
CPersAcc::PersAcc()
{
    ht_uint1 c_incQuePktCnt[SCH_THREAD_CNT] = {0};
    ht_uint1 c_decQuePktCnt[SCH_THREAD_CNT] = {0};
    bool c_incQueRdPtr[SCH_THREAD_CNT] = {false};
    bool c_decFrontBlkInfo[SCH_THREAD_CNT] = {false};

#ifndef _HTV
    bool c_queEmpty[SCH_THREAD_CNT];
#endif
    for (uint32_t i = 0; i<SCH_THREAD_CNT; i++) {
        S_queTooFull[i] = S_queCnt[i] > (ACC_QUE_DEPTH - 4);
        // uncomment the following statement to stress forward progress mechanism
        //c_nxtQueTooFull[i] = S_queCnt[i] > 4;
#ifndef _HTV
        c_queEmpty[i] = S_queCnt[i]==0;
#endif
    }
    acc_ram_addr_t ramWrAddr = 0;
    bool ramWe = false;
    bool c_incCurrQueWrPtr = false;
	if (!RecvMsgBusy_accPush()) {
        uint16_t pktCid = RecvMsg_accPush().m_cmd.m_connId;
        sch_thr_idx_t cidIdx = cid2que(pktCid);
        assert(S_queCnt[cidIdx] < ACC_QUE_DEPTH);
        ramWrAddr = cidIdx<<ACC_QUE_DEPTH_W | S_queWrPtr[cidIdx];
        S_ramDat.write_addr(ramWrAddr);
        S_ramDat.write_mem(RecvMsg_accPush());
        ramWe = true;
        S_queWrPtr[cidIdx] += 1;
        c_incCurrQueWrPtr = (cidIdx == SR_whichQue);
        S_queCnt[cidIdx] += 1;
        //fprintf (stderr,"ACC: accPush: cidIdx=%d S_queCnt=%d @ %lld\n",
        //         (int)cidIdx, (int)S_queCnt[cidIdx],
        //         (long long)sc_time_stamp().value() / 10000);
        if (RecvMsg_accPush().m_bPktDone)
            //S_quePktCnt[cidIdx]++;
            c_incQuePktCnt[cidIdx] = 1;
	}
	if (!RecvMsgBusy_blkStart()) {
#ifndef _HTV
        extern FILE *tfp;
        if (false && tfp) {
            fprintf(tfp,"ACC: push queBlkInfo blkIdx=%d depth=%d\n",
                    (int)RecvMsg_blkStart().m_blkIdx,
                    (int)S_queBlkInfo.size());
            fflush(tfp);
        }
#endif
        S_queBlkInfo.push(RecvMsg_blkStart());
	}


    bool c_blkDone = false;
    bool c_frontDecd = false;

    // which queue will we work on next?
    bool c_myQReady = false;
    sch_thr_vec_t c_popVec = 0xffff;
    sch_thr_vec_t c_pressureVec = 0x0000;
    bool c_nxtRelievePressure = false;
    for (uint32_t i = 0; i<SCH_THREAD_CNT; i++) {
        bool c_frontEmpty = !SR_frontBlkCntMatch[i];
        if (c_frontEmpty) {
            c_popVec &= ~(1<<i);
        } else {
            c_myQReady = true;
        }
        if (SR_queTooFull[i] && !SR_frontBlkIsZero[i]) {
            c_nxtRelievePressure = true;
            c_pressureVec |= 1<<i;
        }
    }
    if (c_nxtRelievePressure) {
        tzc(c_pressureVec, S_nextQue);
    } else {
        tzc(c_popVec, S_nextQue);
    }
    S_qReady = c_myQReady || c_nxtRelievePressure;

    bool loadCmd = false;
    switch (S_accState) {
    case GET_PKT_CNT: {
        // For each buffer the first entry of queBlkInfo contains the total packets
        // in the first index. This is pushed before ctl sends the first packet to sch
        // The second entry in queBlkInfo contains the number of packets in each queue
        // This is pushed after ctl pushes all packets to sch.
        CBlkInfo info = S_queBlkInfo.front();
        S_blkPktCnt = info.m_pktCnt[0];
        if (!S_queBlkInfo.empty()) {
            S_queBlkInfo.pop();
            S_accState = GET_QUE_CNT;
        }
#ifndef _HTV
        extern FILE *tfp;
        if (tfp && !S_queBlkInfo.empty()) {
            fprintf(tfp,"ACC: GET_PKT_CNT blkIdx=%d blkPktCnt=%d\n",
                    (int)info.m_blkIdx,
                    (int)S_blkPktCnt);
            fflush(tfp);
        }
#endif
    }
        break;
    case GET_QUE_CNT: {
        S_frontBlkInfo = S_queBlkInfo.front();

        if (!S_queBlkInfo.empty()) {
            S_queBlkInfo.pop();
            S_accState = GET_NXT_QUE;
        }
#ifndef _HTV
        extern FILE *tfp;
        if (tfp && !S_queBlkInfo.empty()) {
            fprintf(tfp,"ACC: GET_QUE_CNT blkIdx=%d blkPktCnt=%d idxPktCnt=",
                    (int)S_frontBlkInfo.m_blkIdx, (int)S_blkPktCnt);
	    for (int i=0; i<16; i++)
		fprintf(tfp, "%s%d", i ? "," : "",
		    (int)S_frontBlkInfo.m_pktCnt[i]);
	    fprintf(tfp, " @ %lld\n", HT_CYCLE());
            fflush(tfp);
        }
#endif
    }
        break;
    case GET_NXT_QUE: {
        for (uint32_t i = 0; i<SCH_THREAD_CNT; i++)
            S_whichQueDcd[i] = false;
        S_whichQue = S_nextQue;
        S_whichQueDcd[S_nextQue] = true;
        if(S_qReady) {
            S_accState = STAGE_QUE_NUM;
            //fprintf(stderr,"ACC: GET_NXT_QUE nextQue=%d queCnt=%d relievePressure=%d @ %lld\n",
            //        (int)S_nextQue, (int)SR_queCnt[S_whichQue],
            //        (int)c_nxtRelievePressure,
            //        (long long)sc_time_stamp().value() / 10000);
        }

    }
        break;
    case STAGE_QUE_NUM:
        S_accState = STAGE_BRAM;
        break;
    case STAGE_BRAM:
        loadCmd = true;
        S_accState = QUE_LOOP;
        break;
    case QUE_LOOP: {
        if (!SendHostDataBusy()) {
            for (uint32_t i = 0; i<SCH_THREAD_CNT; i++) {
                c_decQuePktCnt[i] = (ht_uint1)(S_cmd.m_bPktDone &&  SR_whichQueDcd[i]);
                c_decFrontBlkInfo[i] =  S_cmd.m_bPktDone &&  SR_whichQueDcd[i];
                c_incQueRdPtr[i] = SR_whichQueDcd[i];
            }
            if (S_cmd.m_bPktDone) {
                S_blkPktCnt--;
            }
            c_frontDecd = true;
#ifndef _HTV
            assert(!c_queEmpty[SR_whichQue]);
            extern FILE *tfp;
            if (tfp) {
                fprintf(tfp, "ACC: New Packet ");
                fprintf(tfp, "cid=0x%04x", (int)S_cmd.m_cmd.m_connId);
                fprintf(tfp, " blkIdx=%d pktDone=%d numCmd=%d numOpt=%d @ %lld\n",
                    (int)S_cmd.m_blkIndex, (int)S_cmd.m_bPktDone,
                    (int)S_cmd.m_numCmd, (int)S_cmd.m_numOpt,
                    (long long)sc_time_stamp().value() / 10000);
                fprintf (tfp,"ACC:            whichQue=%d rdPtr=%d wrPtr=%d\n", (int)SR_whichQue,
                         (int)S_queRdPtr[SR_whichQue],
                         (int)S_queWrPtr[SR_whichQue]);
                if (S_remCmd)
                    fprintf(tfp,"ACC: OutQue=\n");
                fflush(tfp);
            }
#endif
            // start a new packet
            S_remCmd = S_cmd.m_numCmd;
            S_remOpt = S_cmd.m_numOpt;
            S_idxCmd = 0;
            S_idxOpt = 0;

            uint64_t d = 0;
            uint32_t w;
            if (S_remCmd > 1) {
                w = S_cmd.m_raw.m_word[S_idxCmd];
                d = (uint64_t)w;
                w = S_cmd.m_raw.m_word[S_idxCmd + 1];
                d |= (uint64_t)((uint64_t)w << 32);

                S_idxCmd += 2;
                S_remCmd -= 2;

    #ifndef _HTV
                extern FILE *tfp;
                if (tfp) {
                    fprintf(tfp, "ACC:          %016llx\n", (long long unsigned)d);
                }
    #endif
                SendHostData(d);
            }


            S_accState = FINISH_PKT;
        }

    }
        break;
    case FINISH_PKT: {
        if (!SendMsgBusy_accPop() && !SendHostDataBusy()) {
            uint64_t d = 0;
            uint32_t w;
            if (S_remCmd > 1) {
                w = S_cmd.m_raw.m_word[S_idxCmd];
                d = (uint64_t)w;
                w = S_cmd.m_raw.m_word[S_idxCmd + 1];
                d |= (uint64_t)((uint64_t)w << 32);

                S_idxCmd += 2;
                S_remCmd -= 2;

    #ifndef _HTV
                extern FILE *tfp;
                if (tfp) {
                    fprintf(tfp, "ACC:          %016llx\n", (long long unsigned)d);
                }
    #endif
                SendHostData(d);
            } else if (S_remCmd > 0) {
                w = S_cmd.m_raw.m_word[S_idxCmd];
                d = (uint64_t)w;

                S_idxCmd += 1;
                S_remCmd -= 1;

                if (S_remOpt > 0) {
                    w = S_cmd.m_rspOpt.m_word[S_idxOpt];
                    d |= (uint64_t)((uint64_t)w << 32);

                    S_idxOpt += 1;
                    S_remOpt -= 1;
                }

    #ifndef _HTV
                extern FILE *tfp;
                if (tfp) {
                    fprintf(tfp, "ACC:          %016llx\n", (long long unsigned)d);
                }
    #endif
                SendHostData(d);
            } else if (S_remOpt > 1) {
                w = S_cmd.m_rspOpt.m_word[S_idxOpt];
                d = (uint64_t)w;
                w = S_cmd.m_rspOpt.m_word[S_idxOpt+1];
                d |= (uint64_t)((uint64_t)w << 32);

                S_idxOpt += 2;
                S_remOpt -= 2;

    #ifndef _HTV
                extern FILE *tfp;
                if (tfp) {
                    fprintf(tfp, "ACC:          %016llx\n", (long long unsigned)d);
                }
    #endif
                SendHostData(d);
            } else if (S_remOpt > 0) {
                w = S_cmd.m_rspOpt.m_word[S_idxOpt];
                d = (uint64_t)w;

                S_idxOpt += 1;
                S_remOpt -= 1;

#ifndef _HTV
                extern FILE *tfp;
                if (tfp) {
                    fprintf(tfp, "ACC:          %016llx\n", (long long unsigned)d);
                }
#endif
                SendHostData(d);
            }
        }
        if (!S_remOpt && !S_remCmd) {
            if (S_cmd.m_bPktDone) {
#ifndef _HTV
                extern FILE *tfp;
                if (tfp) {
                    fprintf(tfp,"ACC: pktDone blkIdx=%d blkPktCnt=%d whichQue=%d idxPktCnt=",
                            (int)S_frontBlkInfo.m_blkIdx,
                            (int)S_blkPktCnt, (int)SR_whichQue);
		    for (int i=0; i<16; i++)
			fprintf(tfp, "%s%d", i ? "," : "",
			    (int)S_frontBlkInfo.m_pktCnt[i]);
                    fprintf(tfp, " @ %lld\n", HT_CYCLE());
                    fflush(tfp);
                }
#endif
            }
            SendMsg_accPop(SR_whichQue);

            bool c_frontEmpty = true;
            for (uint32_t i=0; i<SCH_THREAD_CNT; i++ )
                if (!SR_frontBlkIsZero[i])
                    c_frontEmpty = false;
            c_blkDone = c_frontEmpty;
            if (c_blkDone)
                S_accState = FINISH_BLK;
            else if (SR_frontBlkIsZero[SR_whichQue] ||
                     SR_queCnt[SR_whichQue]==0 || SR_lastQueCntZero)
                // if the last cycle count was zero and not this cycle
                // then we read the same cycle as write which doesn't work
                S_accState = GET_NXT_QUE;
            else {
                loadCmd = true;
                S_accState = QUE_LOOP;
                //fprintf(stderr,"ACC: FINISH_PKT whichQue=%d queCnt=%d @ %lld\n",
                //        (int)SR_whichQue, (int)SR_queCnt[SR_whichQue],
                //        (long long)sc_time_stamp().value() / 10000);
            }
        }

    }
        break;
    case FINISH_BLK: {
        if (!SendMsgBusy_blkFinish() && !SendHostDataBusy()) {
#ifndef _HTV
            extern FILE *tfp;
            if (tfp) {
                fprintf(tfp, "ACC:          %016llx\n", (long long unsigned)CNY_CMD_NXT_RCV_BUF);
            }
#endif
            SendHostData(CNY_CMD_NXT_RCV_BUF);

            SendMsg_blkFinish(true);
            S_accState = GET_PKT_CNT;
        }
    }
        break;
    default:
        assert(0);
    }

#ifndef _HTV
    if (c_blkDone ) {
        extern FILE *tfp;
        if (tfp) {
            fprintf(tfp, "ACC: BLK_FINISH blkIdx=%d blkInfoCnt=%d pktCnt=",
                    (int)S_cmd.m_blkIndex,
                    (int)S_queBlkInfo.size());
	    for (int i=0; i<16; i++)
		fprintf(tfp, "%s%d", i ? "," : "", (int)S_queCnt[i]);
	    fprintf(tfp, " @ %lld\n", HT_CYCLE());
            fflush(tfp);
        }
        assert (S_blkPktCnt == 0);
    }
#endif

    for (uint32_t i = 0; i<SCH_THREAD_CNT; i++) {
        switch (c_incQuePktCnt[i]<<1 | c_decQuePktCnt[i]) {
        case 0: S_quePktCnt[i] = SR_quePktCnt[i]; break;
        case 1: S_quePktCnt[i] = SR_quePktCnt[i]-1; break;
        case 2: S_quePktCnt[i] = SR_quePktCnt[i]+1; break;
        case 3: S_quePktCnt[i] = SR_quePktCnt[i]; break;
        }
        if (c_incQueRdPtr[i]) {
            S_queRdPtr[i] = S_queRdPtr[i]+1;
            S_queCnt[i] = S_queCnt[i] - 1;
        }
        S_frontBlkInfo.m_pktCnt[i] = c_decFrontBlkInfo[i] ?
                    S_frontBlkInfo.m_pktCnt[i]-1 :
                    S_frontBlkInfo.m_pktCnt[i]-0;

        S_frontBlkIsZero[i] = S_frontBlkInfo.m_pktCnt[i] == 0;
        S_frontBlkCntMatch[i] = S_frontBlkInfo.m_pktCnt[i] <= S_quePktCnt[i] &&
                !S_frontBlkIsZero[i];
        //if (S_quePktCnt[i] != 0 || c_nxtIncQuePktCnt[i])
        //    S_pktAvailVec |= (sch_thr_vec_t)(1<<i);
    }

    S_lastQueCntZero = SR_queCnt[SR_whichQue] == 0 ||
            (SR_queCnt[SR_whichQue] == 1 && c_incQueRdPtr[SR_whichQue] && c_incCurrQueWrPtr);

    if (loadCmd) {
        //if (SR_ramRdError)
        //    fprintf(stderr,"ACC: ramRdErrorE whichQue=%d queCnt=%d rdPtr=%d wrPtr=%d @ %lld\n",
        //            (int)SR_whichQue, (int)SR_queCnt[SR_whichQue],
        //            (int)SR_queRdPtr[SR_whichQue],
        //            (int)SR_queWrPtr[SR_whichQue],
        //            (long long)sc_time_stamp().value() / 10000);
        assert(!SR_ramRdError);
        S_cmd = S_ramDat.read_mem();
    }


#ifndef _HTV
    extern FILE *tfp;
    if (tfp && c_popVec != 0 && S_accState == GET_NXT_QUE)
        fprintf (tfp, "ACC: qReady=%d relievePressure=%d accState=%d popVec=0x%04x nextQue=%d whichQue=%d @ %lld\n",
                 (int)S_qReady, (int)c_nxtRelievePressure, (int)S_accState,
                 (int)c_popVec,
                 (int)S_nextQue, (int)S_whichQue,
                 (long long)sc_time_stamp().value() / 10000);
    if (false && c_nxtRelievePressure)
        fprintf (stderr,"ACC: relieve pressure @ %lld\n",
                 (long long)sc_time_stamp().value() / 10000);
    if (false && SR_qReady && SendHostDataBusy())
        fprintf (stderr,"ACC: HIF OutQue full @ %lld\n",
                 (long long)sc_time_stamp().value() / 10000);
#endif
    acc_ram_addr_t ramRdAddr = SR_whichQue<<ACC_QUE_DEPTH_W | S_queRdPtr[SR_whichQue];
    S_ramDat.read_addr(ramRdAddr);

    S_ramRdError = (ramRdAddr==ramWrAddr) && ramWe;

    // THe HT state machine does nothing except make the tools happy
    if (PR_htValid) {
        switch (PR_htInst) {
		case ACC_SPIN: {
            if (SendMsgBusy_accPop() || SendHostDataBusy() ||
                    !(SR_qReady && !S_incLastCycle)) {
                S_incLastCycle = false;
				HtRetry();
				break;
			}

            S_incLastCycle = false;
			// start a new packet

            HtContinue(ACC_FINISH);

		}
		break;
		case ACC_FINISH: {
			if (SendMsgBusy_blkFinish() || SendHostDataBusy()) {
				HtRetry();
				break;
			}
            HtContinue(ACC_SPIN);

		}
		break;
		default:
			if (SendReturnBusy_acc()) {
				HtRetry();
				break;
			}
			SendReturn_acc();
			assert(0);
		}
	}


	if (GR_htReset) {
        S_accState = GET_PKT_CNT;
	}
}

void
CPersAcc::tzc(const uint16_t & m, ht_uint4 & cnt) {
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
