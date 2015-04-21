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
			P_test00_0_v0_data = (uint16_t)0x0001a43b4d1c60c0LL;
			HtContinue(TEST00_ST0);
			break;
		}
		case TEST00_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_uint16_t(PR_memAddr + 0, P_test00_0_v0_data);
			WriteMemPause(TEST00_LD0);
			break;
		}
		case TEST00_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_0_v4_data(PR_memAddr + 0, 0, 1);
			ReadMemPause(TEST00_CHK);
			break;
		}
		case TEST00_CHK: {
			if ((uint16_t)GR_test00_0_v4_data[0][0] != (uint16_t)0x0001a43b4d1c60c0LL) {
				HtAssert(0, (uint32_t)0x00010000);
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
