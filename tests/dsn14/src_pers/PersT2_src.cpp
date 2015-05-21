#include "Ht.h"
#include "PersT2.h"

void CPersT2::PersT2()
{
	T1_cmdQueSize = m_htCmdQue.size();

	if (PR1_htValid) {

#ifndef _HTV
		if (HT_CYCLE() >= 14870)
			bool stop = true;
#endif
		switch (PR1_htInst) {
		case T2_INIT:
		{
			P1_loopCnt = 0;
			S_totalCnt[INT(PR1_htId)] = 0;

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

			P1_loopCnt += 1;

			printf("   P_loopCnt = %d\n", (int)P1_loopCnt);

			if (P1_loopCnt == T2_LOOP_CNT)
				RecvReturnPause_sum(T2_RTN);
			else
				HtContinue(T2_CALL);
		}
		break;
		case T2_JOIN:
		{
			S_totalCnt[INT(PR1_htId)] += P1_sumCnt;

			RecvReturnJoin_sum();
		}
		break;
		case T2_RTN:
		{
			if (SendReturnBusy_t2()) {
				HtRetry();
				break;
			}

			SendReturn_t2(S_totalCnt[INT(PR1_htId)]);
		}
		break;
		default:
			assert(0);
		}
	}
}
