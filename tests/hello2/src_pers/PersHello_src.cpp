#include "Ht.h"
#include "PersHello.h"

void CPersHello::PersHello()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case HELLO_WRITE: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem(P_pBuf, (uint64_t)0x006f6c6c6548LL /*Hello*/);
			WriteMemPause(HELLO_RTN);
		}
		break;
		case HELLO_RTN: {
			if (SendReturnBusy_htmain()) {
				HtRetry();
				break;
			}

			if (PR_hs.a != 0x45454545 || PR_hs.b != 0x3423 || PR_hs.d != 0x92929292 ||
				PR_hs.c[0] != 0x12 || PR_hs.c[1] != 0x67 || PR_hs.c[2] != -34)
				HtAssert(0, 0);

			SendReturn_htmain(PR_hs, PR_pBuf, 0);
		}
		break;
		default:
			assert(0);
		}
	}
}
