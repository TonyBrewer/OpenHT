#include "Ht.h"
#include "PersTest11.h"

void CPersTest11::PersTest11() {
	if (PR2_htValid) {
		switch (PR2_htInst) {
		case TEST11_ENTRY: {
			HtContinue(TEST11_WR0);
			break;
		}
		case TEST11_WR0: {
			S_test11_0_src_s0_data.write_addr(1);
			S_test11_0_src_s0_data.write_mem().test11_0_src_v0_data = ((int8_t)0x0003587582487520LL);
			HtContinue(TEST11_ST0);
			break;
		}
		case TEST11_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			S_test11_0_src_s0_data.read_addr(1);
			WriteMem_int8_t(PR2_memAddr + 0, S_test11_0_src_s0_data.read_mem().test11_0_src_v0_data);
			WriteMemPause(TEST11_LD0);
			break;
		}
		case TEST11_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test11_0_dst_v0_data(PR2_memAddr + 0);
			ReadMemPause(TEST11_CHK0);
			break;
		}
		case TEST11_CHK0: {
			if (PR2_test11_0_dst_v0_data != ((int8_t)0x0003587582487520LL)) {
				HtAssert(0, 0);
			}
			HtContinue(TEST11_RTN);
			break;
		}
		case TEST11_RTN: {
			if (SendReturnBusy_test11()) {
				HtRetry();
				break;
			}
			SendReturn_test11();
			break;
		}
		default:
			assert(0);
		}
	}
}
