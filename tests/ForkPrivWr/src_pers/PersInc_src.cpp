#include "Ht.h"
#include "PersInc.h"

void CPersInc::PersInc()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case INC_INIT:
		{
			S_loopCnt[PR_htId] = 0;
			S_instCnt[PR_htId] = 0;
			S_joinCnt[PR_htId] = 0;

			P_instCnt = 0;
			P_joinCnt = 0;

			uint64_t data = GR_arrayMem.data.u64;
			GW_arrayMem.write_addr(S_joinCnt[PR_htId]);
			GW_arrayMem.data.u64 = data;

			HtContinue(INC_LOOP1);
		}
		break;
		case INC_LOOP1:
		{
			// start two forked calls and one non-forked call
			if (SendCallBusy_fork1(1) || SendCallBusy_fork2() || SendCallBusy_call()) {
				HtRetry();
				break;
			}

			if (S_loopCnt[PR_htId] == 100) {
				HtContinue( INC_RETURN );
				break;
			}

			S_loopCnt[PR_htId] += 1;

			S_instCnt[PR_htId] += 1;
			P_instCnt += 1;

			SendCallFork_fork1(INC_JOIN1, 1);
			SendCallFork_fork2(INC_JOIN2);
			SendCall_call( INC_LOOP2 );
		}
		break;
		case INC_LOOP2:
		{
			S_instCnt[PR_htId] += 1;
			P_instCnt += 1;

			RecvReturnPause_fork1( INC_LOOP3, 1 );
		}
		break;
		case INC_LOOP3:
		{
			S_instCnt[PR_htId] += 1;
			P_instCnt += 1;

			RecvReturnPause_fork2( INC_LOOP1 );
		}
		break;
		case INC_JOIN1:
		{
			S_joinCnt[PR_htId] += 1;
			P_joinCnt += 1;

			RecvReturnJoin_fork1(1);
		}
		break;
		case INC_JOIN2:
		{
			S_joinCnt[PR_htId] += 1;
			P_joinCnt += 1;

			RecvReturnJoin_fork2();
		}
		break;
		case INC_RETURN:
		{
			if (SendReturnBusy_htmain()) {
				HtRetry();
				break;
			}

			bool bError = S_instCnt[PR_htId] != P_instCnt || S_joinCnt[PR_htId] != P_joinCnt;

			SendReturn_htmain(bError);
		}
		break;
		default:
			assert(0);
		}
	}
}
