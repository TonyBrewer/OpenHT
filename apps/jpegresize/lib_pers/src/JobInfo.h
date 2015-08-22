/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#pragma once

#include <stdint.h>

#ifndef _MSC_VER
#include <unistd.h>
#endif

#include "Ht.h"
#include "JpegCommon.h"
//#include "PersConfig.h"

#if defined(_MSC_VER)
#define _ALIGNED(x) __declspec(align(x))
#elif defined(__GNUC__)
#define _ALIGNED(x) __attribute__ ((aligned(x)))
#endif

struct JpegHdr;

typedef int16_t PntWghtInt_t;

//typedef float PntWght_t;

// information passed to coprocessor to process an image scaling operation

#define MAX_APP_MARKER_CNT 8
struct JobApp {
	uint32_t m_appCnt;
	uint32_t m_pad[5];
	uint8_t m_appMarker[8];
	uint32_t m_appLength[8];
	uint8_t * m_pAppBase[8];
};

#define MAX_COM_MARKER_CNT 1
struct JobCom {
	uint32_t m_comCnt;
	uint8_t m_comMarker;
	uint32_t m_comLength;
	uint8_t * m_pComBase;
};

// Decode huffman tables, one element per table per quadword
struct JobDhtTbls {
	uint64_t m_huffCode : 8;
	uint64_t m_lookup : 18;
	int64_t m_maxCode : 18;
	int64_t m_valOffset : 20;	// only least significant 8 bits are used
};

// Decode huffman info
struct _ALIGNED(64) JobDht {
	bool m_bUsed;
	uint8_t m_bits[16];
	JobDhtTbls _ALIGNED(64) m_dhtTbls[256];
};

struct _ALIGNED(64) JobDqt {
	int16_t m_quantTbl[8][8];
};

// Decode component parameters
struct JobDcp {
	uint64_t m_compId : 8; // component ID char

	uint64_t m_blkRowsPerMcu : 2;		// number of blocks in mcu's row
	uint64_t m_blkColsPerMcu : 2;		// 1 or 2

	// dc/ac/qt table indexs
	uint64_t m_dcDhtId : 1;
	uint64_t m_acDhtId : 1;
	uint64_t m_dqtId : 2;

	uint64_t m_compRows : 14;	// imageRows bumped to full MCU rows
	uint64_t m_compCols : 14;	// imageCols bumped to full MCU columns
	uint64_t m_compBufColLines : 8; // compCols bumped to next 64B increment
	uint64_t m_compBufRowBlks : 11; // compCols bumped to next MCU increment

	uint8_t * m_pCompBuf;
}; 

union JobRstInfo {
    struct _ALIGNED(8) {
	uint64_t m_mcuRow : 11;
	uint64_t m_mcuCol : 11;
	uint64_t m_offset : 26;	// how big does it need to be?
	uint64_t m_firstRstMcuCnt : 11;
	uint64_t m_pad : 5;
    };
  uint64_t m_u64;
};

struct _ALIGNED(64) JobDec {
	uint64_t m_compCnt : 2;

	// image size in pixels
	uint64_t m_imageRows : 14;
	uint64_t m_imageCols : 14;

	// image maximum vertical and horizontal sampling factors
	uint64_t m_maxBlkColsPerMcu : 2;
	uint64_t m_maxBlkRowsPerMcu : 2;

	// image size in MCUs
	uint64_t m_mcuRows : 11;
	uint64_t m_mcuCols : 11;

	uint64_t m_dqtValid : 4;

	// restart parameters
	//   we handle either one restart for entire image, or a separate restart per row of MCUs
	uint64_t m_rstMcuCnt : 11;	// number of Mcu's to process per restart, 0 => all mcus
	uint64_t m_rstMcuCntOrig : 5;  // number of Mcu's per restart in original data
	uint64_t m_rstCnt : 11;		// number of restart file positions specified in header
	uint64_t m_pad2 : 5;
	uint64_t m_rstBlkCnt : 22;	// number of blocks in a restart

	uint8_t * m_pRstBase;
	JobRstInfo m_rstInfo[MAX_RST_LIST_SIZE];

	// huffman decode info
	JobDht m_dcDht[2];
	JobDht m_acDht[2];
	JobDqt m_dqt[4];

	// Decode component parameters (64B total)
	JobDcp m_dcp[MAX_MCU_COMPONENTS];
};

struct JobPntWght {
	PntWghtInt_t m_w[COPROC_PNT_WGHT_CNT];
};

struct JobHcp {
	uint64_t m_blkRowsPerMcu : 2;	// 1 or 2
	uint64_t m_blkColsPerMcu : 2;

