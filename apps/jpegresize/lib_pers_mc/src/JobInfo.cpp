/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include <stdlib.h>
#include <math.h>

#include "Ht.h"
using namespace Ht;
#include "JpegCommon.h"
#include "JobInfo.h"
#include "JpegHuffman.h"
#include "JpegResizeHif.h"





void* JobInfo::operator new( size_t, int threadId, JpegResizeHif * resizeHif, bool check, bool useHostMem, bool preAllocSrcMem)
{
  //m_pJpegResizeHif=resizeHif;
  JobInfo * pJobInfo = resizeHif->getJobInfoSlot(threadId, resizeHif, check, useHostMem, preAllocSrcMem);
  pJobInfo->m_threadId = threadId;
  return pJobInfo;
}

void JobInfo::operator delete(void * pVoid)
{
  JobInfo * pJobInfo = (JobInfo *)pVoid;
  pJobInfo->m_pJpegResizeHif->releaseJobInfoSlot(pJobInfo);
}


inline int64_t jdivRoundUp (int64_t a, int64_t b)
{
  return (a + b - 1) / b;
}

#define PNT_WGHT_HASH_W 8
class HashPntWght {
  // quick lookup structure to find matching pntWght in existing list
  struct Entry {
    int16_t m_pntWghtIdx;
    int16_t m_nextEntry;
  };
  
public:
  HashPntWght(JobPntWght (&pntWghtList)[COPROC_MAX_IMAGE_PNTS], uint16_t &pntWghtListSize)
  : m_pntWghtList(pntWghtList), m_pntWghtListSize(pntWghtListSize)
  {
    m_pntWghtListSize = 0;
    m_freeEntryPos = 1 << PNT_WGHT_HASH_W;
    memset(m_entries, 0xff, sizeof(Entry) * (1<<PNT_WGHT_HASH_W));
  }
  int16_t findPntWghtInList(JobPntWght &pntWght)
  {
    int16_t entryIdx = calcHash(pntWght);
    for (;;) {
      if (m_entries[entryIdx].m_pntWghtIdx == -1)
	// first entry in hash list is invalid
	break;
      
      if (memcmp(&m_pntWghtList[m_entries[entryIdx].m_pntWghtIdx], &pntWght, sizeof(JobPntWght)) == 0)
	// hit
	return m_entries[entryIdx].m_pntWghtIdx;
      
      if (m_entries[entryIdx].m_nextEntry == -1) {
	// end of list, add entry
	entryIdx = m_entries[entryIdx].m_nextEntry = m_freeEntryPos++;
	break;
      }
      
      entryIdx = m_entries[entryIdx].m_nextEntry;
    }
    
    memcpy(&m_pntWghtList[m_pntWghtListSize], &pntWght, sizeof(JobPntWght));
    m_entries[entryIdx].m_nextEntry = -1;
    return m_entries[entryIdx].m_pntWghtIdx = m_pntWghtListSize++;
  }
  
private:
  int16_t calcHash(JobPntWght const &pntWght)
  {
    int16_t hash = 0;
    for (int i = 0; i < COPROC_PNT_WGHT_CNT; i += 1) {
      hash = (hash << 1) + pntWght.m_w[i];
      hash = ((hash >> 8) + hash) & ((1<<PNT_WGHT_HASH_W)-1);
    }
    return hash;
  }
  
private:
  JobPntWght (&m_pntWghtList)[COPROC_MAX_IMAGE_PNTS];
  uint16_t &m_pntWghtListSize;
  int m_freeEntryPos;
  Entry m_entries[COPROC_MAX_IMAGE_PNTS+(1<<PNT_WGHT_HASH_W)];
};

void calcDqtRecipTbl( JobDqt const & inDqt, JobDqt & outRecipDqt );
void calcPointWeights( uint16_t origRows, uint16_t scaledPnts, uint16_t blksPerMcu,
		       uint16_t &filterWidth,
		       JobPntInfo (&pntInfo)[COPROC_MAX_IMAGE_PNTS],
		       uint16_t &pntWghtListSize, JobPntWght (&pntWghtList)[COPROC_MAX_IMAGE_PNTS] );
double getResizeFilterWeight( double x );
double calcSinc( double x );
int fixPnt(double x, int fracW);
void countUniquePntWghts(int scaleRows, JobPntWght (&pntWghtList)[COPROC_MAX_IMAGE_PNTS]);

