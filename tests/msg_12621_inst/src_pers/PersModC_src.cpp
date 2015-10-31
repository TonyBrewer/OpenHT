#include "Ht.h"
#include "PersModC.h"

void CPersModC::PersModC()
{
	HtAssert(!PR_htValid, 0);

	if (!GR_htReset && !RecvMsgBusy_BtoC()) {
		ht_uint4 msg = RecvMsg_BtoC();
		HtAssert((msg >> 2) == i_replId.read() / (BTOC_FI)+1u, msg);
		HtAssert((msg & 0x3) == i_replId.read() % (BTOC_FI)+1u, msg);

		bool bMsgSendBusy = SendMsgBusy_CtoD();
		HtAssert(!bMsgSendBusy, 0);
		if (!bMsgSendBusy)
			SendMsg_CtoD(msg);
	}
}
