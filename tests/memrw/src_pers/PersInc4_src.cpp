#include "Ht.h"
#include "PersInc4.h"

void
CPersInc4::PersInc4()
{
	if (PR_htValid) {
		//sc_uint<INC_WR_RSP_GRP_ID_W> wrRspGrpId = (sc_uint<INC_WR_RSP_GRP_ID_W>)((P_htId << 2) | (P_htId & 3));
		switch (PR_htInst) {
		case INC4_INIT:
		{
			P_loopIdx = 0;

			//if (m_htIdPool.empty())
			// wait until all threads have started
			HtContinue(INC4_READ);
			//else
			//	HtContinue(INC4_INIT);
		}
		break;
		case INC4_READ:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			// Calculate memory read address
			MemAddr_t memRdAddr = SR_arrayAddr + ((P_loopBase + P_loopIdx) << 3);

			Inc4Ptr_t arrayMemWrPtr = (PR_htId << 7) | P_loopIdx;

			// Issue read request to memory
			ReadMem_arrayMem4(memRdAddr, arrayMemWrPtr);

			bool bLast = P_loopIdx == P_elemCnt - 1;
			if (bLast) {
				P_loopIdx = 0;
				ReadMemPause(INC4_WRITE);
			} else {
				P_loopIdx += 1;
				HtContinue(INC4_READ);
			}

			// Set address for reading memory response data
			P_arrayMemRdPtr = (PR_htId << 7) | P_loopIdx;
		}
		break;
		case INC4_WRITE:
		{
			//if (WriteMemBusy(rspGrpId)) {
			if (WriteMemBusy()) {
				//if (WriteMemBusy(wrRspGrpId)) {
				HtRetry();
				break;
			}

			// Increment memory data
			uint64_t memWrData = GR_arrayMem4.data + 1;

			// Calculate memory write address
			MemAddr_t memWrAddr = SR_arrayAddr + ((P_loopBase + P_loopIdx) << 3);

			// Issue write memory request
			//WriteMem(wrRspGrpId, memWrAddr, memWrData);
			WriteMem(memWrAddr, memWrData);

			bool bLast = P_loopIdx == P_elemCnt - 1;
			if (bLast) {
				P_loopIdx = 0;
				//WriteMemPause(wrRspGrpId, INC4_RTN);
				WriteMemPause(INC4_RTN);
				//HtPause();
			} else {
				P_loopIdx += 1;
				HtContinue(INC4_WRITE);
			}

			// Set address for reading memory response data
			P_arrayMemRdPtr = (PR_htId << 7) | P_loopIdx;
		}
		break;
		case INC4_RTN:
		{
			if (SendReturnBusy_inc4()) {
				HtRetry();
				break;
			}

			SendReturn_inc4(P_elemCnt);

			break;
		}
		default:
			assert(0);
		}
	}
}
