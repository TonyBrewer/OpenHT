#include "Ht.h"
#include "PersTest.h"

void CPersTest::PersTest()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case CALL:
		{
			P_err = SR_msgVar != 1;

			HtContinue(RTN);
		}
		break;
		case RTN:
		{
			if (SendReturnBusy_htmain()) {
				HtRetry();
				break;
			}

			SendReturn_htmain(P_err, P_arg0, P_arg1, P_arg2, P_arg3, P_arg4,
					  P_arg5, P_arg6, P_arg7, P_arg8, P_arg9);
		}
		break;
		default:
			assert(0);
		}
	}
}
