#include "Ht.h"
#include "PersModB.h"

void CPersModB::PersModB()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case MODB:
		{
			if (SendReturnBusy_modB()) {
				HtRetry();
				break;
			}

			// Return to host interface
			SendReturn_modB();
		}
		break;
		default:
			assert(0);
		}
	}

	if (!GR_htReset && !RecvMsgBusy_AtoB()) {
		ht_uint4 msg = RecvMsg_AtoB();
		HtAssert((msg & 0x3) == 0 && (msg >> 2) == i_replId.read()+1u, msg);

		for (int i = 0; i < C_REPL/B_REPL; i += 1) {
			bool bMsgSendBusyC = SendMsgBusy_BtoC(i);
			HtAssert(!bMsgSendBusyC, 0);
			if (!bMsgSendBusyC)
				SendMsg_BtoC(i, msg + 1 + i);
		}

		bool bMsgSendBusyD = SendMsgBusy_BtoD(1);
		HtAssert(!bMsgSendBusyD, 0);
		if (!bMsgSendBusyD)
			SendMsg_BtoD(1, msg);
	}
}
