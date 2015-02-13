#include "Ht.h"
#include "PersMult.h"

// Include file with primitives
#include "PersMult_prim.h"

void CPersMult::PersMult() {

	// Always set S_op1Mem, S_op2Mem to the proper address
	// This should happen on every clock cycle to ensure they are
	//   always set correctly for each thread
	S_op1Mem.read_addr(PR1_htId);
	S_op2Mem.read_addr(PR1_htId);

	// Instruction decoding for Cycle 1
	if (PR1_htValid) {
		switch (PR1_htInst) {

		// Main Entry Point from PersProcess_src.cpp -> MULT_LD1
		// Determine Address of Coefficient for the corresponding calcIdx
		// Load Coefficient into op1Mem indexed by the thread ID
		// Issue Read Request and continue to MULT_LD2
		case MULT_LD1: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			MemAddr_t memRdAddr = (ht_uint48)(SR_cofAddrBase + (PR1_calcIdx << 3));
			ReadMem_op1Mem(memRdAddr, PR1_htId);
			HtContinue(MULT_LD2);
		}
		break;

		// Determine Address of Input Data for the corresponding calcIdx/rcvIdx
		// Load Input Data into op2Mem indexed by the thread ID
		// Issue Read Request and wait until complete, then continue to MULT_CALC
		case MULT_LD2: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			MemAddr_t memRdAddr = (ht_uint48)(SR_inAddrBase + ((PR1_rcvIdx - PR1_calcIdx) << 3));
			ReadMem_op2Mem(memRdAddr, PR1_htId);
			ReadMemPause(MULT_CALC);
		}
		break;

		// Store 2 loaded inputs as 32-bit values in op1 and op2
		// Since these were 32-bit values in the host, we should do this
		case MULT_CALC: {

			P1_op1 = (int32_t)S_op1Mem.read_mem();
			P1_op2 = (int32_t)S_op2Mem.read_mem();
			HtContinue(MULT_RTN);
		}
		break;

		// Finished with multiply operation, return to PROCESS with the result and any errors
		case MULT_RTN: {
			if (SendReturnBusy_mult()) {
				HtRetry();
				break;
			}

			SendReturn_mult(PR1_result, PR1_errs);
		}
		break;

		default:
			assert(0);
		}
	}

	// Instantiate primitive - outputs valid 5 cycles later on Stage 6
	int64_t multOut;
	multiplier(P1_op1, P1_op2, multOut, mult_prm_state1);


	// Instruction decoding for Cycle 6
	if (PR6_htValid) {
		switch (PR6_htInst) {

		case MULT_LD1: {
			// Do Nothing
		}
		break;

		case MULT_LD2: {
			// Do Nothing
		}
		break;

		case MULT_CALC: {
			// Save result into a private variable so it can be reread when reentering the pipline
			P6_result = multOut;
		}
		break;

		case MULT_RTN: {
			// Do Nothing
		}
		break;

		default:
			assert(0);

		}
	}
}
