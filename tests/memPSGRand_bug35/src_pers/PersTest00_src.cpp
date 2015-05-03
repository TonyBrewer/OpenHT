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
			PW_test00_0_src_u0_data.write_addr(1);
			PW_test00_0_src_u0_data.test00_0_src_v0_data[0] = ((int8_t)0x001e17270f92a5a0LL);
			P_test00_0_src_u0_data_RdAddr1 = (ht_uint1)1;
			HtContinue(TEST00_ST0);
			break;
		}
		case TEST00_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_int8_t(PR_memAddr + 0, PR_test00_0_src_u0_data.test00_0_src_v0_data[0]);
			WriteMemPause(TEST00_LD0);
			break;
		}
		case TEST00_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_0_dst_v0_data(PR_memAddr + 0, 8, 11, 1, 1);
			P_test00_0_dst_u0_data_RdAddr1 = (ht_uint4)8;
			P_test00_0_dst_u0_data_RdAddr2 = (ht_uint4)11;
			ReadMemPause(TEST00_CHK);
			break;
		}
		case TEST00_CHK: {
			if ((int8_t)GR_test00_0_dst_u0_data[1].test00_0_dst_v0_data != ((int8_t)0x001e17270f92a5a0LL)) {
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
