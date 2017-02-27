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
			WriteMem_gvar(SR_addr, 1);
			WriteMemPause(BUG2_RTN);
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
