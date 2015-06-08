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
			break;
		}
		case TEST00_LD0: {
			break;
		}
		case TEST00_CHK: {
			S_test00_0_dst_s0_data[1].read_addr(5, 1);
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
			S_test00_0_dst_s0_data[1].write_addr(6, 1);
			test00_0_dst_s0_data_struct data;
			data.test00_0_dst_v0_data = ((int64_t)0x0014f2c0273dcfa0LL);
			S_test00_0_dst_s0_data[1].write_mem(data);
			HtContinue(TEST00_ST0);
			break;
		}
		case TEST00_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test00_0_dst_s0_data(PR2_memAddr + 0, 6, 1, 1);
			WriteMemPause(TEST00_LD0);
			break;
		}
		case TEST00_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_0_dst_s0_data(PR2_memAddr + 0, 5, 1, 1);
			ReadMemPause(TEST00_CHK);
			break;
		}
		case TEST00_CHK: {
			if ((int64_t)S_test00_0_dst_s0_data[1].read_mem().test00_0_dst_v0_data != ((int64_t)0x0014f2c0273dcfa0LL)) {
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
