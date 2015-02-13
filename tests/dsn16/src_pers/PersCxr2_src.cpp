#include "Ht.h"
#include "PersCxr2.h"

void
CPersCxr2::PersCxr2()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case CXR2_0:
		{
			if (SendReturnBusy_cxr2() || SendTransferBusy_cxr2b()) {
				HtRetry();
				break;
			}

			P_trail = (uint64_t)(P_trail << 4);
			P_trail |= 1;

			if (P_instCnt <= 1)
				SendReturn_cxr2(P_trail);
			else
				SendTransfer_cxr2b(P_instCnt, P_trail);
		}
		break;
		default:
			assert(0);
		}
	}
}
