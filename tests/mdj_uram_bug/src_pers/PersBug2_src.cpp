#include "Ht.h"
#include "PersBug2.h"

#define BUSY_RETRY(b) {if (b) {HtRetry(); break;}}

void
CPersBug2::PersBug2()
{
	if (PR4_htValid) {
		switch (PR4_htInst) {
		case BUG2_RETURN: {
			BUSY_RETRY(SendReturnBusy_bug2());

			SendReturn_bug2();
		}
		break;
		default:
			assert(0);
		}
	}
}
