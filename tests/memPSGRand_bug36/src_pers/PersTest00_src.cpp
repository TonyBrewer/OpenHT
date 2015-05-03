#include "Ht.h"
#include "PersTest00.h"

void CPersTest00::PersTest00() {
	if (PR1_htValid) {
		switch (PR1_htInst) {
		case TEST00_ENTRY: {
			HtContinue(TEST00_WR);
			break;
		}
		case TEST00_WR: {
			S_test00_0_src_u0_data.write_addr(1);
			S_test00_0_src_u0_data.write_mem().test00_0_src_v2_data = ((uint16_t)0x001bcfbd25fc3420LL);
			HtContinue(TEST00_ST0);
			break;
		}
		case TEST00_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			S_test00_0_src_u0_data.read_addr(1);
			WriteMem_uint16_t(PR1_memAddr + 0, S_test00_0_src_u0_data.read_mem().test00_0_src_v2_data);
			WriteMemPause(TEST00_LD0);
			break;
		}
		case TEST00_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_0_src_v3_data(PR1_memAddr + 0, 1, 1);
			ReadMemPause(TEST00_CHK);
			break;
		}
		case TEST00_CHK: {
			S_test00_0_src_u0_data.read_addr(1);
			if ((uint16_t)SR_test00_0_src_u0_data.read_mem().test00_0_src_v3_data != ((uint16_t)0x001bcfbd25fc3420LL)) {
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
