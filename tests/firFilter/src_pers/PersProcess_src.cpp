#include "Ht.h"
#include "PersProcess.h"

void CPersProcess::PersProcess() {
	if (PR_htValid) {
		switch (PR_htInst) {

		// Main Entry Point from PersCtl_src.cpp -> PROCESS_ENTRY
		// Calculate the number of multiplies we need to do and store in P_calcLen
		// Dispatch threads to do a muliply operation for that index
		// Continue spawning multiply threads until P_calcIdx == P_calcLen, at which point
		//   wait for all threads to return before continuing to PROCESS_STORE
		case PROCESS_ENTRY: {
			if (SendCallBusy_mult()) {
				HtRetry();
				break;
			}

			P_calcLen = ((P_rcvIdx+1) < SR_numTaps) ? (P_rcvIdx+1) : SR_numTaps;

			if (P_calcIdx < P_calcLen) {
				SendCallFork_mult(PROCESS_JOIN, P_rcvIdx, P_calcIdx);
				HtContinue(PROCESS_ENTRY);
				P_calcIdx += 1;
			} else {
				RecvReturnPause_mult(PROCESS_STORE);
			}
		}
		break;

		// Join spawned Mult threads
		// Sum Returned data from P_result into P_sum to later be stored to memory
		// Accumulate errors from the returning thread
		case PROCESS_JOIN: {
			P_sum += P_result;
			P_errs += P_errRcv;
			RecvReturnJoin_mult();
		}
		break;

		// Store P_sum into memory as an output of the FIR Filter
		// This is one element (index) in the entire output
		// Issue Write Request and wait until complete, then continue to PROCESS_RTN
		case PROCESS_STORE: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}

			MemAddr_t memWrAddr = (ht_uint48)(SR_outAddrBase + (P_rcvIdx << 3));
			WriteMem(memWrAddr, P_sum);
			WriteMemPause(PROCESS_RTN);
		}
		break;

		// Done with work for this Index
		// Return to CTL to echo the Index to the Host and accumulate errors
		case PROCESS_RTN: {
			if (SendReturnBusy_process()) {
				HtRetry();
				break;
			}

			SendReturn_process(P_rcvIdx, P_errs);
		}
		break;

		default:
			assert(0);

		}
	}
}
