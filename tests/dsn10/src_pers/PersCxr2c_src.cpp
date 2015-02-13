#include "Ht.h"
#include "PersCxr2c.h"

void CPersCxr2c::PersCxr2c()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case CXR2c_0:
		{
			if (SendReturnBusy_cxr2e1() || SendReturnBusy_cxr2e2()) {
				HtRetry();
				break;
			}

			P_trail |= 4;

			if (P_entry == 0)
				SendReturn_cxr2e1(P_trail);
			else
				SendReturn_cxr2e2(P_trail);
		}
		break;
		default:
			assert(0);
		}
	}
}
