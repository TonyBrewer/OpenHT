#include "Ht.h"
#include "PersCtl.h"

void CPersCtl::PersCtl()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case CTL_INIT:
		{
			HtContinue(CTL_SEND_MSG_0E);
		}
		break;
		case CTL_SEND_MSG_0E:
		{
			if (SendMsgBusy_CtlTo0A(2)) {
				HtRetry();
				break;
			}

			S_msgRcvd0E = false;

			// Send messages to modA
			SendMsg_CtlTo0A(2, 0);

			HtContinue(CTL_SEND_MSG_1E);
		}
		break;
		case CTL_SEND_MSG_1E:
		{
			if (SendMsgBusy_CtlTo1A(2)) {
				HtRetry();
				break;
			}

			S_msgRcvd1E = false;

			// Send messages to modA
			SendMsg_CtlTo1A(2, 0);

			HtContinue(CTL_RTN);
		}
		break;
		case CTL_RTN:
		{
			if (SendReturnBusy_htmain()) {
				HtRetry();
				break;
			}

			if (!S_msgRcvd0E || !S_msgRcvd1E) {
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

	if (!GR_htReset && !RecvMsgBusy_E0toCtl()) {
		ht_uint4 msg = RecvMsg_E0toCtl();
		HtAssert(msg == 0, msg);
		S_msgRcvd0E = msg == 0;
	}

	if (!GR_htReset && !RecvMsgBusy_E1toCtl()) {
		ht_uint4 msg = RecvMsg_E1toCtl();
		HtAssert(msg == 0, msg);
		S_msgRcvd1E = msg == 0;
	}
}
