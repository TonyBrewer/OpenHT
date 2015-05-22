#include "Ht.h"
#include "PersTest00.h"

void CPersTest00::PersTest00() {
	if (PR1_htValid) {
		switch (PR1_htInst) {
		case TEST00_ENTRY: {
			break;
		}
		case TEST00_WR: {
			break;
		}
		case TEST00_ST0: {
			break;
		}
		case TEST00_ST1: {
			break;
		}
		case TEST00_LD0: {
			break;
		}
		case TEST00_LD1: {
			break;
		}
		case TEST00_CHK: {
			if ((int32_t)GR1_test00_0_dst_u0_data.test00_0_dst_v1_data[3] != ((int32_t)0x0012ab67f08e8dc0LL)) {
				HtAssert(0, 0);
			}
			break;
		}
		case TEST00_RTN: {
			break;
		}
		default:
			assert(0);
		}
	}
	if (PR2_htValid) {
		switch (PR2_htInst) {
		case TEST00_ENTRY: {
			HtContinue(TEST00_WR);
			break;
		}
		case TEST00_WR: {
			P2_test00_0_src_v0_data[4][0] = ((int32_t)0x0012ab67f08e8dc0LL);
			GW2_test00_1_src_v0_data.write_addr(1, 2);
			GW2_test00_1_src_v0_data = ((uint64_t)0x00135b2e8db75580LL);
			HtContinue(TEST00_ST0);
			break;
		}
		case TEST00_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test00_0_src_v0_data(PR2_memAddr + 0, 4, 0, 1);
			HtContinue(TEST00_ST1);
			break;
		}
		case TEST00_ST1: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test00_1_src_v0_data(PR2_memAddr + 64, 1, 2, 1);
			WriteMemPause(TEST00_LD0);
			break;
		}
		case TEST00_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_0_dst_v1_data(PR2_memAddr + 0);
			HtContinue(TEST00_LD1);
			break;
		}
		case TEST00_LD1: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_1_src_v0_data(PR2_memAddr + 64, 1, 2, 1);
			P2_test00_1_src_v0_data_RdAddr1 = (ht_uint1)1;
			P2_test00_1_src_v0_data_RdAddr2 = (ht_uint3)2;
			ReadMemPause(TEST00_CHK);
			break;
		}
		case TEST00_CHK: {
			if ((uint64_t)GR2_test00_1_src_v0_data != ((uint64_t)0x00135b2e8db75580LL)) {
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
