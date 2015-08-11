#include "Ht.h"
#include "PersCtl.h"

void CPersCtl::PersCtl()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case ENTRY:
		{
			HtContinue(CALL_F1);
			break;
		}
		case CALL_F1:
		{
			if (SendCallBusy_f1()) {
				HtRetry();
				break;
			}

			SendCall_f1(CALL_F2, PR_addr);
			break;
		}
		case CALL_F2:
		{
			if (SendCallBusy_f2()) {
				HtRetry();
				break;
			}

			SendCall_f2(CALL_F3, PR_addr);
			break;
		}
		case CALL_F3:
		{
			if (SendCallBusy_f3()) {
				HtRetry();
				break;
			}

			SendCall_f3(CALL_F4, PR_addr);
			break;
		}
		case CALL_F4:
		{
			if (SendCallBusy_f4()) {
				HtRetry();
				break;
			}

			SendCall_f4(CALL_F5, PR_addr);
			break;
		}
		case CALL_F5:
		{
			if (SendCallBusy_f5()) {
				HtRetry();
				break;
			}

			SendCall_f5(CALL_F8, PR_addr);
			break;
		}
		case CALL_F8:
		{
			if (SendCallBusy_f8()) {
				HtRetry();
				break;
			}

			SendCall_f8(RETURN, PR_addr);
			break;
		}
		case RETURN:
		{
			if (SendReturnBusy_htmain()) {
				HtRetry();
				break;
			}

			SendReturn_htmain();
			break;
		}
		default:
			assert(0);
		}
	}
}
