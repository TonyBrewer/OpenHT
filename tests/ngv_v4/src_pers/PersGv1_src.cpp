#include "Ht.h"
#include "PersGv1.h"

void CPersGv1::PersGv1()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case GV1_ENTRY:
		{
			ht_uint1 idx1 = (PR_htId >> 2) & 1;
			ht_uint2 idx2 = PR_htId & 3;

			P_gvAddr1 = idx1;
			P_gvAddr2 = idx2;

			P_x = PR_htId;
			bool bSelGvarD = !((PR_htId & 1) ^ 1);
			if (bSelGvarD)
				GW_GvarD[idx1][idx2].m_x = P_x;
			else {
				GW_GvarA.write_addr(P_gvAddr1, P_gvAddr2);
				GW_GvarA.m_x = P_x;
			}
			P_loopCnt = 0;
			HtContinue(GV1_LOOP);
		}
		break;
		case GV1_LOOP:
		{
			ht_uint1 idx1 = (PR_htId >> 2) & 1;
			ht_uint2 idx2 = PR_htId & 3;

			bool bSelGvarD = !((PR_htId & 1) ^ 1);
			if (bSelGvarD) {
				if (GR_GvarD[idx1][idx2].m_x != PR_x)
					HtAssert(0, 0);
			}
			else {
				if (GR_GvarA.m_x != PR_x)
					HtAssert(0, 0);
			}

			P_x += 1;

			if (bSelGvarD)
				GW_GvarD[idx1][idx2].m_x = P_x;
			else {
				GW_GvarA.write_addr(P_gvAddr1, P_gvAddr2);
				GW_GvarA.m_x = P_x;
			}
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
