#include "Ht.h"
#include "PersCtl.h"

void
CPersCtl::PersCtl()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case CTL_ENTRY: {
			HtContinue(CTL_MULADD);
		}
		break;
		case CTL_MULADD: {
			if (SendCallBusy_muladd()) {
				HtRetry();
				break;
			}

			if (P_vecIdx < SR_vecLen) {
			  SendCallFork_muladd(CTL_JOIN, P_vecIdx, P_t);
				HtContinue(CTL_MULADD);
				P_vecIdx += P_vecStride;
			} else {
				RecvReturnPause_muladd(CTL_RTN);
			}
		}
		break;
		case CTL_JOIN: {
			RecvReturnJoin_muladd();
		}
		break;
		case CTL_RTN: {
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
