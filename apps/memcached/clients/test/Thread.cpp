/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT memcached application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#define    GENERATE_STRINGS
#include "Thread.h"
#undef GENERATE_STRINGS

#ifndef _WIN32
#define	SLEEP	sleep(1);
#include <sys/time.h>
#else
#define	SLEEP	;
#endif

uint64_t	getTime(uint64_t *currUsec = 0, int *currSec = 0);
int		printTime = 0xffffffff;

int	DATA_MAX =	2000;		// max lengths can + PRE and APP
#define KEY_MAX		250		// max lengths for keys
#define	PRE_MAX		200
#define	APP_MAX		200

volatile bool	    CThread::m_pauseThreads = false;	// pause for error print
volatile int	    threadFails = 0;			// # thread failures

CThread::CThread(int tid, bool bBinary, bool bUdp, ArgValues &argValues)
{
    args = argValues;
    m_tid = tid;

    m_tcpRespBufLen = 0;
    m_udpRespBufLen = 0;
    m_bUdpEnabled = bUdp;
    m_bBinaryEnabled = bBinary;
    m_udpReqId = 2;
    m_udpFd = 0;
    m_tcpFd = 0;
    m_udpDropped = 0;
    DATA_MAX = args.dataMax;

    m_rnd.seed(m_tid + args.seed);

    char * pBuf = new char [32768];

    if(m_trace3) {
	printf("thread %d\n", m_tid);
    }

    // generate random keys
    for (int i = 0; i < m_keyCnt; i += 1) {
	int hl = sprintf(pBuf, "Key%02d_t%02d_%d", i, m_tid, args.noFlush);
	int len = hl + (m_rnd() % (KEY_MAX - hl));
	for (int j = hl; j < len; j += 1)
	    pBuf[j] = 'a' + m_rnd() % 26;
	pBuf[len] = '\0';
	m_randKey[i] = pBuf;
	m_keyState[i].m_state = INVALID;
	m_keyState[i].m_preIdx = -1;
	m_keyState[i].m_appIdx = -1;
	if(m_trace3) {
	    printf("key[%4d]   length  %3ld\n", i, m_randKey[i].length());
	}
    }

    // generate random data
    for (int i = 0; i < m_dataCnt; i += 1) {
	int hl = sprintf(pBuf, "Data%02d_t%02d_", i, m_tid);
	int len = (m_rnd() % DATA_MAX) + 1;
	if(len < args.dataMin) {
	    len = args.dataMin;
	}
	if(len > hl) {
	    for (int j = hl; j < len; j += 1)
		pBuf[j] = 'a' + m_rnd() % 26;
	}
	pBuf[len] = '\0';
	m_randData[i] = pBuf;
	if(m_trace3) {
	    printf("data[%4d]   length  %4d\n", i, len);
	}

	sprintf(pBuf, "Prepend");
	len = strlen(pBuf) + m_rnd() % PRE_MAX;
	for (int j = strlen(pBuf); j < len; j++)
	    pBuf[j] = 'a' + m_rnd() % 26;
	pBuf[len] = '\0';
	m_prependData[i] = pBuf;

	sprintf(pBuf, "Append");
	len = strlen(pBuf) + m_rnd() % APP_MAX;
	for (int j = strlen(pBuf); j < len; j++ )
	    pBuf[j] = 'a' + m_rnd() % 26;
	pBuf[len] = '\0';
	m_appendData[i] = pBuf;
    }

    // generate random flags
    for (int i = 0; i < m_flagCnt; i += 1) {
	m_randFlags[i] = m_rnd();
    }

    delete pBuf;
    for(int i=0; i<eCmdCnt; i++) {
	m_cmdCnt[i] = 0;
    }
}

CThread::~CThread()
{
    if (m_udpFd)
	closesocket(m_udpFd);
    if (m_tcpFd)
	closesocket(m_tcpFd);
}

#ifdef _WIN32
extern bool	threadsStart;
#else
volatile extern bool	threadsStart;
#endif
volatile bool		CThread::m_serverInitComplete = false;

void *
CThread::Startup(void *pCThread)
{
    CThread	*cThread = (CThread*)pCThread;

    while(threadsStart == false) {
	SLEEP
    }
    if(cThread->m_tid == 0) {
	cThread->UdpSocket();
	cThread->TcpSocket();
	if(cThread->args.noFlush == 0) {
	    cThread->FlushAll();
	}
	m_serverInitComplete = true;
    } else {
	while(m_serverInitComplete == false) {
	    SLEEP
	}
	cThread->UdpSocket();
	cThread->TcpSocket();
    }
    if((cThread->m_tid == 0 && cThread->args.test == 0)
		 || cThread->args.test == 1) {
	printf("%s test  seed %u\n",
	    (cThread->args.test == 0) ? "rand" : "dir", cThread->args.seed);
    }
    switch(cThread->args.test) {
	case 0:							// random
	    cThread->TestRandom();
	    break;
	case 1:
	    cThread->TestDirected();
	    break;
	case 2:
	    cThread->TestPerf();
	    break;
	case 3:
	    cThread->TestStats();
	    break;
	}
    return(0);
}

#include	"ThreadActions.h"

int	actionsLock = 0;
int	keyTimeouts[2] = { 0, 0 };
int	abThreads[2] = { 0, 0 };	// 0 = bin


