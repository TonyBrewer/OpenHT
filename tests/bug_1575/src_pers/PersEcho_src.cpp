#include "Ht.h"
#include "PersEcho.h"

void CPersEcho::PersEcho() {
  if (PR_htValid) {
    switch (PR_htInst) {

    case ECHO_RTN: {
      if (SendReturnBusy_echo()) {
	HtRetry();
	break;
      }
      //printf("Returning %d\n", P_count);
      
      SendReturn_echo(P_count);
    }
      break;

    default:
      assert(0);

    }
  }
}
