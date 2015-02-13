#include "Ht.h"
#include "PersModA.h"

void CPersModA::PersModA()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case MODA:
		{
			if (SendReturnBusy_modA()) {
				HtRetry();
				break;
			}

			// Return to host interface
			SendReturn_modA();
		}
		break;
		default:
			assert(0);
		}
	}

	if (!GR_htReset && !RecvMsgBusy_CtlToA(2)) {
		ht_uint4 msg = RecvMsg_CtlToA(2);
		HtAssert(msg == 0, msg);

		for (int i = 0; i < B_REPL; i += 1) {
			bool bMsgSendBusyC = SendMsgBusy_AtoB(i);
			HtAssert(!bMsgSendBusyC, 0);
			if (!bMsgSendBusyC)
				SendMsg_AtoB(i, (i+1) << 2);
		}

		bool bMsgSendBusyE = SendMsgBusy_AtoE();
		HtAssert(!bMsgSendBusyE, 0);
		if (!bMsgSendBusyE)
			SendMsg_AtoE(msg);
	}
}
