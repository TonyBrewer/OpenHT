#include "Ht.h"
#include "PersMuladd.h"

// Include file with primitives
#include "PersMuladd_prim.h"

void CPersMuladd::PersMuladd()
{
	// Always set S_opXMem, S_opYMem to the proper address (every clock cycle)
	S_opXMem.read_addr(PR1_htId);
	S_opYMem.read_addr(PR1_htId);
	S_resMem.read_addr(PR1_htId);

	// Force "Inputs Valid" to default to false unless true in the ADD_PAUSE instruction
	P1_i_vld = false;

	// Work to be done on Cycle 1
	if (PR1_htValid) {
		switch (PR1_htInst) {
		case MULADD_LD1: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			// Memory read request - OpX
			MemAddr_t memRdAddr = SR_opXAddr + (P1_vecIdx << 3);
			ReadMem_opXMem(memRdAddr, PR1_htId, 1);
			HtContinue(MULADD_LD2);
			break;
		}
		case MULADD_LD2: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			// Memory read request - OpY
			MemAddr_t memRdAddr = SR_opYAddr + (P1_vecIdx << 3);
			ReadMem_opYMem(memRdAddr, PR1_htId, 1);
			ReadMemPause(MULADD_CALC);
			break;
		}
		case MULADD_CALC: {

			// Setup private variables, always reflect 64-bit copy of opX and opY
			// Used as inputs to the black box multiplier/adder
			P1_x = S_opXMem.read_mem();
			P1_y = S_opYMem.read_mem();

			// Mark inputs as valid,
			if (SR_primRdy) {
				P1_i_vld = true;
				HtPause(MULADD_ST);
			} else {
				P1_i_vld = false;
				HtContinue(MULADD_CALC);
			}
			break;
		}
		case MULADD_ST: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}

			// Memory write request - Multiply/Add Result
			MemAddr_t memWrAddr = SR_opYAddr + (P1_vecIdx << 3);
			WriteMem(memWrAddr, S_resMem.read_mem());
			WriteMemPause(MULADD_RTN);
		}
		break;
		case MULADD_RTN: {
			if (SendReturnBusy_muladd()) {
				HtRetry();
				break;
			}

			// Return finished product back to PersSub
			SendReturn_muladd();
			break;
		}
		default:
			assert(0);
		}
	}

	// Temporary variables to use as outputs to the primitive
	// (these are not saved between cycles)
	bool o_rdy;
	uint64_t o_res;
	ht_uint7 o_htId;
	bool o_vld;

	// Instantiate clocked primitive
	muladd_wrap(GR_htReset, P1_y, P1_x, PR1_t, PR1_htId, P1_i_vld, o_rdy, o_res, o_htId, o_vld, muladd_prm_state1);
	S_primRdy = o_rdy;

	// Check for valid outputs from the primitive
	if (o_vld) {

		// Store Result into shared ram to be written to memory later
		S_resMem.write_addr(o_htId);
		S_resMem.write_mem(o_res);

		// Wake up the thread (with corresponding htId)
		HtResume(o_htId);
	}
}
