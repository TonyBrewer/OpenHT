#include "Ht.h"
#include "PersTest09.h"

void CPersTest09::PersTest09() {
	if (PR_htValid) {
		switch (PR_htInst) {
		case TEST09_ENTRY: {
			HtContinue(TEST09_WR);
			break;
		}
		case TEST09_WR: {
			GW_test09_0_src_u0_data.write_addr(2);
			GW_test09_0_src_u0_data.test09_0_src_s3_data.test09_0_src_v7_data[0] = ((int16_t)0x0010e64c6a067880LL);
			GW_test09_1_src_v0_data.write_addr(3);
			GW_test09_1_src_v0_data = ((int64_t)0x0013485b5ea66da0LL);
			GW_test09_2_src_v0_data[0].write_addr(0);
			GW_test09_2_src_v0_data[0] = ((uint64_t)0x000821fac2137b40LL);
			P_test09_0_src_u0_data_RdAddr1 = (ht_uint2)2;
			P_test09_2_src_v0_data_RdAddr1 = (ht_uint1)0;
			HtContinue(TEST09_ST0);
			break;
		}
		case TEST09_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_int16_t(PR_memAddr + 0, GR_test09_0_src_u0_data.test09_0_src_s3_data.test09_0_src_v7_data[0]);
			HtContinue(TEST09_ST1);
			break;
		}
		case TEST09_ST1: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test09_1_src_v0_data(PR_memAddr + 64, 3, 1);
			HtContinue(TEST09_ST2);
			break;
		}
		case TEST09_ST2: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_uint64_t(PR_memAddr + 128, GR_test09_2_src_v0_data[0]);
			WriteMemPause(TEST09_LD0);
			break;
		}
		case TEST09_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test09_0_dst_v0_data(PR_memAddr + 0);
			HtContinue(TEST09_LD1);
			break;
		}
		case TEST09_LD1: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test09_1_dst_v0_data(PR_memAddr + 64, 10, 14, 2, 1);
			HtContinue(TEST09_LD2);
			break;
		}
		case TEST09_LD2: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test09_2_dst_v1_data(PR_memAddr + 128);
			ReadMemPause(TEST09_CHK);
			break;
		}
		case TEST09_CHK: {
			if ((int16_t)P_test09_0_dst_v0_data != ((int16_t)0x0010e64c6a067880LL)) {
				HtAssert(0, 0);
			}
			S_test09_1_dst_v0_data[2][2].read_addr(10, 14);
			if ((int64_t)S_test09_1_dst_v0_data[2][2].read_mem() != ((int64_t)0x0013485b5ea66da0LL)) {
				HtAssert(0, 0);
			}
			if ((uint64_t)S_test09_2_dst_s0_data.test09_2_dst_v1_data != ((uint64_t)0x000821fac2137b40LL)) {
				HtAssert(0, 0);
			}
			HtContinue(TEST09_RTN);
			break;
		}
		case TEST09_RTN: {
			if (SendReturnBusy_test09()) {
				HtRetry();
				break;
			}
			SendReturn_test09();
			break;
		}
		default:
			assert(0);
		}
	}
}
