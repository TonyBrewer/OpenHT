/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#pragma once

struct CSyscTop;

namespace Ht {
	class CHtHifLib;
	class CHtHifUnitMap;

	extern int g_htDebug;
	extern struct CHtCtrlMsg *g_pCtrlIntfQues[];

	void * AlignedMmap(uint32_t align, uint32_t size);

#define HT_HIF_MEM_BLK_IN 0
#define HT_HIF_MEM_BLK_OUT 1
#define HT_HIF_MEM_APP_UNIT 2
#define HT_HIF_MEM_UNIT 3
#define HT_HIF_MEM_REGIONS 4

	class CHtHifMem : public CHtHifLibBase {
	public:
		CHtHifMem() {
			m_pHtHifBase = 0;
			m_pMemRegionList = 0;
			m_pHtHifMem = 0;
		}
		~CHtHifMem() {
			if (m_pHtHifMem) {
#if !defined(CNYOS_API) && !defined(WIN32)
				if (m_bHtHifMemHuge) {
					munlock(m_pHtHifMem, HUGE_PAGE_SIZE);
					if (munmap(m_pHtHifMem, HUGE_PAGE_SIZE) != 0)
						 fprintf(stderr, "HTLIB: Host huge page free failed (0x%llx)\n", (long long)m_pHtHifMem);
				} else
#endif
					ht_free_memalign(m_pHtHifMem);
				m_pHtHifMem = 0;
			}
			if (m_pMemRegionList) {
				delete[] m_pMemRegionList;
				m_pMemRegionList = 0;
			}
		}

		void AllocHifMem(CHtHifLib * pHtHifBase, int unitCnt, size_t unitSize);

		uint8_t * GetAppPersMemBase();
		uint8_t * GetFlushLinesBase();
		uint8_t * GetCtlQueMemBase(int aeIdx);
		uint8_t * GetCtlInQueMemBase(int unitId);
		uint8_t * GetCtlOutQueMemBase(int unitId);
		uint8_t * GetInBlkBase(int unitId);
		uint8_t * GetOutBlkBase(int unitId);
		uint8_t * GetAppUnitMemBase(int unitId);
		uint8_t * GetUnitMemBase(int unitId);

		void DumpState(FILE *fp);

		void InitHifMemory();

	private:
		 class CHtHifMemRegion {
		 public:
			 CHtHifMemRegion() {
				 m_sortKey = 0;
				 m_offset = 0;
				 m_bCleared = false;
				 m_pName = 0;
			 }
			 ~CHtHifMemRegion() {
				 if (m_pName) {
					 free((void *)m_pName);
					 m_pName = 0;
				 }
			 }

			 void InitRegion(char const * pName, uint64_t size, uint8_t numaSet) {
				 m_pName = (char const *)malloc(strlen(pName)+1);
				 strcpy((char *)m_pName, pName);
				 m_size = size;
				 m_numaSet = numaSet;
			 }

			 void SetMemOffset(uint64_t offset) { m_offset = offset; }

			 char const * GetName() { return m_pName; }
			 uint64_t GetMemSize() { return m_size; }
			 uint64_t GetMemOffset() { return m_offset; }
			 uint8_t GetNumaSet() { return m_numaSet; }
			 uint64_t GetSortKey() { return m_sortKey; }

			 void ClearMemRegion();

			 static void SetHtHifBase(CHtHifLib * pHtHifBase) { m_pHtHifBase = pHtHifBase; }
			 static void SetHtHifMemBase(uint8_t * pHtHifMemBase) { m_pHtHifMemBase = pHtHifMemBase; }

			 uint8_t * GetMemBase() { return m_pHtHifMemBase + m_offset; }

		 private:
			 union {
				 struct {
					 uint64_t m_size:56;
					 uint64_t m_numaSet:8;
				 };
				 uint64_t m_sortKey;
			 };
			 uint64_t m_offset:56;
			 uint64_t m_bCleared:8;
			 char const * m_pName;
			 static CHtHifLib * m_pHtHifBase;
			 static uint8_t * m_pHtHifMemBase;
		 };

	private:
		CHtHifLib * m_pHtHifBase;
		int m_aeCnt;

		int m_memRegionCnt;
		CHtHifMemRegion * m_pMemRegionList;

		size_t m_htHifMemSize;
		uint8_t * m_pHtHifMem;
		bool m_bHtHifMemHuge;

