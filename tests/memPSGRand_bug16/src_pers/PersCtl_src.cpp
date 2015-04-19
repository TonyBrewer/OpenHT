#include "Ht.h"
#include "PersCtl.h"

void CPersCtl::PersCtl() {
	if (PR_htValid) {
		switch (PR_htInst) {
		case CTL_TEST00: {
			if (SendCallBusy_test00()) {
				HtRetry();
				break;
			}
			SendCall_test00(CTL_TEST01, PR_memAddr);
			break;
		}
		case CTL_TEST01: {
			if (SendCallBusy_test01()) {
				HtRetry();
				break;
			}
			SendCall_test01(CTL_TEST02, PR_memAddr);
			break;
		}
		case CTL_TEST02: {
			if (SendCallBusy_test02()) {
				HtRetry();
				break;
			}
			SendCall_test02(CTL_TEST03, PR_memAddr);
			break;
		}
		case CTL_TEST03: {
			if (SendCallBusy_test03()) {
				HtRetry();
				break;
			}
			SendCall_test03(CTL_TEST04, PR_memAddr);
			break;
		}
		case CTL_TEST04: {
			if (SendCallBusy_test04()) {
				HtRetry();
				break;
			}
			SendCall_test04(CTL_RTN, PR_memAddr);
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
