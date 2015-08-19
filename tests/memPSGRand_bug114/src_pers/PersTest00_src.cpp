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
			P_test00_0_src_u0_data.test00_0_src_v1_data[0][1] = ((uint16_t)0x001b7a1e970896e0LL);
			HtContinue(TEST00_ST0);
			break;
		}
		case TEST00_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test00_0_src_v1_data(PR_memAddr + 0, 0, 1, 1);
			WriteMemPause(TEST00_LD0);
			break;
		}
		case TEST00_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_0_dst_v0_data(PR_memAddr + 0, 0, 3);
			P_test00_0_dst_v0_data_RdAddr1 = (ht_uint1)0;
			ReadMemPause(TEST00_CHK0);
			break;
		}
		case TEST00_CHK0: {
			if (GR_test00_0_dst_v0_data[3][3] != ((uint16_t)0x001b7a1e970896e0LL)) {
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
