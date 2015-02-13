#include "Ht.h"
#include "PersCtl.h"

void
CPersCtl::PersCtl()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case CTL_ENTRY:
		{
			if (SendCallBusy_inc0() || SendCallBusy_inc1() || SendCallBusy_inc2() || SendCallBusy_inc3()
			    || SendCallBusy_inc4() || SendCallBusy_inc5() || SendCallBusy_inc6() || SendCallBusy_inc7()
			    || SendCallBusy_inc8() || SendCallBusy_inc9() || SendCallBusy_inc10() || SendCallBusy_inc11()
			    || SendCallBusy_inc12()) {
				HtRetry();
				break;
			}
#ifndef _HTV
			printf("started testId %d\n", (int)P_testId);
#endif

			switch (P_testId) {
			case 0:
				SendCall_inc0(CTL_RTN, P_loopBase, P_elemCnt);
				break;
			case 1:
				SendCall_inc1(CTL_RTN, P_loopBase, P_elemCnt);
				break;
			case 2:
				SendCall_inc2(CTL_RTN, P_loopBase, P_elemCnt);
				break;
			case 3:
				SendCall_inc3(CTL_RTN, P_loopBase, P_elemCnt);
				break;
			case 4:
				SendCall_inc4(CTL_RTN, P_loopBase, P_elemCnt);
				break;
			case 5:
				SendCall_inc5(CTL_RTN, P_loopBase, P_elemCnt);
				break;
			case 6:
				SendCall_inc6(CTL_RTN, P_loopBase, P_elemCnt);
				break;
			case 7:
				SendCall_inc7(CTL_RTN, P_loopBase, P_elemCnt);
				break;
			case 8:
				SendCall_inc8(CTL_RTN, P_loopBase, P_elemCnt);
				break;
			case 9:
				SendCall_inc9(CTL_RTN, P_loopBase, P_elemCnt);
				break;
			case 10:
				SendCall_inc10(CTL_RTN, P_loopBase, P_elemCnt);
				break;
			case 11:
				SendCall_inc11(CTL_RTN, P_loopBase, P_elemCnt);
				break;
			case 12:
				SendCall_inc12(CTL_RTN, P_loopBase, P_elemCnt);
				break;
			default:
				assert(0);
			}
			break;
		}
		case CTL_RTN:
		{
			if (SendReturnBusy_htmain()) {
				HtRetry();
				break;
			}

			SendReturn_htmain(P_elemCnt, P_threadId);
			break;
		}
		default:
			assert(0);
		}
	}
}
