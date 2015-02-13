#include "Ht.h"
#include "PersT4.h"

void CPersT4::PersT4()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case T4_INIT:
		{
			P_loopCnt = 0;
			S_totalCnt = 0;

			uint64_t ht_noload tmp = GR_strSetL_data0();
			tmp++;

			HtContinue(T4_CALL);
		}
		break;
		case T4_CALL:
		{
			if (SendCallBusy_sum()) {
				HtRetry();
				break;
			}

			SendCallFork_sum(T4_JOIN);

			P_loopCnt += 1;

			if (P_loopCnt == T4_LOOP_CNT)
				RecvReturnPause_sum(T4_RTN);
			else
				HtContinue(T4_CALL);
		}
		break;
		case T4_JOIN:
		{
			S_totalCnt += P_sumCnt;

			RecvReturnJoin_sum();
		}
		break;
		case T4_RTN:
		{
			if (SendReturnBusy_t4()) {
				HtRetry();
				break;
			}

			SendReturn_t4(S_totalCnt);
		}
		break;
		default:
			assert(0);
		}
	}
}
