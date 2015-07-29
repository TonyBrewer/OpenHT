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
			S_test00_0_src_v0_data.write_addr(0, 6);
			S_test00_0_src_v0_data.write_mem((int32_t)0x000e04fde06069c0LL);
			HtContinue(TEST00_WR1);
			break;
		}
		case TEST00_WR1: {
			S_test00_0_src_v0_data.write_addr(0, 7);
			S_test00_0_src_v0_data.write_mem((int32_t)0x000e04fde06069c0LL);
			HtContinue(TEST00_WR2);
			break;
		}
		case TEST00_WR2: {
			S_test00_0_src_v0_data.write_addr(0, 8);
			S_test00_0_src_v0_data.write_mem((int32_t)0x000e04fde06069c0LL);
			HtContinue(TEST00_WR3);
			break;
		}
		case TEST00_WR3: {
			S_test00_0_src_v0_data.write_addr(0, 9);
			S_test00_0_src_v0_data.write_mem((int32_t)0x000e04fde06069c0LL);
			HtContinue(TEST00_WR4);
			break;
		}
		case TEST00_WR4: {
			S_test00_0_src_v0_data.write_addr(0, 10);
			S_test00_0_src_v0_data.write_mem((int32_t)0x000e04fde06069c0LL);
			HtContinue(TEST00_WR5);
			break;
		}
		case TEST00_WR5: {
			S_test00_0_src_v0_data.write_addr(0, 11);
			S_test00_0_src_v0_data.write_mem((int32_t)0x000e04fde06069c0LL);
			HtContinue(TEST00_WR6);
			break;
		}
		case TEST00_WR6: {
			S_test00_0_src_v0_data.write_addr(0, 12);
			S_test00_0_src_v0_data.write_mem((int32_t)0x000e04fde06069c0LL);
			HtContinue(TEST00_ST0);
			break;
		}
		case TEST00_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test00_0_src_v0_data(PR_memAddr + 0, 0, 6, 11);
			WriteMemPause(TEST00_LD0);
			break;
		}
		case TEST00_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_0_src_v0_data(PR_memAddr + 0, 1, 15);
			ReadMemPause(TEST00_CHK0);
			break;
		}
		case TEST00_CHK0: {
			SR_test00_0_src_v0_data.read_addr(0, 1);
			if (SR_test00_0_src_v0_data.read_mem() != ((int32_t)0x000e04fde06069c0LL)) {
				HtAssert(0, 0);
			}
			HtContinue(TEST00_CHK1);
			break;
		}
		case TEST00_CHK1: {
			S_test00_0_src_v0_data.read_addr(0, 2);
			if (SR_test00_0_src_v0_data.read_mem() != ((int32_t)0x000e04fde06069c0LL)) {
				printf("EXP: 0x%08x\n", (int32_t)0x000e04fde06069c0LL);
				printf("ACT: 0x%08x\n", (int32_t)SR_test00_0_src_v0_data.read_mem());
				HtAssert(0, 0);
			}
			HtContinue(TEST00_CHK2);
			break;
		}
		case TEST00_CHK2: {
			SR_test00_0_src_v0_data.read_addr(0, 3);
			if (SR_test00_0_src_v0_data.read_mem() != ((int32_t)0x000e04fde06069c0LL)) {
				HtAssert(0, 0);
			}
			HtContinue(TEST00_CHK3);
			break;
		}
		case TEST00_CHK3: {
			S_test00_0_src_v0_data.read_addr(0, 4);
			if (S_test00_0_src_v0_data.read_mem() != ((int32_t)0x000e04fde06069c0LL)) {
				HtAssert(0, 0);
			}
			HtContinue(TEST00_CHK4);
			break;
		}
		case TEST00_CHK4: {
			SR_test00_0_src_v0_data.read_addr(0, 5);
			if (SR_test00_0_src_v0_data.read_mem() != ((int32_t)0x000e04fde06069c0LL)) {
				HtAssert(0, 0);
			}
			HtContinue(TEST00_CHK5);
			break;
		}
		case TEST00_CHK5: {
			S_test00_0_src_v0_data.read_addr(0, 6);
			if (SR_test00_0_src_v0_data.read_mem() != ((int32_t)0x000e04fde06069c0LL)) {
				HtAssert(0, 0);
			}
			HtContinue(TEST00_CHK6);
			break;
		}
		case TEST00_CHK6: {
			SR_test00_0_src_v0_data.read_addr(0, 7);
			if (SR_test00_0_src_v0_data.read_mem() != ((int32_t)0x000e04fde06069c0LL)) {
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
