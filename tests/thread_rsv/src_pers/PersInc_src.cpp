#include "Ht.h"
#include "PersInc.h"

void CPersInc::PersInc()
{
    if (PR_htValid) {
		switch (PR_htInst) {
		case ENTRY:
			{
				HtBarrier(LOCK, 1 << INC_HTID_W);
			}
			break;
		case LOCK:
			{
				if (SendCallBusy_lock()) {
					HtRetry();
					break;
				}

				SendCall_lock(INC);
			}
			break;
		case INC: 
			{
				if (PR_wait < 10) {
					P_wait += 1;
					HtContinue(INC);
					break;
				}

				if (SendCallBusy_unlock()) {
					HtRetry();
					break;
				}

				S_cnt += 1;

				SendCall_unlock(BAR);
			}
			break;
		case BAR:
			{
				HtBarrier(RTN, 1 << INC_HTID_W);
			}
			break;
		case RTN: 
			{
				if (SendReturnBusy_htmain()) {
					HtRetry();
					break;
				}

				SendReturn_htmain(SR_cnt);
			}
			break;
		default:
			assert(0);
		}
	}
}
