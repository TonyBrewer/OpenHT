#include "Ht.h"
#include "PersPoll3.h"

void CPersPoll3::PersPoll3()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case POLL3_INIT:
		{
			P_loopCnt = 0;
			P_err = 0;

			HtContinue(POLL3_READ);
		}
		break;
		case POLL3_READ:
		{
			if (ReadMemBusy() || SendReturnBusy_poll3()) {
				HtRetry();
				break;
			}

			// Check if end of loop
			if (P_loopCnt == LOOP_CNT || P_err) {
				// Return to host interface
				SendReturn_poll3(P_err);
			} else {
				// Calculate memory read address
				MemAddr_t memRdAddr = P_arrayAddr + ((P_loopCnt & 0xf) << 3);

				// Issue read request to memory
				ReadMem_arrayMem3(memRdAddr, (Poll3Addr1_t)PR_htId);

				// Set address for reading memory response data
				P_arrayMemRdPtr = (Poll3Addr1_t)PR_htId;

				HtContinue(POLL3_TEST);
			}
		}
		break;
		case POLL3_TEST:
		{
			if (ReadMemBusy() || ReadMemPoll()) {
				HtRetry();
				break;
			}
			if (GR_arrayMem3_data() != (P_loopCnt & 0xf)) {
				HtAssert(0, 0);
				P_err += 1;
			}

			// Poll3rement loop count
			P_loopCnt = P_loopCnt + 1;

			HtContinue(POLL3_READ);
		}
		break;
		default:
			assert(0);
		}
	}
}
