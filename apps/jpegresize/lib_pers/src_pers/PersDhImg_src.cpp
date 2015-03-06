/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#if defined(DHIMG_FIXTURE)

#include "Ht.h"
#include "PersDhImg.h"

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

void
CPersDhImg::PersDhImg()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case DHIMG_ENTRY: {
			BUSY_RETRY (SendCallBusy_dhInfo());

			P_imageIdx = 0;

			SendCall_dhInfo(DHIMG_HORZ, P_imageIdx, PR_pJobInfo);
		}
		break;
		case DHIMG_HORZ: {
			BUSY_RETRY (SendCallBusy_dec() || SendCallBusy_hwrk() || SendCallBusy_hdm());

			SendCallFork_dec(DHIMG_DEC_JOIN, PR_imageIdx, PR_jobId);
			SendCallFork_hwrk(DHIMG_HCTL_JOIN, PR_imageIdx, 3);
			SendCallFork_hdm(DHIMG_HDM_JOIN);
			
			RecvReturnPause_dec(DHIMG_DEC_CONT);
		}
		break;
		case DHIMG_DEC_JOIN: {
			RecvReturnJoin_dec();
		}
		break;
		case DHIMG_HCTL_JOIN: {
			RecvReturnJoin_hwrk();
		}
		break;
		case DHIMG_HDM_JOIN: {
			RecvReturnJoin_hdm();
		}
		break;
		case DHIMG_DEC_CONT: {
			RecvReturnPause_hwrk(DHIMG_HCTL_CONT);
		}
		break;
		case DHIMG_HCTL_CONT: {
			RecvReturnPause_hdm(DHIMG_HDM_CONT);
		}
		break;
		case DHIMG_HDM_CONT: {
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
