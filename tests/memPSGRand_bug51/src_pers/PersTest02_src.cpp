#include "Ht.h"
#include "PersTest02.h"

void CPersTest02::PersTest02() {
	if (PR1_htValid) {
		switch (PR1_htInst) {
		case TEST02_ENTRY: {
			HtContinue(TEST02_WR);
			break;
		}
		case TEST02_WR: {
			GW1_test02_2_src_s0_data[0].test02_2_src_v2_data[0] = ((int32_t)0x000f769b29b0de80LL);
			GW1_test02_3_src_v0_data[2][1].write_addr(0);
			GW1_test02_3_src_v0_data[2][1] = ((uint16_t)0x00119150471acf80LL);
			HtContinue(TEST02_ST0);
			break;
		}
		case TEST02_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test02_0_dst_s0_data(PR1_memAddr + 0);
			HtContinue(TEST02_ST1);
			break;
		}
		case TEST02_ST1: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test02_1_src_v0_data(PR1_memAddr + 64);
			HtContinue(TEST02_ST2);
			break;
		}
		case TEST02_ST2: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test02_2_src_v2_data(PR1_memAddr + 128, 0, 0, 1);
			HtContinue(TEST02_ST3);
			break;
		}
		case TEST02_ST3: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_uint16_t(PR1_memAddr + 192, GR1_test02_3_src_v0_data[2][1]);
			WriteMemPause(TEST02_LD0);
			break;
		}
		case TEST02_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test02_0_dst_s0_data(PR1_memAddr + 0);
			HtContinue(TEST02_LD1);
			break;
		}
		case TEST02_LD1: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test02_1_dst_v0_data(PR1_memAddr + 64);
			HtContinue(TEST02_LD2);
			break;
		}
		case TEST02_LD2: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test02_2_dst_v0_data(PR1_memAddr + 128);
			HtContinue(TEST02_LD3);
			break;
		}
		case TEST02_LD3: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test02_3_dst_v0_data(PR1_memAddr + 192, 0, 8, 1);
			ReadMemPause(TEST02_CHK);
			break;
		}
		case TEST02_CHK: {
			if ((int16_t)PR1_test02_0_dst_s0_data.test02_0_dst_v0_data != ((int16_t)0x00102586b946bdc0LL)) {
				HtAssert(0, 0);
			}
			if ((int16_t)PR1_test02_0_dst_s0_data.test02_0_dst_v1_data != ((int16_t)0x0004dbed0c8b8ba0LL)) {
				HtAssert(0, 0);
			}
			if ((int32_t)PR1_test02_0_dst_s0_data.test02_0_dst_s1_data[0].test02_0_dst_v3_data != ((int32_t)0x00097da2cf0219e0LL)) {
				HtAssert(0, 0);
			}
			if ((int32_t)P1_test02_1_dst_v0_data[0] != ((int32_t)0x000c1276415dc900LL)) {
				HtAssert(0, 0);
			}
			if ((int32_t)SR_test02_2_dst_v0_data != ((int32_t)0x000f769b29b0de80LL)) {
				HtAssert(0, 0);
			}
			if ((uint16_t)PR1_test02_3_dst_v0_data != ((uint16_t)0x00119150471acf80LL)) {
				HtAssert(0, 0);
			}
			HtContinue(TEST02_RTN);
			break;
		}
		case TEST02_RTN: {
			if (SendReturnBusy_test02()) {
				HtRetry();
				break;
			}
			SendReturn_test02();
			break;
		}
		default:
			assert(0);
		}
	}
	if (PR2_htValid) {
		switch (PR2_htInst) {
		case TEST02_ENTRY: {
			break;
		}
		case TEST02_WR: {
			GW2_test02_1_src_v0_data[2] = ((int32_t)0x000c1276415dc900LL);
			break;
		}
		case TEST02_ST0: {
			break;
		}
		case TEST02_ST1: {
			break;
		}
		case TEST02_ST2: {
			break;
		}
		case TEST02_ST3: {
			break;
		}
		case TEST02_LD0: {
			break;
		}
		case TEST02_LD1: {
			break;
		}
		case TEST02_LD2: {
			break;
		}
		case TEST02_LD3: {
			break;
		}
		case TEST02_CHK: {
			break;
		}
		case TEST02_RTN: {
			break;
		}
		default:
			assert(0);
		}
	}
	if (PR3_htValid) {
		switch (PR3_htInst) {
		case TEST02_ENTRY: {
			break;
		}
		case TEST02_WR: {
			PW3_test02_0_dst_s0_data.test02_0_dst_v0_data = ((int16_t)0x00102586b946bdc0LL);
			PW3_test02_0_dst_s0_data.test02_0_dst_v1_data = ((int16_t)0x0004dbed0c8b8ba0LL);
			PW3_test02_0_dst_s0_data.test02_0_dst_s1_data[0].test02_0_dst_v3_data = ((int32_t)0x00097da2cf0219e0LL);
			P3_test02_3_src_v0_data_RdAddr1 = (ht_uint3)0;
			break;
		}
		case TEST02_ST0: {
			break;
		}
		case TEST02_ST1: {
			break;
		}
		case TEST02_ST2: {
			break;
		}
		case TEST02_ST3: {
			break;
		}
		case TEST02_LD0: {
			break;
		}
		case TEST02_LD1: {
			break;
		}
		case TEST02_LD2: {
			break;
		}
		case TEST02_LD3: {
			P3_test02_3_dst_v0_data_RdAddr1 = (ht_uint2)0;
			P3_test02_3_dst_v0_data_RdAddr2 = (ht_uint4)8;
			break;
		}
		case TEST02_CHK: {
			break;
		}
		case TEST02_RTN: {
			break;
		}
		default:
			assert(0);
		}
	}
}
