#include "Ht.h"
#include "PersTest14.h"

void CPersTest14::PersTest14() {
	if (PR2_htValid) {
		switch (PR2_htInst) {
		case TEST14_ENTRY: {
			HtContinue(TEST14_WR);
			break;
		}
		case TEST14_WR: {
			GW2_test14_0_0_data[0].write_addr(2);
			GW2_test14_0_0_data[0] = (uint16_t)0x0011d11ae26e7480LL;
			HtContinue(TEST14_ST0);
			break;
		}
		case TEST14_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test14_0_0_data(PR2_memAddr + 0, 2, 1);
			WriteMemPause(TEST14_LD0);
			break;
		}
		case TEST14_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test14_0_1_data(PR2_memAddr + 0, 0, 1);
			ReadMemPause(TEST14_CHK);
			break;
		}
		case TEST14_CHK: {
			if ((uint16_t)GR2_test14_0_1_data[0] != (uint16_t)0x0011d11ae26e7480LL) {
				HtAssert(0, (uint32_t)0x000e0000);
			}
			HtContinue(TEST14_RTN);
			break;
		}
		case TEST14_RTN: {
			if (SendReturnBusy_test14()) {
				HtRetry();
				break;
			}
			SendReturn_test14();
			break;
		}
		default:
			assert(0);
		}
	}
}
