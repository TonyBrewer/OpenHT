#include "Ht.h"
#include "PersCxr2.h"

void
CPersCxr2::PersCxr2()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case CXR2_4:
		{
			HtContinue(CXR2_1);
		}
		break;
		case CXR2_1:
		{
			if (SendTransferBusy_cxr2b()) {
				HtRetry();
				break;
			}

			SendTransfer_cxr2b();

			break;
		}
		default:
			assert(0);
		}
	}
}
