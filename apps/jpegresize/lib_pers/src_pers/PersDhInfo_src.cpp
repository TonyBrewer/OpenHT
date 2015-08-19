/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "PersConfig.h"

#if defined(DHINFO)

#include "Ht.h"
#include "PersDhInfo.h"

#ifndef _HTV
#include "JobInfo.h"
#define ASSERT_MSG assert_msg
#else
#define ASSERT_MSG(...)
#endif

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

// Read jobInfo.m_horz and send to horz modules
void CPersDhInfo::PersDhInfo()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case DHINFO_ENTRY: {
			P_pInfoBase = PR_pJobInfo + JOB_INFO_DEC_OFFSET;

			P_readIdx = 0;
			P_readCnt = 1;	// total memory lines to read

			ASSERT_MSG(offsetof(JobInfo, m_dec) == JOB_INFO_DEC_OFFSET, "JOB_INFO_DEC_OFFSET = 0x%x\n", (int)offsetof(JobInfo, m_dec));
			P_memAddr = P_pInfoBase;

			S_sectionId = JOB_INFO_SECTION_DEC;
			S_imageIdx = P_imageIdx;

			HtContinue(DINFO_READ);
		}
		break;
		case DINFO_READ: {
			BUSY_RETRY (ReadMemBusy());

			ht_uint4 qwCnt = 8;

			ReadMem_ReadRspFunc(P_memAddr, P_readIdx, qwCnt);

			P_memAddr += 64;

			P_readIdx += 1;
			if (P_readIdx == P_readCnt)
				// must pause to get outImageRows and pntWghtListSize
				ReadMemPause(DINFO_RSTINFO);
			else
				HtContinue(DINFO_READ);
		}
		break;
		case DINFO_RSTINFO: {
			// now that we have m_rstCnt, continue reading rstInfo
			P_readIdx = 1;
			P_readCnt = P_readIdx + (S_dec.m_rstCnt + 7)/8;	// number of memory lines to pullin

			HtContinue(DINFO_RSTINFO_READ);
		}
		break;
		case DINFO_RSTINFO_READ: {
			BUSY_RETRY (ReadMemBusy());

			ht_uint4 qwCnt = 8;

			ReadMem_ReadRspFunc(P_memAddr, P_readIdx, qwCnt);

			P_memAddr += 64;

			P_readIdx += 1;
			if (P_readIdx == P_readCnt)
				HtContinue(DINFO_DCDHT);
			else
				HtContinue(DINFO_RSTINFO_READ);
		}
		break;
		case DINFO_DCDHT: {
			#define START_OF_DEC_DCDHT_QW 0x208
			ASSERT_MSG(offsetof(JobInfo, m_dec.m_dcDht) - offsetof(JobInfo, m_dec) == START_OF_DEC_DCDHT_QW*8,
				"START_OF_DEC_DCDHT_QW = 0x%x\n", (int)(offsetof(JobInfo, m_dec.m_dcDht) - offsetof(JobInfo, m_dec))/8);
			ht_uint13 memLines = 0x8d;
			ASSERT_MSG((sizeof(JobDec) - (offsetof(JobInfo, m_dec.m_dcDht) - offsetof(JobInfo, m_dec)) + 63)/64 == memLines,
				"MEMLINES_OF_DEC_DCDHT = 0x%x\n",
				(sizeof(JobDec) - (offsetof(JobInfo, m_dec.m_dcDht) - offsetof(JobInfo, m_dec)) + 63)/64);

			P_readIdx = START_OF_DEC_DCDHT_QW/8;
			P_readCnt = P_readIdx + memLines;	// number of memory lines to pullin

			P_memAddr = PR_pInfoBase + P_readIdx * 64;

			HtContinue(DINFO_DCDHT_READ);
		}
		break;
		case DINFO_DCDHT_READ: {
			BUSY_RETRY (ReadMemBusy());

			ht_uint4 qwCnt = 8;

			ReadMem_ReadRspFunc(P_memAddr, P_readIdx, qwCnt);

			P_memAddr += 64;

			P_readIdx += 1;
			if (P_readIdx == P_readCnt)
				// Must pause because sectionId changes in HINFO_ENTRY
				ReadMemPause(HINFO_ENTRY);
			else
				HtContinue(DINFO_DCDHT_READ);
		}
		break;
		case HINFO_ENTRY: {
			P_pInfoBase = PR_pJobInfo + JOB_INFO_HORZ_OFFSET;

			P_readIdx = 0;
			P_readCnt = 3;	// total memory lines to read

			ASSERT_MSG(offsetof(JobInfo, m_horz) == JOB_INFO_HORZ_OFFSET, "JOB_INFO_HORZ_OFFSET = 0x%x\n", (int)offsetof(JobInfo, m_horz));
			P_memAddr = P_pInfoBase;

			S_sectionId = JOB_INFO_SECTION_HORZ;

			HtContinue(HINFO_READ);
		}
		break;
		case HINFO_READ: {
			BUSY_RETRY (ReadMemBusy());

			ht_uint4 qwCnt = 8;

			ReadMem_ReadRspFunc(P_memAddr, P_readIdx, qwCnt);

			P_memAddr += 64;

			P_readIdx += 1;
			if (P_readIdx == P_readCnt)
				// must pause to get outImageRows and pntWghtListSize
				ReadMemPause(HINFO_PNT_INFO);
			else
				HtContinue(HINFO_READ);
		}
		break;
		case HINFO_PNT_INFO: {
			ASSERT_MSG(offsetof(JobHorz, m_pntInfo)/8 == HINFO_PNT_INFO_QOFF,
				"HINFO_PNT_INFO_QOFF = 0x%x\n", (int)offsetof(JobHorz, m_pntInfo)/8);

			P_readIdx = HINFO_PNT_INFO_QOFF/8;
			ht_uint13 outRdCnt = P_readIdx + (S_horz.m_outImageCols+7)/8;     // number of memory lines to pullin outImageRows+1 16-bit values
			P_readCnt = P_readIdx + S_horz.m_inImageCols/16 + 2;	// number of memory lines to pullin inImageRows/2+1 16-bit values
			if (outRdCnt > P_readCnt)
				P_readCnt = outRdCnt;
			P_memAddr = PR_pInfoBase + P_readIdx * 64;
			HtContinue(HINFO_PNT_INFO_READ);
		}
		break;
		case HINFO_PNT_INFO_READ: {
			BUSY_RETRY (ReadMemBusy());

			ht_uint4 qwCnt = 8;

			ReadMem_ReadRspFunc(P_memAddr, P_readIdx, qwCnt);

			P_memAddr += 64;

			P_readIdx += 1;
			if (P_readIdx == P_readCnt) {
				HtContinue(HINFO_PNT_WGHT_LIST);
			} else
				HtContinue(HINFO_PNT_INFO_READ);
		}
		break;
		case HINFO_PNT_WGHT_LIST: {
			ASSERT_MSG((offsetof(JobInfo, m_horz.m_pntWghtList) - offsetof(JobInfo, m_horz))/8 == HINFO_PNT_WGHT_LIST_QOFF,
				"HINFO_PNT_WGHT_LIST_QOFF = 0x%x\n", (int)(offsetof(JobInfo, m_horz.m_pntWghtList) - offsetof(JobInfo, m_horz))/8);

			P_readIdx = HINFO_PNT_WGHT_LIST_QOFF/8;
			P_readCnt = P_readIdx + (S_horz.m_pntWghtListSize*4+7)/8;	// number of memory lines to pullin outImageRows+1 16-bit values

			P_memAddr = PR_pInfoBase + P_readIdx * 64;

			HtContinue(HINFO_PNT_WGHT_LIST_READ);
		}
		break;
		case HINFO_PNT_WGHT_LIST_READ: {
			BUSY_RETRY (ReadMemBusy());

			ht_uint4 qwCnt = 8;

			ReadMem_ReadRspFunc(P_memAddr, P_readIdx, qwCnt);

			P_memAddr += 64;

			P_readIdx += 1;
			if (P_readIdx == P_readCnt)
				ReadMemPause(DHINFO_RETURN);
			else
				HtContinue(HINFO_PNT_WGHT_LIST_READ);
		}
		break;
		case DHINFO_RETURN: {
			BUSY_RETRY( SendReturnBusy_dhInfo() );

			SendReturn_dhInfo();
		}
		break;
		default:
			assert(0);
		}
	}
}

