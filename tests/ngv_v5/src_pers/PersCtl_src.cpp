#include "Ht.h"
#include "PersCtl.h"

void CPersCtl::PersCtl()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case ENTRY:
			if (SendCallBusy_gv1()) {
				HtRetry();
				break;
			}

			SendCall_gv1(CALL_GV2);
			break;
		case CALL_GV2:
			if (SendCallBusy_gv2()) {
				HtRetry();
				break;
			}

			SendCall_gv2(CALL_GV3);
			break;
		case CALL_GV3:
			if (SendCallBusy_gv3()) {
				HtRetry();
				break;
			}

			SendCall_gv3(CALL_GV4, PR_addr);
			break;
		case CALL_GV4:
			if (SendCallBusy_gv4()) {
				HtRetry();
				break;
			}

			SendCall_gv4(CALL_GV5, PR_addr);
			break;
		case CALL_GV5:
			if (SendCallBusy_gv5()) {
				HtRetry();
				break;
			}

			SendCall_gv5(CALL_GV6, PR_addr);
			break;
		case CALL_GV6:
			if (SendCallBusy_gv6()) {
				HtRetry();
				break;
			}

			SendCall_gv6(CALL_GV7, PR_addr);
			break;
		case CALL_GV7:
			if (SendCallBusy_gv7()) {
				HtRetry();
				break;
			}

			SendCall_gv7(CALL_GV9, PR_addr);
			break;
		case CALL_GV9:
			if (SendCallBusy_gv9()) {
				HtRetry();
				break;
			}

			SendCall_gv9(CALL_GV11, PR_addr);
			break;
		case CALL_GV11:
			if (SendCallBusy_gv11()) {
				HtRetry();
				break;
			}

			SendCall_gv11(CALL_GV13, PR_addr);
			break;
		case CALL_GV13:
			if (SendCallBusy_gv13()) {
				HtRetry();
				break;
			}

			SendCall_gv13(CALL_GV15, PR_addr);
			break;
		case CALL_GV15:
			if (SendCallBusy_gv15()) {
				HtRetry();
				break;
			}

			SendCall_gv15(RETURN);
			break;
		case RETURN:
			if (SendReturnBusy_htmain()) {
				HtRetry();
				break;
			}

			SendReturn_htmain();
			break;
		default:
			assert(0);
		}
	}
}