void CThread::TestRandom()
{
    ECmd	cmdCmd;
    int		cmdKey;
    int		cmdFlag;
    int		cmdData;
    uint64_t	cmdCas;
    bool	casVal;
    ERsp	cmdExpected = eNoreply;
    uint32_t	cmdTmr = 0;
    bool	testPass;
    uint64_t	incDecData;
    uint64_t	incDecVal;
    uint64_t	incDecExp = 0;
    bool	testQuiet = 0;
    int		getQcnt;
    int		loopc;
    GetQ        gq;
    int		i, j, ab;
    uint64_t	stopTime;
    bool	keyPresent;
    int		testState;
    Actions	action;
    bool	traceIt;
    bool	keyTimeout;
    int		testCnt = 0;		// total tests

    m_genErr = false;
    m_timeCmds = false;
    if(m_tid < args.threadCntE) {
	m_genErr = true;
    } else if((m_tid + args.threadCntE) < args.threadCntT) {
	m_timeCmds = true;
    }
    if(m_genErr == false) for(i=0; i<(m_keyCnt * args.fillPct) / 100; i++) {
	m_respCheckOk = true;
	if(m_trace2) {
	    printf("%2d %s   key %3d   flag %3d   data %3d   exp %s\n",
		m_tid, cmdStr[eSetCmd], i, 0, i, cmdExpStr[eStored]);
	}
	Store(eSetCmd, i, 0, i, 0, eStored);	// sets m_gCas and m_gCasState
        if(m_respCheckOk == true) {
	    SetItemState(VALID, eSetCmd, i, 0, i, m_gCas, m_gCasState);
	} else {
	    printf("Unable to set key %d\n", i);
	}
    }
    stopTime = (args.perfTime == 0) ? 0
			: getTime() + ((uint64_t)args.perfTime * 1000000);

    ab = (m_bBinaryEnabled == true) ? 0 : 1;
    __sync_fetch_and_add(&abThreads[ab], 1);	// bump correct thread count

    while(1) {
	getTime(&m_currUsec, &m_currSec);
	if(stopTime != 0) {
	    if(m_currUsec >= stopTime) {
		break;
	    }
	} else if(args.cmdCnt-- <= 0) {
	    break;
	}
	while(m_pauseThreads == true) {
	    SLEEP
	}
	cmdCmd = getCmdRnd(m_rnd());
	m_rand32 = m_rnd();
	cmdKey = m_rand32 % m_keyCnt;		// key to use
	traceIt = (m_trace2 || cmdKey == args.traceKey) ? true : false;
	testPass = ((m_rnd() & 0x07) == 0) ? false : true;
	testQuiet = (m_rnd() & 0x01) ? true : false;
	cmdFlag = m_rnd() % m_flagCnt;	// flag to use
	cmdData = m_rnd() % m_dataCnt;	// dataset data, prep, append
	casVal = false;

	if(m_bUdpEnabled == true) testQuiet = false;
	getQcnt = m_rnd() % args.getMax;
	incDecData = m_rnd.rand64();
	incDecVal = m_rnd.rand64();
	m_randCas = ((incDecVal & 0x0f) == 0) ? 0 : incDecVal;
	m_opaque = m_rnd();
	//
	//  CheckKeyTime sets the timeout in keyState (0 for not timed)
	//  and checks if the key is being timed (sets keyTimeout if needed)
	//
	if(CheckKeyTime(cmdCmd, cmdKey, keyTimeout) == false) {
	    continue;			// near the timeout, skip
	}
	if(keyTimeout == true) {		// if timed out, invalidate key
	    SetItemState(INVALID, cmdCmd, cmdKey, 0, 0, 0, C_INV);
	    __sync_fetch_and_add(&keyTimeouts[ab], 1);
	}
	keyPresent = KeyItemPresent(cmdKey);
	if(keyPresent == true) {
	    GetKeyState(cmdKey, cmdFlag, cmdData, cmdCas, casVal);
	}
#ifdef _WIN32
	incDecData &= 0x7fffffff;
	incDecVal &= 0x7fffffff;
#endif
	m_respCheckOk = true;
	m_cmdCnt[cmdCmd]++;
	testState = 0;
	testState |= (keyPresent == true) ? 1 : 0;
	testState |= (casVal == true)     ? 2 : 0;
	testState |= (testPass == true)   ? 4 : 0;
	testState |= (testQuiet == true)  ? 8 : 0;
	m_tState = testState;
	if(m_bBinaryEnabled == true) {
	    action = BinaryActions[cmdCmd][testState];
	} else {
	    action = AsciiActions[cmdCmd][testState];
	}
	while(__sync_fetch_and_or(&actionsLock, 1) == 1) ;// lock
	testCnt++;			// total number of tests
	if(   (args.progC && (testCnt % args.progC) == 0)
	   || (args.progT && (m_currSec % args.progT) == 0) && printTime != m_currSec) {
		printTime = m_currSec;
		PrintTrace(args.progP != 0);
	}
	if(ActionsCount[ab][cmdCmd][testState] != 0xffff) {
	    ActionsCount[ab][cmdCmd][testState]++;
	}
	__sync_fetch_and_and(&actionsLock, 0);	// unlock
	
	switch(action) {
	case SKIP:
	    continue;
	case SETC:	// NK NC TP R  set
	case SETD:	// K  NC TP R  set
	    cmdCas = 0;
	    cmdExpected = eStored;
	    break;
	case SET8:	// NK NC TF R  set
	case SET0:	// NK NC TF NR  set
	    cmdCas = ~0;
	    cmdExpected = eNotFound;
	    break;
	case SET4:	// NK NC TP NR  set
	    cmdCas = 0;
	    cmdExpected = eNoreply;
	    break;
	case SET1:	// K  NC TF NR set
	    cmdCas = ~0;
	    cmdExpected = eExists;
	    break;
	case SET3:	// K  C  F  NR set
	    cmdCas++;
	    cmdExpected = eExists;
	    break;
	case SET5:	// K  NC TP NR set
	    cmdCas = 0;
	    cmdExpected = eStored;
	    break;
	case SET7:	// K  C TP NR set
	    cmdExpected = eNoreply;
	    break;
	case SET9:	// K  NC TF R set
	    cmdCas = ~0;
	    cmdExpected = eExists;
	    break;
	case SETB:	// K  C  TF R set
	    cmdCas++;
	    cmdExpected = eExists;
	    break;
	case SETF:	// K  C  TP R set
	    cmdExpected = eStored;
	    break;

	case ADD1:	// K  NC TF NR
	    cmdCas = ~0;
	    cmdExpected = eExists;
	    break;
	case ADD3:	// K  C  TF  NR
	    cmdCas++;
	    cmdExpected = eExists;
	    break;
	case ADD4:	// NK NC TP NR
	    cmdCas = 0;
	    cmdExpected = eNoreply;
	    break;
	case ADD5:	// K  NC TP NR
	    cmdCas = 0;
	    cmdExpected = eExists;
	    break;
	case ADD7:	// K  C TP NR
	    cmdExpected = eStored;		// xx ????
	    break;
	case ADD8:	// NK NC TF R
	case ADD0:	// NK NC TF NR
	    cmdCas = ~0;
	    cmdExpected = eNotFound;
	    break;
	case ADD9:	// K  NC TF R
	    cmdCas = ~0;
	    cmdExpected = eExists;
	    break;
	case ADDB:	// K  C  TF R
	    cmdCas++;
	    cmdExpected = eExists;
	    break;
	case ADDC:	// NK NC TP R
	    cmdCas = 0;
	    cmdExpected = eStored;
	    break;
	case ADDD:	// K  NC TP R
	    cmdCas = 0;
	    cmdExpected = eExists;
	    break;
	case ADDF:	// K  C  TP R
	    cmdExpected = eStored;		// xx ????
	    break;

	case REP1:	// K  NC TF NR
	    cmdCas = ~0;
	    cmdExpected = eExists;
	    break;
	case REP3:	// K  C  TF  NR
	    cmdCas++;
	    cmdExpected = eExists;
	    break;
	case REP4:	// NK NC TP NR
	    cmdCas = 0;
	    cmdExpected = eNotFound;
	    break;
	case REP5:	// K  NC TP NR
	    cmdCas = 0;
	    cmdExpected = eStored;
	    break;
	case REP7:	// K  C TP NR
	    cmdExpected = eNoreply;
	    break;
	case REP8:	// NK NC TF R
	case REP0:	// NK NC TF NR
	    cmdCas = ~0;
	    cmdExpected = eNotFound;
	    break;
	case REP9:	// K  NC TF R
	    cmdCas = ~0;
	    cmdExpected = eExists;
	    break;
	case REPB:	// K  C  TF R
	    cmdCas++;
	    cmdExpected = eExists;
	    break;
	case REPC:	// NK NC TP R
	    cmdCas = 0;
	    cmdExpected = eNotFound;
	    break;
	case REPD:	// K  NC TP R
	    cmdCas = 0;
	    cmdExpected = eStored;
	    break;
	case REPF:	// K  C  TP R
	    cmdExpected = eStored;
	    break;

	case APP1:	// K  NC TF NR
	    cmdCas = ~0;
	    cmdExpected = eExists;
	    break;
	case APP3:	// K  C  TF  NR
	    cmdCas++;
	    cmdExpected = eExists;
	    break;
	case APP4:	// NK NC TP NR
	    cmdCas = 0;
	    cmdExpected = eNotStored;
	    break;
	case APP5:	// K  NC TP NR
	    if(m_keyState[cmdKey].m_appIdx != -1) continue;
	    cmdCas = 0;
	    cmdExpected = eStored;
	    break;
	case APP7:	// K  C TP NR
	    if(m_keyState[cmdKey].m_appIdx != -1) continue;
	    cmdExpected = eNoreply;
	    break;
	case APP8:	// NK NC TF R
	case APP0:	// NK NC TF NR
	    cmdCas = ~0;
	    cmdExpected = eNotStored;
	    break;
	case APP9:	// K  NC TF R
	    cmdCas = ~0;
	    cmdExpected = eExists;
	    break;
	case APPB:	// K  C  TF R
	    cmdCas++;
	    cmdExpected = eExists;
	    break;
	case APPC:	// NK NC TP R
	    cmdCas = 0;
	    cmdExpected = eNotStored;
	    break;
	case APPD:	// K  NC TP R
	    if(m_keyState[cmdKey].m_appIdx != -1) continue;
	    cmdCas = 0;
	    cmdExpected = eStored;
	    break;
	case APPF:	// K  C  TP R
	    if(m_keyState[cmdKey].m_appIdx != -1) continue;
	    cmdExpected = eStored;
	    break;

	case PRE1:	// K  NC TF NR
	    cmdCas = ~0;
	    cmdExpected = eExists;
	    break;
	case PRE3:	// K  C  TF  NR
	    cmdCas++;
	    cmdExpected = eExists;
	    break;
	case PRE4:	// NK NC TP NR
	    cmdCas = 0;
	    cmdExpected = eNotStored;
	    break;
	case PRE5:	// K  NC TP NR
	    if(m_keyState[cmdKey].m_preIdx != -1) continue;
	    cmdCas = 0;
	    cmdExpected = eStored;
	    break;
	case PRE7:	// K  C TP NR
	    if(m_keyState[cmdKey].m_preIdx != -1) continue;
	    cmdExpected = eNoreply;
	    break;
	case PRE8:	// NK NC TF R
	case PRE0:	// NK NC TF NR
	    cmdCas = ~0;
	    cmdExpected = eNotStored;
	    break;
	case PRE9:	// K  NC TF R
	    cmdCas = ~0;
	    cmdExpected = eExists;
	    break;
	case PREB:	// K  C  TF R
	    cmdCas++;
	    cmdExpected = eExists;
	    break;
	case PREC:	// NK NC TP R
	    cmdCas = 0;
	    cmdExpected = eNotStored;
	    break;
	case PRED:	// K  NC TP R
	    if(m_keyState[cmdKey].m_preIdx != -1) continue;
	    cmdCas = 0;
	    cmdExpected = eStored;
	    break;
	case PREF:	// K  C  TP R
	    if(m_keyState[cmdKey].m_preIdx != -1) continue;
	    cmdExpected = eStored;
	    break;

	case CAS1:	// K  NC TF NR
	    cmdCas = ~0;
	    cmdExpected = eExists;
	    break;
	case CAS3:	// K  C  TF  NR
	    cmdCas++;
	    cmdExpected = eExists;
	    break;
	case CAS4:	// NK NC TP NR
	    cmdCas = 0;
	    cmdExpected = eStored;
	    break;
	case CAS5:	// K  NC TP NR
	    cmdCas = 0;
	    cmdExpected = eStored;
	    break;
	case CAS7:	// K  C TP NR
	    cmdExpected = eNoreply;
	    break;
	case CAS8:	// NK NC TF R
	case CAS0:	// NK NC TF NR
	    cmdCas = ~0;
	    cmdExpected = eNotFound;
	    break;
	case CAS9:	// K  NC TF R
	    cmdCas = ~0;
	    cmdExpected = eExists;
	    break;
	case CASB:	// K  C  TF R
	    cmdCas++;
	    cmdExpected = eExists;
	    break;
	case CASC:	// NK NC TP R
	    cmdCas = 0;
	    cmdExpected = eStored;
	    break;
	case CASD:	// K  NC TP R
	    cmdCas = 0;
	    cmdExpected = eStored;
	    break;
	case CASF:	// K  C  TP R
	    cmdExpected = eStored;
	    break;

	case GET1:	// K  NC TF NR
	    cmdCas = ~0;
	    cmdExpected = eGetHit;
	    break;
	case GET3:	// K  C  TF  NR
	    cmdCas++;
	    cmdExpected = eGetHit;
	    break;
	case GET4:	// NK NC TP NR
	    cmdCas = 0;
	    cmdExpected = eNotFound;
	    break;
	case GET5:	// K  NC TP NR
	    cmdCas = 0;
	    cmdExpected = eGetHit;
	    break;
	case GET7:	// K  C TP NR
	    cmdExpected = eGetHit;
	    break;
	case GET8:	// NK NC TF R
	case GET0:	// NK NC TF NR
	    cmdCas = ~0;
	    cmdExpected = eNotFound;
	    break;
	case GET9:	// K  NC TF R
	    cmdCas = ~0;
	    cmdExpected = eGetHit;
	    break;
	case GETB:	// K  C  TF R
	    cmdCas++;
	    cmdExpected = eGetHit;
	    break;
	case GETC:	// NK NC TP R
	    cmdCas = 0;
	    cmdExpected = eNotFound;
	    break;
	case GETD:	// K  NC TP R
	    cmdCas = 0;
	    cmdExpected = eGetHit;
	    break;
	case GETF:	// K  C  TP R
	    cmdExpected = eGetHit;
	    break;

	case GTS0: case GTS1: case GTS2: case GTS3: case GTS4:
	case GTS5: case GTS6: case GTS7: case GTS8: case GTS9:
	case GTSA: case GTSB: case GTSC: case GTSD: case GTSE: case GTSF:
	    continue;		// gets only in ascii

	case GTK1:	// K  NC TF NR
	    cmdCas = ~0;
	    cmdExpected = eGetHit;
	    break;
	case GTK3:	// K  C  TF  NR
	    cmdCas++;
	    cmdExpected = eGetHit;
	    break;
	case GTK4:	// NK NC TP NR
	    cmdCas = 0;
	    cmdExpected = eNotFound;
	    break;
	case GTK5:	// K  NC TP NR
	    cmdCas = 0;
	    cmdExpected = eGetHit;
	    break;
	case GTK7:	// K  C TP NR
	    cmdExpected = eGetHit;
	    break;
	case GTK8:	// NK NC TF R
	case GTK0:	// NK NC TF NR
	    cmdCas = ~0;
	    cmdExpected = eNotFound;
	    break;
	case GTK9:	// K  NC TF R
	    cmdCas = ~0;
	    cmdExpected = eGetHit;
	    break;
	case GTKB:	// K  C  TF R
	    cmdCas++;
	    cmdExpected = eGetHit;
	    break;
	case GTKC:	// NK NC TP R
	    cmdCas = 0;
	    cmdExpected = eNotFound;
	    break;
	case GTKD:	// K  NC TP R
	    cmdCas = 0;
	    cmdExpected = eGetHit;
	    break;
	case GTKF:	// K  C  TP R
	    cmdExpected = eGetHit;
	    break;

	case GTR0: case GTR1: case GTR2: case GTR3: case GTR4:
	case GTR5: case GTR6: case GTR7: case GTR8: case GTR9:
	case GTRA: case GTRB: case GTRC: case GTRD: case GTRE: case GTRF:
	    if(m_timeCmds == true) {
		continue;		// no timeouts
	    }
	    break;

	case GAT1:	// K  NC TF NR
	    cmdCas = ~0;
	    cmdExpected = eGetHit;
	    break;
	case GAT3:	// K  C  TF  NR
	    cmdCas++;
	    cmdExpected = eGetHit;
	    break;
	case GAT4:	// NK NC TP NR
	    cmdCas = 0;
	    cmdExpected = eNotFound;
	    break;
	case GAT5:	// K  NC TP NR
	    cmdCas = 0;
	    cmdExpected = eGetHit;
	    break;
	case GAT7:	// K  C TP NR
	    cmdExpected = eGetHit;
	    break;
	case GAT8:	// NK NC TF R
	case GAT0:	// NK NC TF NR
	    cmdCas = ~0;
	    cmdExpected = eNotFound;
	    break;
	case GAT9:	// K  NC TF R
	    cmdCas = ~0;
	    cmdExpected = eGetHit;
	    break;
	case GATB:	// K  C  TF R
	    cmdCas++;
	    cmdExpected = eGetHit;
	    break;
	case GATC:	// NK NC TP R
	    cmdCas = 0;
	    cmdExpected = eNotFound;
	    break;
	case GATD:	// K  NC TP R
	    cmdCas = 0;
	    cmdExpected = eGetHit;
	    break;
	case GATF:	// K  C  TP R
	    cmdExpected = eGetHit;
	    break;

	case INC5: case DEC5:	// K  NC TP NR
	case INCD: case DECD:	// K  NC TP R
	    cmdCas = 0;		// force the store (and new cas)
	case INCF: case DECF:	// K  C  TP R
	case INC7: case DEC7:	// K  C  TP NR
	    cmdExpected = eValue;
//
// xx ??? fixme
// if the most siginificant bit of the memory value is set
// you will get a inc/dec/non-numeric error on some cases (but not all)
// need the source to figure it out
//
	    incDecData &= 0x7fffffffffffffff;
	    if(traceIt) {
		printf("%2d %s    key %3d   flag %3d              "
			"exp %s MV %18lx\n",
			m_tid,
			(cmdCmd == eIncrCmd) ? "IncrStore" : "DecrStore",
			cmdKey, cmdFlag, cmdExpStr[cmdExpected], incDecData);
	    }
	    m_cmdCmd = eSetCmd;
	    ClearKeyTimeout(cmdKey);	// no timeout on store
	    Store(eSetCmd, cmdKey, cmdFlag, incDecData, cmdCas, eStored);
	    if(m_respCheckOk == true) {
		SetItemState(VALID, cmdCmd, cmdKey, cmdFlag, cmdData,
				m_gCas, m_gCasState);
		cmdCas = m_gCas;
		if(cmdCmd == eIncrCmd) {
		    incDecExp = incDecVal + incDecData;
		} else {
		    if(incDecVal > incDecData) {
			incDecExp = 0;	// force answer to 0
		    } else {
			incDecExp = incDecData - incDecVal;
		    }
		}
	    } else {
		ErrorMsg("Inc Dec set failure", 0, 0);
	    }
	    break;
	case INC8: case DEC8:	// NK NC TF R
	case INC0: case DEC0:	// NK NC TF NR
	    cmdCas = m_gCas = ~0;
	    cmdTmr = 0xffffffff;		// force fail
	    cmdExpected = eNotFound;
	    incDecExp = incDecVal;		// will seed with this
	    incDecData = 0;			// make the print reasonable
	    break;
	case INC1: case DEC1:	// K  NC TF NR
	case INC9: case DEC9:	// K  NC TF R
	    cmdCas = m_gCas = ~0;
	    cmdExpected = eExists;		// bad cas
	    break;
	case INC3: case DEC3:	// K  C  TF  NR
	case INCB: case DECB:	// K  C  TF R
	    m_gCas = cmdCas;
	    cmdExpected = eNonNumeric;		// inc/dec non numeric
	    break;
	case INC4: case DEC4:	// NK NC TP NR
	case INCC: case DECC:	// NK NC TP R
	    cmdCas = m_gCas = ~0;
	    cmdTmr = 0;
	    cmdExpected = eValue;
	    incDecExp = incDecVal;		// will seed with this
	    incDecData = 0;			// make the print reasonable
	    break;

	case DEL1:	// K  NC TF NR
	    cmdCas = ~0;
	    cmdExpected = eExists;
	    break;
	case DEL3:	// K  C  TF  NR
	    cmdCas++;
	    cmdExpected = eExists;
	    break;
	case DEL4:	// NK NC TP NR
	    cmdCas = 0;
	    cmdExpected = eNotFound;
	    break;
	case DEL5:	// K  NC TP NR
	    cmdCas = 0;
	    cmdExpected = eDeleted;
	    break;
	case DEL7:	// K  C TP NR
	    cmdExpected = eNoreply;
	    break;
	case DEL8:	// NK NC TF R
	case DEL0:	// NK NC TF NR
	    cmdCas = ~0;
	    cmdExpected = eNotFound;
	    break;
	case DEL9:	// K  NC TF R
	    cmdCas = ~0;
	    cmdExpected = eExists;
	    break;
	case DELB:	// K  C  TF R
	    cmdCas++;
	    cmdExpected = eExists;
	    break;
	case DELC:	// NK NC TP R
	    cmdCas = 0;
	    cmdExpected = eNotFound;
	    break;
	case DELD:	// K  NC TP R
	    cmdCas = 0;
	    cmdExpected = eDeleted;
	    break;
	case DELF:	// K  C  TP R
	    cmdExpected = eDeleted;
	    break;

	case TCH1:	// K  NC TF NR
	    cmdCas = ~0;
	    cmdExpected = eTouched;
	    break;
	case TCH3:	// K  C  TF  NR
	    cmdCas++;
	    cmdExpected = eTouched;
	    break;
	case TCH4:	// NK NC TP NR
	    cmdCas = 0;
	    cmdExpected = eNotFound;
	    break;
	case TCH5:	// K  NC TP NR
	    cmdCas = 0;
	    cmdExpected = eTouched;
	    break;
	case TCH7:	// K  C TP NR
	    cmdExpected = eTouched;
	    break;
	case TCH8:	// NK NC TF R
	case TCH0:	// NK NC TF NR
	    cmdCas = ~0;
	    cmdExpected = eNotFound;
	    break;
	case TCH9:	// K  NC TF R
	    cmdCas = ~0;
	    cmdExpected = eTouched;
	    break;
	case TCHB:	// K  C  TF R
	    cmdCas++;
	    cmdExpected = eTouched;
	    break;
	case TCHC:	// NK NC TP R
	    cmdCas = 0;
	    cmdExpected = eNotFound;
	    break;
	case TCHD:	// K  NC TP R
	    cmdCas = 0;
	    cmdExpected = eTouched;
	    break;
	case TCHF:	// K  C  TP R
	    cmdExpected = eTouched;
	    break;

	case STS1:	// K  NC TF NR
	    cmdCas = ~0;
	    cmdExpected = eTouched;
	    break;
	case STS3:	// K  C  TF  NR
	    cmdCas++;
	    cmdExpected = eTouched;
	    break;
	case STS4:	// NK NC TP NR
	    cmdCas = 0;
	    cmdExpected = eNotFound;
	    break;
	case STS5:	// K  NC TP NR
	    cmdCas = 0;
	    cmdExpected = eTouched;
	    break;
	case STS7:	// K  C TP NR
	    cmdExpected = eTouched;
	    break;
	case STS8:	// NK NC TF R
	case STS0:	// NK NC TF NR
	    cmdCas = ~0;
	    cmdExpected = eNotFound;
	    break;
	case STS9:	// K  NC TF R
	    cmdCas = ~0;
	    cmdExpected = eTouched;
	    break;
	case STSB:	// K  C  TF R
	    cmdCas++;
	    cmdExpected = eTouched;
	    break;
	case STSC:	// NK NC TP R
	    cmdCas = 0;
	    cmdExpected = eNotFound;
	    break;
	case STSD:	// K  NC TP R
	    cmdCas = 0;
	    cmdExpected = eTouched;
	    break;
	case STSF:	// K  C  TP R
	    cmdExpected = eTouched;
	    break;

	case VER1:	// K  NC TF NR
	    cmdCas = ~0;
	    cmdExpected = eTouched;
	    break;
	case VER3:	// K  C  TF  NR
	    cmdCas++;
	    cmdExpected = eTouched;
	    break;
	case VER4:	// NK NC TP NR
	    cmdCas = 0;
	    cmdExpected = eNotFound;
	    break;
	case VER5:	// K  NC TP NR
	    cmdCas = 0;
	    cmdExpected = eTouched;
	    break;
	case VER7:	// K  C TP NR
	    cmdExpected = eTouched;
	    break;
	case VER8:	// NK NC TF R
	case VER0:	// NK NC TF NR
	    cmdCas = ~0;
	    cmdExpected = eNotFound;
	    break;
	case VER9:	// K  NC TF R
	    cmdCas = ~0;
	    cmdExpected = eTouched;
	    break;
	case VERB:	// K  C  TF R
	    cmdCas++;
	    cmdExpected = eTouched;
	    break;
	case VERC:	// NK NC TP R
	    cmdCas = 0;
	    cmdExpected = eNotFound;
	    break;
	case VERD:	// K  NC TP R
	    cmdCas = 0;
	    cmdExpected = eTouched;
	    break;
	case VERF:	// K  C  TP R
	    cmdExpected = eTouched;
	    break;

	case NOP1:	// K  NC TF NR
	    cmdCas = ~0;
	    cmdExpected = eTouched;
	    break;
	case NOP3:	// K  C  TF  NR
	    cmdCas++;
	    cmdExpected = eTouched;
	    break;
	case NOP4:	// NK NC TP NR
	    cmdCas = 0;
	    cmdExpected = eNotFound;
	    break;
	case NOP5:	// K  NC TP NR
	    cmdCas = 0;
	    cmdExpected = eTouched;
	    break;
	case NOP7:	// K  C TP NR
	    cmdExpected = eTouched;
	    break;
	case NOP8:	// NK NC TF R
	case NOP0:	// NK NC TF NR
	    cmdCas = ~0;
	    cmdExpected = eNotFound;
	    break;
	case NOP9:	// K  NC TF R
	    cmdCas = ~0;
	    cmdExpected = eTouched;
	    break;
	case NOPB:	// K  C  TF R
	    cmdCas++;
	    cmdExpected = eTouched;
	    break;
	case NOPC:	// NK NC TP R
	    cmdCas = 0;
	    cmdExpected = eNotFound;
	    break;
	case NOPD:	// K  NC TP R
	    cmdCas = 0;
	    cmdExpected = eTouched;
	    break;
	case NOPF:	// K  C  TP R
	    cmdExpected = eTouched;
	    break;
	//
	// ascii versions
	//
	case ASET0:	// NK NC TF NR
	case ASET1:	// K  NC TF NR
	case ASET3:	// K  C  TF NR
	case ASET4:	// NK NC TP NR
	case ASET5:	// K  NC TP NR
	case ASET7:	// K  C  TP NR
	    cmdExpected = eNoreply;
	    break;
	case ASET8:	// NK NC TF R
	case ASET9:	// K  NC TF R
	case ASETB:	// K  C  TF R
	case ASETC:	// NK NC TP R
	case ASETD:	// K  NC TP R
	case ASETF:	// K  C  TP R
	    cmdExpected = eStored;
	    break;

	case AADD0:	// NK NC TF NR
	case AADD4:	// NK NC TP NR
	    cmdExpected = eNoreply;
	    break;
	case AADD8:	// NK NC TF R
	case AADDC:	// NK NC TP R
	    cmdExpected = eStored;
	    break;
	case AADD1:	// K  NC TF NR
	case AADD3:	// K  C  TF NR
	case AADD5:	// K  NC TP NR
	case AADD7:	// K  C  TP NR
	case AADD9:	// K  NC TF R
	case AADDB:	// K  C  TF R
	case AADDD:	// K  NC TP R
	case AADDF:	// K  C  TP R
	    cmdExpected = eNotStored;
	    break;

	case AREP0:	// NK NC TF NR
	case AREP4:	// NK NC TP NR
	case AREP8:	// NK NC TF R
	case AREPC:	// NK NC TP R
	    cmdExpected = eNotStored;
	    break;
	case AREP1:	// K  NC TF NR
	case AREP3:	// K  C  TF NR
	case AREP5:	// K  NC TP NR
	case AREP7:	// K  C  TP NR
	    cmdExpected = eNoreply;
	    break;
	case AREP9:	// K  NC TF R
	case AREPB:	// K  C  TF R
	case AREPD:	// K  NC TP R
	case AREPF:	// K  C  TP R
	    cmdExpected = eStored;
	    break;

	case AAPP0:	// NK NC TF NR
	case AAPP4:	// NK NC TP NR
	case AAPP8:	// NK NC TF R
	case AAPPC:	// NK NC TP R
	    cmdExpected = eNotStored;
	    break;
	case AAPP1:	// K  NC TF NR
	case AAPP3:	// K  C  TF NR
	case AAPP5:	// K  NC TP NR
	case AAPP7:	// K  C  TP NR
	    if(m_keyState[cmdKey].m_appIdx != -1) continue;
	    cmdExpected = eNoreply;
	    break;
	case AAPP9:	// K  NC TF R
	case AAPPB:	// K  C  TF R
	case AAPPD:	// K  NC TP R
	case AAPPF:	// K  C  TP R
	    if(m_keyState[cmdKey].m_appIdx != -1) continue;
	    cmdExpected = eStored;
	    break;

	case APRE0:	// NK NC TF NR
	case APRE4:	// NK NC TP NR
	case APRE8:	// NK NC TF R
	case APREC:	// NK NC TP R
	    cmdExpected = eNotStored;
	    break;
	case APRE1:	// K  NC TF NR
	case APRE3:	// K  C  TF NR
	case APRE5:	// K  NC TP NR
	case APRE7:	// K  C  TP NR
	    if(m_keyState[cmdKey].m_preIdx != -1) continue;
	    cmdExpected = eNoreply;
	    break;
	case APRE9:	// K  NC TF R
	case APREB:	// K  C  TF R
	case APRED:	// K  NC TP R
	case APREF:	// K  C  TP R
	    if(m_keyState[cmdKey].m_preIdx != -1) continue;
	    cmdExpected = eStored;
	    break;

	case ACAS0:	// NK NC TF NR
	case ACAS4:	// NK NC TP NR
	case ACAS8:	// NK NC TF R
	case ACASC:	// NK NC TP R
	    cmdExpected = eNotFound;
	    break;
	case ACAS1:	// K  NC TF NR
	case ACAS5:	// K  NC TP NR
	case ACAS9:	// K  NC TF R
	case ACASD:	// K  NC TP R
	    cmdCas = ~0;// force cas failure
	    cmdExpected = eExists;
	    break;
	case ACAS3:	// K  C  TF NR
	case ACAS7:	// K  C  TP NR
	    cmdExpected = eNoreply;
	    break;
	case ACASB:	// K  C  TF R
	case ACASF:	// K  C  TP R
	    cmdExpected = eStored;
	    break;

	case AGET0:	// NK NC TF NR
	case AGET4:	// NK NC TP NR
	case AGET8:	// NK NC TF R
	case AGETC:	// NK NC TP R
	    cmdExpected = eGetMiss;
	    break;
	case AGET1:	// K  NC TF NR
	case AGET5:	// K  NC TP NR
	case AGET9:	// K  NC TF R
	case AGETD:	// K  NC TP R
	    cmdExpected = eGetHit;
	    break;
	case AGET3:	// K  C  TF NR
	case AGET7:	// K  C  TP NR
	    cmdExpected = eGetHit;
	    break;
	case AGETB:	// K  C  TF R
	case AGETF:	// K  C  TP R
	    cmdExpected = eGetHit;
	    break;

	case AGTS0:	// NK NC TF NR
	case AGTS4:	// NK NC TP NR
	case AGTS8:	// NK NC TF R
	case AGTSC:	// NK NC TP R
	    cmdExpected = eGetMiss;
	    break;
	case AGTS1:	// K  NC TF NR
	case AGTS5:	// K  NC TP NR
	case AGTS9:	// K  NC TF R
	case AGTSD:	// K  NC TP R
	    cmdExpected = eGetHit;
	    break;
	case AGTS3:	// K  C  TF NR
	case AGTS7:	// K  C  TP NR
	    cmdExpected = eGetHit;
	    break;
	case AGTSB:	// K  C  TF R
	case AGTSF:	// K  C  TP R
	    cmdExpected = eGetHit;
	    break;

	case AGTK0: case AGTK1: case AGTK2: case AGTK3: case AGTK4:
	case AGTK5: case AGTK6: case AGTK7: case AGTK8: case AGTK9:
	case AGTKA: case AGTKB: case AGTKC: case AGTKD: case AGTKE: case AGTKF:
	    continue;	// no gtk in ascii

	case AGTR0: case AGTR1: case AGTR2: case AGTR3: case AGTR4:
	case AGTR5: case AGTR6: case AGTR7: case AGTR8: case AGTR9:
	case AGTRA: case AGTRB: case AGTRC: case AGTRD: case AGTRE: case AGTRF:
	    continue;	// no gtr in ascii

	case AGAT0: case AGAT1: case AGAT2: case AGAT3: case AGAT4:
	case AGAT5: case AGAT6: case AGAT7: case AGAT8: case AGAT9:
	case AGATA: case AGATB: case AGATC: case AGATD: case AGATE: case AGATF:
	    continue;	// no gat in ascii

	case AINC0: case ADEC0:	// NK NC TF NR
	case AINC4: case ADEC4:	// NK NC TP NR
	case AINC8: case ADEC8:	// NK NC TF R
	case AINCC: case ADECC:	// NK NC TP R
	    cmdExpected = eNotFound;
	    break;
	case AINC1: case ADEC1:	// K  NC TF NR
	case AINC3: case ADEC3:	// K  C  TF NR
	case AINC9: case ADEC9:	// K  NC TF R
	case AINCB: case ADECB:	// K  C  TF R
	    cmdExpected = eNonNumeric;
	    break;
	case AINC5: case ADEC5:	// K  NC TP NR
	case AINC7: case ADEC7:	// K  C  TP NR
	case AINCD: case ADECD:	// K  NC TP R
	case AINCF: case ADECF:	// K  C  TP R
	    continue;
	    break;

	case ADEL0:	// NK NC TF NR
	case ADEL4:	// NK NC TP NR
	case ADEL8:	// NK NC TF R
	case ADELC:	// NK NC TP R
	    cmdExpected = eNotFound;
	    break;

	case ADEL1:	// K  NC TF NR
	case ADEL3:	// K  C  TF NR
	case ADEL5:	// K  NC TP NR
	case ADEL7:	// K  C  TP NR
	    cmdExpected = eNoreply;
	    break;
	case ADEL9:	// K  NC TF R
	case ADELB:	// K  C  TF R
	case ADELD:	// K  NC TP R
	case ADELF:	// K  C  TP R
	    cmdExpected = eDeleted;
	    break;

	case ATCH1:	// K  NC TF NR
	    cmdCas = ~0;
	    cmdExpected = eTouched;
	    break;
	case ATCH3:	// K  C  TF  NR
	    cmdCas++;
	    cmdExpected = eTouched;
	    break;
	case ATCH4:	// NK NC TP NR
	    cmdCas = 0;
	    cmdExpected = eNotFound;
	    break;
	case ATCH5:	// K  NC TP NR
	    cmdCas = 0;
	    cmdExpected = eTouched;
	    break;
	case ATCH7:	// K  C TP NR
	    cmdExpected = eTouched;
	    break;
	case ATCH8:	// NK NC TF R
	case ATCH0:	// NK NC TF NR
	    cmdCas = ~0;
	    cmdExpected = eNotFound;
	    break;
	case ATCH9:	// K  NC TF R
	    cmdCas = ~0;
	    cmdExpected = eTouched;
	    break;
	case ATCHB:	// K  C  TF R
	    cmdCas++;
	    cmdExpected = eTouched;
	    break;
	case ATCHC:	// NK NC TP R
	    cmdCas = 0;
	    cmdExpected = eNotFound;
	    break;
	case ATCHD:	// K  NC TP R
	    cmdCas = 0;
	    cmdExpected = eTouched;
	    break;
	case ATCHF:	// K  C  TP R
	    cmdExpected = eTouched;
	    break;

	case ASTS1:	// K  NC TF NR
	    cmdCas = ~0;
	    cmdExpected = eTouched;
	    break;
	case ASTS3:	// K  C  TF  NR
	    cmdCas++;
	    cmdExpected = eTouched;
	    break;
	case ASTS4:	// NK NC TP NR
	    cmdCas = 0;
	    cmdExpected = eNotFound;
	    break;
	case ASTS5:	// K  NC TP NR
	    cmdCas = 0;
	    cmdExpected = eTouched;
	    break;
	case ASTS7:	// K  C TP NR
	    cmdExpected = eTouched;
	    break;
	case ASTS8:	// NK NC TF R
	case ASTS0:	// NK NC TF NR
	    cmdCas = ~0;
	    cmdExpected = eNotFound;
	    break;
	case ASTS9:	// K  NC TF R
	    cmdCas = ~0;
	    cmdExpected = eTouched;
	    break;
	case ASTSB:	// K  C  TF R
	    cmdCas++;
	    cmdExpected = eTouched;
	    break;
	case ASTSC:	// NK NC TP R
	    cmdCas = 0;
	    cmdExpected = eNotFound;
	    break;
	case ASTSD:	// K  NC TP R
	    cmdCas = 0;
	    cmdExpected = eTouched;
	    break;
	case ASTSF:	// K  C  TP R
	    cmdExpected = eTouched;
	    break;

	case AVER1:	// K  NC TF NR
	    cmdCas = ~0;
	    cmdExpected = eTouched;
	    break;
	case AVER3:	// K  C  TF  NR
	    cmdCas++;
	    cmdExpected = eTouched;
	    break;
	case AVER4:	// NK NC TP NR
	    cmdCas = 0;
	    cmdExpected = eNotFound;
	    break;
	case AVER5:	// K  NC TP NR
	    cmdCas = 0;
	    cmdExpected = eTouched;
	    break;
	case AVER7:	// K  C TP NR
	    cmdExpected = eTouched;
	    break;
	case AVER8:	// NK NC TF R
	case AVER0:	// NK NC TF NR
	    cmdCas = ~0;
	    cmdExpected = eNotFound;
	    break;
	case AVER9:	// K  NC TF R
	    cmdCas = ~0;
	    cmdExpected = eTouched;
	    break;
	case AVERB:	// K  C  TF R
	    cmdCas++;
	    cmdExpected = eTouched;
	    break;
	case AVERC:	// NK NC TP R
	    cmdCas = 0;
	    cmdExpected = eNotFound;
	    break;
	case AVERD:	// K  NC TP R
	    cmdCas = 0;
	    cmdExpected = eTouched;
	    break;
	case AVERF:	// K  C  TP R
	    cmdExpected = eTouched;
	    break;

	case ANOP1:	// K  NC TF NR
	    cmdCas = ~0;
	    cmdExpected = eTouched;
	    break;
	case ANOP3:	// K  C  TF  NR
	    cmdCas++;
	    cmdExpected = eTouched;
	    break;
	case ANOP4:	// NK NC TP NR
	    cmdCas = 0;
	    cmdExpected = eNotFound;
	    break;
	case ANOP5:	// K  NC TP NR
	    cmdCas = 0;
	    cmdExpected = eTouched;
	    break;
	case ANOP7:	// K  C TP NR
	    cmdExpected = eTouched;
	    break;
	case ANOP8:	// NK NC TF R
	case ANOP0:	// NK NC TF NR
	    cmdCas = ~0;
	    cmdExpected = eNotFound;
	    break;
	case ANOP9:	// K  NC TF R
	    cmdCas = ~0;
	    cmdExpected = eTouched;
	    break;
	case ANOPB:	// K  C  TF R
	    cmdCas++;
	    cmdExpected = eTouched;
	    break;
	case ANOPC:	// NK NC TP R
	    cmdCas = 0;
	    cmdExpected = eNotFound;
	    break;
	case ANOPD:	// K  NC TP R
	    cmdCas = 0;
	    cmdExpected = eTouched;
	    break;
	case ANOPF:	// K  C  TP R
	    cmdExpected = eTouched;
	    break;

	default:
	    printf("Bad State Cmd %s   State 0x%02x  Action 0x%04x\n",
			cmdStr[cmdCmd], testState, action);
	    assert(0);
	    break;
	}
	//
	//  now send the command
	//
	m_cmdCmd = cmdCmd;
	switch(cmdCmd) {
	    case eSetCmd: case eAddCmd: case eReplaceCmd:
	    case eAppendCmd: case ePrependCmd:	// set cmds
	    case eCasCmd:				// cas
		if(traceIt) {
		    printf("%2d %s   key %3d   flag %3d   data %3d   exp %s   "
			"0x%lx (%ld)\n",
			m_tid, cmdStr[cmdCmd], cmdKey, cmdFlag, cmdData,
			cmdExpStr[cmdExpected], cmdCas, cmdCas);
		}
		//
		//  if noreply, m_gCasState will be false
		//
		Store(cmdCmd, cmdKey, cmdFlag, cmdData, cmdCas, cmdExpected);
		if(m_respCheckOk == true
			&& (cmdExpected == eStored || cmdExpected == eNoreply)){
		    SetItemState(VALID, cmdCmd, cmdKey, cmdFlag, cmdData,
			m_gCas, m_gCasState);		// set state of key
		}
		break;
            case eGetCmd: case eGetsCmd: case eGetkCmd: case eGatCmd:// get cmds
		if(traceIt) {
		    printf("%2d %s   key %3d   flag %3d   data %3d   exp %s\n",
			m_tid, cmdStr[cmdCmd], cmdKey, cmdFlag, cmdData,
			cmdExpStr[cmdExpected]);
		}
		Get(cmdCmd, cmdKey, cmdFlag, cmdData, cmdExpected, cmdCas);
		if(m_respCheckOk == true && cmdExpected == eGetHit) {
		    //
		    // set cas state
		    //
		    SetItemState(VALID, eCmdCnt, cmdKey, cmdFlag, cmdData,
			m_gCas, m_gCasState);		// set state of key
		}
		break;
	    case eIncrCmd:
	    case eDecrCmd:
		if(traceIt) {
		    if(cmdExpected == eValue) {
			printf("%2d %s   key %3d   flag %3d              "
			    "exp %s\n%18lx  %c  %18lx  =  %18lx\n",
			    m_tid, cmdStr[cmdCmd], cmdKey, cmdFlag,
			    cmdExpStr[cmdExpected],
			    incDecData,
			    (cmdCmd == eIncrCmd) ? '+' : '-',
			    incDecVal, incDecExp);
		    } else {
			printf("%2d %s   key %3d   flag %3d              "
			    "exp %s ID %18lx\n",
			    m_tid, cmdStr[cmdCmd], cmdKey, cmdFlag,
			    cmdExpStr[cmdExpected], incDecVal);
		    }
		}
		m_gCas = cmdCas;
		if(m_bBinaryEnabled == true
				&& (cmdTmr == 0 || cmdTmr == 0xffffffff)) {
		    SetKeyNextTimeout(cmdKey, cmdTmr);
		}
		Arith(cmdCmd, cmdKey, incDecVal, incDecExp, cmdTmr,
			cmdExpected, cmdCas);
		if(cmdExpected == eValue || cmdExpected == eNoreply) {
		    if(traceIt) {
			printf("%2d %s      key %3d                         "
			    "exp %s\n",
			    m_tid, (cmdCmd == eIncrCmd) ? "IncrDel" : "DecrDel",
			    cmdKey, cmdExpStr[eDeleted]);
		    }
		    loopc = 4;
		    while(loopc-- > 0) {
			Delete(cmdKey, eDeleted, 0);	// 4 tries 
			if(m_respCheckOk == true) {
			    break;
			}
		    }
		    SetItemState(INVALID, cmdCmd, cmdKey, 0, 0, 0, C_INV);
		}
		break;
	    case eDelCmd:			// del
		if(traceIt) {
		    printf("%2d %s   key %3d                         exp %s\n",
			m_tid, cmdStr[cmdCmd], cmdKey, cmdExpStr[cmdExpected]);
		}
		loopc = 4;
		while(loopc-- > 0) {
		    Delete(cmdKey, cmdExpected, cmdCas);	// 4 tries 
		    if(m_respCheckOk == true) {
			break;
		    }
		}
		if(cmdExpected == eDeleted || cmdExpected == eNoreply) {
		    SetItemState(INVALID, cmdCmd, cmdKey, 0, 0, 0, C_INV);
		}
		break;
	    case eTouchCmd:
		if(traceIt) {
		    printf("%2d %s   key %3d                         exp %s\n",
			m_tid, cmdStr[cmdCmd], cmdKey, cmdExpStr[cmdExpected]);
		}
		Touch(cmdKey , cmdExpected, cmdCas);
		break;
	    case eStatsCmd:
		if(traceIt) {
		    printf("%2d %s\n", m_tid, cmdStr[cmdCmd]);
		}
 		Stats();
		break;
	    case eVersionCmd:
		if(traceIt) {
		    printf("%2d %s\n", m_tid, cmdStr[cmdCmd]);
		}
 		Version();
		break;
	    case eNoopCmd:
		if(traceIt) {
		    printf("%2d %s\n", m_tid, cmdStr[cmdCmd]);
		}
 		Noop();
		break;
	    case eGetrCmd:
		if(m_genErr == true) {
		    continue;			// no getQ in error test
		}
		getQcnt++;			// 0-4 to 1-5
		if(m_bBinaryEnabled == true) {
		    getQcnt = 1;
		}
		if(traceIt) {
		    printf("%2d %s for %d loops\n",
			m_tid, cmdStr[cmdCmd], getQcnt);
		}

		for(int j=0; j<getQcnt; j++) {
		    cmdCmd = eGetCmd;			// use get command
		    gq.cmdKey = cmdKey = m_rnd() % m_keyCnt;	// key to use
		    gq.cmdExpected = (KeyItemPresent(cmdKey) == true)
						? eGetHit : eGetMiss;
		    if(gq.cmdExpected == eGetHit) {
			GetKeyState(cmdKey, cmdFlag, cmdData, cmdCas, casVal);
			gq.cmdFlag = cmdFlag;		// flag to use
			gq.cmdData = cmdData;		// dataset to use
			gq.cmdCas = (casVal == true) ? cmdCas : 0;
		    } else {
			gq.cmdFlag = cmdFlag = m_rnd() % m_flagCnt;// flag used
			gq.cmdData = cmdData = m_rnd() % m_dataCnt;// dataset
			gq.cmdCas = cmdCas = m_rnd.rand64();
		    }
		    cmdExpected = eNoreply;
		    if(gq.cmdExpected == eGetMiss) {
			gq.cmdExpected = eNoreply;
		    }
		    if(traceIt) {
			printf("%2d %s   key %3d   flag %3d   data %3d   "
			    "exp %s\n",
			    m_tid, cmdStr[cmdCmd], cmdKey, cmdFlag, cmdData,
			    cmdExpStr[gq.cmdExpected]);
		    }
		    getQvect.push_back(gq);
                    if(m_bBinaryEnabled == true) {
		        Get(cmdCmd, cmdKey, cmdFlag, cmdData, cmdExpected,
								gq.cmdCas);
                    }
		}

                if(m_bBinaryEnabled == false) {
                    AsciiMultiGet(getQvect);	// send the single message
                    gq.cmdKey = -1;		// for END ascii only
                    getQvect.push_back(gq);
                }
		//
		//  all queued, process responses
		//
		for(size_t i = 0; i<getQvect.size(); i++) {
		    GetQ &gq = getQvect[i];
		    if(traceIt && gq.cmdKey != -1) {
			printf("rtn for         key %3d   flag %3d   data %3d"
				"   exp %s\n",
				gq.cmdKey, gq.cmdFlag, gq.cmdData,
				cmdExpStr[gq.cmdExpected]);
		    }
		    GetReply(gq.cmdKey, gq.cmdFlag, gq.cmdData, gq.cmdExpected);
		}
                getQvect.clear();
		break;
	    default:
		assert(0);
	}

	if(m_respCheckOk == false) {
		break;
	}
    }
    if(testQuiet == true && m_bBinaryEnabled == true) {
	if(m_trace2) {
	    printf("%2d Quit  NoReply\n", m_tid);
	}
	Quit(eNoreply);
    } else {
	if(m_trace2) {
	    printf("%2d Quit  Reply\n", m_tid);
	}
	Quit(eStored);
    }
    if(0 && m_trace2) {
	printf("%2d exiting\n", m_tid);
	if(m_tid == 0) {
	    for(i=0; i<eCmdCnt; i++) {
		printf("%s\t\t%d\n", cmdStr[i], m_cmdCnt[i]);
	    }
	}
    }
    //
    //  dec the correct thread count, print if last
    //
    if(__sync_sub_and_fetch(&abThreads[ab], 1) == 0 && args.verbose >= 1) {
	printf("\n%s Stats R=reply P=pass C=casV K=keyV\n",
			(ab == 0) ? "Binary" : "Ascii");
	printf("\t    rpck rpcK rpCk rpCK rPck rPcK rPCk rPCK ");
	printf(      "Rpck RpcK RpCk RpCK RPck RPcK RPCk RPCK");
	for(i=0; i<eCmdCnt; i++) {
	    for(j=0; j<16; j++) {
		if(j == 0) {
		    printf("\n%s  ", cmdStr[i]);
		}
		printf("%04x ", ActionsCount[ab][i][j]);
	    }
	}
	printf("\n");
	printf("%s Key Timeouts %d\n",
		(ab == 0) ? "Binary" : "Ascii", keyTimeouts[ab]);
    }
}
	

