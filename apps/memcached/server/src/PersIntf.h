/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT memcached application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#pragma once

#define CNY_UNIT_UDP_CONN_CNT	16u
#define CNY_UNIT_UDP_CONN_MASK	(CNY_UNIT_UDP_CONN_CNT-1)

#define CNY_MAX_UNIT_CONN_CNT	512
#define CNY_MAX_UNIT_CONN_MASK	(CNY_MAX_UNIT_CONN_CNT-1)

#define CNY_CMD_PROTOCOL_DW_CNT	1
#define CNY_CMD_SET_DW_CNT		4
#define CNY_CMD_DATA_DW_CNT		2
#define CNY_CMD_GET_DW_CNT		3
#define CNY_CMD_DELETE_DW_CNT	3
#define CNY_CMD_OTHER_DW_CNT	2
#define CNY_CMD_ARITH_DW_CNT	4
#define CNY_CMD_TOUCH_DW_CNT	3
#define CNY_CMD_KEY_DW_CNT		2
#define CNY_CMD_GET_END_DW_CNT	1
#define CNY_BIN_CMD_DW_CNT		5
#define CNY_CMD_ERROR_DW_CNT	5

#define CNY_CMD_STORE_QW_CNT	4
#define CNY_CMD_GET_QW_CNT		2
#define CNY_CMD_GET_END_QW_CNT	1
#define CNY_CMD_DELETE_QW_CNT	2
#define CNY_CMD_ARITH_QW_CNT	3
#define CNY_CMD_TOUCH_QW_CNT	2
#define CNY_CMD_BIN_OTHER_QW_CNT	3
#define CNY_CMD_BIN_STORE_QW_CNT	5
#define CNY_CMD_BIN_GET_QW_CNT		3
#define CNY_CMD_BIN_DELETE_QW_CNT	4
#define CNY_CMD_BIN_ARITH_QW_CNT	6
#define CNY_CMD_BIN_TOUCH_QW_CNT	3
#define CNY_CMD_ERROR_QW_CNT	3

struct CCnyCpCmd {
	// first 32-bits
	uint32_t	m_cmd:8;
	uint32_t	m_ascii:1;
	uint32_t	m_noreply:1;
	uint32_t	m_keyInit:1;
    uint32_t	m_cmdMs:1;
    uint32_t	m_pad1:4;
	uint32_t	m_connId:16;

	// optional second 32-bits
	union {
		struct {
			uint32_t	m_keyPos:20;
			uint32_t	m_keyLen:12;
		};
		struct {
			uint32_t	m_dataPos:20;
			uint32_t	m_dataLen:12;
		};
		struct {
			uint32_t	m_errCode:8;
			uint32_t	m_reqCmd:8;
			uint32_t	m_pad2:16;
		};
	};

	// optional third 32-bits
	uint32_t	m_keyHash;

	// optional fourth 32-bit value
	uint32_t	m_valueLen;

	// optional fifth 32-bits
	uint32_t	m_opaque;
};

union CRspCmdQw {
	uint32_t	m_rspDw[16];
	uint64_t	m_rspQw[8];
	CCnyCpCmd	m_rspCmd;
};

// client->server commands
// binary constants
#define BINARY_REQ_MAGIC	0x80

#define BIN_REQ_CMD_GET			0x00
#define BIN_REQ_CMD_SET			0x01
#define BIN_REQ_CMD_ADD			0x02
#define BIN_REQ_CMD_REPLACE		0x03
#define BIN_REQ_CMD_DELETE		0x04
#define BIN_REQ_CMD_INCR		0x05
#define BIN_REQ_CMD_DECR		0x06
#define BIN_REQ_CMD_GETQ		0x09
#define BIN_REQ_CMD_GETK		0x0c
#define BIN_REQ_CMD_GETKQ		0x0d
#define BIN_REQ_CMD_APPEND		0x0e
#define BIN_REQ_CMD_PREPEND		0x0f