// initialize jobInfo for new job
void jobInfoZero( JobInfo * pJobInfo )
{
  pJobInfo->m_app.m_appCnt = 0;
  pJobInfo->m_com.m_comCnt = 0;
  pJobInfo->m_dec.m_dqtValid = 0;
  pJobInfo->m_dec.m_rstCnt = 0;
  pJobInfo->m_dec.m_rstMcuCnt = 0;
}

void jobInfoFree( JobInfo * pJobInfo )
{
  //JobDec & dec = pJobInfo->m_dec;
  //for (int ci = 0; ci < dec.m_compCnt; ci += 1) {
  //	if (dec.m_dcp[ci].m_pCompBuf)
  //		ht_cp_free_memalign( dec.m_dcp[ci].m_pCompBuf );
  //}
  
  //JobHorz & horz = pJobInfo->m_horz;
  //for (int ci = 0; ci < horz.m_compCnt; ci += 1) {
  //	if (horz.m_hcp[ci].m_pOutCompBuf)
  //		ht_cp_free_memalign( horz.m_hcp[ci].m_pOutCompBuf );
  //}
  
  //JobVert & vert = pJobInfo->m_vert;
  //for (int ci = 0; ci < vert.m_compCnt; ci += 1) {
  //	if (vert.m_vcp[ci].m_pOutCompBuf)
  //		ht_cp_free_memalign( vert.m_vcp[ci].m_pOutCompBuf );
  //}
  
  //JobEnc & enc = pJobInfo->m_enc;
  //if (enc.m_pRstBase)
  //	ht_free_memalign( enc.m_pRstBase );
}

