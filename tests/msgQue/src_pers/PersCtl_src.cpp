#include "Ht.h"
#include "PersCtl.h"

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

void CPersCtl::PersCtl()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case CTL_ENTRY:
		{
			P_loopCnt = 0;
			P_errCnt = 0;

			HtContinue(CTL_FORK);
		}
		break;
		case CTL_FORK:
		{
			BUSY_RETRY(SendCallBusy_src1() || SendCallBusy_dst1());

			SendCallFork_src1(CTL_JOIN, 0);
			SendCallFork_dst1(CTL_JOIN, 1);

			if (P_loopCnt == 10)
				RecvReturnPause_dst1(CTL_RETURN);
			else {
				P_loopCnt += 1;
				RecvReturnPause_dst1(CTL_FORK);
			}
		}
		break;
		case CTL_JOIN:
		{
			switch (P_modId) {
			case 0:
				RecvReturnJoin_src1();
				break;
			case 1:
				RecvReturnJoin_dst1();
				break;
			default:
				assert(0);
			}

			P_errCnt += P_error;
		}
		break;
		case CTL_RETURN:
		{
			BUSY_RETRY(SendReturnBusy_htmain());

			// Return to host interface
			SendReturn_htmain(P_errCnt != 0);
		}
		break;
		default:
			assert(0);
		}
	}
}