	uint64_t m_inCompRows : 14;		// imageRows / m_blkRowsPerMcu
	uint64_t m_inCompCols : 14;		// imageCols / m_blkColsPerMcu
	uint64_t m_inCompBufRows : 14;	// compCols bumped to next MCU increment
	uint64_t m_inCompBufCols : 14;	// compCols bumped to next 64B increment

	uint8_t * m_pInCompBuf;	// input component buffer

	uint64_t m_outCompRows : 14;	// imageRows / m_blkRowsPerMcu
	uint64_t m_outCompCols : 14;	// imageCols / m_blkColsPerMcu
	uint64_t m_outCompBlkRows : 11;
	uint64_t m_outCompBlkCols : 11; // memory lines per row of MCU blocks

	uint8_t * m_pOutCompBuf;		// vertically scaled output buffer
};

union JobPntInfo {
	struct {
		int16_t m_pntWghtStart;	// input image starting row for point weights
		int16_t m_pntWghtIdx;	// index into m_pntWghtList for output row
		uint64_t m_pntWghtOut:2;  // output enable indexed by inCol
		uint64_t m_pntWghtOutUp:2;  // output enable indexed by inCol for upScale
		uint64_t m_pntWghtStartBit:2;  // output enable indexed by inCol for upScale
		uint64_t m_pntWghtUpStartBit:1;  // output enable indexed by inCol for upScale
	};
	uint64_t m_u64;
};

struct _ALIGNED(64) JobHorz {

	// qwIdx 0
	uint64_t m_compCnt : 2;

	// image size in pixels
	uint64_t m_inImageRows : 14;
	uint64_t m_inImageCols : 14;
	uint64_t m_outImageRows : 14;
	uint64_t m_outImageCols : 14;

	// image maximum vertical and horizontal sampling factors
	uint64_t m_maxBlkColsPerMcu : 2;
	uint64_t m_maxBlkRowsPerMcu : 2;

	// qwIdx 1
	uint64_t _ALIGNED(8) m_mcuRows : 11;
	uint64_t m_mcuCols : 11;
	uint64_t m_mcuBlkRowCnt : 3;
	uint64_t m_mcuRowRstInc : 4;

	// qwIdx 2 - 17
	JobHcp m_hcp[MAX_MCU_COMPONENTS];

	// qwIdx 18
	uint16_t m_filterWidth;
	uint16_t m_pntWghtListSize;

	// qwIdx 24
	JobPntInfo _ALIGNED(64) m_pntInfo[COPROC_MAX_IMAGE_PNTS];

	// qwIdx 24+8192
	JobPntWght _ALIGNED(64) m_pntWghtList[COPROC_MAX_IMAGE_PNTS];// weights to be used for an output point
};

struct JobVcp {
	uint64_t m_blkRowsPerMcu : 2;	// 1 or 2
	uint64_t m_blkColsPerMcu : 2;

	uint64_t m_inCompRows : 14;		// imageRows / m_blkRowsPerMcu
	uint64_t m_inCompCols : 14;		// imageCols / m_blkColsPerMcu
	uint64_t m_inCompBlkRows : 11;
	uint64_t m_inCompBlkCols : 11;	// compCols bumped to next 64B imcrement

	uint8_t * m_pInCompBuf;	// input component buffer

	uint64_t m_outCompRows : 14;	// imageRows / m_blkRowsPerMcu
	uint64_t m_outCompCols : 14;	// imageCols / m_blkColsPerMcu
	uint64_t m_outCompBufRows : 14; // compRows bumped to next MCU increment
	uint64_t m_outCompBufCols : 14; // compCols bumped to next 64B increment

	uint8_t * m_pOutCompBuf;		// vertically scaled output buffer
};

struct _ALIGNED(64) JobVert {

	// qwIdx 0
	uint64_t m_compCnt : 2;

	// image size in pixels
	uint64_t m_inImageRows : 14;
	uint64_t m_inImageCols : 14;
	uint64_t m_outImageRows : 14;
	uint64_t m_outImageCols : 14;

	// image maximum vertical and horizontal sampling factors
	uint64_t m_maxBlkRowsPerMcu : 2;
	uint64_t m_maxBlkColsPerMcu : 2;

	// qwIdx 1
	uint64_t _ALIGNED(8) m_inMcuRows : 11;
	uint64_t m_inMcuCols : 11;
	uint64_t m_outMcuRows : 11;
	uint64_t m_outMcuCols : 11;

	// qwIdx 2 - 17
	JobVcp _ALIGNED(8) m_vcp[MAX_MCU_COMPONENTS];

	// qwIdx 18
	uint16_t m_filterWidth;
	uint16_t m_pntWghtListSize;

	// qwIdx 24
	JobPntInfo _ALIGNED(64) m_pntInfo[COPROC_MAX_IMAGE_PNTS];

