#include "Ht.h"
#include "PersCalc4.h"

// Include file with primitives
#include "PersMult_prim.h"

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

void
CPersCalc4::PersCalc4()
{
	// Always set S_op3 to the proper address (every clock cycle)
	S_op2.read_addr(PR1_htId);
	S_op3.read_addr(PR1_htId);
	S_op3X.read_addr(PR1_htId);

	// Force "Inputs Valid" to default to false unless true in the CALC4_ISSUE instruction
	P1_i_vld = false;

	if (PR1_htValid) {
		switch (PR1_htInst) {
		case CALC4_LDB: {
			BUSY_RETRY(ReadMemBusy());

			// Memory read request
			MemAddr_t memRdAddr = SR_op2Addr + (P1_vecIdx << 3);
			ReadMem_op2(memRdAddr, PR1_htId, 1);
			HtContinue(CALC4_LDC);
		}
		break;
		case CALC4_LDC: {
			BUSY_RETRY(ReadMemBusy());

			// Memory read request
			MemAddr_t memRdAddr = SR_op3Addr + (P1_vecIdx << 3);
			ReadMem_op3(memRdAddr, PR1_htId, 1);
			ReadMemPause(CALC4_ISSUE);
		}
		break;
		case CALC4_ISSUE: {

			// Setup private variables, always reflect 64-bit copy of op3
			// Used as inputs to the black box multiplier
			P1_op3Mem = (uint64_t)S_op3.read_mem();

			// Mark inputs as valid, set htId
			P1_i_htId = PR1_htId;
			P1_i_vld = true;

			// Pause thread and wait for primitive to calculate the result...
			// (will return to CALC4_STA)
			HtPause(CALC4_STA);
		}
		break;
		case CALC4_STA: {
			BUSY_RETRY(WriteMemBusy());

			uint64_t result = (uint64_t)(SR_op2.read_mem() + SR_op3X.read_mem());

			// Memory write request
			MemAddr_t memWrAddr = SR_op1Addr + (P1_vecIdx << 3);
			WriteMem(memWrAddr, result);
			WriteMemPause(CALC4_RTN);
		}
		break;
		case CALC4_RTN: {
			BUSY_RETRY(SendReturnBusy_calc4());

			SendReturn_calc4();
		}
		break;
		default:
			assert(0);
		}
	}

	// Temporary variables to use as outputs to the primitive
	// (these are not saved between cycles)
	uint64_t o_res;
	ht_uint9 o_htId;
	bool o_vld;

	// Instantiate clocked primitive
	mult_wrap(P1_op3Mem, SR_scalar, P1_i_htId, P1_i_vld, o_res, o_htId, o_vld, mult_prm_state1);

	// Check for valid outputs from the primitive
	if (o_vld) {

		// Store Result into shared ram to be written to memory later
		S_op3X.write_addr(o_htId);
		S_op3X.write_mem(o_res);

		// Wake up the thread (with corresponding htId)
		HtResume(o_htId);
	}
}
