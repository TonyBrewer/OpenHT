#include "Ht.h"
#include "PersCtl.h"

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

void
CPersCtl::PersCtl()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case CTL_ENTRY: {
			BUSY_RETRY(SendReturnBusy_htmain());

			SendReturn_htmain();
		}
		break;
		default:
			assert(0);
		}
	}

	// Async functionality
	if (!S_msgData.empty() && !SendHostMsgBusy()) {
		SendHostMsg(TO_HOST_MSG, (ht_uint56)S_msgData.front());
		S_msgData.pop();
	}
}
