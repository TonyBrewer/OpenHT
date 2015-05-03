#include "Ht.h"
#include "PersTest00.h"

void CPersTest00::PersTest00() {
	if (PR_htValid) {
		switch (PR_htInst) {
		case TEST00_ENTRY: {
			HtContinue(TEST00_WR);
			break;
		}
		case TEST00_WR: {
			S_test00_0_src_u0_data[1].write_addr(1);
			S_test00_0_src_u0_data[1].write_mem().test00_0_src_s0_data.test00_0_src_v4_data = ((uint64_t)0x001dbc79c0644a00LL);
			HtContinue(TEST00_ST0);
			break;
		}
		case TEST00_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test00_0_src_v4_data(PR_memAddr + 0, 1, 1, 1);
			WriteMemPause(TEST00_LD0);
			break;
		}
		case TEST00_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_0_src_v4_data(PR_memAddr + 0, 1, 1, 1);
			ReadMemPause(TEST00_CHK);
			break;
		}
		case TEST00_CHK: {
			SR_test00_0_src_u0_data[1].read_addr(1);
			if ((uint64_t)S_test00_0_src_u0_data[1].read_mem().test00_0_src_s0_data.test00_0_src_v4_data != ((uint64_t)0x001dbc79c0644a00LL))
				HtAssert(0, (uint32_t)0x00000000);
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
