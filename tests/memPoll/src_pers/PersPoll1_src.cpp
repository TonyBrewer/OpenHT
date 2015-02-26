#include "Ht.h"
#include "PersPoll1.h"

void CPersPoll1::PersPoll1()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case POLL1_INIT:
		{
			P_loopCnt = 0;
			P_err = 0;

			HtContinue(POLL1_READ);
		}
		break;
		case POLL1_READ:
		{
			if (ReadMemBusy() || SendReturnBusy_poll1()) {
				HtRetry();
				break;
			}

			// Check if end of loop
			if (P_loopCnt == LOOP_CNT || P_err) {
				// Return to host interface
				SendReturn_poll1(P_err);
			} else {
				// Calculate memory read address
				MemAddr_t memRdAddr = P_arrayAddr + ((P_loopCnt & 0xf) << 3);

				// Issue read request to memory
				ReadMem_arrayMem1(memRdAddr, (Poll1Addr1_t)PR_htId);

				// Set address for reading memory response data
				P_arrayMemRdPtr = (Poll1Addr1_t)PR_htId;

				HtContinue(POLL1_WAIT);
			}
		}
		break;
		case POLL1_WAIT:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			ReadMemPoll(POLL1_TEST);
			break;
		}
		case POLL1_TEST:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			if (GR_arrayMem1.data != (P_loopCnt & 0xf)) {
				HtAssert(0, 0);
				P_err += 1;
			}

			// Poll1rement loop count
			P_loopCnt = P_loopCnt + 1;

			HtContinue(POLL1_READ);
		}
		break;
		default:
			assert(0);
		}
	}
}