void jobInfoInit( JobInfo * pJobInfo, int height, int width, int scale, bool check)
{
  {
    JobDec & dec = pJobInfo->m_dec;
    
    dec.m_maxBlkColsPerMcu = 0;
    dec.m_maxBlkRowsPerMcu = 0;
    int ci=0;
    for (; ci < dec.m_compCnt; ci += 1) {
      dec.m_maxBlkColsPerMcu = MAX( dec.m_maxBlkColsPerMcu, dec.m_dcp[ci].m_blkColsPerMcu );
      dec.m_maxBlkRowsPerMcu = MAX( dec.m_maxBlkRowsPerMcu, dec.m_dcp[ci].m_blkRowsPerMcu );
    }
    for (; ci < 3; ci += 1) {
      dec.m_dcp[ci].m_blkColsPerMcu = 0;
      dec.m_dcp[ci].m_blkRowsPerMcu = 0;
    }
    
    dec.m_mcuRows = jdivRoundUp( dec.m_imageRows, dec.m_maxBlkRowsPerMcu * DCTSIZE );
    dec.m_mcuCols = jdivRoundUp( dec.m_imageCols, dec.m_maxBlkColsPerMcu * DCTSIZE );
    
    // Determine last row/col size for sample factors
    dec.m_rstBlkCnt = 0;
    ci = 0;
    for (; ci < dec.m_compCnt; ci += 1) {
      //dec.m_dcp[ci].m_blkRowsInLastMcuRow = ((dec.m_imageRows - 1) / DCTSIZE) % dec.m_dcp[ci].m_blkRowsPerMcu + 1;
      //dec.m_dcp[ci].m_blkColsInLastMcuCol = ((dec.m_imageCols - 1) / DCTSIZE) % dec.m_dcp[ci].m_blkColsPerMcu + 1;
      
      dec.m_rstBlkCnt += dec.m_dcp[ci].m_blkRowsPerMcu * dec.m_dcp[ci].m_blkColsPerMcu;
    }
    
    if (dec.m_rstMcuCnt == 0)
      dec.m_rstBlkCnt *= dec.m_mcuRows * dec.m_mcuCols;
    else
      dec.m_rstBlkCnt *= dec.m_mcuCols;
    
    // set restart row/col info
    dec.m_rstInfo[0].m_mcuRow = 0;
    dec.m_rstInfo[0].m_mcuCol = 0;
    
    for (int ri = 1; ri < dec.m_rstCnt; ri += 1) {
      // increment row/col by restart interval
      dec.m_rstInfo[ri].m_mcuRow = dec.m_rstInfo[ri-1].m_mcuRow;
      dec.m_rstInfo[ri].m_mcuCol = dec.m_rstInfo[ri-1].m_mcuCol + dec.m_rstMcuCnt;
      while (dec.m_rstInfo[ri].m_mcuCol >= dec.m_mcuCols) {
	dec.m_rstInfo[ri].m_mcuCol -= dec.m_mcuCols;
	dec.m_rstInfo[ri].m_mcuRow += 1;
      }
    }
    
    // calculate derived huffman table info
    //calcDhtDerivedTbl( dec.m_dcDht[0] );
    //calcDhtDerivedTbl( dec.m_dcDht[1] );
    //calcDhtDerivedTbl( dec.m_acDht[0] );
    //calcDhtDerivedTbl( dec.m_acDht[1] );
    
    // Allocate space for decoded input image
    ci = 0;
    for (; ci < dec.m_compCnt; ci += 1) {
      bool bRowUpscale = dec.m_maxBlkRowsPerMcu == 2 && dec.m_dcp[ci].m_blkRowsPerMcu == 1;
      bool bColUpscale = dec.m_maxBlkColsPerMcu == 2 && dec.m_dcp[ci].m_blkColsPerMcu == 1;
      
      dec.m_dcp[ci].m_compRows = bRowUpscale ? (dec.m_imageRows + 1)/2 : dec.m_imageRows;
      dec.m_dcp[ci].m_compCols = bColUpscale ? (dec.m_imageCols + 1)/2 : dec.m_imageCols;
      
      int64_t bufRows = (dec.m_dcp[ci].m_compRows + (dec.m_dcp[ci].m_blkRowsPerMcu * DCTSIZE - 1)) & ~(dec.m_dcp[ci].m_blkRowsPerMcu * DCTSIZE - 1);
      int64_t bufCols = (dec.m_dcp[ci].m_compCols + MEM_LINE_SIZE-1) & ~(MEM_LINE_SIZE-1);
      
      dec.m_dcp[ci].m_compBufColLines = bufCols >> 6;		assert((bufCols & 0x3f) == 0);
      dec.m_dcp[ci].m_compBufRowBlks = bufRows >> 3;		assert((bufRows & 7) == 0);
      
      // memory is allocated my JobInfo allocator for max size image
      assert(bufRows * bufCols <= MAX_IMAGE_COMP_SIZE);
    }
    for (; ci < 3; ci += 1) {
      
      dec.m_dcp[ci].m_compRows = 0;
      dec.m_dcp[ci].m_compCols = 0;
      
      dec.m_dcp[ci].m_compBufColLines = 0;
      dec.m_dcp[ci].m_compBufRowBlks = 0;
      
    }
  }
  
  /////////////////////////////////////////
  // Horizontal scaling info
  
  {
    JobDec & dec = pJobInfo->m_dec;
    JobHorz & horz = pJobInfo->m_horz;
    
    horz.m_compCnt = dec.m_compCnt;
    horz.m_pntWghtListSize = 0;
    
    horz.m_maxBlkRowsPerMcu = dec.m_maxBlkRowsPerMcu;
    horz.m_maxBlkColsPerMcu = dec.m_maxBlkColsPerMcu;
    
    horz.m_inImageRows = dec.m_imageRows;
    horz.m_inImageCols = dec.m_imageCols;
    
    horz.m_outImageRows = horz.m_inImageRows;
    
    horz.m_outImageCols = width > 0 ? width : (int)floor(scale/100.0*horz.m_inImageCols+0.5);
    
    if (horz.m_outImageCols < DCTSIZE * horz.m_maxBlkColsPerMcu) horz.m_outImageCols = DCTSIZE * horz.m_maxBlkColsPerMcu;
    
    horz.m_mcuRows = dec.m_mcuRows;
    horz.m_mcuCols = dec.m_mcuCols;
    
    horz.m_mcuBlkRowCnt = 0;
    horz.m_mcuRowRstInc = dec.m_rstCnt == 1 ? 1 : 8;
    
    for (int ci = 0; ci < 3; ci += 1) {
      // upscale factors
      horz.m_hcp[ci].m_blkRowsPerMcu = dec.m_dcp[ci].m_blkRowsPerMcu;
      horz.m_hcp[ci].m_blkColsPerMcu = dec.m_dcp[ci].m_blkColsPerMcu;
      
      horz.m_mcuBlkRowCnt += horz.m_hcp[ci].m_blkRowsPerMcu;
      
      // input component buffer
      horz.m_hcp[ci].m_inCompRows = dec.m_dcp[ci].m_compRows;
      horz.m_hcp[ci].m_inCompCols = dec.m_dcp[ci].m_compCols;
      horz.m_hcp[ci].m_inCompBufRows = dec.m_dcp[ci].m_compBufRowBlks * DCTSIZE;
      horz.m_hcp[ci].m_inCompBufCols = dec.m_dcp[ci].m_compBufColLines * MEM_LINE_SIZE;
      horz.m_hcp[ci].m_pInCompBuf = dec.m_dcp[ci].m_pCompBuf;
      
      // output component buffer
      bool bRowUpscale = horz.m_maxBlkRowsPerMcu == 2 && horz.m_hcp[ci].m_blkRowsPerMcu == 1;
      bool bColUpscale = horz.m_maxBlkColsPerMcu == 2 && horz.m_hcp[ci].m_blkColsPerMcu == 1;
      
      horz.m_hcp[ci].m_outCompRows = bRowUpscale ? (horz.m_outImageRows + 1)/2 : horz.m_outImageRows;
      horz.m_hcp[ci].m_outCompCols = bColUpscale ? (horz.m_outImageCols + 1)/2 : horz.m_outImageCols;
      
      uint64_t bufRows = horz.m_hcp[ci].m_outCompBlkRows = horz.m_mcuRows * horz.m_hcp[ci].m_blkRowsPerMcu;			
      
      uint64_t colsPerMcu = DCTSIZE  * horz.m_hcp[ci].m_blkColsPerMcu;
      if (ci < horz.m_compCnt)
	horz.m_hcp[ci].m_outCompBlkCols = (horz.m_hcp[ci].m_outCompCols + colsPerMcu-1) / colsPerMcu * horz.m_hcp[ci].m_blkColsPerMcu;
      else
	horz.m_hcp[ci].m_outCompBlkCols = 0;
      int64_t bufCols = horz.m_hcp[ci].m_outCompBlkCols * MEM_LINE_SIZE;
      
      // memory is allocated my JobInfo allocator for max size image
      assert(bufRows * bufCols <= MAX_IMAGE_COMP_SIZE);
    }
    
    // create vertical filter weights for each output row
    calcPointWeights( horz.m_inImageCols, horz.m_outImageCols, horz.m_maxBlkColsPerMcu,
		      horz.m_filterWidth, horz.m_pntInfo,
		      horz.m_pntWghtListSize, horz.m_pntWghtList );
    
    if (horz.m_maxBlkColsPerMcu == 2)
      horz.m_filterWidth |= 1;
    
    //printf( "Horz weight sets    : %d\n", horz.m_pntWghtListSize);
  }
  
  /////////////////////////////////////////
  // Vertical scaling info
  {
    JobHorz & horz = pJobInfo->m_horz;
    JobVert & vert = pJobInfo->m_vert;
    
    vert.m_compCnt = horz.m_compCnt;
    vert.m_pntWghtListSize = 0;
    
    vert.m_maxBlkRowsPerMcu = horz.m_maxBlkRowsPerMcu;
    vert.m_maxBlkColsPerMcu = horz.m_maxBlkColsPerMcu;
    
    vert.m_inImageRows = horz.m_outImageRows;
    vert.m_inImageCols = horz.m_outImageCols;
    
    vert.m_outImageRows = height > 0 ? height : (int)floor(scale/100.0*vert.m_inImageRows+0.5);
    vert.m_outImageCols = vert.m_inImageCols;
    
    if (vert.m_outImageRows < DCTSIZE * vert.m_maxBlkRowsPerMcu) vert.m_outImageRows = DCTSIZE * vert.m_maxBlkRowsPerMcu;
    
    vert.m_inMcuRows = (vert.m_inImageRows + vert.m_maxBlkRowsPerMcu * DCTSIZE-1) / (vert.m_maxBlkRowsPerMcu * DCTSIZE);
    vert.m_inMcuCols = (vert.m_inImageCols + vert.m_maxBlkColsPerMcu * DCTSIZE-1) / (vert.m_maxBlkColsPerMcu * DCTSIZE);
    
    vert.m_outMcuRows = (vert.m_outImageRows + vert.m_maxBlkRowsPerMcu * DCTSIZE-1) / (vert.m_maxBlkRowsPerMcu * DCTSIZE);
    vert.m_outMcuCols = (vert.m_outImageCols + vert.m_maxBlkColsPerMcu * DCTSIZE-1) / (vert.m_maxBlkColsPerMcu * DCTSIZE);
    
    for (int ci = 0; ci < 3; ci += 1) {
      // upscale factors
      vert.m_vcp[ci].m_blkRowsPerMcu = horz.m_hcp[ci].m_blkRowsPerMcu;
      vert.m_vcp[ci].m_blkColsPerMcu = horz.m_hcp[ci].m_blkColsPerMcu;
      
      // input component buffer
      vert.m_vcp[ci].m_inCompRows = horz.m_hcp[ci].m_outCompRows;
      vert.m_vcp[ci].m_inCompCols = horz.m_hcp[ci].m_outCompCols;
      vert.m_vcp[ci].m_inCompBlkRows = horz.m_hcp[ci].m_outCompBlkRows;
      vert.m_vcp[ci].m_inCompBlkCols = horz.m_hcp[ci].m_outCompBlkCols;
      vert.m_vcp[ci].m_pInCompBuf = horz.m_hcp[ci].m_pOutCompBuf;
      
      // output component buffer
      bool bRowUpscale = vert.m_maxBlkRowsPerMcu == 2 && vert.m_vcp[ci].m_blkRowsPerMcu == 1;
      bool bColUpscale = vert.m_maxBlkColsPerMcu == 2 && vert.m_vcp[ci].m_blkColsPerMcu == 1;
      
      if (ci < vert.m_compCnt) {
	vert.m_vcp[ci].m_outCompRows = bRowUpscale ? (vert.m_outImageRows + 1)/2 : vert.m_outImageRows;
	vert.m_vcp[ci].m_outCompCols = bColUpscale ? (vert.m_outImageCols + 1)/2 : vert.m_outImageCols;
      } else {
	vert.m_vcp[ci].m_outCompRows = 0;
	vert.m_vcp[ci].m_outCompCols = 0;
      }
      
      int64_t bufRows = (vert.m_vcp[ci].m_outCompRows + vert.m_vcp[ci].m_blkRowsPerMcu * DCTSIZE - 1) & ~(vert.m_vcp[ci].m_blkRowsPerMcu * DCTSIZE - 1);
      int64_t bufCols = (vert.m_vcp[ci].m_outCompCols + MEM_LINE_SIZE-1) & ~(MEM_LINE_SIZE-1);
      
      vert.m_vcp[ci].m_outCompBufRows = bufRows;
      vert.m_vcp[ci].m_outCompBufCols = bufCols;
      
      // memory is allocated my JobInfo allocator for max size image
      assert(bufRows * bufCols <= MAX_IMAGE_COMP_SIZE);
    }
    
    // create vertical filter weights for each output row
    calcPointWeights( vert.m_inImageRows, vert.m_outImageRows, vert.m_maxBlkRowsPerMcu,
		      vert.m_filterWidth, vert.m_pntInfo,
		      vert.m_pntWghtListSize, vert.m_pntWghtList );
    
    if (vert.m_maxBlkRowsPerMcu == 2)
      vert.m_filterWidth |= 1;
    
    //printf( "Vert weight sets    : %d\n", vert.m_pntWghtListSize);
  }
  
  /////////////////////////////////////////
  // jpeg encoding info
  
  {
    JobDec & dec = pJobInfo->m_dec;
    JobVert & vert = pJobInfo->m_vert;
    JobEnc & enc = pJobInfo->m_enc;
    
    enc.m_compCnt = vert.m_compCnt;
    enc.m_imageCols = vert.m_outImageCols;
    enc.m_imageRows = vert.m_outImageRows;
    
    enc.m_maxBlkRowsPerMcu = vert.m_maxBlkRowsPerMcu;
    enc.m_maxBlkColsPerMcu = vert.m_maxBlkColsPerMcu;
    
    enc.m_mcuRows = jdivRoundUp( enc.m_imageRows, enc.m_maxBlkRowsPerMcu * DCTSIZE );
    enc.m_mcuCols = jdivRoundUp( enc.m_imageCols, enc.m_maxBlkColsPerMcu * DCTSIZE );
    
    enc.m_mcuRowsPerRst = (enc.m_mcuRows + MAX_ENC_RST_CNT - 1) / MAX_ENC_RST_CNT;
    enc.m_rstCnt = (enc.m_mcuRows + enc.m_mcuRowsPerRst - 1) / enc.m_mcuRowsPerRst;
    
    int blksPerMcu = 0;
    for (int ci = 0; ci < 3; ci += 1) {
      enc.m_ecp[ci].m_blkRowsPerMcu = dec.m_dcp[ci].m_blkRowsPerMcu;
      enc.m_ecp[ci].m_blkColsPerMcu = dec.m_dcp[ci].m_blkColsPerMcu;
      if (ci < enc.m_compCnt) {
	enc.m_ecp[ci].m_blkRowsInLastMcuRow = ((enc.m_imageRows - 1) / DCTSIZE) % enc.m_ecp[ci].m_blkRowsPerMcu + 1;
	enc.m_ecp[ci].m_blkColsInLastMcuCol = ((enc.m_imageCols - 1) / DCTSIZE) % enc.m_ecp[ci].m_blkColsPerMcu + 1;
      } else {
	enc.m_ecp[ci].m_blkRowsInLastMcuRow = 0;
	enc.m_ecp[ci].m_blkColsInLastMcuCol = 0;
      }
      
      // output component buffer
      enc.m_ecp[ci].m_compRows = vert.m_vcp[ci].m_outCompRows;
      enc.m_ecp[ci].m_compCols = vert.m_vcp[ci].m_outCompCols;
      enc.m_ecp[ci].m_compBufCols = vert.m_vcp[ci].m_outCompBufCols;
      enc.m_ecp[ci].m_pCompBuf = vert.m_vcp[ci].m_pOutCompBuf;
      
      blksPerMcu += (int)(enc.m_ecp[ci].m_blkRowsPerMcu * enc.m_ecp[ci].m_blkColsPerMcu);
      
      enc.m_ecp[ci].m_compId = dec.m_dcp[ci].m_compId;
      enc.m_ecp[ci].m_dcDhtId = dec.m_dcp[ci].m_dcDhtId;
      enc.m_ecp[ci].m_acDhtId = dec.m_dcp[ci].m_acDhtId;
      enc.m_ecp[ci].m_dqtId = dec.m_dcp[ci].m_dqtId;
    }
    
    enc.m_rstOffset = (uint32_t)(enc.m_mcuCols * blksPerMcu * 64);
    //int fail = ht_posix_memalign( (void **)&enc.m_pRstBase, 64, enc.m_mcuRows * enc.m_rstOffset );
    //assert(!fail);
    
    calcDqtRecipTbl( dec.m_dqt[0], enc.m_dqt[0] );
    calcDqtRecipTbl( dec.m_dqt[1], enc.m_dqt[1] );
    calcDqtRecipTbl( dec.m_dqt[2], enc.m_dqt[2] );
    calcDqtRecipTbl( dec.m_dqt[3], enc.m_dqt[3] );
    
    compute_huffman_table(enc.m_dcDht[0].m_huffCode, enc.m_dcDht[0].m_huffSize, s_dc_lum_bits, s_dc_lum_val);
    compute_huffman_table(enc.m_acDht[0].m_huffCode, enc.m_acDht[0].m_huffSize, s_ac_lum_bits, s_ac_lum_val);
    if (pJobInfo->m_enc.m_compCnt > 1)
    {
      compute_huffman_table(enc.m_dcDht[1].m_huffCode, enc.m_dcDht[1].m_huffSize, s_dc_chroma_bits, s_dc_chroma_val);
      compute_huffman_table(enc.m_acDht[1].m_huffCode, enc.m_acDht[1].m_huffSize, s_ac_chroma_bits, s_ac_chroma_val);
    }
    
    if (check) {
      for (int i = 0; i < MAX_ENC_RST_CNT; i += 1)
	enc.m_rstLength[i] = 0xdeadbeaf;
    }
  }
}

