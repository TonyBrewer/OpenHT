#include "Ht.h"
#include "PersCtl.h"

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

void CPersCtl::PersCtl()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case CTL_ENTRY:
		{
			S_bSendCtlMsg = true;

			HtContinue(CTL_WAIT);
		}
		break;
		case CTL_WAIT:
		{
			BUSY_RETRY(!SR_bRcvdCtlMsg);

			HtContinue(CTL_RETURN);
		}
		break;
		case CTL_RETURN:
		{
			BUSY_RETRY(SendReturnBusy_htmain());

			// Return to host interface
			SendReturn_htmain();
		}
		break;
		default:
			assert(0);
		}
	}

	if (SR_bSendCtlMsg) {
		S_bSendCtlMsg = false;
		CCtlMsg ctlMsg;
		ctlMsg.m_unit = SR_unitId;
		ctlMsg.m_hops = 0;
		SendMsg_mi(ctlMsg);
	} else if (RecvMsgReady_mi()) {
		CCtlMsg ctlMsg = RecvMsg_mi();
		if (ctlMsg.m_unit == SR_unitId) {
			assert_msg(ctlMsg.m_hops == HT_UNIT_CNT * HT_AE_CNT - 1, "ctlMsg.hops = %d", ctlMsg.m_hops);
			S_bRcvdCtlMsg = true;
		} else if (!SendMsgBusy_mi()) {
			ctlMsg.m_hops += 1;
			SendMsg_mi(ctlMsg);
		}
	}
}
