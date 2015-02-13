#include "Ht.h"
#include "PersCtl.h"

void
CPersCtl::PersCtl()
{
	bool idxPoolBusy = S_idxPoolInitCnt < (1u << THREAD_W) || S_idxPool.empty();
	if (PR_htValid) {
		switch (PR_htInst) {
		case CTL_ENTRY:
		{
			if (SendReturnBusy_htmain()) {
				HtRetry();
				break;
			}

			P_opTotal = 0;
			P_vecIdx = 0;

			if (P_operation != 0)                           // currently only support an opcode of 0
				SendReturn_htmain(P_opTotal);           // an empty block, just return
			else
				HtContinue(CTL_READ);
		}
		break;
		case CTL_READ:
		{
			if (SendCallBusy_read() || idxPoolBusy) {
				HtRetry();
				break;
			}
			MemAddr_t c_offset = (MemAddr_t)(P_vecIdx << 3);
			bool flush;
			if (P_opTotal < P_vecLen) {
				flush = false;
				S_threadActiveCnt += 1;
				HtContinue(CTL_READ);
			} else {
				// this call flushes the pipe
				// addresses and operation are not used
				flush = true;
				HtContinue(CTL_DRAIN);
			}
			// will launch one every ~6 cycles
			// since the pipe is long enough this will result in a slow ramp until all available threads
			// are used
			P_idx = S_idxPool.front();
			S_idxPool.pop();
			SendCallFork_read(CTL_JOIN, P_idx, P_vaBase + c_offset, P_vbBase + c_offset, P_vtBase + c_offset, P_scalar,
					  P_operation, flush);
			P_opTotal += 1;
			P_vecIdx += P_auStride;
		}
		break;
		case CTL_DRAIN:
		{
			// Wait for all read asyncCalls to complete
			RecvReturnPause_read(CTL_RTN);
		}
		break;
		case CTL_JOIN:
		{
			// control was passed from read to pipe
			S_writeCnt += 1;
			S_threadActiveCnt -= 1;
			S_idxPool.push(P_idx);
			RecvReturnJoin_read();
		}
		break;
		case CTL_RTN:
		{
			if (SendReturnBusy_htmain()) {
				HtRetry();
				break;
			}
			SendReturn_htmain(S_writeCnt - 1);
		}
		break;
		default:
			assert(0);
		}
	}
	if (GR_htReset) {
		S_threadActiveCnt = 0;
		S_writeCnt = 0;
		S_idxPoolInitCnt = 0;
	} else if (S_idxPoolInitCnt < (1u << THREAD_W)) {
		S_idxPool.push(S_idxPoolInitCnt & ((1ul << THREAD_W) - 1));
		S_idxPoolInitCnt += 1;
	}
}
