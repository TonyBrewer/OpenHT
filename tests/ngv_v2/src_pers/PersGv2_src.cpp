#include "Ht.h"
#include "PersGv2.h"

void CPersGv2::PersGv2()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case GV2_ENTRY:
		{
			P_x = PR_htId;
			P_gvAddr = (PR_htId & 1) ^ 1;
			GW_gvar.write_addr(P_gvAddr);
			GW_gvar.m_x[PR_htId] = P_x;

			P_loopCnt = 0;
			HtContinue(GV2_LOOP);
		}
			break;
		case GV2_LOOP:
		{
			if (GR_gvar.m_x[PR_htId] != PR_x)
				HtAssert(0, 0);

			GW_gvar.write_addr(PR_gvAddr);
			GW_gvar.m_x[PR_htId].AtomicInc();
			P_x += 1;

			HtContinue(P_loopCnt++ < 100 ? GV2_LOOP : GV2_RETURN);
		}
			break;
		case GV2_RETURN:
		{
			if (SendReturnBusy_gv2()) {
				HtRetry();
				break;
			}

			SendReturn_gv2();
		}
		break;
		default:
			assert(0);
		}
	}
}