#define BIN_REQ_CMD_SETQ		0x11
#define BIN_REQ_CMD_ADDQ		0x12
#define BIN_REQ_CMD_REPLACEQ	0x13
#define BIN_REQ_CMD_DELETEQ		0x14
#define BIN_REQ_CMD_INCRQ		0x15
#define BIN_REQ_CMD_DECRQ		0x16
#define BIN_REQ_CMD_QUITQ		0x17
#define BIN_REQ_CMD_APPENDQ		0x19
#define BIN_REQ_CMD_PREPENDQ	0x1a
#define BIN_REQ_CMD_TOUCH		0x1c
#define BIN_REQ_CMD_GAT			0x1d
#define BIN_REQ_CMD_GATQ		0x1e
#define BIN_REQ_CMD_GATK		0x23
#define BIN_REQ_CMD_GATKQ		0x24


// coproc->host commands
#define CNY_RSP_CMD_MAX		256
typedef uint8_t				CnyRspCmd_t;

#define CNY_CMD_GET			0
#define CNY_CMD_SET			1
#define CNY_CMD_ADD			2
#define CNY_CMD_REPLACE		3
#define CNY_CMD_DELETE		4
#define CNY_CMD_INCR		5
#define CNY_CMD_DECR		6

#define CNY_CMD_APPEND		0xe
#define CNY_CMD_PREPEND		0xf

#define CNY_CMD_TOUCH		0x1c

#define CNY_CMD_NXT_RCV_BUF	0x20	// advance to next recieve buffer
#define CNY_CMD_NXT_RSP_BUF	0x21	// advance to next response buffer
#define CNY_CMD_DATA		0x22
#define CNY_CMD_GETS		0x23
#define CNY_CMD_KEY			0x24
#define CNY_CMD_CAS			0x25
#define CNY_CMD_GET_END		0x26
#define CNY_CMD_OTHER		0x27
#define CNY_CMD_PROTOCOL	0x28
#define CNY_CMD_ERROR		0x29

#define CNY_CMD_BIN_GET			(BINARY_REQ_MAGIC | BIN_REQ_CMD_GET)
#define CNY_CMD_BIN_SET			(BINARY_REQ_MAGIC | BIN_REQ_CMD_SET)
#define CNY_CMD_BIN_ADD			(BINARY_REQ_MAGIC | BIN_REQ_CMD_ADD)
#define CNY_CMD_BIN_REPLACE		(BINARY_REQ_MAGIC | BIN_REQ_CMD_REPLACE)
#define CNY_CMD_BIN_DELETE		(BINARY_REQ_MAGIC | BIN_REQ_CMD_DELETE)
#define CNY_CMD_BIN_INCR		(BINARY_REQ_MAGIC | BIN_REQ_CMD_INCR)
#define CNY_CMD_BIN_DECR		(BINARY_REQ_MAGIC | BIN_REQ_CMD_DECR)
#define CNY_CMD_BIN_GETQ		(BINARY_REQ_MAGIC | BIN_REQ_CMD_GETQ)
#define CNY_CMD_BIN_GETK		(BINARY_REQ_MAGIC | BIN_REQ_CMD_GETK)
#define CNY_CMD_BIN_GETKQ		(BINARY_REQ_MAGIC | BIN_REQ_CMD_GETKQ)
#define CNY_CMD_BIN_APPEND		(BINARY_REQ_MAGIC | BIN_REQ_CMD_APPEND)
#define CNY_CMD_BIN_PREPEND		(BINARY_REQ_MAGIC | BIN_REQ_CMD_PREPEND)
#define CNY_CMD_BIN_SETQ		(BINARY_REQ_MAGIC | BIN_REQ_CMD_SETQ)
#define CNY_CMD_BIN_ADDQ		(BINARY_REQ_MAGIC | BIN_REQ_CMD_ADDQ)
#define CNY_CMD_BIN_REPLACEQ	(BINARY_REQ_MAGIC | BIN_REQ_CMD_REPLACEQ)
#define CNY_CMD_BIN_DELETEQ		(BINARY_REQ_MAGIC | BIN_REQ_CMD_DELETEQ)
#define CNY_CMD_BIN_INCRQ		(BINARY_REQ_MAGIC | BIN_REQ_CMD_INCRQ)
#define CNY_CMD_BIN_DECRQ		(BINARY_REQ_MAGIC | BIN_REQ_CMD_DECRQ)
#define CNY_CMD_BIN_APPENDQ		(BINARY_REQ_MAGIC | BIN_REQ_CMD_APPENDQ)
#define CNY_CMD_BIN_PREPENDQ	(BINARY_REQ_MAGIC | BIN_REQ_CMD_PREPENDQ)
#define CNY_CMD_BIN_TOUCH		(BINARY_REQ_MAGIC | BIN_REQ_CMD_TOUCH)
#define CNY_CMD_BIN_GAT			(BINARY_REQ_MAGIC | BIN_REQ_CMD_GAT)
#define CNY_CMD_BIN_GATQ		(BINARY_REQ_MAGIC | BIN_REQ_CMD_GATQ)
#define CNY_CMD_BIN_GATK		(BINARY_REQ_MAGIC | BIN_REQ_CMD_GATK)
#define CNY_CMD_BIN_GATKQ		(BINARY_REQ_MAGIC | BIN_REQ_CMD_GATKQ)
#define CNY_CMD_BIN_DATA		0xC0

