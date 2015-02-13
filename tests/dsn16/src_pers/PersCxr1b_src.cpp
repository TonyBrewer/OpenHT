#include "Ht.h"
#include "PersCxr1b.h"

void
CPersCxr1b::PersCxr1b()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case CXR1b_0:
		{
			if (SendCallBusy_cxr2()) {
				HtRetry();
				break;
			}

			P_trail = (uint64_t)(P_trail << 4) | 0xc;

			SendCall_cxr2(CXR1b_1, 2, P_trail);
		}
		break;
		case CXR1b_1:
		{
			if (SendCallBusy_cxr2()) {
				HtRetry();
				break;
			}

			P_trail = (uint64_t)(P_trail << 4) | 0xd;

			SendCall_cxr2(CXR1b_2, 3, P_trail);
		}
		break;
		case CXR1b_2:
		{
			if (SendReturnBusy_htmain()) {
				HtRetry();
				break;
			}

			P_trail = (uint64_t)(P_trail << 4) | 0xe;

			SendReturn_htmain(P_trail);

			break;
		}
		default:
			assert(0);
		}
	}
}
