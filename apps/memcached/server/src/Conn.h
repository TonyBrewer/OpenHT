/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT memcached application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#pragma once

#define CNY_XMIT_QUE_HIGHWAT	4

/** Maximum length of a key. */
#define KEY_MAX_LENGTH 250

#define CAS_STR_LEN	28

// maximum number of IOVs per transmitted packet
#define MAX_MSG_IOV_ENTRIES 7

#define XMIT_WR_BUF_SIZE	(sizeof(CMcdBinRspHdr) + KEY_MAX_LENGTH)

struct CItem;
struct CWorker;

// Declarations for tokenizing command line
typedef struct token_s {
    char *value;
    size_t length;
} token_t;

#define COMMAND_TOKEN 0
#define SUBCOMMAND_TOKEN 1
#define KEY_TOKEN 1

#define MAX_TOKENS 8

struct CMsgHdr {
	msghdr			m_msghdr;
	char			m_casStr[CAS_STR_LEN];
	iovec			m_iov[MAX_MSG_IOV_ENTRIES];
	CItem *			m_pItem;
	CMsgHdr *		m_pNext;
};

struct CXmitMsg {
	int				m_msgNum;
	int				m_msgCnt;
	char *			m_pStatsBuf;

	CMsgHdr *		m_pMsgHdrFront;
	CMsgHdr *		m_pMsgHdrBack;

    char            m_udpHdr[UDP_HEADER_SIZE];
	char 			m_wrBuf[XMIT_WR_BUF_SIZE];

	const char *	m_pSimpleMsg;

	bool			m_bCloseAfterXmit;
    bool            m_bFreeAfterXmit;

	CUdpReqInfo		m_udpReqInfo;

    uint64_t        m_xmitLinkTime;
	CXmitMsg *		m_pNext;
};

enum EStoreItemType {
    NOT_STORED=0, STORED, EXISTS, NOT_FOUND
};

enum EDeltaResultType {
    OK, NON_NUMERIC, EOM, DELTA_ITEM_NOT_FOUND, DELTA_ITEM_CAS_MISMATCH
};

/**
 * The structure representing a connection into memcached.
 */

class CConn {
public:
	CXmitMsg * AddXmitMsg();
	CMsgHdr * AddMsgHdr();

	void OutString(const char *str);
	bool AddIov(const void *buf, size_t len);
	void LinkForTransmit();
    void Transmit();
	void BuildUdpHeader(CXmitMsg * pXmitMsg);
	bool IsUdp() { return m_transport == udp_transport; }
	bool IsTcp() { return m_transport == tcp_transport; }
	bool UpdateEvent(const short new_flags);
	void WriteAndFree(char *buf, size_t bytes);
	void CloseConn();
    void SendCloseToCoproc();
	void FreeConn();
	void FreeXmitMsg(CXmitMsg * pXmitMsg, bool bInXmitList=true);
	EStoreItemType DoStoreItem(CItem *it, int comm);
	EDeltaResultType DoAddDelta(const char *key, const size_t nkey,
                                    const uint32_t hv, const bool incr, 
				    const uint64_t delta,
                                    char *buf, uint64_t *cas);
	void CompleteCmdStore();
	void CompleteBinStore();
	void AddBinHeader(uint16_t err, size_t hdr_len, size_t key_len, size_t body_len);
	void WriteBinResponse(const void *d, size_t hlen, size_t keylen, size_t dlen);
    void HandleBinaryProtocolError(EMcdBinRspStatus err);
	void WriteBinError(EMcdBinRspStatus err);
    void FindRecvBufString(char * &pStr, int &strLen, uint32_t cmdStrPos, uint32_t cmdStrLen, bool keyInit);

	void ProcessCommand(char *command);
	size_t TokenizeCommand(char *command, token_t *tokens, const size_t max_tokens);
	void ProcessVerbosityCommand(token_t *tokens, const size_t ntokens);
	void ProcessSlabsAutomoveCommand(token_t *tokens, const size_t ntokens);
	bool SetNoreplyMaybe(token_t *tokens, size_t ntokens);
	void ProcessStat(token_t *tokens, const size_t ntokens);
	void ProcessStatsDetail(const char *command);
	void ProcessBinStat(char * pKey, int keyLen);

