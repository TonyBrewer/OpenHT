#include "Ht.h"
#include "PersF4.h"

void CPersF4::PersF4()
{
	if (PR1_htValid) {
		switch (PR1_htInst) {
		case F4_ENTRY:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			ht_uint2 rdGrpId = 0;

			S_rslt64[rdGrpId] = 0;

			ReadMem_func64(rdGrpId, PR1_addr, 0x15, 256);

			ReadMemPause(rdGrpId, F4_RD32);
			break;
		}
		case F4_RD32:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			HtAssert(SR_rslt64[0] == 9574400, 0);

			ht_uint2 rdGrpId = 1;

			S_rslt32[rdGrpId] = 0;

			ReadMem_func32(rdGrpId, PR1_addr + 256 * 8, 0x75, 64);

			ReadMemPause(rdGrpId, F4_RD16);
			break;
		}
		case F4_RD16:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			HtAssert(SR_rslt32[1] == 341376, 0);

			ht_uint2 rdGrpId = 2;

			ReadMem_func16(rdGrpId, PR1_addr, 0x9);

			ReadMemPause(rdGrpId, F4_RD64DLY);
			break;
		}
		case F4_RD64DLY:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			ht_uint2 rdGrpId = 3;

			S_rslt64Dly[rdGrpId] = 0;

			ReadMem_func64Dly(rdGrpId, PR1_addr, 0x19, 32);

			ReadMemPause(rdGrpId, F4_RD64GBL);
			break;
		}
		case F4_RD64GBL:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			HtAssert(SR_rslt64Dly[3] == 71424, 0);

			ht_uint2 rdGrpId = 3;

			ReadMem_gbl(rdGrpId, PR1_addr);

			P1_gvAddr = 3;

			ReadMemPause(rdGrpId, F4_RETURN);
			break;
		}
		case F4_RETURN: {
			if (SendReturnBusy_f4()) {
				HtRetry();
				break;
			}

			HtAssert(GR1_gbl.u64 == 123, 0);

			SendReturn_f4();
			break;
		}
		default:
			assert(0);
		}
	}

	T1_rdRspData = 0;
	S_rslt64Dly[3] += T11_rdRspData;
}

void CPersF4::ReadMemResp_func64(ht_uint8 rdRspElemIdx, sc_uint<F4_RD_GRP_ID_W> ht_noload rdRspGrpId, ht_uint5 ht_noload rdRspInfo, uint64_t rdRspData)
{
	S_rslt64[rdRspGrpId] += (uint64_t)(rdRspData * rdRspElemIdx);
}

void CPersF4::ReadMemResp_func32(ht_uint6 rdRspElemIdx, sc_uint<F4_RD_GRP_ID_W> ht_noload rdRspGrpId, ht_uint7 ht_noload rdRspInfo, uint32_t rdRspData)
{
	S_rslt32[rdRspGrpId] += (uint32_t)(rdRspData * rdRspElemIdx);
}

void CPersF4::ReadMemResp_func16(sc_uint<F4_RD_GRP_ID_W> ht_noload rdRspGrpId, ht_uint4 ht_noload rdRspInfo, int16_t ht_noload rdRspData)
{
	HtAssert(rdRspGrpId == 2, 0);
	HtAssert(rdRspData == 123, 0);
	HtAssert(rdRspInfo == 9, 0);
}

void CPersF4::ReadMemResp_func64Dly(ht_uint5 rdRspElemIdx, sc_uint<F4_RD_GRP_ID_W> ht_noload rdRspGrpId, ht_uint5 ht_noload rdRspInfo, uint64_t rdRspData)
{
	HtAssert(rdRspGrpId == 3, 0);
	T1_rdRspData = (uint64_t)(rdRspData * rdRspElemIdx);
}
