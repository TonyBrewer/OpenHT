#include "Ht.h"
#include "PersIobk.h"

#ifndef _HTV
extern FILE *tfp;
#endif

void
CPersIobk::PersIobk()
{

	switch (SR_outState) {
	case 0: {
	  if (S_outQ.full() || (!S_outQ.empty() && (S_idleTimer == 0 || S_callDone))) {
	    S_outState = 1;
#ifndef _HTV
	    if (tfp) fprintf(tfp,"outState 0->1\n");
#endif
	  }
	  break;
	}
	case 1: {
	  if (S_flushCnt == 0) {
	    S_outState = 2;
	  } else if (S_outQ.empty()){
	    S_outState = 0;
	  } else if (!SendHostDataBusy()){
	    SendHostData(S_outQ.front());
#ifndef _HTV
	    if (tfp) 
	      fprintf(tfp,"outState 1: send 0x%016llx flushCnt=%d\n",
		      (long long)S_outQ.front(), (int)S_flushCnt);
#endif
	    S_outQ.pop();
	    S_flushCnt--;
	  }
	  break;
	}
	case 2: {
	  if (SendHostDataBusy()) {
	    break;
	  }
	  if (S_outQ.empty()) 
	    S_outState = 0;
	  else
	    S_outState = 1;
	  FlushHostData();
#ifndef _HTV
	  if (tfp) fprintf(tfp,"outState 2: flush\n");
#endif
	  S_flushCnt = S_flushArg;
	  break;
	}
	default: {
	    S_outState = 0;
	    //HtAssert(false,0);
	}
     	}

	S_callDone = false;
	if (PR_htValid) {
	  S_idleTimer--;

		switch (PR_htInst) {
		case IOBK_IDLE:
		{
			if (SendReturnBusy_htmain()) {
				HtRetry();
				break;
			}

			if (P_cnt) {
				HtContinue(IOBK_POP);
			} else {
#ifndef _HTV
				if (tfp) fprintf(tfp, "AU%d SendReturn(0)\n",
						 (int)P_au);
#endif
				SendReturn_htmain(0);
			}
		}
		break;
		case IOBK_POP:
		{
			if (RecvHostDataBusy()) {
				HtRetry();
				break;
			}

			P_recv = RecvHostData();
#ifndef _HTV
			if (tfp) fprintf(tfp, "AU%d RecvHostData(0x%016llx)\n",
					 (int)P_au, (long long)P_recv);
#endif

			HtContinue(IOBK_PUSH);
		}
		break;
		case IOBK_PUSH:
		{
		  if (/*SendHostDataBusy() || */S_outQ.full() || SendReturnBusy_htmain()) {
				HtRetry();
				break;
			}

			S_idleTimer = IDLE_CNT;

			uint64_t exp = 0;
			exp |= (uint64_t)((uint64_t)P_au << 56);
			exp |= (uint64_t)((uint64_t)P_call << 32);
			exp |= (uint64_t)P_word;

			if (exp != P_recv) {
#ifndef _HTV
				printf("ERROR: AU%d expected 0x%016llx != 0x%016llx\n",
				       (int)P_au, (long long)exp, (long long)P_recv);
#endif
				P_errs += 1;
			}
			HtAssert(!P_errs, 0);

#ifndef _HTV
			if (tfp) fprintf(tfp, "AU%d SendHostData(0x%016llx)\n",
					 (int)P_au, (long long)exp);
#endif
			//SendHostData(exp);
			S_outQ.push(exp);

			P_word += 1;

			if (!P_cnt || P_word == P_cnt) {
				S_callDone = true;
				HtContinue(IOBK_RETURN);
			} else {
				HtContinue(IOBK_POP);
			}
		}
		break;
		case IOBK_RETURN:
		  {
		    if (SendReturnBusy_htmain() ||!S_outQ.empty()) {
		      HtRetry();
		      break;
		    }
#ifndef _HTV
		    if (tfp) fprintf(tfp, "AU%d SendReturn(%d)\n",
				     (int)P_au, (int)P_errs);
#endif
		    SendReturn_htmain(P_errs);
		  }
		break;
		default:
			assert(0);
		}
	}

	if (GR_htReset) {
		S_flushArg = 100;
		S_flushCnt = 0;
		S_outState = 0;
		S_idleTimer = IDLE_CNT;
	}
}
