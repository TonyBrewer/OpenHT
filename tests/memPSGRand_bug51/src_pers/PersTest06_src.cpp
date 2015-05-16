#include "Ht.h"
#include "PersTest06.h"

void CPersTest06::PersTest06() {
	if (PR_htValid) {
		switch (PR_htInst) {
		case TEST06_ENTRY: {
			HtContinue(TEST06_WR);
			break;
		}
		case TEST06_WR: {
			S_test06_0_src_v0_data[1] = ((uint64_t)0x000c8f6ca5badde0LL);
			S_test06_1_src_v0_data = ((int64_t)0x0013e3099b42c600LL);
			S_test06_2_src_s0_data[1].write_addr(6);
			S_test06_2_src_s0_data[1].write_mem().test06_2_src_v0_data = ((uint64_t)0x001ac994d5140440LL);
			S_test06_3_dst_u0_data.write_addr(2, 1);
			S_test06_3_dst_u0_data.write_mem().test06_3_dst_v0_data = ((int32_t)0x0018f2d7daf46f80LL);
			PW_test06_4_src_v0_data[0].write_addr(3);
			PW_test06_4_src_v0_data[0] = ((uint8_t)0x000e7647aef729e0LL);
			P_test06_4_src_v0_data_RdAddr1 = (ht_uint2)3;
			HtContinue(TEST06_ST0);
			break;
		}
		case TEST06_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_uint64_t(PR_memAddr + 0, SR_test06_0_src_v0_data[1]);
			HtContinue(TEST06_ST1);
			break;
		}
		case TEST06_ST1: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_int64_t(PR_memAddr + 64, S_test06_1_src_v0_data);
			HtContinue(TEST06_ST2);
			break;
		}
		case TEST06_ST2: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test06_2_src_s0_data(PR_memAddr + 128, 6, 1);
			HtContinue(TEST06_ST3);
			break;
		}
		case TEST06_ST3: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			S_test06_3_dst_u0_data.read_addr(2, 1);
			WriteMem_test06_3_dst_u0_data_union(PR_memAddr + 192, SR_test06_3_dst_u0_data.read_mem());
			HtContinue(TEST06_ST4);
			break;
		}
		case TEST06_ST4: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_uint8_t(PR_memAddr + 256, PR_test06_4_src_v0_data[0]);
			WriteMemPause(TEST06_LD0);
			break;
		}
		case TEST06_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test06_0_src_v0_data(PR_memAddr + 0, 1, 1);
			HtContinue(TEST06_LD1);
			break;
		}
		case TEST06_LD1: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test06_1_dst_v0_data(PR_memAddr + 64, 2, 1);
			P_test06_1_dst_v0_data_RdAddr1 = (ht_uint2)2;
			HtContinue(TEST06_LD2);
			break;
		}
		case TEST06_LD2: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test06_2_src_s0_data(PR_memAddr + 128, 6, 1);
			HtContinue(TEST06_LD3);
			break;
		}
		case TEST06_LD3: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test06_3_dst_u0_data(PR_memAddr + 192, 2, 1, 1);
			HtContinue(TEST06_LD4);
			break;
		}
		case TEST06_LD4: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test06_4_dst_v0_data(PR_memAddr + 256);
			ReadMemPause(TEST06_CHK);
			break;
		}
		case TEST06_CHK: {
			if ((uint64_t)S_test06_0_src_v0_data[1] != ((uint64_t)0x000c8f6ca5badde0LL)) {
				HtAssert(0, 0);
			}
			if ((int64_t)PR_test06_1_dst_v0_data != ((int64_t)0x0013e3099b42c600LL)) {
				HtAssert(0, 0);
			}
			S_test06_2_src_s0_data[1].read_addr(6);
			if ((uint64_t)SR_test06_2_src_s0_data[1].read_mem().test06_2_src_v0_data != ((uint64_t)0x001ac994d5140440LL)) {
				HtAssert(0, 0);
			}
			SR_test06_3_dst_u0_data.read_addr(2, 1);
			if ((int32_t)SR_test06_3_dst_u0_data.read_mem().test06_3_dst_v0_data != ((int32_t)0x0018f2d7daf46f80LL)) {
				HtAssert(0, 0);
			}
			if ((uint8_t)PR_test06_4_dst_v0_data != ((uint8_t)0x000e7647aef729e0LL)) {
				HtAssert(0, 0);
			}
			HtContinue(TEST06_RTN);
			break;
		}
		case TEST06_RTN: {
			if (SendReturnBusy_test06()) {
				HtRetry();
				break;
			}
			SendReturn_test06();
			break;
		}
		default:
			assert(0);
		}
	}
}
