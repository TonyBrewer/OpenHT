#include "Ht.h"
#include "PersDispatch.h"

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

void
CPersDispatch::PersDispatch()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case DISPATCH_ENTRY: {
			BUSY_RETRY(SendReturnBusy_htmain());

			SendReturn_htmain();
		}
		break;
		default:
			assert(0);
		}
	}
}
