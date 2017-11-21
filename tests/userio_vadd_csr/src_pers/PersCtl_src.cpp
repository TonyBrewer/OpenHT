#include "Ht.h"
#include "PersCtl.h"

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

void
CPersCtl::PersCtl()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case CTL_ENTRY: {
			HtContinue(CTL_ST);
		}
		break;
		case CTL_ST: {
			BUSY_RETRY(SendCallBusy_addSt());

			SendCallFork_addSt(CTL_JOIN_ST, SR_vecLen);
			HtContinue(CTL_ADD);
		}
		break;
		case CTL_ADD: {
			BUSY_RETRY(SendCallBusy_addLd());

			if (P_vecIdx < SR_vecLen) {
				SendCallFork_addLd(CTL_JOIN_LD, P_vecIdx);
				HtContinue(CTL_ADD);
				P_vecIdx += P_vecStride;
			} else {
				RecvReturnPause_addLd(CTL_ST_RTN);
			}
		}
		break;
		case CTL_JOIN_ST: {
			RecvReturnJoin_addSt();
		}
		break;
		case CTL_JOIN_LD: {
			RecvReturnJoin_addLd();
		}
		break;
		case CTL_ST_RTN: {
			RecvReturnPause_addSt(CTL_RTN);
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
