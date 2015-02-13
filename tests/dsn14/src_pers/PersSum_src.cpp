#include "Ht.h"
#include "PersSum.h"

void CPersSum::PersSum()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case SUM_INIT:
		{
			P_loopCnt = 0;

			HtContinue(SUM_LOOP);
		}
		break;
		case SUM_LOOP:
		{
			if (SendReturnBusy_sum()) {
				HtRetry();
				break;
			}

			P_loopCnt += 1;

			if (P_loopCnt == SUM_LOOP_CNT)
				SendReturn_sum((uint32_t)P_loopCnt);
			else
				HtContinue(SUM_LOOP);
		}
		break;
		default:
			assert(0);
		}
	}
}