void
CThread::TestDirected()
{
    uint32_t	cmdTmr = 0;
    ERsp	expected;
    bool	timedOut;
    uint32_t	timeoutTime;
    int		key;
    int		i;


    m_timeCmds = true;			// turn on timeouts
    m_genErr = false;
    m_rand32 = 15 << 16;		// timeout = 8 + 1

    key = 1;
    if(CheckKeyTime(eAddCmd, key, timedOut) == false) {
	printf("bad test 1\n");
    }
    if(m_trace2) {
	printf("%s   key %3d   flag %3d   data %3d   exp %s\n",
	    cmdStr[eAddCmd], key, 2, 3, cmdExpStr[eStored]);
    }
    Store(eAddCmd, key, 6, 5, 0, eStored);	// will start timeout
    timeoutTime = m_keyState[key].m_timeout - m_currTime;
    if(m_trace2) {
	printf("timeout time %u\n", timeoutTime);
    }
    i = 0;
    while(1) {
	printf("%s   key %3d   flag %3d   data %3d   exp %s loop %d\n",
	    cmdStr[eGetCmd], key, 2, 5, cmdExpStr[eGetHit], i);
	Get(eGetCmd, key, 6, 5, eGetHit, 0);
	sleep(1);			// 5 before
Touch(key, eTouched, 0);
	i++;
    }

    sleep(9);			// 5 after

    if(m_trace2) {
	printf("%s   key %3d   flag %3d   data %3d   exp %s\n",
	    cmdStr[eGetCmd], key, 2, 5, cmdExpStr[eGetMiss]);
    }
    Get(eGetCmd, key, 6, 5, eGetMiss, 0);


return;
    if(m_trace2) {
	printf("%s   key %3d   flag %3d   data %3d   exp %s\n",
	    cmdStr[eAddCmd], 1, 2, 3, cmdExpStr[eStored]);
    }
    Store(eAddCmd, 1, 6, 5, 0, eStored);

    if(m_trace2) {
	printf("%s   key %3d   flag %3d   data %3d   exp %s\n",
	    cmdStr[eSetCmd], 3, 8, 8, cmdExpStr[eStored]);
    }
    Store(eSetCmd, 3, 8, 7, 0, eStored);

    expected = (m_bBinaryEnabled == true) ? eExists : eNotStored;
    if(m_trace2) {
	printf("%s   key %3d   flag %3d   data %3d   exp %s\n",
	    cmdStr[eAddCmd], 3, 9, 9, cmdExpStr[expected]);
    }
    Store(eAddCmd, 3, 9, 5, 0, expected);

    expected = (m_bBinaryEnabled == true) ? eNotFound : eNotStored;
    if(m_trace2) {
	printf("%s   key %3d   flag %3d   data %3d   exp %s\n",
	    cmdStr[eReplaceCmd], 1, 10, 4, cmdExpStr[expected]);
    }
    Store(eReplaceCmd, 2, 10, 4, 0, expected);

    if(m_trace2) {
	printf("%s   key %3d   flag %3d   data %3d   exp %s\n",
	    cmdStr[eReplaceCmd], 3, 11, 2, cmdExpStr[eStored]);
    }
    Store(eReplaceCmd, 3, 11, 2, 0, eStored);

    if(m_trace2) {
	printf("%s   key %3d   flag %3d   data %3d   exp %s\n",
	    cmdStr[eGetCmd], 1, 2, 5, cmdExpStr[eGetHit]);
    }
    Get(eGetCmd, 1, 6, 5, eGetHit, 0);

    if(m_trace2) {
	printf("%s   key %3d                         exp %s\n",
	    cmdStr[eDelCmd], 1, cmdExpStr[eDeleted]);
    }
    Delete(1, eDeleted, 0);

    if(m_trace2) {
	printf("%s   key %3d   flag %3d   data %3d   exp %s\n",
	    cmdStr[eGetCmd], 1, 6, 6, cmdExpStr[eGetMiss]);
    }
    Get(eGetCmd, 1, 6, 5, eGetMiss, 0);

    if(m_trace2) {
	printf("%s   key %3d                         exp %s\n",
	    cmdStr[eDelCmd], 1, cmdExpStr[eNotFound]);
    }
    Delete(1, eNotFound, 0);

    if(m_trace2) {
	printf("%s\n", cmdStr[eStatsCmd]);
    }
    Stats();

    if(m_trace2) {
	printf("%s    key %3d   flag %3d              exp %s ID %19ld\n",
		"IncrStore",
		5, 5, cmdExpStr[eStored], (uint64_t)123456789);
    }
    Store(eSetCmd, 5, 2, (uint64_t)123456789, 0, eStored);

    cmdTmr = 0;
    if(m_trace2) {
	printf("%s   key %3d  tmr 0x%016x exp %s  DV %19ld \n",
	    cmdStr[eIncrCmd], 5, cmdTmr, cmdExpStr[eValue],(uint64_t)123457000);
    }
    Arith(eIncrCmd, 5, 211, 123457000, cmdTmr, eValue, 0);

    cmdTmr = 0xffffffff;
    if(m_trace2) {
	printf("%s   key %3d  cas 0x%016x exp %s\n",
	    cmdStr[eDecrCmd], 4, cmdTmr, cmdExpStr[eNotFound]);
    }
    Arith(eDecrCmd, 4, 12, 0x123, cmdTmr, eNotFound, 0);

    if(m_trace2) {
	printf("%s   key %3d                         exp %s\n",
	    cmdStr[eTouchCmd], 5, cmdExpStr[eTouched]);
    }
    Touch(5, eTouched, 0);

    if(m_trace2) {
	printf("%s   key %3d                         exp %s\n",
	    cmdStr[eTouchCmd], 0, cmdExpStr[eNotFound]);
    }
    Touch(0, eNotFound, 0);

    if(m_trace2) {
	printf("%s   key %3d   flag %3d   data %3d   exp %s\n",
	    cmdStr[eGetsCmd], 3, 11, 2, cmdExpStr[eGetHit]);
    }
    uint64_t casUnique = Get(eGetsCmd, 3, 11, 2, eGetHit, 0);

    if(m_trace2) {
	printf("%s   key %3d   flag %3d   data %3d   exp %s\n",
	    cmdStr[eCasCmd], 3, 15, 1, cmdExpStr[eStored]);
    }
    Store(eCasCmd, 3, 15, 1, casUnique, eStored);

    if(m_trace2) {
	printf("%s   key %3d   flag %3d   data %3d   exp %s\n",
	    cmdStr[eCasCmd], 3, 14, 3, cmdExpStr[eExists]);
    }
    Store(eCasCmd, 3, 14, 3, casUnique, eExists);

    EnableUdp(false);

    if(m_trace2) {
	printf("%s   key %3d   flag %3d   data %3d   exp %s\n",
	    cmdStr[eGetCmd], 3, 15, 1, cmdExpStr[eGetHit]);
    }
    Get(eGetCmd, 3, 15, 1, eGetHit, 0);

    string prependStr = "Prepend$";
    if(m_trace2) {
	printf("%s   key %3d   flag %3d   data %3d   exp %s\n",
	    cmdStr[ePrependCmd], 3, 3, 0, cmdExpStr[eStored]);
    }
    Store(ePrependCmd, 3, 3, prependStr, 0, eStored);

    string checkStr = prependStr + GetDataStr(1);
    if(m_trace2) {
	printf("%s   key %3d   flag %3d   data %3d   exp %s\n",
	    cmdStr[eGetCmd], 3, 15, 1, cmdExpStr[eGetHit]);
    }
    Get(eGetCmd, 3, 15, checkStr, eGetHit, 0);

    string appendStr = "$Append";
    if(m_trace2) {
	printf("%s   key %3d   flag %3d   data %3d   exp %s\n",
	    cmdStr[eAppendCmd], 3, 3, 0, cmdExpStr[eStored]);
    }
    Store(eAppendCmd, 3, 3, appendStr, 0, eStored);

    string checkStr2 = checkStr + appendStr;
    if(m_trace2) {
	printf("%s   key %3d   flag %3d   data %3d   exp %s\n",
	    cmdStr[eGetCmd], 3, 15, 1, cmdExpStr[eGetHit]);
    }
    Get(eGetCmd, 3, 15, checkStr2, eGetHit, 0);

    if(m_trace2) {
	printf("%s  disabled\n", cmdStr[eVersionCmd]);
    }
    Version();

    if(m_trace2) {
	printf("%s\n", cmdStr[eNoopCmd]);
    }
    Noop();

    Quit(eStored);
}

