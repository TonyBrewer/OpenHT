#include "Ht.h"
#include "PersTest03.h"

void CPersTest03::PersTest03() {
	if (PR_htValid) {
		switch (PR_htInst) {
		case TEST03_ENTRY: {
			HtContinue(TEST03_WR);
			break;
		}
		case TEST03_WR: {
			GW_test03_0_0_data[0] = (uint16_t)0x000efb1080b3d7c0LL;
			HtContinue(TEST03_ST0);
			break;
		}
		case TEST03_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test03_0_0_data(PR_memAddr + 0);
			WriteMemPause(TEST03_LD0);
			break;
		}
		case TEST03_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test03_0_1_data(PR_memAddr + 0, 1, 1, 1, 1);
			P_test03_0_1_data_RdAddr1 = (ht_uint2)1;
			P_test03_0_1_data_RdAddr2 = (ht_uint1)1;
			ReadMemPause(TEST03_CHK);
			break;
		}
		case TEST03_CHK: {
			if ((uint16_t)PR_test03_0_1_data[1] != (uint16_t)0x000efb1080b3d7c0LL) {
				HtAssert(0, (uint32_t)0x00030000);
			}
			HtContinue(TEST03_RTN);
			break;
		}
		case TEST03_RTN: {
			if (SendReturnBusy_test03()) {
				HtRetry();
				break;
			}
			SendReturn_test03();
			break;
		}
		default:
			assert(0);
		}
	}
}
