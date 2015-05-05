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
			S_test00_0_src_u0_data.write_addr(0, 10);
			S_test00_0_src_u0_data.write_mem().test00_0_src_u1_data[0][0].test00_0_src_v1_data = ((uint64_t)0x001169336de7cda0LL);
			S_test00_0_src_u0_data.write_mem().test00_0_src_u1_data[1][0].test00_0_src_v1_data = ((uint64_t)0x001169336de7cda0LL);
			S_test00_0_src_u0_data.write_mem().test00_0_src_u1_data[0][1].test00_0_src_v1_data = ((uint64_t)0x001169336de7cda0LL);
			S_test00_0_src_u0_data.write_mem().test00_0_src_u1_data[1][1].test00_0_src_v1_data = ((uint64_t)0x001169336de7cda0LL);
			S_test00_0_src_u0_data.write_mem().test00_0_src_u1_data[0][2].test00_0_src_v1_data = ((uint64_t)0x001169336de7cda0LL);
			S_test00_0_src_u0_data.write_mem().test00_0_src_u1_data[1][2].test00_0_src_v1_data = ((uint64_t)0x001169336de7cda0LL);
			S_test00_1_src_v0_data = ((uint16_t)0x0003834a4c86abe0LL);
			P_test00_2_src_u0_data[0].test00_2_src_u3_data.test00_2_src_v5_data = ((int16_t)0x0016951177fd7880LL);
			GW_test00_3_src_v0_data.write_addr(1);
			GW_test00_3_src_v0_data = ((int8_t)0x001e9fcdc4e5a160LL);
			GW_test00_4_src_u0_data[0][0].test00_4_src_u2_data.test00_4_src_v6_data[1][0] = ((uint8_t)0x0017cdc6cc1a0560LL);
			HtContinue(TEST00_ST0);
			break;
		}
		case TEST00_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			S_test00_0_src_u0_data.read_addr(0, 10);
			WriteMem_test00_0_src_u0_data_union(PR_memAddr + 0, SR_test00_0_src_u0_data.read_mem());
			HtContinue(TEST00_ST1);
			break;
		}
		case TEST00_ST1: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_uint16_t(PR_memAddr + 64, SR_test00_1_src_v0_data);
			HtContinue(TEST00_ST2);
			break;
		}
		case TEST00_ST2: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test00_2_src_v5_data(PR_memAddr + 128, 0);
			HtContinue(TEST00_ST3);
			break;
		}
		case TEST00_ST3: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test00_3_src_v0_data(PR_memAddr + 192, 1, 1);
			HtContinue(TEST00_ST4);
			break;
		}
		case TEST00_ST4: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test00_4_src_v6_data(PR_memAddr + 256, 0, 0, 1, 0, 1);
			WriteMemPause(TEST00_LD0);
			break;
		}
		case TEST00_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_0_src_u0_data(PR_memAddr + 0, 0, 10, 1);
			HtContinue(TEST00_LD1);
			break;
		}
		case TEST00_LD1: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_1_dst_v0_data(PR_memAddr + 64, 5, 0, 1, 1);
			P_test00_1_dst_v0_data_RdAddr1 = (ht_uint3)5;
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
			ReadMem_test00_4_dst_v0_data(PR_memAddr + 256, 1, 2, 1);
			P_test00_4_dst_v0_data_RdAddr1 = (ht_uint1)1;
			P_test00_4_dst_v0_data_RdAddr2 = (ht_uint2)2;
			ReadMemPause(TEST00_CHK);
			break;
		}
		case TEST00_CHK: {
			SR_test00_0_src_u0_data.read_addr(0, 10);
			if ((uint64_t)S_test00_0_src_u0_data.read_mem().test00_0_src_u1_data[0][0].test00_0_src_v1_data != ((uint64_t)0x001169336de7cda0LL)) {
				HtAssert(0, 0);
			}
			if ((uint64_t)S_test00_0_src_u0_data.read_mem().test00_0_src_u1_data[1][0].test00_0_src_v1_data != ((uint64_t)0x001169336de7cda0LL)) {
				HtAssert(0, 0);
			}
			if ((uint64_t)S_test00_0_src_u0_data.read_mem().test00_0_src_u1_data[0][1].test00_0_src_v1_data != ((uint64_t)0x001169336de7cda0LL)) {
				HtAssert(0, 0);
			}
			if ((uint64_t)S_test00_0_src_u0_data.read_mem().test00_0_src_u1_data[1][1].test00_0_src_v1_data != ((uint64_t)0x001169336de7cda0LL)) {
				HtAssert(0, 0);
			}
			if ((uint64_t)S_test00_0_src_u0_data.read_mem().test00_0_src_u1_data[0][2].test00_0_src_v1_data != ((uint64_t)0x001169336de7cda0LL)) {
				HtAssert(0, 0);
			}
			if ((uint64_t)S_test00_0_src_u0_data.read_mem().test00_0_src_u1_data[1][2].test00_0_src_v1_data != ((uint64_t)0x001169336de7cda0LL)) {
				HtAssert(0, 0);
			}
			if ((uint16_t)PR_test00_1_dst_v0_data[0][1] != ((uint16_t)0x0003834a4c86abe0LL)) {
				HtAssert(0, 0);
			}
			if ((int16_t)PR_test00_2_dst_v0_data[0] != ((int16_t)0x0016951177fd7880LL)) {
				HtAssert(0, 0);
			}
			if ((int8_t)GR_test00_3_dst_v0_data[0] != ((int8_t)0x001e9fcdc4e5a160LL)) {
				HtAssert(0, 0);
			}
			if ((uint8_t)GR_test00_4_dst_v0_data != ((uint8_t)0x0017cdc6cc1a0560LL)) {
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
