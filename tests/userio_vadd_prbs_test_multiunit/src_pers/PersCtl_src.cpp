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

			S_statusCnt = 0;
			S_active = true;

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
			BUSY_RETRY(SendMsgBusy_initMsg());

			SendMsg_initMsg(true);
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

			S_active = false;

			SendReturn_htmain(PR_error0,
					  PR_error1,
					  PR_error2,
					  PR_error3);
		}
		break;
		default:
			assert(0);
		}
	}

	// Status Link (Link 8/9)
	{
		// Get Data
		packet_t inPacket;
		if (RecvUioReady_status() && SR_active) {
			inPacket = RecvUioData_status();

			// Only send to host every X cycles
			if (SR_statusCnt == 100000) {
				if (!SendHostMsgBusy()) {
					SendHostMsg(STATUS, (ht_uint56)inPacket.data.lower);
					S_statusCnt = 0;
				}
			}


			else {
				S_statusCnt = SR_statusCnt + 1;
			}
		}
	}
}
