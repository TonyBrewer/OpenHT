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
			P_reqCnt = 0;

			// Set address for reading memory response data
			P_arrayMemRdPtr = PR_htId;

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
				sc_uint<MEM_ADDR_W> memRdAddr = (sc_uint<MEM_ADDR_W>)(SR_arrayAddr + (((P_loopCnt + P_reqCnt) * 2) << 3));

				sc_uint<2> rdDstId = P_reqCnt;
				bool bLast = P_reqCnt == 3;

				// Issue read request to memory
				switch (rdDstId) {
				case 0: ReadMem_arrayMem1Fld1(memRdAddr, PR_htId); break;
				case 1: ReadMem_arrayMem1Fld2(memRdAddr, PR_htId); break;
				case 2: ReadMem_arrayMem2Fld1(memRdAddr, PR_htId); break;
				case 3: ReadMem_arrayMem2Fld2(memRdAddr, PR_htId); break;
				}

				if (bLast) {
					P_reqCnt = 0;
					ReadMemPause(INC_WRITE);
				} else {
					P_reqCnt += 1;
					HtContinue(INC_READ);
				}
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
			uint64_t memWrData = 0;
			sc_uint<2> rdDstId = P_reqCnt;
			switch (rdDstId) {
			case 0: memWrData = GR_arrayMem[0].fld1 + 1; break;
			case 1: memWrData = GR_arrayMem[0].fld2 + 1; break;
			case 2: memWrData = GR_arrayMem[1].fld1 + 1; break;
			case 3: memWrData = GR_arrayMem[1].fld2 + 1; break;
			}

			// Calculate memory write address
			sc_uint<MEM_ADDR_W> memWrAddr = (sc_uint<MEM_ADDR_W>)(SR_arrayAddr + (((P_loopCnt + P_reqCnt) * 2 + 1) << 3));

			bool bLast = P_reqCnt == 3;

			// Issue write memory request
			WriteMem(memWrAddr, memWrData);

			if (bLast) {
				// Increment loop count
				P_loopCnt = P_loopCnt + 4;
				P_reqCnt = 0;

				WriteMemPause(INC_READ);
			} else {
				P_reqCnt += 1;
				HtContinue(INC_WRITE);
			}
		}
		break;
		default:
			assert(0);
		}
	}
}
