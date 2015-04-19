#include "Ht.h"
#include "PersTest03.h"

void CPersTest03::PersTest03() {
	if (PR_htValid) {
		switch (PR_htInst) {
		case TEST03_ENTRY: {
			HtContinue(TEST03_WR);
			break;
		}
		case TEST03_WR: {
			P_test03_0_0_data = (uint32_t)0x000d46ec2a186de0LL;
			GW_test03_1_2_data.test03_1_5_data = (uint16_t)0x0007085429ba5040LL;
			P_test03_2_10_data.test03_2_10_data[2] = (uint16_t)0x000950a69cccfe20LL;
			HtContinue(TEST03_ST0);
			break;
		}
		case TEST03_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_uint32_t(PR_memAddr + 0, P_test03_0_0_data);
			HtContinue(TEST03_ST1);
			break;
		}
		case TEST03_ST1: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test03_1_5_data(PR_memAddr + 32);
			HtContinue(TEST03_ST2);
			break;
		}
		case TEST03_ST2: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test03_2_10_data(PR_memAddr + 64);
			WriteMemPause(TEST03_LD0);
			break;
		}
		case TEST03_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test03_0_1_data(PR_memAddr + 0, 1, 1);
			P_test03_0_1_data_RdAddr = (ht_uint1)1;
			HtContinue(TEST03_LD1);
			break;
		}
		case TEST03_LD1: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test03_1_9_data(PR_memAddr + 32, 4, 1);
			P_test03_1_9_data_RdAddr = (ht_uint3)4;
			HtContinue(TEST03_LD2);
			break;
		}
		case TEST03_LD2: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test03_2_10_data(PR_memAddr + 64);
			ReadMemPause(TEST03_CHK);
			break;
		}
		case TEST03_CHK: {
			if ((uint32_t)PR_test03_0_1_data != (uint32_t)0x000d46ec2a186de0LL) {
				HtAssert(0, (uint32_t)0x00030000);
			}
			if ((uint16_t)PR_test03_1_9_data != (uint16_t)0x0007085429ba5040LL) {
				HtAssert(0, (uint32_t)0x00030001);
			}
			if ((uint16_t)PR_test03_2_10_data.test03_2_10_data[2] != (uint16_t)0x000950a69cccfe20LL) {
				HtAssert(0, (uint32_t)0x00030002);
			}
			HtContinue(TEST03_RTN);
			break;
		}
		case TEST03_RTN: {
			if (SendReturnBusy_test03()) {
				HtRetry();
				break;
			}
			SendReturn_test03();
			break;
		}
		default:
			assert(0);
		}
	}
}
