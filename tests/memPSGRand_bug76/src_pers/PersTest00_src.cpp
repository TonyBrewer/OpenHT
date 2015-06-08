#include "Ht.h"
#include "PersTest00.h"

void CPersTest00::PersTest00() {
	if (PR1_htValid) {
		switch (PR1_htInst) {
		case TEST00_ENTRY: {
			break;
		}
		case TEST00_WR: {
			break;
		}
		case TEST00_ST0: {
			SR_test00_0_src_s0_data.read_addr(26, 2);
			break;
		}
		case TEST00_LD0: {
			break;
		}
		case TEST00_CHK: {
			S_test00_0_src_s0_data.read_addr(3, 0);
			break;
		}
		case TEST00_RTN: {
			break;
		}
		default:
			assert(0);
		}
	}
	if (PR2_htValid) {
		switch (PR2_htInst) {
		case TEST00_ENTRY: {
			HtContinue(TEST00_WR);
			break;
		}
		case TEST00_WR: {
			S_test00_0_src_s0_data.write_addr(26, 2);
			test00_0_src_s0_data_struct temp_S_test00_0_src_s0_data;
			temp_S_test00_0_src_s0_data.test00_0_src_v0_data[0] = ((int32_t)0x001411ded6c3e600LL);
			temp_S_test00_0_src_s0_data.test00_0_src_v0_data[1] = ((int32_t)0x001411ded6c3e600LL);
			S_test00_0_src_s0_data.write_mem(/*(test00_0_src_s0_data_struct)*/temp_S_test00_0_src_s0_data);
			HtContinue(TEST00_ST0);
			break;
		}
		case TEST00_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test00_0_src_s0_data_struct(PR2_memAddr + 0, S_test00_0_src_s0_data.read_mem());
			WriteMemPause(TEST00_LD0);
			break;
		}
		case TEST00_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_0_src_s0_data(PR2_memAddr + 0, 3, 0, 1);
			ReadMemPause(TEST00_CHK);
			break;
		}
		case TEST00_CHK: {
			if ((int32_t)S_test00_0_src_s0_data.read_mem().test00_0_src_v0_data[0] != ((int32_t)0x001411ded6c3e600LL)) {
				HtAssert(0, 0);
			}
			if ((int32_t)SR_test00_0_src_s0_data.read_mem().test00_0_src_v0_data[1] != ((int32_t)0x001411ded6c3e600LL)) {
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
