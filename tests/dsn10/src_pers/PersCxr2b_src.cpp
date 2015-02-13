#include "Ht.h"
#include "PersCxr2b.h"

void CPersCxr2b::PersCxr2b()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case CXR2b_0:
		{
			if (SendReturnBusy_cxr2e1() || SendReturnBusy_cxr2e2() || SendTransferBusy_cxr2c()) {
				HtRetry();
				break;
			}

			P_trail |= 2;

			if (P_instCnt <= 2) {
				if (P_entry == 0)
					SendReturn_cxr2e1(P_trail);
				else
					SendReturn_cxr2e2(P_trail);
			} else {
				SendTransfer_cxr2c(P_instCnt, P_trail, P_entry);
			}
		}
		break;
		default:
			assert(0);
		}
	}
}
