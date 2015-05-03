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
			GW_test00_0_dst_s0_data[1][0].write_addr(1);
			GW_test00_0_dst_s0_data[1][0].test00_0_dst_s1_data[0].test00_0_dst_s2_data.test00_0_dst_v0_data[0][0] = ((uint16_t)0x00130455e6465b40LL);
			GW_test00_0_dst_s0_data[1][0].test00_0_dst_s1_data[0].test00_0_dst_s2_data.test00_0_dst_v0_data[1][0] = ((uint16_t)0x00130455e6465b40LL);
			GW_test00_0_dst_s0_data[1][0].test00_0_dst_s1_data[0].test00_0_dst_s2_data.test00_0_dst_v0_data[0][1] = ((uint16_t)0x00130455e6465b40LL);
			GW_test00_0_dst_s0_data[1][0].test00_0_dst_s1_data[0].test00_0_dst_s2_data.test00_0_dst_v0_data[1][1] = ((uint16_t)0x00130455e6465b40LL);
			HtContinue(TEST00_ST0);
			break;
		}
		case TEST00_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test00_0_dst_s2_data(PR_memAddr + 64, 1, 1, 0, 0, 1);
			WriteMemPause(TEST00_LD0);
			break;
		}
		case TEST00_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_0_dst_s2_data(PR_memAddr + 64, 1, 1, 0, 0, 1);
			P_test00_0_dst_s0_data_RdAddr1 = (ht_uint4)1;
			ReadMemPause(TEST00_CHK);
			break;
		}
		case TEST00_CHK: {
			if ((uint16_t)GR_test00_0_dst_s0_data[1][0].test00_0_dst_s1_data[0].test00_0_dst_s2_data.test00_0_dst_v0_data[0][0] != ((uint16_t)0x00130455e6465b40LL)) {
				HtAssert(0, 0);
			}
			if ((uint16_t)GR_test00_0_dst_s0_data[1][0].test00_0_dst_s1_data[0].test00_0_dst_s2_data.test00_0_dst_v0_data[1][0] != ((uint16_t)0x00130455e6465b40LL)) {
				HtAssert(0, 0);
			}
			if ((uint16_t)GR_test00_0_dst_s0_data[1][0].test00_0_dst_s1_data[0].test00_0_dst_s2_data.test00_0_dst_v0_data[0][1] != ((uint16_t)0x00130455e6465b40LL)) {
				HtAssert(0, 0);
			}
			if ((uint16_t)GR_test00_0_dst_s0_data[1][0].test00_0_dst_s1_data[0].test00_0_dst_s2_data.test00_0_dst_v0_data[1][1] != ((uint16_t)0x00130455e6465b40LL)) {
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
