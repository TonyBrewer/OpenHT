#include "Ht.h"
#include "PersCalc1.h"

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

void
CPersCalc1::PersCalc1()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case CALC1_LDA: {
			BUSY_RETRY(ReadMemBusy());

			// Memory read request
			MemAddr_t memRdAddr = SR_op1Addr + (P_vecIdx << 3);
			ReadMem_op1(memRdAddr);
			ReadMemPause(CALC1_STC);
		}
		break;
		case CALC1_STC: {
			BUSY_RETRY(WriteMemBusy());

			uint64_t result = PR_op1;

			// Memory write request
			MemAddr_t memWrAddr = SR_op3Addr + (P_vecIdx << 3);
			WriteMem(memWrAddr, result);
			WriteMemPause(CALC1_RTN);
		}
		break;
		case CALC1_RTN: {
			BUSY_RETRY(SendReturnBusy_calc1());

			SendReturn_calc1();
		}
		break;
		default:
			assert(0);
		}
	}
}
