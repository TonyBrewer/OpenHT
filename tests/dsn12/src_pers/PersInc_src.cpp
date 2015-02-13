#include "Ht.h"
#include "PersInc.h"

void CPersInc::PersInc()
{
#if INC_RSM_CNT == 1
	if (GR_htReset) {
		S_rsmRdy = 0;
	} else if (SR_rsmRdy) {
		S_rsmRdy = 0;
#if INC_HTID_W == 0
		HtResume(0);
#else
		HtResume(SR_rsmHtId);
#endif
	}
#endif

	if (PR_htValid) {
		switch (PR_htInst) {
		case INC_INIT:
		{
			P_loopCnt = 0;

#if INC_RSM_CNT == 1
			S_rsmHtId = PR_htId;
			S_rsmRdy = true;

			HtPause(INC_READ);
#else
			HtContinue(INC_READ);
#endif
		}
		break;
		case INC_READ:
		{
			if (ReadMemBusy() || SendReturnBusy_htmain()) {
				HtRetry();
				break;
			}

			// Check if end of loop
			if (P_loopCnt == P_elemCnt) {
				// Return to host interface
				SendReturn_htmain(P_loopCnt);
			} else {
				// Calculate memory read address
				MemAddr_t memRdAddr = SR_arrayAddr + (P_loopCnt << 3);

				// Issue read request to memory
#if INC_HTID_W == 0
				ReadMem_arrayMem(memRdAddr);
#else
				ReadMem_arrayMem(memRdAddr, PR_htId);
#endif

				// Set address for reading memory response data
				P_arrayMemRdPtr = PR_htId;

#if INC_RD_POLL==1
				HtContinue(INC_READ_POLL);
#else
				ReadMemPause(INC_WRITE);
#endif
			}
		}
		break;
#if INC_RD_POLL==1
		case INC_READ_POLL:
  #if INC_RSP_GRP_W == 0
			if (ReadMemBusy() || ReadMemPoll()) {
  #else
			if (ReadMemBusy() || ReadMemPoll(PR_rspGrpVar)) {
#endif
				HtRetry();
				break;
			}

			HtContinue(INC_WRITE);
			break;
#endif
		case INC_WRITE:
		{
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}

			// Increment memory data
			uint64_t memWrData = GR_arrayMem_data() + 1;

			// Calculate memory write address
			MemAddr_t memWrAddr = SR_arrayAddr + (P_loopCnt << 3);

			// Issue write memory request
			WriteMem(memWrAddr, memWrData);

			// Increment loop count
			P_loopCnt = P_loopCnt + 1;

#if INC_WR_POLL==1
			HtContinue(INC_WRITE_POLL);
#else
			WriteMemPause(INC_READ);
#endif
		}
		break;
#if INC_WR_POLL==1
		case INC_WRITE_POLL:
			if (WriteMemBusy() || WriteMemPoll()) {
				HtRetry();
				break;
			}

			HtContinue(INC_READ);
			break;
#endif
		default:
			assert(0);
		}
	}
}

#if INC_HTID_W == 0 && INC_RSP_GRP_W == 0
void CPersInc::ReadMemResp_arrayMem(sc_uint<64> rdRspData)
{
	GW_arrayMem_data(0, rdRspData);
}
#elif INC_HTID_W == 0 && INC_RSP_GRP_W != 0
void CPersInc::ReadMemResp_arrayMem(sc_uint<INC_RD_GRP_ID_W> ht_noload rdRspGrpId, sc_uint<64> rdRspData)
{
	GW_arrayMem_data(0, rdRspData);
}
#else
void CPersInc::ReadMemResp_arrayMem(sc_uint<INC_RD_GRP_ID_W> rdRspGrpId, sc_uint<INC_MIF_DST_ARRAYMEM_INFO_W> rdRspInfo, sc_uint<64> rdRspData)
{
	assert(rdRspGrpId == rdRspInfo);

	HtId_t wrAddr = (rdRspGrpId | rdRspInfo) & ((1u << INC_HTID_W)-1);

	GW_arrayMem_data(wrAddr, rdRspData);
}
#endif
