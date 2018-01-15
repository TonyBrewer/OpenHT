#include "Ht.h"
#include "PersLoop.h"

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

void
CPersLoop::PersLoop()
{
	for (int i = 0; i < 8; i++) {
		if (RecvUioReady_in(i) && !SendUioBusy_out(i)) {
			packet_t pkt = RecvUioData_in(i);
			SendUioData_out(i, pkt);
		}
	}
	if (!SendUioBusy_status()) {
		packet_t pkt;
		pkt.status.fatal_alm = 0;
		pkt.status.corr_alm = 0;
		pkt.status.chan_up = 0xFF;
		pkt.status.lane_up = 0xFF;
		SendUioData_status(pkt);
	}

	// CSR Logic
	if (RecvUioCsrCmdReady()) {
		uio_csr_rq_t uio_rq = RecvUioCsrCmd();

		// Write
		if (uio_rq.cmd == HT_CSR_WR) {

			switch (uio_rq.addr) {

				case 0x00: S_reg0 = uio_rq.data; break;
				case 0x08: S_reg1 = uio_rq.data; break;
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
				case 0x08: rdData = SR_reg1; break;
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
