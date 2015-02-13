#include "Ht.h"
#include "PersGv1.h"

void CPersGv1::PersGv1()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case GV1_ENTRY:
		{
			P_x = PR_htId;

			// D_1x
			GW_gv_R_1x_1F_m0[PR_htId].m_x = P_x;

			GW_gv_R_1x_1F_m1[PR_htId].m_x = P_x;

			GW_gv_R_1x_2F_m0[PR_htId].m_x[0] = P_x;

			GW_gv_R_1x_2F_m1[PR_htId].m_x[0] = P_x;

			GW_gv_R_1x_1F_AI_m0[PR_htId].m_x = P_x;

			GW_gv_R_1x_1F_AI_m1[PR_htId].m_x = P_x;

			GW_gv_R_1x_2F_AI_m0[PR_htId].m_x[0] = P_x;

			GW_gv_R_1x_2F_AI_m1[PR_htId].m_x[0] = P_x;

			GW_gv_R_1x_1F_AA_m0[PR_htId].m_x = P_x;

			GW_gv_R_1x_1F_AA_m1[PR_htId].m_x = P_x;

			GW_gv_R_1x_4F_AA_m0[PR_htId].m_x[0] = P_x;

			GW_gv_R_1x_4F_AA_m1[PR_htId].m_x[0] = P_x;

			// D_1x1x
			GW_gv_R_1x1x_1F_m0[PR_htId][0].m_x = P_x;

			GW_gv_R_1x1x_1F_m1[PR_htId][0].m_x = P_x;

			GW_gv_R_1x1x_2F_m0[PR_htId].m_x[0] = P_x;

			GW_gv_R_1x1x_2F_m1[PR_htId].m_x[0] = P_x;

			GW_gv_R_1x1x_1F_AI_m0[PR_htId][0].m_x = P_x;

			GW_gv_R_1x1x_1F_AI_m1[PR_htId][0].m_x = P_x;

			GW_gv_R_1x1x_2F_AI_m0[PR_htId].m_x[0] = P_x;

			GW_gv_R_1x1x_2F_AI_m1[PR_htId].m_x[0] = P_x;

			GW_gv_R_1x1x_1F_AA_m0[PR_htId][0].m_x = P_x;

			GW_gv_R_1x1x_1F_AA_m1[PR_htId][0].m_x = P_x;

			GW_gv_R_1x1x_4F_AA_m0[PR_htId].m_x[0] = P_x;

			GW_gv_R_1x1x_4F_AA_m1[PR_htId].m_x[0] = P_x;

			// D_1x1x1x
			GW_gv_R_1x1x1x_1F_m0[PR_htId][0].m_x = P_x;

			GW_gv_R_1x1x1x_1F_m1[PR_htId][0].m_x = P_x;

			GW_gv_R_1x1x1x_2F_m0[PR_htId].m_x[0] = P_x;

			GW_gv_R_1x1x1x_2F_m1[PR_htId].m_x[0] = P_x;

			GW_gv_R_1x1x1x_1F_AI_m0[PR_htId][0].m_x = P_x;

			GW_gv_R_1x1x1x_1F_AI_m1[PR_htId][0].m_x = P_x;

			GW_gv_R_1x1x1x_2F_AI_m0[PR_htId].m_x[0] = P_x;

			GW_gv_R_1x1x1x_2F_AI_m1[PR_htId].m_x[0] = P_x;

			GW_gv_R_1x1x1x_1F_AA_m0[PR_htId][0].m_x = P_x;

			GW_gv_R_1x1x1x_1F_AA_m1[PR_htId][0].m_x = P_x;

			GW_gv_R_1x1x1x_4F_AA_m0[PR_htId].m_x[0] = P_x;

			GW_gv_R_1x1x1x_4F_AA_m1[PR_htId].m_x[0] = P_x;

			// D_1x1x2x2x
			GW_gv_R_1x1x2x2x_1F_m0[PR_htId][0].m_x = P_x;

			GW_gv_R_1x1x2x2x_1F_m1[PR_htId][0].m_x = P_x;

			GW_gv_R_1x1x2x2x_2F_m0[PR_htId].m_x[0] = P_x;

			GW_gv_R_1x1x2x2x_2F_m1[PR_htId].m_x[0] = P_x;

			GW_gv_R_1x1x2x2x_1F_AI_m0[PR_htId][0].m_x = P_x;

			GW_gv_R_1x1x2x2x_1F_AI_m1[PR_htId][0].m_x = P_x;

			GW_gv_R_1x1x2x2x_2F_AI_m0[PR_htId].m_x[0] = P_x;

			GW_gv_R_1x1x2x2x_2F_AI_m1[PR_htId].m_x[0] = P_x;

			GW_gv_R_1x1x2x2x_1F_AA_m0[PR_htId][0].m_x = P_x;

			GW_gv_R_1x1x2x2x_1F_AA_m1[PR_htId][0].m_x = P_x;

			GW_gv_R_1x1x2x2x_4F_AA_m0[PR_htId].m_x[0] = P_x;

			GW_gv_R_1x1x2x2x_4F_AA_m1[PR_htId].m_x[0] = P_x;

			P_loopCnt = 0;
			HtContinue(GV1_LOOP);
		}
			break;
		case GV1_LOOP:
		{
			// D_1x
			if (GR_gv_R_1x_1F_m0[PR_htId].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x_1F_m1[PR_htId].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x_2F_m0[PR_htId].m_x[0] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x_2F_m1[PR_htId].m_x[0] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x_1F_AI_m0[PR_htId].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x_1F_AI_m1[PR_htId].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x_2F_AI_m0[PR_htId].m_x[0] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x_2F_AI_m1[PR_htId].m_x[0] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x_1F_AA_m0[PR_htId].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x_1F_AA_m1[PR_htId].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x_4F_AA_m0[PR_htId].m_x[0] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x_4F_AA_m1[PR_htId].m_x[0] != PR_x) HtAssert(0, 0);

			// D_1x1x
			if (GR_gv_R_1x1x_1F_m0[PR_htId][0].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x_1F_m1[PR_htId][0].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x_2F_m0[PR_htId].m_x[0] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x_2F_m1[PR_htId].m_x[0] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x_1F_AI_m0[PR_htId][0].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x_1F_AI_m1[PR_htId][0].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x_2F_AI_m0[PR_htId].m_x[0] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x_2F_AI_m1[PR_htId].m_x[0] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x_1F_AA_m0[PR_htId][0].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x_1F_AA_m1[PR_htId][0].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x_4F_AA_m0[PR_htId].m_x[0] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x_4F_AA_m1[PR_htId].m_x[0] != PR_x) HtAssert(0, 0);

			// D_1x1x1x
			if (GR_gv_R_1x1x1x_1F_m0[PR_htId][0].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x1x_1F_m1[PR_htId][0].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x1x_2F_m0[PR_htId].m_x[0] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x1x_2F_m1[PR_htId].m_x[0] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x1x_1F_AI_m0[PR_htId][0].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x1x_1F_AI_m1[PR_htId][0].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x1x_2F_AI_m0[PR_htId].m_x[0] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x1x_2F_AI_m1[PR_htId].m_x[0] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x1x_1F_AA_m0[PR_htId][0].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x1x_1F_AA_m1[PR_htId][0].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x1x_4F_AA_m0[PR_htId].m_x[0] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x1x_4F_AA_m1[PR_htId].m_x[0] != PR_x) HtAssert(0, 0);

			// D_1x1x2x2x
			if (GR_gv_R_1x1x2x2x_1F_m0[PR_htId][0].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x2x2x_1F_m1[PR_htId][0].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x2x2x_2F_m0[PR_htId].m_x[0] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x2x2x_2F_m1[PR_htId].m_x[0] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x2x2x_1F_AI_m0[PR_htId][0].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x2x2x_1F_AI_m1[PR_htId][0].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x2x2x_2F_AI_m0[PR_htId].m_x[0] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x2x2x_2F_AI_m1[PR_htId].m_x[0] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x2x2x_1F_AA_m0[PR_htId][0].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x2x2x_1F_AA_m1[PR_htId][0].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x2x2x_4F_AA_m0[PR_htId].m_x[0] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x2x2x_4F_AA_m1[PR_htId].m_x[0] != PR_x) HtAssert(0, 0);

			P_x += 1;

			// D_1x
			GW_gv_R_1x_1F_m0[PR_htId].m_x = P_x;

			GW_gv_R_1x_1F_m1[PR_htId].m_x = P_x;

			GW_gv_R_1x_2F_m0[PR_htId].m_x[0] = P_x;

			GW_gv_R_1x_2F_m1[PR_htId].m_x[0] = P_x;

			GW_gv_R_1x_1F_AI_m0[PR_htId].m_x.AtomicInc();

			GW_gv_R_1x_1F_AI_m1[PR_htId].m_x.AtomicInc();

			GW_gv_R_1x_2F_AI_m0[PR_htId].m_x[0].AtomicInc();

			GW_gv_R_1x_2F_AI_m1[PR_htId].m_x[0].AtomicInc();

			GW_gv_R_1x_1F_AA_m0[PR_htId].m_x.AtomicAdd(1u);

			GW_gv_R_1x_1F_AA_m1[PR_htId].m_x = P_x;

			GW_gv_R_1x_4F_AA_m0[PR_htId].m_x[0].AtomicAdd(1u);

			GW_gv_R_1x_4F_AA_m1[PR_htId].m_x[0] = P_x;

			// D_1x1x
			GW_gv_R_1x1x_1F_m0[PR_htId][0].m_x = P_x;

			GW_gv_R_1x1x_1F_m1[PR_htId][0].m_x = P_x;

			GW_gv_R_1x1x_2F_m0[PR_htId].m_x[0] = P_x;

			GW_gv_R_1x1x_2F_m1[PR_htId].m_x[0] = P_x;

			GW_gv_R_1x1x_1F_AI_m0[PR_htId][0].m_x.AtomicInc();

			GW_gv_R_1x1x_1F_AI_m1[PR_htId][0].m_x.AtomicInc();

			GW_gv_R_1x1x_2F_AI_m0[PR_htId].m_x[0].AtomicInc();

			GW_gv_R_1x1x_2F_AI_m1[PR_htId].m_x[0].AtomicInc();

			GW_gv_R_1x1x_1F_AA_m0[PR_htId][0].m_x.AtomicAdd(1u);

			GW_gv_R_1x1x_1F_AA_m1[PR_htId][0].m_x = P_x;

			GW_gv_R_1x1x_4F_AA_m0[PR_htId].m_x[0].AtomicAdd(1u);

			GW_gv_R_1x1x_4F_AA_m1[PR_htId].m_x[0] = P_x;

			// D_1x1x1x
			GW_gv_R_1x1x1x_1F_m0[PR_htId][0].m_x = P_x;

			GW_gv_R_1x1x1x_1F_m1[PR_htId][0].m_x = P_x;

			GW_gv_R_1x1x1x_2F_m0[PR_htId].m_x[0] = P_x;

			GW_gv_R_1x1x1x_2F_m1[PR_htId].m_x[0] = P_x;

			GW_gv_R_1x1x1x_1F_AI_m0[PR_htId][0].m_x.AtomicInc();

			GW_gv_R_1x1x1x_1F_AI_m1[PR_htId][0].m_x.AtomicInc();

			GW_gv_R_1x1x1x_2F_AI_m0[PR_htId].m_x[0].AtomicInc();

			GW_gv_R_1x1x1x_2F_AI_m1[PR_htId].m_x[0].AtomicInc();

			GW_gv_R_1x1x1x_1F_AA_m0[PR_htId][0].m_x.AtomicAdd(1u);

			GW_gv_R_1x1x1x_1F_AA_m1[PR_htId][0].m_x = P_x;

			GW_gv_R_1x1x1x_4F_AA_m0[PR_htId].m_x[0].AtomicAdd(1u);

			GW_gv_R_1x1x1x_4F_AA_m1[PR_htId].m_x[0] = P_x;

			// D_1x1x2x2x
			GW_gv_R_1x1x2x2x_1F_m0[PR_htId][0].m_x = P_x;

			GW_gv_R_1x1x2x2x_1F_m1[PR_htId][0].m_x = P_x;

			GW_gv_R_1x1x2x2x_2F_m0[PR_htId].m_x[0] = P_x;

			GW_gv_R_1x1x2x2x_2F_m1[PR_htId].m_x[0] = P_x;

			GW_gv_R_1x1x2x2x_1F_AI_m0[PR_htId][0].m_x.AtomicInc();

			GW_gv_R_1x1x2x2x_1F_AI_m1[PR_htId][0].m_x.AtomicInc();

			GW_gv_R_1x1x2x2x_2F_AI_m0[PR_htId].m_x[0].AtomicInc();

			GW_gv_R_1x1x2x2x_2F_AI_m1[PR_htId].m_x[0].AtomicInc();

			GW_gv_R_1x1x2x2x_1F_AA_m0[PR_htId][0].m_x.AtomicAdd(1u);

			GW_gv_R_1x1x2x2x_1F_AA_m1[PR_htId][0].m_x = P_x;

			GW_gv_R_1x1x2x2x_4F_AA_m0[PR_htId].m_x[0].AtomicAdd(1u);

			GW_gv_R_1x1x2x2x_4F_AA_m1[PR_htId].m_x[0] = P_x;

			HtContinue(P_loopCnt++ < 100 ? GV1_LOOP : GV1_RETURN);
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
