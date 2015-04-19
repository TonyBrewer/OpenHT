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
			PW_test01_0_0_data[0].write_addr(1, 1);
			PW_test01_0_0_data[0] = (uint32_t)0x0008fc35477e21e0LL;
			P_test01_0_0_data_RdAddr1 = (ht_uint1)1;
			P_test01_0_0_data_RdAddr2 = (ht_uint2)1;
			HtContinue(TEST01_ST0);
			break;
		}
		case TEST01_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_uint32_t(PR_memAddr + 0, PR_test01_0_0_data[0]);
			WriteMemPause(TEST01_LD0);
			break;
		}
		case TEST01_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test01_0_1_data(PR_memAddr + 0);
			ReadMemPause(TEST01_CHK);
			break;
		}
		case TEST01_CHK: {
			if ((uint32_t)GR_test01_0_1_data != (uint32_t)0x0008fc35477e21e0LL) {
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
