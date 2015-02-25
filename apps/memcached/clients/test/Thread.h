/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT memcached application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#pragma once

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <list>
#include <vector>
using namespace std;

#ifdef _WIN32
#include <Winsock2.h>
#include "linux/linux.h"			// linux to windows
#else

#include <unistd.h>
#include <assert.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#define closesocket		close
#define WSACleanup()
#define WSAGetLastError()       errno

#define INVALID_SOCKET          -1
#define SOCKET_ERROR            -1
#define WSAECONNRESET           -1

typedef int			SOCKET;
typedef struct sockaddr		SOCKADDR;

#include <sys/socket.h>
#endif

#include "mtrand.h"

#include "BinaryProtocol.h"

extern volatile int	threadFails;

struct ArgValues {
	int		test;		// 0=random  1=directed  2=perf
	int		seed;		// random number seed 0=pick one
	string		hostName;	// host to connect to
	string		hostIp;		// ip address
	string		cmdPctFile;	// file to parse for command Pct
	int		tcpPort;	// tcp port to use
	int		udpPort;	// udp port to use
	int		binding;	// 0=auto, 1=binary 2=ascii
	int		noFlush;	// 0=flush on start
	int		cmdCnt;		// number of loops to run (random)
	int		verbose;	// 0=disabled 1=min 2=all
	int		threadCnt;	// # threads to use
	int		threadCntE;	// # threadCnt threads to gen errors
	int		threadCntT;	// # threadCnt threads to test timeouts
	int		perfStoreCnt;	// perf # stores per thread
	int		perfGetCnt;	// perf # get per thread
	int		perfKeyLength;	// perf key length
	int		perfDataLength;	// perf data length
	int		perfStop;	// stop after stores
	int		stop;		// stop on error 0=no
	int		getMax;		// max number of getQ to send together
	int		statTest;	// stat test to run
	int		fillPct;	// percent keys to fill at start
	int		perfTime;	// seconds to rum perf
	int		dataMax;	// max data length
	int		dataMin;	// min data length
	int		traceKey;	// key to trace all acitvity on
	uint32_t	keyTimeWindow;	// time +- to ignore expiring timers
	int		progC;		// prog test count
	int		progT;		// prog time sec
	int		progP;		// use . instead of threads
};

class	CmdPct {
    public:
	    CmdPct() { percent = minRnd = maxRnd = -1; }

	int	percent;			// set 22   22%
	int	minRnd;				// min rand number
	int	maxRnd;				// max rand number
};

enum ECmd { eSetCmd, eAddCmd, eReplaceCmd, eAppendCmd, ePrependCmd,
	    eCasCmd,
	    eGetCmd, eGetsCmd, eGetkCmd,
	    eGetrCmd, eGatCmd,			// eGetrCmd is multiple getQ 
	    eIncrCmd, eDecrCmd,
	    eDelCmd, eTouchCmd,
	    eStatsCmd, eVersionCmd, eNoopCmd,
	    eCmdCnt };

enum ERsp { eNoreply, eStored, eNotStored, eGetHit, eGetMiss,
		eDeleted, eNotFound, eStatsEnd, eValue, eTouched,
		eExists, eSwallow, eNonNumeric, eInvArgs };

enum ErrType	{ eMagic, eCmd, eKeyLen, eDataType,
		  eErrCnt };

enum Action	{ aSkip, aDropConn, aUnkCmd, aInvArg, aNotFound, aError, aError2, aTAS };

#ifdef GENERATE_STRINGS
const char	*cmdStr[] = {
    "SetCmd    ", "AddCmd    ", "ReplaceCmd", "AppendCmd ", "PrependCmd",
    "CasCmd    ",
    "GetCmd    ", "GetsCmd   ", "GetkCmd   ", "GetrCmd   ", "GatCmd    ",
    "IncrCmd   ", "DecrCmd   ",
    "DelCmd    ", "TouchCmd  ",
    "StatsCmd  ", "VersCmd   ", "NoopCmd   "};

const char	*cmdExpStr[] = {
    "NoReply", "Stored", "NotStored", "GetHit", "GetMiss",
    "Deleted", "NotFound",
    "StatsEnd", "Value", "Touched", "Exists", "Swallow",
    "NonNumeric", "InvArgs" };

