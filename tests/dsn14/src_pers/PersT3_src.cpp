#include "Ht.h"
#include "PersT3.h"

void CPersT3::PersT3()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case T3_INIT:
		{
			P_loopCnt = 0;
			S_totalCnt = 0;

			HtContinue(T3_CALL);
		}
		break;
		case T3_CALL:
		{
			if (SendCallBusy_sum()) {
				HtRetry();
				break;
			}

			SendCallFork_sum(T3_JOIN);

			P_loopCnt += 1;

			if (P_loopCnt == T3_LOOP_CNT)
				RecvReturnPause_sum(T3_RTN);
			else
				HtContinue(T3_CALL);
		}
		break;
		case T3_JOIN:
		{
			S_totalCnt += P_sumCnt;

			RecvReturnJoin_sum();
		}
		break;
		case T3_RTN:
		{
			if (SendReturnBusy_t3()) {
				HtRetry();
				break;
			}

			SendReturn_t3(S_totalCnt);
		}
		break;
		default:
			assert(0);
		}
	}
}
