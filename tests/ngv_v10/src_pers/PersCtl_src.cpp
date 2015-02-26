#include "Ht.h"
#include "PersCtl.h"

void CPersCtl::PersCtl()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case ENTRY:
		{
			if (SendCallBusy_gv1() || SendCallBusy_gv2()) {
				HtRetry();
				break;
			}

			SendCallFork_gv1(JOIN_GV1, PR_addr);
			SendCallFork_gv2(JOIN_GV2);

			//if (PR_forkCnt < 8)
				//HtContinue(ENTRY);
			//else
				HtContinue(PAUSE_GV1);

			P_forkCnt += 1;
			break;
		}
		case JOIN_GV1:
		{
			RecvReturnJoin_gv1();
			break;
		}
		case JOIN_GV2:
		{
			RecvReturnJoin_gv2();
			break;
		}
		case PAUSE_GV1:
		{
			RecvReturnPause_gv1(PAUSE_GV2);
			break;
		}
		case PAUSE_GV2:
		{
			RecvReturnPause_gv2(RETURN);
			break;
		}
		case RETURN:
		{
			if (SendReturnBusy_htmain()) {
				HtRetry();
				break;
			}

			SendReturn_htmain();
			break;
		}
		default:
			assert(0);
		}
	}
}
