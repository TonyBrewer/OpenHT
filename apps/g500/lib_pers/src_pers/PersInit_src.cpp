#include "Ht.h"
#include "PersInit.h"

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

void
CPersInit::PersInit()
{
	for (int i=0; i<8; i++)
		S_init[i] = 0xffffffffffffffffull;

	if (PR_htValid) {
		switch (PR_htInst) {
		case INIT_RUN: {

			BUSY_RETRY(WriteMemBusy());

			assert(SR_bfsSize < (1ull << 32));
			uint32_t rem = (uint32_t)(P_endIdx - P_bfsIdx);

			ht_uint4 cnt;
			if (rem < 8)
				cnt = (ht_uint4)rem;
			else
				cnt = 8;

			MemAddr_t memAddr = SR_bfsAddr + (MemAddr_t)(P_bfsIdx << 3);
			WriteMem_init(memAddr, 0, cnt);

			P_bfsIdx += 8;

			if (rem <= 8)
				WriteMemPause(INIT_RTN);
			else
				HtContinue(INIT_RUN);
		}
		break;
		case INIT_RTN: {
			BUSY_RETRY(SendReturnBusy_init());

			SendReturn_init();
		}
		break;
		default:
			assert(0);
		}
	}
}