		uint32_t m_ctlQueMemSize;
		uint32_t m_ctlInQueMemSize;
		uint32_t m_ctlOutQueMemSize;
		uint32_t m_inBlkMemSize;
		uint32_t m_outBlkMemSize;
		uint64_t m_appUnitMemSize;
		uint32_t m_unitMemSize;
		uint32_t m_flushLinesMemSize;
	};

	class CHtHifMonitor {
	public:
		CHtHifMonitor() {
			m_lock = 0;
		}

		void Clear() {
			m_callCnt = 0;
			m_rtnCnt = 0;
			m_ihmCnt = 0;
			m_ohmCnt = 0;
			m_ohdBytes = 0;
			m_ihdBytes = 0;
		}

		void operator += (CHtHifMonitor &rhs) {
			m_callCnt += rhs.m_callCnt;
			m_rtnCnt += rhs.m_rtnCnt;
			m_ihmCnt += rhs.m_ihmCnt;
			m_ohmCnt += rhs.m_ohmCnt;
			m_ohdBytes += rhs.m_ohdBytes;
			m_ihdBytes += rhs.m_ihdBytes;
		}

		void IncCallCnt() { Lock(); m_callCnt += 1; Unlock(); }
		void IncRtnCnt() { Lock(); m_rtnCnt += 1; Unlock(); }
		void IncIhmCnt() { Lock(); m_ihmCnt += 1; Unlock(); }
		void IncOhmCnt() { Lock(); m_ohmCnt += 1; Unlock(); }
		void IncOhdBytes(int bytes) { Lock(); m_ohdBytes += bytes; Unlock(); }
		void IncIhdBytes(int bytes) { Lock(); m_ihdBytes += bytes; Unlock(); }

		int GetCallCnt() { return m_callCnt; }
		int GetRtnCnt() { return m_rtnCnt; }
		int GetIhmCnt() { return m_ihmCnt; }
		int GetOhmCnt() { return m_ohmCnt; }
		uint64_t GetIhdBytes() { return m_ihdBytes; }
		uint64_t GetOhdBytes() { return m_ohdBytes; }

		void Lock() { while (__sync_fetch_and_or(&m_lock, 1) != 0); }
		void Unlock() { m_lock = 0; __sync_synchronize(); }

	private:
		int m_callCnt;
		int m_rtnCnt;
		int m_ihmCnt;
		int m_ohmCnt;
		uint64_t m_ohdBytes;
		uint64_t m_ihdBytes;

		int64_t volatile m_lock;
	};

	#define HT_HIF_UNIT_CNT_MAX 64
	#define HT_HIF_AU_CNT_MAX 16

	class CHtHifUnitMap {
	public:
		CHtHifUnitMap() {
			memset(this, 0, sizeof(*this));
		}

		void SetAeCnt(int aeCnt) {
			assert(aeCnt <= HT_HIF_AE_CNT_MAX);
			m_aeCnt = aeCnt;
		}

		void SetAuCnt(int aeId, int auCnt) {
			assert(aeId < m_aeCnt);
			assert(auCnt <= HT_HIF_AU_CNT_MAX);
			m_auCnt[aeId] = auCnt;
		}

		int GetAeCnt() { return m_aeCnt; }
		int GetUnitCnt() { return m_unitCnt; }
		int GetAuCnt(int aeId) { return m_auCnt[aeId]; }

		void Insert(int unitId, int aeId, int auId, int numaSet) {
			assert(unitId < HT_HIF_UNIT_CNT_MAX);
			assert(aeId < HT_HIF_AE_CNT_MAX);
			assert(auId < HT_HIF_AU_CNT_MAX);

			m_unitCnt += 1;
			m_maxUnitId = max(m_maxUnitId, unitId);

			m_unitMap[aeId][auId] = unitId;
			m_aeAuMap[unitId].m_ae = aeId;
			m_aeAuMap[unitId].m_au = auId;
			m_numaSet[unitId] = numaSet;
		}

		void FindAeAu(int unitId, int &ae, int &au) {
			assert(unitId < HT_HIF_UNIT_CNT_MAX);
			ae = m_aeAuMap[unitId].m_ae;
			au = m_aeAuMap[unitId].m_au;
		}

		void FindUnitId(int ae, int au, int &unitId) {
			assert(ae < m_aeCnt);
			assert(au < HT_HIF_AU_CNT_MAX);
			unitId = m_unitMap[ae][au];
		}

