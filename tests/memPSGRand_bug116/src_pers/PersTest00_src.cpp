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
			PW_test00_0_dst_u0_data.write_addr(0);
			PW_test00_0_dst_u0_data.test00_0_dst_v0_data[0][0] = ((int32_t)0x0015f6080d6fd5c0LL);
			PW_test00_0_dst_u0_data.test00_0_dst_v0_data[0][1] = ((int32_t)0x0015f6080d6fd5c0LL);
			PW_test00_0_dst_u0_data.test00_0_dst_v0_data[0][2] = ((int32_t)0x0015f6080d6fd5c0LL);
			PW_test00_0_dst_u0_data.test00_0_dst_v0_data[0][3] = ((int32_t)0x0015f6080d6fd5c0LL);
			PW_test00_0_dst_u0_data.test00_0_dst_v0_data[0][4] = ((int32_t)0x0015f6080d6fd5c0LL);
			PW_test00_0_dst_u0_data.test00_0_dst_v0_data[1][0] = ((int32_t)0x0015f6080d6fd5c0LL);
			PW_test00_0_dst_u0_data.test00_0_dst_v0_data[1][1] = ((int32_t)0x0015f6080d6fd5c0LL);
			PW_test00_0_dst_u0_data.test00_0_dst_v0_data[1][2] = ((int32_t)0x0015f6080d6fd5c0LL);
			PW_test00_0_dst_u0_data.test00_0_dst_v0_data[1][3] = ((int32_t)0x0015f6080d6fd5c0LL);
			PW_test00_0_dst_u0_data.test00_0_dst_v0_data[1][4] = ((int32_t)0x0015f6080d6fd5c0LL);
			PW_test00_0_dst_u0_data.test00_0_dst_v0_data[2][0] = ((int32_t)0x0015f6080d6fd5c0LL);
			PW_test00_0_dst_u0_data.test00_0_dst_v0_data[2][1] = ((int32_t)0x0015f6080d6fd5c0LL);
			PW_test00_0_dst_u0_data.test00_0_dst_v0_data[2][2] = ((int32_t)0x0015f6080d6fd5c0LL);
			PW_test00_0_dst_u0_data.test00_0_dst_v0_data[2][3] = ((int32_t)0x0015f6080d6fd5c0LL);
			PW_test00_0_dst_u0_data.test00_0_dst_v0_data[2][4] = ((int32_t)0x0015f6080d6fd5c0LL);
			HtContinue(TEST00_ST0);
			break;
		}
		case TEST00_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test00_0_dst_u0_data(PR_memAddr + 0, 0);
			WriteMemPause(TEST00_LD0);
			break;
		}
		case TEST00_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_0_dst_u0_data(PR_memAddr + 0, 0);
			P_test00_0_dst_u0_data_RdAddr1 = (ht_uint1)0;
			ReadMemPause(TEST00_CHK0);
			break;
		}
		case TEST00_CHK0: {
			if (PR_test00_0_dst_u0_data.test00_0_dst_v0_data[0][0] != ((int32_t)(ht_int32)0x000000000d6fd5c0LL)) {
				HtAssert(0, 0);
			}
			if (PR_test00_0_dst_u0_data.test00_0_dst_v0_data[0][1] != ((int32_t)(ht_int32)0x000000000d6fd5c0LL)) {
				HtAssert(0, 0);
			}
			if (PR_test00_0_dst_u0_data.test00_0_dst_v0_data[0][2] != ((int32_t)(ht_int32)0x000000000d6fd5c0LL)) {
				HtAssert(0, 0);
			}
			if (PR_test00_0_dst_u0_data.test00_0_dst_v0_data[0][3] != ((int32_t)(ht_int32)0x000000000d6fd5c0LL)) {
				HtAssert(0, 0);
			}
			if (PR_test00_0_dst_u0_data.test00_0_dst_v0_data[0][4] != ((int32_t)(ht_int32)0x000000000d6fd5c0LL)) {
				HtAssert(0, 0);
			}
			if (PR_test00_0_dst_u0_data.test00_0_dst_v0_data[1][0] != ((int32_t)(ht_int32)0x000000000d6fd5c0LL)) {
				HtAssert(0, 0);
			}
			if (PR_test00_0_dst_u0_data.test00_0_dst_v0_data[1][1] != ((int32_t)(ht_int32)0x000000000d6fd5c0LL)) {
				HtAssert(0, 0);
			}
			if (PR_test00_0_dst_u0_data.test00_0_dst_v0_data[1][2] != ((int32_t)(ht_int32)0x000000000d6fd5c0LL)) {
				HtAssert(0, 0);
			}
			if (PR_test00_0_dst_u0_data.test00_0_dst_v0_data[1][3] != ((int32_t)(ht_int32)0x000000000d6fd5c0LL)) {
				HtAssert(0, 0);
			}
			if (PR_test00_0_dst_u0_data.test00_0_dst_v0_data[1][4] != ((int32_t)(ht_int32)0x000000000d6fd5c0LL)) {
				HtAssert(0, 0);
			}
			if (PR_test00_0_dst_u0_data.test00_0_dst_v0_data[2][0] != ((int32_t)(ht_int32)0x000000000d6fd5c0LL)) {
				HtAssert(0, 0);
			}
			if (PR_test00_0_dst_u0_data.test00_0_dst_v0_data[2][1] != ((int32_t)(ht_int32)0x000000000d6fd5c0LL)) {
				HtAssert(0, 0);
			}
			if (PR_test00_0_dst_u0_data.test00_0_dst_v0_data[2][2] != ((int32_t)(ht_int32)0x000000000d6fd5c0LL)) {
				HtAssert(0, 0);
			}
			if (PR_test00_0_dst_u0_data.test00_0_dst_v0_data[2][3] != ((int32_t)(ht_int32)0x000000000d6fd5c0LL)) {
				HtAssert(0, 0);
			}
			if (PR_test00_0_dst_u0_data.test00_0_dst_v0_data[2][4] != ((int32_t)(ht_int32)0x000000000d6fd5c0LL)) {
				HtAssert(0, 0);
			}
			if (PR_test00_0_dst_u0_data.test00_0_dst_v1_data != ((uint32_t)(ht_uint32)0x000000000d6fd5c0LL)) {
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
