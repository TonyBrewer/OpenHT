#include "Ht.h"
#include "PersCalc3.h"

// Include file with primitives
#include "PersAdd_prim.h"

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

void
CPersCalc3::PersCalc3()
{
	// Always set S_op3 to the proper address (every clock cycle)
	S_op1.read_addr(PR1_htId);
	S_op2.read_addr(PR1_htId);
	S_op3.read_addr(PR1_htId);

	// Force "Inputs Valid" to default to false unless true in the CALC2_ISSUE instruction
	P1_i_vld = false;

	if (PR1_htValid) {
		switch (PR1_htInst) {
		case CALC3_LDA: {
			BUSY_RETRY(ReadMemBusy());

			// Memory read request
			MemAddr_t memRdAddr = SR_op1Addr + (P1_vecIdx << 3);
			ReadMem_op1(memRdAddr, PR1_htId, 1);
			HtContinue(CALC3_LDB);
		}
		break;
		case CALC3_LDB: {
			BUSY_RETRY(ReadMemBusy());

			// Memory read request
			MemAddr_t memRdAddr = SR_op2Addr + (P1_vecIdx << 3);
			ReadMem_op2(memRdAddr, PR1_htId, 1);
			ReadMemPause(CALC3_ISSUE);
		}
		break;
		case CALC3_ISSUE: {

			// Setup private variables, always reflect 64-bit copy of op1/op2
			// Used as inputs to the black box adder
			P1_op1Mem = (uint64_t)S_op1.read_mem();
			P1_op2Mem = (uint64_t)S_op2.read_mem();

			// Mark inputs as valid
			if (SR_primRdy) {
				P1_i_vld = true;
				HtPause(CALC3_STC);
			} else {
				P1_i_vld = false;
				HtContinue(CALC3_ISSUE);
			}
		}
		break;
		case CALC3_STC: {
			BUSY_RETRY(WriteMemBusy());

			uint64_t result = (uint64_t)S_op3.read_mem();

			// Memory write request
			MemAddr_t memWrAddr = SR_op3Addr + (P1_vecIdx << 3);
			WriteMem(memWrAddr, result);
			WriteMemPause(CALC3_RTN);
		}
		break;
		case CALC3_RTN: {
			BUSY_RETRY(SendReturnBusy_calc3());

			SendReturn_calc3();
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
	add_wrap(GR_htReset, P1_op1Mem, P1_op2Mem, PR1_htId, P1_i_vld, o_rdy, o_res, o_htId, o_vld, add_prm_state1);
	S_primRdy = o_rdy;

	// Check for valid outputs from the primitive
	if (o_vld) {

		// Store Result into shared ram to be written to memory later
		S_op3.write_addr(o_htId);
		S_op3.write_mem(o_res);

		// Wake up the thread (with corresponding htId)
		HtResume(o_htId);
	}
}