const char	*errStr[] = {
	"Magic", "Cmd", "Keylen", "DataType"
    };

const char	*actStr[] = {
    "Skip", "DropConn", "UnkCmd", "InvArg", "NotFound", "Error", "Error2", "TestStop" };

CmdPct	cmdPct[eCmdCnt];

#else
extern const char	*cmdStr[];
extern const char	*cmdExpStr[];
extern const char	*errStr[];
extern const char	*actStr[];
extern CmdPct		cmdPct[eCmdCnt];
#endif	// GENERATE_STRINGS

struct GetQ {
    int		cmdKey;
    int		cmdFlag;
    int		cmdData;
    ERsp	cmdExpected;
    uint64_t	cmdCas;
};

#define RESP_BUF_SIZE	4096

#define UDP_HEADER_SIZE	8

struct CUdpHdr {
	CUdpHdr() { m_pad = 0; }
	uint16_t	m_reqId;
	uint16_t	m_seqNum;
	uint16_t	m_pktCnt;
	uint16_t	m_pad;
};

struct CMultiGetList {
        int m_keyIdx;
        int m_flagsIdx;
        int m_dataIdx;
        ERsp m_reply;
};

ECmd	getCmdRnd(uint32_t r);


class CThread {
public:
	CThread(int tid, bool bBinary, bool bUdp, ArgValues &argvalues);
	~CThread();

	static void	*Startup(void *pCthread);
	void	TestRandom();
	void	TestDirected();
	void	TestPerf();
	void	TestStats();
	void	printPerfData(void);
	void	ErrorMsg(string msg, string *expected, string *got);
	bool	CheckHdrOk(const char *eMsg,
				protocol_binary_response_header &hdr,
				protocol_binary_response_header &exp,
				SkipPbrh testPbrh);
	bool	CheckDataOk(const char *eMsg, uint8_t *got, uint8_t *exp,
				int length, bool asciiData);

	void TcpSocket();
	void UdpSocket();

	void EnableUdp(bool bUdpEnabled) {
		m_bUdpEnabled = bUdpEnabled;
	}
	void EnableBinary(bool bBinaryEnabled) {
		m_bBinaryEnabled = bBinaryEnabled;
	}

	void	BinaryForceError(int key, msghdr &msghdr);
	void	AsciiForceError(int key, msghdr &msghdr);

	bool RespGetToEol(string &statString);
	bool RespGetToEolTcp(string &statString);
	bool RespGetToEolUdp(string &statString);

	bool RespCheck(string respStr, bool bErrMsg=false);
	bool RespCheckUdp(string respStr, bool bErrMsg=false);
	bool RespCheckTcp(string respStr, bool bErrMsg=false);

	bool RespCheck(uint64_t value, bool bErrMsg=false);
	bool RespCheckUdp(uint64_t value, bool bErrMsg=false);
	bool RespCheckTcp(uint64_t value, bool bErrMsg=false);

	bool RespCheck(uint64_t value, size_t size, bool bErrMsg=false);
	bool RespCheckUdp(uint64_t value, size_t size, bool bErrMsg=false);
	bool RespCheckTcp(uint64_t value, size_t size, bool bErrMsg=false);

	uint64_t RespGetUint64(string &vStr);
	uint64_t RespGetUint64Udp(string &vStr);
	uint64_t RespGetUint64Tcp(string &vStr);

	void SendMsg(msghdr & msghdr);
	void SendMsgUdp(msghdr & msghdr);
	void SendMsgTcp(msghdr & msghdr);

	void Store(ECmd cmd, int keyIdx, int flagsIdx, uint64_t value,
					uint64_t casUnique, ERsp reply);
	void Store(ECmd cmd, int keyIdx, int flagsIdx, string &str,
					uint64_t casUnique, ERsp reply);
	void Store(ECmd cmd, int keyIdx, int flagsIdx, int dataIdx,
					uint64_t casUnique, ERsp reply);

        uint64_t GetReply(int keyIdx, int flagsIdx, int dataIdx, ERsp reply);
        uint64_t BinaryGetReply(int keyIdx,int flagsIdx,int dataIdx,ERsp reply);
        uint64_t AsciiGetReply(int keyIdx, int flagsIdx,int dataIdx,ERsp reply);

