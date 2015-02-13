#include "Ht.h"
#include "PersGv1.h"

void CPersGv1::PersGv1()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case GV1_ENTRY:
		{
			P_x = PR_htId;
			P_gvAddr = PR_htId & 1;

			GW_Gvar[0][0].write_addr(P_gvAddr);
			GW_Gvar[0][0].m_x = P_x;

			P_loopCnt = 0;
			HtContinue(GV1_LOOP);
		}
		break;
		case GV1_LOOP:
		{
			if (GR_Gvar[0][0].m_x != PR_x)
				HtAssert(0, 0);

			P_x += 1;

			GW_Gvar[0][0].write_addr(PR_gvAddr);
#if USE_ATOMIC_INC == 0
			GW_Gvar[0][0].m_x = P_x;
#else
			GW_Gvar[0][0].m_x.AtomicInc();
#endif
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
