#include "Ht.h"
#include "PersCtl.h"

//
// Test dimensions
//  Number of threads
//  Number of declared barriers
//  Number of active barriers
//  Module replication
//  Thread pause/resume
//  

void CPersCtl::PersCtl()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case CTL_ENTRY:
			{
				HtContinue(CTL_FORK);
			}
			break;
		case CTL_FORK:
		{
			if (SendCallBusy_barrier()) {
				HtRetry();
				break;
			}

			uint16_t threadCnt = (1 << BARRIER_HTID_W) * BARRIER_REPL_CNT;
			SendCallFork_barrier(CTL_JOIN, threadCnt, P_threadCnt);

			// Check if end of forks
			P_threadCnt += 1;
			if (P_threadCnt == threadCnt)
				// Return to host interface
				RecvReturnPause_barrier(CTL_LOOP);
			else
				HtContinue(CTL_FORK);
		}
		break;
		case CTL_JOIN:
		{
			RecvReturnJoin_barrier();

			if (P_error)
				P_errCnt += 1;
		}
		break;
		case CTL_LOOP:
		{
			if (SendReturnBusy_htmain()) {
				HtRetry();
				break;
			}

			SendMsg_clr(true);

			P_testCnt += 1;
			P_threadCnt = 0;

			if (P_testCnt == 50)
				SendReturn_htmain(P_errCnt);
			else
				HtContinue(CTL_FORK);
		}
		break;
		default:
			assert(0);
		}
	}
}
