#include "Ht.h"
#include "PersSub.h"

#define BUSY_RETRY(b) {if (b) {HtRetry(); break;}}

#ifndef min
#define min(a,b) (((a)<(b)) ? (a) : (b))
#endif

void
CPersSub::PersSub()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case SUB_ENTRY:
			HtContinue(SUB_RETURN);
			break;
		case SUB_RETURN:
			BUSY_RETRY(SendReturnBusy_sub());
			SendReturn_sub();
			break;
		default:
			assert(0);
		}
	}
}

