#include "Ht.h"
#include "PersCxr2c.h"

void
CPersCxr2c::PersCxr2c()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case CXR2c_0:
		{
			if (SendReturnBusy_cxr2()) {
				HtRetry();
				break;
			}

			P_trail |= 4;

			SendReturn_cxr2(P_trail);
		}
		break;
		default:
			assert(0);
		}
	}
}
