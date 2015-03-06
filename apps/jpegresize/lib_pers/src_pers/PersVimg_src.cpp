/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "PersConfig.h"

#if defined(VIMG)

#include "Ht.h"
#include "PersVimg.h"

void
CPersVimg::PersVimg()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case VIMG_ENTRY: {
			if (SendCallBusy_veInfo()) {
				HtRetry();
				break;
			}
			SendCall_veInfo(VIMG_FORK, 0, PR_jobId, PR_pJobInfo);
		}
		break;
		case VIMG_FORK: {
			if (SendCallBusy_vsm() || SendCallBusy_vctl() || SendCallBusy_vwm()) {
				HtRetry();
				break;
			}
			SendCallFork_vsm(VIMG_VSM_JOIN, 0);
			SendCallFork_vctl(VIMG_VCTL_JOIN, 0);
			SendCallFork_vwm(VIMG_VWM_JOIN, 0);
			
			RecvReturnPause_vsm(VIMG_VSM_CONT);
		}
		break;
		case VIMG_VSM_JOIN: {
			RecvReturnJoin_vsm();
		}
		break;
		case VIMG_VCTL_JOIN: {
			RecvReturnJoin_vctl();
		}
		break;
		case VIMG_VWM_JOIN: {
			RecvReturnJoin_vwm();
		}
		break;
		case VIMG_VSM_CONT: {
			RecvReturnPause_vctl(VIMG_VCTL_CONT);
		}
		break;
		case VIMG_VCTL_CONT: {
			RecvReturnPause_vwm(VIMG_VWM_CONT);
		}
		break;
		case VIMG_VWM_CONT: {
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
