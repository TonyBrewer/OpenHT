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
			S_test00_0_src_s0_data[1].test00_0_src_v0_data = ((uint8_t)0x0019dd5784fd41e0LL);
			HtContinue(TEST00_ST0);
			break;
		}
		case TEST00_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_uint8_t(PR_memAddr + 0, S_test00_0_src_s0_data[1].test00_0_src_v0_data);
			WriteMemPause(TEST00_LD0);
			break;
		}
		case TEST00_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_0_dst_v1_data(PR_memAddr + 0, 1, 0, 3);
			P_test00_0_dst_s0_data_RdAddr1 = (ht_uint5)29;
			P_test00_0_dst_s0_data_RdAddr2 = (ht_uint4)10;
			ReadMemPause(TEST00_CHK0);
			break;
		}
		case TEST00_CHK0: {
			if (GR_test00_0_dst_s0_data[1][0].test00_0_dst_s1_data.test00_0_dst_s2_data[3].test00_0_dst_v1_data != ((uint8_t)0x0019dd5784fd41e0LL)) {
				printf("\n");
				printf("EXP: 0x%02x\n", (uint8_t)0x0019dd5784fd41e0LL);
				printf("ACT: 0x%02x\n", GR_test00_0_dst_s0_data[1][0].test00_0_dst_s1_data.test00_0_dst_s2_data[3].test00_0_dst_v1_data);
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