// Send message to other horz modules
void CPersDhInfo::ReadMemResp_ReadRspFunc(ht_uint3 rspIdx, sc_uint<JOB_INFO_MEM_LINE_IDX_W> rdRsp_info, uint64_t rdRspData)
{
	JobInfoMsg hinfoMsg;
	hinfoMsg.m_sectionId = S_sectionId;
	hinfoMsg.m_imageIdx = S_imageIdx;
	hinfoMsg.m_rspQw = (rdRsp_info << 3) | rspIdx;
	hinfoMsg.m_data = rdRspData;

	SendMsg_jobInfo( hinfoMsg );

	if (hinfoMsg.m_sectionId == JOB_INFO_SECTION_DEC) {
		if (hinfoMsg.m_rspQw == 1)
			S_dec.m_rstCnt = (hinfoMsg.m_data >> 16) & MCU_ROWS_MASK;
	}

	if (hinfoMsg.m_sectionId == JOB_INFO_SECTION_HORZ) {
		if (hinfoMsg.m_rspQw == 0) {
			S_horz.m_inImageCols = (hinfoMsg.m_data >> 16) & IMAGE_COLS_MASK;
            S_horz.m_outImageCols = (hinfoMsg.m_data >> 44) & IMAGE_COLS_MASK;
		}

		if (hinfoMsg.m_rspQw == 18)
			S_horz.m_pntWghtListSize = (hinfoMsg.m_data >> 16) & IMAGE_COLS_MASK;
	}
}

#endif
