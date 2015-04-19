#include "Ht.h"
#include "PersTest00.h"

void CPersTest00::PersTest00() {
	if (PR_htValid) {
		switch (PR_htInst) {
		case TEST00_ENTRY: {
			HtContinue(TEST00_WR);
			break;
		}
		case TEST00_WR: {
			GW_test00_g_0_0_0_data = (uint16_t)0x0015c20462c8ce60LL;
			P_test00_p_0_2_0_data[0] = (uint16_t)0x001e2a75cc536260LL;
			S_test00_s_1_4_0_data.write_addr(1);
			S_test00_s_1_4_0_data.write_mem((uint32_t)0x001fff1d35a161a0LL);
			GW_test00_g_0_7_0_data[1].write_addr(4);
			GW_test00_g_0_7_0_data[1] = (uint8_t)0x00155ea1c93695a0LL;
			P_test00_g_0_7_0_data_RdAddr = (ht_uint3)4;
			HtContinue(TEST00_ST0);
			break;
		}
		case TEST00_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_uint16_t(PR_memAddr + 0, GR_test00_g_0_0_0_data);
			HtContinue(TEST00_ST1);
			break;
		}
		case TEST00_ST1: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test00_p_0_2_0_data(PR_memAddr + 32);
			HtContinue(TEST00_ST2);
			break;
		}
		case TEST00_ST2: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test00_s_1_4_0_data(PR_memAddr + 64, 1, 1);
			HtContinue(TEST00_ST3);
			break;
		}
		case TEST00_ST3: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_uint8_t(PR_memAddr + 96, GR_test00_g_0_7_0_data[1]);
			WriteMemPause(TEST00_LD0);
			break;
		}
		case TEST00_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_g_0_1_0_data(PR_memAddr + 0);
			HtContinue(TEST00_LD1);
			break;
		}
		case TEST00_LD1: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_p_0_3_0_data(PR_memAddr + 32);
			HtContinue(TEST00_LD2);
			break;
		}
		case TEST00_LD2: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_g_0_6_0_data(PR_memAddr + 64);
			HtContinue(TEST00_LD3);
			break;
		}
		case TEST00_LD3: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_p_0_8_0_data(PR_memAddr + 96, 1, 1);
			ReadMemPause(TEST00_CHK);
			break;
		}
		case TEST00_CHK: {
			if (GR_test00_g_0_1_0_data != (uint16_t)0x0015c20462c8ce60LL) {
				HtAssert(0, (uint32_t)0x00000000);
			}
			if (PR_test00_p_0_3_0_data[1] != (uint16_t)0x001e2a75cc536260LL) {
				HtAssert(0, (uint32_t)0x00000001);
			}
			if (GR_test00_g_0_6_0_data != (uint32_t)0x001fff1d35a161a0LL) {
				HtAssert(0, (uint32_t)0x00000002);
			}
			if (P_test00_p_0_8_0_data[1] != (uint8_t)0x00155ea1c93695a0LL) {
				HtAssert(0, (uint32_t)0x00000003);
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
