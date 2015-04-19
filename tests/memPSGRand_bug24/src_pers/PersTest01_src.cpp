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
			S_test01_0_v0_data.write_addr(2, 0);
			S_test01_0_v0_data.write_mem((uint32_t)0x001e30bb590b7c20LL);
			HtContinue(TEST01_ST0);
			break;
		}
		case TEST01_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test01_0_v0_data(PR_memAddr + 0, 2, 0, 1);
			WriteMemPause(TEST01_LD0);
			break;
		}
		case TEST01_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test01_0_v1_data(PR_memAddr + 0, 1, 1);
			P_test01_0_v1_data_RdAddr1 = (ht_uint1)1;
			ReadMemPause(TEST01_CHK);
			break;
		}
		case TEST01_CHK: {
			printf("EXP (Test01): 0x%08x\n", (uint32_t)0x001e30bb590b7c20LL);
			printf("ACT (Test01): 0x%08x\n", (uint32_t)PR_test01_0_v1_data);
			if ((uint32_t)PR_test01_0_v1_data != (uint32_t)0x001e30bb590b7c20LL) {
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
