#include "Ht.h"
#include "PersCtl.h"

void CPersCtl::PersCtl()
{
	if (PR_htValid) {
		switch (PR_htInst) {

		case CTL_ROW: {
			if (SendCallBusy_row()) {
				HtRetry();
				break;
			}

			// Generate a seperate thread for each row of the result matrix
			if (P_rowIdx < SR_mcRow) {
				SendCallFork_row(CTL_JOIN, P_rowIdx, 0);

				HtContinue(CTL_ROW);
				P_rowIdx += P_rowStride;
			} else {
				RecvReturnPause_row(CTL_RTN);
			}
			break;
		}
		case CTL_JOIN: {
			RecvReturnJoin_row();
			break;
		}
		case CTL_RTN: {
			if (SendReturnBusy_htmain()) {
				HtRetry();
				break;
			}

			// Finished calculating result matrix
			SendReturn_htmain();
			break;
		}
		default:
			assert(0);

		}
	}
}
