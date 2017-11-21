#include "Ht.h"
#include "PersLoop.h"

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

void
CPersLoop::PersLoop()
{
	for (int i = 0; i < 2; i++) {
		if (RecvUioReady_inA(i) && !SendUioBusy_outA(i)) {
			tmp_t tmp = RecvUioData_inA(i);
			SendUioData_outA(i, tmp);
		}

		if (RecvUioReady_inB(i) && !SendUioBusy_outB(i)) {
			tmp_t tmp = RecvUioData_inB(i);
			SendUioData_outB(i, tmp);
		}
	}

	// CSR Logic
	if (RecvUioCsrCmdReady()) {
		uio_csr_rq_t uio_rq = RecvUioCsrCmd();

		// Write
		if (uio_rq.cmd == HT_CSR_WR) {

			switch (uio_rq.addr) {

				case 0x00: S_reg0 = uio_rq.data; break;
				//case 0x08: ...
		        	//case 0x10: ...

				default: {
					fprintf(stderr, "Attempted access to undefined register\n");
					HtAssert(0, 0);
				}
				break;
			}
		}

		// Read
		else {

			uint64_t rdData = 0;

			switch (uio_rq.addr) {

				case 0x00: rdData = SR_reg0; break;
				//case 0x08: ...
		        	//case 0x10: ...

				default: {
					fprintf(stderr, "Attempted access to undefined register\n");
					HtAssert(0, 0);
				}
			break;
			}
			
			SendUioCsrRdRsp(rdData);

		}
	}
}
