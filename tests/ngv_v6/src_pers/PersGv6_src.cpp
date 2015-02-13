#include "Ht.h"
#include "PersGv6.h"

void CPersGv6::PersGv6()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case GV6_ENTRY:
		{
			P_x = PR_htId;

			// D_2x2x2x
			GW_gv_R_2x2x2x_1F_m0[PR_htId][0].m_x = P_x;

			GW_gv_R_2x2x2x_1F_m1[PR_htId][0].m_x = P_x;

			GW_gv_R_2x2x2x_2F_m0[PR_htId].m_x[0] = P_x;

			GW_gv_R_2x2x2x_2F_m1[PR_htId].m_x[0] = P_x;

			GW_gv_R_2x2x2x_1F_AI_m0[PR_htId][0].m_x = P_x;

			GW_gv_R_2x2x2x_1F_AI_m1[PR_htId][0].m_x = P_x;

			GW_gv_R_2x2x2x_2F_AI_m0[PR_htId].m_x[0] = P_x;

			GW_gv_R_2x2x2x_2F_AI_m1[PR_htId].m_x[0] = P_x;

			GW_gv_R_2x2x2x_1F_AA_m0[PR_htId][0].m_x = P_x;

			GW_gv_R_2x2x2x_1F_AA_m1[PR_htId][0].m_x = P_x;

			GW_gv_R_2x2x2x_4F_AA_m0[PR_htId].m_x[0] = P_x;

			GW_gv_R_2x2x2x_4F_AA_m1[PR_htId].m_x[0] = P_x;

			P_loopCnt = 0;
			HtContinue(GV6_LOOP);
		}
			break;
		case GV6_LOOP:
		{
			// D_2x2x2x
			if (GR_gv_R_2x2x2x_1F_m0[PR_htId][0].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_2x2x2x_1F_m1[PR_htId][0].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_2x2x2x_2F_m0[PR_htId].m_x[0] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_2x2x2x_2F_m1[PR_htId].m_x[0] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_2x2x2x_1F_AI_m0[PR_htId][0].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_2x2x2x_1F_AI_m1[PR_htId][0].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_2x2x2x_2F_AI_m0[PR_htId].m_x[0] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_2x2x2x_2F_AI_m1[PR_htId].m_x[0] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_2x2x2x_1F_AA_m0[PR_htId][0].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_2x2x2x_1F_AA_m1[PR_htId][0].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_2x2x2x_4F_AA_m0[PR_htId].m_x[0] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_2x2x2x_4F_AA_m1[PR_htId].m_x[0] != PR_x) HtAssert(0, 0);

			P_x += 1;

			// D_2x2x2x
			GW_gv_R_2x2x2x_1F_m0[PR_htId][0].m_x = P_x;

			GW_gv_R_2x2x2x_1F_m1[PR_htId][0].m_x = P_x;

			GW_gv_R_2x2x2x_2F_m0[PR_htId].m_x[0] = P_x;

			GW_gv_R_2x2x2x_2F_m1[PR_htId].m_x[0] = P_x;

			GW_gv_R_2x2x2x_1F_AI_m0[PR_htId][0].m_x.AtomicInc();

			GW_gv_R_2x2x2x_1F_AI_m1[PR_htId][0].m_x.AtomicInc();

			GW_gv_R_2x2x2x_2F_AI_m0[PR_htId].m_x[0].AtomicInc();

			GW_gv_R_2x2x2x_2F_AI_m1[PR_htId].m_x[0].AtomicInc();

			GW_gv_R_2x2x2x_1F_AA_m0[PR_htId][0].m_x.AtomicAdd(1u);

			GW_gv_R_2x2x2x_1F_AA_m1[PR_htId][0].m_x = P_x;

			GW_gv_R_2x2x2x_4F_AA_m0[PR_htId].m_x[0].AtomicAdd(1u);

			GW_gv_R_2x2x2x_4F_AA_m1[PR_htId].m_x[0] = P_x;


			HtContinue(P_loopCnt++ < 100 ? GV6_LOOP : GV6_RETURN);
		}
			break;
		case GV6_RETURN:
		{
			if (SendReturnBusy_gv6()) {
				HtRetry();
				break;
			}

			SendReturn_gv6();
		}
			break;
		default:
			assert(0);
		}
	}
}
