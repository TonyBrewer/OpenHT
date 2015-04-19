#include "Ht.h"
#include "PersTest01.h"

void CPersTest01::PersTest01() {
	if (PR_htValid) {
		switch (PR_htInst) {
		case TEST01_ENTRY: {
			HtContinue(TEST01_WR);
			break;
		}
		case TEST01_WR: {
			GW_test01_0_0_data.test01_0_0_data[0] = (uint32_t)0x000a6fb2efa357a0LL;
			HtContinue(TEST01_ST0);
			break;
		}
		case TEST01_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test01_0_0_data(PR_memAddr + 0);
			WriteMemPause(TEST01_LD0);
			break;
		}
		case TEST01_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test01_0_3_data(PR_memAddr + 0);
			ReadMemPause(TEST01_CHK);
			break;
		}
		case TEST01_CHK: {
			if ((uint32_t)P_test01_0_2_data.test01_0_3_data[0] != (uint32_t)0x000a6fb2efa357a0LL) {
				HtAssert(0, (uint32_t)0x00010000);
			}
			HtContinue(TEST01_RTN);
			break;
		}
		case TEST01_RTN: {
			if (SendReturnBusy_test01()) {
				HtRetry();
				break;
			}
			SendReturn_test01();
			break;
		}
		default:
			assert(0);
		}
	}
}
