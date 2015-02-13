#include "Ht.h"
#include "PersPoll0.h"

void CPersPoll0::PersPoll0()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case POLL0_INIT:
		{
			P_loopCnt = 0;
			P_err = 0;

			HtContinue(POLL0_READ);
		}
		break;
		case POLL0_READ:
		{
			if (ReadMemBusy() || SendReturnBusy_poll0()) {
				HtRetry();
				break;
			}

			// Check if end of loop
			if (P_loopCnt == LOOP_CNT || P_err) {
				// Return to host interface
				SendReturn_poll0(P_err);
			} else {
				// Calculate memory read address
				MemAddr_t memRdAddr = P_arrayAddr + ((P_loopCnt & 0xf) << 3);

				// Issue read request to memory
				ReadMem_arrayMem0(memRdAddr, (Poll0Addr1_t)PR_htId);

				// Set address for reading memory response data
				P_arrayMemRdPtr = (Poll0Addr1_t)PR_htId;

				HtContinue(POLL0_TEST);
			}
		}
		break;
		case POLL0_TEST:
		{
			if (ReadMemBusy() || ReadMemPoll()) {
				HtRetry();
				break;
			}
			if (GR_arrayMem0_data() != (P_loopCnt & 0xf)) {
				HtAssert(0, 0);
				P_err += 1;
			}

			// Poll0rement loop count
			P_loopCnt = P_loopCnt + 1;

			HtContinue(POLL0_READ);
		}
		break;
		default:
			assert(0);
		}
	}
}
