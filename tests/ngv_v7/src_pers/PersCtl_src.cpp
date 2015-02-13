#include "Ht.h"
#include "PersCtl.h"

void CPersCtl::PersCtl()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case ENTRY:
		{
			if (SendCallBusy_gv1() || SendCallBusy_gv2() || SendCallBusy_gv3() || 
				SendCallBusy_gv4() || SendCallBusy_gv5() || SendCallBusy_gv6())
			{
				HtRetry();
				break;
			}

			SendCallFork_gv1(JOIN_GV1);
			SendCallFork_gv2(JOIN_GV2);
			SendCallFork_gv3(JOIN_GV3);
			SendCallFork_gv4(JOIN_GV4);
			SendCallFork_gv5(JOIN_GV5);
			SendCallFork_gv6(JOIN_GV6);

			if (PR_forkCnt < 8)
				HtContinue(ENTRY);
			else
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
		case JOIN_GV3:
		{
			RecvReturnJoin_gv3();
			break;
		}
		case JOIN_GV4:
		{
			RecvReturnJoin_gv4();
			break;
		}
		case JOIN_GV5:
		{
			RecvReturnJoin_gv5();
			break;
		}
		case JOIN_GV6:
		{
			RecvReturnJoin_gv6();
			break;
		}
		case PAUSE_GV1:
		{
			RecvReturnPause_gv1(PAUSE_GV2);
			break;
		}
		case PAUSE_GV2:
		{
			RecvReturnPause_gv2(PAUSE_GV3);
			break;
		}
		case PAUSE_GV3:
		{
			RecvReturnPause_gv3(PAUSE_GV4);
			break;
		}
		case PAUSE_GV4:
		{
			RecvReturnPause_gv4(PAUSE_GV5);
			break;
		}
		case PAUSE_GV5:
		{
			RecvReturnPause_gv5(PAUSE_GV6);
			break;
		}
		case PAUSE_GV6:
		{
			RecvReturnPause_gv6(RETURN);
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
