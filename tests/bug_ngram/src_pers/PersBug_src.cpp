#include "Ht.h"
#include "PersBug.h"
void
CPersBug::PersBug()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case BUG_RTN: {
			if (SendReturnBusy_htmain()) {
				HtRetry();
				break;
			}

			SendReturn_htmain();
		}
		break;
		default:
			assert(0);
		}
	}
}
