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
			GW1_test00_0_src_v0_data[0].write_addr(1);
			GW1_test00_0_src_v0_data[0] = ((uint64_t)0x0001054a2ec7ca00LL);
			S_test00_2_src_s0_data.test00_2_src_v2_data[1] = ((uint8_t)0x001cdebbaaa818e0LL);
			HtContinue(TEST00_ST0);
			break;
		}
		case TEST00_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test00_0_src_v0_data(PR1_memAddr + 0, 1, 1);
			HtContinue(TEST00_ST1);
			break;
		}
		case TEST00_ST1: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_int32_t(PR1_memAddr + 64, PR1_test00_1_src_v0_data[1][0]);
			HtContinue(TEST00_ST2);
			break;
		}
		case TEST00_ST2: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test00_2_src_v2_data(PR1_memAddr + 128);
			WriteMemPause(TEST00_LD0);
			break;
		}
		case TEST00_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_0_src_v0_data(PR1_memAddr + 0, 1, 1);
			HtContinue(TEST00_LD1);
			break;
		}
		case TEST00_LD1: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_1_src_v0_data(PR1_memAddr + 64, 2, 1, 0, 1);
			HtContinue(TEST00_LD2);
			break;
		}
		case TEST00_LD2: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_2_dst_v0_data(PR1_memAddr + 128, 5, 1);
			ReadMemPause(TEST00_CHK);
			break;
		}
		case TEST00_CHK: {
			if ((uint64_t)GR1_test00_0_src_v0_data[0] != ((uint64_t)0x0001054a2ec7ca00LL)) {
				HtAssert(0, (uint32_t)0x00000000);
			}
			if ((int32_t)PR1_test00_1_src_v0_data[1][0] != ((int32_t)0x0015c6016da18080LL)) {
				HtAssert(0, (uint32_t)0x00000001);
			}
			if ((uint8_t)GR1_test00_2_dst_v0_data != ((uint8_t)0x001cdebbaaa818e0LL)) {
				HtAssert(0, (uint32_t)0x00000002);
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
	if (PR2_htValid) {
		switch (PR2_htInst) {
		case TEST00_ENTRY: {
			break;
		}
		case TEST00_WR: {
			PW2_test00_1_src_v0_data[1][0].write_addr(2);
			PW2_test00_1_src_v0_data[1][0] = ((int32_t)0x0015c6016da18080LL);
			P2_test00_1_src_v0_data_RdAddr1 = (ht_uint2)2;
			break;
		}
		case TEST00_ST0: {
			break;
		}
		case TEST00_ST1: {
			break;
		}
		case TEST00_ST2: {
			break;
		}
		case TEST00_LD0: {
			P2_test00_0_src_v0_data_RdAddr1 = (ht_uint3)1;
			break;
		}
		case TEST00_LD1: {
			P2_test00_1_src_v0_data_RdAddr1 = (ht_uint2)2;
			break;
		}
		case TEST00_LD2: {
			P2_test00_2_dst_v0_data_RdAddr1 = (ht_uint3)5;
			break;
		}
		case TEST00_CHK: {
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
