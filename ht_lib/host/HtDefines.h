/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */

// HIF internal commands
#define HIF_CMD_CTL_PARAM	0
#define HIF_CMD_IBLK_ADR	1
#define HIF_CMD_OBLK_ADR	2
#define HIF_CMD_IBLK_PARAM	3
#define HIF_CMD_OBLK_PARAM	4
#define HIF_CMD_IBLK_RDY	5
#define HIF_CMD_OBLK_AVL	6
#define HIF_CMD_OBLK_FLSH_ADR	7

#define HIF_CMD_OBLK_RDY	8
#define HIF_CMD_IBLK_AVL	9
#define HIF_CMD_ASSERT		10
#define HIF_CMD_ASSERT_COLL	11

// HIF argument widths
#define HIF_ARG_CTL_TIME_W	16

#define HIF_ARG_IBLK_SIZE_W	5
#define HIF_ARG_IBLK_SMAX_W	29
#define HIF_ARG_IBLK_CNT_W	6

#define HIF_ARG_OBLK_SIZE_W	5
#define HIF_ARG_OBLK_SMAX_W	29
#define HIF_ARG_OBLK_CNT_W	6
#define HIF_ARG_OBLK_TIME_W	28

#define MODEL_HIF_CTL_QUE_CNT_W	9
#define MODEL_HIF_CTL_QUE_CNT	(1u << MODEL_HIF_CTL_QUE_CNT_W)

// HIF configurable arguments
//#define HIF_ARG_IBLK_SIZE_LG2	16
//#define HIF_ARG_IBLK_SIZE	(1u << HIF_ARG_IBLK_SIZE_LG2)
//#define HIF_ARG_IBLK_CNT_LG2	4
//#define HIF_ARG_IBLK_CNT	(1u << HIF_ARG_IBLK_CNT_LG2)

//#define HIF_ARG_OBLK_SIZE_LG2	16
//#define HIF_ARG_OBLK_SIZE	(1u << HIF_ARG_OBLK_SIZE_LG2)
//#define HIF_ARG_OBLK_CNT_LG2	4
//#define HIF_ARG_OBLK_CNT	(1u << HIF_ARG_OBLK_CNT_LG2)

// Data queue types
#define HIF_DQ_NULL 1
#define HIF_DQ_DATA 2
#define HIF_DQ_FLSH 3
#define HIF_DQ_CALL 6
#define HIF_DQ_HALT 7

// CSR Operations
#define HT_CSR_RD 0
#define HT_CSR_WR 1

#ifndef HT_HIF_LIB
# ifndef HT_AE_CNT
# define HT_AE_CNT	1
# endif

# ifndef HT_UNIT_CNT
# define HT_UNIT_CNT	1
# endif
#endif