	// qwIdx 24+4096
	JobPntWght _ALIGNED(64) m_pntWghtList[COPROC_MAX_IMAGE_PNTS];// weights to be used for an output point
};

// Decode huffman info
struct _ALIGNED(64) JobEncDht {
	uint16_t _ALIGNED(8) m_huffCode[256];
	uint8_t m_huffSize[256];
};

struct JobEcp {
	uint64_t m_compId : 8; // component ID char

	uint64_t m_blkRowsPerMcu : 2;		// aka horzSampFactor
	uint64_t m_blkColsPerMcu : 2;		// 1 or 2
	uint64_t m_blkRowsInLastMcuRow : 2;	// number of blk rows in last mcu row
	uint64_t m_blkColsInLastMcuCol : 2;

	uint64_t m_compRows : 14;		// imageRows / m_blkRowsPerMcu
	uint64_t m_compCols : 14;		// imageCols / m_blkColsPerMcu
	uint64_t m_compBufCols : 14;	// compCols bumped to next 64B imcrement

	// dc/ac/qt table indexs
	uint64_t m_dcDhtId : 1;
	uint64_t m_acDhtId : 1;
	uint64_t m_dqtId : 2;

	uint8_t * m_pCompBuf;	// input component buffer
};

struct JobEnc {
	uint64_t m_compCnt : 2;

	// image size in pixels
	uint64_t m_imageRows : 14;
	uint64_t m_imageCols : 14;

	// image maximum vertical and horizontal sampling factors
	uint64_t m_maxBlkColsPerMcu : 2;
	uint64_t m_maxBlkRowsPerMcu : 2;

	// image size in MCUs
	uint64_t m_mcuRows : 11;
	uint64_t m_mcuCols : 11;

	// huffman encode info
	JobEncDht m_dcDht[2];	// 64B aligned, 96QW per dht table
	JobEncDht m_acDht[2];
	JobDqt m_dqt[4];		// 16 QW per dqt

	// Encode component parameters (64B total)
	JobEcp m_ecp[MAX_MCU_COMPONENTS];

	// restart parameters
	uint64_t m_rstCnt : 11;			// number of restarts
	uint64_t m_mcuRowsPerRst : 4;	// number of Mcu's rows to process per restart
	uint8_t * m_pRstBase;
	uint32_t m_rstOffset;		// offset in m_pRstBase from one rst to the next
	uint32_t m_rstLength[MAX_ENC_RST_CNT];
};

struct JobInfo {
public:
	void* operator new( size_t, int threadId, bool check=0, bool useHostMem=0, bool preAllocSrcMem=0);
	void operator delete(void*);

	static void SetThreadCnt( int threadCnt ) { m_threadCnt = threadCnt; }

	static void SetHifMemBase( uint8_t * pHifMemBase ) {
		m_pHifInPicBase = pHifMemBase;
		m_pHifRstBase = pHifMemBase + (long long)m_threadCnt * MAX_JPEG_IMAGE_SIZE;
		m_pHifJobInfoBase = pHifMemBase + (long long)m_threadCnt * 3 * MAX_JPEG_IMAGE_SIZE;
	}

	static void Lock() {
		while (__sync_fetch_and_or(&m_lock, 1) == 1)
			usleep(1);
	}

	static void Unlock() {
		__sync_synchronize();
		m_lock = 0;
	}

public:
	JobApp m_app;			// APP marker info
	JobDec m_dec;			// jpeg decode info
	JobHorz m_horz;			// horizontal scaling info
	JobVert m_vert;			// vertical scaling info
	JobEnc m_enc;			// jpeg encode info
	JobCom m_com;			// COM marker info

	uint8_t * m_pInPic;		// allocated memory for input pic
	uint32_t m_inPicSize;

	// not used by coproc below this line
	JobInfo * m_pPoolNext;
	bool m_bFirstUse;
	int m_threadId;

private:
	static JobInfo * m_pPoolHead;
	static volatile int64_t m_lock;
	static uint8_t * m_pHifInPicBase;
	static uint8_t * m_pHifRstBase;
	static uint8_t * m_pHifJobInfoBase;
	static unsigned int m_threadIdx;
	static unsigned int m_threadCnt;
	static bool * m_pThreadJobInfo;
	static uint8_t * m_pThreadLog;
	static int m_threadLogWrIdx;
};

void jobInfoZero( JobInfo * pJobInfo );
void jobInfoInit( JobInfo * pJobInfo, int height, int width, int scale, bool check=0);
void jobInfoFree( JobInfo * pJobInfo );
char const * jobInfoCheck( JobInfo * pJobInfo, int x, int y, int scale );