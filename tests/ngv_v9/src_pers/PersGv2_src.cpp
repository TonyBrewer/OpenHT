#include "Ht.h"
#include "PersGv2.h"

void CPersGv2::PersGv2()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case GV2_ENTRY:
		{
			P_gvAddr = 0;
			P_errCnt = 0;
			P_loopCnt = 0;

			HtContinue(GV2_READ1);
			break;
		}
		case GV2_READ1:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			ReadMem_mrgv(PR_addr, 0, 5);

			P_addr += 5 * 14 * 8;

			HtContinue(GV2_READ2);
			break;
		}
		case GV2_READ2:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			ReadMem_mrgv(PR_addr, 5, 1);

			P_addr += 1 * 14 * 8;

			HtContinue(GV2_READ3);
			break;
		}
		case GV2_READ3:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			ReadMem_mrgv(PR_addr, 6, 2);

			P_addr += 2 * 14 * 8;

			ReadMemPause(GV2_LOOP);
			break;
		}
		case GV2_LOOP:
		{
			if (GR_gvar2.m_a != (char)(0x40 + P_loopCnt + PR_htId))
				P_errCnt += 1;
			if (GR_gvar2.m_b[0] != (short)(0x4001 + P_loopCnt + PR_htId))
				P_errCnt += 1;
			if (GR_gvar2.m_b[1] != (short)(0x4002 + P_loopCnt + PR_htId))
				P_errCnt += 1;
			if (GR_gvar2.m_c[0].m_s != (char)(0x41 + P_loopCnt + PR_htId))
				P_errCnt += 1;
			if (GR_gvar2.m_c[1].m_sa != (ht_int1)(0 + PR_htId))
				P_errCnt += 1;
			if (GR_gvar2.m_c[1].m_sb != (ht_int3)(5 + PR_htId))
				P_errCnt += 1;
			if (GR_gvar2.m_c[1].m_sc != (ht_int2)(2 + PR_htId))
				P_errCnt += 1;
			if (GR_gvar2.m_c[2].m_s != (char)(0 + PR_htId))
				P_errCnt += 1;
			if (GR_gvar2.m_d != 0x40000000 + P_loopCnt + PR_htId)
				P_errCnt += 1;
			for (int i = 0; i < 12; i += 1) {
				if (GR_gvar2.m_e[i] != 0x4000000000000000LL + P_loopCnt + i + PR_htId)
					P_errCnt += 1;
			}

			P_gvAddr += 1;
			P_loopCnt += 1;
			HtContinue(P_loopCnt < 8 ? GV2_LOOP : GV2_RETURN);
			break;
		}
		case GV2_RETURN:
		{
			if (SendReturnBusy_gv2()) {
				HtRetry();
				break;
			}

			SendReturn_gv2(P_errCnt);
		}
		break;
		default:
			assert(0);
		}
	}
}