void calcDqtRecipTbl( JobDqt const & inDqt, JobDqt & outRecipDqt )
{
  for (uint64_t col = 0; col < DCTSIZE; col += 1) {
    for (uint64_t row = 0; row < DCTSIZE; row += 1) {
      if (inDqt.m_quantTbl[row][col] == 0)
	outRecipDqt.m_quantTbl[row][col] = 0;
      else
	outRecipDqt.m_quantTbl[row][col] = (int16_t)(256.0 / inDqt.m_quantTbl[row][col]);
    }
  }
}

void calcPointWeights( uint16_t origRows, uint16_t scaledPnts, uint16_t blksPerMcu,
		       uint16_t &filterWidth,
		       JobPntInfo (&pntInfo)[COPROC_MAX_IMAGE_PNTS],
		       uint16_t &pntWghtListSize, JobPntWght (&pntWghtList)[COPROC_MAX_IMAGE_PNTS] )
{
  // Calculate the weights to be multiplied to a contiguous list of input points to determine
  // a single output point. The pntWghtStart is the input image row corresponding to the
  // first pntWght, successive pntWghts correspond to successive input image row pixels.
  // Note that pntWghtStart will be negative at the beginning of a row/col. Negative indexes
  // match up with missing pixels leading up to the first row/col pixel. The weights associated
  // with negative start is zero. The pntWghtIdx is a list of indexes into the pntWghtList
  // table to indicate the pntWghts to be used with each specific output image point.
  
  //HashPntWght hashPntWght(pntWghtList, pntWghtListSize);
  int pntWghtIdx[PNT_WGHT_SIZE];
  for (int i=0; i<PNT_WGHT_SIZE; i++) pntWghtIdx[i] = -1;
  
  double scale = (double)scaledPnts / origRows;
  double support = 3.0 / scale;
  if (support > 7.5) support = 7.5;
  filterWidth = (uint8_t)(support * 2.0 + 0.5);
  assert(filterWidth <= COPROC_PNT_WGHT_CNT);
  int filterStart = COPROC_PNT_WGHT_CNT - filterWidth;
  int ptsPerSwath = (int)blksPerMcu * 4 * 8;
  
  double pntWghts[COPROC_MAX_IMAGE_PNTS];
  JobPntWght normPntWght;
  for (int pnt = 0; pnt <= origRows; pnt += 1) {
    pntInfo[pnt].m_u64 = 0ll;
  }
  bool pntWghtOutUpLast = false;
  int ptsPerSwathUp = ptsPerSwath >> 1;
  int pntUp = 0;
  for (int pnt = 0; pnt < scaledPnts; pnt += 1) {
    double bisect = (pnt+0.5)/scale;
    double startRnd = (bisect - support) < 0 ? -0.5 : 0.5;
    int start = (int) (bisect - support + startRnd);
    pntInfo[pnt].m_pntWghtStart |= (start - filterStart - 1); // used by DV code, not HW
    int pntInfoIdx = (start - filterStart - 1) + 17;
    int pntInfoIdxUp = (start - filterStart - 1) + 18;
    if (pntInfoIdx & 0x001)
      pntInfo[pntInfoIdx>>1].m_pntWghtOut |= 0x2;
    else
      pntInfo[pntInfoIdx>>1].m_pntWghtOut |= 0x1;;
    if (!pntWghtOutUpLast) {// in upScale mode we gen every other idx, need to know which idx for bEven
      if (pntInfoIdxUp & 0x001)
	pntInfo[pntInfoIdxUp>>1].m_pntWghtOutUp |= 0x2;
      else
	pntInfo[pntInfoIdxUp>>1].m_pntWghtOutUp |= 0x1;
      pntUp++;
    }
    if ((pnt % ptsPerSwath) == 0) {
      if (pntInfoIdx & 0x001)
	pntInfo[pntInfoIdx>>1].m_pntWghtStartBit |= 0x2;
      else
	pntInfo[pntInfoIdx>>1].m_pntWghtStartBit |= 0x1;
    }
    if ((pnt % ptsPerSwathUp) == 0) {
      pntInfo[pntInfoIdxUp>>1].m_pntWghtUpStartBit = 0x1;
    }
    
    pntWghtOutUpLast = !pntWghtOutUpLast;
    
    int thisIdx;
    if (start < 0)
      thisIdx = start + COPROC_PNT_WGHT_CNT/2;
    else if (start > (origRows - filterWidth)) {
      thisIdx = filterWidth - (origRows - start) + COPROC_PNT_WGHT_CNT/2;
      assert(thisIdx >= COPROC_PNT_WGHT_CNT/2 && thisIdx < COPROC_PNT_WGHT_CNT);
    } else
      thisIdx = (int)((bisect-(int)bisect)*(PNT_WGHT_SIZE-COPROC_PNT_WGHT_CNT)) + COPROC_PNT_WGHT_CNT;
    if (pntWghtIdx[thisIdx] == -1) {
      int stop = (int) (bisect + support + 0.5);
      double density = 0.0;
      for (int n=0; n < COPROC_PNT_WGHT_CNT; n++) {
	int pntRow = start + n;
	if (pntRow < 0 || pntRow >= origRows || n >= (stop-start)) {
	  pntWghts[n] = 0;
	} else {
	  density += pntWghts[n] = getResizeFilterWeight( scale * ((start+n) - bisect + 0.5) );
	  if (pntWghts[n] != 0 && filterWidth < n+1)
	    filterWidth = n+1;
	}
      }
      double densityRecip = 1.0 / density;
      for (int n=0; n < COPROC_PNT_WGHT_CNT; n++) {
	if (n < filterStart)
	  normPntWght.m_w[n] = 0;
	else
	  normPntWght.m_w[n] = fixPnt(pntWghts[n-filterStart] * densityRecip, COPROC_PNT_WGHT_FRAC_W);
      }
      memcpy(&pntWghtList[pntWghtListSize], &normPntWght, sizeof(JobPntWght));
      pntWghtIdx[thisIdx] = pntWghtListSize++;
    }
    
    pntInfo[pnt].m_pntWghtIdx = pntWghtIdx[thisIdx];
  }
  // set these to benign values for horizontal resizing block
  pntInfo[scaledPnts].m_pntWghtStart |= (8192-1) & 0x3fff;	// max column index
  pntInfo[scaledPnts].m_pntWghtIdx = 0;
}

