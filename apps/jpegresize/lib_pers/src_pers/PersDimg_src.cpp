/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "PersConfig.h"

#ifdef DEC_FIXTURE

#include "Ht.h"
#include "PersDimg.h"

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

void
CPersDimg::PersDimg()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case DIMG_INFO: {
			BUSY_RETRY(SendCallBusy_dhInfo());

			SendCall_dhInfo(DIMG_DEC, S_imageIdx, P_jobId, P_pJobInfo);

			S_imageIdx += 1u;
		}
		break;
		case DIMG_DEC: {
			BUSY_RETRY(SendCallBusy_dec());

			SendCall_dec(DIMG_RETURN, S_imageIdx, P_jobId);
		}
		break;
		case DIMG_RETURN: {
			BUSY_RETRY(SendReturnBusy_htmain());

			SendReturn_htmain(P_jobId);
		}
		break;
		default:
			assert(0);
		}
	}

	if (GR_htReset)
		S_imageIdx = 0;
}
#endif
