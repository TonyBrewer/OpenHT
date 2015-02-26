#include "Ht.h"
#include "PersPause.h"

void CPersPause::PersPause()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case PAUSE_SWITCH:
		{
			switch (P_tstId) {
			case 0:
				if (SendCallBusy_rd1()) {
					HtRetry();
					break;
				}

				SendCall_rd1(PAUSE_RTN, P_arrayAddr);
				break;
			case 1:
				if (SendCallBusy_rd2()) {
					HtRetry();
					break;
				}

				SendCall_rd2(PAUSE_RTN, P_arrayAddr);
				break;
			case 2:
				if (SendCallBusy_rd3()) {
					HtRetry();
					break;
				}

				SendCall_rd3(PAUSE_RTN, P_arrayAddr);
				break;
			case 3:
				if (SendCallBusy_rd4()) {
					HtRetry();
					break;
				}

				SendCall_rd4(PAUSE_RTN, P_arrayAddr);
				break;
			case 4:
				if (SendCallBusy_wr1()) {
					HtRetry();
					break;
				}

				SendCall_wr1(PAUSE_RTN, P_arrayAddr);
				break;
			case 5:
				if (SendCallBusy_wr2()) {
					HtRetry();
					break;
				}

				SendCall_wr2(PAUSE_RTN, P_arrayAddr);
				break;
			case 6:
				if (SendCallBusy_wr3()) {
					HtRetry();
					break;
				}

				SendCall_wr3(PAUSE_RTN, P_arrayAddr);
				break;
			case 7:
				if (SendCallBusy_wr4()) {
					HtRetry();
					break;
				}

				SendCall_wr4(PAUSE_RTN, P_arrayAddr);
				break;
			default:
				HtAssert(0, P_tstId);
				break;
			}
		}
		break;
		case PAUSE_RTN:
		{
			if (SendReturnBusy_htmain()) {
				HtRetry();
				break;
			}

			// Return to host interface
			SendReturn_htmain(P_err);
		}
		break;
		default:
			assert(0);
		}
	}
}
