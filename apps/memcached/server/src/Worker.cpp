/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT memcached application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "MemCached.h"

#if !defined(WIN32) && defined(MCD_NUMA)
//#include <numa.h>
//#include <numaif.h>
uint32_t CWorker::m_evenCpuSet;
uint32_t CWorker::m_oddCpuSet;
uint32_t CWorker::m_coreCnt;
#endif

#include "Timer.h"
#include "Powersave.h"
uint64_t pktRecvTime;
uint64_t blkSendTime;

CWorker **g_pWorkers;
extern double g_clocks_per_sec;
extern uint64_t g_activityCntDwn;

#define WORKER_BLOCK_ALLOC_ALIGN	(256*1024)
#define WORKER_BLOCK_ALLOC_SIZE		WORKER_BLOCK_ALLOC_ALIGN

void * AlignedMmap(uint32_t align, uint32_t size)
{
#if defined(MCD_NUMA) && !defined(WIN32)
    uint32_t total = size + align;
    uint8_t * pMem = (uint8_t*)mmap(0, total, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (pMem == 0)
        return 0;

    // unmap unneeded memory
    uint64_t addr = (uint64_t)pMem;
    if ((addr & (align-1)) != 0) {
        uint32_t leading = align - (addr & (align-1));
        if (munmap(pMem, leading) != 0)
            perror("unmap failed 1");
        pMem += leading;
        total -= leading;
    }

    if (total > size) {
        if (munmap(pMem + size, total - size) != 0)
            perror("unmap failed 2");
    }

    memset(pMem, 0, size);
    //numa_setlocal_memory(pMem, size);
    //printf("tid=%d, pMem=0x%llx\n", tid, (long long)pMem);
    return pMem;

#else
    void * pMem;
    posix_memalign(&pMem, align, size);
    memset(pMem, 0, size);
    return pMem;
#endif
}

static void WorkerThreadTimer(int fd, short which, void *arg) {
    CWorker *me = (CWorker *)arg;

    struct timeval t;

    t.tv_sec = 0;
    t.tv_usec = 0;

    if (g_settings.powersave) {
	
	PowersaveSchedule(me);
	
    } else {
	// Non-powersave path - every Worker just handles itself.
	g_pTimers[5]->Start(me->m_unitId);
	g_pSamples[2]->Sample(me->m_freeCurr, me->m_unitId);

	//me->m_pMcdUnit->RecvMsgLoop();

	// Push close messages on recv buffer
	(void)me->AddCloseMsgToRecvBuffer();

	// flush receive buffer if non-empty and fill time expired
	(void)me->FlushRecvBuf();

	// process response commands
	(void)me->ProcessRespBuf();

	// transmit packet from list of ready connections
	(void)me->TransmitFromList();

	// process commands from main thread
	(void)me->HandleDispatchCmds();
    
	g_pTimers[5]->Stop(me->m_unitId);

    }

    // Powersave Worker 0 slowspin support - only in powersave mode 3
    if (g_settings.powersave && (g_settings.powersave_preset > 2)
	&& (me->m_unitId == 0) && (me->m_slowSpin == true)) {

	t.tv_usec = 1;
    }
    
    // create an timer event to force partially filled packet buffer to be sent to coproc

    evtimer_set(&me->m_timerEvent, WorkerThreadTimer, me);
    event_base_set(me->m_base, &me->m_timerEvent);
    evtimer_add(&me->m_timerEvent, &t);      
}

static void RegisterThreadInitialized(void) {
    pthread_mutex_lock(&init_lock);
    init_count++;
    pthread_cond_signal(&init_cond);
    pthread_mutex_unlock(&init_lock);
}

int CWorker::HandleDispatchCmds()
{
    /*
    * Processes an incoming "handle a new connection" item or other main thread commands.
    */
    int rc = 0;

    while (!m_dispCmdQue.IsEmpty()) {

    rc = 1;
    CDispCmd & dispCmd = m_dispCmdQue.Front();

    switch (dispCmd.m_cmd) {
    case 'c':
        {
            // check if space exists on recvBuf
            if (m_recvBufWrPktIdx < (int)(MCD_RECV_BUF_MAX_BLK_SIZE - CNY_RECV_BUF_HDR_BLOCK_SIZE))
                // current receive buffer is full, advance to next buffer
                FlushRecvBuf(true);

            // if not then just return
            if (!m_bRecvBufSpaceAvail && !IsRecvBufSpaceAvail())
                return rc;

            // if UDP socket then create one for each personality unit thread
            int connCnt = dispCmd.m_transport == udp_transport ? CNY_UNIT_UDP_CONN_CNT : 1;
            for (int i = 0; i < connCnt; i += 1) {
                CConn *c = NewConn(dispCmd.m_sfd, dispCmd.m_bListening, i == 0 ? dispCmd.m_eventFlags : 0,
                    dispCmd.m_readBufferSize, dispCmd.m_transport, m_base, this);

                if (c == NULL) {
                    if (IS_UDP(dispCmd.m_transport)) {
                        fprintf(stderr, "Can't listen for events on UDP socket\n");
                        exit(1);
                    } else {
                        if (g_settings.m_verbose > 0) {
                            fprintf(stderr, "Unable to allocate new connection structure\n");
                        }
                        close_socket(dispCmd.m_sfd);
                    }
                } else {
                    c->m_pWorker = this;
                }
            }
        } 
        break;
        /* we were told to flip the lock type and report in */
    case 'l':
        m_itemLockType = ITEM_LOCK_GRANULAR;
        RegisterThreadInitialized();
        break;
    case 'g':
        m_itemLockType = ITEM_LOCK_GLOBAL;
        RegisterThreadInitialized();
        break;
    }

    m_dispCmdQue.Pop();
    }

    return rc;
}

void CWorker::InitCpuSets()
{
#if !defined(WIN32) && defined(MCD_NUMA)
    // initialize the cpu set for even and odd threads
    FILE *fp;

    const char * pPath = "/sys/devices/system/cpu/present";
    if (!(fp = fopen(pPath, "r"))) {
        fprintf(stderr, "Unable to open %s\n", pPath);
        exit(1);
    }

    int cpuLo, cpuHi;
    if (fscanf(fp, "%d-%d", &cpuLo, &cpuHi) != 2) {
        fprintf(stderr, "Unexpected format: %s\n", pPath);
        exit(1);
    }
    fclose(fp);

    m_coreCnt = cpuHi+1;

    const char * pCpu0 = "/sys/devices/system/cpu/cpu0/topology/core_siblings";
    if (!(fp = fopen(pCpu0, "r"))) {
        fprintf(stderr, "Unable to open %s\n", pCpu0);
        exit(1);
    }

    int cpu0MaskHi, cpu0MaskLo;
    if (fscanf(fp, "%x,%x", &cpu0MaskHi, &cpu0MaskLo) != 2) {
        fprintf(stderr, "Unexpected format: %s\n", pCpu0);
        exit(1);
    }
    fclose(fp);

    m_evenCpuSet = 0;
    m_oddCpuSet = 0;

    for (int i = 0; i < cpuHi+1; i += 1) {
        if (cpu0MaskLo & (1 << i))
            m_evenCpuSet |= (1 << i);
        else
            m_oddCpuSet |= (1 << i);
    }

    if (g_settings.m_cnyVerbose > 0)
        printf("coreCnt=%d, evenCpuSet=%x, oddCpuSet=%x\n", m_coreCnt, m_evenCpuSet, m_oddCpuSet);
#endif
}

/*
* Creates a worker thread.
*/
void CWorker::CreateThread(int tid) {
    pthread_attr_t  attr;
    int             ret;
    pthread_t		threadId;

    static int tidArray[32];
    tidArray[tid] = tid;

    pthread_attr_init(&attr);

    if ((ret = pthread_create(&threadId, &attr, CWorker::StartWorkerThread, (void *)&tidArray[tid])) != 0) {
        fprintf(stderr, "Can't create thread: %s\n", strerror(ret));
        exit(1);
    }
}

void * CWorker::StartWorkerThread(void * arg)
{
    uint32_t tid = *(int *)arg;
    assert(tid < 16);

    // assign thread to cpu set
#if !defined(WIN32) && defined(MCD_NUMA)
    uint32_t cpuId = -1;
    if (tid & 1) {
        // odd
        for (cpuId = 0; cpuId < m_coreCnt; cpuId += 1) {
            if (m_oddCpuSet & (1 << cpuId)) {
                m_oddCpuSet &= ~(1 << cpuId);
                break;
            }
        }
    } else {
        // even
        for (cpuId = 0; cpuId < m_coreCnt; cpuId += 1) {
            if (m_evenCpuSet & (1 << cpuId)) {
                m_evenCpuSet &= ~(1 << cpuId);
                break;
            }
        }
    }

    if (g_settings.m_cnyVerbose > 0)
        printf("tid %d => cpu %d\n", tid, cpuId);

    cpu_set_t cpuSet;
    CPU_ZERO(&cpuSet);
    CPU_SET(cpuId, &cpuSet);

    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuSet);

#endif

    // allocate a block of memory for thread specific memory allocator
    size_t workerSize = sizeof(CWorker);
    assert(workerSize < WORKER_BLOCK_ALLOC_SIZE);

    uint8_t * pBlock = (uint8_t *) AlignedMmap(WORKER_BLOCK_ALLOC_ALIGN, WORKER_BLOCK_ALLOC_SIZE);
    if (!pBlock) {
        perror("AlignedMmap failed");
        exit(1);
    }

    uint32_t blockSize = WORKER_BLOCK_ALLOC_SIZE;

    // allocate memory for worker thread
    g_pWorkers[tid] = (CWorker *)pBlock;
    memset(pBlock, 0, sizeof(CWorker));

    pBlock += sizeof(CWorker);
    blockSize -= sizeof(CWorker);

    g_pWorkers[tid]->Setup(tid, pBlock, blockSize);

    g_pWorkers[tid]->StartEventLoop();

    return 0;
}