	bool ProcessCmdUnused();

	bool ProcessCmdStore();
	bool ProcessCmdData();
	bool ProcessCmdGet();
	bool ProcessCmdGetEnd();
	bool ProcessCmdDelete();
	bool ProcessCmdError();
	bool ProcessCmdArith();
	bool ProcessCmdTouch();
	bool ProcessCmdOther();
	bool ProcessCmdKey();

	bool ProcessCmdBinStore();
	bool ProcessCmdBinData();
	bool ProcessCmdBinGet();
	bool ProcessCmdBinDelete();
	bool ProcessCmdBinArith();
	bool ProcessCmdBinOther();
	bool ProcessCmdBinTouch();
	bool ProcessCmdBinAppendPrepend();

	void ProcessSettingsStats();
	void ProcessServerStats();
	void ThreadStatsAggregate(CThreadStats *stats);
	void SlabStatsAggregate(CThreadStats *stats, CSlabStats *out);
	void AppendAsciiStats(const char *key, const size_t klen, const char *val, const size_t vlen);
	void AppendBinStats(const char *key, const size_t klen, const char *val, const size_t vlen);
	bool GrowStatsBuf(size_t needed);
	void AppendStat(const char *name, const char *fmt, ...);
	void AddStats(const char *key, const size_t klen, const char *val, const size_t vlen);

	void DoSlabsStats();
	void DoItemStats();

public:
    int				m_sfd;
    struct event	m_event;
	short			m_connId;		// connection ID used on coprocessor
    short			m_eventFlags;	// current state of m_event flags
	bool			m_bXmitActive;
	bool			m_bListening;
	bool			m_bEventActive;	// m_event is active in event loop
	bool			m_bFree;
	bool			m_bCloseReqSent;
    uint8_t         m_caxCode;

    char *			m_pItemRd;  /** when we read in an item's value, it goes here */
    int				m_itemRdLen;

	char			m_partialStr[KEY_MAX_LENGTH];
	short			m_partialLen;

    CItem *			m_pItem;

    bool            m_bXmitConnActive;
    CConn *         m_pXmitConnNext;

    // For powersave - linked list of pending (not yet registered with libevent) connections
    CConn *			m_psPendNext;

    /**
     * item is used to hold an item structure created after reading the command
     * line of set/add/replace commands, but before we finished reading the actual
     * data. The data is read into ITEM_data(item) to avoid extra copying.
     */

	uint32_t			m_xmitMsgCnt;
	CXmitMsg *			m_pXmitMsgFront;
	CXmitMsg *			m_pXmitMsgBack;

	// State for command begin processed from coproc to queued response
	CXmitMsg *			m_pXmitMsg;
	CMsgHdr *			m_pMsgHdr;
	iovec *				m_pIov;

    int					m_msgbytes;		/* number of bytes in current msg */

    EProtocol			m_protocol;   /* which protocol this connection speaks */
    ETransport			m_transport; /* what transport is used by this connection */

    /* data for UDP clients */
	int					m_udpSeqNum;
	CUdpReqInfo *		m_pRspUdpReqInfo;	// udpReqInfo associated with command being processed

    bool				m_noreply;   /* True if the reply should not be sent. */
	bool				m_ascii;

    /* current stats command */
    struct {
        char *	m_pBuffer;
        size_t	m_size;
        size_t	m_offset;
    } m_stats;

    /* Binary protocol stuff */
    uint64_t			m_cas;		/* the cas to return */
    uint8_t				m_cmd;		/* current command being processed */
	uint8_t				m_opcode;
    uint32_t			m_opaque;
    CConn *				m_pNext;     /* Used for generating a list of conn structures */
    CWorker *			m_pWorker; /* Pointer to the thread object serving this connection */
};
