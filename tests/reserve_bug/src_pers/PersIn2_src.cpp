#include "Ht.h"
#include "PersIn2.h"

#define BUSY_RETRY(b) {if (b) {HtRetry(); break;}}

#ifndef min
#define min(a,b) (((a)<(b)) ? (a) : (b))
#endif

void
CPersIn2::PersIn2()
{
    if (PR_htValid) {
        switch (PR_htInst) {
            case IN2_ENTER:
                S_xIdx = 0;
                S_xDimLen = PR_xDimLen;
                S_sum = 0;
                S_addrB += (uint32_t)(PR_yDimOff * PR_xDimLen * TYPE_SIZE);
                HtContinue(IN2_OPEN);
                break;

            case IN2_OPEN:
                // Open read stream B, once
                if (!ReadStreamBusy_B())
                    ReadStreamOpen_B(SR_addrB, PR_xDimLen);
		S_inBbusy = true;
                HtContinue(IN2_RETURN);
                break;

            case IN2_RETURN: {
		BUSY_RETRY(SR_inBbusy);
                BUSY_RETRY(SendReturnBusy_in2());
                SendReturn_in2();
            }
		break;

            default:
                assert(0);
        }
    }
    
    if (ReadStreamReady_B())
    {
        GW_bBuf.write_addr(SR_xIdx);
        GW_bBuf = ReadStream_B();
        S_xIdx += 1;
	S_inBbusy = !ReadStreamLast_B();
    }
}
