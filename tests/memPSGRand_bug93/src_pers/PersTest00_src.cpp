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
			S_test00_0_src_s0_data.write_addr(3);
			S_test00_0_src_s0_data.write_mem().test00_0_src_v0_data = ((int64_t)0x0016a3afc64d5360LL);
			S_test00_0_src_s0_data.write_mem().test00_0_src_v1_data = ((ht_uint63)0x0009badc566bd8c0LL);
			S_test00_0_src_s0_data.write_mem().test00_0_src_v2_data = ((int64_t)0x00039a12b3c03900LL);
			S_test00_0_src_s0_data.write_mem().test00_0_src_v3_data = ((ht_uint4)0x001bd99804d94c20LL);
			HtContinue(TEST00_ST0);
			break;
		}
		case TEST00_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test00_0_src_s0_data(PR_memAddr + 0);
			WriteMemPause(TEST00_LD0);
			break;
		}
		case TEST00_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_0_src_s0_data(PR_memAddr + 0);
			ReadMemPause(TEST00_CHK0);
			break;
		}
		case TEST00_CHK0: {
			S_test00_0_src_s0_data.read_addr(14);
			if (SR_test00_0_src_s0_data.read_mem().test00_0_src_v0_data != ((int64_t)0x0016a3afc64d5360LL)) {
				HtAssert(0, 0);
			}
			if (SR_test00_0_src_s0_data.read_mem().test00_0_src_v1_data != ((ht_uint63)0x0009badc566bd8c0LL)) {
				printf("EXP = 0x%016lx\n", (uint64_t)0x0009badc566bd8c0LL);
				printf("ACT = 0x%016lx\n", (uint64_t)SR_test00_0_src_s0_data.read_mem().test00_0_src_v1_data);
				HtAssert(0, 0);
			}
			if (S_test00_0_src_s0_data.read_mem().test00_0_src_v2_data != ((ht_int64)0x00039a12b3c03900LL)) {
				HtAssert(0, 0);
			}
			if (SR_test00_0_src_s0_data.read_mem().test00_0_src_v3_data != ((ht_uint4)0x001bd99804d94c20LL)) {
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
