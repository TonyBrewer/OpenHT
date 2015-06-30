#include "Ht.h"
#include "PersWr4.h"

void CPersWr4::PersWr4()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case WR4_INIT:
		{
			P_loopCnt = 0;
			P_err = 0;
			P_pauseLoopCnt = 0;

			HtContinue(WR4_WRITE1b);
		}
		break;
		case WR4_WRITE1a:
		case WR4_WRITE1b:
		{
			if (PR_htInst != (PR_pauseDst ? WR4_WRITE1a : WR4_WRITE1b)) {
				HtAssert(0, 0);
				P_err += 1;
			}
			P_pauseDst ^= 1;

			if (WriteMemBusy() || SendReturnBusy_wr4()) {
				HtRetry();
				break;
			}

			// Check if end of loop
			if (P_loopCnt == PAUSE_LOOP_CNT || P_err) {
				// Return to host interface
				SendReturn_wr4(P_err);
			} else {
				// Calculate memory read address
				MemAddr_t memRdAddr = P_arrayAddr + ((P_loopCnt & 0xf) << 3);

				// Issue read request to memory
				WriteMem(memRdAddr, (P_loopCnt & 0xf));

				// Set address for reading memory response data

				HtContinue(WR4_WRITE2);
			}
		}
		break;
		case WR4_WRITE2:
		{
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}

			// Calculate memory read address
			MemAddr_t memRdAddr = P_arrayAddr + (((P_loopCnt + 1) & 0xf) << 3);

			// Issue read request to memory
			WriteMem(memRdAddr, ((P_loopCnt + 1) & 0xf));

			HtContinue(WR4_WRITE3);
		}
		break;
		case WR4_WRITE3:
		{
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}

			// Calculate memory read address
			MemAddr_t memRdAddr = P_arrayAddr + (((P_loopCnt + 2) & 0xf) << 3);

			// Issue read request to memory
			WriteMem(memRdAddr, ((P_loopCnt + 2) & 0xf));

			HtContinue(WR4_WRITE4);
		}
		break;
		case WR4_WRITE4:
		{
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}

			// Calculate memory read address
			MemAddr_t memRdAddr = P_arrayAddr + (((P_loopCnt + 3) & 0xf) << 3);

			// Issue read request to memory
			WriteMem(memRdAddr, ((P_loopCnt + 3) & 0xf));

			HtContinue(WR4_LOOP);
		}
		break;
		case WR4_LOOP:
		{
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}

			// wait a few instructions for last response to line up with call to ReadMemPause
			if (P_pauseLoopCnt == 0) {
				P_pauseLoopCnt = 0;
				P_loopCnt += 1;

				if (PR_pauseDst)
					WriteMemPause(WR4_WRITE1a);
				else
					WriteMemPause(WR4_WRITE1b);
			} else {
				P_pauseLoopCnt += 1;
				HtContinue(WR4_LOOP);
			}
		}
		break;
		default:
			assert(0);
		}
	}

#ifndef _HTV
	if (r_m1_wrRspRdy) {
#if (WR4_HTID_W == 0)
		if (!r_wrGrpRsmWait)
			printf("-");
		else if (r_wrGrpRspCnt == 1)
			printf("1");
		else
			printf("+");
#else
#if (WR4_HTID_W < 2)
		if (!r_wrGrpRsmWait[INT(r_m1_wrRspTid & ((1 << WR2_WR_GRP_ID_W) - 1))])
			printf("-");
		else if (r_wrGrpRspCnt[INT(r_m1_wrRspTid & ((1 << WR2_WR_GRP_ID_W) - 1))] == 1)
			printf("4");
		else
			printf("+");
#else
		{
			m_wrGrpReqState1.read_addr(r_m1_wrGrpId);
			m_wrGrpRspState0.read_addr(r_m1_wrGrpId);
			CWrGrpRspState c_m1_wrGrpRspState = m_wrGrpRspState0.read_mem();
			CWrGrpReqState c_m1_wrGrpReqState = m_wrGrpReqState1.read_mem();

			if (c_m1_wrGrpReqState.m_pause != c_m1_wrGrpRspState.m_pause)
				printf("-");
			else if (c_m1_wrGrpReqState.m_cnt - c_m1_wrGrpRspState.m_cnt == 1)
				printf("4");
			else
				printf("+");
		}
#endif
#endif
	}
#endif
}
