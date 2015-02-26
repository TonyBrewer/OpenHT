#include "Ht.h"
#include "PersCtl.h"

void CPersCtl::PersCtl()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case CTL_PRIV1:
		{
			if (SendCallBusy_priv1()) {
				HtRetry();
				break;
			}
			SendCall_priv1(CTL_PRIV2, PR_memAddr);
			break;
		}
		case CTL_PRIV2:
		{
			if (SendCallBusy_priv2()) {
				HtRetry();
				break;
			}

			P_err = PR_err || PR_rtnErr;

			SendCall_priv2(CTL_RTN, PR_memAddr);
			break;
		}
		case CTL_RTN:
		{
			if (SendReturnBusy_main()) {
				HtRetry();
				break;
			}

			P_err = PR_err || PR_rtnErr;

			SendReturn_main(P_err);
			break;
		}
		default:
			assert(0);
		}
	}
}
