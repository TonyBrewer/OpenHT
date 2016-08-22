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
			ReadMemPause(ADD_ST);
		}
		break;
		case ADD_ST: {
			BUSY_RETRY(WriteMemBusy());

			// Memory write request
			MemAddr_t memWrAddr = SR_op1Addr + (P_vecIdx << 3);
			WriteMem(memWrAddr, PR_op1 + 1);
			WriteMemPause(ADD_RTN);
		}
		break;
		case ADD_RTN: {
			BUSY_RETRY(SendReturnBusy_add());

			SendReturn_add();
		}
		break;
		default:
			assert(0);
		}
	}
}
