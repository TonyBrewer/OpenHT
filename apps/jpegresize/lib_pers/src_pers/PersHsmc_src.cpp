/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "PersConfig.h"

#if defined(HSMC)

#include "Ht.h"
#include "PersHsmc.h"

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

/////////////////////////////////////
// Mimic messaging from Jpeg Decoder
//	 Each decoder will handle a single image before transitioning the another image.
//   Each decoder can handle a fixed number of MCU rows at a time.
//   The number of rows that can be simultaneously handled is a build time parameter.
//   A mcu block is transmitted as eight successive message on consecutive clocks.
//   A restart is handled by a single IHUF module.

// this module starts a thread on hsmw for each restart of an image

void CPersHsmc::PersHsmc()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case HSMC_ENTRY: {
			P_rstIdx = 0;
			HtContinue(HSMC_START);
		}
		break;
		case HSMC_START: {
			BUSY_RETRY( SendCallBusy_hsmw() );

			if (P_rstIdx == S_dec.m_rstCnt)
				RecvReturnPause_hsmw(HSMC_RETURN);
			else {
				bool bSingleRst = S_dec.m_rstCnt == 1;
				SendCallFork_hsmw( HSMC_JOIN, P_imageIdx, bSingleRst, P_rstIdx );

				HtContinue( HSMC_START );
			}

			P_rstIdx += 1;
		}
		break;
		case HSMC_JOIN: {
			RecvReturnJoin_hsmw();
		}
		break;
		case HSMC_RETURN: {
			BUSY_RETRY( SendReturnBusy_hsmc() );

			SendReturn_hsmc();
		}
		break;
		default:
			assert(0);
		}
	}

	RecvJobInfo();
}

void CPersHsmc::RecvJobInfo()
{
	// Receive hinfo messages, these may come in any order so
	//   keep a count to know when all have arrived

	if (!GR_htReset && !RecvMsgBusy_jobInfo()) {
		JobInfoMsg hinfoMsg = RecvMsg_jobInfo();

		if (hinfoMsg.m_sectionId == JOB_INFO_SECTION_DEC) {
			switch (hinfoMsg.m_rspQw) {
			case 1: // second quadword of jobInfo.m_dec
				{
					S_dec.m_rstCnt = (hinfoMsg.m_data >> 16) & 0x7ff;
				}
				break;
			default:
				break;
			}
		}
	}
}

#endif
