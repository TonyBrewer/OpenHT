#include "Ht.h"
#include "PersCtl.h"

void CPersCtl::PersCtl() {
	if (PR_htValid) {
		switch (PR_htInst) {
		case CTL_ENTRY: {
			switch (PR_testId) {
			case 0: {
				if (SendCallBusy_rdPause()) {
					HtRetry();
					break;
				}
				SendCall_rdPause(CTL_RTN, PR_threadId, PR_memAddr);
				break;
			}
			case 1: {
				if (SendCallBusy_wrPause()) {
					HtRetry();
					break;
				}
				SendCall_wrPause(CTL_RTN, PR_threadId, PR_memAddr);
				break;
			}
			default:
				assert(0);
			}
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
