#include "Ht.h"
#include "PersInc12.h"

void
CPersInc12::PersInc12()
{
	if (PR1_htValid) {
		switch (PR1_htInst) {
		case INC12_INIT:
		{
			P1_loopIdx[0][1] = 0;

			P1_wrGrpId = PR1_htId ^ 0xa;

			// wait until all threads have started
			HtContinue(INC12_READ);
		}
		break;
		case INC12_READ:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			// Calculate memory read address
			MemAddr_t memRdAddr = SR_arrayAddr + ((P1_loopBase + P1_loopIdx[0][1]) << 3);

			Inc12Ptr_t arrayMemWrPtr = (PR1_htId << 7) | P1_loopIdx[0][1];

			// Issue read request to memory
			ReadMem_arrayMem12(memRdAddr, arrayMemWrPtr);

			bool bLast = P1_loopIdx[0][1] == P1_elemCnt - 1;
			if (bLast) {
				P1_loopIdx[0][1] = 0;
				ReadMemPause(INC12_WRITE);
			} else {
				P1_loopIdx[0][1] += 1;
				HtContinue(INC12_READ);
			}

			// Set address for reading memory response data
			P1_arrayMemRdPtr = (PR1_htId << 7) | P1_loopIdx[0][1];
		}
		break;
		case INC12_WRITE:
		{
			//if (WriteMemBusy(rspGrpId)) {
			if (WriteMemBusy()) {
				//if (WriteMemBusy(wrRspGrpId)) {
				HtRetry();
				break;
			}

			// Increment memory data
			uint64_t memWrData = GR1_arrayMem12.data + 1;

			// Calculate memory write address
			MemAddr_t memWrAddr = SR_arrayAddr + ((P1_loopBase + P1_loopIdx[0][1]) << 3);

			// Issue write memory request
			//WriteMem(wrRspGrpId, memWrAddr, memWrData);
			WriteMem(PR1_wrGrpId, memWrAddr, memWrData);

			bool bLast = P1_loopIdx[0][1] == P1_elemCnt - 1;
			if (bLast) {
				P1_loopIdx[0][1] = 0;
				//WriteMemPause(wrRspGrpId, INC12_RTN);
				WriteMemPause(PR1_wrGrpId, INC12_RTN);
				//HtPause();
			} else {
				P1_loopIdx[0][1] += 1;
				HtContinue(INC12_WRITE);
			}

			// Set address for reading memory response data
			P1_arrayMemRdPtr = (PR1_htId << 7) | P1_loopIdx[0][1];
		}
		break;
		case INC12_RTN:
		{
			if (SendReturnBusy_inc12()) {
				HtRetry();
				break;
			}

			SendReturn_inc12(P1_elemCnt);

			break;
		}
		default:
			assert(0);
		}
	}
}
