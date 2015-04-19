#include "Ht.h"
#include "PersTest07.h"

void CPersTest07::PersTest07() {
	if (PR_htValid) {
		switch (PR_htInst) {
		case TEST07_ENTRY: {
			HtContinue(TEST07_WR);
			break;
		}
		case TEST07_WR: {
			GW_test07_0_0_data.write_addr(0, 0);
			GW_test07_0_0_data = (uint32_t)0x0006290c56c26fc0LL;
			HtContinue(TEST07_ST0);
			break;
		}
		case TEST07_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test07_0_0_data(PR_memAddr + 0, 0, 0, 1);
			WriteMemPause(TEST07_LD0);
			break;
		}
		case TEST07_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test07_0_0_data(PR_memAddr + 0, 1, 0, 1);
			P_test07_0_0_data_RdAddr1 = (ht_uint1)1;
			P_test07_0_0_data_RdAddr2 = (ht_uint2)0;
			ReadMemPause(TEST07_CHK);
			break;
		}
		case TEST07_CHK: {
			if ((uint32_t)GR_test07_0_0_data != (uint32_t)0x0006290c56c26fc0LL) {
				HtAssert(0, (uint32_t)0x00070000);
			}
			HtContinue(TEST07_RTN);
			break;
		}
		case TEST07_RTN: {
			if (SendReturnBusy_test07()) {
				HtRetry();
				break;
			}
			SendReturn_test07();
			break;
		}
		default:
			assert(0);
		}
	}
}
