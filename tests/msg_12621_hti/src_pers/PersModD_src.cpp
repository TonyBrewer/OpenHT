#include "Ht.h"
#include "PersModD.h"
#include "Pers_src.h"

void CPersModD::PersModD()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case MODD:
		{
			if (SendReturnBusy_modD()) {
				HtRetry();
				break;
			}

			// Return to host interface
			SendReturn_modD();
		}
		break;
		default:
			assert(0);
		}
	}

	for (int i = 0; i < C_REPL/D_REPL; i += 1) {
		if (!GR_htReset && !RecvMsgBusy_CtoD(i)) {
			ht_uint4 msg = RecvMsg_CtoD(i);
			HtAssert((msg >> 2) == i_replId.read()+1u, msg);
			if (msg >= 0)
				S_msgRcvd |= (1 << ((msg-1)%(C_REPL/D_REPL)+1)) & ((1 << (C_REPL/D_REPL+1))-1);
		}
	}

	if (!GR_htReset && !RecvMsgBusy_BtoD(1)) {
		ht_uint4 msg = RecvMsg_BtoD(1);
		HtAssert(msg == (i_replId.read()+1u)<<2, msg);
		if (msg >= 0)
			S_msgRcvd |= 1;
	}

	if (!GR_htReset && S_msgRcvd == (1<<(C_REPL/B_REPL+1))-1) {
		S_msgRcvd = 0;
		bool bMsgSendBusy = SendMsgBusy_DtoE(3);
		HtAssert(!bMsgSendBusy, 0);
		if (!bMsgSendBusy)
			SendMsg_DtoE(3, ((i_replId.read()+1) << 2) & 0xf);
	}
}
