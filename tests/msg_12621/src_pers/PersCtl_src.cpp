#include "Ht.h"
#include "PersCtl.h"

void CPersCtl::PersCtl()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case CTL_INIT:
		{
			HtContinue(CTL_A);
		}
		break;
		case CTL_A:
		{
			if (SendCallBusy_modA()) {
				HtRetry();
				break;
			}

			SendCall_modA(CTL_B);
		}
		break;
		case CTL_B:
		{
			if (SendCallBusy_modB()) {
				HtRetry();
				break;
			}

			SendCall_modB(CTL_C);
		}
		break;
		case CTL_C:
		{
			if (SendCallBusy_modC()) {
				HtRetry();
				break;
			}

			SendCall_modC(CTL_D);
		}
		break;
		case CTL_D:
		{
			if (SendCallBusy_modD()) {
				HtRetry();
				break;
			}

			SendCall_modD(CTL_E);
		}
		break;
		case CTL_E:
		{
			if (SendCallBusy_modE()) {
				HtRetry();
				break;
			}

			SendCall_modE(CTL_SEND_MSG);
		}
		break;
		case CTL_SEND_MSG:
		{
			if (SendMsgBusy_CtlToA(2)) {
				HtRetry();
				break;
			}

			S_msgRcvd = false;

			// Send messages to modA
			SendMsg_CtlToA(2, 0);

			HtContinue(CTL_RTN);
		}
		break;
		case CTL_RTN:
		{
			if (SendReturnBusy_htmain()) {
				HtRetry();
				break;
			}

			if (!S_msgRcvd) {
				HtRetry();
				break;
			}

			// Return to host interface
			SendReturn_htmain(P_elemCnt);
		}
		break;
		default:
			assert(0);
		}
	}

	if (!GR_htReset && !RecvMsgBusy_EtoCtl()) {
		ht_uint4 msg = RecvMsg_EtoCtl();
		HtAssert(msg == 0, msg);
		S_msgRcvd = msg == 0;
	}
}
