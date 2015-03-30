/*****************************************************************************/
// Generated with htl
//
// Hybrid Thread File - HostIntf.h
//
//-----------------------------------------------------------------------------
//
// Copyright (c) 2013 : Created by Convey Computer Corp. This file is the
// confidential and proprietary property of Convey Computer Corp.
//
/*****************************************************************************/
#pragma once

//#include <stdint.h>

// Module generated defines/typedefs/structs/unions



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
#define HIF_ARG_IBLK_SMAX_W	20
#define HIF_ARG_IBLK_CNT_W	4

#define HIF_ARG_OBLK_SIZE_W	5
#define HIF_ARG_OBLK_SMAX_W	20
#define HIF_ARG_OBLK_CNT_W	4
#define HIF_ARG_OBLK_TIME_W	28

#define HIF_CTL_QUE_CNT_W	9
#define HIF_CTL_QUE_CNT		(1u << HIF_CTL_QUE_CNT_W)

// HIF configurable arguments
#define HIF_CTL_QUE_TIME	1000	// 1k ns

#define HIF_ARG_IBLK_SIZE_LG2	16
#define HIF_ARG_IBLK_SIZE	(1u << HIF_ARG_IBLK_SIZE_LG2)
#define HIF_ARG_IBLK_CNT_LG2	4
#define HIF_ARG_IBLK_CNT	(1u << HIF_ARG_IBLK_CNT_LG2)

#define HIF_ARG_OBLK_SIZE_LG2	16
#define HIF_ARG_OBLK_SIZE	(1u << HIF_ARG_OBLK_SIZE_LG2)
#define HIF_ARG_OBLK_CNT_LG2	4
#define HIF_ARG_OBLK_CNT	(1u << HIF_ARG_OBLK_CNT_LG2)


#ifndef HT_AE_CNT
#define HT_AE_CNT	1
#endif

#ifndef HT_UNIT_CNT
#define HT_UNIT_CNT	1
#endif

#define HT_NUMA_SET_MAX 4

struct CHtHifParams {
#ifndef _HTV
	CHtHifParams() {
		m_ctlTimerUSec = 10;
		m_iBlkTimerUSec = 1000;
		m_oBlkTimerUSec = 1000;
		m_appPersMemSize = 0;
		m_appUnitMemSize = 0;
		m_numaSetCnt = 0;
		memset(m_numaSetUnitCnt, 0, sizeof(uint8_t) * HT_NUMA_SET_MAX);
	};
#endif

	uint32_t m_ctlTimerUSec;
	uint32_t m_iBlkTimerUSec;
	uint32_t m_oBlkTimerUSec;
	uint64_t m_appPersMemSize;
	uint64_t m_appUnitMemSize;
	int32_t m_numaSetCnt;
	uint8_t m_numaSetUnitCnt[HT_NUMA_SET_MAX];
};

#define HIF_DQ_NULL 1
#define HIF_DQ_DATA 2
#define HIF_DQ_FLSH 3
#define HIF_DQ_CALL 6
#define HIF_DQ_HALT 7

// Consolidated list of user defined host messages
#define IHM_MSG2X 16
#define OHM_MSG2X 16


