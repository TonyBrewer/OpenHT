#include "Ht.h"
#include "PersGv3.h"

void CPersGv3::PersGv3()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case GV3_ENTRY:
		{
			P_x = PR_htId;

			// D_2x
			GW_gv_R_2x_1F_m0[PR_htId].m_x = P_x;

			GW_gv_R_2x_1F_m1[PR_htId].m_x = P_x;

			GW_gv_R_2x_2F_m0[PR_htId].m_x[0] = P_x;

			GW_gv_R_2x_2F_m1[PR_htId].m_x[0] = P_x;

			GW_gv_R_2x_1F_AI_m0[PR_htId].m_x = P_x;

			GW_gv_R_2x_1F_AI_m1[PR_htId].m_x = P_x;

			GW_gv_R_2x_2F_AI_m0[PR_htId].m_x[0] = P_x;

			GW_gv_R_2x_2F_AI_m1[PR_htId].m_x[0] = P_x;

			GW_gv_R_2x_1F_AA_m0[PR_htId].m_x = P_x;

			GW_gv_R_2x_1F_AA_m1[PR_htId].m_x = P_x;

			GW_gv_R_2x_4F_AA_m0[PR_htId].m_x[0] = P_x;

			GW_gv_R_2x_4F_AA_m1[PR_htId].m_x[0] = P_x;

			// D_2x2x
			GW_gv_R_2x2x_1F_m0[PR_htId][2].m_x = P_x;

			GW_gv_R_2x2x_1F_m1[PR_htId][2].m_x = P_x;

			GW_gv_R_2x2x_2F_m0[PR_htId].m_y[0] = P_x;

			GW_gv_R_2x2x_2F_m1[PR_htId].m_y[0] = P_x;

			GW_gv_R_2x2x_1F_AI_m0[PR_htId][2].m_x = P_x;

			GW_gv_R_2x2x_1F_AI_m1[PR_htId][2].m_x = P_x;

			GW_gv_R_2x2x_2F_AI_m0[PR_htId].m_y[0] = P_x;

			GW_gv_R_2x2x_2F_AI_m1[PR_htId].m_y[0] = P_x;

			GW_gv_R_2x2x_1F_AA_m0[PR_htId][2].m_x = P_x;

			GW_gv_R_2x2x_1F_AA_m1[PR_htId][2].m_x = P_x;

			GW_gv_R_2x2x_4F_AA_m0[PR_htId].m_x[0] = P_x;

			GW_gv_R_2x2x_4F_AA_m1[PR_htId].m_x[0] = P_x;

			// D_2x2x2x
			GW_gv_R_2x2x2x_1F_m0[PR_htId][2].m_x = P_x;

			GW_gv_R_2x2x2x_1F_m1[PR_htId][2].m_x = P_x;

			GW_gv_R_2x2x2x_2F_m0[PR_htId].m_y[0] = P_x;

			GW_gv_R_2x2x2x_2F_m1[PR_htId].m_y[0] = P_x;

			GW_gv_R_2x2x2x_1F_AI_m0[PR_htId][2].m_x = P_x;

			GW_gv_R_2x2x2x_1F_AI_m1[PR_htId][2].m_x = P_x;

			GW_gv_R_2x2x2x_2F_AI_m0[PR_htId].m_y[0] = P_x;

			GW_gv_R_2x2x2x_2F_AI_m1[PR_htId].m_y[0] = P_x;

			GW_gv_R_2x2x2x_1F_AA_m0[PR_htId][2].m_x = P_x;

			GW_gv_R_2x2x2x_1F_AA_m1[PR_htId][2].m_x = P_x;

			GW_gv_R_2x2x2x_4F_AA_m0[PR_htId].m_x[2] = P_x;

			GW_gv_R_2x2x2x_4F_AA_m1[PR_htId].m_x[2] = P_x;

			// D_1x1x2x2x
			GW_gv_R_1x1x2x2x_1F_m0[PR_htId][2].m_x = P_x;

			GW_gv_R_1x1x2x2x_1F_m1[PR_htId][2].m_x = P_x;

			GW_gv_R_1x1x2x2x_2F_m0[PR_htId].m_y[0] = P_x;

			GW_gv_R_1x1x2x2x_2F_m1[PR_htId].m_y[0] = P_x;

			GW_gv_R_1x1x2x2x_1F_AI_m0[PR_htId][2].m_x = P_x;

			GW_gv_R_1x1x2x2x_1F_AI_m1[PR_htId][2].m_x = P_x;

			GW_gv_R_1x1x2x2x_2F_AI_m0[PR_htId].m_y[0] = P_x;

			GW_gv_R_1x1x2x2x_2F_AI_m1[PR_htId].m_y[0] = P_x;

			GW_gv_R_1x1x2x2x_1F_AA_m0[PR_htId][2].m_x = P_x;

			GW_gv_R_1x1x2x2x_1F_AA_m1[PR_htId][2].m_x = P_x;

			GW_gv_R_1x1x2x2x_4F_AA_m0[PR_htId].m_x[2] = P_x;

			GW_gv_R_1x1x2x2x_4F_AA_m1[PR_htId].m_x[2] = P_x;

			P_loopCnt = 0;
			HtContinue(GV3_LOOP);
		}
			break;
		case GV3_LOOP:
		{
			// D_2x
			if (GR_gv_R_2x_1F_m0[PR_htId].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_2x_1F_m1[PR_htId].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_2x_2F_m0[PR_htId].m_x[0] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_2x_2F_m1[PR_htId].m_x[0] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_2x_1F_AI_m0[PR_htId].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_2x_1F_AI_m1[PR_htId].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_2x_2F_AI_m0[PR_htId].m_x[0] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_2x_2F_AI_m1[PR_htId].m_x[0] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_2x_1F_AA_m0[PR_htId].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_2x_1F_AA_m1[PR_htId].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_2x_4F_AA_m0[PR_htId].m_x[0] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_2x_4F_AA_m1[PR_htId].m_x[0] != PR_x) HtAssert(0, 0);

			// D_2x2x
			if (GR_gv_R_2x2x_1F_m0[PR_htId][2].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_2x2x_1F_m1[PR_htId][2].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_2x2x_2F_m0[PR_htId].m_y[0] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_2x2x_2F_m1[PR_htId].m_y[0] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_2x2x_1F_AI_m0[PR_htId][2].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_2x2x_1F_AI_m1[PR_htId][2].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_2x2x_2F_AI_m0[PR_htId].m_y[0] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_2x2x_2F_AI_m1[PR_htId].m_y[0] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_2x2x_1F_AA_m0[PR_htId][2].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_2x2x_1F_AA_m1[PR_htId][2].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_2x2x_4F_AA_m0[PR_htId].m_x[0] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_2x2x_4F_AA_m1[PR_htId].m_x[0] != PR_x) HtAssert(0, 0);

			// D_2x2x2x
			if (GR_gv_R_2x2x2x_1F_m0[PR_htId][2].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_2x2x2x_1F_m1[PR_htId][2].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_2x2x2x_2F_m0[PR_htId].m_y[0] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_2x2x2x_2F_m1[PR_htId].m_y[0] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_2x2x2x_1F_AI_m0[PR_htId][2].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_2x2x2x_1F_AI_m1[PR_htId][2].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_2x2x2x_2F_AI_m0[PR_htId].m_y[0] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_2x2x2x_2F_AI_m1[PR_htId].m_y[0] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_2x2x2x_1F_AA_m0[PR_htId][2].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_2x2x2x_1F_AA_m1[PR_htId][2].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_2x2x2x_4F_AA_m0[PR_htId].m_x[2] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_2x2x2x_4F_AA_m1[PR_htId].m_x[2] != PR_x) HtAssert(0, 0);

			// D_1x1x2x2x
			if (GR_gv_R_1x1x2x2x_1F_m0[PR_htId][2].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x2x2x_1F_m1[PR_htId][2].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x2x2x_2F_m0[PR_htId].m_y[0] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x2x2x_2F_m1[PR_htId].m_y[0] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x2x2x_1F_AI_m0[PR_htId][2].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x2x2x_1F_AI_m1[PR_htId][2].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x2x2x_2F_AI_m0[PR_htId].m_y[0] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x2x2x_2F_AI_m1[PR_htId].m_y[0] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x2x2x_1F_AA_m0[PR_htId][2].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x2x2x_1F_AA_m1[PR_htId][2].m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x2x2x_4F_AA_m0[PR_htId].m_x[2] != PR_x) HtAssert(0, 0);
			if (GR_gv_R_1x1x2x2x_4F_AA_m1[PR_htId].m_x[2] != PR_x) HtAssert(0, 0);

			P_x += 1;

			// D_2x
			GW_gv_R_2x_1F_m0[PR_htId].m_x = P_x;

			GW_gv_R_2x_1F_m1[PR_htId].m_x = P_x;

			GW_gv_R_2x_2F_m0[PR_htId].m_x[0] = P_x;

			GW_gv_R_2x_2F_m1[PR_htId].m_x[0] = P_x;

			GW_gv_R_2x_1F_AI_m0[PR_htId].m_x.AtomicInc();

			GW_gv_R_2x_1F_AI_m1[PR_htId].m_x.AtomicInc();

			GW_gv_R_2x_2F_AI_m0[PR_htId].m_x[0].AtomicInc();

			GW_gv_R_2x_2F_AI_m1[PR_htId].m_x[0].AtomicInc();

			GW_gv_R_2x_1F_AA_m0[PR_htId].m_x.AtomicAdd(1u);

			GW_gv_R_2x_1F_AA_m1[PR_htId].m_x = P_x;

			GW_gv_R_2x_4F_AA_m0[PR_htId].m_x[0].AtomicAdd(1u);

			GW_gv_R_2x_4F_AA_m1[PR_htId].m_x[0] = P_x;

			// D_2x2x
			GW_gv_R_2x2x_1F_m0[PR_htId][2].m_x = P_x;

			GW_gv_R_2x2x_1F_m1[PR_htId][2].m_x = P_x;

			GW_gv_R_2x2x_2F_m0[PR_htId].m_y[0] = P_x;

			GW_gv_R_2x2x_2F_m1[PR_htId].m_y[0] = P_x;

			GW_gv_R_2x2x_1F_AI_m0[PR_htId][2].m_x.AtomicInc();

			GW_gv_R_2x2x_1F_AI_m1[PR_htId][2].m_x.AtomicInc();

			GW_gv_R_2x2x_2F_AI_m0[PR_htId].m_y[0].AtomicInc();

			GW_gv_R_2x2x_2F_AI_m1[PR_htId].m_y[0].AtomicInc();

			GW_gv_R_2x2x_1F_AA_m0[PR_htId][2].m_x.AtomicAdd(1u);

			GW_gv_R_2x2x_1F_AA_m1[PR_htId][2].m_x = P_x;

			GW_gv_R_2x2x_4F_AA_m0[PR_htId].m_x[0].AtomicAdd(1u);

			GW_gv_R_2x2x_4F_AA_m1[PR_htId].m_x[0] = P_x;

			// D_2x2x2x
			GW_gv_R_2x2x2x_1F_m0[PR_htId][2].m_x = P_x;

			GW_gv_R_2x2x2x_1F_m1[PR_htId][2].m_x = P_x;

			GW_gv_R_2x2x2x_2F_m0[PR_htId].m_y[0] = P_x;

			GW_gv_R_2x2x2x_2F_m1[PR_htId].m_y[0] = P_x;

			GW_gv_R_2x2x2x_1F_AI_m0[PR_htId][2].m_x.AtomicInc();

			GW_gv_R_2x2x2x_1F_AI_m1[PR_htId][2].m_x.AtomicInc();

			GW_gv_R_2x2x2x_2F_AI_m0[PR_htId].m_y[0].AtomicInc();

			GW_gv_R_2x2x2x_2F_AI_m1[PR_htId].m_y[0].AtomicInc();

			GW_gv_R_2x2x2x_1F_AA_m0[PR_htId][2].m_x.AtomicAdd(1u);

			GW_gv_R_2x2x2x_1F_AA_m1[PR_htId][2].m_x = P_x;

			GW_gv_R_2x2x2x_4F_AA_m0[PR_htId].m_x[2].AtomicAdd(1u);

			GW_gv_R_2x2x2x_4F_AA_m1[PR_htId].m_x[2] = P_x;

			// D_1x1x2x2x
			GW_gv_R_1x1x2x2x_1F_m0[PR_htId][2].m_x = P_x;

			GW_gv_R_1x1x2x2x_1F_m1[PR_htId][2].m_x = P_x;

			GW_gv_R_1x1x2x2x_2F_m0[PR_htId].m_y[0] = P_x;

			GW_gv_R_1x1x2x2x_2F_m1[PR_htId].m_y[0] = P_x;

			GW_gv_R_1x1x2x2x_1F_AI_m0[PR_htId][2].m_x.AtomicInc();

			GW_gv_R_1x1x2x2x_1F_AI_m1[PR_htId][2].m_x.AtomicInc();

			GW_gv_R_1x1x2x2x_2F_AI_m0[PR_htId].m_y[0].AtomicInc();

			GW_gv_R_1x1x2x2x_2F_AI_m1[PR_htId].m_y[0].AtomicInc();

			GW_gv_R_1x1x2x2x_1F_AA_m0[PR_htId][2].m_x.AtomicAdd(1u);

			GW_gv_R_1x1x2x2x_1F_AA_m1[PR_htId][2].m_x = P_x;

			GW_gv_R_1x1x2x2x_4F_AA_m0[PR_htId].m_x[2].AtomicAdd(1u);

			GW_gv_R_1x1x2x2x_4F_AA_m1[PR_htId].m_x[2] = P_x;

			HtContinue(P_loopCnt++ < 100 ? GV3_LOOP : GV3_RETURN);
		}
			break;
		case GV3_RETURN:
		{
			if (SendReturnBusy_gv3()) {
				HtRetry();
				break;
			}

			SendReturn_gv3();
		}
			break;
		default:
			assert(0);
		}
	}
}
