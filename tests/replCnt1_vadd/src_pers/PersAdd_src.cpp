#include "Ht.h"
#include "PersAdd.h"

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

void
CPersAdd::PersAdd()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case ADD_LD1: {
			BUSY_RETRY(ReadMemBusy());

			// Memory read request
			MemAddr_t memRdAddr = SR_op1Addr + (P_vecIdx << 3);
			ReadMem_op1(memRdAddr);
			HtContinue(ADD_LD2);
		}
		break;
		case ADD_LD2: {
			BUSY_RETRY(ReadMemBusy());

			// Memory read request
			MemAddr_t memRdAddr = SR_op2Addr + (P_vecIdx << 3);
			ReadMem_op2(memRdAddr);
			ReadMemPause(ADD_ST);
		}
		break;
		case ADD_ST: {
			BUSY_RETRY(WriteMemBusy());

			P_result = PR_op1 + PR_op2;

			// Memory write request
			MemAddr_t memWrAddr = SR_resAddr + (P_vecIdx << 3);
			WriteMem(memWrAddr, P_result);
			WriteMemPause(ADD_RTN);
		}
		break;
		case ADD_RTN: {
			BUSY_RETRY(SendReturnBusy_add());
			SendReturn_add((ht_uint1)SR_replId, P_result);
		}
		break;
		default:
			assert(0);
		}
	}
}
