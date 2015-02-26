#include "Ht.h"
#include "PersGv2.h"

void CPersGv2::PersGv2()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case GV2_ENTRY:
		{
			P_x = PR_htId;

			P_loopCnt = 0;
			HtContinue(GV2_LOOP);
		}
			break;
		case GV2_LOOP:
		{
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
