#include "Ht.h"
#include "PersTst.h"

void CPersTst::PersTst()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case TST_ENTRY:
			{
				if (ReadMemBusy() || SendCallBusy_rdo()) {
					HtRetry();
					break;
				}

				ht_uint48 coprocAddr = SR_coprocRdAddr+P_offset*8;
				ht_uint48 hostAddr = SR_hostRdAddr+P_offset*8;
				ht_uint9 idx = (ht_uint9)P_offset;
				ht_uint4 qwCnt = P_qwCnt & 0xf;

				ht_uint48 coprocWrAddr = SR_coprocWrAddr+P_offset*8;
				ht_uint48 hostWrAddr = SR_hostWrAddr+P_offset*8;

				switch (P_testMode) {
				case 0:
					ReadMem_sharedStruct_coproc(coprocAddr, idx, qwCnt);
					break;
				case 1:
					ReadMem_sharedVarIdx1_host(hostAddr, idx, qwCnt);
					break;
				case 2:
					ReadMem_sharedVarAddr1_host(hostAddr, idx, qwCnt);
					break;
				case 3:
					ReadMem_sharedDramAddr2_host(hostAddr, idx >> 6, idx & 0x3f, qwCnt);
					break;
				case 4:
					ReadMem_sharedBramAddr1_host(hostAddr, idx & 0x3f, idx >> 6, qwCnt);
					break;
				case 5:
					ReadMem_sharedQueue_host(hostAddr, qwCnt);
					break;
				case 6:
					ReadMem_sharedStruct_host(hostAddr, idx, qwCnt);
					break;
				case 7:
					SendCall_rdo(TST_RTN, P_testMode, hostAddr, hostWrAddr, idx, qwCnt);
					break;
				case 8:
					SendCall_rdo(TST_RTN, P_testMode, coprocAddr, coprocWrAddr, idx, qwCnt);
					break;
				default:
					HtAssert(0, 0);
				}

				if (P_testMode < 7)
					ReadMemPause(TST_S1);
			}
			break;
		case TST_S1:
			{
				if (WriteMemBusy()) {
					HtRetry();
					break;
				}

				ht_uint48 coprocAddr = SR_coprocWrAddr+P_offset*8;
				ht_uint48 hostAddr = SR_hostWrAddr+P_offset*8;
				ht_uint9 idx = (ht_uint9)P_offset;
				ht_uint4 qwCnt = P_qwCnt & 0xf;

				switch (P_testMode) {
				case 0:
					WriteMem_sharedStruct_coproc(coprocAddr, idx, qwCnt);
					break;
				case 1:
					WriteMem_sharedVarIdx1_host(hostAddr, idx, qwCnt);
					break;
				case 2:
					WriteMem_sharedVarAddr1_host(hostAddr, idx, qwCnt);
					break;
				case 3:
					WriteMem_sharedDramAddr2_host(hostAddr, idx >> 6, idx & 0x3f, qwCnt);
					break;
				case 4:
					WriteMem_sharedBramAddr1_host(hostAddr, idx & 0x3f, idx >> 6, qwCnt);
					break;
				case 5:
					WriteMem_sharedQueue_host(hostAddr, qwCnt);
					break;
				case 6:
					WriteMem_sharedStruct_host(hostAddr, idx, qwCnt);
					break;
				default:
					HtAssert(0, 0);
				}

				WriteMemPause(TST_RTN);
			}
			break;
		case TST_RTN:
			{
				if (SendReturnBusy_htmain()) {
					HtRetry();
					break;
				}

		//assert(r_tstToMif_reqAvlCnt == 32);
				SendReturn_htmain();
			}
			break;
		default:
			assert(0);
		}
	}
}
