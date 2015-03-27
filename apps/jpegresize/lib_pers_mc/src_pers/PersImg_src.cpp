/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "PersConfig.h"

#if defined(IMG_FIXTURE)

#include "Ht.h"
#include "PersImg.h"

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

void
CPersImg::PersImg()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case IMG_ENTRY: {
			BUSY_RETRY (SendCallBusy_dhInfo());

			P_imageIdx = 0;

			SendCall_dhInfo(IMG_DH_FORK, P_imageIdx, PR_pJobInfo);
		}
		break;
		case IMG_DH_FORK: {
			BUSY_RETRY (SendCallBusy_dec() || SendCallBusy_hwrk() || SendCallBusy_veInfo());

			SendCallFork_dec(IMG_DEC_JOIN, PR_imageIdx, PR_jobId);
			SendCallFork_hwrk(IMG_HWRK_JOIN, PR_imageIdx, P_persMode);
			
			SendCall_veInfo(IMG_VE_FORK, P_imageIdx, PR_pJobInfo);
		}
		break;
		case IMG_VE_FORK: {
			BUSY_RETRY (SendCallBusy_vctl() || SendCallBusy_enc());

			SendCallFork_vctl(IMG_VCTL_JOIN, PR_imageIdx & 1, P_persMode);
			SendCallFork_enc(IMG_ENC_JOIN, PR_jobId, PR_imageIdx, PR_pJobInfo);
			
			HtContinue(IMG_DEC_PAUSE);
		}
		break;
		case IMG_DEC_PAUSE: {
			RecvReturnPause_dec(IMG_HWRK_PAUSE);
		}
		break;
		case IMG_HWRK_PAUSE: {
			RecvReturnPause_hwrk(IMG_VCTL_PAUSE);
		}
		break;
		case IMG_VCTL_PAUSE: {
			RecvReturnPause_vctl(IMG_ENC_PAUSE);
		}
		break;
		case IMG_ENC_PAUSE: {
			RecvReturnPause_enc(IMG_RETURN);
		}
		break;
		case IMG_RETURN: {
			if (SendReturnBusy_htmain()) {
				HtRetry();
				break;
			}
			SendReturn_htmain(PR_jobId);
		}
		break;
		case IMG_DEC_JOIN: {
			RecvReturnJoin_dec();
		}
		break;
		case IMG_HWRK_JOIN: {
			RecvReturnJoin_hwrk();
		}
		break;
		case IMG_VCTL_JOIN: {
			RecvReturnJoin_vctl();
		}
		break;
		case IMG_ENC_JOIN: {
			RecvReturnJoin_enc();
		}
		break;
		default:
			assert(0);
		}
	}
}

#endif
