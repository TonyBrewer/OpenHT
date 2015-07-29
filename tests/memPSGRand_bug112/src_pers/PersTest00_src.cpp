#include "Ht.h"
#include "PersTest00.h"

void CPersTest00::PersTest00() {
	if (PR1_htValid) {
		switch (PR1_htInst) {
		case TEST00_ENTRY: {
			HtContinue(TEST00_WR0);
			break;
		}
		case TEST00_WR0: {
			GW1_test00_0_dst_s0_data[0][0].write_addr(14, 13);
			GW1_test00_0_dst_s0_data[0][0].test00_0_dst_v0_data[0][0] = ((uint8_t)0x001af188b6cf7780LL);
			GW1_test00_0_dst_s0_data[0][0].test00_0_dst_v0_data[1][0] = ((uint8_t)0x001af188b6cf7780LL);
			GW1_test00_0_dst_s0_data[0][0].test00_0_dst_v0_data[2][0] = ((uint8_t)0x001af188b6cf7780LL);
			GW1_test00_0_dst_s0_data[0][0].test00_0_dst_v1_data = ((uint32_t)0x001de80faced6660LL);
			GW1_test00_0_dst_s0_data[0][0].test00_0_dst_v2_data = ((int64_t)0x0002809850655dc0LL);
			HtContinue(TEST00_ST0);
			break;
		}
		case TEST00_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test00_0_dst_s0_data(PR1_memAddr + 0, 0, 0, 1);
			WriteMemPause(TEST00_LD0);
			break;
		}
		case TEST00_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_0_dst_s0_data(PR1_memAddr + 0, 6, 5, 0, 1);
			ReadMemPause(TEST00_CHK0);
			break;
		}
		case TEST00_CHK0: {
			if (GR1_test00_0_dst_s0_data[0][0].test00_0_dst_v0_data[0][0] != ((uint8_t)0x001af188b6cf7780LL)) {
				HtAssert(0, 0);
			}
			if (GR1_test00_0_dst_s0_data[0][0].test00_0_dst_v0_data[1][0] != ((uint8_t)0x001af188b6cf7780LL)) {
				HtAssert(0, 0);
			}
			if (GR1_test00_0_dst_s0_data[0][0].test00_0_dst_v0_data[2][0] != ((uint8_t)0x001af188b6cf7780LL)) {
				HtAssert(0, 0);
			}
			if (GR1_test00_0_dst_s0_data[0][0].test00_0_dst_v1_data != ((uint32_t)0x001de80faced6660LL)) {
				HtAssert(0, 0);
			}
			if (GR1_test00_0_dst_s0_data[0][0].test00_0_dst_v2_data != ((int64_t)0x0002809850655dc0LL)) {
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
	if (PR2_htValid) {
		switch (PR2_htInst) {
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
			P2_test00_0_dst_s0_data_RdAddr1 = (ht_uint4)6;
			P2_test00_0_dst_s0_data_RdAddr2 = (ht_uint5)5;
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
