#include "Ht.h"
#include "PersRd2.h"

#if (RD2_ADDR1_W == 1)
#define PR_HTID 0
#else
#define PR_HTID (Rd2Addr1_t)PR_htId
#endif

void CPersRd2::PersRd2()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case RD2_INIT:
		{
			P_loopCnt = 0;
			P_err = 0;
			P_pauseLoopCnt = 0;

			P_arrayMemRd1Ptr = PR_HTID;

			HtContinue(RD2_READ1);
		}
		break;
		case RD2_READ1:
		{
			if (ReadMemBusy() || SendReturnBusy_rd2()) {
				HtRetry();
				break;
			}

			// Check if end of loop
			if (P_loopCnt == PAUSE_LOOP_CNT || P_err) {
				// Return to host interface
				SendReturn_rd2(P_err);
			} else {
				// Calculate memory read address
				MemAddr_t memRdAddr = P_arrayAddr + ((P_loopCnt & 0xf) << 3);

				// Issue read request to memory
				ReadMem_rd2Mem(memRdAddr, PR_HTID, 0);

				// Set address for reading memory response data

				HtContinue(RD2_READ2);
			}
		}
		break;
		case RD2_READ2:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			// Calculate memory read address
			MemAddr_t memRdAddr = P_arrayAddr + (((P_loopCnt + 1) & 0xf) << 3);

			// Issue read request to memory
			ReadMem_rd2Mem(memRdAddr, PR_HTID, 1);

			HtContinue(RD2_READ3);
		}
		break;
		case RD2_READ3:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			// Calculate memory read address
			MemAddr_t memRdAddr = P_arrayAddr + (((P_loopCnt + 2) & 0xf) << 3);

			// Issue read request to memory
			ReadMem_rd2Mem(memRdAddr, PR_HTID, 2);

			HtContinue(RD2_READ4);
		}
		break;
		case RD2_READ4:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			// Calculate memory read address
			MemAddr_t memRdAddr = P_arrayAddr + (((P_loopCnt + 3) & 0xf) << 3);

			// Issue read request to memory
			ReadMem_rd2Mem(memRdAddr, PR_HTID, 3);

			HtContinue(RD2_LOOP);
		}
		break;
		case RD2_LOOP:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			// wait a few instructions for last response to line up with call to ReadMemPause
			if (P_pauseLoopCnt == 3) {
				P_pauseLoopCnt = 0;
				P_arrayMemRd2Ptr = 0;

				ReadMemPause(RD2_TEST1);
			} else {
				P_pauseLoopCnt += 1;
				HtContinue(RD2_LOOP);
			}
		}
		break;
		case RD2_TEST1:
		{
			if (GR_rd2Mem_data() != (P_loopCnt & 0xf)) {
				HtAssert(0, 0);
				P_err += 1;
			}

			P_arrayMemRd2Ptr = 1;

			HtContinue(RD2_TEST2);
		}
		break;
		case RD2_TEST2:
		{
			if (GR_rd2Mem_data() != ((P_loopCnt + 1) & 0xf)) {
				HtAssert(0, 0);
				P_err += 1;
			}

			P_arrayMemRd2Ptr = 2;

			HtContinue(RD2_TEST3);
		}
		break;
		case RD2_TEST3:
		{
			if (GR_rd2Mem_data() != ((P_loopCnt + 2) & 0xf)) {
				HtAssert(0, 0);
				P_err += 1;
			}

			P_arrayMemRd2Ptr = 3;

			HtContinue(RD2_TEST4);
		}
		break;
		case RD2_TEST4:
		{
			if (GR_rd2Mem_data() != ((P_loopCnt + 3) & 0xf)) {
				HtAssert(0, 0);
				P_err += 1;
			}

			// Pauserement loop count
			P_loopCnt = P_loopCnt + 1;

			HtContinue(RD2_READ1);
		}
		break;
		default:
			assert(0);
		}
	}

	if (r_m1_rdRspRdy) {
#ifndef _HTV
#if (RD2_ADDR1_W == 1)
		if (!r_rdGrpRsmWait)
			printf("-");
		else if (r_rdGrpRspCnt == 1)
			printf("2");
		else
			printf("+");
#else
#if (RD2_ADDR1_W < 3)
		if (!r_rdGrpRsmWait[INT(r_m1_rdRspGrpId)])
			printf("-");
		else if (r_rdGrpRspCnt[INT(r_m1_rdRspGrpId)] == 1)
			printf("2");
		else
			printf("+");
#else
		m_rdGrpRsmWait[r_m1_rdRspGrpId & 3].read_addr(r_m1_rdRspGrpId >> 2);
		if (!m_rdGrpRsmWait[r_m1_rdRspGrpId & 3].read_mem())
			printf("-");
		else if (r_rdGrpRspCnt[r_m1_rdRspGrpId & 3] == 1)
			printf("2");
		else
			printf("+");
#endif
#endif
#endif
	}
}