	uint64_t Get(ECmd cmd, int keyIdx, int flagsIdx,int dataIdx,
			ERsp reply, uint64_t casUnique);
	uint64_t Get(ECmd cmd, int keyIdx, int flagsIdx, string &dataStr,
			ERsp reply, uint64_t casUnique);
	void Delete(int keyIdx, ERsp reply, uint64_t casUnique);
	void Arith(ECmd cmd, int keyIdx, uint64_t inV, uint64_t outV,
			uint32_t expTmr, ERsp reply, uint64_t casUnique);
	void Touch(int keyIdx, ERsp reply, uint64_t casUnique);
	void Other(const char *pCmdStr, ERsp reply);
	void Quit(ERsp reply);
	void Version();
	void Noop();
	void FlushAll();
	void Stats(const char *pSubStat=0, bool printData = false,
					const char *hdr = 0);

	void AsciiStore(ECmd cmd, int keyIdx, int flagsIdx, uint64_t value,
					uint64_t casUnique, ERsp reply);
	void AsciiStore(ECmd cmd, int keyIdx, int flagsIdx, string & str,
					uint64_t casUnique, ERsp reply);
	void AsciiStore(ECmd cmd, int keyIdx, int flagsIdx, int dataIdx,
					uint64_t casUnique, ERsp reply);

	uint64_t AsciiGet(ECmd cmd, int keyIdx, int flagsIdx, int dataIdx,
					ERsp reply);
	uint64_t AsciiGet(ECmd cmd, int keyIdx, int flagsIdx, string &dataStr,
					ERsp reply);
        void    AsciiMultiGet(vector<GetQ> &getQvect);

	void AsciiDelete(int keyIdx, ERsp reply);
	void AsciiArith(ECmd cmd, int keyIdx, uint64_t inV, uint64_t outV,
					uint64_t expTmr, ERsp reply);
	void AsciiTouch(int keyIdx, ERsp reply);
	void AsciiQuit(ERsp reply);
	void AsciiVersion();
	void AsciiNoop();
	void AsciiFlushAll();
	void AsciiStats(const char *pSubStat, bool printData = false,
					const char *hdr = 0);

	void BinaryStore(ECmd cmd, int keyIdx, int flagsIdx, uint64_t value,
				 uint64_t casUnique, ERsp reply);
	void BinaryStore(ECmd cmd, int keyIdx, int flagsIdx, string & str,
				uint64_t casUnique, ERsp reply);
	void BinaryStore(ECmd cmd, int keyIdx, int flagsIdx, int dataIdx,
				uint64_t casUnique, ERsp reply);

	void BinaryClientError();
	uint64_t BinaryGet(ECmd cmd, int keyIdx, int flagsIdx,
				int dataIdx, ERsp reply, uint64_t casUnique);
	uint64_t BinaryGet(ECmd cmd, int keyIdx, int flagsIdx,
				string &dataStr, ERsp reply,uint64_t casUnique);


	void BinaryDelete(int keyIdx, ERsp reply, uint64_t casUnique);
	void BinaryArith(ECmd cmd, int keyIdx, uint64_t inV, uint64_t outV,
			    uint32_t expTmr, ERsp reply, uint64_t casUnique);
	void BinaryTouch(int keyIdx, ERsp reply, uint64_t casUnique);
	void BinaryOther(const char *pCmdStr, ERsp reply);
	void BinaryQuit(ERsp reply);
	void BinaryVersion();
	void BinaryNoop();
	void BinaryFlushAll();
	void BinaryStats(const char *pSubStat, bool printData = false,
			const char *hdr = 0);

	bool BinaryRespRead(void *pBuf, size_t len);
	bool BinaryRespReadTcp(void * pBuf, size_t size);
	bool BinaryRespReadUdp(void * pBuf, size_t size);

	void BinaryRespSkip(ssize_t len);
	void BinaryRespSkipUdp(ssize_t len);
	void BinaryRespSkipTcp(ssize_t len);

	void	Test_DeleteAddGet();
	string	&GetDataStr(int dataIdx) { return m_randData[dataIdx]; }
	void	CreateUdpHeader(char * udphdr);
	bool	ParseUdpHeader(CUdpHdr * pHdr);
	bool	GetBinEnabled() { return(m_bBinaryEnabled); }
	bool	GetUdpEnabled() { return(m_bUdpEnabled); }
	int	GetUdpDropped() { return(m_udpDropped); }
	bool	GetIsTiming() { return(m_timeCmds); }

