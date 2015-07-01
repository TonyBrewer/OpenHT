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
			S_test00_0_dst_s0_data.test00_0_dst_s1_data.test00_0_dst_s2_data.test00_0_dst_v1_data = ((int32_t)0x0002daa2633de3e0LL);
			S_test00_0_dst_s0_data.test00_0_dst_s1_data.test00_0_dst_s2_data.test00_0_dst_v2_data[0][0] = ((int32_t)0x0000ba052338b540LL);
			S_test00_0_dst_s0_data.test00_0_dst_s1_data.test00_0_dst_s2_data.test00_0_dst_v2_data[0][1] = ((int32_t)0x0000ba052338b540LL);
			S_test00_0_dst_s0_data.test00_0_dst_s1_data.test00_0_dst_s2_data.test00_0_dst_v2_data[0][2] = ((int32_t)0x0000ba052338b540LL);
			S_test00_0_dst_s0_data.test00_0_dst_s1_data.test00_0_dst_s2_data.test00_0_dst_v2_data[0][3] = ((int32_t)0x0000ba052338b540LL);
			S_test00_0_dst_s0_data.test00_0_dst_s1_data.test00_0_dst_s2_data.test00_0_dst_v3_data = ((uint32_t)0x00194f52bcc93520LL);
			HtContinue(TEST00_ST0);
			break;
		}
		case TEST00_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test00_0_dst_s2_data_struct(PR_memAddr + 0, SR_test00_0_dst_s0_data.test00_0_dst_s1_data.test00_0_dst_s2_data);
			WriteMemPause(TEST00_LD0);
			break;
		}
		case TEST00_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_0_dst_s2_data(PR_memAddr + 0);
			ReadMemPause(TEST00_CHK0);
			break;
		}
		case TEST00_CHK0: {
			if ((int32_t)S_test00_0_dst_s0_data.test00_0_dst_s1_data.test00_0_dst_s2_data.test00_0_dst_v1_data != ((int32_t)0x0002daa2633de3e0LL)) {
				printf("EXP: 0x%08x\n", (int32_t)0x0002daa2633de3e0LL);
				printf("ACT: 0x%08x\n\n", (int32_t)S_test00_0_dst_s0_data.test00_0_dst_s1_data.test00_0_dst_s2_data.test00_0_dst_v1_data);
				HtAssert(0, 0);
			}
			if ((int32_t)SR_test00_0_dst_s0_data.test00_0_dst_s1_data.test00_0_dst_s2_data.test00_0_dst_v2_data[0][0] != ((int32_t)0x0000ba052338b540LL)) {
				HtAssert(0, 0);
			}
			if ((int32_t)S_test00_0_dst_s0_data.test00_0_dst_s1_data.test00_0_dst_s2_data.test00_0_dst_v2_data[0][1] != ((int32_t)0x0000ba052338b540LL)) {
				HtAssert(0, 0);
			}
			if ((int32_t)SR_test00_0_dst_s0_data.test00_0_dst_s1_data.test00_0_dst_s2_data.test00_0_dst_v2_data[0][2] != ((int32_t)0x0000ba052338b540LL)) {
				HtAssert(0, 0);
			}
			if ((int32_t)S_test00_0_dst_s0_data.test00_0_dst_s1_data.test00_0_dst_s2_data.test00_0_dst_v2_data[0][3] != ((int32_t)0x0000ba052338b540LL)) {
				HtAssert(0, 0);
			}
			if ((uint32_t)S_test00_0_dst_s0_data.test00_0_dst_s1_data.test00_0_dst_s2_data.test00_0_dst_v3_data != ((uint32_t)0x00194f52bcc93520LL)) {
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
