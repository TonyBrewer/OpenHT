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
			GW_test00_0_src_u0_data.write_addr(0, 9);
			GW_test00_0_src_u0_data.test00_0_src_u1_data.test00_0_src_v2_data = ((uint64_t)0x00000aa97390de80LL);
			P_test00_0_src_u0_data_RdAddr1 = (ht_uint1)0;
			P_test00_0_src_u0_data_RdAddr2 = (ht_uint4)9;
			HtContinue(TEST00_ST0);
			break;
		}
		case TEST00_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_uint64_t(PR_memAddr + 0, GR_test00_0_src_u0_data.test00_0_src_u1_data.test00_0_src_v2_data);
			WriteMemPause(TEST00_LD0);
			break;
		}
		case TEST00_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_0_src_v0_data(PR_memAddr + 0);
			P_test00_0_src_u0_data_RdAddr1 = (ht_uint1)0;
			P_test00_0_src_u0_data_RdAddr2 = (ht_uint4)9;
			ReadMemPause(TEST00_CHK0);
			break;
		}
		case TEST00_CHK0: {
			if (GR_test00_0_src_u0_data.test00_0_src_u1_data.test00_0_src_v0_data != ((uint64_t)0x00000aa97390de80LL)) {
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
