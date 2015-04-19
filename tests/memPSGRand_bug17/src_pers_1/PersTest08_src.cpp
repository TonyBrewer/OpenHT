#include "Ht.h"
#include "PersTest08.h"

void CPersTest08::PersTest08() {
	if (PR_htValid) {
		switch (PR_htInst) {
		case TEST08_ENTRY: {
			HtContinue(TEST08_WR);
			break;
		}
		case TEST08_WR: {
			P_test08_0_0_data = (uint16_t)0x000ec5952f7755c0LL;
			HtContinue(TEST08_ST0);
			break;
		}
		case TEST08_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_uint16_t(PR_memAddr + 0, P_test08_0_0_data);
			WriteMemPause(TEST08_LD0);
			break;
		}
		case TEST08_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test08_0_1_data(PR_memAddr + 0, 0, 2, 0, 1);
			P_test08_0_1_data_RdAddr1 = (ht_uint1)0;
			P_test08_0_1_data_RdAddr2 = (ht_uint2)2;
			ReadMemPause(TEST08_CHK);
			break;
		}
		case TEST08_CHK: {
			printf("EXP: 0x%04x\n", (uint16_t)0x000ec5952f7755c0LL);
			printf("ACT: 0x%04x\n", (uint16_t)GR_test08_0_1_data[0]);
			if ((uint16_t)GR_test08_0_1_data[0] != (uint16_t)0x000ec5952f7755c0LL) {
				HtAssert(0, (uint32_t)0x00080000);
			}
			HtContinue(TEST08_RTN);
			break;
		}
		case TEST08_RTN: {
			if (SendReturnBusy_test08()) {
				HtRetry();
				break;
			}
			SendReturn_test08();
			break;
		}
		default:
			assert(0);
		}
	}
}
