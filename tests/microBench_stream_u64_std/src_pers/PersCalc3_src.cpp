#include "Ht.h"
#include "PersCalc3.h"

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

void
CPersCalc3::PersCalc3()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case CALC3_LDA: {
			BUSY_RETRY(ReadMemBusy());

			// Memory read request
			MemAddr_t memRdAddr = SR_op1Addr + (P_vecIdx << 3);
			ReadMem_op1(memRdAddr);
			HtContinue(CALC3_LDB);
		}
		break;
		case CALC3_LDB: {
			BUSY_RETRY(ReadMemBusy());

			// Memory read request
			MemAddr_t memRdAddr = SR_op2Addr + (P_vecIdx << 3);
			ReadMem_op2(memRdAddr);
			ReadMemPause(CALC3_STC);
		}
		break;
		case CALC3_STC: {
			BUSY_RETRY(WriteMemBusy());

			uint64_t result = PR_op1 + PR_op2;

			// Memory write request
			MemAddr_t memWrAddr = SR_op3Addr + (P_vecIdx << 3);
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
}
