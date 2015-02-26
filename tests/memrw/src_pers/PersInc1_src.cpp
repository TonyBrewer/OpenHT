#include "Ht.h"
#include "PersInc1.h"

void
CPersInc1::PersInc1()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case INC1_INIT:
		{
			P_loopIdx = 0;

			P_waitCnt += 1;

			if (m_htIdPool.empty() || PR_waitCnt >= 500)
				// wait until all threads have started
				HtContinue(INC1_READ);
			else
				HtContinue(INC1_INIT);
		}
		break;
		case INC1_READ:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			// Calculate memory read address
			MemAddr_t memRdAddr = SR_arrayAddr + ((P_loopBase + P_loopIdx) << 3);

			Inc1Ptr_t arrayMemWrPtr = (PR_htId << 7) | P_loopIdx;

			// Issue read request to memory
			ReadMem_arrayMem1(memRdAddr, arrayMemWrPtr);

			bool bLast = P_loopIdx == P_elemCnt - 1;
			if (bLast) {
				P_loopIdx = 0;
				ReadMemPause(INC1_WRITE);
			} else {
				P_loopIdx += 1;
				HtContinue(INC1_READ);
			}

			// Set address for reading memory response data
			P_arrayMemRdPtr = (PR_htId << 7) | P_loopIdx;
		}
		break;
		case INC1_WRITE:
		{
			//if (WriteMemBusy(rspGrpId)) {
			if (WriteMemBusy()) {
				//if (WriteMemBusy(wrRspGrpId)) {
				HtRetry();
				break;
			}

			// Increment memory data
			uint64_t memWrData = GR_arrayMem1.data + 1;

			// Calculate memory write address
			MemAddr_t memWrAddr = SR_arrayAddr + ((P_loopBase + P_loopIdx) << 3);

			// Issue write memory request
			//WriteMem(wrRspGrpId, memWrAddr, memWrData);
			WriteMem(memWrAddr, memWrData);

			bool bLast = P_loopIdx == P_elemCnt - 1;
			if (bLast) {
				P_loopIdx = 0;
				//WriteMemPause(wrRspGrpId, INC_RTN);
				WriteMemPause(INC1_RTN);
				//HtPause();
			} else {
				P_loopIdx += 1;
				HtContinue(INC1_WRITE);
			}

			// Set address for reading memory response data
			P_arrayMemRdPtr = (PR_htId << 7) | P_loopIdx;
		}
		break;
		case INC1_RTN:
		{
			if (SendReturnBusy_inc1()) {
				HtRetry();
				break;
			}

			SendReturn_inc1(P_elemCnt);

			break;
		}
		default:
			assert(0);
		}
	}
}
