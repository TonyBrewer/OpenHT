#include "Ht.h"
#include "PersHello.h"

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

void
CPersHello::PersHello()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case HELLO_WRITE: {
			BUSY_RETRY(WriteMemBusy());

			WriteMem(P_pBuf, (uint64_t)0x006f6c6c6548LL /*Hello*/);
			WriteMemPause(HELLO_RTN);
		}
		break;
		case HELLO_RTN: {
			BUSY_RETRY(SendReturnBusy_htmain());

			SendReturn_htmain();
		}
		break;
		default:
			assert(0);
		}
	}
}
