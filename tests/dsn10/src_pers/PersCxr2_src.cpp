#include "Ht.h"
#include "PersCxr2.h"

void CPersCxr2::PersCxr2()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case CXR2_0E1:
		{
			if (SendReturnBusy_cxr2e1() || SendTransferBusy_cxr2b()) {
				HtRetry();
				break;
			}

			P_trail = (uint64_t)(P_trail << 4);
			P_trail |= 1;

			if (P_instCnt <= 1)
				SendReturn_cxr2e1(P_trail);
			else
				SendTransfer_cxr2b(P_instCnt, P_trail, 0);
		}
		break;
		case CXR2_0E2:
		{
			if (SendReturnBusy_cxr2e2() || SendTransferBusy_cxr2b()) {
				HtRetry();
				break;
			}

			P_trail = (uint64_t)(P_trail << 4);
			P_trail |= 1;

			if (P_instCnt <= 1)
				SendReturn_cxr2e2(P_trail);
			else
				SendTransfer_cxr2b(P_instCnt, P_trail, 1);
		}
		break;
		default:
			assert(0);
		}
	}
}
