/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "PersConfig.h"

#if defined(HIMG)

#include "Ht.h"
#include "PersHimg.h"

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

void
CPersHimg::PersHimg()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case HIMG_ENTRY: {
			BUSY_RETRY (SendCallBusy_dhInfo());

			P_imageIdx = 0;

			SendCall_dhInfo(HIMG_HORZ, P_imageIdx, P_jobId, PR_pJobInfo);
		}
		break;
		case HIMG_HORZ: {
			BUSY_RETRY (SendCallBusy_hsmc() || SendCallBusy_hwrk() || SendCallBusy_hdm());

			SendCallFork_hsmc(HIMG_HSMC_JOIN, 0);
			SendCallFork_hwrk(HIMG_HCTL_JOIN, PR_imageIdx);
			SendCallFork_hdm(HIMG_HDM_JOIN);
			
			RecvReturnPause_hsmc(HIMG_HSMC_CONT);
		}
		break;
		case HIMG_HSMC_JOIN: {
			RecvReturnJoin_hsmc();
		}
		break;
		case HIMG_HCTL_JOIN: {
			RecvReturnJoin_hwrk();
		}
		break;
		case HIMG_HDM_JOIN: {
			RecvReturnJoin_hdm();
		}
		break;
		case HIMG_HSMC_CONT: {
			RecvReturnPause_hwrk(HIMG_HCTL_CONT);
		}
		break;
		case HIMG_HCTL_CONT: {
			RecvReturnPause_hdm(HIMG_HDM_CONT);
		}
		break;
		case HIMG_HDM_CONT: {
			if (SendReturnBusy_htmain()) {
				HtRetry();
				break;
			}
			SendReturn_htmain(PR_jobId);
		}
		break;
		default:
			assert(0);
		}
	}
}

#endif