/*
* Set up a thread's information.
*/
void CWorker::Setup(int tid, uint8_t *pBlock, size_t blockSize) {
    short i;
    
    m_unitId = tid;

    if (g_settings.powersave) {
	m_powersaveTimer = (uint64_t)CTimer::Rdtsc() + ((tid==0 ? 20 : 5) * g_clocks_per_sec);
	m_noWorkCount = 0;
	m_workload = 1;
	for (i=0; i<g_settings.num_threads; i++) {
	    m_responsibleFor[i] = (i==tid) ? true : false;
	}
	m_psPendHead = 0;
	m_slowSpin = false;
	pthread_mutex_init(&m_psPendLock, 0);
    }
    
    sleep(3);
    m_pFreeMem = pBlock;
    m_freeMemSize = (uint32_t)blockSize;

    m_base = event_init();
    if (! m_base) {
        fprintf(stderr, "Can't allocate event base\n");
        exit(1);
    }

    m_freeCurr = 0;
    m_connAllocCnt = 0;

    InitRespCmdTbl();

    m_recvBufRdIdx = 0;
    m_recvBufWrIdx = 0;
    m_recvUdpRdIdx = 0;
    m_recvUdpWrIdx = 0;
    m_recvBufWrHdrIdx = 0;
    m_recvBufWrPktIdx = CNY_RECV_BUF_HDR_BLOCK_SIZE;
    m_recvBufWrPktCnt = 0;
    m_bRecvBufSpaceAvail = true;
	
    m_pXmitConnFront = 0;
	m_pXmitConnBack = 0;
	m_xmitConnCnt = 0;

#ifdef MCD_NUMA
    m_pMcdUnit = g_pHtHif->AllocAuUnit(tid&1);
#else
    m_pMcdUnit = new CHtAuUnit(g_pHtHif);
#endif

    m_pRecvUdpBufBase = (CUdpReqInfo *)Malloc(MCD_RECV_UDP_SIZE * sizeof(CUdpReqInfo));
    m_pRecvBufBase = (char *)m_pMcdUnit->GetAuUnitMemBase();

#ifdef CHECK_USING_MODEL
    m_checkUnit.Init(tid, m_pRecvBufBase);
#endif

    m_pMcdUnit->SendHostMsg(MCD_RECV_BUF_BASE, (uint64_t)m_pRecvBufBase);
    m_pMcdUnit->SendCall_htmain();

    if (pthread_mutex_init(&g_stats.mutex, NULL) != 0) {
        perror("Failed to initialize mutex");
        exit(EXIT_FAILURE);
    }
}

