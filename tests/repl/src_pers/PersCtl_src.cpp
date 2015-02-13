#include "Ht.h"
#include "PersCtl.h"

void CPersCtl::PersCtl()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case ENTRY:
			{
				P_errorCnt = 0;
				P_loopCnt = 0;
				P_result = 0;

				if (P_loopCnt == P_loopCntLimit)
					HtContinue(LOOP_CHK);
				else
					HtContinue(CALL_A);
			}
			break;
		case CALL_A:
			{
				if (SendCallBusy_ModA()) {
					HtRetry();
					break;
				}

				SendCall_ModA(LOOP_CHK, 0, 1, P_result);
			}
			break;
		//case CALL_B:
		//	{
		//		if (SendCallBusy_ModB()) {
		//			HtRetry();
		//			break;
		//		}

		//		SendCall_ModB(LOOP_CHK, 4, 2, P_result);
		//	}
		//	break;
		case LOOP_CHK:
			{
				if (SendReturnBusy_htmain()) {
					HtRetry();
					break;
				}

				if (P_result != 0x01)
					P_errorCnt += 1;

				P_loopCnt += 1;

				if (P_loopCnt == P_loopCntLimit)
					SendReturn_htmain(P_errorCnt);
				else
					HtContinue(CALL_A);
			}
			break;
		default:
			assert(0);
		}
	}
}
