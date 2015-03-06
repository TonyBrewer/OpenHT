/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "PersConfig.h"

#if defined(VWM)

#include "Ht.h"
#include "PersVwm.h"

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

void CPersVwm::PersVwm()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case VWM_ENTRY: {
			S_mcuRowCompleteCnt = 0;

			HtContinue(VWM_LOOP);
		}
		break;
		case VWM_LOOP: {

			if (S_mcuRowCompleteCnt < S_outMcuRows)
				HtContinue(VWM_LOOP);
			else
				HtContinue(VWM_RETURN);
		}
		break;
		case VWM_RETURN: {
			BUSY_RETRY(SendReturnBusy_vwm());

			SendReturn_vwm();
		}
		break;
		default:
			assert(0);
		}
	}
#if defined(FORCE_VWM_STALL) && !defined(_HTV)
	static int stallCnt = 0;
	stallCnt += 1;
#endif

	if (RecvMsgReady_jem()
#if defined(FORCE_VWM_STALL) && !defined(_HTV)
		&& stallCnt >= 8
#endif
		) {
		JpegEncMsg ht_noload jem = RecvMsg_jem();

		if (jem.m_bEndOfMcuRow)
			printf("VSM: mcuRow %d, bEndOfMcuRow %d, @ %lld\n", (int)jem.m_rstIdx, (int)jem.m_bEndOfMcuRow, HT_CYCLE());

		if (jem.m_bEndOfMcuRow)
			S_mcuRowCompleteCnt += 1;

#if defined(FORCE_VWM_STALL) && !defined(_HTV)
		stallCnt = 0;
#endif
	}

	// Receive vinfo messages, these may come in any order so
	//   keep a count to know when all have arrived

	if (!GR_htReset && !RecvMsgBusy_veInfo()) {
		JobInfoMsg veInfoMsg = RecvMsg_veInfo();

		if (veInfoMsg.m_sectionId == JOB_INFO_SECTION_VERT) {
			switch (veInfoMsg.m_rspQw) {
			case 1: // first quadword of jobInfo.m_vert
				{
					S_outMcuRows = (veInfoMsg.m_data >> 22) & 0x7ff;
				}
				break;
			default:
				{
				}
				break;
			}
		}
	}
}

#endif
