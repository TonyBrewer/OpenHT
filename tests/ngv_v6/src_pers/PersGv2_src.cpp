#include "Ht.h"
#include "PersGv2.h"

void CPersGv2::PersGv2()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case GV2_ENTRY:
		{
			P_x = PR_htId;

			// D_1x2x
			GW_gv_R_1x2x_1F_m0[PR_htId][1].m_x = P_x;

			GW_gv_R_1x2x_1F_m1[PR_htId][1].m_x = P_x;

			GW_gv_R_1x2x_2F_m0[PR_htId].m_x[1] = P_x;

			GW_gv_R_1x2x_2F_m1[PR_htId].m_x[1] = P_x;

			GW_gv_R_1x2x_1F_AI_m0[PR_htId][1].m_x = P_x;

			GW_gv_R_1x2x_1F_AI_m1[PR_htId][1].m_x = P_x;

			GW_gv_R_1x2x_2F_AI_m0[PR_htId].m_x[1] = P_x;

			GW_gv_R_1x2x_2F_AI_m1[PR_htId].m_x[1] = P_x;

			GW_gv_R_1x2x_1F_AA_m0[PR_htId][1].m_x = P_x;

			GW_gv_R_1x2x_1F_AA_m1[PR_htId][1].m_x = P_x;

			GW_gv_R_1x2x_4F_AA_m0[PR_htId].m_x[1] = P_x;

			GW_gv_R_1x2x_4F_AA_m1[PR_htId].m_x[1] = P_x;

			// D_1x1x
			GW_gv_R_1x1x_1F_m0[PR_htId][1].m_x = P_x;

			GW_gv_R_1x1x_1F_m1[PR_htId][1].m_x = P_x;

			GW_gv_R_1x1x_2F_m0[PR_htId].m_x[1] = P_x;

			GW_gv_R_1x1x_2F_m1[PR_htId].m_x[1] = P_x;

			GW_gv_R_1x1x_1F_AI_m0[PR_htId][1].m_x = P_x;

			GW_gv_R_1x1x_1F_AI_m1[PR_htId][1].m_x = P_x;

			GW_gv_R_1x1x_2F_AI_m0[PR_htId].m_x[1] = P_x;

			GW_gv_R_1x1x_2F_AI_m1[PR_htId].m_x[1] = P_x;

			GW_gv_R_1x1x_1F_AA_m0[PR_htId][1].m_x = P_x;

			GW_gv_R_1x1x_1F_AA_m1[PR_htId][1].m_x = P_x;

			GW_gv_R_1x1x_4F_AA_m0[PR_htId].m_x[1] = P_x;

			GW_gv_R_1x1x_4F_AA_m1[PR_htId].m_x[1] = P_x;

			// D_1x1x1x
			GW_gv_R_1x1x1x_1F_m0[PR_htId][1].m_x = P_x;

			GW_gv_R_1x1x1x_1F_m1[PR_htId][1].m_x = P_x;

			GW_gv_R_1x1x1x_2F_m0[PR_htId].m_x[1] = P_x;

			GW_gv_R_1x1x1x_2F_m1[PR_htId].m_x[1] = P_x;

			GW_gv_R_1x1x1x_1F_AI_m0[PR_htId][1].m_x = P_x;

			GW_gv_R_1x1x1x_1F_AI_m1[PR_htId][1].m_x = P_x;

			GW_gv_R_1x1x1x_2F_AI_m0[PR_htId].m_x[1] = P_x;

			GW_gv_R_1x1x1x_2F_AI_m1[PR_htId].m_x[1] = P_x;

			GW_gv_R_1x1x1x_1F_AA_m0[PR_htId][1].m_x = P_x;

			GW_gv_R_1x1x1x_1F_AA_m1[PR_htId][1].m_x = P_x;

			GW_gv_R_1x1x1x_4F_AA_m0[PR_htId].m_x[1] = P_x;

			GW_gv_R_1x1x1x_4F_AA_m1[PR_htId].m_x[1] = P_x;

			// D_1x1x2x2x
			GW_gv_R_1x1x2x2x_1F_m0[PR_htId][1].m_x = P_x;

			GW_gv_R_1x1x2x2x_1F_m1[PR_htId][1].m_x = P_x;

			GW_gv_R_1x1x2x2x_2F_m0[PR_htId].m_x[1] = P_x;

			GW_gv_R_1x1x2x2x_2F_m1[PR_htId].m_x[1] = P_x;

			GW_gv_R_1x1x2x2x_1F_AI_m0[PR_htId][1].m_x = P_x;

			GW_gv_R_1x1x2x2x_1F_AI_m1[PR_htId][1].m_x = P_x;

			GW_gv_R_1x1x2x2x_2F_AI_m0[PR_htId].m_x[1] = P_x;

			GW_gv_R_1x1x2x2x_2F_AI_m1[PR_htId].m_x[1] = P_x;

			GW_gv_R_1x1x2x2x_1F_AA_m0[PR_htId][1].m_x = P_x;

			GW_gv_R_1x1x2x2x_1F_AA_m1[PR_htId][1].m_x = P_x;

			GW_gv_R_1x1x2x2x_4F_AA_m0[PR_htId].m_x[1] = P_x;

			GW_gv_R_1x1x2x2x_4F_AA_m1[PR_htId].m_x[1] = P_x;

			P_loopCnt = 0;
			HtContinue(GV2_LOOP);
			break;
		}
		case GV2_LOOP:
		{
			// D_1x2x
			if (GR_gv_R_1x2x_1F_m0[PR_htId][1].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x2x_1F_m1[PR_htId][1].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x2x_2F_m0[PR_htId].m_x[1] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x2x_2F_m1[PR_htId].m_x[1] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x2x_1F_AI_m0[PR_htId][1].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x2x_1F_AI_m1[PR_htId][1].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x2x_2F_AI_m0[PR_htId].m_x[1] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x2x_2F_AI_m1[PR_htId].m_x[1] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x2x_1F_AA_m0[PR_htId][1].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x2x_1F_AA_m1[PR_htId][1].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x2x_4F_AA_m0[PR_htId].m_x[1] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x2x_4F_AA_m1[PR_htId].m_x[1] != PR_x) HtAssert(0, 0);

			// D_1x1x
			if (GR_gv_R_1x1x_1F_m0[PR_htId][1].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x_1F_m1[PR_htId][1].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x_2F_m0[PR_htId].m_x[1] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x_2F_m1[PR_htId].m_x[1] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x_1F_AI_m0[PR_htId][1].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x_1F_AI_m1[PR_htId][1].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x_2F_AI_m0[PR_htId].m_x[1] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x_2F_AI_m1[PR_htId].m_x[1] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x_1F_AA_m0[PR_htId][1].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x_1F_AA_m1[PR_htId][1].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x_4F_AA_m0[PR_htId].m_x[1] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x_4F_AA_m1[PR_htId].m_x[1] != PR_x) HtAssert(0, 0);

			// D_1x1x1x
			if (GR_gv_R_1x1x1x_1F_m0[PR_htId][1].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x1x_1F_m1[PR_htId][1].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x1x_2F_m0[PR_htId].m_x[1] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x1x_2F_m1[PR_htId].m_x[1] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x1x_1F_AI_m0[PR_htId][1].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x1x_1F_AI_m1[PR_htId][1].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x1x_2F_AI_m0[PR_htId].m_x[1] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x1x_2F_AI_m1[PR_htId].m_x[1] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x1x_1F_AA_m0[PR_htId][1].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x1x_1F_AA_m1[PR_htId][1].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x1x_4F_AA_m0[PR_htId].m_x[1] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x1x_4F_AA_m1[PR_htId].m_x[1] != PR_x) HtAssert(0, 0);

			// D_1x1x2x2x
			if (GR_gv_R_1x1x2x2x_1F_m0[PR_htId][1].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x2x2x_1F_m1[PR_htId][1].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x2x2x_2F_m0[PR_htId].m_x[1] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x2x2x_2F_m1[PR_htId].m_x[1] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x2x2x_1F_AI_m0[PR_htId][1].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x2x2x_1F_AI_m1[PR_htId][1].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x2x2x_2F_AI_m0[PR_htId].m_x[1] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x2x2x_2F_AI_m1[PR_htId].m_x[1] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x2x2x_1F_AA_m0[PR_htId][1].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x2x2x_1F_AA_m1[PR_htId][1].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x2x2x_4F_AA_m0[PR_htId].m_x[1] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x2x2x_4F_AA_m1[PR_htId].m_x[1] != PR_x) HtAssert(0, 0);

			P_x += 1;

			// D_1x2x
			GW_gv_R_1x2x_1F_m0[PR_htId][1].m_x = P_x;

			GW_gv_R_1x2x_1F_m1[PR_htId][1].m_x = P_x;

			GW_gv_R_1x2x_2F_m0[PR_htId].m_x[1] = P_x;

			GW_gv_R_1x2x_2F_m1[PR_htId].m_x[1] = P_x;

			GW_gv_R_1x2x_1F_AI_m0[PR_htId][1].m_x.AtomicInc();

			GW_gv_R_1x2x_1F_AI_m1[PR_htId][1].m_x.AtomicInc();

			GW_gv_R_1x2x_2F_AI_m0[PR_htId].m_x[1].AtomicInc();

			GW_gv_R_1x2x_2F_AI_m1[PR_htId].m_x[1].AtomicInc();

			GW_gv_R_1x2x_1F_AA_m0[PR_htId][1].m_x.AtomicAdd(1u);

			GW_gv_R_1x2x_1F_AA_m1[PR_htId][1].m_x = P_x;

			GW_gv_R_1x2x_4F_AA_m0[PR_htId].m_x[1].AtomicAdd(1u);

			GW_gv_R_1x2x_4F_AA_m1[PR_htId].m_x[1] = P_x;

			// D_1x1x
			GW_gv_R_1x1x_1F_m0[PR_htId][1].m_x = P_x;

			GW_gv_R_1x1x_1F_m1[PR_htId][1].m_x = P_x;

			GW_gv_R_1x1x_2F_m0[PR_htId].m_x[1] = P_x;

			GW_gv_R_1x1x_2F_m1[PR_htId].m_x[1] = P_x;

			GW_gv_R_1x1x_1F_AI_m0[PR_htId][1].m_x = P_x;

			GW_gv_R_1x1x_1F_AI_m1[PR_htId][1].m_x = P_x;

			GW_gv_R_1x1x_2F_AI_m0[PR_htId].m_x[1] = P_x;

			GW_gv_R_1x1x_2F_AI_m1[PR_htId].m_x[1] = P_x;

			GW_gv_R_1x1x_1F_AA_m0[PR_htId][1].m_x.AtomicAdd(1u);

			GW_gv_R_1x1x_1F_AA_m1[PR_htId][1].m_x = P_x;

			GW_gv_R_1x1x_4F_AA_m0[PR_htId].m_x[1].AtomicAdd(1u);

			GW_gv_R_1x1x_4F_AA_m1[PR_htId].m_x[1] = P_x;

			// D_1x1x1x
			GW_gv_R_1x1x1x_1F_m0[PR_htId][1].m_x = P_x;

			GW_gv_R_1x1x1x_1F_m1[PR_htId][1].m_x = P_x;

			GW_gv_R_1x1x1x_2F_m0[PR_htId].m_x[1] = P_x;

			GW_gv_R_1x1x1x_2F_m1[PR_htId].m_x[1] = P_x;

			GW_gv_R_1x1x1x_1F_AI_m0[PR_htId][1].m_x.AtomicInc();

			GW_gv_R_1x1x1x_1F_AI_m1[PR_htId][1].m_x.AtomicInc();

			GW_gv_R_1x1x1x_2F_AI_m0[PR_htId].m_x[1].AtomicInc();

			GW_gv_R_1x1x1x_2F_AI_m1[PR_htId].m_x[1].AtomicInc();

			GW_gv_R_1x1x1x_1F_AA_m0[PR_htId][1].m_x.AtomicAdd(1u);

			GW_gv_R_1x1x1x_1F_AA_m1[PR_htId][1].m_x.AtomicAdd(1u);

			GW_gv_R_1x1x1x_4F_AA_m0[PR_htId].m_x[1].AtomicAdd(1u);

			GW_gv_R_1x1x1x_4F_AA_m1[PR_htId].m_x[1].AtomicAdd(1u);

			// D_1x1x2x2x
			GW_gv_R_1x1x2x2x_1F_m0[PR_htId][1].m_x = P_x;

			GW_gv_R_1x1x2x2x_1F_m1[PR_htId][1].m_x = P_x;

			GW_gv_R_1x1x2x2x_2F_m0[PR_htId].m_x[1] = P_x;

			GW_gv_R_1x1x2x2x_2F_m1[PR_htId].m_x[1] = P_x;

			GW_gv_R_1x1x2x2x_1F_AI_m0[PR_htId][1].m_x.AtomicInc();

			GW_gv_R_1x1x2x2x_1F_AI_m1[PR_htId][1].m_x.AtomicInc();

			GW_gv_R_1x1x2x2x_2F_AI_m0[PR_htId].m_x[1].AtomicInc();

			GW_gv_R_1x1x2x2x_2F_AI_m1[PR_htId].m_x[1].AtomicInc();

			GW_gv_R_1x1x2x2x_1F_AA_m0[PR_htId][1].m_x.AtomicAdd(1u);

			GW_gv_R_1x1x2x2x_1F_AA_m1[PR_htId][1].m_x = P_x;

			GW_gv_R_1x1x2x2x_4F_AA_m0[PR_htId].m_x[1].AtomicAdd(1u);

			GW_gv_R_1x1x2x2x_4F_AA_m1[PR_htId].m_x[1] = P_x;

			HtContinue(P_loopCnt++ < 100 ? GV2_LOOP : GV2_RETURN);
			break;
		}
		case GV2_RETURN:
		{
			if (SendReturnBusy_gv2()) {
				HtRetry();
				break;
			}

			SendReturn_gv2();
			break;
		}
		default:
			assert(0);
		}
	}
}
