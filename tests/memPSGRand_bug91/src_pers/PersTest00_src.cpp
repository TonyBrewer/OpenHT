#include "Ht.h"
#include "PersTest00.h"

void CPersTest00::PersTest00() {
	if (PR3_htValid) {
		switch (PR3_htInst) {
		case TEST00_ENTRY: {
			HtContinue(TEST00_WR0);
			break;
		}
		case TEST00_WR0: {
			GW3_test00_0_src_v0_data[1][0].write_addr(12, 1);
			GW3_test00_0_src_v0_data[1][0] = ((int64_t)0x001f51cea6e7ca00LL);
			HtContinue(TEST00_ST0);
			break;
		}
		case TEST00_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test00_0_src_v0_data(PR3_memAddr + 0, 1, 0, 1);
			WriteMemPause(TEST00_LD0);
			break;
		}
		case TEST00_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_0_dst_v2_data(PR3_memAddr + 0, 11, 1);
			P3_test00_0_dst_u0_data_RdAddr1 = (ht_uint5)11;
			P3_test00_0_dst_u0_data_RdAddr2 = (ht_uint3)3;
			ReadMemPause(TEST00_CHK0);
			break;
		}
		case TEST00_CHK0: {
			if (PR3_test00_0_dst_u0_data.test00_0_dst_v2_data != ((int64_t)0x001f51cea6e7ca00LL)) {
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
