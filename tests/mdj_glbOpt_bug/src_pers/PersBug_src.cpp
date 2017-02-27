#include "Ht.h"
#include "PersBug.h"

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

void
CPersBug::PersBug()
{
	if (PR2_htValid) {
		switch (PR2_htInst) {
		case BUG_WRITE: {
			GW2_gvar.write_addr(1);
			GW2_gvar = 55;
			HtContinue(BUG_CALL);
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
