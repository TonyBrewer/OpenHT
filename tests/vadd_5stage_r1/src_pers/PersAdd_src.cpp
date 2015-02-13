#include "Ht.h"
#include "PersAdd.h"

// Include file with primitives
#include "PersAdd_prim.h"

void
CPersAdd::PersAdd()
{
	if (PR6_htValid) {
		switch (PR6_htInst) {
		case ADD_LD1: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			// Memory read request
			MemAddr_t memRdAddr = SR_op1Addr + (P6_vecIdx << 3);
			ReadMem_op1Mem(memRdAddr, PR6_htId);
			HtContinue(ADD_LD2);
		}
		break;
		case ADD_LD2: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			// Memory read request
			MemAddr_t memRdAddr = SR_op2Addr + (P6_vecIdx << 3);
			ReadMem_op2Mem(memRdAddr, PR6_htId);
			ReadMemPause(ADD_ST);
		}
		break;
		case ADD_ST: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}

			// Memory write request
			MemAddr_t memWrAddr = SR_resAddr + (P6_vecIdx << 3);
			WriteMem(memWrAddr, T6_res);
			WriteMemPause(ADD_RTN);
		}
		break;
		case ADD_RTN: {
			if (SendReturnBusy_add()) {
				HtRetry();
				break;
			}

			SendReturn_add(T6_res);
		}
		break;
		default:
			assert(0);
		}
	}

	if (PR1_htValid && PR1_htInst == ADD_ST) {
		bool stop = true;
	}

	S_op1Mem.read_addr(PR1_htId);
	S_op2Mem.read_addr(PR1_htId);
	T1_a = S_op1Mem.read_mem();
	T1_b = S_op2Mem.read_mem();

	// use clocked primitive
	add_5stage(T1_a, T1_b, T6_res, add_prm_state1);

}
