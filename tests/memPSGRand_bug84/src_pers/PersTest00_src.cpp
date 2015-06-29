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
			P_test00_0_dst_s0_data[1].test00_0_dst_s2_data.test00_0_dst_v0_data[0][0] = ((uint64_t)0x000e45f5024727c0LL);
			P_test00_0_dst_s0_data[1].test00_0_dst_s2_data.test00_0_dst_v0_data[1][0] = ((uint64_t)0x000e45f5024727c0LL);
			P_test00_0_dst_s0_data[1].test00_0_dst_s2_data.test00_0_dst_v1_data = ((ht_int9)0x00063744d70dade0LL);
			HtContinue(TEST00_ST0);
			break;
		}
		case TEST00_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test00_0_dst_s2_data(PR_memAddr + 0, 1);
			WriteMemPause(TEST00_LD0);
			break;
		}
		case TEST00_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_0_dst_s2_data(PR_memAddr + 0, 1);
			ReadMemPause(TEST00_CHK0);
			break;
		}
		case TEST00_CHK0: {
			if ((uint64_t)PR_test00_0_dst_s0_data[1].test00_0_dst_s2_data.test00_0_dst_v0_data[0][0] != ((uint64_t)0x000e45f5024727c0LL)) {
				HtAssert(0, 0);
			}
			if ((uint64_t)PR_test00_0_dst_s0_data[1].test00_0_dst_s2_data.test00_0_dst_v0_data[1][0] != ((uint64_t)0x000e45f5024727c0LL)) {
				HtAssert(0, 0);
			}
			if (/*(ht_int9)*/P_test00_0_dst_s0_data[1].test00_0_dst_s2_data.test00_0_dst_v1_data != ((ht_int9)0x00063744d70dade0LL)) {
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
