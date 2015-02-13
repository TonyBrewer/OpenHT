#include "Ht.h"
#include "PersCtl.h"

void CPersCtl::PersCtl() {
	if (PR_htValid) {
		switch (PR_htInst) {

		// Main Entry Point from Host -> CTL_ENTRY
		// Check if AU is unused (lastIdx == -1) and return if so
		// P_rcvAu is technically created here, but never used (is in for debug if needed)
		// Continue to CTL_RECV to listen for Host Data
		case CTL_ENTRY: {
			if (P_lastIdx == -1) {
				HtContinue(CTL_RTN);
			}

			HtContinue(CTL_RECV);
		}
		break;

		// Store Received Host Data into P_rcvIdx
		// Dispatch threads to process the received Index
		// Continue listening until P_rcvIdx == S_lastIdx, at which point
		//   wait for all threads to return before continuing to CTL_RTN
		case CTL_RECV: {
			if (RecvHostDataBusy() || SendCallBusy_process()) {
				HtRetry();
				break;
			}

			P_rcvIdx = (uint32_t)RecvHostData();

			if (P_rcvIdx != (uint32_t)P_lastIdx) {
				SendCallFork_process(CTL_SEND, P_rcvIdx);
				HtContinue(CTL_RECV);
			} else {
				SendCallFork_process(CTL_SEND, P_rcvIdx);
				RecvReturnPause_process(CTL_RTN);
			}

		}
		break;

		// Join spawned Process threads
		// The P_sndIdx and P_errRcv variables are passed in as these threads return,
		//   send that back to the Host to signal that index being completed
		// Accumulate errors from the returning thread
		// P_sndIdx should be the same as P_rcvIdx for that thread
		case CTL_SEND: {
			if (SendHostDataBusy()) {
				HtRetry();
				break;
			}

			P_errs += P_errRcv;
			SendHostData((uint64_t)P_sndIdx);
			RecvReturnJoin_process();
		}
		break;

		// Done with all work for this AU
		// Return to the Host with the number of total errors seen
		case CTL_RTN: {
			if (SendReturnBusy_htmain()) {
				HtRetry();
				break;
			}

			SendReturn_htmain(P_errs);
		}
		break;

		default:
			assert(0);

		}
	}
}