// initialize the personality response command table
void CWorker::InitRespCmdTbl()
{
    for (int i = 0; i < 0x80; i += 1)
        m_pRespCmdTbl[i] = &CConn::ProcessCmdUnused;

    for (int i = 0x80; i < CNY_RSP_CMD_MAX; i += 1)
        m_pRespCmdTbl[i] = &CConn::ProcessCmdBinOther;

    m_pRespCmdTbl[CNY_CMD_SET] = &CConn::ProcessCmdStore;
    m_pRespCmdTbl[CNY_CMD_ADD] = &CConn::ProcessCmdStore;
    m_pRespCmdTbl[CNY_CMD_REPLACE] = &CConn::ProcessCmdStore;
    m_pRespCmdTbl[CNY_CMD_APPEND] = &CConn::ProcessCmdStore;
    m_pRespCmdTbl[CNY_CMD_PREPEND] = &CConn::ProcessCmdStore;
    m_pRespCmdTbl[CNY_CMD_CAS] = &CConn::ProcessCmdStore;
    m_pRespCmdTbl[CNY_CMD_DATA] = &CConn::ProcessCmdData;
    m_pRespCmdTbl[CNY_CMD_GET] = &CConn::ProcessCmdGet;
    m_pRespCmdTbl[CNY_CMD_GETS] = &CConn::ProcessCmdGet;
    m_pRespCmdTbl[CNY_CMD_GET_END] = &CConn::ProcessCmdGetEnd;
    m_pRespCmdTbl[CNY_CMD_DELETE] = &CConn::ProcessCmdDelete;
    m_pRespCmdTbl[CNY_CMD_INCR] = &CConn::ProcessCmdArith;
    m_pRespCmdTbl[CNY_CMD_DECR] = &CConn::ProcessCmdArith;
    m_pRespCmdTbl[CNY_CMD_TOUCH] = &CConn::ProcessCmdTouch;

    m_pRespCmdTbl[CNY_CMD_KEY] = &CConn::ProcessCmdKey;

    m_pRespCmdTbl[CNY_CMD_BIN_SET] = &CConn::ProcessCmdBinStore;
    m_pRespCmdTbl[CNY_CMD_BIN_SETQ] = &CConn::ProcessCmdBinStore;
    m_pRespCmdTbl[CNY_CMD_BIN_ADD] = &CConn::ProcessCmdBinStore;
    m_pRespCmdTbl[CNY_CMD_BIN_ADDQ] = &CConn::ProcessCmdBinStore;
    m_pRespCmdTbl[CNY_CMD_BIN_REPLACE] = &CConn::ProcessCmdBinStore;
    m_pRespCmdTbl[CNY_CMD_BIN_REPLACEQ] = &CConn::ProcessCmdBinStore;
    m_pRespCmdTbl[CNY_CMD_BIN_APPEND] = &CConn::ProcessCmdBinAppendPrepend;
    m_pRespCmdTbl[CNY_CMD_BIN_APPENDQ] = &CConn::ProcessCmdBinAppendPrepend;
    m_pRespCmdTbl[CNY_CMD_BIN_PREPEND] = &CConn::ProcessCmdBinAppendPrepend;
    m_pRespCmdTbl[CNY_CMD_BIN_PREPENDQ] = &CConn::ProcessCmdBinAppendPrepend;
    m_pRespCmdTbl[CNY_CMD_BIN_DATA] = &CConn::ProcessCmdBinData;
    m_pRespCmdTbl[CNY_CMD_BIN_GET] = &CConn::ProcessCmdBinGet;
    m_pRespCmdTbl[CNY_CMD_BIN_GETQ] = &CConn::ProcessCmdBinGet;
    m_pRespCmdTbl[CNY_CMD_BIN_GETK] = &CConn::ProcessCmdBinGet;
    m_pRespCmdTbl[CNY_CMD_BIN_GETKQ] = &CConn::ProcessCmdBinGet;
    m_pRespCmdTbl[CNY_CMD_BIN_DELETE] = &CConn::ProcessCmdBinDelete;
    m_pRespCmdTbl[CNY_CMD_BIN_DELETEQ] = &CConn::ProcessCmdBinDelete;
    m_pRespCmdTbl[CNY_CMD_BIN_INCR] = &CConn::ProcessCmdBinArith;
    m_pRespCmdTbl[CNY_CMD_BIN_INCRQ] = &CConn::ProcessCmdBinArith;
    m_pRespCmdTbl[CNY_CMD_BIN_DECR] = &CConn::ProcessCmdBinArith;
    m_pRespCmdTbl[CNY_CMD_BIN_DECRQ] = &CConn::ProcessCmdBinArith;
    m_pRespCmdTbl[CNY_CMD_BIN_TOUCH] = &CConn::ProcessCmdBinTouch;
    m_pRespCmdTbl[CNY_CMD_BIN_GAT] = &CConn::ProcessCmdBinTouch;
    m_pRespCmdTbl[CNY_CMD_BIN_GATQ] = &CConn::ProcessCmdBinTouch;
    m_pRespCmdTbl[CNY_CMD_BIN_GATK] = &CConn::ProcessCmdBinTouch;
    m_pRespCmdTbl[CNY_CMD_BIN_GATKQ] = &CConn::ProcessCmdBinTouch;

    m_pRespCmdTbl[CNY_CMD_OTHER] = &CConn::ProcessCmdOther;
    m_pRespCmdTbl[CNY_CMD_ERROR] = &CConn::ProcessCmdError;
}

