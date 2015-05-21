#include "Ht.h"
#include "PersAdd.h"

// Include file with primitives
#include "PersAdd_prim.h"

void
CPersAdd::PersAdd()
{
	// Set read address of op1Mem/op2Mem/resMem variables
	// These will always be the same in every instruction for each thread
	S_op1Mem.read_addr(PR1_htId);
	S_op2Mem.read_addr(PR1_htId);
	S_resMem.read_addr(PR1_htId);

	// Force "Inputs Valid" to default to false unless true in the ADD_PAUSE instruction
	P1_i_vld = false;

	if (PR1_htValid) {
		switch (PR1_htInst) {
		case ADD_LD1: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			// Memory read request - Operand 1
			MemAddr_t memRdAddr = SR_op1Addr + (P1_vecIdx << 3);
			ReadMem_op1Mem(memRdAddr, PR1_htId);
			HtContinue(ADD_LD2);
		}
		break;
		case ADD_LD2: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			// Memory read request - Operand 2
			MemAddr_t memRdAddr = SR_op2Addr + (P1_vecIdx << 3);
			ReadMem_op2Mem(memRdAddr, PR1_htId);
			ReadMemPause(ADD_PAUSE);
		}
		break;
		case ADD_PAUSE: {
			// Store op1 and op2 into private variables 'a' and 'b'.
			P1_a = S_op1Mem.read_mem();
			P1_b = S_op2Mem.read_mem();

			// Mark inputs as valid, set htId
			P1_i_htId = PR1_htId;
			P1_i_vld = true;

			// Pause thread and wait for primitive to calculate the result...
			// (will return to ADD_ST)
			HtPause(ADD_ST);
		}
		break;
		case ADD_ST: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}

			// Memory write request - Addition Result
			MemAddr_t memWrAddr = SR_resAddr + (P1_vecIdx << 3);
			WriteMem(memWrAddr, S_resMem.read_mem());
			WriteMemPause(ADD_RTN);
		}
		break;
		case ADD_RTN: {
			if (SendReturnBusy_add()) {
				HtRetry();
				break;
			}

			// Return Result from shared ram 'resMem'
			SendReturn_add(S_resMem.read_mem());
		}
		break;
		default:
			assert(0);
		}
	}

	// Temporary variables to use as outputs to the primitive
	// (these are not saved between cycles)
	uint64_t o_res;
	ht_uint7 o_htId;
	bool o_vld;

	// use clocked primitive
	add_5stage(P1_a, P1_b, P1_i_htId, P1_i_vld, o_res, o_htId, o_vld, add_prm_state1);

	// Check for valid outputs from the primitive
	if (o_vld) {
		// Store Result into shared ram to be written to memory later
		S_resMem.write_addr(o_htId);
		S_resMem.write_mem(o_res);

		// Wake up the thread (with corresponding htId)
		HtResume(o_htId);
	}
}
