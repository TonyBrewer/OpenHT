#include "Ht.h"
#include "PersTest10.h"

void CPersTest10::PersTest10() {
	if (PR1_htValid) {
		switch (PR1_htInst) {
		case TEST10_ENTRY: {
			HtContinue(TEST10_WR);
			break;
		}
		case TEST10_WR: {
			HtContinue(TEST10_ST0);
			break;
		}
		case TEST10_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test10_0_0_data(PR1_memAddr + 0, 0, 1);
			WriteMemPause(TEST10_LD0);
			break;
		}
		case TEST10_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test10_0_1_data(PR1_memAddr + 0, 0, 1);
			ReadMemPause(TEST10_CHK);
			break;
		}
		case TEST10_CHK: {
			if ((uint16_t)GR1_test10_0_1_data[0] != (uint16_t)0x00180873ececb9e0LL) {
				HtAssert(0, (uint32_t)0x000a0000);
			}
			HtContinue(TEST10_RTN);
			break;
		}
		case TEST10_RTN: {
			if (SendReturnBusy_test10()) {
				HtRetry();
				break;
			}
			SendReturn_test10();
			break;
		}
		default:
			assert(0);
		}
	}
	if (PR2_htValid) {
		switch (PR2_htInst) {
		case TEST10_ENTRY: {
			break;
		}
		case TEST10_WR: {
			PW2_test10_0_0_data[1].write_addr(0);
			PW2_test10_0_0_data[1] = (uint16_t)0x00180873ececb9e0LL;
			break;
		}
		case TEST10_ST0: {
			break;
		}
		case TEST10_LD0: {
			break;
		}
		case TEST10_CHK: {
			break;
		}
		case TEST10_RTN: {
			break;
		}
		default:
			assert(0);
		}
	}
}
