#include "Ht.h"
#include "PersTest00.h"

void CPersTest00::PersTest00() {
	if (PR_htValid) {
		switch (PR_htInst) {
		case TEST00_ENTRY: {
			HtContinue(TEST00_WR);
			break;
		}
		case TEST00_WR: {
			GW_test00_0_src_u0_data[2].write_addr(1);
			GW_test00_0_src_u0_data[2].test00_0_src_v4_data = ((uint32_t)0x0010d0513432a1c0LL);
			GW_test00_1_src_v0_data = ((uint8_t)0x00057c35945dad20LL);
			GW_test00_2_src_v0_data[0][1].write_addr(7, 4);
			GW_test00_2_src_v0_data[0][1] = ((int32_t)0x0004242d473aa9a0LL);
			S_test00_3_src_v0_data = ((ht_int7)0x0015a72e9631b220LL);
			P_test00_4_src_v0_data[3][1] = ((uint8_t)0x00018e55670822a0LL);
			HtContinue(TEST00_ST0);
			break;
		}
		case TEST00_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test00_0_src_v4_data(PR_memAddr + 0, 1, 2, 1);
			HtContinue(TEST00_ST1);
			break;
		}
		case TEST00_ST1: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_uint8_t(PR_memAddr + 64, GR_test00_1_src_v0_data);
			HtContinue(TEST00_ST2);
			break;
		}
		case TEST00_ST2: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test00_2_src_v0_data(PR_memAddr + 128, 7, 4, 0, 1, 1);
			HtContinue(TEST00_ST3);
			break;
		}
		case TEST00_ST3: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_ht_int7(PR_memAddr + 192, SR_test00_3_src_v0_data);
			HtContinue(TEST00_ST4);
			break;
		}
		case TEST00_ST4: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test00_4_src_v0_data(PR_memAddr + 256, 3, 1, 1);
			WriteMemPause(TEST00_LD0);
			break;
		}
		case TEST00_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_0_dst_v3_data(PR_memAddr + 0, 2, 4, 1);
			P_test00_0_dst_s0_data_RdAddr1 = (ht_uint2)2;
			P_test00_0_dst_s0_data_RdAddr2 = (ht_uint3)4;
			HtContinue(TEST00_LD1);
			break;
		}
		case TEST00_LD1: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_1_dst_v2_data(PR_memAddr + 64);
			HtContinue(TEST00_LD2);
			break;
		}
		case TEST00_LD2: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_2_dst_v0_data(PR_memAddr + 128);
			HtContinue(TEST00_LD3);
			break;
		}
		case TEST00_LD3: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_3_dst_v0_data(PR_memAddr + 192);
			HtContinue(TEST00_LD4);
			break;
		}
		case TEST00_LD4: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_4_dst_v3_data(PR_memAddr + 256, 0, 0, 1, 1, 2, 1);
			P_test00_4_dst_s0_data_RdAddr1 = (ht_uint1)0;
			ReadMemPause(TEST00_CHK);
			break;
		}
		case TEST00_CHK: {
			if ((uint32_t)PR_test00_0_dst_s0_data.test00_0_dst_v3_data[0][0] != ((uint32_t)0x0010d0513432a1c0LL)) {
				HtAssert(0, 0);
			}
			if ((uint8_t)GR_test00_1_dst_s0_data.test00_1_dst_v2_data != ((uint8_t)0x00057c35945dad20LL)) {
				HtAssert(0, 0);
			}
			if ((int32_t)GR_test00_2_dst_v0_data != ((int32_t)0x0004242d473aa9a0LL)) {
				HtAssert(0, 0);
			}
			if ((ht_int7)S_test00_3_dst_v0_data != ((ht_int7)0x0015a72e9631b220LL)) {
				HtAssert(0, 0);
			}
			if ((uint8_t)GR_test00_4_dst_s0_data[0][1].test00_4_dst_v3_data[1][2] != ((uint8_t)0x00018e55670822a0LL)) {
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
