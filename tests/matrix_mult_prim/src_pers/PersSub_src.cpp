#include "Ht.h"
#include "PersSub.h"

void CPersSub::PersSub()
{

	if (PR_htValid) {
		switch (PR_htInst) {
		case SUB_ENTRY: {
			if (SendCallBusy_mult()) {
				HtRetry();
				break;
			}

			// Generate a seperate thread for each multiply operation within a matrix element
			if (P_calcIdx < SR_comRC) {
				SendCallFork_mult(SUB_JOIN, P_rowIdx, P_eleIdx, P_calcIdx);

				HtContinue(SUB_ENTRY);
				P_calcIdx += 1;
			} else {
				RecvReturnPause_mult(SUB_STORE);
			}
			break;
		}
		case SUB_JOIN: {
			// Add resulting products into a sum variable
			// The resulting element will be the sum of all multiply operations
			P_eleSum += P_result;
			RecvReturnJoin_mult();
			break;
		}
		case SUB_STORE: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}

			// Memory write request - element
			MemAddr_t memWrAddr = (ht_uint48)(SR_mcBase + ((P_rowIdx*SR_mcCol) << 3) + (P_eleIdx << 3));
			WriteMem(memWrAddr, P_eleSum);
			WriteMemPause(SUB_RTN);
			break;
		}
		case SUB_RTN: {
			if (SendReturnBusy_sub()) {
				HtRetry();
				break;
			}

			// Finished calculating matrix element
			SendReturn_sub();
			break;
		}
		default:
			assert(0);
		}
	}
}
