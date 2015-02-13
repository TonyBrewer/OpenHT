#include "Ht.h"
#include "PersGv4.h"

void CPersGv4::PersGv4()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case GV4_ENTRY:
		{
			P_x = PR_htId;
			P_gvAddr = PR_htId;
			P_gvAddr2 = 3u;
			P_gvAddr3 = 1u;

			// D_1x2x
			GW_gv_B_1x2x_1F_m0.write_addr(PR_htId, 3u);
			GW_gv_B_1x2x_1F_m0.m_x = P_x;

			GW_gv_B_1x2x_1F_m1.write_addr(PR_htId, 3u);
			GW_gv_B_1x2x_1F_m1.m_x = P_x;

			GW_gv_B_1x2x_2F_m0.write_addr(PR_htId);
			GW_gv_B_1x2x_2F_m0.m_y[1] = P_x;

			GW_gv_B_1x2x_2F_m1.write_addr(PR_htId);
			GW_gv_B_1x2x_2F_m1.m_y[1] = P_x;

			GW_gv_B_1x2x_1F_AI_m0.write_addr(PR_htId, 3u);
			GW_gv_B_1x2x_1F_AI_m0.m_x = P_x;

			GW_gv_B_1x2x_1F_AI_m1.write_addr(PR_htId, 3u);
			GW_gv_B_1x2x_1F_AI_m1.m_x = P_x;

			GW_gv_B_1x2x_2F_AI_m0.write_addr(PR_htId);
			GW_gv_B_1x2x_2F_AI_m0.m_y[1] = P_x;

			GW_gv_B_1x2x_2F_AI_m1.write_addr(PR_htId);
			GW_gv_B_1x2x_2F_AI_m1.m_y[1] = P_x;

			GW_gv_B_1x2x_1F_AA_m0.write_addr(PR_htId, 3u);
			GW_gv_B_1x2x_1F_AA_m0.m_x = P_x;

			GW_gv_B_1x2x_1F_AA_m1.write_addr(PR_htId, 3u);
			GW_gv_B_1x2x_1F_AA_m1.m_x = P_x;

			GW_gv_B_1x2x_4F_AA_m0.write_addr(PR_htId);
			GW_gv_B_1x2x_4F_AA_m0.m_x[3] = P_x;

			GW_gv_B_1x2x_4F_AA_m1.write_addr(PR_htId);
			GW_gv_B_1x2x_4F_AA_m1.m_x[3] = P_x;

			// D_2x2x
			GW_gv_B_2x2x_1F_m0.write_addr(PR_htId, 3u);
			GW_gv_B_2x2x_1F_m0.m_x = P_x;

			GW_gv_B_2x2x_1F_m1.write_addr(PR_htId, 3u);
			GW_gv_B_2x2x_1F_m1.m_x = P_x;

			GW_gv_B_2x2x_2F_m0.write_addr(PR_htId);
			GW_gv_B_2x2x_2F_m0.m_y[1] = P_x;

			GW_gv_B_2x2x_2F_m1.write_addr(PR_htId);
			GW_gv_B_2x2x_2F_m1.m_y[1] = P_x;

			GW_gv_B_2x2x_1F_AI_m0.write_addr(PR_htId, 3u);
			GW_gv_B_2x2x_1F_AI_m0.m_x = P_x;

			GW_gv_B_2x2x_1F_AI_m1.write_addr(PR_htId, 3u);
			GW_gv_B_2x2x_1F_AI_m1.m_x = P_x;

			GW_gv_B_2x2x_2F_AI_m0.write_addr(PR_htId);
			GW_gv_B_2x2x_2F_AI_m0.m_y[1] = P_x;

			GW_gv_B_2x2x_2F_AI_m1.write_addr(PR_htId);
			GW_gv_B_2x2x_2F_AI_m1.m_y[1] = P_x;

			GW_gv_B_2x2x_1F_AA_m0.write_addr(PR_htId, 3u);
			GW_gv_B_2x2x_1F_AA_m0.m_x = P_x;

			GW_gv_B_2x2x_1F_AA_m1.write_addr(PR_htId, 3u);
			GW_gv_B_2x2x_1F_AA_m1.m_x = P_x;

			GW_gv_B_2x2x_4F_AA_m0.write_addr(PR_htId);
			GW_gv_B_2x2x_4F_AA_m0.m_x[3] = P_x;

			GW_gv_B_2x2x_4F_AA_m1.write_addr(PR_htId);
			GW_gv_B_2x2x_4F_AA_m1.m_x[3] = P_x;

			// D_2x2x2x
			GW_gv_B_2x2x2x_1F_m0.write_addr(PR_htId, 3u);
			GW_gv_B_2x2x2x_1F_m0.m_x = P_x;

			GW_gv_B_2x2x2x_1F_m1.write_addr(PR_htId, 3u);
			GW_gv_B_2x2x2x_1F_m1.m_x = P_x;

			GW_gv_B_2x2x2x_2F_m0.write_addr(PR_htId);
			GW_gv_B_2x2x2x_2F_m0.m_y[1] = P_x;

			GW_gv_B_2x2x2x_2F_m1.write_addr(PR_htId);
			GW_gv_B_2x2x2x_2F_m1.m_y[1] = P_x;

			GW_gv_B_2x2x2x_1F_AI_m0.write_addr(PR_htId, 3u);
			GW_gv_B_2x2x2x_1F_AI_m0.m_x = P_x;

			GW_gv_B_2x2x2x_1F_AI_m1.write_addr(PR_htId, 3u);
			GW_gv_B_2x2x2x_1F_AI_m1.m_x = P_x;

			GW_gv_B_2x2x2x_2F_AI_m0.write_addr(PR_htId);
			GW_gv_B_2x2x2x_2F_AI_m0.m_y[1] = P_x;

			GW_gv_B_2x2x2x_2F_AI_m1.write_addr(PR_htId);
			GW_gv_B_2x2x2x_2F_AI_m1.m_y[1] = P_x;

			GW_gv_B_2x2x2x_1F_AA_m0.write_addr(PR_htId, 3u);
			GW_gv_B_2x2x2x_1F_AA_m0.m_x = P_x;

			GW_gv_B_2x2x2x_1F_AA_m1.write_addr(PR_htId, 3u);
			GW_gv_B_2x2x2x_1F_AA_m1.m_x = P_x;

			GW_gv_B_2x2x2x_4F_AA_m0.write_addr(PR_htId);
			GW_gv_B_2x2x2x_4F_AA_m0.m_x[3] = P_x;

			GW_gv_B_2x2x2x_4F_AA_m1.write_addr(PR_htId);
			GW_gv_B_2x2x2x_4F_AA_m1.m_x[3] = P_x;

			// D_1x1x2x2x
			GW_gv_B_1x1x2x2x_1F_m0.write_addr(PR_htId, 3u);
			GW_gv_B_1x1x2x2x_1F_m0.m_x = P_x;

			GW_gv_B_1x1x2x2x_1F_m1.write_addr(PR_htId, 3u);
			GW_gv_B_1x1x2x2x_1F_m1.m_x = P_x;

			GW_gv_B_1x1x2x2x_2F_m0.write_addr(PR_htId);
			GW_gv_B_1x1x2x2x_2F_m0.m_y[1] = P_x;

			GW_gv_B_1x1x2x2x_2F_m1.write_addr(PR_htId);
			GW_gv_B_1x1x2x2x_2F_m1.m_y[1] = P_x;

			GW_gv_B_1x1x2x2x_1F_AI_m0.write_addr(PR_htId, 3u);
			GW_gv_B_1x1x2x2x_1F_AI_m0.m_x = P_x;

			GW_gv_B_1x1x2x2x_1F_AI_m1.write_addr(PR_htId, 3u);
			GW_gv_B_1x1x2x2x_1F_AI_m1.m_x = P_x;

			GW_gv_B_1x1x2x2x_2F_AI_m0.write_addr(PR_htId);
			GW_gv_B_1x1x2x2x_2F_AI_m0.m_y[1] = P_x;

			GW_gv_B_1x1x2x2x_2F_AI_m1.write_addr(PR_htId);
			GW_gv_B_1x1x2x2x_2F_AI_m1.m_y[1] = P_x;

			GW_gv_B_1x1x2x2x_1F_AA_m0.write_addr(PR_htId, 3u);
			GW_gv_B_1x1x2x2x_1F_AA_m0.m_x = P_x;

			GW_gv_B_1x1x2x2x_1F_AA_m1.write_addr(PR_htId, 3u);
			GW_gv_B_1x1x2x2x_1F_AA_m1.m_x = P_x;

			GW_gv_B_1x1x2x2x_4F_AA_m0.write_addr(PR_htId);
			GW_gv_B_1x1x2x2x_4F_AA_m0.m_x[3] = P_x;

			GW_gv_B_1x1x2x2x_4F_AA_m1.write_addr(PR_htId);
			GW_gv_B_1x1x2x2x_4F_AA_m1.m_x[3] = P_x;

			P_loopCnt = 0;
			HtContinue(GV4_LOOP);
		}
		break;
		case GV4_LOOP:
		{
			// D_1x2x
			if (GR_gv_B_1x2x_1F_m0.m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_B_1x2x_1F_m1.m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_B_1x2x_2F_m0.m_y[1] != PR_x) HtAssert(0, 0);
			if (GR_gv_B_1x2x_2F_m1.m_y[1] != PR_x) HtAssert(0, 0);
			if (GR_gv_B_1x2x_1F_AI_m0.m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_B_1x2x_1F_AI_m1.m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_B_1x2x_2F_AI_m0.m_y[1] != PR_x) HtAssert(0, 0);
			if (GR_gv_B_1x2x_2F_AI_m1.m_y[1] != PR_x) HtAssert(0, 0);
			if (GR_gv_B_1x2x_1F_AA_m0.m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_B_1x2x_1F_AA_m1.m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_B_1x2x_4F_AA_m0.m_x[3] != PR_x) HtAssert(0, 0);
			if (GR_gv_B_1x2x_4F_AA_m1.m_x[3] != PR_x) HtAssert(0, 0);

			// D_2x2x
			if (GR_gv_B_2x2x_1F_m0.m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_B_2x2x_1F_m1.m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_B_2x2x_2F_m0.m_y[1] != PR_x) HtAssert(0, 0);
			if (GR_gv_B_2x2x_2F_m1.m_y[1] != PR_x) HtAssert(0, 0);
			if (GR_gv_B_2x2x_1F_AI_m0.m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_B_2x2x_1F_AI_m1.m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_B_2x2x_2F_AI_m0.m_y[1] != PR_x) HtAssert(0, 0);
			if (GR_gv_B_2x2x_2F_AI_m1.m_y[1] != PR_x) HtAssert(0, 0);
			if (GR_gv_B_2x2x_1F_AA_m0.m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_B_2x2x_1F_AA_m1.m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_B_2x2x_4F_AA_m0.m_x[3] != PR_x) HtAssert(0, 0);
			if (GR_gv_B_2x2x_4F_AA_m1.m_x[3] != PR_x) HtAssert(0, 0);

			// D_2x2x2x
			if (GR_gv_B_2x2x2x_1F_m0.m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_B_2x2x2x_1F_m1.m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_B_2x2x2x_2F_m0.m_y[1] != PR_x) HtAssert(0, 0);
			if (GR_gv_B_2x2x2x_2F_m1.m_y[1] != PR_x) HtAssert(0, 0);
			if (GR_gv_B_2x2x2x_1F_AI_m0.m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_B_2x2x2x_1F_AI_m1.m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_B_2x2x2x_2F_AI_m0.m_y[1] != PR_x) HtAssert(0, 0);
			if (GR_gv_B_2x2x2x_2F_AI_m1.m_y[1] != PR_x) HtAssert(0, 0);
			if (GR_gv_B_2x2x2x_1F_AA_m0.m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_B_2x2x2x_1F_AA_m1.m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_B_2x2x2x_4F_AA_m0.m_x[3] != PR_x) HtAssert(0, 0);
			if (GR_gv_B_2x2x2x_4F_AA_m1.m_x[3] != PR_x) HtAssert(0, 0);

			// D_1x1x2x2x
			if (GR_gv_B_1x1x2x2x_1F_m0.m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_B_1x1x2x2x_1F_m1.m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_B_1x1x2x2x_2F_m0.m_y[1] != PR_x) HtAssert(0, 0);
			if (GR_gv_B_1x1x2x2x_2F_m1.m_y[1] != PR_x) HtAssert(0, 0);
			if (GR_gv_B_1x1x2x2x_1F_AI_m0.m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_B_1x1x2x2x_1F_AI_m1.m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_B_1x1x2x2x_2F_AI_m0.m_y[1] != PR_x) HtAssert(0, 0);
			if (GR_gv_B_1x1x2x2x_2F_AI_m1.m_y[1] != PR_x) HtAssert(0, 0);
			if (GR_gv_B_1x1x2x2x_1F_AA_m0.m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_B_1x1x2x2x_1F_AA_m1.m_x != PR_x) HtAssert(0, 0);
			if (GR_gv_B_1x1x2x2x_4F_AA_m0.m_x[3] != PR_x) HtAssert(0, 0);
			if (GR_gv_B_1x1x2x2x_4F_AA_m1.m_x[3] != PR_x) HtAssert(0, 0);

			P_x += 1;

			// D_1x2x
			GW_gv_B_1x2x_1F_m0.write_addr(PR_htId, 3u);
			GW_gv_B_1x2x_1F_m0.m_x = P_x;

			GW_gv_B_1x2x_1F_m1.write_addr(PR_htId, 3u);
			GW_gv_B_1x2x_1F_m1.m_x = P_x;

			GW_gv_B_1x2x_2F_m0.write_addr(PR_htId);
			GW_gv_B_1x2x_2F_m0.m_y[1] = P_x;

			GW_gv_B_1x2x_2F_m1.write_addr(PR_htId);
			GW_gv_B_1x2x_2F_m1.m_y[1] = P_x;

			GW_gv_B_1x2x_1F_AI_m0.write_addr(PR_htId, 3u);
			GW_gv_B_1x2x_1F_AI_m0.m_x.AtomicInc();

			GW_gv_B_1x2x_1F_AI_m1.write_addr(PR_htId, 3u);
			GW_gv_B_1x2x_1F_AI_m1.m_x.AtomicInc();

			GW_gv_B_1x2x_2F_AI_m0.write_addr(PR_htId);
			GW_gv_B_1x2x_2F_AI_m0.m_y[1].AtomicInc();

			GW_gv_B_1x2x_2F_AI_m1.write_addr(PR_htId);
			GW_gv_B_1x2x_2F_AI_m1.m_y[1].AtomicInc();

			GW_gv_B_1x2x_1F_AA_m0.write_addr(PR_htId, 3u);
			GW_gv_B_1x2x_1F_AA_m0.m_x.AtomicAdd(1u);

			GW_gv_B_1x2x_1F_AA_m1.write_addr(PR_htId, 3u);
			GW_gv_B_1x2x_1F_AA_m1.m_x = P_x;

			GW_gv_B_1x2x_4F_AA_m0.write_addr(PR_htId);
			GW_gv_B_1x2x_4F_AA_m0.m_x[3].AtomicAdd(1u);

			GW_gv_B_1x2x_4F_AA_m1.write_addr(PR_htId);
			GW_gv_B_1x2x_4F_AA_m1.m_x[3] = P_x;

			// D_2x2x
			GW_gv_B_2x2x_1F_m0.write_addr(PR_htId, 3u);
			GW_gv_B_2x2x_1F_m0.m_x = P_x;

			GW_gv_B_2x2x_1F_m1.write_addr(PR_htId, 3u);
			GW_gv_B_2x2x_1F_m1.m_x = P_x;

			GW_gv_B_2x2x_2F_m0.write_addr(PR_htId);
			GW_gv_B_2x2x_2F_m0.m_y[1] = P_x;

			GW_gv_B_2x2x_2F_m1.write_addr(PR_htId);
			GW_gv_B_2x2x_2F_m1.m_y[1] = P_x;

			GW_gv_B_2x2x_1F_AI_m0.write_addr(PR_htId, 3u);
			GW_gv_B_2x2x_1F_AI_m0.m_x.AtomicInc();

			GW_gv_B_2x2x_1F_AI_m1.write_addr(PR_htId, 3u);
			GW_gv_B_2x2x_1F_AI_m1.m_x.AtomicInc();

			GW_gv_B_2x2x_2F_AI_m0.write_addr(PR_htId);
			GW_gv_B_2x2x_2F_AI_m0.m_y[1].AtomicInc();

			GW_gv_B_2x2x_2F_AI_m1.write_addr(PR_htId);
			GW_gv_B_2x2x_2F_AI_m1.m_y[1].AtomicInc();

			GW_gv_B_2x2x_1F_AA_m0.write_addr(PR_htId, 3u);
			GW_gv_B_2x2x_1F_AA_m0.m_x.AtomicAdd(1u);

			GW_gv_B_2x2x_1F_AA_m1.write_addr(PR_htId, 3u);
			GW_gv_B_2x2x_1F_AA_m1.m_x = P_x;

			GW_gv_B_2x2x_4F_AA_m0.write_addr(PR_htId);
			GW_gv_B_2x2x_4F_AA_m0.m_x[3].AtomicAdd(1u);

			GW_gv_B_2x2x_4F_AA_m1.write_addr(PR_htId);
			GW_gv_B_2x2x_4F_AA_m1.m_x[3] = P_x;

			// D_2x2x2x
			GW_gv_B_2x2x2x_1F_m0.write_addr(PR_htId, 3u);
			GW_gv_B_2x2x2x_1F_m0.m_x = P_x;

			GW_gv_B_2x2x2x_1F_m1.write_addr(PR_htId, 3u);
			GW_gv_B_2x2x2x_1F_m1.m_x = P_x;

			GW_gv_B_2x2x2x_2F_m0.write_addr(PR_htId);
			GW_gv_B_2x2x2x_2F_m0.m_y[1] = P_x;

			GW_gv_B_2x2x2x_2F_m1.write_addr(PR_htId);
			GW_gv_B_2x2x2x_2F_m1.m_y[1] = P_x;

			GW_gv_B_2x2x2x_1F_AI_m0.write_addr(PR_htId, 3u);
			GW_gv_B_2x2x2x_1F_AI_m0.m_x.AtomicInc();

			GW_gv_B_2x2x2x_1F_AI_m1.write_addr(PR_htId, 3u);
			GW_gv_B_2x2x2x_1F_AI_m1.m_x.AtomicInc();

			GW_gv_B_2x2x2x_2F_AI_m0.write_addr(PR_htId);
			GW_gv_B_2x2x2x_2F_AI_m0.m_y[1].AtomicInc();

			GW_gv_B_2x2x2x_2F_AI_m1.write_addr(PR_htId);
			GW_gv_B_2x2x2x_2F_AI_m1.m_y[1].AtomicInc();

			GW_gv_B_2x2x2x_1F_AA_m0.write_addr(PR_htId, 3u);
			GW_gv_B_2x2x2x_1F_AA_m0.m_x.AtomicAdd(1u);

			GW_gv_B_2x2x2x_1F_AA_m1.write_addr(PR_htId, 3u);
			GW_gv_B_2x2x2x_1F_AA_m1.m_x = P_x;

			GW_gv_B_2x2x2x_4F_AA_m0.write_addr(PR_htId);
			GW_gv_B_2x2x2x_4F_AA_m0.m_x[3].AtomicAdd(1u);

			GW_gv_B_2x2x2x_4F_AA_m1.write_addr(PR_htId);
			GW_gv_B_2x2x2x_4F_AA_m1.m_x[3] = P_x;

			// D_1x1x2x2x
			GW_gv_B_1x1x2x2x_1F_m0.write_addr(PR_htId, 3u);
			GW_gv_B_1x1x2x2x_1F_m0.m_x = P_x;

			GW_gv_B_1x1x2x2x_1F_m1.write_addr(PR_htId, 3u);
			GW_gv_B_1x1x2x2x_1F_m1.m_x = P_x;

			GW_gv_B_1x1x2x2x_2F_m0.write_addr(PR_htId);
			GW_gv_B_1x1x2x2x_2F_m0.m_y[1] = P_x;

			GW_gv_B_1x1x2x2x_2F_m1.write_addr(PR_htId);
			GW_gv_B_1x1x2x2x_2F_m1.m_y[1] = P_x;

			GW_gv_B_1x1x2x2x_1F_AI_m0.write_addr(PR_htId, 3u);
			GW_gv_B_1x1x2x2x_1F_AI_m0.m_x.AtomicInc();

			GW_gv_B_1x1x2x2x_1F_AI_m1.write_addr(PR_htId, 3u);
			GW_gv_B_1x1x2x2x_1F_AI_m1.m_x.AtomicInc();

			GW_gv_B_1x1x2x2x_2F_AI_m0.write_addr(PR_htId);
			GW_gv_B_1x1x2x2x_2F_AI_m0.m_y[1].AtomicInc();

			GW_gv_B_1x1x2x2x_2F_AI_m1.write_addr(PR_htId);
			GW_gv_B_1x1x2x2x_2F_AI_m1.m_y[1].AtomicInc();

			GW_gv_B_1x1x2x2x_1F_AA_m0.write_addr(PR_htId, 3u);
			GW_gv_B_1x1x2x2x_1F_AA_m0.m_x.AtomicAdd(1u);

			GW_gv_B_1x1x2x2x_1F_AA_m1.write_addr(PR_htId, 3u);
			GW_gv_B_1x1x2x2x_1F_AA_m1.m_x = P_x;

			GW_gv_B_1x1x2x2x_4F_AA_m0.write_addr(PR_htId);
			GW_gv_B_1x1x2x2x_4F_AA_m0.m_x[3].AtomicAdd(1u);

			GW_gv_B_1x1x2x2x_4F_AA_m1.write_addr(PR_htId);
			GW_gv_B_1x1x2x2x_4F_AA_m1.m_x[3] = P_x;

			HtContinue(P_loopCnt++ < 100 ? GV4_LOOP : GV4_RETURN);
		}
		break;
		case GV4_RETURN:
		{
			if (SendReturnBusy_gv4()) {
				HtRetry();
				break;
			}

			SendReturn_gv4();
		}
		break;
		default:
			assert(0);
		}
	}
}
