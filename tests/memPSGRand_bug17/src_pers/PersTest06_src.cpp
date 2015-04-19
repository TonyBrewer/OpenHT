#include "Ht.h"
#include "PersTest06.h"

void CPersTest06::PersTest06() {
	if (PR_htValid) {
		switch (PR_htInst) {
		case TEST06_ENTRY: {
			HtContinue(TEST06_WR);
			break;
		}
		case TEST06_WR: {
			P_test06_0_0_data[0] = (uint16_t)0x0006f11ab6f957e0LL;
			HtContinue(TEST06_ST0);
			break;
		}
		case TEST06_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_uint16_t(PR_memAddr + 0, P_test06_0_0_data[0]);
			WriteMemPause(TEST06_LD0);
			break;
		}
		case TEST06_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test06_0_1_data(PR_memAddr + 0, 0, 1);
			P_test06_0_1_data_RdAddr1 = (ht_uint1)0;
			ReadMemPause(TEST06_CHK);
			break;
		}
		case TEST06_CHK: {
			if ((uint16_t)GR_test06_0_1_data != (uint16_t)0x0006f11ab6f957e0LL) {
				HtAssert(0, (uint32_t)0x00060000);
			}
			HtContinue(TEST06_RTN);
			break;
		}
		case TEST06_RTN: {
			if (SendReturnBusy_test06()) {
				HtRetry();
				break;
			}
			SendReturn_test06();
			break;
		}
		default:
			assert(0);
		}
	}
}
