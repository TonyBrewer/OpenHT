#include "Ht.h"
#include "PersTest03.h"

void CPersTest03::PersTest03() {
	if (PR2_htValid) {
		switch (PR2_htInst) {
		case TEST03_ENTRY: {
			HtContinue(TEST03_WR);
			break;
		}
		case TEST03_WR: {
			GW2_test03_0_src_v0_data[0][0].write_addr(0);
			GW2_test03_0_src_v0_data[0][0] = ((int64_t)0x00103b23c8678660LL);
			P2_test03_0_src_v0_data_RdAddr1 = (ht_uint2)0;
			HtContinue(TEST03_ST0);
			break;
		}
		case TEST03_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_int64_t(PR2_memAddr + 0, GR2_test03_0_src_v0_data[0][0]);
			WriteMemPause(TEST03_LD0);
			break;
		}
		case TEST03_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test03_0_src_v0_data(PR2_memAddr + 0, 0, 0, 0, 1);
			P2_test03_0_src_v0_data_RdAddr1 = (ht_uint2)0;
			ReadMemPause(TEST03_CHK);
			break;
		}
		case TEST03_CHK: {
			if ((int64_t)GR2_test03_0_src_v0_data[0][0] != ((int64_t)0x00103b23c8678660LL)) {
				HtAssert(0, 0);
			}
			HtContinue(TEST03_RTN);
			break;
		}
		case TEST03_RTN: {
			if (SendReturnBusy_test03()) {
				HtRetry();
				break;
			}
			SendReturn_test03();
			break;
		}
		default:
			assert(0);
		}
	}
}
