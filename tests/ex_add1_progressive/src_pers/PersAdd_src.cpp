#include "Ht.h"
#include "PersAdd.h"

#define BUSY_RETRY(b) {if (b) {HtRetry(); break;}}

void
CPersAdd::PersAdd()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case ADD: {
			BUSY_RETRY(SendReturnBusy_htmain());

			uint64_t res = P_op1 + P_op2;
			SendReturn_htmain(res);
		}
		break;
		default:
			assert(0);
		}
	}
}
