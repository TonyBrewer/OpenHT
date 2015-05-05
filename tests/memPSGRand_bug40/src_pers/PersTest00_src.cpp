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
			GW2_test00_0_src_v0_data.write_addr(8, 14);
			GW2_test00_0_src_v0_data = ((uint32_t)0x0005bedda4786260LL);
			P2_test00_0_src_v0_data_RdAddr1 = (ht_uint4)8;
			P2_test00_0_src_v0_data_RdAddr2 = (ht_uint4)14;
			HtContinue(TEST00_ST0);
			break;
		}
		case TEST00_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_uint32_t(PR2_memAddr + 0, GR2_test00_0_src_v0_data);
			WriteMemPause(TEST00_LD0);
			break;
		}
		case TEST00_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_0_dst_v7_data(PR2_memAddr + 0, 0, 0, 1);
			P2_test00_0_dst_s0_data_RdAddr1 = (ht_uint2)0;
			ReadMemPause(TEST00_CHK);
			break;
		}
		case TEST00_CHK: {
			if ((uint32_t)PR2_test00_0_dst_s0_data[0].test00_0_dst_s1_data.test00_0_dst_v7_data != ((uint32_t)0x0005bedda4786260LL)) {
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
