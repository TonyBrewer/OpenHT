#include "Ht.h"
#include "PersEcho.h"

void
CPersEcho::PersEcho()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case ECHO_INIT:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			// Set address for reading memory response data
			P_arrayMemRdPtr = 2;
			P_dataCnt = 0;

			sc_uint<MEM_ADDR_W> memRdAddr = (sc_uint<MEM_ADDR_W>)r_arrayAddr;

			// Issue read request to memory
			ReadMem_arrayMem(memRdAddr, 2);

			ReadMemPause(ECHO_DATA);
		}
		break;
		case ECHO_DATA:
		{
			if (ReadMemBusy() || SendReturnBusy_echo() || RecvHostDataBusy() || SendHostDataBusy()) {
				HtRetry();
				break;
			}

			// handle adding value to popped queue data and pushing to output queue

			if (RecvHostDataMarker()) {
				SendHostDataMarker();

				// Return to host interface
				SendReturn_echo(P_dataCnt);
			} else {
				uint64_t inData = PeekHostData();

				RecvHostData();

				uint64_t outData = inData + GR_arrayMem_data();

				SendHostData(outData);

				P_dataCnt += 1;

				// Calculate memory read address
				sc_uint<MEM_ADDR_W> memRdAddr = (sc_uint<MEM_ADDR_W>)(r_arrayAddr + (P_dataCnt << 3));

				// Issue read request to memory
				ReadMem_arrayMem(memRdAddr, 2);

				ReadMemPause(ECHO_DATA);
			}
		}
		break;
		default:
			assert(0);
		}
	}
}
