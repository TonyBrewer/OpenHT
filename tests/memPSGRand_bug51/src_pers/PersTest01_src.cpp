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
			GW_test01_0_src_v0_data[0] = ((int64_t)0x001c9031d35e9d20LL);
			HtContinue(TEST01_ST0);
			break;
		}
		case TEST01_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test01_0_src_v0_data(PR_memAddr + 0);
			WriteMemPause(TEST01_LD0);
			break;
		}
		case TEST01_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test01_0_src_v0_data(PR_memAddr + 0);
			ReadMemPause(TEST01_CHK);
			break;
		}
		case TEST01_CHK: {
			if ((int64_t)GR_test01_0_src_v0_data[0] != ((int64_t)0x001c9031d35e9d20LL)) {
				HtAssert(0, 0);
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
