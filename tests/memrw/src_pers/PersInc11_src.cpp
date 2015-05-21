#include "Ht.h"
#include "PersInc11.h"

void
CPersInc11::PersInc11()
{
	if (PR1_htValid) {
		switch (PR1_htInst) {
		case INC11_INIT:
		{
			P1_loopIdx[0][1] = 0;

			P1_wrGrpId = PR1_htId ^ 0x5;

			// wait until all threads have started
			HtContinue(INC11_READ);
		}
		break;
		case INC11_READ:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			// Calculate memory read address
			MemAddr_t memRdAddr = SR_arrayAddr + ((P1_loopBase + P1_loopIdx[0][1]) << 3);

			Inc11Ptr_t arrayMemWrPtr = (PR1_htId << 7) | P1_loopIdx[0][1];

			// Issue read request to memory
			ReadMem_arrayMem11(memRdAddr, arrayMemWrPtr);

			bool bLast = P1_loopIdx[0][1] == P1_elemCnt - 1;
			if (bLast) {
				P1_loopIdx[0][1] = 0;
				ReadMemPause(INC11_WRITE);
			} else {
				P1_loopIdx[0][1] += 1;
				HtContinue(INC11_READ);
			}

			// Set address for reading memory response data
			P1_arrayMemRdPtr = (PR1_htId << 7) | P1_loopIdx[0][1];
		}
		break;
		case INC11_WRITE:
		{
			if (WriteMemBusy()) {
				//if (WriteMemBusy(wrRspGrpId)) {
				HtRetry();
				break;
			}

			// Increment memory data
			uint64_t memWrData = GR1_arrayMem11.data + 1;

			// Calculate memory write address
			MemAddr_t memWrAddr = SR_arrayAddr + ((P1_loopBase + P1_loopIdx[0][1]) << 3);

			// Issue write memory request
			WriteMem(PR1_wrGrpId, memWrAddr, memWrData);

			bool bLast = P1_loopIdx[0][1] == P1_elemCnt - 1;
			if (bLast) {
				P1_loopIdx[0][1] = 0;
				//WriteMemPause(wrRspGrpId, INC11_RTN);
				WriteMemPause(PR1_wrGrpId, INC11_RTN);
				//HtPause();
			} else {
				P1_loopIdx[0][1] += 1;
				HtContinue(INC11_WRITE);
			}

			// Set address for reading memory response data
			P1_arrayMemRdPtr = (PR1_htId << 7) | P1_loopIdx[0][1];
		}
		break;
		case INC11_RTN:
		{
			if (SendReturnBusy_inc11()) {
				HtRetry();
				break;
			}

			SendReturn_inc11(P1_elemCnt);

			break;
		}
		default:
			assert(0);
		}
	}
}
