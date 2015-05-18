#include "Ht.h"
#include "PersTest05.h"

void CPersTest05::PersTest05() {
	if (PR1_htValid) {
		switch (PR1_htInst) {
		case TEST05_ENTRY: {
			HtContinue(TEST05_WR);
			break;
		}
		case TEST05_WR: {
			GW1_test05_1_src_v0_data.write_addr(1, 2);
			GW1_test05_1_src_v0_data = ((uint32_t)0x000e0ee0743246a0LL);
			S_test05_2_src_v0_data[1][0] = ((uint64_t)0x0016f0f7024f88c0LL);
			S_test05_3_dst_s0_data[1].write_addr(1);
			S_test05_3_dst_s0_data[1].write_mem().test05_3_dst_v0_data = ((int8_t)0x000fe190cc1c5d20LL);
			HtContinue(TEST05_ST0);
			break;
		}
		case TEST05_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_int16_t(PR1_memAddr + 0, PR1_test05_0_src_v0_data[2]);
			HtContinue(TEST05_ST1);
			break;
		}
		case TEST05_ST1: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test05_1_src_v0_data(PR1_memAddr + 64, 1, 2, 1);
			HtContinue(TEST05_ST2);
			break;
		}
		case TEST05_ST2: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test05_2_src_v0_data(PR1_memAddr + 128, 1, 0, 1);
			HtContinue(TEST05_ST3);
			break;
		}
		case TEST05_ST3: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			SR_test05_3_dst_s0_data[1].read_addr(1);
			WriteMem_test05_3_dst_s0_data_struct(PR1_memAddr + 192, SR_test05_3_dst_s0_data[1].read_mem());
			WriteMemPause(TEST05_LD0);
			break;
		}
		case TEST05_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test05_0_dst_v0_data(PR1_memAddr + 0, 2, 0, 1);
			HtContinue(TEST05_LD1);
			break;
		}
		case TEST05_LD1: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test05_1_src_v0_data(PR1_memAddr + 64, 1, 2, 1);
			HtContinue(TEST05_LD2);
			break;
		}
		case TEST05_LD2: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test05_2_dst_v0_data(PR1_memAddr + 128, 2, 14, 0, 0, 1);
			HtContinue(TEST05_LD3);
			break;
		}
		case TEST05_LD3: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test05_3_dst_s0_data(PR1_memAddr + 192, 1, 1, 1);
			ReadMemPause(TEST05_CHK);
			break;
		}
		case TEST05_CHK: {
			if ((int16_t)GR1_test05_0_dst_u0_data[2].test05_0_dst_v0_data[0][1] != ((int16_t)0x0004b4ff2aff2220LL)) {
				HtAssert(0, 0);
			}
			if ((uint32_t)GR1_test05_1_src_v0_data != ((uint32_t)0x000e0ee0743246a0LL)) {
				HtAssert(0, 0);
			}
			if ((uint64_t)GR1_test05_2_dst_v0_data[0][0] != ((uint64_t)0x0016f0f7024f88c0LL)) {
				HtAssert(0, 0);
			}
			SR_test05_3_dst_s0_data[1].read_addr(1);
			if ((int8_t)SR_test05_3_dst_s0_data[1].read_mem().test05_3_dst_v0_data != ((int8_t)0x000fe190cc1c5d20LL)) {
				HtAssert(0, 0);
			}
			HtContinue(TEST05_RTN);
			break;
		}
		case TEST05_RTN: {
			if (SendReturnBusy_test05()) {
				HtRetry();
				break;
			}
			SendReturn_test05();
			break;
		}
		default:
			assert(0);
		}
	}
	if (PR2_htValid) {
		switch (PR2_htInst) {
		case TEST05_ENTRY: {
			break;
		}
		case TEST05_WR: {
			PW2_test05_0_src_v0_data[2].write_addr(3, 0);
			PW2_test05_0_src_v0_data[2] = ((int16_t)0x0004b4ff2aff2220LL);
			P2_test05_0_src_v0_data_RdAddr1 = (ht_uint2)3;
			P2_test05_0_src_v0_data_RdAddr2 = (ht_uint1)0;
			break;
		}
		case TEST05_ST0: {
			break;
		}
		case TEST05_ST1: {
			break;
		}
		case TEST05_ST2: {
			break;
		}
		case TEST05_ST3: {
			break;
		}
		case TEST05_LD0: {
			break;
		}
		case TEST05_LD1: {
			P2_test05_1_src_v0_data_RdAddr1 = (ht_uint1)1;
			P2_test05_1_src_v0_data_RdAddr2 = (ht_uint2)2;
			break;
		}
		case TEST05_LD2: {
			P2_test05_2_dst_v0_data_RdAddr1 = (ht_uint4)2;
			P2_test05_2_dst_v0_data_RdAddr2 = (ht_uint4)14;
			break;
		}
		case TEST05_LD3: {
			break;
		}
		case TEST05_CHK: {
			break;
		}
		case TEST05_RTN: {
			break;
		}
		default:
			assert(0);
		}
	}
}
