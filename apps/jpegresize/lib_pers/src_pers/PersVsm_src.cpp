/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "PersConfig.h"

#if defined(VSM)

#include "Ht.h"
#include "PersVsm.h"

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

void
CPersVsm::PersVsm()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case VSM_ENTRY: {

			P_msgRow = 0;

			HtContinue(VSM_SEND);
		}
		break;
		case VSM_SEND: {
			// send series of message mimicing horz resizing
			BUSY_RETRY(SendMsgBusy_hrm());
			HorzResizeMsg hrm;

			hrm.m_mcuRow = PR_msgRow;
			hrm.m_bEndOfImage = P_msgRow+1 == S_mcuRows;

			SendMsg_hrm(hrm);

			if (hrm.m_bEndOfImage)
				printf("PersVsm: mcuRow %d, bEndOfImage %d\n", (int)hrm.m_mcuRow, (int)hrm.m_bEndOfImage);

			P_msgRow += 1;
			if (P_msgRow >= S_mcuRows)
				HtContinue(VSM_RETURN);
			else
				HtContinue(VSM_SEND);
		}
		break;
		case VSM_RETURN: {
			BUSY_RETRY (SendReturnBusy_vsm());

			SendReturn_vsm();
		}
		break;
		default:
			assert(0);
		}
	}

	// Receive vinfo messages
	if (!GR_htReset && !RecvMsgBusy_veInfo()) {
		JobInfoMsg veInfoMsg = RecvMsg_veInfo();

		if (veInfoMsg.m_sectionId == JOB_INFO_SECTION_VERT) {
			switch (veInfoMsg.m_rspQw) {
			case 0: // first quadword of jobInfo.m_vert
				{
					ImageRows_t inImageRows = (veInfoMsg.m_data >> 2) & IMAGE_ROWS_MASK;
					ImageCols_t inImageCols = (veInfoMsg.m_data >> 16) & IMAGE_COLS_MASK;
					ht_uint2 maxBlkRowsPerMcu = (veInfoMsg.m_data >> 58) & 0x3;
					ht_uint2 maxBlkColsPerMcu = (veInfoMsg.m_data >> 60) & 0x3;

					S_mcuRows = ((inImageRows + DCTSIZE * maxBlkRowsPerMcu -1) / (DCTSIZE * maxBlkRowsPerMcu)) & MCU_ROWS_MASK;
					S_mcuCols = ((inImageCols + DCTSIZE * maxBlkColsPerMcu -1) / (DCTSIZE * maxBlkColsPerMcu)) & MCU_COLS_MASK;
				}
				break;
			default:
				break;
			}
		}
	}
}

#endif
