#include "Ht.h"
#include "PersF2.h"

void CPersF2::PersF2()
{
	if (PR1_htValid) {
		switch (PR1_htInst) {
		case F2_ENTRY:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			S_rslt64 = 0;

			ReadMem_func64(PR1_addr, 0x15, 256);

			ReadMemPause(F2_RD32);
			break;
		}
		case F2_RD32:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			HtAssert(SR_rslt64 == 9574400, 0);

			S_rslt32 = 0;

			ReadMem_func32(PR1_addr + 256 * 8, 0x75, 64);

			ReadMemPause(F2_RD16);
			break;
		}
		case F2_RD16:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			HtAssert(SR_rslt32 == 341376, 0);

			ReadMem_func16(PR1_addr, 0x9);

			ReadMemPause(F2_RD64DLY);
			break;
		}
		case F2_RD64DLY:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			S_rslt64Dly = 0;

			ReadMem_func64Dly(PR1_addr, 0x19, 32);

			ReadMemPause(F2_RETURN);
			break;
		}
		case F2_RETURN: {
			if (SendReturnBusy_f2()) {
				HtRetry();
				break;
			}

			HtAssert(SR_rslt64Dly == 71424, 0);

			SendReturn_f2();
			break;
		}
		default:
			assert(0);
		}
	}

	T1_rdRspData = 0;
	S_rslt64Dly += T11_rdRspData;
}

void CPersF2::ReadMemResp_func64(ht_uint8 rdRspElemIdx, ht_uint5 ht_noload rdRspInfo, uint64_t rdRspData)
{
	S_rslt64 += (uint64_t)(rdRspData * rdRspElemIdx);
}

void CPersF2::ReadMemResp_func32(ht_uint6 rdRspElemIdx, ht_uint7 ht_noload rdRspInfo, uint32_t rdRspData)
{
	S_rslt32 += (uint32_t)(rdRspData * rdRspElemIdx);
}

void CPersF2::ReadMemResp_func16(ht_uint4 ht_noload rdRspInfo, int16_t ht_noload rdRspData)
{
	HtAssert(rdRspData == 123, 0);
	HtAssert(rdRspInfo == 9, 0);
}

void CPersF2::ReadMemResp_func64Dly(ht_uint5 rdRspElemIdx, ht_uint5 ht_noload rdRspInfo, uint64_t rdRspData)
{
	T1_rdRspData = (uint64_t)(rdRspData * rdRspElemIdx);
}