uint64_t
getTime(uint64_t *currUsec, int *currSec)
{
    uint64_t		rtn;
    struct timeval	tvTime;

    gettimeofday(&tvTime, 0);
    rtn = tvTime.tv_sec;
    if(currSec != 0) {
	*currSec = rtn;
    }
    rtn *= 1000000;
    rtn += tvTime.tv_usec;
    if(currUsec != 0) {
	*currUsec = rtn;
    }
    return(rtn);
}

//
//	create all keys (#keys = m_perfStoreCnt)
//	create all unique data hdrs (#hdrs = m_perfStoreCnt)
//	
//	time writing all keys
//
//	time get random keys (#gets = m_perfGetCnt)
//
void
CThread::TestPerf()
{
    int		i, j;
    int		keyIndex;
    char	*buf;
    char	bufD[256];

    if(args.perfStoreCnt > 9999999 || m_tid > 9999) {
	printf("max store coun t is 9999999   max thread is 9999\n");
	exit(-1);
    }

    m_perfKeyTbl = (char **)malloc(args.perfStoreCnt * sizeof(char *));
    m_perfKeyArray = (char *)malloc(args.perfStoreCnt * (args.perfKeyLength + 1));

    m_dataHdrLength = 4+7+2+4+1;	// "Data%d_t%d_"  9999999 store cnt max, 9999 thread max
    m_perfDataTbl = (char **)malloc(args.perfStoreCnt * sizeof(char *));
    m_perfDataArray = (char *)malloc(args.perfStoreCnt * (m_dataHdrLength + 1));

    buf = (char *)malloc(args.perfKeyLength);

    for(i=0; i<args.perfStoreCnt; i++) {
	//
	//	build the key
	//
	m_perfKeyTbl[i] = &m_perfKeyArray[i * (args.perfKeyLength + 1)];
	sprintf(buf, "Key%d_t%d_", i, m_tid);
	for (j = strlen(buf); j < args.perfKeyLength; j++) {
	    buf[j] = 'a' + m_rnd() % 26;	// fill with random
	}
	buf[args.perfKeyLength] = 0;
	memcpy(m_perfKeyTbl[i], buf, args.perfKeyLength + 1);
	//
	//	build the data header
	//
	m_perfDataTbl[i] = &m_perfDataArray[i * (m_dataHdrLength + 1)];
	sprintf(bufD, "Data%d_t%d_", i, m_tid);
	for(j=strlen(bufD); j < m_dataHdrLength; j++) {
	    bufD[j] = 'a' + m_rnd() % 26;	// fill with random 
	}
	bufD[m_dataHdrLength] = 0;
	memcpy(m_perfDataTbl[i], bufD, m_dataHdrLength + 1);
    }
    m_perfDataFixedLength = args.perfDataLength - m_dataHdrLength;
    m_perfDataFixed = (char *)malloc(m_perfDataFixedLength + 1);
    for(i=0; i<m_perfDataFixedLength; i++) {
	m_perfDataFixed[i] = 'a';
    }
    m_perfDataFixed[m_perfDataFixedLength] = 0;
    //
    //	time writimg all the keys with the data
    //
    m_startTime = getTime();
    for(i=0; i<args.perfStoreCnt; i++) {
	if(m_trace2) {
	    printf("%s   key %3d   flag %3d   data %3d   exp %s\n",
		cmdStr[eSetCmd], i, 8, i, cmdExpStr[eStored]);
	}
	Store(eSetCmd, i, 8, i, 0, eStored);	// flags the same
    }
    m_stopTime = getTime();
    m_storeTime = m_stopTime - m_startTime;
    if(args.perfStop != 0) {
	printf("\nPerf Hit enter to continue  or ^C to exit ");
	fflush(stdout);
	getc(stdin);
    }
    //
    //	time random gets
    //
    m_startTime = getTime();
    for(i=0; i<args.perfGetCnt; i++) {
	keyIndex = rand() % args.perfStoreCnt;	// random across all stores
	if(m_trace2) {
	    printf("%s   key %3d   flag %3d   data %3d   exp %s\n",
		cmdStr[eGetCmd], keyIndex, 8, keyIndex, cmdExpStr[eGetHit]);
	}
	Get(eGetCmd, keyIndex, 8, keyIndex, eGetHit, 0);
    }
    m_stopTime = getTime();
    m_getTime = m_stopTime - m_startTime;

    Quit(eStored);
    free(m_perfKeyArray);
    free(m_perfKeyTbl);

    free(m_perfDataArray);
    free(m_perfDataTbl);
}

