#include "Ht.h"
#include "PersRdo.h"

// Test description
//  1. Read 64-bytes from coproc memory (initialized by host)
//  2. Read various alignments and qwCnts from coproc memory
//  3. Repeat steps 1 and 2 for host memory (initialized by host)

void CPersRdo::PersRdo()
{
#ifndef _HTV
	//if (HT_CYCLE() == 3642)
	//	bool stop = true;
#endif
	T1_bHtResume = false;

	if (PR_htValid) {
		switch (PR_htInst) {
		case RDO0:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			// Make request for multi-quadword access to coproc memory
			switch (P_testMode) {
			case 7:
				ReadMem_shReg_host(P_rdAddr, P_idx, P_qwCnt);
				break;
			case 8:
				ReadMem_shReg_coproc(P_rdAddr, P_idx, P_qwCnt);
				break;
			default:
				assert(0);
			}

			ReadMemPause(RDO1);
		}
		break;
		case RDO1:
		{
			T1_bHtResume = true;
			T1_rsmHtId = PR_htId;
			HtPause(RDO2);
		}
		break;
		case RDO2:
		{
			// call routine to verify read results
			if (SendTransferBusy_wro()) {
				HtRetry();
				break;
			}

			SendTransfer_wro(P_testMode, P_wrAddr, P_idx, P_qwCnt,
				S_shReg[P_idx+0], S_shReg[P_idx+1], S_shReg[P_idx+2], S_shReg[P_idx+3],
				S_shReg[P_idx+4], S_shReg[P_idx+5], S_shReg[P_idx+6], S_shReg[P_idx+7]);

			break;
		}
		default:
			assert(0);
		}
	}

	if (T2_bHtResume)
		HtResume(T2_rsmHtId);
}
