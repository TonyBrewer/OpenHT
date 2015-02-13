#include "Ht.h"
#include "PersStream.h"

#define BUSY_RETRY(b) {if (b) {HtRetry(); break;}}

// Host <-> Coprocessor Data Streaming Example
// Uses shared state machine to handle work
//   - Continue until HostDataMarker is received
// Return number of bytes processed

void CPersStream::PersStream() {
	if (PR_htValid) {
		switch (PR_htInst) {

		// Main entry point from Host (htmain call)
		// Intialize shared variables
		case STREAM_ENTRY: {
			S_done = false;
			S_inst = 0;
			HtContinue(STREAM_WORK);
		}
		break;

		// Wait for shared state machine to finish work
		case STREAM_WORK: {
			if (SR_done == true) {
				HtContinue(STREAM_RTN);
			} else {
				HtContinue(STREAM_WORK);
			}
		}
		break;

		case STREAM_RTN: {
			BUSY_RETRY(SendReturnBusy_htmain());

			// Return number of bytes seen to the host
			SendReturn_htmain(SR_rcvByteCnt);
		}
		break;

		default:
			assert(0);
		}
	}

	// ** Functionality outside of Instuctions **
	// This logic will be executed every clock cycle

	// Shared state machine to process/echo received data
	// Instruction Decoding
	//   - 0 -> IDLE
	//   - 1 -> READ
	//   - 2 -> WRITE

	switch(SR_inst) {

	// IDLE
	case 0: {
		if (SR_done == false) {
			// Work to be done, continue to READ
			S_inst = 1;
		}
	}
	break;

	// READ
	case 1: {
		if (!RecvHostDataBusy()) {
			// Check for marker
			if (RecvHostDataMarker()) {
				// Marker Present, done
				S_done = true;
				S_inst = 0;
			} else {
				// Data Present, echo
				S_temp = RecvHostData();
				S_rcvByteCnt = SR_rcvByteCnt + 8;
				S_inst = 2;
			}
		}
	}
	break;

	// WRITE
	case 2: {
		if (!SendHostDataBusy()) {
			// Send data to host
			SendHostData(SR_temp);
			S_inst = 1;
		}
	}
	break;

	default:
		assert(0);
	}
}