struct RfwCache {
  RfwCache() { m_v = false; }
  double m_x;
  double m_r;
  bool m_v;
};

double getResizeFilterWeight( double x )
{
  // caching code is not thread safe...
  //	static RfwCache cache[256];
  
  //	uint64_t hash64 = *(uint64_t *)&x;
  //	uint8_t hash8 = ((hash64 >> 56) + (hash64 >> 48)) & 0xff;
  
  //	if (cache[hash8].m_x == x && cache[hash8].m_v)
  //		return cache[hash8].m_r;
  
  #define RESIZE_FILTER_BLUR 1.0
  #define RESIZE_FILTER_SCALE (1.0/3.0)
  
  double x_blur=fabs((double) x)/RESIZE_FILTER_BLUR;  /* X offset with blur scaling */
  double scale = calcSinc( x_blur * RESIZE_FILTER_SCALE );
  double weight = scale * calcSinc( x_blur );
  
  //	cache[hash8].m_x = x;
  //	cache[hash8].m_r = weight;
  //	cache[hash8].m_v = true;
  
  return weight;
}

double calcSinc( double x )
{
  // implement a cache of previous values, must have a fast, simple hash
  //static double cacheX[256];
  //static double cacheR[256];
  //static int missCnt = 0;
  
  //uint64_t hash64 = *(uint64_t *)&x;
  //uint8_t hash8 = ((hash64 >> 56) + (hash64 >> 48)) & 0xff;
  
  //if (cacheX[hash8] == x)
  //	return cacheR[hash8];
  
  //missCnt += 1;
  
  assert(x < 4.0);
  double xx = x * x;
  
  /*
   *	Maximum absolute relative error 6.3e-6 < 1/2^17.
   */
  const double c0 = 0.173610016489197553621906385078711564924e-2L;
  const double c1 = -0.384186115075660162081071290162149315834e-3L;
  const double c2 = 0.393684603287860108352720146121813443561e-4L;
  const double c3 = -0.248947210682259168029030370205389323899e-5L;
  const double c4 = 0.107791837839662283066379987646635416692e-6L;
  const double c5 = -0.324874073895735800961260474028013982211e-8L;
  const double c6 = 0.628155216606695311524920882748052490116e-10L;
  const double c7 = -0.586110644039348333520104379959307242711e-12L;
  const double p = c0+xx*(c1+xx*(c2+xx*(c3+xx*(c4+xx*(c5+xx*(c6+xx*c7))))));
  const double r = ((xx-1.0)*(xx-4.0)*(xx-9.0)*(xx-16.0)*p);
  
  //cacheX[hash8] = x;
  //cacheR[hash8] = r;
  
  return r;
}

