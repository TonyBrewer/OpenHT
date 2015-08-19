#include "Ht.h"
#include "PersTest12.h"

void CPersTest12::PersTest12() {
	if (PR_htValid) {
		switch (PR_htInst) {
		case TEST12_ENTRY: {
			HtContinue(TEST12_WR0);
			break;
		}
		case TEST12_WR0: {
			S_test12_0_src_v0_data.write_addr(25, 6);
			S_test12_0_src_v0_data.write_mem((ht_int31)0x00039bff556a9f60LL);
			HtContinue(TEST12_ST0);
			break;
		}
		case TEST12_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			S_test12_0_src_v0_data.read_addr(25, 6);
			WriteMem_ht_int31(PR_memAddr + 0, S_test12_0_src_v0_data.read_mem());
			WriteMemPause(TEST12_LD0);
			break;
		}
		case TEST12_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test12_0_src_v0_data(PR_memAddr + 0, 25, 6, 1);
			ReadMemPause(TEST12_CHK0);
			break;
		}
		case TEST12_CHK0: {
			S_test12_0_src_v0_data.read_addr(25, 6);
			if (S_test12_0_src_v0_data.read_mem() != ((ht_int31)0x00039bff556a9f60LL)) {
				HtAssert(0, 0);
			}
			HtContinue(TEST12_RTN);
			break;
		}
		case TEST12_RTN: {
			if (SendReturnBusy_test12()) {
				HtRetry();
				break;
			}
			SendReturn_test12();
			break;
		}
		default:
			assert(0);
		}
	}
}
