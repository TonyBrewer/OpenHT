#include "Ht.h"
#include "PersT2.h"

void CPersT2::PersT2()
{
	T1_cmdQueSize = m_htCmdQue.size();

	if (PR_htValid) {

#ifndef _HTV
		if (HT_CYCLE() >= 14870)
			bool stop = true;
#endif
		switch (PR_htInst) {
		case T2_INIT:
		{
			P_loopCnt = 0;
			S_totalCnt[INT(PR_htId)] = 0;

			HtContinue(T2_CALL);
		}
		break;
		case T2_CALL:
		{
			if (SendCallBusy_sum()) {
				HtRetry();
				break;
			}

			SendCallFork_sum(T2_JOIN);

			P_loopCnt += 1;

			printf("   P_loopCnt = %d\n", (int)P_loopCnt);

			if (P_loopCnt == T2_LOOP_CNT)
				RecvReturnPause_sum(T2_RTN);
			else
				HtContinue(T2_CALL);
		}
		break;
		case T2_JOIN:
		{
			S_totalCnt[INT(PR_htId)] += P_sumCnt;

			RecvReturnJoin_sum();
		}
		break;
		case T2_RTN:
		{
			if (SendReturnBusy_t2()) {
				HtRetry();
				break;
			}

			SendReturn_t2(S_totalCnt[INT(PR_htId)]);
		}
		break;
		default:
			assert(0);
		}
	}
}
