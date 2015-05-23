#include "Ht.h"
#include "PersTest00.h"

void CPersTest00::PersTest00() {
	if (PR2_htValid) {
		switch (PR2_htInst) {
		case TEST00_ENTRY: {
			HtContinue(TEST00_WR);
			break;
		}
		case TEST00_WR: {
			GW2_test00_0_src_s0_data.write_addr(12, 6);
			GW2_test00_0_src_s0_data.test00_0_src_v9_data = ((int16_t)0x0018d928e5a82900LL);
			HtContinue(TEST00_ST0);
			break;
		}
		case TEST00_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_int16_t(PR2_memAddr + 0, GR2_test00_0_src_s0_data.test00_0_src_v9_data);
			WriteMemPause(TEST00_LD0);
			break;
		}
		case TEST00_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_0_src_v2_data(PR2_memAddr + 0, 12, 6, 0, 0, 1);
			ReadMemPause(TEST00_CHK);
			break;
		}
		case TEST00_CHK: {
			if ((int16_t)GR2_test00_0_src_s0_data.test00_0_src_s1_data.test00_0_src_v2_data[0][0] != ((int16_t)0x0018d928e5a82900LL)) {
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
	if (PR3_htValid) {
		switch (PR3_htInst) {
		case TEST00_ENTRY: {
			break;
		}
		case TEST00_WR: {
			P3_test00_0_src_s0_data_RdAddr1 = (ht_uint5)12;
			P3_test00_0_src_s0_data_RdAddr2 = (ht_uint3)6;
			break;
		}
		case TEST00_ST0: {
			break;
		}
		case TEST00_LD0: {
			P3_test00_0_src_s0_data_RdAddr1 = (ht_uint5)12;
			P3_test00_0_src_s0_data_RdAddr2 = (ht_uint3)6;
			break;
		}
		case TEST00_CHK: {
			break;
		}
		case TEST00_RTN: {
			break;
		}
		default:
			assert(0);
		}
	}
}
