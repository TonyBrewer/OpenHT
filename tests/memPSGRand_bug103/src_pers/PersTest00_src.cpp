#include "Ht.h"
#include "PersTest00.h"

void CPersTest00::PersTest00() {
	if (PR_htValid) {
		switch (PR_htInst) {
		case TEST00_ENTRY: {
			HtContinue(TEST00_WR0);
			break;
		}
		case TEST00_WR0: {
			S_test00_0_dst_u0_data[4].write_addr(3);
			S_test00_0_dst_u0_data[4].write_mem().test00_0_dst_v0_data[0][0] = ((uint8_t)0x001791e73186ba40LL);
			S_test00_0_dst_u0_data[4].write_mem().test00_0_dst_v0_data[0][1] = ((uint8_t)0x001791e73186ba40LL);
			HtContinue(TEST00_ST0);
			break;
		}
		case TEST00_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test00_0_dst_u0_data(PR_memAddr + 0, 4);
			WriteMemPause(TEST00_LD0);
			break;
		}
		case TEST00_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_0_dst_u0_data(PR_memAddr + 0, 4);
			ReadMemPause(TEST00_CHK0);
			break;
		}
		case TEST00_CHK0: {
			SR_test00_0_dst_u0_data[4].read_addr(3);
			if (SR_test00_0_dst_u0_data[4].read_mem().test00_0_dst_v0_data[0][0] != ((uint8_t)0x001791e73186ba40LL)) {
				HtAssert(0, 0);
			}
			if (S_test00_0_dst_u0_data[4].read_mem().test00_0_dst_v0_data[0][1] != ((uint8_t)0x001791e73186ba40LL)) {
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
