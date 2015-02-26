#include "Ht.h"
#include "PersTst2.h"
void
CPersTst2::PersTst2()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case TST2_READ:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			P_gvarAddr = 1;
			ReadMem_gvar(P_memAddr + 8, P_gvarAddr, P_gvarAddr, 0);

			ReadMemPause(TST2_RTN);
		}
		break;
		case TST2_RTN:
		{
			if (SendReturnBusy_tst2()) {
				HtRetry();
				break;
			}

			if (GR_gvar[0].data != 0xdeadbeef00002345ULL) {
				HtAssert(0, 0);
				P_err += 1;
			}

			SendReturn_tst2(P_err);
		}
		break;
		default:
			assert(0);
		}
	}
}
