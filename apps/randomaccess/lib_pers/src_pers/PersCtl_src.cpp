#include "Ht.h"
#include "PersCtl.h"

#define BUSY_RETRY(b) {if (b) {HtRetry(); break;}}

void
CPersCtl::PersCtl()
{
    SR_tblIdx0.read_addr(PR_thrdIdx);
    SR_tblIdx1.read_addr(PR_thrdIdx);
    if (PR_htValid) {
        switch (PR_htInst) {
            case CTL_ENTRY: {
                BUSY_RETRY(SendCallBusy_GupsCore());
                SendCallFork_GupsCore(CTL_JOIN, (uint64_t)((uint64_t)SR_tblIdx1.read_mem() << 32) | SR_tblIdx0.read_mem());
                P_thrdIdx += 1;
                if (PR_thrdIdx < (ThreadCnt - 1))
                    HtContinue(CTL_ENTRY);
                else
                    RecvReturnPause_GupsCore(CTL_RTN);
		break;
            }
            case CTL_JOIN: {
                RecvReturnJoin_GupsCore();
		break;
            }
            case CTL_RTN: {
                BUSY_RETRY(SendReturnBusy_htmain());
                SendReturn_htmain();
		break;
            }
            default:
                assert(0);
        }
    }
}
