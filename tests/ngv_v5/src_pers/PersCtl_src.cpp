#include "Ht.h"
#include "PersCtl.h"

void CPersCtl::PersCtl()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case ENTRY:
		{
					  if (SendCallBusy_gv1()) {
						  HtRetry();
						  break;
					  }

					  SendCall_gv1(CALL_GV2);
					  break;
		}
		case CALL_GV2:
		{
						 if (SendCallBusy_gv2()) {
							 HtRetry();
							 break;
						 }

						 SendCall_gv2(CALL_GV3);
						 break;
		}
		case CALL_GV3:
		{
						 if (SendCallBusy_gv3()) {
							 HtRetry();
							 break;
						 }

						 SendCall_gv3(RETURN, PR_addr);
						 break;
		}
		case RETURN:
		{
			if (SendReturnBusy_htmain()) {
				HtRetry();
				break;
			}

			SendReturn_htmain();
			break;
		}
		default:
			assert(0);
		}
	}
}
