#include "Ht.h"
#include "PersGv2.h"

void CPersGv2::PersGv2()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case GV2_ENTRY:
		{
			ht_uint1 idx1 = (PR_htId >> 2) & 1;
			ht_uint2 idx2 = PR_htId & 3;

			P_x = PR_htId;
			P_gvAddr = (PR_htId & 1) ^ 1;
			GW_Gvar[idx1][idx2].write_addr(P_gvAddr);
			GW_Gvar[idx1][idx2].m_x = P_x;

			P_loopCnt = 0;
			HtContinue(GV2_LOOP);
			break;
		}
		case GV2_LOOP:
		{
			ht_uint1 idx1 = (PR_htId >> 2) & 1;
			ht_uint2 idx2 = PR_htId & 3;

			if (GR_Gvar[idx1][idx2].m_x != PR_x)
				HtAssert(0, 0);

			P_x += 1;

			GW_Gvar[idx1][idx2].write_addr(PR_gvAddr);
#if USE_ATOMIC_INC == 0
			GW_Gvar[idx1][idx2].m_x = P_x;
#else
			GW_Gvar[idx1][idx2].m_x.AtomicInc();
#endif
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
