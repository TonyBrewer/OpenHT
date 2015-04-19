#include "Ht.h"
#include "PersTest00.h"

void CPersTest00::PersTest00() {
	if (PR_htValid) {
		switch (PR_htInst) {
		case TEST00_ENTRY: {
			HtContinue(TEST00_WR);
			break;
		}
		case TEST00_WR: {
			S_test00_0_v0_data = (uint32_t)0x001e30bb590b7c20LL;
			HtContinue(TEST00_ST0);
			break;
		}
		case TEST00_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test00_0_v0_data(PR_memAddr + 0);
			WriteMemPause(TEST00_LD0);
			break;
		}
		case TEST00_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_0_v1_data(PR_memAddr + 0, 1, 1);
			P_test00_0_v1_data_RdAddr1 = (ht_uint1)1;
			ReadMemPause(TEST00_CHK);
			break;
		}
		case TEST00_CHK: {
			printf("EXP (Test00): 0x%08x\n", (uint32_t)0x001e30bb590b7c20LL);
			printf("ACT (Test00): 0x%08x\n", (uint32_t)PR_test00_0_v1_data);
			if ((uint32_t)PR_test00_0_v1_data != (uint32_t)0x001e30bb590b7c20LL) {
				HtAssert(0, (uint32_t)0x00010000);
			}
			HtContinue(TEST00_RTN);
			break;
		}
		case TEST00_RTN: {
			if (SendReturnBusy_test00()) {
				HtRetry();
				break;
			}
			SendReturn_test00();
			break;
		}
		default:
			assert(0);
		}
	}
}
