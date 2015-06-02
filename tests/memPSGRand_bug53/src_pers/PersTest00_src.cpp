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
			PW_test00_0_dst_s0_data.test00_0_dst_u0_data[0][0].test00_0_dst_v3_data = ((uint64_t)0x000612cd9aa914c0LL);
			HtContinue(TEST00_ST0);
			break;
		}
		case TEST00_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test00_0_dst_u0_data_union(PR_memAddr + 0, PR_test00_0_dst_s0_data.test00_0_dst_u0_data[0][0]);
			WriteMemPause(TEST00_LD0);
			break;
		}
		case TEST00_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_0_dst_u0_data(PR_memAddr + 0, 0, 0, 1);
			ReadMemPause(TEST00_CHK);
			break;
		}
		case TEST00_CHK: {
			if ((uint64_t)PR_test00_0_dst_s0_data.test00_0_dst_u0_data[0][0].test00_0_dst_v3_data != ((uint64_t)0x000612cd9aa914c0LL)) {
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
