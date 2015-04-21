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
			S_test00_0_u0_data.test00_0_v0_data[0][0] = (uint32_t)0x001549320c3c1a20LL;
			HtContinue(TEST00_ST0);
			break;
		}
		case TEST00_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_uint32_t(PR_memAddr + 0, SR_test00_0_u0_data.test00_0_v0_data[0][0]);
			WriteMemPause(TEST00_LD0);
			break;
		}
		case TEST00_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_0_v4_data(PR_memAddr + 0);
			ReadMemPause(TEST00_CHK);
			break;
		}
		case TEST00_CHK: {
			if ((uint32_t)GR_test00_0_v4_data != (uint32_t)0x001549320c3c1a20LL) {
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
