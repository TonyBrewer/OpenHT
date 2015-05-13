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
			PW_test00_0_src_v0_data[2][0].write_addr(1, 0);
			PW_test00_0_src_v0_data[2][0] = ((int64_t)0x001614a68845f080LL);
			HtContinue(TEST00_ST0);
			break;
		}
		case TEST00_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test00_0_src_v0_data(PR_memAddr + 0, 1, 0, 1);
			WriteMemPause(TEST00_LD0);
			break;
		}
		case TEST00_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_0_dst_v0_data(PR_memAddr + 0, 3, 1, 1);
			ReadMemPause(TEST00_CHK);
			break;
		}
		case TEST00_CHK: {
			SR_test00_0_dst_s0_data[0].read_addr(3, 1);
			if ((int64_t)S_test00_0_dst_s0_data[0].read_mem().test00_0_dst_v0_data[0][0] != ((int64_t)0x001614a68845f080LL)) {
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
