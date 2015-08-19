#include "Ht.h"
#include "PersCtl.h"

void CPersCtl::PersCtl() {
	if (PR_htValid) {
		switch (PR_htInst) {
		case CTL_TEST10: {
			if (SendCallBusy_test10()) {
				HtRetry();
				break;
			}
			SendCall_test10(CTL_TEST11, PR_memAddr);
			break;
		}
		case CTL_TEST11: {
			if (SendCallBusy_test11()) {
				HtRetry();
				break;
			}
			SendCall_test11(CTL_TEST12, PR_memAddr);
			break;
		}
		case CTL_TEST12: {
			if (SendCallBusy_test12()) {
				HtRetry();
				break;
			}
			SendCall_test12(CTL_RTN, PR_memAddr);
			break;
		}
		case CTL_RTN: {
			if (SendReturnBusy_main()) {
				HtRetry();
				break;
			}
			SendReturn_main();
			break;
		}
		default:
			assert(0);
		}
	}
}
