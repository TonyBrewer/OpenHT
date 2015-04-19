#include "Ht.h"
#include "PersTest00.h"

void CPersTest00::PersTest00() {
	if (PR_htValid) {
		switch (PR_htInst) {
		case TEST00_ENTRY: {
			HtContinue(TEST00_WR);
			break;
		}
		case TEST00_WR: {
			S_test00_0_u0_data[0].write_addr(1);
			S_test00_0_u0_data[0].write_mem().test00_0_u2_data.test00_0_v2_data[0][1] = (uint32_t)0x001df0c19e778e20LL;
			HtContinue(TEST00_ST0);
			break;
		}
		case TEST00_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			S_test00_0_u0_data[0].read_addr(1);
			WriteMem_uint32_t(PR_memAddr + 0, SR_test00_0_u0_data[0].read_mem().test00_0_u2_data.test00_0_v2_data[0][1]);
			WriteMemPause(TEST00_LD0);
			break;
		}
		case TEST00_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_0_v7_data(PR_memAddr + 0);
			ReadMemPause(TEST00_CHK);
			break;
		}
		case TEST00_CHK: {
			if ((uint32_t)S_test00_0_v7_data != (uint32_t)0x001df0c19e778e20LL) {
				HtAssert(0, (uint32_t)0x00000000);
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
