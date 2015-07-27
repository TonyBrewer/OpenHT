#include "Ht.h"
#include "PersTest00.h"

void CPersTest00::PersTest00() {
	if (PR2_htValid) {
		switch (PR2_htInst) {
		case TEST00_ENTRY: {
			HtContinue(TEST00_WR0);
			break;
		}
		case TEST00_WR0: {
			PW2_test00_0_src_u0_data.write_addr(7, 1);
			PW2_test00_0_src_u0_data.test00_0_src_v2_data[0] = ((ht_int64)0x001009ebfa9d5960LL);
			PW2_test00_0_src_u0_data.test00_0_src_v1_data = ((int64_t)0x001f03aaf0a824c0LL);
			HtContinue(TEST00_ST0);
			break;
		}
		case TEST00_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test00_0_src_u0_data(PR2_memAddr + 0);
			WriteMemPause(TEST00_LD0);
			break;
		}
		case TEST00_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_0_src_u0_data(PR2_memAddr + 0);
			P2_test00_0_src_u0_data_RdAddr1 = (ht_uint3)4;
			P2_test00_0_src_u0_data_RdAddr2 = (ht_uint2)1;
			ReadMemPause(TEST00_CHK0);
			break;
		}
		case TEST00_CHK0: {
			if (PR2_test00_0_src_u0_data.test00_0_src_v0_data != ((ht_int32)0x00000000fa9d5960LL)) {
				HtAssert(0, 0);
			}
			if (PR2_test00_0_src_u0_data.test00_0_src_v1_data != ((ht_int64)0x001f03aaf0a824c0LL)) {
				printf("EXP - 0x%08x\n", (int32_t)0x00000000001f03aaLL);
				printf("ACT - 0x%08x\n", (int32_t)PR2_test00_0_src_u0_data.test00_0_src_v1_data);
				HtAssert(0, 0);
			}
			if (PR2_test00_0_src_u0_data.test00_0_src_v2_data[0] != ((ht_int64)0x001009ebfa9d5960LL)) {
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
