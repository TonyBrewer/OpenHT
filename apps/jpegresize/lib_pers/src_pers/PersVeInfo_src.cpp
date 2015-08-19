/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "PersConfig.h"

#if defined(VEINFO)

#include "Ht.h"
#include "PersVeInfo.h"

#ifndef _HTV
#include "JobInfo.h"
#define ASSERT_MSG assert_msg
#else
#define ASSERT_MSG(...)
#endif

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

// Read jobInfo.m_vert and send to vert modules
void CPersVeInfo::PersVeInfo()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case VEINFO_ENTRY: {
			// first read encode info, 53 memory lines
			P_memAddr = PR_pJobInfo + JOB_INFO_ENC_OFFSET;

			P_readIdx = 0;
			P_readCnt = 59;	// total memory lines to read

			S_imageIdx = P_imageIdx;

			ASSERT_MSG(offsetof(JobInfo, m_enc) == JOB_INFO_ENC_OFFSET, "JOB_INFO_ENC_OFFSET = 0x%x\n", (int)offsetof(JobInfo, m_enc));

			HtContinue(EINFO_READ);
		}
		break;
		case EINFO_READ: {
			BUSY_RETRY (ReadMemBusy());

			ht_uint4 qwCnt = 8;

			JobInfoReqInfo_t reqInfo = (P_readIdx << 2) | JOB_INFO_SECTION_ENC;

			ReadMem_ReadRspFunc(P_memAddr, reqInfo, qwCnt);

			P_memAddr += 64;

			P_readIdx += 1;
			if (P_readIdx == P_readCnt)
				// must pause to get outImageRows and pntWghtListSize
				HtContinue(VINFO_ENTRY);
			else
				HtContinue(EINFO_READ);
		}
		break;
		case VINFO_ENTRY: {
			P_pJobInfo = PR_pJobInfo + JOB_INFO_VERT_OFFSET;

			P_readIdx = 0;
			P_readCnt = 3;	// total memory lines to read

			ASSERT_MSG(offsetof(JobInfo, m_vert) == JOB_INFO_VERT_OFFSET, "JOB_INFO_VERT_OFFSET = 0x%x\n", (int)offsetof(JobInfo, m_vert));
			P_memAddr = P_pJobInfo;

			HtContinue(VINFO_READ);
		}
		break;
		case VINFO_READ: {
			BUSY_RETRY (ReadMemBusy());

			ht_uint4 qwCnt = 8;

			JobInfoReqInfo_t reqInfo = (P_readIdx << 2) | JOB_INFO_SECTION_VERT;

			ReadMem_ReadRspFunc(P_memAddr, reqInfo, qwCnt);

			P_memAddr += 64;

			P_readIdx += 1;
			if (P_readIdx == P_readCnt)
				// must pause to get outImageRows and pntWghtListSize
				ReadMemPause(VINFO_PNT_INFO);
			else
				HtContinue(VINFO_READ);
		}
		break;
		case VINFO_PNT_INFO: {
			P_readIdx = START_OF_PNT_INFO_QOFF/8;
			ht_uint13 outRdCnt = P_readIdx + (S_outImageRows+8)/8;   // number of memory lines to pull in outImageRows+1 16-bit values
			P_readCnt = P_readIdx + S_inImageRows/16 + 2;	// number of memory lines to pull in (inImageRows+1)/2 16-bit values
			if (outRdCnt > P_readCnt)
				P_readCnt = outRdCnt;

			P_memAddr = PR_pJobInfo + P_readIdx * 64;

			HtContinue(VINFO_PNT_INFO_READ);
		}
		break;
		case VINFO_PNT_INFO_READ: {
			BUSY_RETRY (ReadMemBusy());

			ht_uint4 qwCnt = 8;

			JobInfoReqInfo_t reqInfo = (P_readIdx << 2) | JOB_INFO_SECTION_VERT;

			ReadMem_ReadRspFunc(P_memAddr, reqInfo, qwCnt);

			P_memAddr += 64;

			P_readIdx += 1;
			if (P_readIdx == P_readCnt) {
				HtContinue(VINFO_PNT_WGHT_LIST);
			} else
				HtContinue(VINFO_PNT_INFO_READ);
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

			JobInfoReqInfo_t reqInfo = (P_readIdx << 2) | JOB_INFO_SECTION_VERT;

			ReadMem_ReadRspFunc(P_memAddr, reqInfo, qwCnt);

			P_memAddr += 64;

			P_readIdx += 1;
			if (P_readIdx == P_readCnt)
				ReadMemPause(VINFO_RETURN);
			else
				HtContinue(VINFO_PNT_WGHT_LIST_READ);
		}
		break;
		case VINFO_RETURN: {
			BUSY_RETRY( SendReturnBusy_veInfo() );

			SendReturn_veInfo();
		}
		break;
		default:
			assert(0);
		}
	}
}

// Send message to other vert modules
void CPersVeInfo::ReadMemResp_ReadRspFunc(ht_uint3 rspIdx, sc_uint<JOB_INFO_MEM_LINE_IDX_W+JOB_INFO_SECTION_ID_W> rdRsp_info, uint64_t rdRspData)
{
	JobInfoMsg veInfoMsg;
	veInfoMsg.m_imageIdx = S_imageIdx;
	veInfoMsg.m_sectionId = rdRsp_info & ((1 << JOB_INFO_SECTION_ID_W)-1);
	veInfoMsg.m_rspQw = ((rdRsp_info >> JOB_INFO_SECTION_ID_W) << JOB_INFO_QW_IDX_W) | rspIdx;
	veInfoMsg.m_data = rdRspData;

	SendMsg_veInfo( veInfoMsg );

	if (veInfoMsg.m_sectionId == JOB_INFO_SECTION_VERT) {
		if (veInfoMsg.m_rspQw == 0) {
			S_outImageRows = (veInfoMsg.m_data >> 30) & 0x3fff;
			S_inImageRows = (veInfoMsg.m_data >> 2) & 0x3fff;
		}

		if (veInfoMsg.m_rspQw == 18)
			S_pntWghtListSize = (veInfoMsg.m_data >> 16) & 0x3fff;
	}
}

#endif
