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

				HtContinue(CALL_A);
			}
			break;
		case CALL_A:
			{
				if (P_loopCnt == P_loopCntLimit) {
					RecvReturnPause_ModA(RETURN);
					break;
				}

				if (SendCallBusy_ModA()) {
					HtRetry();
					break;
				}

				SendCallFork_ModA(CALL_A_JOIN, P_loopCnt & 0xff, 0);

				P_loopCnt += 1;
				
				HtContinue(CALL_A);
			}
			break;
		case CALL_A_JOIN:
			{
				ht_uint24 expData = 0x1B1B00 + P_rtnInData;
				if (expData != P_outData)
					P_errorCnt += 1;

				RecvReturnJoin_ModA();
			}
			break;
		case RETURN:
			{
				if (SendReturnBusy_htmain()) {
					HtRetry();
					break;
				}

				SendReturn_htmain(P_errorCnt);
			}
			break;
		default:
			assert(0);
		}
	}
}