/*
* Worker thread: main event loop
*/
void CWorker::StartEventLoop()
{
    /* Any per-thread setup can happen here; thread_init() will block until
    * all threads have finished initializing.
    */

    /* set an indexable thread-specific memory item for the lock type.
    * this could be unnecessary if we pass the CConn *c struct through
    * all item_lock calls...
    */
    m_itemLockType = ITEM_LOCK_GRANULAR;
    pthread_setspecific(item_lock_type_key, &m_itemLockType);

    RegisterThreadInitialized();

    // Allocate upd connections 
    SendPersUdpNewConns();

    // create a timer event to force partially filled packet buffer to be sent to coproc
    struct timeval t;
    t.tv_sec = 0;
    t.tv_usec = 0;

    evtimer_set(&m_timerEvent, WorkerThreadTimer, this);
    event_base_set(m_base, &m_timerEvent);
    evtimer_add(&m_timerEvent, &t);

    event_base_loop(m_base, 0);
}

void CWorker::SendPersUdpNewConns()
{
    // Initialize udp connections in personality, one per packet processing thread
    for (uint32_t i = 0; i < CNY_UNIT_UDP_CONN_CNT; i += 1) {

        uint16_t connId = 0x8000 | i;
        uint16_t pktSize = (negotiating_prot << 8) | udp_transport;

        AddPktToRecvBuffer(connId, pktSize);
    }

    FlushRecvBuf(true);
}

void CWorker::ProcessNewConn(CConn * c) {
    m_connList[c->m_connId] = c;

    if (c->m_transport == udp_transport)
        return;

    // inform coproc of new connection
    uint16_t connId = c->m_connId | 0x8000;
    uint16_t pktSize = ((c->m_transport == udp_transport ? negotiating_prot : g_settings.binding_protocol) << 8) | c->m_transport;

    AddPktToRecvBuffer(connId, pktSize);
}

