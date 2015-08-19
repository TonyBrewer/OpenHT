#include "Ht.h"
#include "PersTest10.h"

void CPersTest10::PersTest10() {
	if (PR2_htValid) {
		switch (PR2_htInst) {
		case TEST10_ENTRY: {
			HtContinue(TEST10_WR0);
			break;
		}
		case TEST10_WR0: {
			GW2_test10_0_src_v0_data.write_addr(0);
			GW2_test10_0_src_v0_data = ((ht_int61)0x0017f8c0c54785a0LL);
			HtContinue(TEST10_ST0);
			break;
		}
		case TEST10_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test10_0_src_v0_data(PR2_memAddr + 0);
			WriteMemPause(TEST10_LD0);
			break;
		}
		case TEST10_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test10_0_src_v0_data(PR2_memAddr + 0);
			P2_test10_0_src_v0_data_RdAddr1 = (ht_uint1)0;
			ReadMemPause(TEST10_CHK0);
			break;
		}
		case TEST10_CHK0: {
			if (GR2_test10_0_src_v0_data != ((ht_int61)0x0017f8c0c54785a0LL)) {
				HtAssert(0, 0);
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
}
