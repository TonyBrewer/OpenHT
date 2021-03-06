#include "Ht.h"
#include "PersBug.h"

void
CPersBug::PersBug()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case BUG_ENTER: {
			P_idx1 = 1;
			P_idx2 = 4;
			PW_v1.write_addr(1, 4);
			PW_v1 = 0x45;
			HtContinue(BUG_RTN);
		}
		break;
		case BUG_RTN: {
			if (SendReturnBusy_bug()) {
				HtRetry();
				break;
			}

			if (PR_v1 != 0x45)
				HtAssert(0, 0);

			SendReturn_bug();
		}
		break;
		default:
			assert(0);
		}
	}
}
