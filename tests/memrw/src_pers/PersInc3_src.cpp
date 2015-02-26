#include "Ht.h"
#include "PersInc3.h"

void
CPersInc3::PersInc3()
{
	if (PR_htValid) {
		//sc_uint<INC_WR_RSP_GRP_ID_W> wrRspGrpId = (sc_uint<INC_WR_RSP_GRP_ID_W>)((P_htId << 2) | (P_htId & 3));
		switch (PR_htInst) {
		case INC3_INIT:
		{
			P_loopIdx[0][1] = 0;

			//if (m_htIdPool.empty())
			// wait until all threads have started
			HtContinue(INC3_READ);
			//else
			//	HtContinue(INC3_INIT);
		}
		break;
		case INC3_READ:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			// Calculate memory read address
			MemAddr_t memRdAddr = SR_arrayAddr + ((P_loopBase + P_loopIdx[0][1]) << 3);

			Inc3Ptr_t arrayMemWrPtr = (PR_htId << 7) | P_loopIdx[0][1];

			// Issue read request to memory
			ReadMem_arrayMem3(memRdAddr, arrayMemWrPtr);

			bool bLast = P_loopIdx[0][1] == P_elemCnt - 1;
			if (bLast) {
				P_loopIdx[0][1] = 0;
				ReadMemPause(INC3_WRITE);
			} else {
				P_loopIdx[0][1] += 1;
				HtContinue(INC3_READ);
			}

			// Set address for reading memory response data
			P_arrayMemRdPtr = (PR_htId << 7) | P_loopIdx[0][1];
		}
		break;
		case INC3_WRITE:
		{
			//if (WriteMemBusy(rspGrpId)) {
			if (WriteMemBusy()) {
				//if (WriteMemBusy(wrRspGrpId)) {
				HtRetry();
				break;
			}

			// Increment memory data
			uint64_t memWrData = GR1_arrayMem3.data+ 1;

			// Calculate memory write address
			MemAddr_t memWrAddr = SR_arrayAddr + ((P_loopBase + P_loopIdx[0][1]) << 3);

			// Issue write memory request
			//WriteMem(wrRspGrpId, memWrAddr, memWrData);
			WriteMem(memWrAddr, memWrData);

			bool bLast = P_loopIdx[0][1] == P_elemCnt - 1;
			if (bLast) {
				P_loopIdx[0][1] = 0;
				//WriteMemPause(wrRspGrpId, INC3_RTN);
				WriteMemPause(INC3_RTN);
				//HtPause();
			} else {
				P_loopIdx[0][1] += 1;
				HtContinue(INC3_WRITE);
			}

			// Set address for reading memory response data
			P_arrayMemRdPtr = (PR_htId << 7) | P_loopIdx[0][1];
		}
		break;
		case INC3_RTN:
		{
			if (SendReturnBusy_inc3()) {
				HtRetry();
				break;
			}

			SendReturn_inc3(P_elemCnt);

			break;
		}
		default:
			assert(0);
		}
	}
}
