#include "Ht.h"
#include "PersBug.h"

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

void
CPersBug::PersBug()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case BUG_WRITE: {

			GW_gvar.write_addr(PR_cnt1, PR_cnt2);
			GW_gvar = PR_val + 1;
			P_val = PR_val + 1;

			if (PR_cnt2 == 7) {
				if (PR_cnt1 == 250) {
					HtContinue(BUG_CALL);
				} else {
					P_cnt1 = PR_cnt1 + 1;
					P_cnt2 = 0;
					HtContinue(BUG_WRITE);
				}
			} else {
				P_cnt2 = PR_cnt2 + 1;
				HtContinue(BUG_WRITE);
			}
		}
		break;
		case BUG_CALL: {
			BUSY_RETRY(SendCallBusy_bug2());
			SendCall_bug2(BUG_RTN);
		}
		break;
		case BUG_RTN: {
			BUSY_RETRY(SendReturnBusy_htmain());

			SendReturn_htmain();
		}
		break;
		default:
			assert(0);
		}
	}
}
