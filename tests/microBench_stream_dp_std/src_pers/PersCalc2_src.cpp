#include "Ht.h"
#include "PersCalc2.h"

// Include file with primitives
#include "PersMult_prim.h"

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

void
CPersCalc2::PersCalc2()
{
	// Always set S_op3 to the proper address (every clock cycle)
	S_op2.read_addr(PR1_htId);
	S_op3.read_addr(PR1_htId);

	// Force "Inputs Valid" to default to false unless true in the CALC2_ISSUE instruction
	P1_i_vld = false;

	if (PR1_htValid) {
		switch (PR1_htInst) {
		case CALC2_LDC: {
			BUSY_RETRY(ReadMemBusy());

			// Memory read request
			MemAddr_t memRdAddr = SR_op3Addr + (P1_vecIdx << 3);
			ReadMem_op3(memRdAddr, PR1_htId, 1);
			ReadMemPause(CALC2_ISSUE);
		}
		break;
		case CALC2_ISSUE: {

			// Setup private variables, always reflect 64-bit copy of op3
			// Used as inputs to the black box multiplier
			P1_op3Mem = (uint64_t)S_op3.read_mem();

			// Mark inputs as valid
			if (SR_primRdy) {
				P1_i_vld = true;
				HtPause(CALC2_STB);
			} else {
				P1_i_vld = false;
				HtContinue(CALC2_ISSUE);
			}
		}
		break;
		case CALC2_STB: {
			BUSY_RETRY(WriteMemBusy());

			uint64_t result = (uint64_t)S_op2.read_mem();

			// Memory write request
			MemAddr_t memWrAddr = SR_op2Addr + (P1_vecIdx << 3);
			WriteMem(memWrAddr, result);
			WriteMemPause(CALC2_RTN);
		}
		break;
		case CALC2_RTN: {
			BUSY_RETRY(SendReturnBusy_calc2());

			SendReturn_calc2();
		}
		break;
		default:
			assert(0);
		}
	}

	// Temporary variables to use as outputs to the primitive
	// (these are not saved between cycles)
	bool o_rdy;
	uint64_t o_res;
	ht_uint9 o_htId;
	bool o_vld;

	// Instantiate clocked primitive
	mult_wrap(GR_htReset, P1_op3Mem, P1_scalar, PR1_htId, P1_i_vld, o_rdy, o_res, o_htId, o_vld, mult_prm_state1);
	S_primRdy = o_rdy;

	// Check for valid outputs from the primitive
	if (o_vld) {

		// Store Result into shared ram to be written to memory later
		S_op2.write_addr(o_htId);
		S_op2.write_mem(o_res);

		// Wake up the thread (with corresponding htId)
		HtResume(o_htId);
	}
}
