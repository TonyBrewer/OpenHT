#include "Ht.h"
#include "PersInc5.h"

void
CPersInc5::PersInc5()
{
	if (PR_htValid) {
		//sc_uint<INC_WR_RSP_GRP_ID_W> wrRspGrpId = (sc_uint<INC_WR_RSP_GRP_ID_W>)((P_htId << 2) | (P_htId & 3));
		switch (PR_htInst) {
		case INC5_INIT:
		{
			P_loopIdx = 0;

			P_rdGrpId = 0x1;

			HtContinue(INC5_READ);
		}
		break;
		case INC5_READ:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			// Calculate memory read address
			MemAddr_t memRdAddr = SR_arrayAddr + ((P_loopBase + P_loopIdx) << 3);

			Inc5Ptr_t arrayMemWrPtr = (Inc5Ptr_t)P_loopIdx;

			// Issue read request to memory
			ReadMem_arrayMem5(PR_rdGrpId, memRdAddr, arrayMemWrPtr);

			bool bLast = P_loopIdx == P_elemCnt - 1;
			if (bLast) {
				P_loopIdx = 0;
				ReadMemPause(PR_rdGrpId, INC5_WRITE);
			} else {
				P_loopIdx += 1;
				HtContinue(INC5_READ);
			}

			// Set address for reading memory response data
			P_arrayMemRdPtr = (Inc5Ptr_t)P_loopIdx;
		}
		break;
		case INC5_WRITE:
		{
			//if (WriteMemBusy(rspGrpId)) {
			if (WriteMemBusy()) {
				//if (WriteMemBusy(wrRspGrpId)) {
				HtRetry();
				break;
			}

			// Increment memory data
			uint64_t memWrData = GR_arrayMem5.data + 1;

			// Calculate memory write address
			MemAddr_t memWrAddr = SR_arrayAddr + ((P_loopBase + P_loopIdx) << 3);

			// Issue write memory request
			//WriteMem(wrRspGrpId, memWrAddr, memWrData);
			WriteMem(memWrAddr, memWrData);

			bool bLast = P_loopIdx == P_elemCnt - 1;
			if (bLast) {
				P_loopIdx = 0;
				//WriteMemPause(wrRspGrpId, INC5_RTN);
				WriteMemPause(INC5_RTN);
				//HtPause();
			} else {
				P_loopIdx += 1;
				HtContinue(INC5_WRITE);
			}

			// Set address for reading memory response data
			P_arrayMemRdPtr = (Inc5Ptr_t)P_loopIdx;
		}
		break;
		case INC5_RTN:
		{
			if (SendReturnBusy_inc5()) {
				HtRetry();
				break;
			}

			SendReturn_inc5(P_elemCnt);

			break;
		}
		default:
			assert(0);
		}
	}
}
