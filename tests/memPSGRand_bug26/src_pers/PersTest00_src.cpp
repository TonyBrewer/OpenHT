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
			PW_test00_0_s0_data[0][0].write_addr(0);
			PW_test00_0_s0_data[0][0].test00_0_v0_data = (uint32_t)0x000f282d40dc1a20LL;
			P_test00_0_s0_data_RdAddr1 = (ht_uint1)0;
			HtContinue(TEST00_ST0);
			break;
		}
		case TEST00_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_uint32_t(PR_memAddr + 0, PR_test00_0_s0_data[0][0].test00_0_v0_data);
			WriteMemPause(TEST00_LD0);
			break;
		}
		case TEST00_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_0_v2_data(PR_memAddr + 0, 0, 0, 1);
			ReadMemPause(TEST00_CHK);
			break;
		}
		case TEST00_CHK: {
			S_test00_0_v2_data[0][0].read_addr(0);
			if ((uint32_t)S_test00_0_v2_data[0][0].read_mem() != (uint32_t)0x000f282d40dc1a20LL) {
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
