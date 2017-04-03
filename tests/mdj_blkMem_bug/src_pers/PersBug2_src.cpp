#include "Ht.h"
#include "PersBug2.h"

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

void
CPersBug2::PersBug2()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case BUG2_WRITE: {
			BUSY_RETRY(WriteMemBusy());

			ht_uint48 memAddr = SR_addr + (ht_uint48)(PR_cnt1 << 6);

			WriteMem_gvar(memAddr, PR_cnt1, 0, 8);

			if (PR_cnt1 == 250) {
				WriteMemPause(BUG2_RTN);
			} else {
				P_cnt1 = PR_cnt1 + 1;
				HtContinue(BUG2_WRITE);
			}
		}
		break;
		case BUG2_RTN: {
			BUSY_RETRY(SendReturnBusy_bug2());

			SendReturn_bug2();
		}
		break;
		default:
			assert(0);
		}
	}
}
