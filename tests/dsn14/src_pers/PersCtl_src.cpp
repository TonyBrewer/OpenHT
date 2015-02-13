#include "Ht.h"
#include "PersCtl.h"

void CPersCtl::PersCtl()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case CTL_INIT:
		{
			P_loopCnt = 0;
			S_totalCnt = 0;

			HtContinue(CTL_T1_ASYNC);
		}
		break;
		case CTL_T1_ASYNC:
		{
			if (SendCallBusy_t1()) {
				HtRetry();
				break;
			}

			SendCallFork_t1(CTL_T1_JOIN);

			P_loopCnt += 1;

			if (P_loopCnt == CTL_LOOP_CNT)
				RecvReturnPause_t1(CTL_T1_CALL);
			else
				HtContinue(CTL_T1_ASYNC);
		}
		break;
		case CTL_T1_JOIN:
		{
			S_totalCnt += P_sumCnt;
			RecvReturnJoin_t1();
		}
		break;
		case CTL_T1_CALL:
		{
			if (SendCallBusy_t1()) {
				HtRetry();
				break;
			}

			assert(S_totalCnt == CTL_LOOP_CNT * T1_LOOP_CNT * SUM_LOOP_CNT);

			SendCall_t1(CTL_T2_ASYNC);

			P_loopCnt = 0;
			S_totalCnt = 0;
		}
		break;
		case CTL_T2_ASYNC:
		{
			if (SendCallBusy_t2()) {
				HtRetry();
				break;
			}

			assert(P_sumCnt == T1_LOOP_CNT * SUM_LOOP_CNT || P_loopCnt > 0);

			SendCallFork_t2(CTL_T2_JOIN);

			P_loopCnt += 1;

			if (P_loopCnt == CTL_LOOP_CNT) {
				P_loopCnt = 0;

				RecvReturnPause_t2(CTL_T2_CALL);
			} else {
				HtContinue(CTL_T2_ASYNC);
			}
		}
		break;
		case CTL_T2_JOIN:
		{
			S_totalCnt += P_sumCnt;
			RecvReturnJoin_t2();
		}
		break;
		case CTL_T2_CALL:
		{
			if (SendCallBusy_t2()) {
				HtRetry();
				break;
			}

			assert(S_totalCnt == CTL_LOOP_CNT * T2_LOOP_CNT * SUM_LOOP_CNT);

			SendCall_t2(CTL_T3_CALL);
		}
		break;
		case CTL_T3_CALL:
		{
			if (SendCallBusy_t3()) {
				HtRetry();
				break;
			}

			assert(P_sumCnt == T2_LOOP_CNT * SUM_LOOP_CNT);

			SendCall_t3(CTL_T4_CALL);
		}
		break;
		case CTL_T4_CALL:
		{
			if (SendCallBusy_t4()) {
				HtRetry();
				break;
			}

			assert(P_sumCnt == T3_LOOP_CNT * SUM_LOOP_CNT);

			SendCall_t4(CTL_RTN);
		}
		break;
		case CTL_RTN:
		{
			if (SendReturnBusy_htmain()) {
				HtRetry();
				break;
			}

			assert(P_sumCnt == T4_LOOP_CNT * SUM_LOOP_CNT);

			SendReturn_htmain();
		}
		break;
		default:
			assert(0);
		}
	}
}
