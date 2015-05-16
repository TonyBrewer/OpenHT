#include "Ht.h"
#include "PersTest11.h"

void CPersTest11::PersTest11() {
	if (PR_htValid) {
		switch (PR_htInst) {
		case TEST11_ENTRY: {
			HtContinue(TEST11_WR);
			break;
		}
		case TEST11_WR: {
			S_test11_0_src_v0_data = ((uint32_t)0x001ee2e1ac669d40LL);
			PW_test11_1_src_v0_data.write_addr(7, 2);
			PW_test11_1_src_v0_data = ((int8_t)0x000a5747e8d17d60LL);
			P_test11_1_src_v0_data_RdAddr1 = (ht_uint3)7;
			P_test11_1_src_v0_data_RdAddr2 = (ht_uint2)2;
			HtContinue(TEST11_ST0);
			break;
		}
		case TEST11_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test11_0_src_v0_data(PR_memAddr + 0);
			HtContinue(TEST11_ST1);
			break;
		}
		case TEST11_ST1: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_int8_t(PR_memAddr + 64, PR_test11_1_src_v0_data);
			WriteMemPause(TEST11_LD0);
			break;
		}
		case TEST11_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test11_0_dst_v0_data(PR_memAddr + 0);
			HtContinue(TEST11_LD1);
			break;
		}
		case TEST11_LD1: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test11_1_src_v0_data(PR_memAddr + 64, 7, 2, 1);
			P_test11_1_src_v0_data_RdAddr1 = (ht_uint3)7;
			P_test11_1_src_v0_data_RdAddr2 = (ht_uint2)2;
			ReadMemPause(TEST11_CHK);
			break;
		}
		case TEST11_CHK: {
			if ((uint32_t)PR_test11_0_dst_v0_data[0] != ((uint32_t)0x001ee2e1ac669d40LL)) {
				HtAssert(0, 0);
			}
			if ((int8_t)PR_test11_1_src_v0_data != ((int8_t)0x000a5747e8d17d60LL)) {
				HtAssert(0, 0);
			}
			HtContinue(TEST11_RTN);
			break;
		}
		case TEST11_RTN: {
			if (SendReturnBusy_test11()) {
				HtRetry();
				break;
			}
			SendReturn_test11();
			break;
		}
		default:
			assert(0);
		}
	}
}
