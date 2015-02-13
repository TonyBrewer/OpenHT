#include "Ht.h"
#include "PersPoll4.h"

void CPersPoll4::PersPoll4()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case POLL4_INIT:
		{
			P_loopCnt = 0;
			P_err = 0;
			P_memReadGrpId = PR_htId;

			HtContinue(POLL4_READ);
		}
		break;
		case POLL4_READ:
		{
			if (ReadMemBusy() || SendReturnBusy_poll4()) {
				HtRetry();
				break;
			}

			// Check if end of loop
			if (P_loopCnt == LOOP_CNT || P_err) {
				// Return to host interface
				SendReturn_poll4(P_err);
			} else {
				// Calculate memory read address
				MemAddr_t memRdAddr = P_arrayAddr + ((P_loopCnt & 0xf) << 3);

				// Issue read request to memory
				ReadMem_arrayMem4(memRdAddr, (Poll4Addr1_t)PR_htId);

				// Set address for reading memory response data
				P_arrayMemRdPtr = (Poll4Addr1_t)PR_htId;

				HtContinue(POLL4_TEST);
			}
		}
		break;
		case POLL4_TEST:
		{
			if (ReadMemBusy() || ReadMemPoll(PR_memReadGrpId)) {
				HtRetry();
				break;
			}
			if (GR_arrayMem4_data() != (P_loopCnt & 0xf)) {
				HtAssert(0, 0);
				P_err += 1;
			}

			// Poll4rement loop count
			P_loopCnt = P_loopCnt + 1;

			HtContinue(POLL4_READ);
		}
		break;
		default:
			assert(0);
		}
	}
}
