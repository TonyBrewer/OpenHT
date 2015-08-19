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
			GW3_test00_0_src_v0_data.write_addr(0);
			GW3_test00_0_src_v0_data = ((int16_t)0x000f137087c11800LL);
			P3_test00_0_src_v0_data_RdAddr1 = (ht_uint5)0;
			HtContinue(TEST00_ST0);
			break;
		}
		case TEST00_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_int16_t(PR3_memAddr + 0, GR3_test00_0_src_v0_data);
			WriteMemPause(TEST00_LD0);
			break;
		}
		case TEST00_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_0_dst_v1_data(PR3_memAddr + 0, 1, 4);
			P3_test00_0_dst_u0_data_RdAddr1 = (ht_uint1)1;
			ReadMemPause(TEST00_CHK0);
			break;
		}
		case TEST00_CHK0: {
			if (GR3_test00_0_dst_u0_data[4].test00_0_dst_v1_data[0][0] != ((int16_t)0x000f137087c11800LL)) {
				printf("\n");
				printf("EXP: 0x%04x\n", (int16_t)0x000f137087c11800LL);
				printf("ACT: 0x%04x\n", GR3_test00_0_dst_u0_data[4].test00_0_dst_v1_data[0][0]);
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
