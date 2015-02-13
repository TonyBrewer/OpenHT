#include "Ht.h"
#include "PersInc2.h"

void CPersInc2::PersInc2()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case INC2_INIT:
		{
			HtContinue(INC2_INIT);
		}
		break;
		default:
			assert(0);
		}
	}
}
