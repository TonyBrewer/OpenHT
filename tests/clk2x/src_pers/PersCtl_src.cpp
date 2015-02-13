#include "Ht.h"
#include "PersCtl.h"

void CPersCtl::PersCtl()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case CTL_ENTRY:
			{
				if (SendCallBusy_clk2x()) {
					HtRetry();
					break;
				}
				SendCall_clk2x(CTL_RTN);
			}
			break;
		case CTL_RTN:
			{
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