void
CThread::printPerfData()
{
    uint64_t	dataXfered;

    dataXfered = sizeof(protocol_binary_response_header) + 4;
    dataXfered += args.perfDataLength;
    dataXfered *= args.perfGetCnt;


    printf("%d Stores in\t%d us\tavg %.1f us\n",
	    args.perfStoreCnt, (int)m_storeTime,
	    (float)m_storeTime/(float)args.perfStoreCnt);
    printf("%d Gets in\t%d us\tavg %.1f us\t%ld Mbytes/sec\n",
	    args.perfGetCnt, (int)m_getTime,
	    (float)m_getTime/(float)args.perfGetCnt,
	    dataXfered/m_getTime);
}

void
CThread::TestStats()
{
    if(args.statTest == 0 || args.statTest == 1) {
	Stats(0, true);
    }
    if(args.statTest == 0 || args.statTest == 2) {
	Stats("items", true);
    }
    if(args.statTest == 0 || args.statTest == 3) {
	Stats("settings", true);
    }
    if(args.statTest == 0 || args.statTest == 4) {
	Stats("sizes", true);
    }
    if(args.statTest == 0 || args.statTest == 5) {
	Stats("slabs", true);
    }
    if(args.statTest == 0 || args.statTest == 6) {
	Stats("detail on", true);
    }
    if(args.statTest == 0 || args.statTest == 7) {
	Stats("detail off", true);
    }
    if(args.statTest == 0 || args.statTest == 8) {
	Stats("detail dump", true);
    }
    if(args.statTest == 0 || args.statTest == 9) {
	Stats("cachedump 2 4", true, "ITEM ");
    }
    if(args.statTest == 0 || args.statTest == 10) {
	Stats("reset", true);
    }
    Quit(eStored);
}
	

