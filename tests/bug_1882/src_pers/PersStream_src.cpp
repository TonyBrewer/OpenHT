#include "Ht.h"
#include "PersStream.h"

// Host -> Coprocessor Data Streaming Example
// Echo 8B quantities until a HostDataMarker is received
// Return number of bytes processed

void
CPersStream::PersStream()
{
	if (PR_htValid) {
		switch (PR_htInst) {

		// Main entry point from Host (htmain call)
		// P_rcvData => 0 (init)
		case STRM_RECV:
		{
			if (RecvHostDataBusy()) {
				HtRetry();
				break;
			}
			
			// Receive data until we see a DataMarker from the host
			if (RecvHostDataMarker()) {
				HtContinue(STRM_RTN);
			} else {
				// Store received data into private variable recvData
				P_rcvData = RecvHostData();
				HtContinue(STRM_ECHO);
			}
		}
		break;

		case STRM_ECHO:
		{
			if (SendHostDataBusy()) {
				HtRetry();
				break;
			}

			// Echo data to back host
			SendHostData(PR_rcvData);
			HtContinue(STRM_RECV);
		}
		break;

		case STRM_RTN:
		{
			if (SendReturnBusy_htmain()) {
				HtRetry();
				break;
			}

			// Return number of bytes seen to the host
			SendReturn_htmain();
		}
		break;

		default:
			assert(0);
		}
	}
}
