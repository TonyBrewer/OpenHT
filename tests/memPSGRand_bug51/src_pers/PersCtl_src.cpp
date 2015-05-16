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
			SendCall_test04(CTL_TEST05, PR_memAddr);
			break;
		}
		case CTL_TEST05: {
			if (SendCallBusy_test05()) {
				HtRetry();
				break;
			}
			SendCall_test05(CTL_TEST06, PR_memAddr);
			break;
		}
		case CTL_TEST06: {
			if (SendCallBusy_test06()) {
				HtRetry();
				break;
			}
			SendCall_test06(CTL_TEST07, PR_memAddr);
			break;
		}
		case CTL_TEST07: {
			if (SendCallBusy_test07()) {
				HtRetry();
				break;
			}
			SendCall_test07(CTL_TEST08, PR_memAddr);
			break;
		}
		case CTL_TEST08: {
			if (SendCallBusy_test08()) {
				HtRetry();
				break;
			}
			SendCall_test08(CTL_TEST09, PR_memAddr);
			break;
		}
		case CTL_TEST09: {
			if (SendCallBusy_test09()) {
				HtRetry();
				break;
			}
			SendCall_test09(CTL_TEST10, PR_memAddr);
			break;
		}
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
