#include "Ht.h"
#include "PersTest00.h"

void CPersTest00::PersTest00() {
	if (PR_htValid) {
		switch (PR_htInst) {
		case TEST00_ENTRY: {
			HtContinue(TEST00_WR0);
			break;
		}
		case TEST00_WR0: {
			GW_test00_0_src_u0_data.write_addr(2);
			GW_test00_0_src_u0_data.test00_0_src_v1_data[0][1] = ((int8_t)0x001d87ae8ceb2ce0LL);
			S_test00_1_src_v0_data[2].write_addr(1, 9);
			S_test00_1_src_v0_data[2].write_mem((ht_uint26)0x000b8798301c4c60LL);
			GW_test00_2_src_s0_data.write_addr(2);
			GW_test00_2_src_s0_data.test00_2_src_u0_data[1][0].test00_2_src_u1_data[2].test00_2_src_v4_data[0][0] = ((int16_t)0x001d433ebe366d80LL);
			GW_test00_3_src_v0_data[1].write_addr(3, 6);
			GW_test00_3_src_v0_data[1] = ((uint16_t)0x00074940f68a6000LL);
			PW_test00_4_src_v0_data[0].write_addr(0, 8);
			PW_test00_4_src_v0_data[0] = ((ht_int40)0x001d23c42ed9b120LL);
			P_test00_2_src_s0_data_RdAddr1 = (ht_uint2)2;
			HtContinue(TEST00_ST0);
			break;
		}
		case TEST00_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test00_0_src_v1_data(PR_memAddr + 0);
			HtContinue(TEST00_ST1);
			break;
		}
		case TEST00_ST1: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test00_1_src_v0_data(PR_memAddr + 64, 1, 9);
			HtContinue(TEST00_ST2);
			break;
		}
		case TEST00_ST2: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_int16_t(PR_memAddr + 128, GR_test00_2_src_s0_data.test00_2_src_u0_data[1][0].test00_2_src_u1_data[2].test00_2_src_v4_data[0][0]);
			HtContinue(TEST00_ST3);
			break;
		}
		case TEST00_ST3: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test00_3_src_v0_data(PR_memAddr + 192, 6);
			HtContinue(TEST00_ST4);
			break;
		}
		case TEST00_ST4: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test00_4_src_v0_data(PR_memAddr + 256, 0, 0);
			WriteMemPause(TEST00_LD0);
			break;
		}
		case TEST00_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_0_dst_v2_data(PR_memAddr + 0, 3, 0, 0, 2, 1);
			P_test00_0_dst_u0_data_RdAddr1 = (ht_uint3)3;
			P_test00_0_dst_u0_data_RdAddr2 = (ht_uint1)0;
			HtContinue(TEST00_LD1);
			break;
		}
		case TEST00_LD1: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_1_dst_v0_data(PR_memAddr + 64, 1, 13, 1);
			P_test00_1_dst_v0_data_RdAddr1 = (ht_uint5)1;
			P_test00_1_dst_v0_data_RdAddr2 = (ht_uint4)13;
			HtContinue(TEST00_LD2);
			break;
		}
		case TEST00_LD2: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_2_dst_v0_data(PR_memAddr + 128, 0, 0, 1);
			HtContinue(TEST00_LD3);
			break;
		}
		case TEST00_LD3: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_3_dst_v10_data(PR_memAddr + 192, 22, 1, 1);
			P_test00_3_dst_s0_data_RdAddr1 = (ht_uint5)22;
			HtContinue(TEST00_LD4);
			break;
		}
		case TEST00_LD4: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_4_dst_v0_data(PR_memAddr + 256);
			ReadMemPause(TEST00_CHK0);
			break;
		}
		case TEST00_CHK0: {
			if (GR_test00_0_dst_u0_data[0][2].test00_0_dst_v2_data[1] != ((int8_t)0x001d87ae8ceb2ce0LL)) {
				HtAssert(0, 0);
			}
			if (GR_test00_1_dst_v0_data != ((ht_uint26)0x000b8798301c4c60LL)) {
				HtAssert(0, 0);
			}
			if (SR_test00_2_dst_v0_data[0][0] != ((int16_t)0x001d433ebe366d80LL)) {
				HtAssert(0, 0);
			}
			if (GR_test00_3_dst_s0_data[1].test00_3_dst_u1_data.test00_3_dst_v10_data[1] != ((uint16_t)0x00074940f68a6000LL)) {
				HtAssert(0, 0);
			}
			SR_test00_4_dst_v0_data[3].read_addr(11);
			if (S_test00_4_dst_v0_data[3].read_mem() != ((ht_int40)0x001d23c42ed9b120LL)) {
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