void
CThread::GetKeyState(int key, int &flag, int &data, uint64_t &cas,
			bool &casValid, KEY_STATE *keyState, uint32_t *timeVal)
{
    flag = m_keyState[key].m_flagIdx;
    data = m_keyState[key].m_dataIdx;
    cas = m_keyState[key].m_cas;
    casValid = m_keyState[key].m_casValid;
    if(keyState != 0) {
	*keyState = m_keyState[key].m_state;
    }
    if(timeVal != 0) {
	*timeVal = m_keyState[key].m_timeout;
    }
}

void
CThread::SetItemState(KEY_STATE valid, ECmd cmd, int key, int flagIdx,
				int dataIdx, uint64_t cas, CAS_STATE casState)
{
    m_keyState[key].m_state = valid;

    if(valid == INVALID) {
	m_keyState[key].m_preIdx = m_keyState[key].m_appIdx = -1;
	m_keyState[key].m_casValid = false;
    } else {
	if(cmd == ePrependCmd) {
	    m_keyState[key].m_preIdx = dataIdx;
	} else if(cmd == eAppendCmd) {
	    m_keyState[key].m_appIdx = dataIdx;
	} else if(cmd != eCmdCnt) {		// set cas only if eCmdCnt
	    m_keyState[key].m_dataIdx = dataIdx;
	    m_keyState[key].m_flagIdx = flagIdx;	// set if not pre/app
	    m_keyState[key].m_preIdx = m_keyState[key].m_appIdx = -1;
	}
	m_keyState[key].m_cas = cas;
	switch(casState) {
	    case C_INV:
		m_keyState[key].m_casValid = false;
		break;
	    case C_VAL:
		m_keyState[key].m_casValid = true;
		break;
	}
    }
    if(key == args.traceKey || m_trace2) {
	printf("   Set state    key %3d   valid %d    cas 0x%lx (%ld)   "
		"casS %d   pre %d   app %d   CTO %u\n",
	    key, valid, cas, cas, casState,
	    m_keyState[key].m_preIdx, m_keyState[key].m_appIdx,
	     m_keyState[key].m_timeout);
    }
}

