#include "Ht.h"
#include "PersRead.h"

void
CPersRead::PersRead()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case READ_ENTRY: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			if (P_flush) {
				// don't start new reads, check that reads are complete before txfr control to pipe
				if (ReadMemPoll()) {
					HtRetry();
					break;
				}
				HtContinue(READ_TFR);
			} else {
				// Memory read request
				ReadMem_opAMem(P_vaAddr, PR_idx, false);
				HtContinue(READ_LDB);
			}
		}
		break;
		case READ_LDB: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			// Memory read request
			ReadMem_opBMem(P_vbAddr, PR_idx);
			ReadMemPause(READ_TFR);
		}
		break;
		case READ_TFR: {
			if (SendTransferBusy_pipe()) {
				HtRetry();
				break;
			}
			SendTransfer_pipe(P_idx, P_vtAddr, P_scalar, P_operation, P_flush);
		}
		break;
		default:
			assert(0);
		}
	}
}
