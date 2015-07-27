#include "Ht.h"
#include "PersTest00.h"

void CPersTest00::PersTest00() {
	if (PR1_htValid) {
		switch (PR1_htInst) {
		case TEST00_ENTRY: {
			HtContinue(TEST00_WR0);
			break;
		}
		case TEST00_WR0: {
			HtContinue(TEST00_ST0);
			break;
		}
		case TEST00_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test00_0_src_s1_data(PR1_memAddr + 0, 31, 24, 3, 1, 0);
			WriteMemPause(TEST00_LD0);
			break;
		}
		case TEST00_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_0_src_s1_data(PR1_memAddr + 0, 31, 24, 3, 1, 0);
			ReadMemPause(TEST00_CHK0);
			break;
		}
		case TEST00_CHK0: {
			if (PR1_test00_0_src_s0_data[3][1].test00_0_src_s1_data[3][0].test00_0_src_v0_data != ((int16_t)0x00076aa0fe052b40LL)) {
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
	if (PR3_htValid) {
		switch (PR3_htInst) {
		case TEST00_ENTRY: {
			break;
		}
		case TEST00_WR0: {
			PW3_test00_0_src_s0_data[3][1].write_addr(31, 24);
			PW3_test00_0_src_s0_data[3][1].test00_0_src_s1_data[3][0].test00_0_src_v0_data = ((int16_t)0x00076aa0fe052b40LL);
			break;
		}
		case TEST00_ST0: {
			break;
		}
		case TEST00_LD0: {
			P3_test00_0_src_s0_data_RdAddr1 = (ht_uint5)31;
			P3_test00_0_src_s0_data_RdAddr2 = (ht_uint5)24;
			break;
		}
		case TEST00_CHK0: {
			break;
		}
		case TEST00_RTN: {
			break;
		}
		default:
			assert(0);
		}
	}
}