bool traceTimeout = false;
void
CThread::ClearKeyTimeout(int key)
{
    if(traceTimeout || key == args.traceKey) {
	printf("        clear\tkey %3d\n", key);
    }
    m_keyState[key].m_timeout = 0;
    m_keyState[key].m_nextTimeout = 0;
}

void
CThread::SetKeyNextTimeout(int key, uint32_t nextTimeout)
{
    if(traceTimeout || key == args.traceKey) {
	printf("SN\tkey %3d   NtimeO %u   CtimeO %u   currT %u\n",
		key, nextTimeout, m_keyState[key].m_timeout, m_currTime);
    }
    m_keyState[key].m_nextTimeout = nextTimeout;
}

uint32_t
CThread::StartKeyTimeout(int key)
{
    m_keyState[key].m_timeout = m_keyState[key].m_nextTimeout;

    if(traceTimeout || key == args.traceKey) {
	printf("        StartKT\tkey %3d   TOT %u   currT %u   sentT %u\n",
		key, m_keyState[key].m_timeout, m_currTime,
		m_keyState[key].m_timeout);
    }
//    return(m_keyState[key].m_timeout - m_currTime);	// relative
    return(m_keyState[key].m_timeout);	// absolute (linux)
}

bool
CThread::KeyItemPresent(int key)
{
    bool	rtn;

    if(m_keyState[key].m_state == VALID) {
	rtn = true;
    } else {
	rtn = false;
    }
    return(rtn);
}

void CThread::UdpSocket()
{
	if(args.udpPort == 0) {
		return;
	}
	//----------------------
	// Create a SOCKET for server
	m_udpFd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (m_udpFd == INVALID_SOCKET) {
		printf("Error at socket(): %d\n", WSAGetLastError());
		WSACleanup();
		exit(-1);
	}

	memset((char *) &m_udpAddr, 0, sizeof(m_udpAddr));
	m_udpAddr.sin_family = AF_INET;
	m_udpAddr.sin_addr.s_addr = inet_addr(args.hostIp.c_str());
	m_udpAddr.sin_port = htons(args.udpPort);

	m_udpAddrLen = sizeof(m_udpAddr);
}

void CThread::TcpSocket()
{
    	if(args.tcpPort == 0) {
		return;
	}
	//----------------------
	// Create a SOCKET for connecting to server
	m_tcpFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_tcpFd == INVALID_SOCKET) {
		printf("Error at socket(): %d\n", WSAGetLastError());
		WSACleanup();
		exit(-1);
	}

	//----------------------
	// The sockaddr_in structure specifies the address family,
	// IP address, and port of the server to be connected to.
	sockaddr_in clientService; 
	clientService.sin_family = AF_INET;
	clientService.sin_addr.s_addr = inet_addr(args.hostIp.c_str());
	clientService.sin_port = htons(args.tcpPort);

	//----------------------
	// Connect to server.
	if ( connect( m_tcpFd, (SOCKADDR*) &clientService, sizeof(clientService) ) == SOCKET_ERROR) {
		printf( "Failed to connect.\n" );
		WSACleanup();
		exit(-1);
	}
}

uint64_t
CThread::Get(ECmd cmd, int keyIdx, int flagsIdx, int dataIdx, ERsp reply,
		uint64_t casUnique)
{
    if(m_bBinaryEnabled) {
	return BinaryGet(cmd, keyIdx, flagsIdx, dataIdx, reply, casUnique);
    } else {
	return AsciiGet(cmd, keyIdx, flagsIdx, dataIdx, reply);
    }
}

uint64_t
CThread::Get(ECmd cmd, int keyIdx, int flagsIdx, string & dataStr, ERsp reply,
		uint64_t casUnique)
{
    if(m_bBinaryEnabled) {
	return BinaryGet(cmd, keyIdx, flagsIdx, dataStr, reply, casUnique);
    } else {
	return AsciiGet(cmd, keyIdx, flagsIdx, dataStr, reply);
    }
}

void
CThread::Store(ECmd cmd, int keyIdx, int flagsIdx, uint64_t value,
			uint64_t casUnique, ERsp reply)
{
    if(m_bBinaryEnabled) {
	BinaryStore(cmd, keyIdx, flagsIdx, value, casUnique, reply);
    } else {
	AsciiStore(cmd, keyIdx, flagsIdx, value, casUnique, reply);
    }
}

void
CThread::Store(ECmd cmd, int keyIdx, int flagsIdx, int dataIdx,
			uint64_t casUnique, ERsp reply)
{
    if(m_bBinaryEnabled) {
	BinaryStore(cmd, keyIdx, flagsIdx, dataIdx, casUnique, reply);
    } else {
	AsciiStore(cmd, keyIdx, flagsIdx, dataIdx, casUnique, reply);
    }
}

void
CThread::Store(ECmd cmd, int keyIdx, int flagsIdx, string &str,
			uint64_t casUnique, ERsp reply)
{
    if(m_bBinaryEnabled) {
	BinaryStore(cmd, keyIdx, flagsIdx, str, casUnique, reply);
    } else {
	AsciiStore(cmd, keyIdx, flagsIdx, str, casUnique, reply);
    }
}

void CThread::Delete(int keyIdx, ERsp reply, uint64_t casUnique)
{
    if(m_bBinaryEnabled) {
	BinaryDelete(keyIdx, reply, casUnique);
    } else {
	AsciiDelete(keyIdx, reply);
    }
}

void CThread::Touch(int keyIdx, ERsp reply, uint64_t casUnique)
{
    if(m_bBinaryEnabled) {
	BinaryTouch(keyIdx, reply, casUnique);
    } else {
	AsciiTouch(keyIdx, reply);
    }
}

void CThread::Quit(ERsp reply)
{
	if (m_bBinaryEnabled)
		BinaryQuit(reply);
	else
		AsciiQuit(reply);
}

void CThread::Noop()
{
	if (m_bBinaryEnabled)
		BinaryNoop();
	else
		AsciiNoop();
}

void CThread::Version()
{
	if (m_bBinaryEnabled)
		BinaryVersion();
	else
		AsciiVersion();
}

void CThread::FlushAll()
{
	if (m_bBinaryEnabled)
		BinaryFlushAll();
	else
		AsciiFlushAll();
}

uint64_t
CThread::GetReply(int keyIdx, int flagsIdx, int dataIdx, ERsp reply)
{
    uint64_t    rtn;

    if(m_bBinaryEnabled) {
        rtn = BinaryGetReply(keyIdx, flagsIdx, dataIdx, reply);
    } else {
        rtn = AsciiGetReply(keyIdx, flagsIdx, dataIdx, reply);
    }
    return(rtn);
}

void CThread::Stats(const char * pSubStat, bool printData, const char *hdr)
{
	if (m_bBinaryEnabled)
		BinaryStats(pSubStat, printData, hdr);
	else
		AsciiStats(pSubStat, printData, hdr);
}

void
CThread::Arith(ECmd cmd, int keyIdx, uint64_t inV, uint64_t outV,
		uint32_t cmdTmr, ERsp reply, uint64_t casUnique)
{
    if(m_bBinaryEnabled) {
	BinaryArith(cmd, keyIdx, inV, outV, cmdTmr, reply, casUnique);
    } else {
	AsciiArith(cmd, keyIdx, inV, outV, cmdTmr, reply);
    }
}

void CThread::SendMsg(msghdr & msghdr)
{
    if (m_bUdpEnabled) {
	SendMsgUdp(msghdr);
    } else {
	SendMsgTcp(msghdr);
    }
}

void CThread::SendMsgUdp(msghdr & msghdr)
{
    do {
	uint32_t res = sendmsg(m_udpFd, &msghdr, 0);

	while (msghdr.msg_iovlen > 0 && msghdr.msg_iov->iov_len <= res) {
	    res -= msghdr.msg_iov->iov_len;
	    msghdr.msg_iov += 1;
	    msghdr.msg_iovlen -= 1;
	}

	if (res > 0) {
	    msghdr.msg_iov->iov_base = ((char *)msghdr.msg_iov->iov_base) + res;
	    msghdr.msg_iov->iov_len -= res;
	}
    } while (msghdr.msg_iovlen > 0);
}

void CThread::SendMsgTcp(msghdr & msghdr)
{
    do {
	uint32_t res = sendmsg(m_tcpFd, &msghdr, 0);

	while (msghdr.msg_iovlen > 0 && msghdr.msg_iov->iov_len <= res) {
	    res -= msghdr.msg_iov->iov_len;
	    msghdr.msg_iov += 1;
	    msghdr.msg_iovlen -= 1;
	}

	if (res > 0) {
	    msghdr.msg_iov->iov_base = ((char *)msghdr.msg_iov->iov_base) + res;
	msghdr.msg_iov->iov_len -= res;
	}
    } while (msghdr.msg_iovlen > 0);
}

bool CThread::RespCheck(uint64_t value, bool bErrMsg)
{
    if (m_bUdpEnabled) {
	return RespCheckUdp(value, bErrMsg);
    } else {
	return RespCheckTcp(value, bErrMsg);
    }
}

bool CThread::RespCheckUdp(uint64_t value, bool bErrMsg)
{
	char buf[64];
	sprintf(buf, "%d", (int)value);

	return RespCheckUdp(buf, bErrMsg);
}

bool CThread::RespCheckTcp(uint64_t value, bool bErrMsg)
{
	char buf[64];
	sprintf(buf, "%d", (int)value);

	return RespCheckTcp(buf, bErrMsg);
}

bool CThread::RespCheck(uint64_t value, size_t size, bool bErrMsg)
{
	if (m_bUdpEnabled)
		return RespCheckUdp(value, size, bErrMsg);
	else
		return RespCheckTcp(value, size, bErrMsg);
}

bool CThread::RespCheckTcp(uint64_t value, size_t size, bool bErrMsg)
{
    // first check what is in buffer
    const char	*pRespStr = (const char *)&value;
    ssize_t	respLen = size;
    bool	rtn = true;

    while (respLen > 0) {
	if (m_tcpRespBufLen == 0) {
	    m_pTcpRespBuf = m_tcpRespBuf;
	    m_tcpRespBufLen = m_recv(m_tcpFd, m_tcpRespBuf, RESP_BUF_SIZE, 0,
			"RespCheckTcp");
	    if(m_tcpRespBufLen == 0) {
		ErrorMsg("RespCheckTcp read failed", 0, 0);
		exit(0);
	    }
	}
	size_t checkLen = min(respLen, m_tcpRespBufLen);
	if (memcmp(m_pTcpRespBuf, pRespStr, checkLen) != 0) {
	    rtn = false;
	}

	m_pTcpRespBuf += checkLen;
	m_tcpRespBufLen -= checkLen;

	pRespStr += checkLen;
	respLen -= checkLen;
    }

    return rtn;
}

