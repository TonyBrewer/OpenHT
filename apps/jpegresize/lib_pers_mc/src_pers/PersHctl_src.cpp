/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "PersConfig.h"

#if defined(HCTL)

#include "Ht.h"
#include "PersHctl.h"

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

void CPersHctl::PersHctl()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case HCTL_ENTRY: {
			HtContinue(HCTL_CALL_HINFO);
		}
		break;
		case HCTL_CALL_HINFO: {
			BUSY_RETRY(SendCallBusy_hwrk());

			SendCall_hwrk(HCTL_RETURN, P_imageIdx);
		}
		break;
		case HCTL_RETURN: {
			BUSY_RETRY(SendReturnBusy_hctl());

			SendReturn_hctl();
		}
		break;
		default:
			assert(0);
		}
	}
}

#endif
