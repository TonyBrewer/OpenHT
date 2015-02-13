#include "Ht.h"
#include "PersLock.h"

void CPersLock::PersLock()
{
    if (PR_htValid) {
		switch (PR_htInst) {
		case LOCK:
			{
				if (SendReturnBusy_lock() || S_lock) {
					HtRetry();
					break;
				}

				S_lock = true;

				SendReturn_lock();
			}
			break;
		case UNLOCK:
			{
				if (SendReturnBusy_unlock()) {
					HtRetry();
					break;
				}

				HtAssert(SR_lock, 0);
				S_lock = false;

				SendReturn_unlock();
			}
			break;
		default:
			assert(0);
		}
    }
}
