#include "Ht.h"
#include "PersBug.h"

#define BUSY_RETRY(b) {if (b) {HtRetry(); break;}}

void
CPersBug::PersBug()
{
	if (PR4_htValid) {
		switch (PR4_htInst) {
		case BUG_RETURN: {
			BUSY_RETRY(SendReturnBusy_htmain());

			SendReturn_htmain();
		}
		break;
		default:
			assert(0);
		}
	}
}
