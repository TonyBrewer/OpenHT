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
#include "PersHor.h"

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

void
CPersHor::PersHor()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case HOR_ENTRY: {
			BUSY_RETRY(SendReturnBusy_hor());

			SendReturn_hor();
		}
		break;
		default:
			assert(0);
		}
	}

	ht_attrib(dont_touch, SR_decMsgVal, "true");
	ht_attrib(dont_touch, SR_decMsg, "true");
	S_decMsgVal = false;
        if (!RecvMsgBusy_dec()) {
                S_decMsgVal = true;
                S_decMsg = RecvMsg_dec();
        }
}
#endif
