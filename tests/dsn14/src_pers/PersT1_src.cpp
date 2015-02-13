#include "Ht.h"
#include "PersT1.h"

void CPersT1::PersT1()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case T1_INIT:
		{
			P_loopCnt = 0;
			S_totalCnt = 0;

			HtContinue(T1_CALL);
		}
		break;
		case T1_CALL:
		{
			if (SendCallBusy_sum()) {
				HtRetry();
				break;
			}

			SendCallFork_sum(T1_JOIN);

			P_loopCnt += 1;

			if (P_loopCnt == T1_LOOP_CNT)
				RecvReturnPause_sum(T1_RTN);
			else
				HtContinue(T1_CALL);
		}
		break;
		case T1_JOIN:
		{
			S_totalCnt += P_sumCnt;

			RecvReturnJoin_sum();
		}
		break;
		case T1_RTN:
		{
			if (SendReturnBusy_t1()) {
				HtRetry();
				break;
			}

			SendReturn_t1(S_totalCnt);
		}
		break;
		default:
			assert(0);
		}
	}
}
