#include "Ht.h"
#include "PersVerify.h"

// Verify description
//  1. Read 64-bytes from coproc memory (initialized by host)
//  2. Read various alignments and qwCnts from coproc memory
//  3. Repeat steps 1 and 2 for host memory (initialized by host)

void CPersVerify::PersVerify()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case VERIFY0:
			{
				if (SendReturnBusy_verify()) {
					HtRetry();
					break;
				}

				bool bFailed = GR_gblArray_data() != P_verifyData;
				P_failed |= bFailed;
				HtAssert(!bFailed, 0);

				P_verifyData += 1;
				P_baseIdx += 1; // gblArray addr
				P_qwCnt -= 1;

				if (P_qwCnt == 0) {
					// Return to test module
					SendReturn_verify(!P_failed);
				} else {
					HtContinue(VERIFY0);
				}
			}
			break;
		default:
			assert(0);
		}
	}
}
