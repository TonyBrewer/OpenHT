#include "Ht.h"
#include "PersStream.h"

void
CPersStream::PersStream()
{
	if (PR_htValid) {
		switch (PR_htInst) {

		// Main entry point from Main.cpp
		// P_rcvAu, P_rcvCnt are populated through call.
		case STRM_IDLE:
		{
			if (SendReturnBusy_htmain()) {
				HtRetry();
				break;
			}

#ifndef _HTV
			printf("SysC:  AU %2d - Processing\n", P_rcvAu);
#endif

			if (P_rcvCnt) {
				HtContinue(STRM_RECV);
			} else {
				// There are no calls to receive...simply return
				SendReturn_htmain(0);
			}
		}
		break;

		case STRM_RECV:
		{
			if (RecvHostDataBusy()) {
				HtRetry();
				break;
			}

			// Store received data into pricate variable recvData
			P_recvData = RecvHostData();

			HtContinue(STRM_SEND);
		}
		break;

		case STRM_SEND:
		{
			if (SendHostDataBusy() || SendReturnBusy_htmain()) {
				HtRetry();
				break;
			}

			// Generate an expected value to compare against received data
			uint64_t expectedData = 0LL;
			expectedData |= (uint64_t)((uint64_t)P_rcvAu << 48);
			expectedData |= (uint64_t)(P_wordCnt + 1);

			if (expectedData != P_recvData) {
#ifndef _HTV
			  printf("SysC:  WARNING - Expected Data did not match Received data!\n");
			  printf("         0x%016llx != 0x%016llx\n",
				 (unsigned long long)expectedData, (unsigned long long)P_recvData);
#endif
				P_errs += 1;
			}
			HtAssert(!P_errs, 0);

			// Send generated data back to the host
			// More error checking will be done there
			SendHostData(expectedData);

			P_wordCnt += 1;

			// Check count so far..
			// Either return to Main.cpp or continue reading in values
			if (P_wordCnt == P_rcvCnt) {
				SendReturn_htmain(P_errs);
			} else {
				HtContinue(STRM_RECV);
			}
		}
		break;
		default:
			assert(0);
		}
	}
}
