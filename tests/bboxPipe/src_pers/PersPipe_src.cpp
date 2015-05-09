#include "Ht.h"
#include "PersPipe.h"
#include "PersPipe_prim.h"


void
CPersPipe::PersPipe()
{
	T1_cmpValid = false;
	T1_addr = (MemAddr_t)0;
	T1_operation = (ht_uint12)0;
	T1_scalar = (uint64_t)0;

	if (PR_htValid) {
		switch (PR_htInst) {
		case PIPE_ENTRY: {
			if (SendReturnBusy_read() || S_writeQueue.full(25)) {
				HtRetry();
				break;
			}
			if (P_flush) {
				HtContinue(PIPE_DRAIN);
			} else {
				// fire off the pipe and return to ctl
				T1_cmpValid = true;
				S_cmpValidCnt += 1;
				T1_addr = P_vtAddr;
				T1_operation = P_operation;
				T1_scalar = P_scalar;
				SendReturn_read(P_idx);
			}
		}
		break;
		case PIPE_DRAIN: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			if (S_cmpValidCnt == 0 && S_writeQueue.empty())
				WriteMemPause(SR_wrGrp, PIPE_RTN);
			else
				HtContinue(PIPE_DRAIN);
		}
		break;
		case PIPE_RTN: {
			if (SendReturnBusy_read()) {
				HtRetry();
				break;
			}
			SendReturn_read(P_idx);
		}
		break;
		default:
			assert(0);
		}
	}

	// operations can be launched through this pipe every cycle.

	T1_opA = GR1_opMem.opA;
	T1_opB = GR1_opMem.opB;

	// may not need the 26th stage for rsltT, but not worried about pipe length at the moment
	bbox_wrap(T25_rsltVm, T25_rsltT,
		  r_reset1x, T1_scalar, T1_operation, T1_opA, T1_opB,
		  bbox_prim_state1);

	if (T26_cmpValid) {
		CWriteQueue c_pushInfo;
		c_pushInfo.m_rsltT = T26_rsltT;
		c_pushInfo.m_addr = T26_addr;
		c_pushInfo.m_hit = T26_rsltVm;
		S_writeQueue.push(c_pushInfo);
		S_cmpValidCnt -= 1;
	}
	if (!S_writeQueue.empty() && !WriteMemBusy()) {
		// hit is always 0 so not adding logic to send a host message
		CWriteQueue c_popInfo;
		c_popInfo = S_writeQueue.front();
		S_writeQueue.pop();
		WriteMem(SR_wrGrp, c_popInfo.m_addr, c_popInfo.m_rsltT);
	}

	if (GR_htReset)
		S_cmpValidCnt = 0;
}
