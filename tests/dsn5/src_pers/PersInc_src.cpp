#include "Ht.h"
#include "PersInc.h"

void
CPersInc::PersInc()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case INC_INIT:
		{
			P_loopCnt = 0;

			HtContinue(INC_READ);
		}
		break;
		case INC_READ:
		{
			if (ReadMemBusy() || SendReturnBusy_htmain()) {
				HtRetry();
				break;
			}

			// Check if end of loop
			if (P_loopCnt == P_elemCnt) {
				// Return to host interface
				SendReturn_htmain(P_loopCnt);
			} else {
				// Calculate memory read address
				sc_uint<MEM_ADDR_W> memRdAddr = (sc_uint<MEM_ADDR_W>)(SR_arrayAddr + ((P_loopCnt * 2) << 3));

				// Issue read request to memory
				ReadMem_sharedData(memRdAddr);

				ReadMemPause(INC_WRITE);
			}
		}
		break;
		case INC_WRITE:
		{
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}

			// Increment memory data
			uint64_t memWrData = S_sharedData + S_sharedInc;

			// Calculate memory write address
			sc_uint<MEM_ADDR_W> memWrAddr = (sc_uint<MEM_ADDR_W>)(SR_arrayAddr + ((P_loopCnt * 2 + 1) << 3));

			// Issue write memory request
			WriteMem(memWrAddr, memWrData);

			// Increment loop count
			P_loopCnt = P_loopCnt + 1;

			WriteMemPause(INC_READ2);
		}
		break;
		case INC_READ2:
		{
			if (ReadMemBusy() || SendReturnBusy_htmain()) {
				HtRetry();
				break;
			}

			// Check if end of loop
			if (P_loopCnt == P_elemCnt) {
				// Return to host interface
				SendReturn_htmain(P_loopCnt);
			} else {
				// Calculate memory read address
				sc_uint<MEM_ADDR_W> memRdAddr = (sc_uint<MEM_ADDR_W>)(SR_arrayAddr + ((P_loopCnt * 2) << 3));

				// Issue read request to memory
				ReadMem_sharedArray1(memRdAddr, 1, 1);

				ReadMemPause(INC_WRITE2);
			}
		}
		break;
		case INC_WRITE2:
		{
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}

			// Increment memory data
			uint64_t memWrData = S_sharedArray1[1] + S_sharedInc;

			// Calculate memory write address
			sc_uint<MEM_ADDR_W> memWrAddr = (sc_uint<MEM_ADDR_W>)(SR_arrayAddr + ((P_loopCnt * 2 + 1) << 3));

			// Issue write memory request
			WriteMem(memWrAddr, memWrData);

			// Increment loop count
			P_loopCnt = P_loopCnt + 1;

			WriteMemPause(INC_READ3);
		}
		break;
		case INC_READ3:
		{
			if (ReadMemBusy() || SendReturnBusy_htmain()) {
				HtRetry();
				break;
			}

			// Check if end of loop
			if (P_loopCnt == P_elemCnt) {
				// SendReturn to host interface
				SendReturn_htmain(P_loopCnt);
			} else {
				// Calculate memory read address
				sc_uint<MEM_ADDR_W> memRdAddr = (sc_uint<MEM_ADDR_W>)(SR_arrayAddr + ((P_loopCnt * 2) << 3));

				// Issue read request to memory
				ReadMem_sharedArray2(memRdAddr, 1, 2, 1);

				ReadMemPause(INC_WRITE3);
			}
		}
		break;
		case INC_WRITE3:
		{
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}

			// Increment memory data
			uint64_t memWrData = S_sharedArray2[1][2] + S_sharedInc;

			// Calculate memory write address
			sc_uint<MEM_ADDR_W> memWrAddr = (sc_uint<MEM_ADDR_W>)(SR_arrayAddr + ((P_loopCnt * 2 + 1) << 3));

			// Issue write memory request
			WriteMem(memWrAddr, memWrData);

			// Increment loop count
			P_loopCnt = P_loopCnt + 1;

			WriteMemPause(INC_READ4);
		}
		break;
		case INC_READ4:
		{
			if (ReadMemBusy() || SendReturnBusy_htmain()) {
				HtRetry();
				break;
			}

			// Check if end of loop
			if (P_loopCnt == P_elemCnt) {
				// Return to host interface
				SendReturn_htmain(P_loopCnt);
			} else {
				// Calculate memory read address
				sc_uint<MEM_ADDR_W> memRdAddr = (sc_uint<MEM_ADDR_W>)(SR_arrayAddr + ((P_loopCnt * 2) << 3));

				// Issue read request to memory
				ReadMem_sharedArray3(memRdAddr, 9, 1, 2, 1);

				ReadMemPause(INC_WRITE4);
			}
		}
		break;
		case INC_WRITE4:
		{
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}

			// Increment memory data
			sc_uint<3> idx1 = 1;
			sc_uint<2> idx2 = 2;

			S_sharedArray3[INT(idx1)][INT(idx2)].read_addr(9);
			uint64_t memWrData = S_sharedArray3[INT(idx1)][INT(idx2)].read_mem() + S_sharedInc;

			// Calculate memory write address
			sc_uint<MEM_ADDR_W> memWrAddr = (sc_uint<MEM_ADDR_W>)(SR_arrayAddr + ((P_loopCnt * 2 + 1) << 3));

			// Issue write memory request
			WriteMem(memWrAddr, memWrData);

			// Increment loop count
			P_loopCnt = P_loopCnt + 1;

			WriteMemPause(INC_READ);
		}
		break;
		case INC_RESET:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			ReadMemPause(INC_RESET1);
			break;
		}
		case INC_RESET1:
		{
			S_sharedInc = 1;

			HtTerminate();
			break;
		}
		default:
			assert(0);
		}
	}

	if (r_reset1x)
		S_sharedInc = 5;
}
