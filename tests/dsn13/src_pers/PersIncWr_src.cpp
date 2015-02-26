#include "Ht.h"
#include "PersIncWr.h"

void CPersIncWr::PersIncWr()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case INCWR_WRITE:
		{
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}

			// Increment memory data
			uint64_t memWrData = GR_arrayMem.data;

			// Calculate memory write address
			MemAddr_t memWrAddr = SR_arrayAddr + (P_loopCnt << 3);

			// Issue write memory request
			WriteMem(memWrAddr, memWrData);

			WriteMemPause(INCWR_RTN);
		}
		break;
		case INCWR_RTN:
		{
			if (SendReturnBusy_incWr()) {
				HtRetry();
				break;
			}

			// Return to incRd
			SendReturn_incWr();
		}
		break;
		default:
			assert(0);
		}
	}
}
