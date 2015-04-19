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
			PW_test02_0_0_data[0].write_addr(1);
			PW_test02_0_0_data[0] = (uint16_t)0x0009524eb45171c0LL;
			HtContinue(TEST02_ST0);
			break;
		}
		case TEST02_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test02_0_0_data(PR_memAddr + 0, 1, 0, 1);
			WriteMemPause(TEST02_LD0);
			break;
		}
		case TEST02_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test02_0_1_data(PR_memAddr + 0, 1, 2, 1);
			P_test02_0_1_data_RdAddr1 = (ht_uint2)1;
			P_test02_0_1_data_RdAddr2 = (ht_uint2)2;
			ReadMemPause(TEST02_CHK);
			break;
		}
		case TEST02_CHK: {
			if ((uint16_t)GR_test02_0_1_data[1] != (uint16_t)0x0009524eb45171c0LL) {
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
