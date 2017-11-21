#include "Ht.h"
#include "PersCtl.h"

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

void
CPersCtl::PersCtl()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case CTL_SEND: {
			BUSY_RETRY(SendCallBusy_send());

			SendCallFork_send(CTL_JOIN_SEND);
			HtContinue(CTL_RECV);
		}
		break;
		case CTL_RECV: {
			BUSY_RETRY(SendCallBusy_recv());

			SendCallFork_recv(CTL_JOIN_RECV);
			HtContinue(CTL_START);
		}
		break;
		case CTL_START: {
			BUSY_RETRY(SendMsgBusy_startMsg());

			SendMsg_startMsg(true);
			RecvReturnPause_send(CTL_SEND_RTN);
		}
		break;
		case CTL_JOIN_SEND: {
			RecvReturnJoin_send();
		}
		break;
		case CTL_JOIN_RECV: {
			RecvReturnJoin_recv();
		}
		break;
		case CTL_SEND_RTN: {
			RecvReturnPause_recv(CTL_RTN);
		}
		break;
		case CTL_RTN: {
			BUSY_RETRY(SendReturnBusy_htmain());

			SendReturn_htmain();
		}
		break;
		default:
			assert(0);
		}
	}
}
