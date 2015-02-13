#include "Ht.h"
#include "PersBug1.h"

void
CPersBug1::PersBug1()
{
	if (PR_htValid) {
		switch (PR_htInstr) {

		// Main entry point from Host (htmain call)
		case BUG1_ENTRY:
		{
			if (SR_bFlag == false) {
				if (SendCallBusy_bug2()) {
					HtRetry();
					break;
				}

				S_bFlag = true;
				SendCall_bug2(BUG1_ENTRY);
			} else {
				if (SendReturnBusy_htmain()) {
					HtRetry();
					break;
				}

				// Return number of bytes seen to the host
				S_bFlag = false;
#				if (BUG1_RTN != 0)
					SendReturn_htmain(0);
#				else
					SendReturn_htmain();
#				endif
			}
		}
		break;
		default:
			assert(0);
		}
	}
}
