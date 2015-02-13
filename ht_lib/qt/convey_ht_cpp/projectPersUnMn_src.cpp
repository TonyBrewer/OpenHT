/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "Ht.h"
#include "Pers%UNIT_NAME%%MODULE_NAME%.h"

void
CPers%UNIT_NAME%%MODULE_NAME%::Pers%UNIT_NAME%%MODULE_NAME%()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case PRT_WRITE: {
			if (MemWriteBusy()) {
				HtRetry();
				break;
			}
			MemWrite(P_pBuf, (uint64_t)0x006f6c6c6548LL /*Hello*/);
			MemWritePause(PRT_RTN);
		}
		break;
		case PRT_RTN: {
			if (ReturnBusy_htmain()) {
				HtRetry();
				break;
			}

			Return_htmain();
		}
		break;
		default:
			assert(0);
		}
	}
}

