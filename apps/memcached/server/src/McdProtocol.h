/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT memcached application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#pragma once
#include <stdint.h>

// Memcached binary protocol header

struct CMcdBinReqHdr {
    uint64_t        m_magic:8;
    uint64_t        m_opcode:8;
    uint64_t        m_keyLen:16;
    uint64_t        m_extraLen:8;
    uint64_t        m_dataType:8;
    uint64_t        m_vBucketId:16;
    uint64_t        m_bodyLen:32;
    uint64_t        m_opaque:32;
    uint64_t        m_uniqueId:64;
};

#define MCD_BIN_REQ_MAGIC       0x80

enum EMcdBinCmd {
    eMcdBinCmdGet =     0x00,
    eMcdBinCmdSet =     0x01,
    eMcdBinCmdAdd =     0x02,
    eMcdBinCmdReplace = 0x03,
    eMcdBinCmdDelete =  0x04,
    eMcdBinCmdIncr =    0x05,
    eMcdBinCmdDecr =    0x06,
    eMcdBinCmdQuit =    0x07,
    eMcdBinCmdFlush =   0x08,
    eMcdBinCmdGetq =    0x09,
    eMcdBinCmdNoop =    0x0a,
    eMcdBinCmdVersion = 0x0b,
    eMcdBinCmdGetk =    0x0c,
    eMcdBinCmdGetkq =   0x0d,
    eMcdBinCmdAppend =  0x0e,
    eMcdBinCmdPrepend = 0x0f,
    eMcdBinCmdStat =    0x10,
    eMcdBinCmdSetq =    0x11,
    eMcdBinCmdAddq =    0x12,
    eMcdBinCmdReplaceq = 0x13,
    eMcdBinCmdDeleteq = 0x14,
    eMcdBinCmdIncrq =   0x15,
    eMcdBinCmdDecrq =   0x16,
    eMcdBinCmdQuitq =   0x17,
    eMcdBinCmdFlushq =  0x18,
    eMcdBinCmdAppendq = 0x19,
    eMcdBinCmdPrependq = 0x1a,
    eMcdBinCmdTouch =   0x1c,
    eMcdBinCmdGat =     0x1d,
    eMcdBinCmdGatq =    0x1e,
    eMcdBinCmdGatk =    0x23,
    eMcdBinCmdGatkq =   0x24
};

enum EMcdDataType {
    eRawBytes=0x00
};

union CMcdBinRspHdr {
    struct {
        uint64_t        m_magic:8;
        uint64_t        m_opcode:8;
        uint64_t        m_keyLen:16;
        uint64_t        m_extraLen:8;
        uint64_t        m_dataType:8;
        uint64_t        m_status:16;
        uint64_t        m_bodyLen:32;
        uint64_t        m_opaque:32;
        uint64_t        m_uniqueId:64;
    };
    uint8_t             m_bytes[1];
};

struct CMcdBinRspGet {
    CMcdBinRspHdr   m_hdr;
    struct {
        uint32_t    m_flags;
    } m_body;
};

struct CMcdBinRspIncr {
    CMcdBinRspHdr   m_hdr;
    struct {
        uint64_t    m_value;
    } m_body;
};

struct CMcdBinRspTouch {
    CMcdBinRspHdr   m_hdr;
    struct {
        uint32_t    m_flags;
    } m_body;
};

#define MCD_BIN_RSP_MAGIC       0x81

enum EMcdBinRspStatus {
    eSuccess=0x00, 
    eKeyNotFound=0x01, 
    eKeyExists=0x02,
    eValueToBig=0x03,
    eInvalid=0x04,
    eNotStored=0x05,
    eBadDelta=0x06,
    eUnknownCmd=0x81,
    eNoMemory=0x82
};
