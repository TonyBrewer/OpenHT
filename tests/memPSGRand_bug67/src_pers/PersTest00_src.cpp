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
			P3_test00_0_src_s0_data.test00_0_src_v0_data[1] = ((uint16_t)0x001290ca82d95ac0LL);
			PW3_test00_1_src_s0_data[1][3].write_addr(1);
			PW3_test00_1_src_s0_data[1][3].test00_1_src_v0_data = ((uint16_t)0x000fd6fd755e9e40LL);
			HtContinue(TEST00_ST0);
			break;
		}
		case TEST00_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test00_0_src_v0_data(PR3_memAddr + 0, 1, 1);
			HtContinue(TEST00_ST1);
			break;
		}
		case TEST00_ST1: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test00_1_src_v0_data(PR3_memAddr + 64, 1, 1, 3, 1);
			WriteMemPause(TEST00_LD0);
			break;
		}
		case TEST00_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_0_dst_v0_data(PR3_memAddr + 0, 0, 0, 1, 1);
			P3_test00_0_dst_v0_data_RdAddr1 = (ht_uint3)0;
			HtContinue(TEST00_LD1);
			break;
		}
		case TEST00_LD1: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_1_src_v1_data(PR3_memAddr + 64, 1, 1, 3, 1, 1);
			P3_test00_1_src_s0_data_RdAddr1 = (ht_uint3)1;
			ReadMemPause(TEST00_CHK);
			break;
		}
		case TEST00_CHK: {
			if ((uint16_t)PR3_test00_0_dst_v0_data[0][1] != ((uint16_t)0x001290ca82d95ac0LL)) {
				HtAssert(0, 0);
			}
			if ((uint16_t)PR3_test00_1_src_s0_data[1][3].test00_1_src_v1_data[1] != ((uint16_t)0x000fd6fd755e9e40LL)) {
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
