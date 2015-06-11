#include "Ht.h"
#include "PersTest00.h"

void CPersTest00::PersTest00() {
	if (PR2_htValid) {
		switch (PR2_htInst) {
		case TEST00_ENTRY: {
			HtContinue(TEST00_WR);
			break;
		}
		case TEST00_WR: {
			PW2_test00_0_src_s0_data[2][1].write_addr(4, 14);
			PW2_test00_0_src_s0_data[2][1].test00_0_src_v4_data = ((int16_t)0x001497429f0822a0LL);
			HtContinue(TEST00_ST0);
			break;
		}
		case TEST00_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test00_0_src_v4_data(PR2_memAddr + 0, 4, 14, 2, 1, 1);
			WriteMemPause(TEST00_LD0);
			break;
		}
		case TEST00_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_0_dst_v0_data(PR2_memAddr + 0, 9, 1);
			ReadMemPause(TEST00_CHK);
			break;
		}
		case TEST00_CHK: {
			SR_test00_0_dst_v0_data.read_addr(9);
			if ((int16_t)SR_test00_0_dst_v0_data.read_mem() != ((int16_t)0x001497429f0822a0LL)) {
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
