#include "Ht.h"
#include "PersCtl.h"

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

void
CPersCtl::PersCtl()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case CTL_ENTRY: {	
			HtContinue(CTL_ADD);
		}
		break;
		case CTL_ADD: {
			BUSY_RETRY(SendCallBusy_add());

			if (P_vecIdx < SR_size) {
				SendCallFork_add(CTL_JOIN, P_vecIdx);
				HtContinue(CTL_ADD);
				P_vecIdx += P_vecStride;
			} else {
				RecvReturnPause_add(CTL_RTN);
			}
		}
		break;
		case CTL_JOIN: {
			RecvReturnJoin_add();
		}
		break;
		case CTL_RTN: {
			BUSY_RETRY(SendReturnBusy_htmain());
			SendReturn_htmain();
		}
		break;
		default:
			assert(0);
		}
	}
}
