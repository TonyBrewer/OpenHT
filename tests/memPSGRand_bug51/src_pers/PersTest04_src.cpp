#include "Ht.h"
#include "PersTest04.h"

void CPersTest04::PersTest04() {
	if (PR1_htValid) {
		switch (PR1_htInst) {
		case TEST04_ENTRY: {
			break;
		}
		case TEST04_WR: {
			break;
		}
		case TEST04_ST0: {
			break;
		}
		case TEST04_ST1: {
			break;
		}
		case TEST04_ST2: {
			break;
		}
		case TEST04_ST3: {
			break;
		}
		case TEST04_ST4: {
			break;
		}
		case TEST04_LD0: {
			break;
		}
		case TEST04_LD1: {
			break;
		}
		case TEST04_LD2: {
			break;
		}
		case TEST04_LD3: {
			break;
		}
		case TEST04_LD4: {
			break;
		}
		case TEST04_CHK: {
			break;
		}
		case TEST04_RTN: {
			break;
		}
		default:
			assert(0);
		}
	}
	if (PR2_htValid) {
		switch (PR2_htInst) {
		case TEST04_ENTRY: {
			HtContinue(TEST04_WR);
			break;
		}
		case TEST04_WR: {
			GW2_test04_0_src_u0_data[1].test04_0_src_v0_data = ((uint64_t)0x001a3c80c4e5bfc0LL);
			PW2_test04_1_src_v0_data.write_addr(2);
			PW2_test04_1_src_v0_data = ((uint32_t)0x000667c9044abb40LL);
			GW2_test04_2_src_v0_data[0][2].write_addr(1);
			GW2_test04_2_src_v0_data[0][2] = ((int8_t)0x000e7f16cfa54140LL);
			S_test04_3_src_u0_data[0][0].write_addr(1, 7);
			S_test04_3_src_u0_data[0][0].write_mem().test04_3_src_v0_data[1][0] = ((int16_t)0x000a21e420f8e980LL);
			S_test04_4_src_v0_data[1] = ((uint16_t)0x001bed17ed4dc2a0LL);
			P2_test04_1_src_v0_data_RdAddr1 = (ht_uint2)2;
			HtContinue(TEST04_ST0);
			break;
		}
		case TEST04_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_uint64_t(PR2_memAddr + 0, GR2_test04_0_src_u0_data[1].test04_0_src_v0_data);
			HtContinue(TEST04_ST1);
			break;
		}
		case TEST04_ST1: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_uint32_t(PR2_memAddr + 64, PR2_test04_1_src_v0_data);
			HtContinue(TEST04_ST2);
			break;
		}
		case TEST04_ST2: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test04_2_src_v0_data(PR2_memAddr + 128, 1, 1);
			HtContinue(TEST04_ST3);
			break;
		}
		case TEST04_ST3: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test04_3_src_v0_data(PR2_memAddr + 192, 1, 7, 0, 1);
			HtContinue(TEST04_ST4);
			break;
		}
		case TEST04_ST4: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test04_4_src_v0_data(PR2_memAddr + 256);
			WriteMemPause(TEST04_LD0);
			break;
		}
		case TEST04_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test04_0_dst_v0_data(PR2_memAddr + 0, 0, 1);
			HtContinue(TEST04_LD1);
			break;
		}
		case TEST04_LD1: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test04_1_src_v0_data(PR2_memAddr + 64, 2, 1);
			P2_test04_1_src_v0_data_RdAddr1 = (ht_uint2)2;
			HtContinue(TEST04_LD2);
			break;
		}
		case TEST04_LD2: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test04_2_dst_v0_data(PR2_memAddr + 128, 3, 13, 0, 0, 1);
			P2_test04_2_dst_v0_data_RdAddr1 = (ht_uint2)3;
			P2_test04_2_dst_v0_data_RdAddr2 = (ht_uint4)13;
			HtContinue(TEST04_LD3);
			break;
		}
		case TEST04_LD3: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test04_3_dst_v0_data(PR2_memAddr + 192, 5, 8, 1);
			P2_test04_3_dst_v0_data_RdAddr1 = (ht_uint4)5;
			P2_test04_3_dst_v0_data_RdAddr2 = (ht_uint4)8;
			HtContinue(TEST04_LD4);
			break;
		}
		case TEST04_LD4: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test04_4_dst_v0_data(PR2_memAddr + 256);
			ReadMemPause(TEST04_CHK);
			break;
		}
		case TEST04_CHK: {
			if ((uint64_t)PR2_test04_0_dst_v0_data[0] != ((uint64_t)0x001a3c80c4e5bfc0LL)) {
				HtAssert(0, 0);
			}
			if ((uint32_t)PR2_test04_1_src_v0_data != ((uint32_t)0x000667c9044abb40LL)) {
				HtAssert(0, 0);
			}
			if ((int8_t)PR2_test04_2_dst_v0_data[0][0] != ((int8_t)0x000e7f16cfa54140LL)) {
				HtAssert(0, 0);
			}
			if ((int16_t)GR2_test04_3_dst_v0_data != ((int16_t)0x000a21e420f8e980LL)) {
				HtAssert(0, 0);
			}
			if ((uint16_t)PR2_test04_4_dst_v0_data != ((uint16_t)0x001bed17ed4dc2a0LL)) {
				HtAssert(0, 0);
			}
			HtContinue(TEST04_RTN);
			break;
		}
		case TEST04_RTN: {
			if (SendReturnBusy_test04()) {
				HtRetry();
				break;
			}
			SendReturn_test04();
			break;
		}
		default:
			assert(0);
		}
	}
}
