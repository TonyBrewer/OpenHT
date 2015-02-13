#include "Ht.h"
#include "PersInc7.h"

void
CPersInc7::PersInc7()
{
	if (PR_htValid) {
		//sc_uint<INC_WR_RSP_GRP_ID_W> wrRspGrpId = (sc_uint<INC_WR_RSP_GRP_ID_W>)((P_htId << 2) | (P_htId & 3));
		//static int cnt = 0;
		//if (++cnt == 151)
		//    bool stop = true;

		//printf("%d - HtId %d: inst %d\n", cnt, (int)PR_htId, (int)PR_htInst);
		switch (PR_htInst) {
		case INC7_INIT:
		{
			P_loopIdx = 0;

			P_rdGrpId = PR_htId ^ 0x5;

			//if (m_htIdPool.empty())
			// wait until all threads have started
			HtContinue(INC7_READ);
			//else
			//	HtContinue(INC7_INIT);
		}
		break;
		case INC7_READ:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			// Calculate memory read address
			MemAddr_t memRdAddr = SR_arrayAddr + ((P_loopBase + P_loopIdx) << 3);

			Inc7Ptr_t arrayMemWrPtr = (PR_htId << 7) | P_loopIdx;

			// Issue read request to memory
			ReadMem_arrayMem7(memRdAddr, arrayMemWrPtr);

			bool bLast = P_loopIdx == P_elemCnt - 1;
			if (bLast) {
				P_loopIdx = 0;
				ReadMemPause(INC7_WRITE);
			} else {
				P_loopIdx += 1;
				HtContinue(INC7_READ);
			}

			// Set address for reading memory response data
			P_arrayMemRdPtr = (PR_htId << 7) | P_loopIdx;
		}
		break;
		case INC7_WRITE:
		{
			//if (WriteMemBusy(rspGrpId)) {
			if (WriteMemBusy()) {
				//if (WriteMemBusy(wrRspGrpId)) {
				HtRetry();
				break;
			}

			// Increment memory data
			uint64_t memWrData = GR_arrayMem7_data() + 1;

			// Calculate memory write address
			MemAddr_t memWrAddr = SR_arrayAddr + ((P_loopBase + P_loopIdx) << 3);

			// Issue write memory request
			//WriteMem(wrRspGrpId, memWrAddr, memWrData);
			WriteMem(memWrAddr, memWrData);

			bool bLast = P_loopIdx == P_elemCnt - 1;
			if (bLast) {
				P_loopIdx = 0;
				//WriteMemPause(wrRspGrpId, INC7_RTN);
				WriteMemPause(INC7_RTN);
				//HtPause();
			} else {
				P_loopIdx += 1;
				HtContinue(INC7_WRITE);
			}

			// Set address for reading memory response data
			P_arrayMemRdPtr = (PR_htId << 7) | P_loopIdx;
		}
		break;
		case INC7_RTN:
		{
			if (SendReturnBusy_inc7()) {
				HtRetry();
				break;
			}

			SendReturn_inc7(P_elemCnt);

			break;
		}
		default:
			assert(0);
		}
	}
}
