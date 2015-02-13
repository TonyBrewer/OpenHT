#include "Ht.h"
#include "PersCtl.h"

void CPersCtl::PersCtl() {
  if (PR_htValid) {
    switch (PR_htInst) {

    case CTL_ENTRY: {
      if (SendCallBusy_echo()) {
	HtRetry();
	break;
      }
      
      if (P_count < P_length) {
	//printf("Sending %d\n", P_count);
	SendCallFork_echo(CTL_JOIN, P_count);
	P_count++;
	HtContinue(CTL_ENTRY);
      } else {
	RecvReturnPause_echo(CTL_RTN);
      }
    }
      break;
      
    case CTL_JOIN: {
      //printf("Got %d, adding to %d, ", P_result, P_sum);
      P_sum += P_result;
      //printf("result %d\n", P_sum);
      RecvReturnJoin_echo();
    }
      break;

    case CTL_RTN: {
      if (SendReturnBusy_htmain()) {
	HtRetry();
	break;
      }
      
      SendReturn_htmain(P_sum);
    }
      break;

    default:
      assert(0);

    }
  }
}
