#include "Ht.h"
#include "PersWro.h"

void CPersWro::PersWro()
{
	T1_bHtResume = false;

	if (PR_htValid) {
		switch (PR_htInst) {
		case WRO0:
		{
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}

			S_shReg[P_idx+0] = P_p0;
			S_shReg[P_idx+1] = P_p1;
			S_shReg[P_idx+2] = P_p2;
			S_shReg[P_idx+3] = P_p3;
			S_shReg[P_idx+4] = P_p4;
			S_shReg[P_idx+5] = P_p5;
			S_shReg[P_idx+6] = P_p6;
			S_shReg[P_idx+7] = P_p7;

			// Make request for multi-quadword access to coproc memory
			switch (P_testMode) {
			case 7:
				WriteMem_shReg_host(P_wrAddr, P_idx, P_qwCnt);
				break;
			case 8:
				WriteMem_shReg_coproc(P_wrAddr, P_idx, P_qwCnt);
				break;
			default:
				assert(0);
			}

			WriteMemPause(WRO1);
		}
		break;
		case WRO1:
		{
			T1_bHtResume = true;
			T1_rsmHtId = (Wro_RsmHtId_t) PR_htId;
			HtPause(WRO2);
		}
		break;
		case WRO2:
		{
			// call routine to verify read results
			if (SendReturnBusy_rdo()) {
				HtRetry();
				break;
			}

			SendReturn_rdo();

			break;
		}
		default:
			assert(0);
		}
	}

	if (T2_bHtResume)
#if (WRO_HTID_W != 0)
		HtResume((sc_uint<WRO_HTID_W>)T2_rsmHtId);
#else
		HtResume();
#endif
}