// error codes
#define CNY_ERR_BAD_FORMAT		0
#define CNY_ERR_INV_DELTA		1
#define CNY_ERR_INV_FLAG		2
#define CNY_ERR_INV_EXPTIME		3
#define CNY_ERR_INV_VLEN		4
#define CNY_ERR_BAD_CMD_EOL		5
#define CNY_ERR_BAD_DATA_EOL	6
#define CNY_ERR_BINARY_MAGIC	7
#define CNY_ERR_CONN_CLOSED		8
#define CNY_ERR_INV_ARG         9

#define CNY_RECV_BUF_HDR_CNT	        16
#define CNY_RECV_BUF_HDR_SIZE           sizeof(uint32_t)
#define CNY_RECV_BUF_HDR_BLOCK_SIZE     (CNY_RECV_BUF_HDR_CNT * CNY_RECV_BUF_HDR_SIZE)
#define CNY_RECV_BUF_ALIGN              64

// personality unit receive buffer constants
#define MCD_RECV_BUF_SIZE           (1024 * 1024)
#define MCD_RECV_BUF_SIZE_MASK      (MCD_RECV_BUF_SIZE-1)
#define MCD_RECV_BUF_SIZE_MASKQ     ((MCD_RECV_BUF_SIZE_MASK << 1) | 1)
#define MCD_RECV_BUF_MAX_BLK_CNT    64
#define MCD_RECV_BUF_MAX_BLK_SIZE   (16 * 1024)
#define MCD_RECV_BUF_MIN_PKT_SIZE   512
#define MCD_RECV_BUF_MAX_PKT_CNT    255     // max packets per recv block

#define MCD_RECV_UDP_SIZE           (MCD_RECV_BUF_SIZE / 1024)          // max of 1024 due to field limit
#define MCD_RECV_UDP_SIZE_MASK      (MCD_RECV_UDP_SIZE-1)
#define MCD_RECV_UDP_SIZE_MASKQ     ((MCD_RECV_UDP_SIZE_MASK << 1) | 1)
#define MCD_RECV_UDP_MIN_BLK_SIZE   8

#define CNY_RESP_BUF_CNT	        64
#define CNY_RESP_BUF_CNT_MASK	    (CNY_RESP_BUF_CNT-1)
#define CNY_RESP_BUF_SIZE	        (1024 * 64)
#define CNY_RESP_BUF_SIZE_MASK	    (CNY_RESP_BUF_SIZE-1)

// minimum space in receive buffer
#define CNY_RECV_BUF_MIN_SIZE	    1500

#define CNY_RESP_CMD_MAX		    256	// jump table for response commands
