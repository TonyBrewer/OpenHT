/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#if defined(VERT) || defined(_WIN32)
#include "Ht.h"
#include "PersVinfo.h"

#ifndef _HTV
#include "JobInfo.h"
#endif

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

// Read jobInfo.m_vert and send to vert modules
void CPersVinfo::PersVinfo()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case VINFO_ENTRY: {
			P_pJobInfo = PR_pJobInfo + JOB_INFO_VERT_OFFSET;

			P_readIdx = 0;
			P_readCnt = 3;	// total memory lines to read

			assert_msg(offsetof(JobInfo, m_vert) == JOB_INFO_VERT_OFFSET, "JOB_INFO_VERT_OFFSET = 0x%x\n", (int)offsetof(JobInfo, m_vert));
			P_memAddr = P_pJobInfo;

			HtContinue(VINFO_READ);
		}
		break;
		case VINFO_READ: {
			BUSY_RETRY (ReadMemBusy());

			ht_uint4 qwCnt = 8;

			ReadMem_ReadRspFunc(P_memAddr, P_readIdx, qwCnt);

			P_memAddr += 64;

			P_readIdx += 1;
			if (P_readIdx == P_readCnt)
				// must pause to get outImageRows and pntWghtListSize
				ReadMemPause(VINFO_PNT_WGHT_START);
			else
				HtContinue(VINFO_READ);
		}
		break;
		case VINFO_PNT_WGHT_START: {
			P_readIdx = START_OF_PNT_WGHT_START_QW/8;
			P_readCnt = P_readIdx + (S_outImageRows+1+31)/32;	// number of memory lines to pullin outImageRows+1 16-bit values

			P_memAddr = PR_pJobInfo + P_readIdx * 64;

			HtContinue(VINFO_PNT_WGHT_START_READ);
		}
		break;
		case VINFO_PNT_WGHT_START_READ: {
			BUSY_RETRY (ReadMemBusy());

			ht_uint4 qwCnt = 8;

			ReadMem_ReadRspFunc(P_memAddr, P_readIdx, qwCnt);

			P_memAddr += 64;

			P_readIdx += 1;
			if (P_readIdx == P_readCnt) {
				HtContinue(VINFO_PNT_WGHT_IDX);
			} else
				HtContinue(VINFO_PNT_WGHT_START_READ);
		}
		break;
		case VINFO_PNT_WGHT_IDX: {
			P_readIdx = START_OF_PNT_WGHT_IDX_QW/8;
			P_readCnt = P_readIdx + (S_outImageRows+31)/32;	// number of memory lines to pullin outImageRows+1 16-bit values

			P_memAddr = PR_pJobInfo + P_readIdx * 64;

			HtContinue(VINFO_PNT_WGHT_IDX_READ);
		}
		break;
		case VINFO_PNT_WGHT_IDX_READ: {
			BUSY_RETRY (ReadMemBusy());

			ht_uint4 qwCnt = 8;

			ReadMem_ReadRspFunc(P_memAddr, P_readIdx, qwCnt);

			P_memAddr += 64;

			P_readIdx += 1;
			if (P_readIdx == P_readCnt)
				HtContinue(VINFO_PNT_WGHT_LIST);
			else
				HtContinue(VINFO_PNT_WGHT_IDX_READ);
		}
		break;
		case VINFO_PNT_WGHT_LIST: {
			P_readIdx = START_OF_PNT_WGHT_LIST_QW/8;
			P_readCnt = P_readIdx + (S_pntWghtListSize*4+7)/8;	// number of memory lines to pullin outImageRows+1 16-bit values

			P_memAddr = PR_pJobInfo + P_readIdx * 64;

			HtContinue(VINFO_PNT_WGHT_LIST_READ);
		}
		break;
		case VINFO_PNT_WGHT_LIST_READ: {
			BUSY_RETRY (ReadMemBusy());

			ht_uint4 qwCnt = 8;

			ReadMem_ReadRspFunc(P_memAddr, P_readIdx, qwCnt);

			P_memAddr += 64;

			P_readIdx += 1;
			if (P_readIdx == P_readCnt)
				ReadMemPause(VINFO_TRANSFER);
			else
				HtContinue(VINFO_PNT_WGHT_LIST_READ);
		}
		break;
		case VINFO_TRANSFER: {
			BUSY_RETRY( SendTransferBusy_vctl() );

			VinfoMsg vinfoMsg;
			vinfoMsg.m_bImageRdy = true;
			vinfoMsg.m_imageIdx = P_imageIdx;

			SendMsg_vinfo( vinfoMsg );

			SendTransfer_vctl(PR_imageIdx);
		}
		break;
		default:
			assert(0);
		}
	}
}

// Send message to other vert modules
void CPersVinfo::ReadMemResp_ReadRspFunc(ht_uint3 rspIdx, sc_uint<VINFO_MIF_DST_READRSPFUNC_INFO_W> rdRsp_info, sc_uint<64> rdRspData)
{
	VinfoMsg vinfoMsg;
	vinfoMsg.m_bImageRdy = false;
	vinfoMsg.m_imageIdx = P_imageIdx;
	vinfoMsg.m_rspQw = (rdRsp_info << 3) | rspIdx;
	vinfoMsg.m_data = rdRspData;

	SendMsg_vinfo( vinfoMsg );

	if (!vinfoMsg.m_bImageRdy) {
		if (vinfoMsg.m_rspQw == 0)
			S_outImageRows = (vinfoMsg.m_data >> 30) & 0x3fff;

		if (vinfoMsg.m_rspQw == 18)
			S_pntWghtListSize = (vinfoMsg.m_data >> 16) & 0x3fff;
	}
}

#endif
