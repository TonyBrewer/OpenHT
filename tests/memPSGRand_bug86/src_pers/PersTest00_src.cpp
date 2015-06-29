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
			GW_test00_0_src_v0_data = ((ht_int46)0x0006b393a747ca00LL);
			S_test00_1_src_v0_data[1][0] = ((uint16_t)0x001f13af7ec9e000LL);
			GW_test00_2_src_v0_data = ((uint8_t)0x0010d29bf3767a40LL);
			P_test00_3_src_v0_data = ((int32_t)0x0004f165caea4040LL);
			PW_test00_4_src_v0_data[1].write_addr(1);
			PW_test00_4_src_v0_data[1] = ((ht_uint49)0x001da09b75ca4ac0LL);
			P_test00_4_src_v0_data_RdAddr1 = (ht_uint3)1;
			HtContinue(TEST00_ST0);
			break;
		}
		case TEST00_ST0: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_test00_0_src_v0_data(PR_memAddr + 0);
			HtContinue(TEST00_ST1);
			break;
		}
		case TEST00_ST1: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_uint16_t(PR_memAddr + 64, SR_test00_1_src_v0_data[1][0]);
			HtContinue(TEST00_ST2);
			break;
		}
		case TEST00_ST2: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_uint8_t(PR_memAddr + 128, GR_test00_2_src_v0_data);
			HtContinue(TEST00_ST3);
			break;
		}
		case TEST00_ST3: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_int32_t(PR_memAddr + 192, P_test00_3_src_v0_data);
			HtContinue(TEST00_ST4);
			break;
		}
		case TEST00_ST4: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_ht_uint49(PR_memAddr + 256, PR_test00_4_src_v0_data[1]);
			WriteMemPause(TEST00_LD0);
			break;
		}
		case TEST00_LD0: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_0_dst_v0_data(PR_memAddr + 0, 1, 1);
			HtContinue(TEST00_LD1);
			break;
		}
		case TEST00_LD1: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_1_dst_v0_data(PR_memAddr + 64, 0, 0, 0, 1);
			HtContinue(TEST00_LD2);
			break;
		}
		case TEST00_LD2: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_2_dst_v0_data(PR_memAddr + 128);
			HtContinue(TEST00_LD3);
			break;
		}
		case TEST00_LD3: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_3_dst_v0_data(PR_memAddr + 192);
			HtContinue(TEST00_LD4);
			break;
		}
		case TEST00_LD4: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_test00_4_src_v0_data(PR_memAddr + 256, 5, 2, 1);
			P_test00_4_src_v0_data_RdAddr1 = (ht_uint3)5;
			ReadMemPause(TEST00_CHK0);
			break;
		}
		case TEST00_CHK0: {
			S_test00_0_dst_v0_data.read_addr(1);
			if ((ht_int46)SR_test00_0_dst_v0_data.read_mem() != ((ht_int46)0x0006b393a747ca00LL)) {
				HtAssert(0, 0);
			}
			if ((uint16_t)SR_test00_1_dst_u0_data[0].test00_1_dst_s0_data.test00_1_dst_v0_data[0][0] != ((uint16_t)0x001f13af7ec9e000LL)) {
				HtAssert(0, 0);
			}
			if ((uint8_t)GR_test00_2_dst_v0_data != ((uint8_t)0x0010d29bf3767a40LL)) {
				HtAssert(0, 0);
			}
			if ((int32_t)GR_test00_3_dst_v0_data != ((int32_t)0x0004f165caea4040LL)) {
				HtAssert(0, 0);
			}
			if ((ht_uint49)PR_test00_4_src_v0_data[2] != ((ht_uint49)0x001da09b75ca4ac0LL)) {
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
