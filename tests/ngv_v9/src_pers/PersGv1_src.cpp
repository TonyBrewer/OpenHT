#include "Ht.h"
#include "PersGv1.h"

void CPersGv1::PersGv1()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case GV1_ENTRY:
		{
			GW_gvar1.write_addr(P_gvAddr);
			GW_gvar1.m_a = (char)(0x40 + P_loopCnt);
			GW_gvar1.m_b[0] = 0x4001 + P_loopCnt;
			GW_gvar1.m_b[1] = 0x4002 + P_loopCnt;
			GW_gvar1.m_c[0].m_s = (char)(0x41 + P_loopCnt);
			GW_gvar1.m_c[1].m_sa = 0;
			GW_gvar1.m_c[1].m_sb = 5;
			GW_gvar1.m_c[1].m_sc = 2;
			GW_gvar1.m_c[2].m_s = 0;
			GW_gvar1.m_d = 0x40000000 + P_loopCnt;
			for (int i = 0; i < 12; i += 1)
				GW_gvar1.m_e[i] = 0x4000000000000000LL + P_loopCnt + i;

			P_gvAddr += 1;
			P_loopCnt += 1;
			HtContinue(P_loopCnt < 8 ? GV1_ENTRY : GV1_WRITE1);
			break;
		}
		case GV1_WRITE1:
		{
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}

			WriteMem_mwgv3(PR_addr, 0, 5);

			P_addr += 5 * 14 * 8;
			P_gvAddr = 5;

			HtContinue(GV1_WRITE2);
			break;
		}
		case GV1_WRITE2:
		{
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}

			WriteMem_CGVar(PR_addr, GR_gvar1);

			P_addr += 1 * 14 * 8;

			WriteReqPause(GV1_WRITE3);
			break;
		}
		case GV1_WRITE3:
		{
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}

			WriteMem_mwgv3(PR_addr, 6, 2);

			P_addr += 2 * 14 * 8;

			WriteMemPause(GV1_RETURN);
		}
			break;
		case GV1_RETURN:
		{
			if (SendReturnBusy_gv1()) {
				HtRetry();
				break;
			}

			SendReturn_gv1();
		}
		break;
		default:
			assert(0);
		}
	}
}
