#include "Ht.h"
#include "PersTest00.h"

void CPersTest00::PersTest00() {
	if (PR1_htValid) {
		switch (PR1_htInst) {
		case TEST00_ENTRY: {
			HtContinue(TEST00_WR0);
			break;
		}
		case TEST00_WR0: {
			GW1_test00_0_src_v0_data[2].write_addr(26, 27);
			GW1_test00_0_src_v0_data[2] = ((int32_t)0x001464e300c7ca00LL);
			HtContinue(TEST00_ST0);
			break;
		}
		case TEST00_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test00_0_src_v0_data(PR1_memAddr + 0, 26, 27, 2, 1);
			WriteMemPause(TEST00_LD0);
			break;
		}
		case TEST00_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_0_dst_v1_data(PR1_memAddr + 0, 6, 0, 2, 0, 2);
			ReadMemPause(TEST00_CHK0);
			break;
		}
		case TEST00_CHK0: {
			if (GR1_test00_0_dst_u0_data[0][2].test00_0_dst_u1_data[0].test00_0_dst_v1_data[2][0] != ((int32_t)0x001464e300c7ca00LL)) {
				printf("EXP: 0x%08x\n", (int32_t)0x001464e300c7ca00LL);
				printf("ACT: 0x%08x\n", (int32_t)GR1_test00_0_dst_u0_data[0][2].test00_0_dst_u1_data[0].test00_0_dst_v1_data[2][0]);
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
		case TEST00_WR0: {
			break;
		}
		case TEST00_ST0: {
			break;
		}
		case TEST00_LD0: {
			P3_test00_0_dst_u0_data_RdAddr1 = (ht_uint1)0;
			P3_test00_0_dst_u0_data_RdAddr2 = (ht_uint4)6;
			break;
		}
		case TEST00_CHK0: {
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
