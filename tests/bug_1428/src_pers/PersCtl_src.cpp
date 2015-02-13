#include "Ht.h"
#include "PersCtl.h"

void
CPersCtl::PersCtl()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case CTL_ENTRY: {
			if (SendReturnBusy_htmain()) {
				HtRetry();
				break;
			}

			SendReturn_htmain();
			break;
		}
		default:
			assert(0);
		}
	}
}
