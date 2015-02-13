#include "Ht.h"
#include "PersInc.h"

void
CPersInc::PersInc()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case INC_INIT:
		{
			if (ReadMemBusy() || SendReturnBusy_htmain()) {
				HtRetry();
				break;
			}

			// Set address for reading memory response data
			P_arrayMemRdPtr = 1;

			if (!RecvHostDataBusy() && RecvHostDataMarker()) {
				// Return to host interface
				SendReturn_htmain(P_loopCnt);
			} else {
				P_loopCnt = 0;

				sc_uint<MEM_ADDR_W> memRdAddr = (sc_uint<MEM_ADDR_W>)SR_arrayAddr;
				P_loopCnt += 1;

				// Issue read request to memory
				ReadMem_arrayMem(memRdAddr, PR_htId, 1);

				ReadMemPause(INC_DATA);
			}
		}
		break;
		case INC_DATA:
		{
			if (ReadMemBusy() || SendReturnBusy_htmain() || RecvHostDataBusy() || SendHostDataBusy()) {
				HtRetry();
				break;
			}

			// handle adding value to popped queue data and pushing to output queue
			if (RecvHostDataMarker()) {
				SendHostDataMarker();

				// Return to host interface
				SendReturn_htmain(P_loopCnt);
			} else {
				uint64_t inData = RecvHostData();

				uint64_t outData = inData + GR_arrayMem_data();

				SendHostData(outData);

				// Calculate memory read address
				sc_uint<MEM_ADDR_W> memRdAddr = (sc_uint<MEM_ADDR_W>)(SR_arrayAddr + (P_loopCnt << 3));
				P_loopCnt += 1;

				// Issue read request to memory
				ReadMem_arrayMem(memRdAddr, PR_htId, 1);

				ReadMemPause(INC_DATA);
			}
		}
		break;
		default:
			assert(0);
		}
	}
}
