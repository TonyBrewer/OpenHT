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
			PW_test00_0_src_u0_data[0].write_addr(1, 1);
			PW_test00_0_src_u0_data[0].test00_0_src_u3_data.test00_0_src_v5_data[0] = ((int16_t)0x00030d46d8f2d5e0LL);
			PW_test00_0_src_u0_data[0].test00_0_src_u3_data.test00_0_src_v5_data[1] = ((int16_t)0x00030d46d8f2d5e0LL);
			HtContinue(TEST00_ST0);
			break;
		}
		case TEST00_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test00_0_src_u3_data(PR_memAddr + 0, 1, 1, 0, 1);
			WriteMemPause(TEST00_LD0);
			break;
		}
		case TEST00_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_0_src_u3_data(PR_memAddr + 0, 1, 1, 0, 1);
			P_test00_0_src_u0_data_RdAddr1 = (ht_uint4)1;
			P_test00_0_src_u0_data_RdAddr2 = (ht_uint2)1;
			ReadMemPause(TEST00_CHK);
			break;
		}
		case TEST00_CHK: {
			if ((int16_t)PR_test00_0_src_u0_data[0].test00_0_src_u3_data.test00_0_src_v5_data[0] != ((int16_t)0x00030d46d8f2d5e0LL)) {
				HtAssert(0, 0);
			}
			if ((int16_t)PR_test00_0_src_u0_data[0].test00_0_src_u3_data.test00_0_src_v5_data[1] != ((int16_t)0x00030d46d8f2d5e0LL)) {
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
