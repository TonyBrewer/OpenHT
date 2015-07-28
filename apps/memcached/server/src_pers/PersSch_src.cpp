/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT memcached application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "Ht.h"
#include "PersSch.h"

void
CPersSch::PersSch()
{
    //sch_thr_idx_t wcid = (sch_thr_idx_t)RecvMsg_schPush().m_connId;
    // UDP must go through the same queue based on pConn index
    uint16_t nxtCid = RecvMsg_schPush().m_connId;
    sch_thr_idx_t wcid = cid2que(nxtCid);
    sch_que_idx_t wadr = S_queWrIdx[wcid];

	S_queDat.write_addr(wcid, wadr);

	if (!RecvMsgBusy_schPush()) {
		S_queDat.write_mem(RecvMsg_schPush());
        S_queWrIdx[wcid] += 1;
	}

	//
	// Stage 1
	//

	sch_que_idx_t radr = S_queRdIdx[PR1_htId];
	S_queDat.read_addr(PR1_htId, radr);

	T1_bEmpty = SR_queWrIdx[PR1_htId] == SR_queRdIdx[PR1_htId];

	//
	// Stage 2
	//

	if (PR2_htValid) {
		switch (PR2_htInst) {
		case SCH_SPIN: {
			if (SendCallBusy_ProcessPkt()
			    || SendMsgBusy_schPop()
			    || TR2_bEmpty) {
				HtRetry();
				break;
			}

			uint16_t cid = S_queDat.read_mem().m_connId;
            conn_addr_t cidAddr = cid2addr(cid);

#ifndef _HTV
            extern FILE *tfp;
            if (tfp)
                fprintf(tfp, "SCH: Calling Pkt cid=0x%04x blkIdx=%d @ %lld\n",
                        (int)cid, (int)S_queDat.read_mem().m_info.m_blkIndex,
                        (long long)sc_time_stamp().value() / 10000);
#endif
			SendCall_ProcessPkt(SCH_RETURN,
					S_queDat.read_mem().m_info,
                    cid, cidAddr);

			S_queRdIdx[PR2_htId] += 1;
			SendMsg_schPop(PR2_htId);
		}
		break;
		case SCH_RETURN: {
			HtContinue(SCH_SPIN);
		}
		break;
		default:
			if (SendReturnBusy_sch()) {
				HtRetry();
				break;
			}
			SendReturn_sch();
			assert(0);
		}
	}

        if (GR_htReset) {
		for (int i=0; i<16; i++) {
			S_queWrIdx[i] = 0;
			S_queRdIdx[i] = 0;
		}
	}
}
