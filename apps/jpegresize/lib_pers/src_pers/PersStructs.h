/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#pragma once

#define IMAGE_ROWS_MASK	(IMAGE_ROWS-1)
typedef sc_uint<IMAGE_ROWS_W>	ImageRows_t;

#define IMAGE_COLS_MASK	(IMAGE_COLS-1)
typedef sc_uint<IMAGE_COLS_W>	ImageCols_t;

#define MCU_ROWS_W	(IMAGE_ROWS_W-3)
#define MCU_ROWS_MASK	((1 << MCU_ROWS_W)-1)
typedef sc_uint<MCU_ROWS_W>		McuRows_t;

#define MCU_COLS_W	(IMAGE_COLS_W-3)
#define MCU_COLS_MASK	((1 << MCU_COLS_W)-1)
typedef sc_uint<MCU_COLS_W>		McuCols_t;

#define IMAGE_IDX_W 2
#define IMAGE_IDX_MASK ((1 << IMAGE_IDX_W)-1)
typedef sc_uint<IMAGE_IDX_W> ImageIdx_t;

typedef sc_uint<PNT_WGHT_SIZE_W> PntWghtIdx_t;

#define PNT_WGHT_IDX_W IMAGE_ROWS_W // needs to be the max of rows or cols
typedef sc_int<(PNT_WGHT_IDX_W+1)> PntWghtCpInt_t;

struct Four_i14 {
	ht_int14	m_i14[4];
};

struct Four_u13 {
	ht_uint13	m_u13[4];
};

struct Four_u12 {
	ht_uint12	m_u12[4];
};

struct Four_i13 {
	ht_int13	m_i13[4];
};

union Four_i16 {
	int16_t		m_i16[4];
	uint64_t	m_u64;
};

union Eight_i16 {
	int16_t		m_i16[8];
	uint64_t	m_u64[2];
};

union Eight_u8 {
	uint8_t		m_u8[8];
	uint64_t	m_u64;
};

union Sixteen_u8 {
	uint8_t		m_u8[16];
	uint64_t	m_u64[2];
};

union Sixteen_i16 {
	int16_t		m_i16[16];
	uint64_t	m_u64[4];
};

// Message from Jpeg Decoder to Horizontal Resizer
struct JpegDecMsg {
	ht_uint2	m_compIdx;		// component index
	ht_uint3	m_decPipeId;	// decode pipe index
	McuRows_t	m_rstIdx;		// full restart index
	bool		m_bEndOfMcuRow;
	bool		m_bRstLast;		// last data for restart
	bool		m_bEndOfImage;	// last data for image
	ImageIdx_t	m_imageIdx;		// image index for double buffering
	ht_uint8	m_jobId;		// host job info index (for debug)
	Eight_u8	m_data;			// column of eight bytes from IDCT
};

// Message to Jpeg Encoder from Vertical Resizer
struct JpegEncMsg {
	ht_uint2	m_compIdx;		// component index
	ht_uint3	m_blkRow;
	McuRows_t	m_rstIdx;		// full restart index
	bool		m_bEndOfMcuRow;
	ImageIdx_t	m_imageIdx;		// image index for double buffering
	Eight_u8	m_data;			// column of eight bytes from IDCT
};

// Message from Horizontal Resizer to Vertical Resizer
struct HorzResizeMsg {
	ht_uint3	m_imageHtId;
	ImageIdx_t	m_imageIdx;
	McuRows_t	m_mcuRow;
	bool		m_bEndOfImage;
};

// Message from Vertical Resizer to Jpeg Encoder
struct VertResizeMsg {
	bool		m_bEndOfMcuRow;
	ImageIdx_t	m_imageIdx;
	ht_uint2	m_compIdx;
	ht_uint1	m_mcuBlkRow;
	ht_uint1	m_mcuBlkCol;
	McuRows_t	m_mcuRow;
	McuCols_t	m_mcuCol;
	ht_uint3	m_blkRow;
	ht_uint4	m_fillCnt;
	uint64_t	m_data;
};

// JobInfo message
#define JOB_INFO_SECTION_DEC	0
#define JOB_INFO_SECTION_HORZ	1
#define JOB_INFO_SECTION_VERT	2
#define JOB_INFO_SECTION_ENC	3

#define JOB_INFO_MEM_LINE_IDX_W		13
#define JOB_INFO_QW_IDX_W			3
#define JOB_INFO_SECTION_ID_W		2

typedef sc_uint<JOB_INFO_MEM_LINE_IDX_W + JOB_INFO_QW_IDX_W>		JobInfoQw_t;
typedef sc_uint<JOB_INFO_MEM_LINE_IDX_W + JOB_INFO_SECTION_ID_W>	JobInfoReqInfo_t;

struct JobInfoMsg {
	ImageIdx_t		m_imageIdx;
	ht_uint2		m_sectionId;
	JobInfoQw_t		m_rspQw;
	uint64_t		m_data;
};

struct DecRstOff {
	ht_int26		e;
	McuCols_t		m;
};
