/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "PersConfig.h"

#if defined(HDM)

#include "Ht.h"
#include "PersHdm.h"

#ifndef _HTV
#include "JobInfo.h"
#endif

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

// Mimic vertical resizer (drop messages)
void CPersHdm::PersHdm()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case HDM_ENTRY: {
			BUSY_RETRY (!S_bEndOfImage || SendReturnBusy_hdm());

			SendReturn_hdm();
		}
		break;
		default:
			assert(0);
		}
	}

	// receive messages
	if (!GR_htReset && !RecvMsgBusy_hrm()) {
		HorzResizeMsg hrm = RecvMsg_hrm();

		S_bEndOfImage = hrm.m_bEndOfImage;
	}
}

#endif
