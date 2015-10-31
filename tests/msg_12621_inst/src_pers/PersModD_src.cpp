#include "Ht.h"
#include "PersModD.h"
#include "Pers_src.h"

void CPersModD::PersModD()
{
	HtAssert(!PR_htValid, 0);

	for (int i = 0; i < CTOD_FI; i += 1) {
		if (!GR_htReset && !RecvMsgBusy_CtoD(i)) {
			ht_uint4 msg = RecvMsg_CtoD(i);
			HtAssert((msg >> 2) == i_replId.read()+1u, msg);
			if (msg >= 0)
				S_msgRcvd |= (1 << ((msg - 1) % (CTOD_FI)+1)) & ((1 << (C_REPL / D_REPL + 1)) - 1);
		}
	}

	if (!GR_htReset && !RecvMsgBusy_BtoD(1)) {
		ht_uint4 msg = RecvMsg_BtoD(1);
		HtAssert(msg == (i_replId.read()+1u)<<2, msg);
		if (msg >= 0)
			S_msgRcvd |= 1;
	}

	if (!GR_htReset && S_msgRcvd == (1 << (CTOD_FI + 1)) - 1) {
		S_msgRcvd = 0;
		bool bMsgSendBusy = SendMsgBusy_DtoE(3);
		HtAssert(!bMsgSendBusy, 0);
		if (!bMsgSendBusy)
			SendMsg_DtoE(3, ((i_replId.read()+1) << 2) & 0xf);
	}
}
