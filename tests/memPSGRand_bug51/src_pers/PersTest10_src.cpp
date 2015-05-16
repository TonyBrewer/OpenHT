#include "Ht.h"
#include "PersTest10.h"

void CPersTest10::PersTest10() {
	if (PR2_htValid) {
		switch (PR2_htInst) {
		case TEST10_ENTRY: {
			HtContinue(TEST10_WR);
			break;
		}
		case TEST10_WR: {
			PW2_test10_0_dst_u0_data[0].write_addr(1, 5);
			PW2_test10_0_dst_u0_data[0].test10_0_dst_v1_data = ((uint64_t)0x00143aef0a600380LL);
			GW2_test10_1_src_v0_data[0][1] = ((int32_t)0x0006047795a2ade0LL);
			P2_test10_0_dst_u0_data_RdAddr1 = (ht_uint1)1;
			P2_test10_0_dst_u0_data_RdAddr2 = (ht_uint3)5;
			HtContinue(TEST10_ST0);
			break;
		}
		case TEST10_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test10_0_dst_u0_data_union(PR2_memAddr + 0, PR2_test10_0_dst_u0_data[0]);
			HtContinue(TEST10_ST1);
			break;
		}
		case TEST10_ST1: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test10_1_src_v0_data(PR2_memAddr + 64);
			WriteMemPause(TEST10_LD0);
			break;
		}
		case TEST10_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test10_0_dst_u0_data(PR2_memAddr + 0, 1, 5, 0, 1);
			P2_test10_0_dst_u0_data_RdAddr1 = (ht_uint1)1;
			P2_test10_0_dst_u0_data_RdAddr2 = (ht_uint3)5;
			HtContinue(TEST10_LD1);
			break;
		}
		case TEST10_LD1: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test10_1_dst_v0_data(PR2_memAddr + 64, 5, 0, 0, 1);
			P2_test10_1_dst_v0_data_RdAddr1 = (ht_uint3)5;
			ReadMemPause(TEST10_CHK);
			break;
		}
		case TEST10_CHK: {
			if ((uint64_t)PR2_test10_0_dst_u0_data[0].test10_0_dst_v1_data != ((uint64_t)0x00143aef0a600380LL)) {
				HtAssert(0, 0);
			}
			if ((int32_t)PR2_test10_1_dst_v0_data[0][0] != ((int32_t)0x0006047795a2ade0LL)) {
				HtAssert(0, 0);
			}
			HtContinue(TEST10_RTN);
			break;
		}
		case TEST10_RTN: {
			if (SendReturnBusy_test10()) {
				HtRetry();
				break;
			}
			SendReturn_test10();
			break;
		}
		default:
			assert(0);
		}
	}
}
