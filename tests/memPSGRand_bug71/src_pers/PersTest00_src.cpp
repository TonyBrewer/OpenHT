#include "Ht.h"
#include "PersTest00.h"

void CPersTest00::PersTest00() {
	if (PR4_htValid) {
		switch (PR4_htInst) {
		case TEST00_ENTRY: {
			HtContinue(TEST00_WR);
			break;
		}
		case TEST00_WR: {
			P4_test00_0_src_v0_data = ((ht_int48)0x0005e9b4c8fece20LL);
			HtContinue(TEST00_ST0);
			break;
		}
		case TEST00_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test00_0_src_v0_data(PR4_memAddr + 0);
			WriteMemPause(TEST00_LD0);
			break;
		}
		case TEST00_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_0_src_v0_data(PR4_memAddr + 0);
			ReadMemPause(TEST00_CHK);
			break;
		}
		case TEST00_CHK: {
			if (PR4_test00_0_src_v0_data != (int64_t)0xffffe9b4c8fece20LL) {
				HtAssert(0, 0);
			}
			HtContinue(TEST00_RTN);
			break;
		}
		case TEST00_RTN: {
			if (SendReturnBusy_test00()) {
				HtRetry();
				break;
			}
			SendReturn_test00();
			break;
		}
		default:
			assert(0);
		}
	}
}
