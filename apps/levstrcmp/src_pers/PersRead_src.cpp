/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT levstrcmp application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
/*
**  (conBase, blkSize, ramIdx, sideIdx)
**
**  for (int blkIdx = ramIdx; blkIdx < blkSize; blkIdx += LINE_STR_CNT)
**		MifReq(conBase + 16 * blkIdx);
**
**  Wait for all reads
**
**  return
*/

#include "Ht.h"
#include "PersRead.h"

void
CPersRead::PersRead()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case READ_ENTRY:
			// entry instruction initializes look indexes to zero
			{
				if (SendReturnBusy_Read()) {
					HtRetry();
					break;
				}

				P_blkIdx = (BlkSize_t)P_lineStrIdx;

				if (P_blkSize <= P_lineStrIdx)
					SendReturn_Read();
				else {
					HtContinue(READ_BLK_LOOP);
				}
				break;
			}
		case READ_BLK_LOOP:
			// loop that reads left block
			{
				if (ReadMemBusy()) {
					HtRetry();
					break;
				}

				MemAddr_t memAddr = P_blkBase + SIZE_OF_STRING * P_blkIdx;
				RamSize_t ramAddr = (RamSize_t)((P_blkIdx >> LINE_STR_CNT_L_W) + P_ramBlkIdx * RAM_BLOCK_SIZE);

				if (P_sideIdx == 0)
					ReadMem_strSetL(memAddr, ramAddr, (LineStrIdxL_t)P_lineStrIdx, 1);
				else
					ReadMem_strSetR(memAddr, ramAddr, (LineStrIdxR_t)P_lineStrIdx, 1);

				P_blkIdx += LINE_STR_CNT_L;
				bool bLastReq = P_blkIdx >= P_blkSize;

				if (bLastReq)
					ReadMemPause(READ_RTN);
				else
					HtContinue(READ_BLK_LOOP);

				break;
			}
		case READ_RTN:
			{
				if (SendReturnBusy_Read()) {
					HtRetry();
					break;
				}

				SendReturn_Read();

				break;
			}
		default:
			assert(0);
		}
	}

	if (GR_htReset)
		S_threadActiveCnt = 0;
}
