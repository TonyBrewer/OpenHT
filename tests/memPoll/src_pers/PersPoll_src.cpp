#include "Ht.h"
#include "PersPoll.h"

#if (POLL_HTID_W == 0)
#define PR_htId 0
#endif

void CPersPoll::PersPoll()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case POLL_POLL4:
		{
			if (SendCallBusy_poll4()) {
				HtRetry();
				break;
			}

			P_errRtn = 0;

			SendCall_poll4(POLL_POLL3, P_arrayAddr);
		}
		break;
		case POLL_POLL3:
		{
			if (SendCallBusy_poll3()) {
				HtRetry();
				break;
			}

			P_errRtn = 0;

			SendCall_poll3(POLL_POLL2, P_arrayAddr);
		}
		break;
		case POLL_POLL2:
		{
			if (SendCallBusy_poll2()) {
				HtRetry();
				break;
			}

			P_errRtn += P_err;

			SendCall_poll2(POLL_POLL1, P_arrayAddr);
		}
		break;
		case POLL_POLL1:
		{
			if (SendCallBusy_poll1()) {
				HtRetry();
				break;
			}

			P_errRtn += P_err;

			SendCall_poll1(POLL_POLL0, P_arrayAddr);
		}
		break;
		case POLL_POLL0:
		{
			if (SendCallBusy_poll0()) {
				HtRetry();
				break;
			}

			P_errRtn += P_err;

			SendCall_poll0(POLL_RTN, P_arrayAddr);
		}
		break;
		case POLL_RTN:
		{
			if (SendReturnBusy_htmain()) {
				HtRetry();
				break;
			}

			P_errRtn += P_err;

			SendReturn_htmain(P_errRtn);
		}
		break;
		default:
			assert(0);
		}
	}
#ifndef _HTV
	//printf("r_rdGrpRspCnt[0] = %d\n", (int)r_rdGrpRspCnt[0]);
#endif
}
