#include "Ht.h"
#include "PersPoll2.h"

void CPersPoll2::PersPoll2()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case POLL2_INIT:
		{
			P_loopCnt = 0;
			P_err = 0;

			HtContinue(POLL2_READ);
		}
		break;
		case POLL2_READ:
		{
			if (ReadMemBusy() || SendReturnBusy_poll2()) {
				HtRetry();
				break;
			}

			// Check if end of loop
			if (P_loopCnt == LOOP_CNT || P_err) {
				// Return to host interface
				SendReturn_poll2(P_err);
			} else {
				// Calculate memory read address
				MemAddr_t memRdAddr = P_arrayAddr + ((P_loopCnt & 0xf) << 3);

				// Issue read request to memory
				ReadMem_arrayMem2(memRdAddr, (Poll2Addr1_t)PR_htId);

				// Set address for reading memory response data
				P_arrayMemRdPtr = (Poll2Addr1_t)PR_htId;

				HtContinue(POLL2_TEST);
			}
		}
		break;
		case POLL2_TEST:
		{
			if (ReadMemBusy() || ReadMemPoll()) {
				HtRetry();
				break;
			}
			if (GR_arrayMem2_data() != (P_loopCnt & 0xf)) {
				HtAssert(0, 0);
				P_err += 1;
			}

			// Poll2rement loop count
			P_loopCnt = P_loopCnt + 1;

			HtContinue(POLL2_READ);
		}
		break;
		default:
			assert(0);
		}
	}
}
