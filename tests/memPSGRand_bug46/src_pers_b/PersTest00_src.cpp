#include "Ht.h"
#include "PersTest00.h"

void CPersTest00::PersTest00() {
	if (PR1_htValid) {
		switch (PR1_htInst) {
		case TEST00_ENTRY: {
			break;
		}
		case TEST00_WR: {
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
		case TEST00_ST3: {
			break;
		}
		case TEST00_ST4: {
			break;
		}
		case TEST00_LD0: {
			break;
		}
		case TEST00_LD1: {
			break;
		}
		case TEST00_LD2: {
			break;
		}
		case TEST00_LD3: {
			break;
		}
		case TEST00_LD4: {
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
	if (PR3_htValid) {
		switch (PR3_htInst) {
		case TEST00_ENTRY: {
			HtContinue(TEST00_WR);
			break;
		}
		case TEST00_WR: {
			GW3_test00_0_src_v0_data = ((int16_t)0x00192aa94393dec0LL);
			GW3_test00_1_src_v0_data[0] = ((uint32_t)0x00111ffb66693520LL);
			GW3_test00_2_src_v0_data.write_addr(3);
			GW3_test00_2_src_v0_data = ((uint32_t)0x001d7f229c1c49a0LL);
			GW3_test00_3_src_v0_data[0][0].write_addr(0, 0);
			GW3_test00_3_src_v0_data[0][0] = ((uint16_t)0x000cec97f4c84740LL);
			GW3_test00_4_src_v0_data.write_addr(1, 9);
			GW3_test00_4_src_v0_data = ((uint64_t)0x000beba9f75cc5c0LL);
			P3_test00_2_src_v0_data_RdAddr1 = (ht_uint3)3;
			P3_test00_3_src_v0_data_RdAddr1 = (ht_uint1)0;
			P3_test00_3_src_v0_data_RdAddr2 = (ht_uint2)0;
			P3_test00_4_src_v0_data_RdAddr1 = (ht_uint1)1;
			P3_test00_4_src_v0_data_RdAddr2 = (ht_uint4)9;
			HtContinue(TEST00_ST0);
			break;
		}
		case TEST00_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_int16_t(PR3_memAddr + 0, GR3_test00_0_src_v0_data);
			HtContinue(TEST00_ST1);
			break;
		}
		case TEST00_ST1: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test00_1_src_v0_data(PR3_memAddr + 64);
			HtContinue(TEST00_ST2);
			break;
		}
		case TEST00_ST2: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_uint32_t(PR3_memAddr + 128, GR3_test00_2_src_v0_data);
			HtContinue(TEST00_ST3);
			break;
		}
		case TEST00_ST3: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_uint16_t(PR3_memAddr + 192, GR3_test00_3_src_v0_data[0][0]);
			HtContinue(TEST00_ST4);
			break;
		}
		case TEST00_ST4: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_uint64_t(PR3_memAddr + 256, GR3_test00_4_src_v0_data);
			WriteMemPause(TEST00_LD0);
			break;
		}
		case TEST00_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_0_dst_v0_data(PR3_memAddr + 0);
			HtContinue(TEST00_LD1);
			break;
		}
		case TEST00_LD1: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_1_dst_v1_data(PR3_memAddr + 64, 0);
			HtContinue(TEST00_LD2);
			break;
		}
		case TEST00_LD2: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_2_dst_v0_data(PR3_memAddr + 128, 1, 1);
			HtContinue(TEST00_LD3);
			break;
		}
		case TEST00_LD3: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_3_dst_v1_data(PR3_memAddr + 192, 12, 0, 0, 0, 0, 1);
			P3_test00_3_dst_s0_data_RdAddr1 = (ht_uint4)12;
			P3_test00_3_dst_s0_data_RdAddr2 = (ht_uint1)0;
			HtContinue(TEST00_LD4);
			break;
		}
		case TEST00_LD4: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_4_dst_v0_data(PR3_memAddr + 256, 1, 1, 1, 2, 1, 1);
			P3_test00_4_dst_s0_data_RdAddr1 = (ht_uint1)1;
			P3_test00_4_dst_s0_data_RdAddr2 = (ht_uint1)1;
			ReadMemPause(TEST00_CHK);
			break;
		}
		case TEST00_CHK: {
			if ((int16_t)P3_test00_0_dst_s0_data.test00_0_dst_v0_data != ((int16_t)0x00192aa94393dec0LL)) {
				HtAssert(0, 0);
			}
			if ((uint32_t)SR_test00_1_dst_u0_data[0].test00_1_dst_v1_data != ((uint32_t)0x00111ffb66693520LL)) {
				HtAssert(0, 0);
			}
			if ((uint32_t)PR3_test00_2_dst_v0_data[1] != ((uint32_t)0x001d7f229c1c49a0LL)) {
				HtAssert(0, 0);
			}
			if ((uint16_t)GR3_test00_3_dst_s0_data[0].test00_3_dst_v1_data[0][0] != ((uint16_t)0x000cec97f4c84740LL)) {
				HtAssert(0, 0);
			}
			if ((uint64_t)PR3_test00_4_dst_s0_data[1].test00_4_dst_v0_data[2][1] != ((uint64_t)0x000beba9f75cc5c0LL)) {
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
