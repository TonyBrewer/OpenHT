#include "Ht.h"
#include "PersCxr1.h"

void
CPersCxr1::PersCxr1()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case CXR1_0:
		{
			if (SendCallBusy_cxr2()) {
				HtRetry();
				break;
			}

			SendCall_cxr2(CXR1_1);
		}
		break;
		case CXR1_1:
		{
			if (SendTransferBusy_cxr1b()) {
				HtRetry();
				break;
			}

			SendTransfer_cxr1b();

			break;
		}
		default:
			assert(0);
		}
	}
}