		int GetNumaSet(int unitId) {
			return m_numaSet[unitId];
		}

	private:
		struct CAeAu {
			int m_ae;
			int m_au;
		};

	private:
		int		m_aeCnt; // total number of AEs
		int		m_unitCnt; // total number of units
		int		m_maxUnitId;
		int		m_auCnt[HT_HIF_AE_CNT_MAX]; // number of units in each AE
		CAeAu	m_aeAuMap[HT_HIF_UNIT_CNT_MAX]; // map from unitId to aeId/auId
		int		m_unitMap[HT_HIF_AE_CNT_MAX][HT_HIF_AU_CNT_MAX]; // map from aeId/auId to unitId
		int		m_numaSet[HT_HIF_UNIT_CNT_MAX];	// numa set for each unit
	};	

	class CHtHifLib;

	class CHtUnitLib : public CHtUnitLibBase {
		friend class CHtHifLib;

	public:
		int GetUnitId() { return m_unitId; }

	public:
		CHtUnitLib() {
			m_pHtHifLib = 0;
			m_pHtUnitBase = 0;
			m_pCtlInQue = 0;
			m_pCtlOutQue = 0;
			m_pOutCtlMsg = 0;
			m_pInBlk = 0;
			m_pInBlkBase = 0;
			m_pOutBlk = 0;
			m_pOutBlkBase = 0;
		}
		CHtUnitLib(CHtHifLib *pHtHif, int unitId);

		virtual ~CHtUnitLib() {}

		void SetUsingCallback(bool bUsingCallback) { 
			m_bUsingCallback = bUsingCallback;
		}

		ERecvType RecvPoll(bool bUseCb=true) { return MsgLoop(bUseCb); }

		void SendHostMsg(uint8_t msgType, uint64_t msgData);
		bool RecvHostMsg(uint8_t & msgType, uint64_t & msgData);

		bool PeekHostData(uint64_t & data);
		int RecvHostData(int qwCnt, uint64_t * pData);
		bool RecvHostDataMarker();
		int SendHostData(int qwCnt, uint64_t * pData);
		bool SendHostDataMarker();
		bool SendHostHalt();
		void FlushHostData() { FlushInBlk(true); }

		bool SendCall(int argc, int * pArgw, uint64_t * pArgv);
		bool RecvReturn(int argc, uint64_t * pArgv);

		uint8_t * GetUnitMemBase();

	private:
		void SendMsgInternal(uint8_t msgType, uint64_t msgData, bool bLock = false, int argc = 0, int * pArgw = 0, uint64_t * pArgv = 0);

		void Callback(ERecvType recvType) {
			m_pHtUnitBase->RecvCallback(recvType);
		}

		bool RecvMsgInternal(uint8_t & msgType, uint64_t & msgData);

		int SendDataInternal(int qwCnt, uint64_t * pData, bool bLock=false);

		void HaltLoop();
		ERecvType MsgLoop(bool bUseCb=false);

		void FlushInBlk(bool bLock=false, int ctl=0);
		void QueOutMsgs();
		void NextOutMsg();
		void NextOutMsgQue() {
			m_outMsgQue.Pop();
			m_outMsgQueLoopCnt = 0;
	#ifdef HT_AVL_TEST
			m_bOutBlkRdySeen = false;
	#endif
		}
		void NextInBlk();
		void NextOutBlk(bool bLock=false);

		void SendMsgDebug(uint8_t msgType, uint64_t msgData, int argc = 0, int * pArgw = 0, uint64_t * pArgv = 0);
		void RecvMsgDebug(uint8_t msgType, uint64_t msgData);

		void InBlkTimer();

		void LockInBlk(bool bLock=true) { while (bLock && __sync_fetch_and_or(&m_inLock, 1) != 0) 
							; assert(m_inLock != 0); }
		void UnlockInBlk(bool bUnlock=true) { if (bUnlock) { m_inLock = 0; __sync_synchronize(); } }

	private:
		CHtHifLib * m_pHtHifLib;
		CHtUnitBase * m_pHtUnitBase;
		int m_unitId;
		uint32_t m_ctlQueMask;
		bool m_bUsingCallback;
		bool m_bFree;

		// Inbound control queue
		CHtCtrlMsg * m_pCtlInQue;
		int volatile m_ctlInWrIdx;

