#include "Ht.h"
#include "PersTest00.h"

void CPersTest00::PersTest00() {
	if (PR3_htValid) {
		switch (PR3_htInst) {
		case TEST00_ENTRY: {
			HtContinue(TEST00_WR0);
			break;
		}
		case TEST00_WR0: {
			P3_test00_0_src_v0_data[1][4] = ((int8_t)0x00172ba1a2d3dda0LL);
			HtContinue(TEST00_ST0);
			break;
		}
		case TEST00_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test00_0_src_v0_data(PR3_memAddr + 0, 1, 4, 1);
			WriteMemPause(TEST00_LD0);
			break;
		}
		case TEST00_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_0_dst_v0_data(PR3_memAddr + 0, 1);
			P3_test00_0_dst_v0_data_RdAddr1 = (ht_uint1)1;
			ReadMemPause(TEST00_CHK0);
			break;
		}
		case TEST00_CHK0: {
			if (PR3_test00_0_dst_v0_data != ((int8_t)0x00172ba1a2d3dda0LL)) {
				printf("EXP - 0x%02x\n", (int8_t)0x00172ba1a2d3dda0LL);
				printf("ACT - 0x%02x\n", (int8_t)PR3_test00_0_dst_v0_data);
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
