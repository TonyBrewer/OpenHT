/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#pragma once

#include <stdint.h>

// maximum image size
#define IMAGE_ROWS_W	13
#define IMAGE_ROWS	(1 << IMAGE_ROWS_W)
#define IMAGE_COLS_W	13
#define IMAGE_COLS	(1 << IMAGE_COLS_W)

#define HUFF_LOOKAHEAD 8

#define DCTSIZE_W					3
#define DCTSIZE						(1 << DCTSIZE_W)
#define DCTSIZE2					64

#define MEM_LINE_SIZE_W				6
#define MEM_LINE_SIZE				(1 << MEM_LINE_SIZE_W)

#define COPROC_PNT_WGHT_CNT			16
#define COPROC_MAX_IMAGE_PNTS_W		13
#define COPROC_MAX_IMAGE_PNTS		(1 << COPROC_MAX_IMAGE_PNTS_W)

#define PNT_WGHT_SIZE_W				8 // min value of 5
#define PNT_WGHT_SIZE				(1 << PNT_WGHT_SIZE_W)

#define MAX_MCU_COMPONENTS_W		2
#define MAX_MCU_COMPONENTS			(1 << MAX_MCU_COMPONENTS_W)

#define MAX_MCU_BLK_COLS_W			1
#define MAX_MCU_BLK_COLS			(1 << MAX_MCU_BLK_COLS_W)

#define MAX_MCU_BLK_ROWS_W			1
#define MAX_MCU_BLK_ROWS			(1 << MAX_MCU_BLK_ROWS_W)

#define VERT_PREFETCH_MCUS_W		4
#define VERT_PREFETCH_MCUS			(1 << VERT_PREFETCH_MCUS_W)
#define VERT_PREFETCH_MCUS_FULL		(VERT_PREFETCH_MCUS-1)

#define VERT_FILTER_COLS			8

#define COPROC_PNT_WGHT_W			12
#define COPROC_PNT_WGHT_FRAC_W		(COPROC_PNT_WGHT_W-1)

#define MAX_RST_LIST_SIZE 512
#define MAX_ENC_RST_CNT 512

#define MAX_JPEG_IMAGE_SIZE			(IMAGE_ROWS * IMAGE_COLS)
#define MAX_IMAGE_COMP_SIZE			(IMAGE_ROWS * IMAGE_COLS)

#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif
