#include "Ht.h"
#include "PersTest00.h"

void CPersTest00::PersTest00() {
	if (PR3_htValid) {
		switch (PR3_htInst) {
		case TEST00_ENTRY: {
			break;
		}
		case TEST00_WR0: {
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
		case TEST00_CHK0: {
			if (GR3_test00_2_dst_v0_data != ((uint32_t)0x001ddbf10abe48e0LL)) {
				HtAssert(0, 0);
			}
			break;
		}
		case TEST00_RTN: {
			break;
		}
		default:
			assert(0);
		}
	}
	if (PR4_htValid) {
		switch (PR4_htInst) {
		case TEST00_ENTRY: {
			break;
		}
		case TEST00_WR0: {
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
		case TEST00_CHK0: {
			SR_test00_3_dst_v0_data.read_addr(1, 7);
			S_test00_4_src_v0_data.read_addr(5, 3);
			break;
		}
		case TEST00_RTN: {
			break;
		}
		default:
			assert(0);
		}
	}
	if (PR5_htValid) {
		switch (PR5_htInst) {
		case TEST00_ENTRY: {
			HtContinue(TEST00_WR0);
			break;
		}
		case TEST00_WR0: {
			GW5_test00_0_src_v0_data[2] = ((ht_int26)0x000f413fb40b4b80LL);
			GW5_test00_1_src_v0_data.write_addr(1);
			GW5_test00_1_src_v0_data = ((uint64_t)0x00189eefce3afa80LL);
			PW5_test00_2_src_v0_data.write_addr(7);
			PW5_test00_2_src_v0_data = ((uint32_t)0x001ddbf10abe48e0LL);
			S_test00_3_src_v0_data = ((ht_int50)0x001ecc361e543ba0LL);
			S_test00_4_src_v0_data.write_addr(14, 4);
			S_test00_4_src_v0_data.write_mem((ht_int19)0x000dba5b1c61b380LL);
			P5_test00_1_src_v0_data_RdAddr1 = (ht_uint2)1;
			HtContinue(TEST00_ST0);
			break;
		}
		case TEST00_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_ht_int26(PR5_memAddr + 0, GR5_test00_0_src_v0_data[2]);
			HtContinue(TEST00_ST1);
			break;
		}
		case TEST00_ST1: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_uint64_t(PR5_memAddr + 64, GR5_test00_1_src_v0_data);
			HtContinue(TEST00_ST2);
			break;
		}
		case TEST00_ST2: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test00_2_src_v0_data(PR5_memAddr + 128, 7);
			HtContinue(TEST00_ST3);
			break;
		}
		case TEST00_ST3: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test00_3_src_v0_data(PR5_memAddr + 192);
			HtContinue(TEST00_ST4);
			break;
		}
		case TEST00_ST4: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test00_4_src_v0_data(PR5_memAddr + 256, 14, 4, 1);
			WriteMemPause(TEST00_LD0);
			break;
		}
		case TEST00_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_0_dst_v0_data(PR5_memAddr + 0, 0, 0, 0, 1);
			P5_test00_0_dst_v0_data_RdAddr1 = (ht_uint5)22;
			P5_test00_0_dst_v0_data_RdAddr2 = (ht_uint1)0;
			HtContinue(TEST00_LD1);
			break;
		}
		case TEST00_LD1: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_1_dst_v0_data(PR5_memAddr + 64, 1);
			P5_test00_1_dst_v0_data_RdAddr1 = (ht_uint3)1;
			HtContinue(TEST00_LD2);
			break;
		}
		case TEST00_LD2: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_2_dst_v0_data(PR5_memAddr + 128);
			HtContinue(TEST00_LD3);
			break;
		}
		case TEST00_LD3: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_3_dst_v0_data(PR5_memAddr + 192, 1, 7, 1);
			HtContinue(TEST00_LD4);
			break;
		}
		case TEST00_LD4: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_4_src_v0_data(PR5_memAddr + 256);
			ReadMemPause(TEST00_CHK0);
			break;
		}
		case TEST00_CHK0: {
			if (PR5_test00_0_dst_v0_data[0][0] != ((ht_int26)0x000f413fb40b4b80LL)) {
				HtAssert(0, 0);
			}
			if (PR5_test00_1_dst_v0_data != ((uint64_t)0x00189eefce3afa80LL)) {
				HtAssert(0, 0);
			}
			if (SR_test00_3_dst_v0_data.read_mem() != ((ht_int50)0x001ecc361e543ba0LL)) {
				HtAssert(0, 0);
			}
			if (S_test00_4_src_v0_data.read_mem() != ((ht_int19)0x000dba5b1c61b380LL)) {
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
