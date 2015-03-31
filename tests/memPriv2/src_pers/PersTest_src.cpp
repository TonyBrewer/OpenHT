#include "Ht.h"
#include "PersTest.h"

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

void CPersTest::PersTest()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case TEST_ENTRY: {
			P_sum = 0;

			HtContinue(TEST_CALL);
			break;
		}
		case TEST_CALL: {
			BUSY_RETRY(SendCallBusy_test2());

			if (P_cnt == 8) {
				RecvReturnPause_test2(TEST_RTN);
				break;
			}

			SendCallFork_test2(TEST_JOIN);

			P_cnt += 1;
			HtContinue(TEST_CALL);
			break;
		}
		case TEST_JOIN: {
			P_sum = P_sum + 1;

			RecvReturnJoin_test2();
			break;
		}
		case TEST_RTN: {
			BUSY_RETRY(SendReturnBusy_main());

			SendReturn_main(P_sum != P_cnt);
			break;
		}
		default:
			assert(0);
		}
	}
}