void CWorker::EventHandler(const int fd, short which, void *arg) {
    CConn *	c;
    char const * pStr;
    int			sfd;
    socklen_t	addrlen;
    struct sockaddr_storage addr;
    int			flags;

    c = (CConn *)arg;
    assert(c != NULL);
    assert(!c->m_bFree);


    /* sanity */
    if (fd != c->m_sfd) {
        if (g_settings.m_verbose > 0)
            fprintf(stderr, "Catastrophic: event fd doesn't match conn fd!\n");
        if (c->m_pWorker)
            c->CloseConn();
        return;
    }

    if (c->m_pWorker) {
	c->m_pWorker->m_wakeCount++;
	
        // handle coproc messages
        //c->m_pWorker->m_pMcdUnit->RecvMsgLoop();

        // process response commands
        c->m_pWorker->ProcessRespBuf();

        // transmit packet from list of ready connections
        c->m_pWorker->TransmitFromList();

        // handle one dispatch command if available
        c->m_pWorker->HandleDispatchCmds();
    }

    if (which & EV_READ) {
        if (c->m_bListening) {
            addrlen = sizeof(addr);
            sfd = (int)accept(c->m_sfd, (struct sockaddr *)&addr, &addrlen);
            int err = errno;
            if (sfd == -1) {
                if (err == EAGAIN || err == EWOULDBLOCK) {
                    /* these are transient, so don't log anything */
                } else if (err == EMFILE) {
                    if (g_settings.m_verbose > 0)
                        fprintf(stderr, "Too many open connections\n");
                    AcceptNewConns(false);
                } else {
                    perror("accept()");
                }

            } else if ((flags = fcntl(sfd, F_GETFL, 0)) < 0 ||
                fcntl(sfd, F_SETFL, flags | O_NONBLOCK) < 0) {
                    perror("setting O_NONBLOCK");
                    close(sfd);

            } else if (g_settings.maxconns_fast &&
                g_stats.m_currConns + g_stats.m_reservedFds >= (uint32_t)g_settings.m_maxConns - 1) {
                    pStr = "ERROR Too many open connections\r\n";
                    write(sfd, pStr, (uint32_t)strlen(pStr));
                    close_socket(sfd);
                    __sync_fetch_and_add(&g_stats.m_rejectedConns, 1);
            } else {
	      dispatch_conn_new(sfd, false, EV_READ | EV_PERSIST,
                    DATA_BUFFER_SIZE, tcp_transport);
            }
        } else {
            //static uint32_t maxInUse = 0;
            //if (maxInUse < c->m_pWorker->GetXmitMsgInUseCnt()) {
            //    maxInUse = c->m_pWorker->GetXmitMsgInUseCnt();
            //    printf("max XmitMsg in use: %d\n", maxInUse);
            //}

            g_pTimers[6]->Start(c->m_pWorker->m_unitId);

            if (c->m_pWorker->GetXmitMsgInUseCnt() < XMIT_MSG_IN_USE_LIMIT) {
                bool bPktRecved = c->m_pWorker->RecvPkt(c);
                if (!bPktRecved)
                    g_pSamples[1]->Sample(0, c->m_pWorker->m_unitId);
            } else if (g_settings.m_cnyVerbose > 0)
                printf("RecvPkt throttled by XMIT_MSG_IN_USE_LIMIT(%d)\n", XMIT_MSG_IN_USE_LIMIT);
    
            g_pTimers[6]->Stop(c->m_pWorker->m_unitId);


        }
    } else
        assert(0);
}

