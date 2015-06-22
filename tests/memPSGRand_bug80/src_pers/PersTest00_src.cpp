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
			S_test00_0_dst_s0_data.test00_0_dst_v0_data = ((ht_int62)0x0001fcabad0714c0LL);
			S_test00_0_dst_s0_data.test00_0_dst_v1_data = ((ht_int12)0x001b711eefa6b380LL);
			HtContinue(TEST00_ST0);
			break;
		}
		case TEST00_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test00_0_dst_s0_data_struct(PR_memAddr + 0, S_test00_0_dst_s0_data);
			WriteMemPause(TEST00_LD0);
			break;
		}
		case TEST00_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_0_dst_s0_data(PR_memAddr + 0);
			ReadMemPause(TEST00_CHK0);
			break;
		}
		case TEST00_CHK0: {
			if ((ht_int62)SR_test00_0_dst_s0_data.test00_0_dst_v0_data != ((ht_int62)0x0001fcabad0714c0LL)) {
				HtAssert(0, 0);
			}
			if ((ht_int12)S_test00_0_dst_s0_data.test00_0_dst_v1_data != ((ht_int12)0x001b711eefa6b380LL)) {
				printf("EXP - 0x%016lx\n", (uint64_t)0x001b711eefa6b380LL);
				printf("ACT - 0x%016lx\n", (uint64_t)S_test00_0_dst_s0_data.test00_0_dst_v1_data);
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
