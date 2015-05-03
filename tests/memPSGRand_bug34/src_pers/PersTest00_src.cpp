#include "Ht.h"
#include "PersTest00.h"

void CPersTest00::PersTest00() {
	if (PR2_htValid) {
		switch (PR2_htInst) {
		case TEST00_ENTRY: {
			break;
		}
		case TEST00_WR: {
			break;
		}
		case TEST00_ST0: {
			break;
		}
		case TEST00_LD0: {
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
	if (PR5_htValid) {
		switch (PR5_htInst) {
		case TEST00_ENTRY: {
			HtContinue(TEST00_WR);
			break;
		}
		case TEST00_WR: {
			P5_test00_0_src_v0_data[1] = ((uint64_t)0x00077623f82d80e0LL);
			HtContinue(TEST00_ST0);
			break;
		}
		case TEST00_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_uint64_t(PR5_memAddr + 192, P5_test00_0_src_v0_data[1]);
			WriteMemPause(TEST00_LD0);
			break;
		}
		case TEST00_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_0_dst_v6_data(PR5_memAddr + 192, 7, 12, 0, 1, 0, 0, 1);
			P5_test00_0_dst_u0_data_RdAddr1 = (ht_uint4)7;
			P5_test00_0_dst_u0_data_RdAddr2 = (ht_uint4)12;
			ReadMemPause(TEST00_CHK);
			break;
		}
		case TEST00_CHK: {
			if ((uint64_t)GR5_test00_0_dst_u0_data[0].test00_0_dst_u1_data[1][0].test00_0_dst_v6_data[0] != ((uint64_t)0x00077623f82d80e0LL)) {
				HtAssert(0, (uint32_t)0x00030003);
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
