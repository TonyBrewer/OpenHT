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
			S_test00_0_s_0_data[2] = (uint8_t)0x001e32903d88ce60LL;
			P_test00_1_p_2_data.test00_1_p_5_data = (uint32_t)0x001544a26db92b60LL;
			HtContinue(TEST00_ST0);
			break;
		}
		case TEST00_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test00_0_s_0_data(PR_memAddr + 0);
			HtContinue(TEST00_ST1);
			break;
		}
		case TEST00_ST1: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_uint32_t(PR_memAddr + 32, PR_test00_1_p_2_data.test00_1_p_5_data);
			WriteMemPause(TEST00_LD0);
			break;
		}
		case TEST00_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_0_p_1_data(PR_memAddr + 0, 0, 1);
			HtContinue(TEST00_LD1);
			break;
		}
		case TEST00_LD1: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_1_s_7_data(PR_memAddr + 32);
			ReadMemPause(TEST00_CHK);
			break;
		}
		case TEST00_CHK: {
			if (PR_test00_0_p_1_data[0] != (uint8_t)0x001e32903d88ce60LL) {
				HtAssert(0, (uint32_t)0x00000000);
			}
			if (SR_test00_1_s_7_data[1] != (uint32_t)0x001544a26db92b60LL) {
				HtAssert(0, (uint32_t)0x00000001);
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
