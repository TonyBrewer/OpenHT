#include "Ht.h"
#include "PersTest12.h"

void CPersTest12::PersTest12() {
	if (PR3_htValid) {
		switch (PR3_htInst) {
		case TEST12_ENTRY: {
			HtContinue(TEST12_WR);
			break;
		}
		case TEST12_WR: {
			S_test12_0_src_v0_data[0].write_addr(6, 6);
			S_test12_0_src_v0_data[0].write_mem((int8_t)0x00045a84446a1dc0LL);
			S_test12_1_dst_s0_data.write_addr(5, 4);
			S_test12_1_dst_s0_data.write_mem().test12_1_dst_s1_data[0].test12_1_dst_s2_data.test12_1_dst_v3_data = ((uint32_t)0x000e13bf963abd40LL);
			S_test12_1_dst_s0_data.write_mem().test12_1_dst_s1_data[0].test12_1_dst_s2_data.test12_1_dst_v4_data = ((int16_t)0x00124daa9eda1d20LL);
			GW3_test12_2_src_u0_data[0].test12_2_src_u1_data.test12_2_src_v1_data = ((uint64_t)0x0003f9826575e0c0LL);
			HtContinue(TEST12_ST0);
			break;
		}
		case TEST12_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test12_0_src_v0_data(PR3_memAddr + 0, 6, 6, 1);
			HtContinue(TEST12_ST1);
			break;
		}
		case TEST12_ST1: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			S_test12_1_dst_s0_data.read_addr(5, 4);
			WriteMem_test12_1_dst_s2_data_struct(PR3_memAddr + 64, S_test12_1_dst_s0_data.read_mem().test12_1_dst_s1_data[0].test12_1_dst_s2_data);
			HtContinue(TEST12_ST2);
			break;
		}
		case TEST12_ST2: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_uint64_t(PR3_memAddr + 128, GR3_test12_2_src_u0_data[0].test12_2_src_u1_data.test12_2_src_v1_data);
			WriteMemPause(TEST12_LD0);
			break;
		}
		case TEST12_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test12_0_dst_v0_data(PR3_memAddr + 0, 0, 1);
			HtContinue(TEST12_LD1);
			break;
		}
		case TEST12_LD1: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test12_1_dst_s2_data(PR3_memAddr + 64, 5, 4, 0, 1);
			HtContinue(TEST12_LD2);
			break;
		}
		case TEST12_LD2: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test12_2_dst_v0_data(PR3_memAddr + 128, 3, 0, 1);
			ReadMemPause(TEST12_CHK);
			break;
		}
		case TEST12_CHK: {
			if ((int8_t)GR3_test12_0_dst_v0_data[0] != ((int8_t)0x00045a84446a1dc0LL)) {
				HtAssert(0, 0);
			}
			SR_test12_1_dst_s0_data.read_addr(5, 4);
			printf("EXP (v3): 0x%08x\n", (uint32_t)0x000e13bf963abd40LL);
			printf("ACT (v3): 0x%08x\n", (uint32_t)SR_test12_1_dst_s0_data.read_mem().test12_1_dst_s1_data[0].test12_1_dst_s2_data.test12_1_dst_v3_data);
			if ((uint32_t)SR_test12_1_dst_s0_data.read_mem().test12_1_dst_s1_data[0].test12_1_dst_s2_data.test12_1_dst_v3_data != ((uint32_t)0x000e13bf963abd40LL)) {
				HtAssert(0, 0);
			}
			printf("EXP (v4): 0x%04x\n", (int16_t)0x00124daa9eda1d20LL);
			printf("ACT (v4): 0x%04x\n", (int16_t)S_test12_1_dst_s0_data.read_mem().test12_1_dst_s1_data[0].test12_1_dst_s2_data.test12_1_dst_v4_data);
			if ((int16_t)S_test12_1_dst_s0_data.read_mem().test12_1_dst_s1_data[0].test12_1_dst_s2_data.test12_1_dst_v4_data != ((int16_t)0x00124daa9eda1d20LL)) {
				HtAssert(0, 0);
			}
			S_test12_2_dst_v0_data[0].read_addr(3);
			if ((uint64_t)SR_test12_2_dst_v0_data[0].read_mem() != ((uint64_t)0x0003f9826575e0c0LL)) {
				HtAssert(0, 0);
			}
			HtContinue(TEST12_RTN);
			break;
		}
		case TEST12_RTN: {
			if (SendReturnBusy_test12()) {
				HtRetry();
				break;
			}
			SendReturn_test12();
			break;
		}
		default:
			assert(0);
		}
	}
}
