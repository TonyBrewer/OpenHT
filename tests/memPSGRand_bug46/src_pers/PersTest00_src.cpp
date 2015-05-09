#include "Ht.h"
#include "PersTest00.h"

void CPersTest00::PersTest00() {
	if (PR3_htValid) {
		switch (PR3_htInst) {
		case TEST00_ENTRY: {
			HtContinue(TEST00_WR);
			break;
		}
		case TEST00_WR: {
			GW3_test00_0_src_v0_data.write_addr(1, 9);
			GW3_test00_0_src_v0_data = ((uint64_t)0x000beba9f75cc5c0LL);
			P3_test00_0_src_v0_data_RdAddr1 = (ht_uint1)1;
			P3_test00_0_src_v0_data_RdAddr2 = (ht_uint4)9;
			HtContinue(TEST00_ST0);
			break;
		}
		case TEST00_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_uint64_t(PR3_memAddr + 0, GR3_test00_0_src_v0_data);
			WriteMemPause(TEST00_LD0);
			break;
		}
		case TEST00_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_0_dst_v0_data(PR3_memAddr + 0, 1, 1, 1, 2, 1, 1);
			P3_test00_0_dst_s0_data_RdAddr1 = (ht_uint1)1;
			P3_test00_0_dst_s0_data_RdAddr2 = (ht_uint1)1;
			ReadMemPause(TEST00_CHK);
			break;
		}
		case TEST00_CHK: {
			if ((uint64_t)PR3_test00_0_dst_s0_data[1].test00_0_dst_v0_data[2][1] != ((uint64_t)0x000beba9f75cc5c0LL)) {
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