int fixPnt(double x, int fracW)
{
  double round = x < 0 ? -0.5 : 0.5;
  int fixPnt = (int)(x * (1 << fracW) + round);
  if (fixPnt == (1 << fracW))
    fixPnt = (1 << fracW)-1;
  assert(fixPnt < (1 << (fracW+1))-1 && fixPnt > -(1 << (fracW+1)));
  return fixPnt;
}

void countUniquePntWghts(int pntWghtSize, JobPntWght (&pntWghtList)[COPROC_MAX_IMAGE_PNTS])
{
  // find number of unique wght Lists
  int uniqueList[COPROC_MAX_IMAGE_PNTS];
  int uniqueSize = 0;
  
  for (int i = 0; i < pntWghtSize; i += 1) {
    int j;
    for (j = 0; j < uniqueSize; j += 1) {
      if (memcmp(&pntWghtList[i], &pntWghtList[uniqueList[j]], sizeof(JobPntWght)) == 0)
	break;
    }
    if (j < uniqueSize)
      continue;
    
    uniqueList[j] = i;
    uniqueSize += 1;
  }
}

char const * jobInfoCheck( JobInfo * pJobInfo, int x, int y, int scale )
{
  if ((scale != 0 && scale > 100) ||
    (scale == 0 && (y > pJobInfo->m_dec.m_imageRows ||
    x > pJobInfo->m_dec.m_imageCols)))
    return "scaling up";
  
  if (pJobInfo->m_dec.m_compCnt == 2)
    return "jpeg component count equal 2";
  
  
  // check if scaling too small
  if ((scale != 0 && (((float)scale * (float)pJobInfo->m_dec.m_imageRows / 100.0 < 8.0) ||
    ((float)scale * (float)pJobInfo->m_dec.m_imageCols / 100.0 < 16.0))) ||
    (scale == 0 && y < 8) ||
    (scale == 0 && x < 16)) {
    return "scaling to less than 16x8 pixels";
    }

    return 0;
}
