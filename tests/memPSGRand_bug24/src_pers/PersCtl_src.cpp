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
			SendCall_test01(CTL_RTN, PR_memAddr);
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
