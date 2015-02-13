#include "Ht.h"
#include "PersFork1.h"

void CPersFork1::PersFork1()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case FORK_ENTRY:
		{
			P_instCnt = S_instCnt;
			S_instCnt += 1;

			if (P_instCnt == 0)
				HtContinue(FORK_RETURN);
			else
				HtContinue(FORK_LOOP);
		}
		break;
		case FORK_LOOP:
		{
			P_instCnt -= 1;

			if (P_instCnt == 0)
				HtContinue(FORK_RETURN);
			else
				HtContinue(FORK_LOOP);
		}
		break;
		case FORK_RETURN:
		{
			if (SendReturnBusy_fork1()) {
				HtRetry();
				break;
			}
			SendReturn_fork1();
		}
		break;
		default:
			assert(0);
		}
	}

	if (GR_htReset)
		S_instCnt = 0;
}