bool CThread::RespCheckUdp(uint64_t value, size_t size, bool bErrMsg)
{
    // first check what is in buffer
    const char * pRespStr = (const char *)&value;
    ssize_t respLen = size;

    while (respLen > 0) {
	if(FillUdpBuffer() == false) {
	    return(false);
	}

	assert(m_udpRespBufLen > 0);

	size_t checkLen = min(respLen, m_udpRespBufLen);
	if (memcmp(m_pUdpRespBuf, pRespStr, checkLen) != 0) {
	    if (bErrMsg)
		assert(0);
	    else
		return false;
	}

	m_pUdpRespBuf += checkLen;
	m_udpRespBufLen -= checkLen;

	pRespStr += checkLen;
	respLen -= checkLen;
    }

    return true;
}

bool CThread::RespCheck(string respStr, bool bErrMsg)
{
	if (m_bUdpEnabled)
		return RespCheckUdp(respStr, bErrMsg);
	else
		return RespCheckTcp(respStr, bErrMsg);
}

bool CThread::RespCheckTcp(string respStr, bool bErrMsg)
{
    // first check what is in buffer
    const char	    *pRespStr = respStr.c_str();
    ssize_t	    respLen = respStr.size();
    fd_set	    read;
    int		    sel;

    m_respCheckOk = true;
    do  {
	if (m_tcpRespBufLen == 0) {
	    timeval tv = {1,1};		// wait 1 sec
	    FD_ZERO(&read);
	    FD_SET(m_tcpFd, &read);
	    sel = select(m_tcpFd + 1, &read, 0, 0, &tv);
	    if(sel != 1) {		// closed
		if(respStr.length() == 0) {
		    return(true);	// closed and expecting 0 length
		} else {
		    return(false);
		}
	    }
	    m_pTcpRespBuf = m_tcpRespBuf;
	    m_tcpRespBufLen = m_recv(m_tcpFd, m_tcpRespBuf, RESP_BUF_SIZE, 0,
			"RespCheckTcp");
	    //if(m_tcpRespBufLen == 0) {
		//ErrorMsg("RespCheckTcp read failed", 0, 0);
		//exit(0);
	    //}
	}

	if (respStr.size() == 0) {
	    assert(m_tcpRespBufLen == 0);
	} else {
	    assert(m_tcpRespBufLen > 0);
	}

	size_t checkLen = min(respLen, m_tcpRespBufLen);
	if (memcmp(m_pTcpRespBuf, pRespStr, checkLen) != 0) {
	    if (bErrMsg)
		assert(0);
	    else {
		// hack till it is fixed
		if(respStr == "STAT ") {
		    if(strncmp(m_pTcpRespBuf, "END\r\n", 5) == 0) {
			return(false);
		    }
		}
		printf("RespCheckTcp failure expected %s   got ", respStr.c_str());
		if(strlen(m_pTcpRespBuf) > 0) {
		    printf("%.*s", (int)checkLen, m_pTcpRespBuf);
		    if(strlen(m_pTcpRespBuf) > checkLen) {
			printf("...");
		    }
		}
		printf("\n");
		m_respCheckOk = false;
		return false;
	    }
	}

	m_pTcpRespBuf += checkLen;
	m_tcpRespBufLen -= checkLen;

	pRespStr += checkLen;
	respLen -= checkLen;
    } while (respLen > 0);

    return true;
}


bool
CThread::RespCheckUdp(string respStr, bool bErrMsg)
{
    // first check what is in buffer
    const char	*pRespStr = respStr.c_str();
    ssize_t	respLen = respStr.size();

    m_respCheckOk = true;
    while (respLen > 0) {
	if(FillUdpBuffer() == false) {
	    m_respCheckOk = false;
	    return(false);
	}
	assert(m_udpRespBufLen > 0);

	size_t checkLen = min(respLen, m_udpRespBufLen);
	if (memcmp(m_pUdpRespBuf, pRespStr, checkLen) != 0) {
	    if (bErrMsg) {
	        assert(0);
	    } else {
		printf("RespCheckTcp failure expected %s   got ",
				respStr.c_str());
		if(strlen(m_pUdpRespBuf) > 0) {
		    printf("%.20s", m_pUdpRespBuf);
		    if(strlen(m_pUdpRespBuf) > 20) {
			printf("...");
		    }
		}
	        printf("\n");
		m_respCheckOk = false;
		return false;
	    }
	}

        m_pUdpRespBuf += checkLen;
        m_udpRespBufLen -= checkLen;

        pRespStr += checkLen;
        respLen -= checkLen;
    }

    return true;
}

bool
CThread::RespGetToEol(string &respStr)
{
    bool    rtn;

    if(m_bUdpEnabled) {
	rtn = RespGetToEolUdp(respStr);
    } else {
	rtn = RespGetToEolTcp(respStr);
    }
    return(rtn);
}

bool
CThread::RespGetToEolUdp(string &statString)
{
    bool	prevWasR = false;
    char	currC;

    statString = "";

    while(1) {
	if (m_udpRespBufLen == 0) {
	    if(FillUdpBuffer() == false) {
		m_respCheckOk = false;
		return(false);
	    }
	}
	assert(m_udpRespBufLen > 0);
	currC = *m_pUdpRespBuf++;
	m_udpRespBufLen--;
	statString += currC;
	if(prevWasR == true && currC == '\n') {
	    break;					//end of line
	}
	prevWasR = currC == '\r';
    }
    return(true);
}

bool
CThread::RespGetToEolTcp(string &statString)
{
    bool	prevWasR = false;
    char	currC;

    statString = "";
    while(1) {
	if (m_tcpRespBufLen == 0) {
	    m_pTcpRespBuf = m_tcpRespBuf;
	    m_tcpRespBufLen = m_recv(m_tcpFd, m_tcpRespBuf, RESP_BUF_SIZE, 0,
			"RespGetToEolTcp");
	    if(m_tcpRespBufLen == 0) {
		ErrorMsg("RespGetToEolTcp read failed", 0, 0);
		return(false);
	    }
	}
	currC = *m_pTcpRespBuf++;
	m_tcpRespBufLen--;
	statString += currC;
	if(prevWasR == true && currC == '\n') {
	    break;									//end of line
	}
	prevWasR = currC == '\r';
    }
    return(true);
}

uint64_t CThread::RespGetUint64(string &vStr)
{
    if (m_bUdpEnabled) {
	return RespGetUint64Udp(vStr);
    } else {
	return RespGetUint64Tcp(vStr);
    }
}

uint64_t CThread::RespGetUint64Udp(string &vStr)
{
	uint64_t	v = 0;
	int			i = 0;
	char		c;
	int			len;
	
	len = vStr.length();
	while(i < len) {
		c = vStr[i];
		if(isdigit(c) == false) {
			break;
		}
		v = v * 10 + c - '0';
		i++;
	}
	return v;
}

uint64_t CThread::RespGetUint64Tcp(string &vStr)
{
    uint64_t	v = 0;
    int		i = 0;
    char	c;
    int		len;
	
    len = vStr.length();
    while(i < len) {
	c = vStr[i];
	if(isdigit(c) == false) {
	    break;
	}
	v = v * 10 + c - '0';
	i++;
    }
    return v;
}

void CThread::CreateUdpHeader(char * hdr)
{
	m_udpReqId += 1;
	m_udpSeqNum = 0;

	*hdr++ = m_udpReqId / 256;
	*hdr++ = m_udpReqId % 256;
	*hdr++ = 0;
	*hdr++ = 0;
	*hdr++ = 0;
	*hdr++ = 1;
	*hdr++ = 0;
	*hdr++ = 0;
}

bool
CThread::ParseUdpHeader(CUdpHdr * pHdr)
{
    pHdr->m_reqId = (m_udpRespBuf[0] << 8) | (unsigned char)m_udpRespBuf[1];
    pHdr->m_seqNum = (m_udpRespBuf[2] << 8) | (unsigned char)m_udpRespBuf[3];
    pHdr->m_pktCnt = (m_udpRespBuf[4] << 8) | (unsigned char)m_udpRespBuf[5];
    pHdr->m_pad = (m_udpRespBuf[6] << 8) | (unsigned char)m_udpRespBuf[7];

    if(pHdr->m_reqId != m_udpReqId || pHdr->m_seqNum != m_udpSeqNum
			|| pHdr->m_pad != 0) {
	printf("%d udp failure id got %d exp %d   seq got %d exp %d   "
		"pad got %d exp 0\n",
		m_tid, pHdr->m_reqId, m_udpReqId, pHdr->m_seqNum,
		m_udpSeqNum, pHdr->m_pad);
	printf("***********************************************************\n");
	return(false);
    }
    assert(pHdr->m_reqId == m_udpReqId);
    assert(pHdr->m_seqNum == m_udpSeqNum); 
    assert(pHdr->m_pad == 0);

    m_udpSeqNum += 1;

    m_pUdpRespBuf += 8;
    m_udpRespBufLen -= 8;
    return(true);
}
//
//  CheckKeyTime sets the timeout in keyState (0 for not timed)
//  and checks if the key is being timed (sets keyTimeout if needed)
//  return false to skip test
bool
CThread::CheckKeyTime(ECmd cmd, int key, bool &timedOut)
{
    int		flag;
    int		data;
    uint64_t	cas;
    bool	casVal;
    KEY_STATE	keyState;
    uint32_t	keyExpTime;
    bool	rtn = true;		// true = execute test
    timeval	tv;
    uint32_t	nextTimeout;

    if(m_timeCmds == false) {		// thread doing timeouts?
	ClearKeyTimeout(key);		// set timeoout and nextTimeout to 0
	timedOut = false;
	return(true);			// execute test
    }
    //
    //	thread is testing timeouts, 
    //
    gettimeofday(&tv, 0);
    m_currTime = tv.tv_sec;
    nextTimeout = ((m_rand32 >> 16) & 0x0f) + m_currTime + 1;
    //
    //	Now, set the new expiration time 
    //	it may not be used
    //
    if(traceTimeout) {
	printf("%s", cmdStr[cmd]);
    }
    if(m_bBinaryEnabled == false && (cmd == ePrependCmd || cmd == eAppendCmd)) {
	//
	//  The ascii versions of append and prepend do not update the timeout
	//  so just set the current timeout as the next
	//
	nextTimeout = m_keyState[key].m_timeout;
	m_keyState[key].m_nextTimeout = m_keyState[key].m_timeout;
	if(traceTimeout) {
	    printf("  FK key %3d  TOT %u\n", key,m_keyState[key].m_nextTimeout);
	}
    }
    SetKeyNextTimeout(key, nextTimeout); // set nextTimeout (not curr timeout)
    GetKeyState(key, flag, data, cas, casVal, &keyState, &keyExpTime);
    if(keyState == VALID) {
	if(m_currTime <= (keyExpTime - args.keyTimeWindow)) {
	    //
	    // has not expired and not close
	    //
	    timedOut = false;
	} else if(m_currTime >= (keyExpTime + args.keyTimeWindow)) {
	    //
	    // has expired and not close
	    //
	    if(m_trace2 || key == args.traceKey) {
		printf("***************************************************\n");
		printf("     Timeout on key %3d   TOT %u   currT %u\n",
			key, keyExpTime, m_currTime);

		printf("Etime %d   tvsec %d   tw %d   cmp %d\n",
		    keyExpTime, m_currTime, args.keyTimeWindow,
		    keyExpTime >= (m_currTime + args.keyTimeWindow));  
	    }
	    ClearKeyTimeout(key);	// set timeoout and nextTimeout to 0
	    timedOut = true;
	} else {
	    if(m_trace2 || key == args.traceKey) {
		printf("  close, skipped\n");
	    }
	    rtn = false;		// in the close range, skip
	}
    }

    return(rtn);
}

void
CThread::PrintTrace(bool dot)
{
    if(dot == true) {
	printf(".");
	fflush(stdout);
	return;
    }
    printf("Active Binary Threads %4d   Active Ascii Threads %4d   "
	"Total Threads %d   timeSec %d\n",
	abThreads[0], abThreads[1], args.threadCnt, m_currSec);
    fflush(stdout);
}
