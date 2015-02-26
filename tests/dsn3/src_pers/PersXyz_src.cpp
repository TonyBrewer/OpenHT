#include "Ht.h"
#include "PersXyz.h"

enum Enum1 { a1, b1, c1 };

#define STACK_POP() ((--x_sp == 2) ? x_stk : (ht_uint2)0)

void
CPersXyz::PersXyz()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case XYZ_INIT:
		{
			P_loopCnt = 0;

			ht_uint2 x_sp = 3;
			ht_uint2 x_stk = XYZ_READ;

			HtContinue(STACK_POP());
		}
		break;
		case XYZ_READ:
		{
			if (ReadMemBusy() || SendReturnBusy_htmain()) {
				HtRetry();
				break;
			}

			Enum1 eb;
			eb = b1;

			Enum1 ec = b1;

			HtAssert(eb == ec, 0);

			enum Enum2 { a2, b2, c1 };

			Enum2 ed;
			ed = b2;

			Enum2 ee = b2;

			HtAssert(ed == ee, 0);

			// Check if end of loop
			if (P_loopCnt == P_elemCnt) {
				// Return to host interface
				SendReturn_htmain(P_loopCnt);
			} else {
				// Calculate memory read address
				sc_uint<MEM_ADDR_W> memRdAddr = (sc_uint<MEM_ADDR_W>)(SR_arrayAddr + (P_loopCnt << 3));

				// Issue read request to memory
				ReadMem_arrayMem(memRdAddr, PR_htId);

				// Set address for reading memory response data
				P_arrayMemRdPtr = PR_htId;

				ReadMemPause(XYZ_WRITE);
			}
		}
		break;
		case XYZ_WRITE:
		{
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}

			// Increment memory data
			uint64_t memWrData = GR_arrayMem.data + 1;

			// Calculate memory write address
			sc_uint<MEM_ADDR_W> memWrAddr = (sc_uint<MEM_ADDR_W>)(SR_arrayAddr + (P_loopCnt << 3));

			// Issue write memory request
			WriteMem(memWrAddr, memWrData);

			// Increment loop count
			P_loopCnt = P_loopCnt + 1;

			WriteMemPause(XYZ_READ);
		}
		break;
		default:
			assert(0);
		}
	}
}
