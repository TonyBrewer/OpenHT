#include "Ht.h"
#include "PersTest00.h"

void CPersTest00::PersTest00() {
	if (PR2_htValid) {
		switch (PR2_htInst) {
		case TEST00_ENTRY: {
			HtContinue(TEST00_WR0);
			break;
		}
		case TEST00_WR0: {
			PW2_test00_0_src_u0_data.write_addr(2);
			PW2_test00_0_src_u0_data.test00_0_src_v0_data = ((int16_t)0x00112f8f2d0ca9e0LL);
			P2_test00_0_src_u0_data_RdAddr1 = (ht_uint2)2;
			HtContinue(TEST00_ST0);
			break;
		}
		case TEST00_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_int16_t(PR2_memAddr + 0, PR2_test00_0_src_u0_data.test00_0_src_v0_data);
			WriteMemPause(TEST00_LD0);
			break;
		}
		case TEST00_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_0_src_v2_data(PR2_memAddr + 0, 2);
			P2_test00_0_src_u0_data_RdAddr1 = (ht_uint2)2;
			ReadMemPause(TEST00_CHK0);
			break;
		}
		case TEST00_CHK0: {
			if (PR2_test00_0_src_u0_data.test00_0_src_v2_data[0] != ((int16_t)0x00112f8f2d0ca9e0LL)) {
				printf("\n");
				printf("EXP: 0x%x\n", (int16_t)0x00112f8f2d0ca9e0LL);
				printf("ACT: 0x%x\n", PR2_test00_0_src_u0_data.test00_0_src_v2_data[0]);
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