	bool	FillUdpBuffer();

private:
	vector<GetQ>		getQvect;
	ArgValues		args;
	bool			m_genErr;
	bool			m_timeCmds;
	uint32_t		m_currTime;
	int			m_tid;	// thread id 0-m_threadCnt
	volatile static bool	m_pauseThreads;
	ECmd			m_cmdCmd;
	int			m_cmdCnt[eCmdCnt];
	uint64_t		m_randCas;
	uint32_t		m_opaque;
	uint32_t		m_rand32; // low 2 bytes key, byte 3 time
	int			m_tState;
#define m_trace2	(args.verbose >= 2)
#define m_trace3	(args.verbose >= 3)
	bool			m_bUdpEnabled;
	int			m_udpDropped;
	bool			m_bBinaryEnabled;
	SOCKET			m_udpFd;
	SOCKET			m_tcpFd;
	uint16_t		m_udpReqId;
	uint16_t		m_udpSeqNum;
	struct sockaddr_in	m_udpAddr;
	socklen_t		m_udpAddrLen;
	MTRand_int32		m_rnd;
	static const int	m_keyCnt = 1024;
	static const int	m_flagCnt = 1024;
	static const int	m_dataCnt = 1024;

	string				m_randKey[m_keyCnt];
	string				m_randData[m_dataCnt];
	string				m_prependData[m_dataCnt];
	string				m_appendData[m_dataCnt];
	uint32_t			m_randFlags[m_flagCnt];
	enum				KEY_STATE { INVALID, VALID };
	uint64_t			m_gCas;
	enum				CAS_STATE { C_INV, C_VAL };
	CAS_STATE			m_gCasState;
	struct	KeyState {
		KEY_STATE		m_state;	// state of key
		int			m_flagIdx;	// flags of valid key
		int			m_dataIdx;	// data of valid key
		int			m_preIdx;	// prepend data -1= none
		int			m_appIdx;	// append data -1 = none
		uint64_t		m_cas;
		bool			m_casValid;	// is cas valid?
		uint32_t		m_timeout;	// curr timeout
		uint32_t		m_nextTimeout;	// next, both unix time
	};
	KeyState			m_keyState[m_keyCnt];

	ssize_t				m_tcpRespBufLen;
	char				m_tcpRespBuf[RESP_BUF_SIZE];
	char *				m_pTcpRespBuf;
	ssize_t				m_udpRespBufLen;
	char				m_udpRespBuf[RESP_BUF_SIZE];
	char *				m_pUdpRespBuf;

	bool				KeyItemPresent(int key);
	void		SetItemState(KEY_STATE valid, ECmd cmd, int key,
				int flagIdx, int dataIdx, uint64_t cas,
				CAS_STATE casVal);
	void		GetKeyState(int key, int &flag, int &data,
				uint64_t &cas, bool &casVal,
				KEY_STATE *keyState=0, uint32_t *timeVal=0);
	bool		CheckKeyTime(ECmd cmd, int key, bool &timedOut);
	void		ClearKeyTimeout(int key);
	void		SetKeyNextTimeout(int key, uint32_t nextTimeout);
	uint32_t	StartKeyTimeout(int key);
	uint64_t	rand64(void) { return(((uint64_t)rand() << 36)
					^ ((uint64_t)rand() << 12) ^ rand()); }
	bool		m_respCheckOk;	// set/clr in RespCheckTcp/Udp
	ssize_t		m_recv(int fd, void *buf, size_t len, int flags,
					char *routine);

	char				**m_perfKeyTbl;
	char				*m_perfKeyArray;
	char				**m_perfDataTbl;
	char				*m_perfDataArray;
	char				*m_perfDataFixed;
	int				m_perfDataFixedLength;
	int				m_asciiStoreIndex;
	int				m_asciiGetIndex;
	int				m_binaryStoreIndex;
	int				m_binaryGetIndex;
	int				m_dataHdrLength;
	volatile static bool		m_serverInitComplete;
	uint64_t			m_startTime;
	uint64_t			m_stopTime;
	uint64_t			m_storeTime;
	uint64_t			m_getTime;
	uint64_t			m_currUsec;
	int				m_currSec;
	void				PrintTrace(bool dot);
};
