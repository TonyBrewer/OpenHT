#include "Ht.h"
#include "PersTest2.h"

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

void CPersTest2::PersTest2()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case TEST2_ENTRY: {
			S_called = true;

			BUSY_RETRY(SendReturnBusy_test2() || S_cnt < 128);

			SendReturn_test2();
			break;
		}
		default:
			assert(0);
		}
	}

	if (GR_htReset)
		S_called = false;

	if (!S_called)
		S_cnt = 0;
	else if (S_cnt < 128)
		S_cnt += 1;
}
