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
			S_test00_0_src_s0_data[1][0].test00_0_src_v0_data[0][0] = ((int64_t)0x001f7a58947f5200LL);
			S_test00_0_src_s0_data[1][0].test00_0_src_v0_data[0][1] = ((int64_t)0x001f7a58947f5200LL);
			S_test00_0_src_s0_data[1][0].test00_0_src_v0_data[0][2] = ((int64_t)0x001f7a58947f5200LL);
			S_test00_0_src_s0_data[1][0].test00_0_src_v1_data[0][0] = ((uint16_t)0x0019fc3898d606e0LL);
			S_test00_0_src_s0_data[1][0].test00_0_src_v1_data[0][1] = ((uint16_t)0x0019fc3898d606e0LL);
			S_test00_0_src_s0_data[1][0].test00_0_src_v1_data[0][2] = ((uint16_t)0x0019fc3898d606e0LL);
			HtContinue(TEST00_ST0);
			break;
		}
		case TEST00_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test00_0_src_s0_data(PR_memAddr + 0, 1, 0, 1);
			WriteMemPause(TEST00_LD0);
			break;
		}
		case TEST00_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_0_src_s0_data(PR_memAddr + 0, 1, 0, 1);
			ReadMemPause(TEST00_CHK);
			break;
		}
		case TEST00_CHK: {
			if ((int64_t)SR_test00_0_src_s0_data[1][0].test00_0_src_v0_data[0][0] != ((int64_t)0x001f7a58947f5200LL)) {
				HtAssert(0, 0);
			}
			if ((int64_t)SR_test00_0_src_s0_data[1][0].test00_0_src_v0_data[0][1] != ((int64_t)0x001f7a58947f5200LL)) {
				HtAssert(0, 0);
			}
			if ((int64_t)SR_test00_0_src_s0_data[1][0].test00_0_src_v0_data[0][2] != ((int64_t)0x001f7a58947f5200LL)) {
				HtAssert(0, 0);
			}
			if ((uint16_t)S_test00_0_src_s0_data[1][0].test00_0_src_v1_data[0][0] != ((uint16_t)0x0019fc3898d606e0LL)) {
				HtAssert(0, 0);
			}
			if ((uint16_t)S_test00_0_src_s0_data[1][0].test00_0_src_v1_data[0][1] != ((uint16_t)0x0019fc3898d606e0LL)) {
				HtAssert(0, 0);
			}
			if ((uint16_t)S_test00_0_src_s0_data[1][0].test00_0_src_v1_data[0][2] != ((uint16_t)0x0019fc3898d606e0LL)) {
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
