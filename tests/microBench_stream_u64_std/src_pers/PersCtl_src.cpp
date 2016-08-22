#include "Ht.h"
#include "PersCtl.h"

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

void
CPersCtl::PersCtl()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case CTL_ENTRY: {
			switch (PR_operation) {
			case (0): {
				HtContinue(CTL_CALC1);
				break;
			}
			case (1): {
				HtContinue(CTL_CALC2);
				break;
			}
			case (2): {
				HtContinue(CTL_CALC3);
				break;
			}
			case (3): {
				HtContinue(CTL_CALC4);
				break;
			}
			default:
				assert(0);
			}
		}
		break;
		case CTL_CALC1: {
			BUSY_RETRY(SendCallBusy_calc1());

			if (P_vecIdx < SR_vecLen) {
				SendCallFork_calc1(CTL_JOIN1, P_vecIdx);
				HtContinue(CTL_CALC1);
				P_vecIdx += P_vecStride;
			} else {
				RecvReturnPause_calc1(CTL_RTN);
			}
		}
		break;
		case CTL_CALC2: {
			BUSY_RETRY(SendCallBusy_calc2());

			if (P_vecIdx < SR_vecLen) {
				SendCallFork_calc2(CTL_JOIN2, P_vecIdx);
				HtContinue(CTL_CALC2);
				P_vecIdx += P_vecStride;
			} else {
				RecvReturnPause_calc2(CTL_RTN);
			}
		}
		break;
		case CTL_CALC3: {
			BUSY_RETRY(SendCallBusy_calc3());

			if (P_vecIdx < SR_vecLen) {
				SendCallFork_calc3(CTL_JOIN3, P_vecIdx);
				HtContinue(CTL_CALC3);
				P_vecIdx += P_vecStride;
			} else {
				RecvReturnPause_calc3(CTL_RTN);
			}
		}
		break;
		case CTL_CALC4: {
			BUSY_RETRY(SendCallBusy_calc4());

			if (P_vecIdx < SR_vecLen) {
				SendCallFork_calc4(CTL_JOIN4, P_vecIdx);
				HtContinue(CTL_CALC4);
				P_vecIdx += P_vecStride;
			} else {
				RecvReturnPause_calc4(CTL_RTN);
			}
		}
		break;
		case CTL_JOIN1: {
			RecvReturnJoin_calc1();
		}
		break;
		case CTL_JOIN2: {
			RecvReturnJoin_calc2();
		}
		break;
		case CTL_JOIN3: {
			RecvReturnJoin_calc3();
		}
		break;
		case CTL_JOIN4: {
			RecvReturnJoin_calc4();
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
