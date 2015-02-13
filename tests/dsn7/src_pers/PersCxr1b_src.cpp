#include "Ht.h"
#include "PersCxr1b.h"

void
CPersCxr1b::PersCxr1b()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case CXR1b_2:
		{
			if (SendCallBusy_cxr2()) {
				HtRetry();
				break;
			}

			SendCall_cxr2(CXR1b_1);
		}
		break;
		case CXR1b_1:
		{
			if (SendReturnBusy_cxr1()) {
				HtRetry();
				break;
			}

			SendReturn_cxr1();

			break;
		}
		default:
			assert(0);
		}
	}
}
