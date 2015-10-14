#include "Ht.h"
#include "PersModE.h"
#include "Pers_src.h"

void CPersModE::PersModE()
{
	HtAssert(!PR_htValid, 0);

	if (GR_htReset)
		S_msgRcvd = 0;

	if (!GR_htReset && !RecvMsgBusy_AtoE()) {
		ht_uint4 msg = RecvMsg_AtoE();
		HtAssert(msg == 0, msg);
		if (msg == 0)
			S_msgRcvd |= 1;
	}

	for (int i = 0; i < D_REPL; i += 1) {
		if (!GR_htReset && !RecvMsgBusy_DtoE(3, i)) {
			ht_uint4 msg = RecvMsg_DtoE(3, i);
			HtAssert(msg == 4 || msg == 8, msg);
			if (msg >= 0)
				S_msgRcvd |= (2 << i) & ((1<<(D_REPL+1))-1);
		}
	}

	if (!GR_htReset && S_msgRcvd == (1<<(D_REPL+1))-1) {
		S_msgRcvd = 0;
		bool bMsgSendBusy = SendMsgBusy_EtoCtl();
		HtAssert(!bMsgSendBusy, 0);
		if (!bMsgSendBusy)
			SendMsg_EtoCtl(0);
	}
}
