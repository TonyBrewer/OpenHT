#include "Ht.h"
#include "PersTest05.h"

void CPersTest05::PersTest05() {
	if (PR_htValid) {
		switch (PR_htInst) {
		case TEST05_ENTRY: {
			HtContinue(TEST05_WR);
			break;
		}
		case TEST05_WR: {
			GW_test05_0_0_data[0] = (uint32_t)0x001d3cfea0c219e0LL;
			HtContinue(TEST05_ST0);
			break;
		}
		case TEST05_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test05_0_0_data(PR_memAddr + 0);
			WriteMemPause(TEST05_LD0);
			break;
		}
		case TEST05_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test05_0_1_data(PR_memAddr + 0, 0, 1);
			P_test05_0_1_data_RdAddr1 = (ht_uint1)0;
			ReadMemPause(TEST05_CHK);
			break;
		}
		case TEST05_CHK: {
			if ((uint32_t)PR_test05_0_1_data[0] != (uint32_t)0x001d3cfea0c219e0LL) {
				HtAssert(0, (uint32_t)0x00050000);
			}
			HtContinue(TEST05_RTN);
			break;
		}
		case TEST05_RTN: {
			if (SendReturnBusy_test05()) {
				HtRetry();
				break;
			}
			SendReturn_test05();
			break;
		}
		default:
			assert(0);
		}
	}
}
