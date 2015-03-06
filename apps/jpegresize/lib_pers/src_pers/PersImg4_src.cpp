/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "PersConfig.h"

#if defined(IMG4)

#include "Ht.h"
#include "PersImg4.h"

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

void
CPersImg4::PersImg4()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case IMG_ENTRY: {
			if (P_persMode == 0)
				HtContinue(IMG_RETURN);
			else {
				// push htId on order queue to maintain order
				S_orderQue.push(PR_htId);
				HtContinue(IMG_DH_WAIT);
			}
		}
		break;
		case IMG_DH_WAIT: {
			BUSY_RETRY(S_orderQue.empty() || S_orderQue.front() != PR_htId || S_imageIdxPool.empty());

			// we are the next htId and an imageIdx is available, start the work
			P_imageIdx = S_imageIdxPool.front();
			S_imageIdxPool.pop();
			S_orderQue.pop();

			if (P_persMode == 2)
				HtContinue(IMG_VE_WAIT);
			else
				HtContinue(IMG_DH_INFO);
		}
		break;
		case IMG_DH_INFO: {
			BUSY_RETRY (SendCallBusy_dhInfo());

			S_bDhBusy[P_imageIdx] = true;

			SendCall_dhInfo(IMG_DH_FORK, P_imageIdx, PR_pJobInfo);
		}
		break;
		case IMG_DH_FORK: {
			BUSY_RETRY (SendCallBusy_dec(P_imageIdx & DEC_REPL_MASK) || SendCallBusy_hwrk(P_imageIdx & HORZ_REPL_MASK));

			SendCallFork_dec(IMG_DEC_JOIN, P_imageIdx & DEC_REPL_MASK, PR_imageIdx, PR_jobId);
			SendCallFork_hwrk(IMG_HWRK_JOIN, P_imageIdx & HORZ_REPL_MASK, PR_htId, PR_imageIdx, P_persMode);

			HtContinue(IMG_VE_WAIT);
		}
		break;
		case IMG_VE_WAIT: {
			// wait until the VE resource is available
			BUSY_RETRY(S_bVeBusy[P_imageIdx]);


			if (P_persMode == 1)
				HtContinue(IMG_DEC_PAUSE);
			else {
				S_bVeBusy[P_imageIdx] = true;
				HtContinue(IMG_VE_INFO);
			}
		}
		break;
		case IMG_VE_INFO: {
			BUSY_RETRY (SendCallBusy_veInfo());

			SendCall_veInfo(IMG_VE_FORK, P_imageIdx, PR_pJobInfo);
		}
		break;
		case IMG_VE_FORK: {
			BUSY_RETRY (SendCallBusy_vctl((P_imageIdx >> 1) & VERT_REPL_MASK) ||
				SendCallBusy_enc((P_imageIdx >> 1) & ENC_REPL_MASK));

			SendCallFork_vctl(IMG_VCTL_JOIN, (P_imageIdx >> 1) & VERT_REPL_MASK, PR_htId, PR_imageIdx, P_persMode);
			SendCallFork_enc(IMG_ENC_JOIN, (P_imageIdx >> 1) & ENC_REPL_MASK, PR_jobId, PR_imageIdx, PR_pJobInfo);
			
			HtContinue(IMG_DEC_PAUSE);
		}
		break;
		case IMG_DEC_PAUSE: {
			RecvReturnPause_dec(IMG_HWRK_PAUSE, P_imageIdx & DEC_REPL_MASK);
		}
		break;
		case IMG_HWRK_PAUSE: {
			RecvReturnPause_hwrk(IMG_VCTL_PAUSE, P_imageIdx & HORZ_REPL_MASK);
		}
		break;
		case IMG_VCTL_PAUSE: {
			// DH completed, free imageIdx
			S_bDhBusy[P_imageIdx] = false;
			S_imageIdxPool.push(P_imageIdx);

			if (P_persMode == 1)
				HtContinue(IMG_RETURN);
			else
				RecvReturnPause_vctl(IMG_ENC_PAUSE, (P_imageIdx >> 1) & VERT_REPL_MASK);
		}
		break;
		case IMG_ENC_PAUSE: {
			RecvReturnPause_enc(IMG_RETURN, (P_imageIdx >> 1) & ENC_REPL_MASK);
		}
		break;
		case IMG_RETURN: {
			if (SendReturnBusy_htmain()) {
				HtRetry();
				break;
			}

			S_bVeBusy[P_imageIdx] = false;

			SendReturn_htmain(PR_jobId);
		}
		break;
		case IMG_DEC_JOIN: {
			RecvReturnJoin_dec(P_imageIdx & DEC_REPL_MASK);
		}
		break;
		case IMG_HWRK_JOIN: {
			RecvReturnJoin_hwrk(P_imageIdx & HORZ_REPL_MASK);
		}
		break;
		case IMG_VCTL_JOIN: {
			RecvReturnJoin_vctl((P_imageIdx >> 1) & VERT_REPL_MASK);
		}
		break;
		case IMG_ENC_JOIN: {
			RecvReturnJoin_enc( (P_imageIdx >> 1) & ENC_REPL_MASK );
		}
		break;
		default:
			assert(0);
		}
	}

	if (S_poolInitCnt < 4) {
		S_imageIdxPool.push(S_poolInitCnt & 3);
		S_poolInitCnt += 1;
	}
}

#endif
