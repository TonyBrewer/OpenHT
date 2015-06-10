#include "Ht.h"
#include "PersTest00.h"

void CPersTest00::PersTest00() {
	if (PR1_htValid) {
		switch (PR1_htInst) {
		case TEST00_ENTRY: {
			break;
		}
		case TEST00_WR0: {
			break;
		}
		case TEST00_ST0: {
			break;
		}
		case TEST00_LD0: {
			break;
		}
		case TEST00_CHK0: {
			SR_test00_0_dst_v0_data.read_addr(11);
			break;
		}
		case TEST00_RTN: {
			break;
		}
		default:
			assert(0);
		}
	}
	if (PR2_htValid) {
		switch (PR2_htInst) {
		case TEST00_ENTRY: {
			HtContinue(TEST00_WR0);
			break;
		}
		case TEST00_WR0: {
			HtContinue(TEST00_ST0);
			break;
		}
		case TEST00_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_uint64_t(PR2_memAddr + 0, PR2_test00_0_src_v0_data[0][0]);
			WriteMemPause(TEST00_LD0);
			break;
		}
		case TEST00_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_0_dst_v0_data(PR2_memAddr + 0, 11, 1);
			ReadMemPause(TEST00_CHK0);
			break;
		}
		case TEST00_CHK0: {
			if ((uint64_t)S_test00_0_dst_v0_data.read_mem() != ((uint64_t)0x00054cd4f5c09a60LL)) {
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
	if (PR3_htValid) {
		switch (PR3_htInst) {
		case TEST00_ENTRY: {
			break;
		}
		case TEST00_WR0: {
			PW3_test00_0_src_v0_data[0][0].write_addr(0, 14);
			PW3_test00_0_src_v0_data[0][0] = ((uint64_t)0x00054cd4f5c09a60LL);
			P3_test00_0_src_v0_data_RdAddr1 = (ht_uint1)0;
			P3_test00_0_src_v0_data_RdAddr2 = (ht_uint4)14;
			break;
		}
		case TEST00_ST0: {
			break;
		}
		case TEST00_LD0: {
			break;
		}
		case TEST00_CHK0: {
			break;
		}
		case TEST00_RTN: {
			break;
		}
		default:
			assert(0);
		}
	}
}