bool CWorker::RecvPkt(CConn * c)
{
    if (!m_bRecvBufSpaceAvail && !IsRecvBufSpaceAvail())
        // all buffer space is in use, just return - events are persistent
        return false;

    //printf("  Recv wrIdx = 0x%x, rdIdx = 0x%x\n", m_recvBufWrIdx, m_recvBufRdIdx);

    int pktSize;
    char * pRecvBufPkt = &m_pRecvBufBase[(m_recvBufWrIdx & MCD_RECV_BUF_SIZE_MASK) + m_recvBufWrPktIdx];
    int maxRecvLen = (int) min((int)(MCD_RECV_BUF_MAX_BLK_SIZE - m_recvBufWrPktIdx - CNY_RECV_BUF_HDR_BLOCK_SIZE), (int)(0x1000-1));
    assert(m_recvBufWrPktIdx + maxRecvLen <= MCD_RECV_BUF_SIZE);

    //int maxRecvLen = c->m_transport == udp_transport ? (CNY_RECV_BUF_SIZE - m_recvBufWrPktIdx-1) : 2000;

    uint16_t connId;
    if (c->m_transport == udp_transport) {

        CUdpReqInfo & udpReqInfo = m_pRecvUdpBufBase[(m_recvUdpWrIdx & MCD_RECV_UDP_SIZE_MASK) + m_recvUdpWrPktCnt];

        udpReqInfo.m_udpReqAddrSize = sizeof(udpReqInfo.m_udpReqAddr);

        pktSize = recvfrom(c->m_sfd, pRecvBufPkt, maxRecvLen, 0, (sockaddr *) &udpReqInfo.m_udpReqAddr, &udpReqInfo.m_udpReqAddrSize);

        if (pktSize == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
            return false;

        if (pktSize <= UDP_HEADER_SIZE)
            return false;

        udpReqInfo.m_udpReqId = (pRecvBufPkt[0] << 8) | (uint8_t)pRecvBufPkt[1];
        int cmdPktCnt = (pRecvBufPkt[4] << 8) | pRecvBufPkt[5];

        if (cmdPktCnt != 1) {
            // inbound UDP commands must reside in a single packet
            char const *pStr = "SERVER_ERROR multi-packet request not supported\r\n";
            int len = (int)strlen(pStr);
            sendto(c->m_sfd, pStr, len, 0, (sockaddr *) &udpReqInfo.m_udpReqAddr, udpReqInfo.m_udpReqAddrSize);
            return false;
        }

        uint32_t udpHash = UdpReqAddrHash(&udpReqInfo.m_udpReqAddr, udpReqInfo.m_udpReqAddrSize);

        int recvUdpIdx = (m_recvUdpWrIdx & MCD_RECV_UDP_SIZE_MASK) + m_recvUdpWrPktCnt;
        assert(recvUdpIdx <= 0x3ff);
        connId = 0x4000 | (udpHash << 10) | recvUdpIdx;
        //printf("  Recv hash = 0x%x, connId = 0x%x, udpIdx = 0x%x\n", udpHash, c->m_connId + udpHash, recvUdpIdx);
        m_recvUdpWrPktCnt += 1;
        udpReqInfo.m_connId = c->m_connId + udpHash;

    } else {
        pktSize = recv(c->m_sfd, pRecvBufPkt, maxRecvLen, 0);
        connId = c->m_connId;
    }

    if (pktSize <= 0) {
        if (pktSize < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
            return false;

        // setting packet size to zero will result in the coproc sending a command back to close the connection
        pktSize = 0;

        if (!c->m_bCloseReqSent && !c->IsUdp()) {
            // remove event to avoid getting future notifications, we free resources when coproc responds
            c->CloseConn();
            c->m_caxCode = 7;

            c->SendCloseToCoproc();
        }

    } else if (!c->m_bCloseReqSent) {

        m_stats.m_bytesRead += pktSize;

        AddPktToRecvBuffer(connId, pktSize);
    }

    return true;
}

int CWorker::AddCloseMsgToRecvBuffer()
{
    int rc = 0;
    while ((m_bRecvBufSpaceAvail || IsRecvBufSpaceAvail()) && !m_closeMsgQue.IsEmpty()) {
        AddPktToRecvBuffer(m_closeMsgQue.Front(), 0);
        m_closeMsgQue.Pop();
	rc = 1;
    }
    return rc;
}

void CWorker::AddPktToRecvBuffer(uint16_t connId, size_t pktSize)
{
    if (m_recvBufWrPktCnt == 0)
        m_recvBufFillTimer = CTimer::Rdtsc() + g_maxRecvBufFillClks;

    // add packet header to receive buffer
    *(uint16_t *)&m_pRecvBufBase[(m_recvBufWrIdx & MCD_RECV_BUF_SIZE_MASK) + m_recvBufWrHdrIdx + 0] = connId;
    *(uint16_t *)&m_pRecvBufBase[(m_recvBufWrIdx & MCD_RECV_BUF_SIZE_MASK) + m_recvBufWrHdrIdx + 2] = (uint16_t)pktSize;

    assert(m_recvBufWrPktIdx >= 40);

    m_recvBufWrHdrIdx += 4;
    m_recvBufWrPktCnt += 1;

    if ((connId & 0x8000) == 0)
        m_recvBufWrPktIdx += (pktSize + CNY_RECV_BUF_ALIGN-1) & ~(CNY_RECV_BUF_ALIGN-1);

    assert(m_recvBufWrPktIdx <= MCD_RECV_BUF_MAX_BLK_SIZE);

    if ((m_recvBufWrPktCnt % CNY_RECV_BUF_HDR_CNT) == 0) {
        // filled current header area, start a new one
        m_recvBufWrHdrIdx = m_recvBufWrPktIdx;
        m_recvBufWrPktIdx += CNY_RECV_BUF_HDR_BLOCK_SIZE;
    }

    if (m_recvBufWrPktIdx > MCD_RECV_BUF_MAX_BLK_SIZE - MCD_RECV_BUF_MIN_PKT_SIZE ||
        m_recvBufWrPktCnt >= MCD_RECV_BUF_MAX_PKT_CNT) {
        FlushRecvBuf(true);
        return;
    }
}

uint32_t CWorker::UdpReqAddrHash(void * pUdpReqAddr, int udpReqSize) {
    uint64_t hash = *(uint64_t *)pUdpReqAddr; // 64-bit value
    hash = hash ^ (hash >> 32);
    hash = hash ^ (hash >> 16);
    hash = hash ^ (hash >> 8);
    hash = hash ^ (hash >> 4);
    return hash & 0xf;
}

int CWorker::FlushRecvBuf(bool bFull)
{
    if (m_recvBufWrPktCnt == 0 || (!bFull && m_recvBufFillTimer > CTimer::Rdtsc()))
        return 0;	// Nothing to do.

    m_bRecvBufSpaceAvail = false;
    assert(m_recvBufWrPktIdx <= MCD_RECV_BUF_MAX_BLK_SIZE);

    // zero unused header entries
    *(uint32_t *)&m_pRecvBufBase[(m_recvBufWrIdx & MCD_RECV_BUF_SIZE_MASK) + m_recvBufWrHdrIdx] = 0xffff0000;   // pktLen = 0xffff

#ifdef CHECK_USING_MODEL
    m_checkRecvBufLenQue.Push(m_recvBufWrPktIdx);
#endif

    assert(m_recvBufWrPktIdx <= MCD_RECV_BUF_MAX_BLK_SIZE);
    assert((m_recvBufWrIdx & MCD_RECV_BUF_SIZE_MASK) + m_recvBufWrPktIdx <= MCD_RECV_BUF_SIZE);

    bool bRecvBufRestart = MCD_RECV_BUF_SIZE - (m_recvBufWrIdx & MCD_RECV_BUF_SIZE_MASK) - m_recvBufWrPktIdx < MCD_RECV_BUF_MAX_BLK_SIZE;
    bool bRecvUdpRestart = MCD_RECV_UDP_SIZE - (m_recvUdpWrIdx & MCD_RECV_UDP_SIZE_MASK) - m_recvUdpWrPktCnt < MCD_RECV_UDP_MIN_BLK_SIZE;

    m_recvBlkSizeQue.Push(CRecvBlkSize(bRecvBufRestart, m_recvBufWrPktIdx, bRecvUdpRestart, m_recvUdpWrPktCnt, CTimer::Rdtsc()));

    uint64_t msgData = ((int64_t)bRecvBufRestart << 48) | ((int64_t)(m_recvBufWrPktCnt & 0xffff) << 32) | (m_recvBufWrIdx & MCD_RECV_BUF_SIZE_MASK);
    m_pMcdUnit->SendHostMsg(CTL_BLK_PUSH, msgData);

    m_recvBufWrIdx += m_recvBufWrPktIdx;
    m_recvUdpWrIdx += m_recvUdpWrPktCnt;

    if (bRecvBufRestart)
        m_recvBufWrIdx = (m_recvBufWrIdx + MCD_RECV_BUF_SIZE_MASK) & ~MCD_RECV_BUF_SIZE_MASK;
    if (bRecvUdpRestart)
        m_recvUdpWrIdx = (m_recvUdpWrIdx + MCD_RECV_UDP_SIZE_MASK) & ~MCD_RECV_UDP_SIZE_MASK;

    m_recvBufWrHdrIdx = 0;
    m_recvBufWrPktIdx = CNY_RECV_BUF_HDR_BLOCK_SIZE;

    m_recvUdpWrPktCnt = 0;
    m_recvBufWrPktCnt = 0;

    if (g_settings.m_cnyVerbose > 0) {
        static int maxRecvBufInUse[8] = { 0 };
        static int maxRecvBufFill[8] = { 0 };

        int inUse = RecvBufInUse();
        if (inUse > maxRecvBufInUse[m_unitId]) {
            maxRecvBufInUse[m_unitId] = inUse;
            printf("tid %d, maxRecvBufInUse %d\n", m_unitId, maxRecvBufInUse[m_unitId]);
        }

        if (m_recvBufWrPktIdx > maxRecvBufFill[m_unitId]) {
            maxRecvBufFill[m_unitId] = m_recvBufWrPktIdx;
            printf("tid %d, maxRecvBufFill %d\n", m_unitId, maxRecvBufFill[m_unitId]);
        }
    }
    return 1;
}

int CWorker::ProcessRespBuf()
{
    int maxCmdCnt = 100;
    int cmdCnt = 0;

    for (;;) {

        if (m_rspCmdQwCnt == 0  &&
                m_pMcdUnit->RecvHostData(1, &m_rspCmdQw.m_rspQw[0]) != 1)
            return cmdCnt;
        else if (m_rspCmdQwCnt == 0)
            m_rspCmdQwCnt = 1;

#ifdef CHECK_USING_MODEL
        bool bCheckRetry = false;
        if (m_checkUnit.m_bCheckRespBuf) {
            m_checkUnit.m_bCheckRespBuf = false;

            int recvBufLen = m_checkRecvBufLenQue.Front();
            m_checkRecvBufLenQue.Pop();

            m_checkUnit.ResetCheckBuf();
            m_checkUnit.ProcessRecvBuf(m_recvBufRdIdx & MCD_RECV_BUF_SIZE_MASK, recvBufLen);
        }

        bCheckRetry = m_checkUnit.CheckRespCmd(&m_rspCmdQw);
        if (bCheckRetry)
            return cmdCnt;
#endif

        uint8_t rspCmd = m_rspCmdQw.m_rspCmd.m_cmd;
        if (rspCmd == CNY_CMD_NXT_RCV_BUF) {

            assert(m_rspCmdQwCnt == 1);
			m_rspCmdQwCnt = 0;

            CRecvBlkSize recvBlkSize = m_recvBlkSizeQue.Front();
            m_recvBlkSizeQue.Pop();

            if (recvBlkSize.m_bBufRestart)
                m_recvBufRdIdx = (m_recvBufRdIdx + MCD_RECV_BUF_SIZE_MASK) & ~MCD_RECV_BUF_SIZE_MASK;
            else
                m_recvBufRdIdx += (int)recvBlkSize.m_blkSize;

            if (recvBlkSize.m_bUdpRestart)
                m_recvUdpRdIdx = (m_recvUdpRdIdx + MCD_RECV_UDP_SIZE_MASK) & ~MCD_RECV_UDP_SIZE_MASK;
            else
                m_recvUdpRdIdx += (int)recvBlkSize.m_udpBlkSize;

            g_pTimers[4]->Sample(CTimer::Rdtsc() - recvBlkSize.m_flushTime, m_unitId);
            continue;
        }

        uint16_t rspConnId = m_rspCmdQw.m_rspCmd.m_connId;
        uint16_t connId;
        CConn * c;
        if (rspConnId & 0x4000) {
            assert(MCD_RECV_UDP_SIZE <= 1024);
            CUdpReqInfo * pRspUdpReqInfo = &m_pRecvUdpBufBase[rspConnId & MCD_RECV_UDP_SIZE_MASK];
            connId = pRspUdpReqInfo->m_connId;
            c = m_connList[connId];
            c->m_pRspUdpReqInfo = pRspUdpReqInfo;
        } else {
            connId = rspConnId;
            c = m_connList[connId];
            c->m_pRspUdpReqInfo = 0;
        }

        assert(c->m_bFree == false);
        assert(connId < CNY_MAX_UNIT_CONN_CNT);

        bool (CConn::*pFunc)();
        pFunc = m_pRespCmdTbl[rspCmd];

        bool bFuncRtn = ((*c).*pFunc)();

        if (! bFuncRtn )
            break;

        cmdCnt += 1;
        if (cmdCnt >= maxCmdCnt)
            break;
    }

    return cmdCnt;
}

CRspCmdQw * CWorker::GetHifRspCmd(int qwCnt)
{
    if (m_rspCmdQwCnt < qwCnt)
        m_rspCmdQwCnt += m_pMcdUnit->RecvHostData(qwCnt - m_rspCmdQwCnt, m_rspCmdQw.m_rspQw + m_rspCmdQwCnt);

    if (m_rspCmdQwCnt < qwCnt)
		// retry later
        return 0;
	else {
		m_rspCmdQwCnt = 0;
		return &m_rspCmdQw;
	}
}

void CWorker::LinkConnForXmit(CConn * pConn)
{
    if (pConn->m_bXmitConnActive)
        return;
        
    pConn->m_bXmitConnActive = true;
    pConn->m_pXmitConnNext = 0;
 
    if (m_pXmitConnFront == 0)
		m_pXmitConnFront = m_pXmitConnBack = pConn;
	else
		m_pXmitConnBack = m_pXmitConnBack->m_pXmitConnNext = pConn;

	m_xmitConnCnt += 1;
}

int CWorker::TransmitFromList()
{
    int rc = 0;
    if (m_pXmitConnFront) {
	rc = 1;
        m_pXmitConnFront->Transmit();

        // remove from list
        CConn * pXmitConn = m_pXmitConnFront;
        m_pXmitConnFront = pXmitConn->m_pXmitConnNext;
        m_xmitConnCnt -= 1;
        pXmitConn->m_bXmitConnActive = false;

        if (pXmitConn->m_pXmitMsgFront) {
            // more to transmit, relink at back
            LinkConnForXmit(pXmitConn);
        }
    }
    return rc;
}

void * CWorker::Malloc(size_t size)
{
    if (size < m_freeMemSize) {
        uint8_t * pMem = m_pFreeMem;
        m_pFreeMem += size;
        m_freeMemSize -= (uint32_t)size;
        return pMem;
    } else {
        if (m_freeMemSize > 4096)
            fprintf(stderr, "Warning Malloc() wasted 0x%x bytes memory\n", m_freeMemSize);

        assert(size <= WORKER_BLOCK_ALLOC_SIZE);
        m_pFreeMem = (uint8_t *) AlignedMmap(WORKER_BLOCK_ALLOC_ALIGN, WORKER_BLOCK_ALLOC_SIZE);

        if (m_pFreeMem == 0) {
            m_freeMemSize = 0;
            return 0;
        }

        m_freeMemSize = WORKER_BLOCK_ALLOC_SIZE;
        return Malloc(size);
    }
}

void * CWorker::Calloc(int cnt, size_t size)
{
    size_t totalSize = cnt * size;
    void * pMem = Malloc(totalSize);
    if (pMem == 0)
        return 0;
    memset(pMem, 0, totalSize);
    return pMem;
}

CMsgHdr * CWorker::AllocMsgHdr()
{
    CMsgHdr * pMsgHdr;
    if (m_pMsgHdrPool) {
        pMsgHdr = m_pMsgHdrPool;
        m_pMsgHdrPool = m_pMsgHdrPool->m_pNext;
        m_msgHdrPoolCnt -= 1;
    } else {
        pMsgHdr = (CMsgHdr *) Malloc(sizeof(CMsgHdr));
        m_msgHdrAllocCnt += 1;
    }

    return pMsgHdr;
}

void CWorker::FreeMsgHdr(CMsgHdr * pMsgHdr)
{
    assert(pMsgHdr->m_pItem == 0);
    pMsgHdr->m_pNext = m_pMsgHdrPool;
    m_pMsgHdrPool = pMsgHdr;
    m_msgHdrPoolCnt += 1;
}

CXmitMsg * CWorker::CallocXmitMsg()
{
    CXmitMsg * pXmitMsg;
    if (m_pXmitMsgPool) {
        pXmitMsg = m_pXmitMsgPool;
        m_pXmitMsgPool = m_pXmitMsgPool->m_pNext;
        m_xmitMsgPoolCnt -= 1;
    } else {
        pXmitMsg = (CXmitMsg *) Malloc(sizeof(CXmitMsg));
        m_xmitMsgAllocCnt += 1;
    }

    if (pXmitMsg)
        memset(pXmitMsg, 0, sizeof(CXmitMsg));

    return pXmitMsg;
}

void CWorker::FreeXmitMsg(CXmitMsg * pXmitMsg)
{
    pXmitMsg->m_pNext = m_pXmitMsgPool;
    m_pXmitMsgPool = pXmitMsg;
    m_xmitMsgPoolCnt += 1;
}

/*
* Free list management for connections.
*/

/*
* Returns a connection from the freelist, if any.
*/
CConn *CWorker::ConnFromFreelist() {
    CConn *c;

    if (m_freeCurr > 0) {
        c = m_freeConns[--m_freeCurr];
        assert(c->m_sfd == 0);
        assert(c->m_bFree == true);
        c->m_bFree = false;
    } else {
        c = NULL;
    }

    return c;
}

/*
* Adds a connection to the freelist. 0 = success.
*/
void CWorker::ConnAddToFreelist(CConn *c) {
    assert(c->m_sfd == 0);
    assert(c->m_bFree == false);
    assert(c->m_bEventActive == false);
    m_freeConns[m_freeCurr++] = c;
    c->m_bFree = true;
}
