#include "Ht.h"
#include "PersMult.h"

// Include file with primitives
#include "PersMult_prim.h"

void CPersMult::PersMult()
{
	// Always set S_op1Mem, S_op2Mem to the proper address (every clock cycle)
	S_op1Mem.read_addr(PR_htId);
	S_op2Mem.read_addr(PR_htId);
	S_resMem.read_addr(PR_htId);

	// Force "Inputs Valid" to default to false unless true in the ADD_PAUSE instruction
	P_i_vld = false;

	// Work to be done on Cycle 1
	if (PR_htValid) {
		switch (PR_htInst) {
		case MULT_LD1: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			// Memory read request - Op1
			MemAddr_t memRdAddr = (ht_uint48)(SR_maBase + (PR_rowIdx << 3) + ((SR_mcRow*PR_calcIdx) << 3));
			ReadMem_op1Mem(memRdAddr, PR_htId, 1);
			HtContinue(MULT_LD2);
			break;
		}
		case MULT_LD2: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			// Memory read request - Op2
			MemAddr_t memRdAddr = (ht_uint48)(SR_mbBase + (PR_eleIdx << 3) + ((SR_mcCol*PR_calcIdx) << 3));
			ReadMem_op2Mem(memRdAddr, PR_htId, 1);
			ReadMemPause(MULT_CALC);
			break;
		}
		case MULT_CALC: {

			// Setup private variables, always reflect 32-bit copy of op1 and op2
			// Used as inputs to the black box multiplier
			P_op1 = (uint32_t)S_op1Mem.read_mem();
			P_op2 = (uint32_t)S_op2Mem.read_mem();

			// Mark inputs as valid, set htId
			P_i_htId = PR_htId;
			P_i_vld = true;

			// Pause thread and wait for primitive to calculate the result...
			// (will return to MULT_RTN)
			HtPause(MULT_RTN);
			break;
		}
		case MULT_RTN: {
			if (SendReturnBusy_mult()) {
				HtRetry();
				break;
			}

			// Return finished product back to PersSub
			SendReturn_mult(S_resMem.read_mem());
			break;
		}
		default:
			assert(0);
		}
	}

	// Temporary variables to use as outputs to the primitive
	// (these are not saved between cycles)
	uint64_t o_res;
	ht_uint4 o_htId;
	bool o_vld;

	// Instantiate clocked primitive
	mult_wrap(P_op1, P_op2, P_i_htId, P_i_vld, o_res, o_htId, o_vld, mult_prm_state1);

	// Check for valid outputs from the primitive
	if (o_vld) {

		// Store Result into shared ram to be written to memory later
		S_resMem.write_addr(o_htId);
		S_resMem.write_mem(o_res);

		// Wake up the thread (with corresponding htId)
		HtResume(o_htId);
	}
}
