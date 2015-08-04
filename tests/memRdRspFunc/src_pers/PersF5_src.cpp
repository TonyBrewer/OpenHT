#include "Ht.h"
#include "PersF5.h"

void CPersF5::PersF5()
{
	if (PR1_htValid) {
		switch (PR1_htInst) {
		case F5_ENTRY:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			S_rslt64[PR1_htId] = 0;

			ReadMem_func64(PR1_addr, 0x15, 256);

			ReadMemPause(F5_RD32);
			break;
		}
		case F5_RD32:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			HtAssert(SR_rslt64[PR1_htId] == 9574400, 0);

			S_rslt32[PR1_htId] = 0;

			ReadMem_func32(PR1_addr + 256 * 8, 0x75, 64);

			ReadMemPause(F5_RD16);
			break;
		}
		case F5_RD16:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			HtAssert(SR_rslt32[PR1_htId] == 341376, 0);

			ReadMem_func16(PR1_addr, 0x9);

			ReadMemPause(F5_RD64DLY);
			break;
		}
		case F5_RD64DLY:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			S_rslt64Dly[PR1_htId] = 0;

			ReadMem_func64Dly(PR1_addr, 0x19, 32);

			ReadMemPause(F5_RETURN);
			break;
		}
		case F5_RETURN: {
			if (SendReturnBusy_f5()) {
				HtRetry();
				break;
			}

			HtAssert(SR_rslt64Dly[PR1_htId] == 71424, 0);

			SendReturn_f5();
			break;
		}
		default:
			assert(0);
		}
	}

	T1_rdRspData = 0;
	S_rslt64Dly[0] += T11_rdRspData;
}

void CPersF5::ReadMemResp_func64(ht_uint8 rdRspElemIdx, sc_uint<F5_RD_GRP_ID_W> ht_noload rdRspGrpId, ht_uint5 ht_noload rdRspInfo, uint64_t rdRspData)
{
	S_rslt64[rdRspGrpId] += (uint64_t)(rdRspData * rdRspElemIdx);
}

void CPersF5::ReadMemResp_func32(ht_uint6 rdRspElemIdx, sc_uint<F5_RD_GRP_ID_W> ht_noload rdRspGrpId, ht_uint7 ht_noload rdRspInfo, uint32_t rdRspData)
{
	S_rslt32[rdRspGrpId] += (uint32_t)(rdRspData * rdRspElemIdx);
}

void CPersF5::ReadMemResp_func16(sc_uint<F5_RD_GRP_ID_W> ht_noload rdRspGrpId, ht_uint4 ht_noload rdRspInfo, int16_t ht_noload rdRspData)
{
	HtAssert(rdRspData == 123, 0);
	HtAssert(rdRspInfo == 9, 0);
}

void CPersF5::ReadMemResp_func64Dly(ht_uint5 rdRspElemIdx, sc_uint<F5_RD_GRP_ID_W> ht_noload rdRspGrpId, ht_uint5 ht_noload rdRspInfo, uint64_t rdRspData)
{
	T1_rdRspData = (uint64_t)(rdRspData * rdRspElemIdx);
}
