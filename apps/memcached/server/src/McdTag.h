/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT memcached application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#pragma once

// Tag structures used by coprocessor
//   Also used by host to initialize free pool

union CTag;

struct CTagPtr {
    CTagPtr() {}
    CTagPtr(int idx) { m_tagIdx = idx; m_pTag = operator->(); }

                operator CTag *() { return operator->(); }
    CTag *      operator -> ();
    bool        operator == (uint32_t idx) { return m_tagIdx == idx; }
    bool        operator != (uint32_t idx) { return m_tagIdx != idx; }
    void        operator = (CTagPtr ptr) { m_tagIdx = ptr.m_tagIdx; m_pTag = operator->(); }
    void        operator = (uint32_t idx) { m_tagIdx = idx; m_pTag = operator->(); }
    void        SetIdx( uint32_t idx ) { m_tagIdx = idx; m_pTag = operator->(); }
    uint32_t    GetIdx() { return m_tagIdx; }

    static void SetTagMemBase(CTag * pTagMemBase) {
        if (m_pTagMemBase == 0)
            m_pTagMemBase = pTagMemBase;

        // m_pTagMemBase is static (thus shared by all CPersUnits)
        // verify that all units provided the same base address
        assert(m_pTagMemBase == pTagMemBase);
    }
    static void SetTagMemSize(uint32_t tagMemSize) {
        if (m_tagMemSize == 0)
            m_tagMemSize = tagMemSize;

        // m_tagMemSize is static (thus shared by all CPersUnits)
        // verify that all units provided the same base address
        assert(m_tagMemSize == tagMemSize);
    }

    // index into tag memory
#define CHECK_TAG_STATE
#ifdef CHECK_TAG_STATE
    uint32_t    m_tagIdx:29;    // supports 32GB in 64 byte chunks
    uint32_t    m_pad:3;        // used for tag state checking
#else
    uint32_t    m_tagIdx:31;    // supports 128GB in 64 byte chunks
    uint32_t    m_pad:1;        // used for locking (such as freeTagBase and HashTbl)
#endif

private:
    static CTag *   m_pTagMemBase;
    static uint32_t m_tagMemSize;
    static CTag *   m_pTag; // used for debug visibility only
};

// Tag entry for a key/value pair
#define TAG_KEY_SIZE    48
union CTag {
public:
    bool        KeyCmp(char const * pKey, int keyLen);
    void        KeyInsert(char const * pKey, int keyLen);
    void        SetNext( CTagPtr ptr ) { m_hashNext = ptr; }
    void        SetNext( int v ) { assert(v == 0); m_hashNext = 0; }
    CTagPtr     GetNext() { return m_hashNext; }
    void        SetExtKeyPtr( int i, CTagPtr ptr ) { m_extKeyPtr[i] = ptr; }
    CTagPtr     GetExtKeyPtr( int i ) { return m_extKeyPtr[i]; }
    char *      GetKey() { return m_key; }
    void        SetExpTime(uint32_t expTime) { m_expTime = expTime; }
    void        SetLruTime(uint32_t lruTime) { m_lruTime = lruTime; }
    uint32_t    GetExpTime() { return m_expTime; }
    uint32_t    GetLruTime() { return m_lruTime; }
    void        SetKeyLen(uint8_t keyLen) { m_keyLen = keyLen; }
    uint8_t     GetKeyLen() { return m_keyLen; }
    void        SetPostFlush(uint32_t isPostFlush) { m_isPostFlush = isPostFlush; }
    bool        IsPostFlush() { return m_isPostFlush; }
    void        Init() {
        m_waitingForTagDelete = m_inUse = m_inFreeList = 0;
    }
    void        SetWaitingForTagDelete(bool bWaiting) {
        assert(m_waitingForTagDelete != bWaiting); assert(m_inUse == 0);
        assert(m_inFreeList == 0); m_waitingForTagDelete = bWaiting;
    }
    void        SetIsInUse(bool bInUse) {
        assert(m_inUse != bInUse); assert(m_waitingForTagDelete == 0);
        assert(m_inFreeList == 0); m_inUse = bInUse;
    }
    void        SetIsInFreeList(bool bInFreeList) {
        assert(m_inFreeList != bInFreeList);
        assert(m_inUse == 0);
        assert(m_waitingForTagDelete == 0);
        m_inFreeList = bInFreeList;
    }

public:
    struct {
        struct {    // 1st qw
            uint32_t    m_keyLen:8;
            uint32_t    m_isPostFlush:1;
#ifdef CHECK_TAG_STATE
            uint32_t    m_pad1:20;
            uint32_t    m_waitingForTagDelete:1;
            uint32_t    m_inUse:1;
            uint32_t    m_inFreeList:1;
#else
            uint32_t    m_pad1:23;
#endif
            CTagPtr     m_hashNext;
        };
        struct {
            uint32_t    m_expTime;          // time when tag expires
            uint32_t    m_lruTime;          // time last accessed
        };
        struct {
            char        m_key[TAG_KEY_SIZE - 4 * sizeof(CTagPtr)];
            CTagPtr     m_extKeyPtr[4];     // extended key pointers
        };
    };

    // tag free list structure
    struct {
        CTagPtr     m_freeNext;
        CTagPtr     m_tagList[15];
    };

    uint8_t     m_qw[64];    // force size to be 64 bytes
};

inline CTag * CTagPtr::operator -> () { return m_pTagMemBase + m_tagIdx; }

union CTagPoolBase {
    void Lock() {
        while ((__sync_fetch_and_or(&m_qw, 1ull << 63) & (1ull << 63)) != 0);
    }
    void Unlock() {
        __sync_synchronize(); 
        m_lock = 0;
    }
    struct {
        uint32_t        m_freeTagCnt;
        CTagPtr         m_freeTagHead;
    };
    struct {
        uint64_t        m_pad:63;
        uint64_t        m_lock:1;
    };
    volatile long long  m_qw;
};
