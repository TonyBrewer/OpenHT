#include "Ht.h"
#include "PersHello.h"
void
CPersHello::PersHello()
{
	if (PR1_htValid) {
		switch (PR1_htInst) {
		case HELLO_WRITE: {
		}
		break;
		case HELLO_RTN: {
		}
		break;
		default:
			assert(0);
		}
	}
	if (PR2_htValid) {
		switch (PR2_htInst) {
		case HELLO_WRITE: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem(P2_pBuf, (uint64_t)0x006f6c6c6548LL /*Hello*/);
			WriteMemPause(HELLO_RTN);
		}
		break;
		case HELLO_RTN: {
			if (SendReturnBusy_htmain()) {
				HtRetry();
				break;
			}

			SendReturn_htmain();
		}
		break;
		default:
			assert(0);
		}
	}
}
