#include "Ht.h"
#include "PersInc0.h"

void
CPersInc0::PersInc0()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case INC0_INIT:
		{
			P_loopIdx = 0;

			HtContinue(INC0_READ);
		}
		break;
		case INC0_READ:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			// Calculate memory read address
			MemAddr_t memRdAddr = SR_arrayAddr + ((P_loopBase + P_loopIdx) << 3);

			Inc0Ptr_t arrayMemWrPtr = (Inc0Ptr_t)P_loopIdx;

			// Issue read request to memory
			ReadMem_arrayMem0(memRdAddr, arrayMemWrPtr);

			bool bLast = P_loopIdx == P_elemCnt - 1;
			if (bLast) {
				P_loopIdx = 0;
				ReadMemPause(INC0_WRITE);
			} else {
				P_loopIdx += 1;
				HtContinue(INC0_READ);
			}

			// Set address for reading memory response data
			P_arrayMemRdPtr = (Inc0Ptr_t)P_loopIdx;
		}
		break;
		case INC0_WRITE:
		{
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}

			// Increment memory data
			uint64_t memWrData = GR_arrayMem0.data + 1;

			// Calculate memory write address
			MemAddr_t memWrAddr = SR_arrayAddr + ((P_loopBase + P_loopIdx) << 3);

			// Issue write memory request
			WriteMem(memWrAddr, memWrData);

			bool bLast = P_loopIdx == P_elemCnt - 1;
			if (bLast) {
				P_loopIdx = 0;
				WriteMemPause(INC0_RTN);
			} else {
				P_loopIdx += 1;
				HtContinue(INC0_WRITE);
			}

			// Set address for reading memory response data
			P_arrayMemRdPtr = (Inc0Ptr_t)P_loopIdx;
		}
		break;
		case INC0_RTN:
		{
			if (SendReturnBusy_inc0()) {
				HtRetry();
				break;
			}

			SendReturn_inc0(P_elemCnt);

			break;
		}
		default:
			assert(0);
		}
	}
}
