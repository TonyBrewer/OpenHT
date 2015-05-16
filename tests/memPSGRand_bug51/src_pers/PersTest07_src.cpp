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
			S_test07_0_src_v0_data[0][0].write_addr(2, 2);
			S_test07_0_src_v0_data[0][0].write_mem((int64_t)0x000b446373a6a660LL);
			HtContinue(TEST07_ST0);
			break;
		}
		case TEST07_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			S_test07_0_src_v0_data[0][0].read_addr(2, 2);
			WriteMem_int64_t(PR_memAddr + 0, S_test07_0_src_v0_data[0][0].read_mem());
			WriteMemPause(TEST07_LD0);
			break;
		}
		case TEST07_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test07_0_dst_v0_data(PR_memAddr + 0);
			ReadMemPause(TEST07_CHK);
			break;
		}
		case TEST07_CHK: {
			if ((int64_t)GR_test07_0_dst_v0_data != ((int64_t)0x000b446373a6a660LL)) {
				HtAssert(0, 0);
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
