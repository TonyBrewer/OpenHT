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
			PW_test00_0_src_u0_data.test00_0_src_v8_data = ((int64_t)0x0009340e672b8ba0LL);
			HtContinue(TEST00_ST0);
			break;
		}
		case TEST00_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test00_0_src_u0_data_union(PR_memAddr + 0, PR_test00_0_src_u0_data);
			WriteMemPause(TEST00_LD0);
			break;
		}
		case TEST00_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_0_src_u0_data(PR_memAddr + 0);
			ReadMemPause(TEST00_CHK);
			break;
		}
		case TEST00_CHK: {
				printf("EXP (v4): 0x%02lx\n", (int8_t)0x00000000000000a0LL);
				printf("ACT (v4): 0x%02lx\n\n", (int8_t)PR_test00_0_src_u0_data.test00_0_src_v4_data);
				printf("EXP (v8): 0x%016lx\n", (int64_t)0x0009340e672b8ba0LL);
				printf("ACT (v8): 0x%016lx\n\n", (int64_t)PR_test00_0_src_u0_data.test00_0_src_v8_data);
				printf("EXP (v9[0][0]): 0x%02lx\n", (int8_t)0x00000000000000a0LL);
				printf("ACT (v9[0][0]): 0x%02lx\n\n", (int8_t)PR_test00_0_src_u0_data.test00_0_src_v9_data[0][0]);
				printf("EXP (v9[0][1]): 0x%02lx\n", (int8_t)0x000000000000008bLL);
				printf("ACT (v9[0][1]): 0x%02lx\n\n", (int8_t)PR_test00_0_src_u0_data.test00_0_src_v9_data[0][1]);
				printf("EXP (v9[0][2]): 0x%02lx\n", (int8_t)0x000000000000002bLL);
				printf("ACT (v9[0][2]): 0x%02lx\n\n", (int8_t)PR_test00_0_src_u0_data.test00_0_src_v9_data[0][2]);
				printf("EXP (v9[1][0]): 0x%02lx\n", (int8_t)0x0000000000000067LL);
				printf("ACT (v9[1][0]): 0x%02lx\n\n", (int8_t)PR_test00_0_src_u0_data.test00_0_src_v9_data[1][0]);
				printf("EXP (v9[1][1]): 0x%02lx\n", (int8_t)0x000000000000000eLL);
				printf("ACT (v9[1][1]): 0x%02lx\n\n", (int8_t)PR_test00_0_src_u0_data.test00_0_src_v9_data[1][1]);
				printf("EXP (v9[1][2]): 0x%02lx\n", (int8_t)0x0000000000000034LL);
				printf("ACT (v9[1][2]): 0x%02lx\n\n", (int8_t)PR_test00_0_src_u0_data.test00_0_src_v9_data[1][2]);
				printf("EXP (v9[2][0]): 0x%02lx\n", (int8_t)0x0000000000000009LL);
				printf("ACT (v9[2][0]): 0x%02lx\n\n", (int8_t)PR_test00_0_src_u0_data.test00_0_src_v9_data[2][0]);
				printf("EXP (v9[2][1]): 0x%02lx\n", (int8_t)0x0000000000000000LL);
				printf("ACT (v9[2][1]): 0x%02lx\n\n", (int8_t)PR_test00_0_src_u0_data.test00_0_src_v9_data[2][1]);

			if ((ht_int8)PR_test00_0_src_u0_data.test00_0_src_v4_data != ((ht_int8)0x00000000000000a0LL)) {
				HtAssert(0, 0);
			}
			if ((ht_int64)PR_test00_0_src_u0_data.test00_0_src_v8_data != ((ht_int64)0x0009340e672b8ba0LL)) {
				HtAssert(0, 0);
			}
			if ((ht_int8)PR_test00_0_src_u0_data.test00_0_src_v9_data[0][0] != ((ht_int8)0x00000000000000a0LL)) {
				HtAssert(0, 0);
			}
			if ((ht_int8)PR_test00_0_src_u0_data.test00_0_src_v9_data[0][1] != ((ht_int8)0x000000000000008bLL)) {
				HtAssert(0, 0);
			}
			if ((ht_int8)PR_test00_0_src_u0_data.test00_0_src_v9_data[0][2] != ((ht_int8)0x000000000000002bLL)) {
				HtAssert(0, 0);
			}
			if ((ht_int8)PR_test00_0_src_u0_data.test00_0_src_v9_data[1][0] != ((ht_int8)0x0000000000000067LL)) {
				HtAssert(0, 0);
			}
			if ((ht_int8)PR_test00_0_src_u0_data.test00_0_src_v9_data[1][1] != ((ht_int8)0x000000000000000eLL)) {
				HtAssert(0, 0);
			}
			if ((ht_int8)PR_test00_0_src_u0_data.test00_0_src_v9_data[1][2] != ((ht_int8)0x0000000000000034LL)) {
				HtAssert(0, 0);
			}
			if ((ht_int8)PR_test00_0_src_u0_data.test00_0_src_v9_data[2][0] != ((ht_int8)0x0000000000000009LL)) {
				HtAssert(0, 0);
			}
			if ((ht_int8)PR_test00_0_src_u0_data.test00_0_src_v9_data[2][1] != ((ht_int8)0x0000000000000000LL)) {
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
