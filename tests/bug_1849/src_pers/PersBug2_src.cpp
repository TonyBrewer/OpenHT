#include "Ht.h"
#include "PersBug2.h"

void
CPersBug2::PersBug2()
{
	if (PR_htValid) {
		switch (PR_htInstr) {

		// Main entry point from Host (htmain call)
		case BUG2_ENTRY:
		{
			if (SendReturnBusy_bug2() || SendReturnBusy_bug2b()) {
				HtRetry();
				break;
			}

			// Return number of bytes seen to the host
#			if (BUG1_PRIV != 0 && BUG2_RTN != 0)
				SendReturn_bug2(0);
#			else
				SendReturn_bug2();
#			endif

			// return from an entry point that is not used
			if (!PR_htValid)
				SendReturn_bug2b();
		}
		break;
		default:
			assert(0);
		}
	}
}
