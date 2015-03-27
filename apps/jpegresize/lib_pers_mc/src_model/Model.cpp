/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "Ht.h"
using namespace Ht;

#ifdef HT_MODEL

#include "JobInfo.h"
#include "JpegDecode.h"
#include "VertResize.h"
#include "HorzResize.h"
#include "JpegEncode.h"
#include "JpegDecMsg.h"
#include "HorzResizeMsg.h"
#include "VertResizeMsg.h"

#include "JpegDecodeDV.h"
#include "HorzResizeDV.h"
#include "VertResizeDV.h"
#include "JpegEncodeDV.h"

void CHtModelAuUnit::UnitThread() {

	do {
		uint8_t jobId;
		uint64_t pVoid;
		uint8_t persMode;
		if (RecvCall_htmain(jobId, pVoid, persMode)) {
			JobInfo * pJobInfo = (JobInfo *) pVoid;

			// decode input image
			jpegDecodeDV( pJobInfo );

			// horizontal resize image
			horzResizeDV( pJobInfo );

			// vertical resize image
			vertResizeDV( pJobInfo );

			// jpeg encode output image
			jpegEncodeDV( pJobInfo );

			while (!SendReturn_htmain(jobId));
		}
	} while (!RecvHostHalt());
}

#if defined(OLD_HT_MODEL)

void HtCoprocCallback(CHtModelAuUnit *, CHtModelAuUnit::ERecvType);

void * CoprocUnitThread(void *);
uint32_t volatile coprocThreadActiveCnt = 0;

void vertResizeDV( JobInfo * pJobInfo, queue<HorzResizeMsg> &horzResizeMsgQue, queue<VertResizeMsg> &vertResizeMsgQue )
{
	vertResizeDV( pJobInfo );
}

void HtCoprocModel()
{
	CHtModelHif *pModelHif = new CHtModelHif;
	int unitCnt = pModelHif->GetAuUnitCnt();
	assert(unitCnt == 1);
	CHtModelAuUnit *pUnit = pModelHif->AllocAuUnit();

	queue<JpegDecMsg> jpegDecMsgQue;
	queue<HorzResizeMsg> horzResizeMsgQue;
	queue<VertResizeMsg> vertResizeMsgQue;

	for (;;) {
		uint8_t jobId;
		uint64_t pVoid;
		uint8_t persMode;

		while (!pUnit->RecvCall_htmain(jobId, pVoid, persMode)) {
			if (pUnit->RecvHostHalt()) {
				delete pModelHif;
				return;
			}
			usleep(1);
		}

		JobInfo * pJobInfo = (JobInfo *) pVoid;

/*
		// decode input image
		jpegDecode( pJobInfo, jpegDecMsgQue );

		// horizontal resize image
		horzResize( pJobInfo, jpegDecMsgQue, horzResizeMsgQue );

		// vertical resize image
		vertResize( pJobInfo, horzResizeMsgQue, vertResizeMsgQue );

		// jpeg encode output image
		jpegEncode( pJobInfo, vertResizeMsgQue );
*/

		// decode input image
		jpegDecode_model( pJobInfo );

		// horizontal resize image
		horzResizeDV( pJobInfo );

		// vertical resize image
		vertResizeDV( pJobInfo );

		// jpeg encode output image
		jpegEncodeDV( pJobInfo );

		pUnit->SendReturn_htmain(jobId);
	}

	delete pModelHif;
}
#endif
#endif
