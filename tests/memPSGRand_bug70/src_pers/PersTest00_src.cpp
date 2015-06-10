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
			PW_test00_0_dst_s0_data.test00_0_dst_s1_data.test00_0_dst_v1_data[0] = ((int8_t)0x000e618a5ec1b240LL);
			PW_test00_0_dst_s0_data.test00_0_dst_s1_data.test00_0_dst_v2_data[0][0] = ((int64_t)0x000c7954e20d9e20LL);
			PW_test00_0_dst_s0_data.test00_0_dst_s1_data.test00_0_dst_v2_data[1][0] = ((int64_t)0x000c7954e20d9e20LL);
			PW_test00_0_dst_s0_data.test00_0_dst_s1_data.test00_0_dst_v2_data[2][0] = ((int64_t)0x000c7954e20d9e20LL);
			PW_test00_0_dst_s0_data.test00_0_dst_s1_data.test00_0_dst_v2_data[3][0] = ((int64_t)0x000c7954e20d9e20LL);
			HtContinue(TEST00_ST0);
			break;
		}
		case TEST00_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test00_0_dst_s1_data_struct(PR_memAddr + 0, PR_test00_0_dst_s0_data.test00_0_dst_s1_data);
			WriteMemPause(TEST00_LD0);
			break;
		}
		case TEST00_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_0_dst_s1_data(PR_memAddr + 0);
			ReadMemPause(TEST00_CHK);
			break;
		}
		case TEST00_CHK: {
			if ((int8_t)PR_test00_0_dst_s0_data.test00_0_dst_s1_data.test00_0_dst_v1_data[0] != ((int8_t)0x000e618a5ec1b240LL)) {
				HtAssert(0, 0);
			}
			if ((int64_t)PR_test00_0_dst_s0_data.test00_0_dst_s1_data.test00_0_dst_v2_data[0][0] != ((int64_t)0x000c7954e20d9e20LL)) {
				HtAssert(0, 0);
			}
			if ((int64_t)PR_test00_0_dst_s0_data.test00_0_dst_s1_data.test00_0_dst_v2_data[1][0] != ((int64_t)0x000c7954e20d9e20LL)) {
				HtAssert(0, 0);
			}
			if ((int64_t)PR_test00_0_dst_s0_data.test00_0_dst_s1_data.test00_0_dst_v2_data[2][0] != ((int64_t)0x000c7954e20d9e20LL)) {
				HtAssert(0, 0);
			}
			if ((int64_t)PR_test00_0_dst_s0_data.test00_0_dst_s1_data.test00_0_dst_v2_data[3][0] != ((int64_t)0x000c7954e20d9e20LL)) {
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
