#include "Ht.h"
#include "PersTst3.h"

void CPersTst3::PersTst3()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case TST3_ENTRY:
			{
				if (ReadMemBusy()) {
					HtRetry();
					break;
				}

				GW_gvar3.data = 0x35;

				ReadMem_gvar31(P_memAddr, PR_htId);

				ReadMemPause(TST3_RTN);
			}
			break;
		case TST3_RTN:
		{
			if (SendReturnBusy_tst3()) {
				HtRetry();
				break;
			}

			ht_uint8 rdData = GR_gvar3.data;

			if (rdData != 0x35)
				HtAssert(0, 0);

			rdData = GR_gvar31.data;

			if (rdData != 0x34)
				HtAssert(0, 0);

			SendReturn_tst3();
		}
		break;
		default:
			assert(0);
		}
	}
}
