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
			PW_test00_0_src_v0_data[0].write_addr(4, 7);
			PW_test00_0_src_v0_data[0] = ((int16_t)0x000e9de7ebf4aec0LL);
			S_test00_1_dst_s0_data.write_addr(3, 3);
			S_test00_1_dst_s0_data.write_mem().test00_1_dst_u0_data.test00_1_dst_u2_data.test00_1_dst_v6_data = ((int16_t)0x000cfb2347be8920LL);
			HtContinue(TEST00_ST0);
			break;
		}
		case TEST00_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test00_0_src_v0_data(PR_memAddr + 0, 4, 7, 0, 1);
			HtContinue(TEST00_ST1);
			break;
		}
		case TEST00_ST1: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test00_1_dst_u2_data(PR_memAddr + 64, 3, 3, 1);
			WriteMemPause(TEST00_LD0);
			break;
		}
		case TEST00_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_0_dst_v3_data(PR_memAddr + 0, 0, 1, 1);
			HtContinue(TEST00_LD1);
			break;
		}
		case TEST00_LD1: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_1_dst_u2_data(PR_memAddr + 64, 3, 3, 1);
			ReadMemPause(TEST00_CHK);
			break;
		}
		case TEST00_CHK: {
			if ((int16_t)PR_test00_0_dst_u0_data[0].test00_0_dst_v3_data[1] != ((int16_t)0x000e9de7ebf4aec0LL)) {
				HtAssert(0, 0);
			}
			SR_test00_1_dst_s0_data.read_addr(3, 3);
			printf("EXP: 0x%04x\n", (int16_t)0x000cfb2347be8920LL);
			printf("ACT: 0x%04x\n", (int16_t)S_test00_1_dst_s0_data.read_mem().test00_1_dst_u0_data.test00_1_dst_u2_data.test00_1_dst_v6_data);
			if ((int16_t)S_test00_1_dst_s0_data.read_mem().test00_1_dst_u0_data.test00_1_dst_u2_data.test00_1_dst_v6_data != ((int16_t)0x000cfb2347be8920LL)) {
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
