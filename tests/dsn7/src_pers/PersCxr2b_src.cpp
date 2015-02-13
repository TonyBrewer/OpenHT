#include "Ht.h"
#include "PersCxr2b.h"

void
CPersCxr2b::PersCxr2b()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case CXR2b_5:
		{
			HtContinue(CXR2b_1);
		}
		break;
		case CXR2b_1:
		{
			if (SendReturnBusy_cxr2()) {
				HtRetry();
				break;
			}

			SendReturn_cxr2();

			break;
		}
		default:
			assert(0);
		}
	}
}
