#include "Ht.h"
#include "PersTest00.h"

void CPersTest00::PersTest00() {
	if (PR5_htValid) {
		switch (PR5_htInst) {
		case TEST00_ENTRY: {
			HtContinue(TEST00_WR);
			break;
		}
		case TEST00_WR: {
			S_test00_0_src_v0_data = ((uint64_t)0x0006fda31171dae0LL);
			HtContinue(TEST00_ST0);
			break;
		}
		case TEST00_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test00_0_src_v0_data(PR5_memAddr + 0);
			WriteMemPause(TEST00_LD0);
			break;
		}
		case TEST00_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_0_src_v0_data(PR5_memAddr + 0);
			ReadMemPause(TEST00_CHK);
			break;
		}
		case TEST00_CHK: {
			printf("EXP: 0x%016lx\n", (uint64_t)0x0006fda31171dae0LL);
			printf("ACT: 0x%016lx\n", (uint64_t)S_test00_0_src_v0_data);
			if ((uint64_t)S_test00_0_src_v0_data != ((uint64_t)0x0006fda31171dae0LL)) {
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
