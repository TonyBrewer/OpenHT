#include "Ht.h"
#include "PersTest02.h"

void CPersTest02::PersTest02() {
	if (PR_htValid) {
		switch (PR_htInst) {
		case TEST02_ENTRY: {
			HtContinue(TEST02_WR);
			break;
		}
		case TEST02_WR: {
			PW_test02_0_v0_data.write_addr(0);
			PW_test02_0_v0_data = (uint16_t)0x0014dbeb2dde6760LL;
			HtContinue(TEST02_ST0);
			break;
		}
		case TEST02_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test02_0_v0_data(PR_memAddr + 0, 0, 1);
			WriteMemPause(TEST02_LD0);
			break;
		}
		case TEST02_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test02_0_v3_data(PR_memAddr + 0);
			ReadMemPause(TEST02_CHK);
			break;
		}
		case TEST02_CHK: {
			if ((uint16_t)P_test02_0_u0_data.test02_0_v3_data != (uint16_t)0x0014dbeb2dde6760LL) {
				HtAssert(0, (uint32_t)0x00020000);
			}
			HtContinue(TEST02_RTN);
			break;
		}
		case TEST02_RTN: {
			if (SendReturnBusy_test02()) {
				HtRetry();
				break;
			}
			SendReturn_test02();
			break;
		}
		default:
			assert(0);
		}
	}
}
