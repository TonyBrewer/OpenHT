#include "Ht.h"
#include "PersAdd.h"

// Include file with primitives
#include "PersAdd_prim.h"

void
CPersAdd::PersAdd()
{
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
			ReadMemPause(ADD_ADD);
		}
		break;
		case ADD_ADD: {

			// Set read address of op1Mem/op2Mem variables
			// This can be done outside of the "if (PR1_htValid)" statement,
			// because these are the same for each clock cycle of a single thread.
			S_op1Mem.read_addr(PR1_htId);
			S_op2Mem.read_addr(PR1_htId);

			// Store op1 and op2 into staged variables 'a' and 'b'.
			T1_a = S_op1Mem.read_mem();
			T1_b = S_op2Mem.read_mem();

			HtContinue(ADD_ST);
		}
		break;
		case ADD_ST: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}

			// Memory write request - Addition Result
			MemAddr_t memWrAddr = SR_resAddr + (P1_vecIdx << 3);
			WriteMem(memWrAddr, P1_res);
			WriteMemPause(ADD_RTN);
		}
		break;
		case ADD_RTN: {
			if (SendReturnBusy_add()) {
				HtRetry();
				break;
			}

			// Return Result from private variable 'res'
			SendReturn_add(P1_res);
		}
		break;
		default:
			assert(0);
		}
	}

	// use clocked primitive
	add_5stage(T1_a, T1_b, T6_res, add_prm_state1);

	if (PR6_htValid) {
		switch (PR6_htInst) {
		case ADD_LD1: {
			// Do nothing
		}
		break;
		case ADD_LD2: {
			// Do nothing
		}
		break;
		case ADD_ADD: {

			// Save the result from the output of the black box
			// 'S6_res' into a private variable 'P6_res'.
			//
			// This ensures that the result is saved in private state
			// for the ADD_ST instruction to write the result to memory.
			P6_res = T6_res;
		}
		break;
		case ADD_ST: {
			// Do nothing
		}
		break;
		case ADD_RTN: {
			// Do nothing
		}
		break;
		default:
			assert(0);
		}
	}

}