		// Outbound control queue
		CHtCtrlMsg * m_pCtlOutQue;
		uint32_t m_outCtlRdIdx;
		CHtCtrlMsg * m_pOutCtlMsg;
		CQueue<CHtCtrlMsg, 512> m_outMsgQue;
		uint32_t m_outMsgQueLoopCnt;
		int m_recvRtnArgc;

		// Inbound data queue
		int volatile m_inBlkAvailCnt;
		int volatile m_inBlkWrIdx;
		int volatile m_inBlkIdx;
		int m_inBlkSize;
		int m_inBlkMask;
		uint64_t * volatile m_pInBlk;
		uint64_t * m_pInBlkBase;
		int64_t volatile m_inLock;

		// Outbound data queue
		int m_outBlkDataSize;
		int m_outBlkRdIdx;
		int m_outBlkIdx;
		int m_outBlkSize;
		int m_outBlkMask;
		uint64_t * m_pOutBlk;
		uint64_t * m_pOutBlkBase;

		// Activity reporting
		CHtHifMonitor m_monitor;

	#ifdef HT_AVL_TEST
		uint32_t volatile m_inAvlMask;
		uint32_t volatile m_outRdyMask;
		bool m_bOutBlkRdySeen;
	#endif
	};

	class CHtHifLib : public CHtHifMem {
		friend class CHtUnitLib;
		friend class CHtHifMem;
	public:
		int GetUnitMHz() { return m_unitMhz; }
		uint64_t GetPartNumber() { return m_partNumber; }

		void StartCoproc() {
			if (!m_bCoprocDispatched)
				DispatchCoproc();
		}

	public:
		CHtHifLib(CHtHifParams * pParams, char const * pHtPers, CHtHifBase * pHtHifBase);
		~CHtHifLib();

		int GetUnitCnt() { return m_aeCnt * m_auCnt; }
		int GetNeedFlush() { return m_htNeedFlush; }
		const CHtHifParams & GetHifParams() { return m_htHifParams; }

		void FreeAllUnits() {
			for (int unitId = 0; unitId < GetUnitCnt(); unitId += 1)
				m_ppHtUnits[unitId]->m_bFree = true;
		}

		CHtUnitLib * AllocUnit(CHtUnitBase * pHtUnitBase, int numaSet);

		void FreeUnit(CHtUnitLibBase * pUnit);

		void InitUnit(int unitId);

		void SendAllHostMsg(uint8_t msgType, uint64_t msgData) {
			for (int i = 0; i < GetUnitCnt(); i += 1) {
				if (m_ppHtUnits[i] == 0)
					InitUnit(i);
				m_ppHtUnits[i]->SendHostMsg(msgType, msgData);
			}
		}

	private:
		void DispatchCoproc();

		void CoprocThread();
		static void *CoprocThreadStart(void *pThis) {
			((CHtHifLib*)pThis)->CoprocThread();
			return 0;
		}

		void HostTimerThread();
		static void *HostTimerThreadStart(void *pThis) {
			((CHtHifLib*)pThis)->HostTimerThread();
			return 0;
		}

		void MonitorThread();
		static void *MonitorThreadStart(void *pThis) {
			((CHtHifLib*)pThis)->MonitorThread();
			return 0;
		}

		void FormatHostDataRate(char * pHdStr, uint64_t hdBytes, int periodUsec);

		void LockAlloc() { while (__sync_fetch_and_or(&m_allocLock, 1) != 0); }
		void UnlockAlloc() { m_allocLock = 0; __sync_synchronize(); }

	public:
		CHtHifUnitMap * GetUnitMap() { return &m_unitMap; }

	protected:
		int m_aeCnt;
		int m_auCnt;
		CHtHifBase * m_pHtHifBase;
		CHtUnitLib ** m_ppHtUnits;
		volatile int64_t m_allocLock;
		CHtHifUnitMap m_unitMap;

		bool m_bCoprocDispatched;
		volatile bool m_bCoprocBusy;
		uint64_t m_partNumber;
		int m_unitMhz;
		int m_abi;
		bool m_htNeedFlush;
		uint64_t m_args[HT_HIF_AE_CNT_MAX];

		CHtHifParams m_htHifParams;
		EAppMode m_appMode;

		bool volatile m_bHaltSent;
		bool volatile m_bTimerHaltSeen;
		bool volatile m_bMonitorHaltSeen;

		CSyscTop * m_pSyscTop;
	};

	CHtUnitLib * AllocUnit(CHtHifLib * pHtHif, int numaSet);

} // namespace Ht
