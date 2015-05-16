#include "Ht.h"
#include "PersTest08.h"

void CPersTest08::PersTest08() {
	if (PR1_htValid) {
		switch (PR1_htInst) {
		case TEST08_ENTRY: {
			HtContinue(TEST08_WR);
			break;
		}
		case TEST08_WR: {
			GW1_test08_1_src_v0_data.write_addr(12, 1);
			GW1_test08_1_src_v0_data = ((uint8_t)0x0003445563e6b260LL);
			S_test08_3_src_u0_data[2][0].write_addr(6);
			S_test08_3_src_u0_data[2][0].write_mem().test08_3_src_v0_data = ((uint32_t)0x000a82f3112399a0LL);
			HtContinue(TEST08_ST0);
			break;
		}
		case TEST08_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test08_0_src_v1_data(PR1_memAddr + 0, 0);
			HtContinue(TEST08_ST1);
			break;
		}
		case TEST08_ST1: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test08_1_src_v0_data(PR1_memAddr + 64, 12, 1, 1);
			HtContinue(TEST08_ST2);
			break;
		}
		case TEST08_ST2: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_uint8_t(PR1_memAddr + 128, PR1_test08_2_src_v0_data[1]);
			HtContinue(TEST08_ST3);
			break;
		}
		case TEST08_ST3: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			SR_test08_3_src_u0_data[2][0].read_addr(6);
			WriteMem_test08_3_src_u0_data_union(PR1_memAddr + 192, S_test08_3_src_u0_data[2][0].read_mem());
			WriteMemPause(TEST08_LD0);
			break;
		}
		case TEST08_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test08_0_dst_v0_data(PR1_memAddr + 0, 3, 0, 1, 1);
			HtContinue(TEST08_LD1);
			break;
		}
		case TEST08_LD1: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test08_1_dst_v0_data(PR1_memAddr + 64, 0, 1, 1);
			HtContinue(TEST08_LD2);
			break;
		}
		case TEST08_LD2: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test08_2_src_v0_data(PR1_memAddr + 128, 1, 6, 1, 1);
			HtContinue(TEST08_LD3);
			break;
		}
		case TEST08_LD3: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test08_3_src_u0_data(PR1_memAddr + 192, 6, 2, 0, 1);
			ReadMemPause(TEST08_CHK);
			break;
		}
		case TEST08_CHK: {
			if ((int16_t)PR1_test08_0_dst_v0_data[1] != ((int16_t)0x001b32fab259c8e0LL)) {
				HtAssert(0, 0);
			}
			if ((uint8_t)PR1_test08_1_dst_v0_data != ((uint8_t)0x0003445563e6b260LL)) {
				HtAssert(0, 0);
			}
			if ((uint8_t)PR1_test08_2_src_v0_data[1] != ((uint8_t)0x000d4ddc54432b00LL)) {
				HtAssert(0, 0);
			}
			SR_test08_3_src_u0_data[2][0].read_addr(6);
			if ((uint32_t)SR_test08_3_src_u0_data[2][0].read_mem().test08_3_src_v0_data != ((uint32_t)0x000a82f3112399a0LL)) {
				HtAssert(0, 0);
			}
			HtContinue(TEST08_RTN);
			break;
		}
		case TEST08_RTN: {
			if (SendReturnBusy_test08()) {
				HtRetry();
				break;
			}
			SendReturn_test08();
			break;
		}
		default:
			assert(0);
		}
	}
	if (PR2_htValid) {
		switch (PR2_htInst) {
		case TEST08_ENTRY: {
			break;
		}
		case TEST08_WR: {
			P2_test08_0_src_s0_data[0].test08_0_src_u0_data.test08_0_src_u1_data[0].test08_0_src_v1_data = ((int16_t)0x001b32fab259c8e0LL);
			PW2_test08_2_src_v0_data[1].write_addr(1, 6);
			PW2_test08_2_src_v0_data[1] = ((uint8_t)0x000d4ddc54432b00LL);
			P2_test08_2_src_v0_data_RdAddr1 = (ht_uint2)1;
			P2_test08_2_src_v0_data_RdAddr2 = (ht_uint3)6;
			break;
		}
		case TEST08_ST0: {
			break;
		}
		case TEST08_ST1: {
			break;
		}
		case TEST08_ST2: {
			break;
		}
		case TEST08_ST3: {
			break;
		}
		case TEST08_LD0: {
			P2_test08_0_dst_v0_data_RdAddr1 = (ht_uint3)3;
			P2_test08_0_dst_v0_data_RdAddr2 = (ht_uint1)0;
			break;
		}
		case TEST08_LD1: {
			P2_test08_1_dst_v0_data_RdAddr1 = (ht_uint3)0;
			P2_test08_1_dst_v0_data_RdAddr2 = (ht_uint2)1;
			break;
		}
		case TEST08_LD2: {
			P2_test08_2_src_v0_data_RdAddr1 = (ht_uint2)1;
			P2_test08_2_src_v0_data_RdAddr2 = (ht_uint3)6;
			break;
		}
		case TEST08_LD3: {
			break;
		}
		case TEST08_CHK: {
			break;
		}
		case TEST08_RTN: {
			break;
		}
		default:
			assert(0);
		}
	}
}
