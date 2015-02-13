#include "Ht.h"
#include "PersCtl.h"

void
CPersCtl::PersCtl()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case CTL_ENTRY: {
			S_sum = 0;
			P_result = 0;
			HtContinue(CTL_ADD);
		}
		break;
		case CTL_ADD: {
			if (SendCallBusy_add()) {
				HtRetry();
				break;
			}

			if (P_vecIdx < SR_vecLen) {
				SendCallFork_add(CTL_JOIN, P_vecIdx);
				HtContinue(CTL_ADD);
				P_vecIdx += P_vecStride;
			} else {
				RecvReturnPause_add(CTL_RTN);
			}
		}
		break;
		case CTL_JOIN: {
			S_sum += P_result;
			RecvReturnJoin_add();
		}
		break;
		case CTL_RTN: {
			if (SendReturnBusy_htmain()) {
				HtRetry();
				break;
			}

			SendReturn_htmain(S_sum);
		}
		break;
		default:
			assert(0);
		}
	}
}
