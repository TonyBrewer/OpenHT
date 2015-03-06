/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "PersConfig.h"

#if defined(VEIMG_FIXTURE)

#include "Ht.h"
#include "PersVeImg.h"

void
CPersVeImg::PersVeImg()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case VEIMG_ENTRY: {
			if (SendCallBusy_veInfo()) {
				HtRetry();
				break;
			}
			SendCall_veInfo(VEIMG_FORK, 0, PR_pJobInfo);
		}
		break;
		case VEIMG_FORK: {
			if (SendCallBusy_vsm() || SendCallBusy_vctl() || SendCallBusy_enc()) {
				HtRetry();
				break;
			}
			SendCallFork_vsm(VEIMG_VSM_JOIN, 0);
			SendCallFork_vctl(VEIMG_VCTL_JOIN, 0);
			SendCallFork_enc(VEIMG_ENC_JOIN, PR_jobId, 0, PR_pJobInfo);
			
			RecvReturnPause_vsm(VEIMG_VSM_CONT);
		}
		break;
		case VEIMG_VSM_JOIN: {
			RecvReturnJoin_vsm();
		}
		break;
		case VEIMG_VCTL_JOIN: {
			RecvReturnJoin_vctl();
		}
		break;
		case VEIMG_ENC_JOIN: {
			RecvReturnJoin_enc();
		}
		break;
		case VEIMG_VSM_CONT: {
			RecvReturnPause_vctl(VEIMG_VCTL_CONT);
		}
		break;
		case VEIMG_VCTL_CONT: {
			RecvReturnPause_enc(VEIMG_ENC_CONT);
		}
		break;
		case VEIMG_ENC_CONT: {
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
