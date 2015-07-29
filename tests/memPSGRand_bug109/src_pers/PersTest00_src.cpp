#include "Ht.h"
#include "PersTest00.h"

void CPersTest00::PersTest00() {
	if (PR_htValid) {
		switch (PR_htInst) {
		case TEST00_ENTRY: {
			HtContinue(TEST00_WR0);
			break;
		}
		case TEST00_WR0: {
			PW_test00_0_src_u0_data.test00_0_src_v0_data[0][0] = ((uint8_t)0x00022ded1f316560LL);
			PW_test00_0_src_u0_data.test00_0_src_v0_data[0][1] = ((uint8_t)0x00022ded1f316560LL);
			PW_test00_0_src_u0_data.test00_0_src_v0_data[1][0] = ((uint8_t)0x00022ded1f316560LL);
			PW_test00_0_src_u0_data.test00_0_src_v0_data[1][1] = ((uint8_t)0x00022ded1f316560LL);
			PW_test00_0_src_u0_data.test00_0_src_v0_data[2][0] = ((uint8_t)0x00022ded1f316560LL);
			PW_test00_0_src_u0_data.test00_0_src_v0_data[2][1] = ((uint8_t)0x00022ded1f316560LL);
			PW_test00_0_src_u0_data.test00_0_src_v0_data[3][0] = ((uint8_t)0x00022ded1f316560LL);
			PW_test00_0_src_u0_data.test00_0_src_v0_data[3][1] = ((uint8_t)0x00022ded1f316560LL);
			HtContinue(TEST00_ST0);
			break;
		}
		case TEST00_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test00_0_src_u0_data(PR_memAddr + 0);
			WriteMemPause(TEST00_LD0);
			break;
		}
		case TEST00_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_0_src_u0_data(PR_memAddr + 0);
			ReadMemPause(TEST00_CHK0);
			break;
		}
		case TEST00_CHK0: {
			if (PR_test00_0_src_u0_data.test00_0_src_v0_data[0][0] != ((uint8_t)0x00022ded1f316560LL)) {
				HtAssert(0, 0);
			}
			if (PR_test00_0_src_u0_data.test00_0_src_v0_data[0][1] != ((uint8_t)0x00022ded1f316560LL)) {
				HtAssert(0, 0);
			}
			if (PR_test00_0_src_u0_data.test00_0_src_v0_data[1][0] != ((uint8_t)0x00022ded1f316560LL)) {
				HtAssert(0, 0);
			}
			if (PR_test00_0_src_u0_data.test00_0_src_v0_data[1][1] != ((uint8_t)0x00022ded1f316560LL)) {
				HtAssert(0, 0);
			}
			if (PR_test00_0_src_u0_data.test00_0_src_v0_data[2][0] != ((uint8_t)0x00022ded1f316560LL)) {
				HtAssert(0, 0);
			}
			if (PR_test00_0_src_u0_data.test00_0_src_v0_data[2][1] != ((uint8_t)0x00022ded1f316560LL)) {
				HtAssert(0, 0);
			}
			if (PR_test00_0_src_u0_data.test00_0_src_v0_data[3][0] != ((uint8_t)0x00022ded1f316560LL)) {
				printf("EXP: 0x%02x\n", (uint8_t)0x00022ded1f316560LL);
				printf("ACT: 0x%02x\n", (uint8_t)PR_test00_0_src_u0_data.test00_0_src_v0_data[3][0]);
				HtAssert(0, 0);
			}
			if (PR_test00_0_src_u0_data.test00_0_src_v0_data[3][1] != ((uint8_t)0x00022ded1f316560LL)) {
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
