#include "Ht.h"
#include "PersInc3.h"

void
CPersInc3::PersInc3()
{
	if (PR1_htValid) {
		switch (PR1_htInst) {
		case INC3_INIT:
		{
			P1_loopIdx[0][1] = 0;

			// wait until all threads have started
			HtContinue(INC3_READ);
		}
		break;
		case INC3_READ:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			// Calculate memory read address
			MemAddr_t memRdAddr = SR_arrayAddr + ((P1_loopBase + P1_loopIdx[0][1]) << 3);

			Inc3Ptr_t arrayMemWrPtr = (PR1_htId << 7) | P1_loopIdx[0][1];

			// Issue read request to memory
			ReadMem_arrayMem3(memRdAddr, arrayMemWrPtr);

			bool bLast = P1_loopIdx[0][1] == P1_elemCnt - 1;
			if (bLast) {
				P1_loopIdx[0][1] = 0;
				ReadMemPause(INC3_WRITE);
			} else {
				P1_loopIdx[0][1] += 1;
				HtContinue(INC3_READ);
			}

			// Set address for reading memory response data
			P1_arrayMemRdPtr = (PR1_htId << 7) | P1_loopIdx[0][1];
		}
		break;
		case INC3_WRITE:
		{
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}

			// Increment memory data
			uint64_t memWrData = GR1_arrayMem3.data+ 1;

			// Calculate memory write address
			MemAddr_t memWrAddr = SR_arrayAddr + ((P1_loopBase + P1_loopIdx[0][1]) << 3);

			// Issue write memory request
			WriteMem(memWrAddr, memWrData);

			bool bLast = P1_loopIdx[0][1] == P1_elemCnt - 1;
			if (bLast) {
				P1_loopIdx[0][1] = 0;
				WriteMemPause(INC3_RTN);
			} else {
				P1_loopIdx[0][1] += 1;
				HtContinue(INC3_WRITE);
			}

			// Set address for reading memory response data
			P1_arrayMemRdPtr = (PR1_htId << 7) | P1_loopIdx[0][1];
		}
		break;
		case INC3_RTN:
		{
			if (SendReturnBusy_inc3()) {
				HtRetry();
				break;
			}

			SendReturn_inc3(P1_elemCnt);

			break;
		}
		default:
			assert(0);
		}
	}
}
