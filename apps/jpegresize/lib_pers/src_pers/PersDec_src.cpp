/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "PersConfig.h"

#ifdef DEC

#include "Ht.h"
#include "PersDec.h"
#include "JobInfoOffsets.h"

#ifndef _HTV
#include "JobInfo.h"
#define ASSERT_MSG assert_msg
#else
#define ASSERT_MSG(...)
#endif

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

void CPersDec::PersDec()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case DEC_START: {
			P_pInPtr = S_pRstBase + (ht_uint48)GR_rstOff.e;
			P_firstRstMcuCnt = GR_rstOff.m;
			P_rstIdx += 1;

			HtContinue(DEC_LOOP);
		}
		break;
		case DEC_LOOP: {
			BUSY_RETRY(S_pidPoolInitCnt < DPIPE_CNT || S_pidPool.empty());
			BUSY_RETRY(SendCallBusy_ihuf());

			ht_uint48 pInPtrEnd = S_pRstBase + (ht_uint48)GR_rstOff.e;
			McuRows_t rstIdx = (McuRows_t)(P_rstIdx - 1);

			P_pid = S_pidPool.front();
			S_pidPool.pop();

			SendCallFork_ihuf(DEC_JOIN, P_pid, P_jobId, P_imageIdx, rstIdx,
					P_pInPtr, pInPtrEnd, S_compCnt, S_numBlk,
					S_mcuColsRst, P_firstRstMcuCnt);

			P_pInPtr = pInPtrEnd;
			P_firstRstMcuCnt = GR_rstOff.m;
			P_rstIdx += 1;

			if (P_rstIdx > S_rstCnt)
				RecvReturnPause_ihuf(DEC_RETURN);
			else
				HtContinue(DEC_LOOP);
		}
		break;
		case DEC_JOIN: {
			RecvReturnJoin_ihuf();
			S_pidPool.push(P_pid);
		}
		break;
		case DEC_RETURN: {
			BUSY_RETRY(SendReturnBusy_dec());

			SendReturn_dec();
		}
		break;
		default:
			assert(0);
		}
	}

	RecvJobInfo();

	if (GR_htReset)
		S_pidPoolInitCnt = 0;
	else if (S_pidPoolInitCnt < DPIPE_CNT) {
		S_pidPool.push((dpipe_id_t)S_pidPoolInitCnt);
		S_pidPoolInitCnt += 1;
	}
}

void CPersDec::RecvJobInfo()
{
	if (!GR_htReset && !RecvMsgBusy_jobInfo()) {
		JobInfoMsg msg = RecvMsg_jobInfo();
		if (msg.m_imageIdx == i_replId && msg.m_sectionId == DEC_SECTION) {
			//  (int)msg.m_rspQw, (long long)msg.m_data);
			if (msg.m_rspQw == 0) {
				S_compCnt = (ht_uint2)msg.m_data;
			}
			if (msg.m_rspQw == 1) {
				S_mcuColsRst = (McuCols_t)(ht_uint5)(msg.m_data >> 11);
				S_rstCnt = (McuRows_t)(msg.m_data >> 16);
				S_numBlk = (ht_uint22)(msg.m_data >> 32);
			}
			if (msg.m_rspQw == 2) {
				ASSERT_MSG(offsetof(JobDec, m_pRstBase) == 2*8,
					   "offsetof(JobDec, m_pRstBase) == %d\n",
					   (int)offsetof(JobDec, m_pRstBase));
				S_pRstBase = (ht_uint48)msg.m_data;
			}
			if (msg.m_rspQw >= DEC_RSTINFO_QOFF && msg.m_rspQw < DEC_RSTINFO_QOFF+DEC_RSTINFO_QCNT) {
				ASSERT_MSG(offsetof(JobDec, m_rstInfo) == DEC_RSTINFO_QOFF*8,
					   "offsetof(JobDec, m_rstInfo) == %d\n",
					   (int)offsetof(JobDec, m_rstInfo));

				McuRows_t a = (McuRows_t)(msg.m_rspQw - (DEC_RSTINFO_QOFF & MCU_ROWS_MASK));
				GW_rstOff.write_addr(a);
				GW_rstOff.e = (ht_int26)(msg.m_data >> 22);
				GW_rstOff.m = (McuCols_t)(msg.m_data >> 48);
			}
		}
	}
}
#endif
