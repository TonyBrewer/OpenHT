/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#define HT_LIB_HIF

#include "Ht.h"
#include "HtHifLib.h"
#include "HtModel.h"

#include <string>
using namespace std;

#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif

namespace Ht {
	// systemc variables
	CHtCtrlMsg *g_pCtrlIntfQues[HT_HIF_AE_CNT_MAX];
	int32_t g_coprocAeRunningCnt;

	int g_htDebug = getenv("CNY_HTDEBUG") ? atoi(getenv("CNY_HTDEBUG")) : false;

	bool isPwrTwo(size_t x) { return (x & (x-1)) == 0; }

	extern void HtCoprocModel(Ht::CHtHifLibBase *);

	class CHtHif;

	// HtHifMem routines
	CHtHifLib * CHtHifMem::CHtHifMemRegion::m_pHtHifBase;
	uint8_t * CHtHifMem::CHtHifMemRegion::m_pHtHifMemBase;

	void CHtHifMem::AllocHifMem(CHtHifLib * pHtHifBase, int unitCnt, size_t unitSize)
	{
		m_pHtHifBase = pHtHifBase;
		m_aeCnt = pHtHifBase->m_unitMap.GetAeCnt();

		CHtHifMemRegion::SetHtHifBase(pHtHifBase);

		// create a list of required memory regions
		m_memRegionCnt = 2 + m_aeCnt + unitCnt * HT_HIF_MEM_REGIONS;

		m_pMemRegionList = new CHtHifMemRegion [m_memRegionCnt];
		if (m_pMemRegionList == 0)
			throw CHtException(eHtBadHostAlloc, string("unable to alloc host memory"));
		memset(m_pMemRegionList, 0, sizeof(CHtHifMemRegion) * m_memRegionCnt);

		m_ctlInQueMemSize = 512 * sizeof(CHtCtrlMsg);
		m_ctlOutQueMemSize = 512 * sizeof(CHtCtrlMsg);
		m_ctlQueMemSize = m_ctlInQueMemSize + m_ctlOutQueMemSize;

		m_inBlkMemSize = HIF_ARG_IBLK_CNT * HIF_ARG_IBLK_SIZE;
		m_outBlkMemSize = HIF_ARG_OBLK_CNT * HIF_ARG_OBLK_SIZE;

		m_flushLinesMemSize = 64 * 8;

		m_unitMemSize = (unitSize + 0xfff) & ~0xfff;

		// round appUnitMemSize up to next 4096 boundary
		m_appUnitMemSize = (pHtHifBase->m_htHifParams.m_appUnitMemSize + 0xfff) & ~0xfffull;

		// round appPersMemSize up to next 4096 boundary
		uint64_t appPersMemSize = (pHtHifBase->m_htHifParams.m_appPersMemSize + 0xfff) & ~0xfffull;

		m_pMemRegionList[0].InitRegion("App Memory", appPersMemSize, 0);
		m_pMemRegionList[1].InitRegion("FlushLines", m_flushLinesMemSize, 0);

		char nameBuf[32];
		for (int aeId = 0; aeId < m_aeCnt; aeId += 1) {
			CHtHifMemRegion * pMemRegion = &m_pMemRegionList[2 + aeId];

			// find a unitId on the specified AE
			int unitId;
			pHtHifBase->m_unitMap.FindUnitId(aeId, 0, unitId);

			sprintf(nameBuf, "AE%d In/Out Ctl Queues", aeId);
			pMemRegion->InitRegion(nameBuf, m_ctlQueMemSize * pHtHifBase->m_unitMap.GetAuCnt(aeId), pHtHifBase->m_unitMap.GetNumaSet(unitId));
		}

		for (int unitId = 0; unitId < unitCnt; unitId += 1) {

			int numaSet = pHtHifBase->m_unitMap.GetNumaSet(unitId);

			CHtHifMemRegion * pMemRegion = &m_pMemRegionList[2 + m_aeCnt + unitId * HT_HIF_MEM_REGIONS];

			sprintf(nameBuf, "Unit %d In Block", unitId);
			pMemRegion[HT_HIF_MEM_BLK_IN].InitRegion(nameBuf, m_inBlkMemSize, numaSet);

			sprintf(nameBuf, "Unit %d Out Block", unitId);
			pMemRegion[HT_HIF_MEM_BLK_OUT].InitRegion(nameBuf, m_outBlkMemSize, numaSet);

			sprintf(nameBuf, "Unit %d App Memory", unitId);
			pMemRegion[HT_HIF_MEM_APP_UNIT].InitRegion(nameBuf, m_appUnitMemSize, numaSet);

			sprintf(nameBuf, "Unit %d Unit Memory", unitId);
			pMemRegion[HT_HIF_MEM_UNIT].InitRegion(nameBuf, m_unitMemSize, numaSet);
		}

		// List is created, now sort based on size of region
		//   sorting on size allows all regions to be aligned on region sized boundaries

		// create list of pointers to each region
		CHtHifMemRegion ** ppMemRegion = new CHtHifMemRegion * [m_memRegionCnt];
		if (ppMemRegion == 0) {
			if (m_pMemRegionList) {
				delete[] m_pMemRegionList;
				m_pMemRegionList = 0;
			}
			throw CHtException(eHtBadHostAlloc, string("unable to alloc host memory"));
		}

		for (int i = 0; i < m_memRegionCnt; i += 1)
			ppMemRegion[i] = &m_pMemRegionList[i];

		// sort the pointers
		for (int i = 0; i < m_memRegionCnt; i += 1) {
			for (int j = i+1; j < m_memRegionCnt; j += 1) {
				if (ppMemRegion[i]->GetSortKey() < ppMemRegion[j]->GetSortKey()) {
					CHtHifMemRegion * pTmp = ppMemRegion[i];
					ppMemRegion[i] = ppMemRegion[j];
					ppMemRegion[j] = pTmp;
				}
			}
		}

		// determine each regions starting offset within single memory allocation
		//   each numaSet must begin on 2MB alignment
		uint64_t offset = 0;
		uint8_t prevNumaSet = -1;
		for (int i = 0; i < m_memRegionCnt; i += 1) {
			if (ppMemRegion[i]->GetNumaSet() != prevNumaSet) {
				// align beginning of numaSet to 2MB boundary
				offset = (offset + 0x200000 - 1) & ~0x1fffffLL;
				prevNumaSet = ppMemRegion[i]->GetNumaSet();
			}

			ppMemRegion[i]->SetMemOffset(offset);
			offset += ppMemRegion[i]->GetMemSize();
		}
		m_htHifMemSize = (offset + 0x200000 - 1) & ~0x1fffffLL;

		delete[] ppMemRegion;

		size_t memAlign = 4*1024*1024;
		if (pHtHifBase->m_htHifParams.m_bHtHifHugePage) {
			if (m_htHifMemSize > HUGE_PAGE_SIZE)
				 throw CHtException(eHtBadHostAlloc, string("HostMemAllocHuge is currently limited to a single 1GB page"));
			m_pHtHifMem = (uint8_t *)CHtHif::HostMemAllocHuge(pHtHifBase->m_htHifParams.m_pHtHifHugePageBase);
			m_bHtHifMemHuge = true;
		} else {
			m_pHtHifMem = (uint8_t *)CHtHif::HostMemAllocAlign(memAlign, m_htHifMemSize, false);
			m_bHtHifMemHuge = false;
		}
		if (!m_pHtHifMem) {
			if (m_pMemRegionList) {
				delete[] m_pMemRegionList;
				m_pMemRegionList = 0;
			}
			throw CHtException(eHtBadHostAlloc, string("unable to alloc host memory"));
		}

		CHtHifMemRegion::SetHtHifMemBase(m_pHtHifMem);

		if (g_htDebug > 2) {
			printf("Host Interface Memory Region Info\n");
			for (int i = 0; i < m_memRegionCnt; i += 1)
				printf("  region %d: name = %-21s numaSet = %d, pBase = %012llx, size = %llx\n", i,
				m_pMemRegionList[i].GetName(), m_pMemRegionList[i].GetNumaSet(),
					(long long)m_pMemRegionList[i].GetMemBase(), (long long)m_pMemRegionList[i].GetMemSize());
			printf("\n");
		}
	}

	void CHtHifMem::InitHifMemory()
	{
		for (int i = 0; i < m_memRegionCnt; i += 1)
			m_pMemRegionList[i].ClearMemRegion();
	}

	inline CHtHifLib::~CHtHifLib()
	{
		// send shutdown message and free host resources
		for (int i = 0; i < GetUnitCnt(); i += 1) {
			if (m_ppHtUnits[i] == 0)
				InitUnit(i);
			while (!m_ppHtUnits[i]->SendHostHalt())
				m_ppHtUnits[i]->HaltLoop();
		}

		// wait for coproc dispatch to complete
		while (m_bCoprocBusy) {
			for (int i = 0; i < GetUnitCnt(); i += 1)
				m_ppHtUnits[i]->HaltLoop();

			usleep(1);
		}

		m_bHaltSent = true;

		// wait for timer thread to terminate
		while (m_htHifParams.m_iBlkTimerUSec > 0 && !m_bTimerHaltSeen);

		// wait for monitor thread to terminate
		while (g_htDebug > 0 && !m_bMonitorHaltSeen);

		if (m_ppHtUnits) {
			delete[] m_ppHtUnits;
			m_ppHtUnits = 0;
		}
	}

	uint8_t * CHtHifMem::GetAppPersMemBase() {
		int region = 0;
		m_pMemRegionList[region].ClearMemRegion();
		return m_pMemRegionList[region].GetMemBase();
	}

	uint8_t * CHtHifMem::GetFlushLinesBase() {
		int region = 1;
		m_pMemRegionList[region].ClearMemRegion();
		return m_pMemRegionList[region].GetMemBase();
	}

	uint8_t * CHtHifMem::GetCtlQueMemBase(int aeIdx) {
		int region = 2 + aeIdx;
		m_pMemRegionList[region].ClearMemRegion();
		return m_pMemRegionList[region].GetMemBase();
	}

	uint8_t * CHtHifMem::GetCtlInQueMemBase(int unitId) {
		int ae, au;
		m_pHtHifBase->m_unitMap.FindAeAu(unitId, ae, au);

		int region = 2 + ae;
		uint64_t regionOffset = m_ctlQueMemSize * au;
		m_pMemRegionList[region].ClearMemRegion();
		return m_pMemRegionList[region].GetMemBase() + regionOffset;
	}

	uint8_t * CHtHifMem::GetCtlOutQueMemBase(int unitId) {
		int ae, au;
		m_pHtHifBase->m_unitMap.FindAeAu(unitId, ae, au);

		int region = 2 + ae;
		uint64_t regionOffset = m_ctlQueMemSize * au + m_ctlInQueMemSize;
		m_pMemRegionList[region].ClearMemRegion();
		return m_pMemRegionList[region].GetMemBase() + regionOffset;
	}

	uint8_t * CHtHifMem::GetInBlkBase(int unitId) {
		int region = 2 + m_aeCnt + unitId * HT_HIF_MEM_REGIONS + HT_HIF_MEM_BLK_IN;
		m_pMemRegionList[region].ClearMemRegion();
		return m_pMemRegionList[region].GetMemBase();
	}

	uint8_t * CHtHifMem::GetOutBlkBase(int unitId) {
		int region = 2 + m_aeCnt + unitId * HT_HIF_MEM_REGIONS + HT_HIF_MEM_BLK_OUT;
		m_pMemRegionList[region].ClearMemRegion();
		return m_pMemRegionList[region].GetMemBase();
	}

	uint8_t * CHtHifMem::GetAppUnitMemBase(int unitId) {
		int region = 2 + m_aeCnt + unitId * HT_HIF_MEM_REGIONS + HT_HIF_MEM_APP_UNIT;
		m_pMemRegionList[region].ClearMemRegion();
		return m_pMemRegionList[region].GetMemBase();
	}

	uint8_t * CHtHifMem::GetUnitMemBase(int unitId) {
		int region = 2 + m_aeCnt + unitId * HT_HIF_MEM_REGIONS + HT_HIF_MEM_UNIT;
		m_pMemRegionList[region].ClearMemRegion();
		return m_pMemRegionList[region].GetMemBase();
	}

	void CHtHifMem::CHtHifMemRegion::ClearMemRegion() {
		if (m_bCleared)
			return;

		if (g_htDebug > 2)
			printf("Cleared HIF memory region %s\n", m_pName);

		memset(GetMemBase(), 0, m_size);

		m_bCleared = true;
	}

	CHtUnitLib::CHtUnitLib(CHtHifLib *pHtHifLib, int unitId)
	{
		m_pHtHifLib = pHtHifLib;
		m_unitId = unitId;
		m_ctlQueMask = 512-1;
		m_bUsingCallback = false;
		m_bFree = false;
		m_inLock = 0;

		// check for power of two
		assert(((HIF_ARG_IBLK_SIZE-1) & HIF_ARG_IBLK_SIZE) == 0);
		assert(((HIF_ARG_IBLK_CNT-1) & HIF_ARG_IBLK_CNT) == 0);
		assert(((HIF_ARG_OBLK_SIZE-1) & HIF_ARG_OBLK_SIZE) == 0);
		assert(((HIF_ARG_OBLK_CNT-1) & HIF_ARG_OBLK_CNT) == 0);
		assert(HIF_ARG_OBLK_CNT > 1);

		m_ctlInWrIdx = 0;
		m_outCtlRdIdx = 0;

		m_inBlkAvailCnt = HIF_ARG_IBLK_CNT;
		m_inBlkWrIdx = 0;
		m_inBlkIdx = 0;
		m_inBlkSize = HIF_ARG_IBLK_SIZE / sizeof(uint64_t);
		m_inBlkMask = HIF_ARG_IBLK_CNT-1;

		m_outBlkDataSize = 0;
		m_outBlkRdIdx = 0;
		m_outBlkIdx = 0;
		m_outBlkSize = HIF_ARG_OBLK_SIZE / sizeof(uint64_t);
		m_outBlkMask = HIF_ARG_OBLK_CNT-1;
		m_recvRtnArgc = 0;

		m_pInBlkBase = (uint64_t *)m_pHtHifLib->GetInBlkBase(unitId);
		m_pInBlk = m_pInBlkBase;

		m_pOutBlkBase = (uint64_t *)m_pHtHifLib->GetOutBlkBase(unitId);
		m_pOutBlk = m_pOutBlkBase;

		m_pCtlInQue = (CHtCtrlMsg *)m_pHtHifLib->GetCtlInQueMemBase(unitId);
		m_pCtlOutQue = (CHtCtrlMsg *)m_pHtHifLib->GetCtlOutQueMemBase(unitId);
		m_pOutCtlMsg = &m_pCtlOutQue[m_outCtlRdIdx & m_ctlQueMask];

		m_monitor.Clear();

	#ifdef HT_AVL_TEST
		m_inAvlMask = 0xffff;
		m_outRdyMask = 0;
		m_bOutBlkRdySeen = false;
	#endif

		int outBlkTimerClks = m_pHtHifLib->GetHifParams().m_oBlkTimerUSec * m_pHtHifLib->GetUnitMHz();
		assert(outBlkTimerClks < (1 << HIF_ARG_OBLK_TIME_W));

		int ctlTimerClks = m_pHtHifLib->GetHifParams().m_ctlTimerUSec * m_pHtHifLib->GetUnitMHz();
		assert(ctlTimerClks < (1 << HIF_ARG_CTL_TIME_W));

		LockInBlk();

		SendMsgInternal(HIF_CMD_IBLK_ADR, (uint64_t)m_pInBlkBase);
		SendMsgInternal(HIF_CMD_OBLK_ADR, (uint64_t)m_pOutBlkBase);
		SendMsgInternal(HIF_CMD_IBLK_PARAM, HIF_ARG_IBLK_SIZE_LG2 |
			(HIF_ARG_IBLK_CNT_LG2 << 8));
		SendMsgInternal(HIF_CMD_OBLK_PARAM, HIF_ARG_OBLK_SIZE_LG2 |
			(HIF_ARG_OBLK_CNT_LG2 << 8) |
			((uint64_t)outBlkTimerClks << 16));

		for (unsigned i = 0; i < HIF_ARG_OBLK_CNT; i++)
			SendMsgInternal(HIF_CMD_OBLK_AVL, i-16);

		SendMsgInternal(HIF_CMD_CTL_PARAM, ctlTimerClks);

		if (m_pHtHifLib->GetNeedFlush())
			SendMsgInternal(HIF_CMD_OBLK_FLSH_ADR, (uint64_t)m_pHtHifLib->GetFlushLinesBase());

		UnlockInBlk();
	}

	CHtHifLibBase * CHtHifLibBase::NewHtHifLibBase(CHtHifParams * pHtHifParams, char const * pHtPers, CHtHifBase * pHtHifBase) {
		return new CHtHifLib(pHtHifParams, pHtPers, pHtHifBase);
	}

	CHtHifLib::CHtHifLib(CHtHifParams * pHtHifParams, char const * pHtPers, CHtHifBase * pHtHifBase)
	{
		m_pHtHifBase = pHtHifBase;
		m_ppHtUnits = 0;
		m_pSyscTop = 0;

		m_htNeedFlush = false;
		m_appMode = m_pHtHifBase->GetAppMode();

		if (m_appMode == eAppRun || m_appMode == eAppVsim) {
			uint64_t aeg2 = 0;
			uint64_t aeg3 = 0;

			m_pHtHifBase->HtCpInfo(&m_htNeedFlush, &m_bCoprocBusy, &aeg2, &aeg3);

			m_partNumber = aeg2;
			m_aeCnt = (aeg3 >> 32) & 0xf;
			if (!m_aeCnt || m_appMode == eAppVsim)
				m_aeCnt = m_pHtHifBase->GetAeCnt();//HT_AE_CNT
			m_unitMhz = (aeg3 >> 16) & 0xffff;
			m_abi = (aeg3 >> 8) & 0xff;
			m_auCnt = (aeg3 >> 0) & 0xff;

			assert(m_auCnt > 0);
		} else {
			m_partNumber = ~0ULL;
			m_aeCnt = m_pHtHifBase->GetAeCnt();//HT_AE_CNT;
			m_unitMhz = 150;
			m_abi = 0x00;
			m_auCnt = m_pHtHifBase->GetUnitCnt();//HT_UNIT_CNT;
		}

		assert(m_aeCnt <= HT_HIF_AE_CNT_MAX);

		m_allocLock = 0;

		// Initialize htHifParams
		if (pHtHifParams)
			m_htHifParams = *pHtHifParams;

		if (m_htHifParams.m_pHtPers == 0)
			m_htHifParams.m_pHtPers = pHtPers;

		int numaSetCnt = max(m_htHifParams.m_numaSetCnt, 1);
		if (numaSetCnt != 1 && numaSetCnt != 2 && numaSetCnt != 4)
			throw CHtException(eHtBadHtHifParam, string("bad HtHifParam parameter, m_numaSetCnt only supports 1, 2 or 4"));

		// Initialize unitMap
		int unitId = 0;
		for (int auId = 0; auId < m_auCnt; auId += 1) {
			for (int aeId = 0; aeId < m_aeCnt; aeId += 1) {
				m_unitMap.Insert(unitId, aeId, auId, aeId % numaSetCnt);
				unitId += 1;
			}
		}

		m_unitMap.SetAeCnt(m_aeCnt);
		for (int aeId = 0; aeId < m_aeCnt; aeId += 1)
			m_unitMap.SetAuCnt(aeId, m_auCnt);

		int unitCnt = m_unitMap.GetUnitCnt();

		try {
			m_ppHtUnits = new CHtUnitLib *[unitCnt];
		}
		catch (std::bad_alloc) {
			throw CHtException(eHtBadHostAlloc, string("unable to alloc host memory"));
		}
		memset(m_ppHtUnits, 0, unitCnt * sizeof(CHtUnitLib *));

		// allocate host memory for coprocessor access
		m_bCoprocDispatched = false;
		try {
			size_t sizeofUnit = sizeof(CHtUnitLib);
			AllocHifMem(this, unitCnt, sizeofUnit);

			if (m_htHifParams.m_numaSetCnt == 0)
				DispatchCoproc();
		}
		catch (CHtException & htException) {
			delete m_ppHtUnits;
			m_ppHtUnits = 0;
			throw htException;
		}
	}

	void CHtHifLib::DispatchCoproc()
	{
		// all units must be initialized to start the coprocessor
		//   i.e. the control queue memory must be zero'ed
		LockAlloc();
		InitHifMemory();
		UnlockAlloc();

		for (int aeIdx = 0; aeIdx < m_aeCnt; aeIdx += 1)
			m_args[aeIdx] = (uint64_t)GetCtlQueMemBase(aeIdx);

		if (m_appMode == eAppSysc) {

			g_coprocAeRunningCnt = m_aeCnt;

			for (int aeIdx = 0; aeIdx < m_aeCnt; aeIdx += 1)
				g_pCtrlIntfQues[aeIdx] = (CHtCtrlMsg *)GetCtlQueMemBase(aeIdx);

			m_pHtHifBase->NewSyscTop();
		}
		else if (m_appMode != eAppModel) {

			m_pHtHifBase->HtCpDispatch(m_args);
		}

		m_bCoprocBusy = true;
		m_bCoprocDispatched = true;

		pthread_t coprocThread;
		pthread_create(&coprocThread, 0, CoprocThreadStart, this);

		m_bHaltSent = false;
		m_bTimerHaltSeen = false;
		m_bMonitorHaltSeen = false;

		if (m_htHifParams.m_iBlkTimerUSec > 0) {
			pthread_t timerThread;
			pthread_create(&timerThread, 0, HostTimerThreadStart, this);
		}

		if (g_htDebug > 0) {
			pthread_t monitorThread;
			pthread_create(&monitorThread, 0, MonitorThreadStart, this);
		}
	}

	void CHtHifLib::CoprocThread()
	{
		pthread_detach(pthread_self());
		if (g_htDebug > 1) fprintf(stderr, "HTLIB: Dispatch Coproc\n");

		if (m_appMode == eAppModel) {

			HtCoprocModel(this);

		} else if (m_appMode == eAppSysc) {

			m_pHtHifBase->HtCoprocSysc();

		} else {

			m_pHtHifBase->HtCpDispatchWait(m_args);
		}

		m_bCoprocBusy = false;

		if (g_htDebug > 1) fprintf(stderr, "HTLIB: Dispatch completed\n");
		pthread_exit(0);
	}

	void CHtHifLib::HostTimerThread()
	{
		pthread_detach(pthread_self());
		for (;;) {
			usleep(m_htHifParams.m_iBlkTimerUSec);

			// check if inbound block queue needs flushed
			for (int i = 0; i < GetUnitCnt(); i += 1) {
				if (m_ppHtUnits[i] == 0)
					continue;

				m_ppHtUnits[i]->InBlkTimer();
			}

			if (m_bHaltSent) {
				// exit timer thread
				m_bTimerHaltSeen = true;
				break;
			}
		}
		pthread_exit(0);
	}

	void CHtUnitLib::InBlkTimer()
	{
		LockInBlk();
		FlushInBlk();
		UnlockInBlk();
	}

	void CHtHifLib::MonitorThread()
	{
		pthread_detach(pthread_self());
		int sleepCnt = 0;
		for (;;) {
			int sleepUsec = 1000000;
			usleep(sleepUsec);	// 1 sec

			if (sleepCnt++ < 10 && !m_bHaltSent)
				continue;

			int periodUsec = sleepUsec * sleepCnt;
			sleepCnt = 0;

			CHtHifMonitor info;
			info.Clear();

			// check if inbound block queue needs flushed
			for (int i = 0; i < GetUnitCnt(); i += 1) {
				if (m_ppHtUnits[i] == 0)
					continue;

				m_ppHtUnits[i]->m_monitor.Lock();
				info += m_ppHtUnits[i]->m_monitor;
				m_ppHtUnits[i]->m_monitor.Clear();
				m_ppHtUnits[i]->m_monitor.Unlock();
			}

			char ohdStr[64];
			char ihdStr[64];

			FormatHostDataRate(ihdStr, info.GetIhdBytes(), periodUsec);
			FormatHostDataRate(ohdStr, info.GetOhdBytes(), periodUsec);

			char timeStr[64];
			time_t rawTime;

			time (&rawTime);
			struct tm * pTimeInfo = localtime( &rawTime);
			strftime(timeStr, 64, "%H:%M:%S", pTimeInfo);

			printf("%s - calls %d, returns %d, ihm %d, ohm %d, ihd %s, ohd %s\n",
				timeStr, info.GetCallCnt(), info.GetRtnCnt(), info.GetIhmCnt(), info.GetOhmCnt(), ihdStr, ohdStr);
			fflush(stdout);

			if (m_bHaltSent) {
				// exit monitor thread
				m_bMonitorHaltSeen = true;
				break;
			}
		}
		pthread_exit(0);
	}

	void CHtHifLib::FormatHostDataRate(char * pHdStr, uint64_t hdBytes, int periodUsec)
	{
		// format as G/M/K bytes per sec
		hdBytes = hdBytes * 1000000 / periodUsec;

		if (hdBytes < 1000)
			sprintf(pHdStr, "%d B/s", (int)hdBytes);
		else if (hdBytes < 10000)
			sprintf(pHdStr, "%5.3f KB/s", hdBytes / 1000.0);
		else if (hdBytes < 100000)
			sprintf(pHdStr, "%5.2f KB/s", hdBytes / 1000.0);
		else if (hdBytes < 1000000)
			sprintf(pHdStr, "%5.1f KB/s", hdBytes / 1000.0);
		else if (hdBytes < 10000000)
			sprintf(pHdStr, "%5.3f MB/s", hdBytes / 1000000.0);
		else if (hdBytes < 100000000)
			sprintf(pHdStr, "%5.2f MB/s", hdBytes / 1000000.0);
		else if (hdBytes < 1000000000)
			sprintf(pHdStr, "%5.1f MB/s", hdBytes / 1000000.0);
		else
			sprintf(pHdStr, "%5.3f GB/s", hdBytes / 1000000000.0);
	}

	CHtUnitLib * CHtHifLib::AllocUnit(CHtUnitBase * pHtUnitBase, int numaSet) {

		if ((numaSet < 0) != (m_htHifParams.m_numaSetCnt == 0) || numaSet >= m_htHifParams.m_numaSetCnt) {
			// either numaSets are not in use or no numaSet was specified

			if (m_htHifParams.m_numaSetCnt > 0)
				printf("Error: AllocUnit() numaSet not specified but htHifParams.m_numaSetCnt > 0\n");

			else if (numaSet >= 0)
				printf("Error: AllocUnit() numaSet specified but htHifParams.m_numaSetCnt == 0\n");

			else
				printf("Error: AllocUnit() numaSet is greater than htHifParams.m_numaSetCnt\n");

			exit(1);
		}

		if (numaSet < 0)
			numaSet = 0;

		LockAlloc();
		int unitId;
		for (unitId = 0; unitId < m_unitMap.GetUnitCnt(); unitId += 1) {
			if (m_unitMap.GetNumaSet(unitId) != numaSet)
				continue;

			if (m_ppHtUnits[unitId] == 0) {
				InitUnit(unitId);
				m_ppHtUnits[unitId]->m_pHtUnitBase = pHtUnitBase;
				break;
			} else if (m_ppHtUnits[unitId]->m_bFree) {
				m_ppHtUnits[unitId]->m_bFree = false;
				m_ppHtUnits[unitId]->m_pHtUnitBase = pHtUnitBase;
				break;
			}
		}
		UnlockAlloc();
		return unitId < m_unitMap.GetUnitCnt() ? m_ppHtUnits[unitId] : 0;
	}

	void CHtHifLib::FreeUnit(CHtUnitLibBase * pUnit) {
		static_cast<CHtUnitLib *>(pUnit)->m_bFree = true;
	}

	void CHtHifLib::InitUnit(int unitId) {
		assert(unitId < GetUnitCnt());
		if (m_ppHtUnits[unitId] == 0)
			m_ppHtUnits[unitId] = new (GetUnitMemBase(unitId)) CHtUnitLib(this, unitId);
	}

	uint8_t * CHtUnitLib::GetUnitMemBase() {
		return m_pHtHifLib->GetAppUnitMemBase(m_unitId);
	}

	void CHtUnitLib::SendHostMsg(uint8_t msgType, uint64_t msgData)
	{
		// reserve <16 for Internal
		assert(msgType > 15);
		LockInBlk();
		FlushInBlk();
		SendMsgInternal(msgType, msgData);
		m_monitor.IncIhmCnt();
		UnlockInBlk();
	}

	bool CHtUnitLib::RecvHostMsg(uint8_t & msgType, uint64_t & msgData)
	{
		bool bRcvd = RecvMsgInternal(msgType, msgData);
		return bRcvd;
	}

	void CHtUnitLib::SendMsgDebug(uint8_t msgType, uint64_t msgData, int argc, int * pArgw, uint64_t * pArgv)
	{
		CHtCtrlMsg ctrlMsg(msgType, msgData);

		uint8_t byte0 = (uint8_t)((msgData >> 0) & 0xffULL);
		uint8_t byte1 = (uint8_t)((msgData >> 8) & 0xffULL);
		uint32_t byte52 = (uint32_t)((msgData >> 16) & 0xfffffffULL);
		uint32_t byte30 = (uint32_t)( msgData & 0xffffffffULL);

		long long unsigned int byte50;
		byte50 = (long long unsigned int)(msgData & 0xffffffffffffULL);

		char c[128];
		string str;

		int ae, au;
		m_pHtHifLib->m_unitMap.FindAeAu(m_unitId, ae, au);

		sprintf(c, " SndMsg AE%d AU%d ", ae, au);
		str.append(c);

		switch (ctrlMsg.GetMsgType()) {
		case HIF_CMD_CTL_PARAM:
			sprintf(c, "HIF_CMD_CTL_PARAM time=0x%07x", byte30);
			str.append(c);
			break;
		case HIF_CMD_IBLK_ADR:
			sprintf(c, "HIF_CMD_IBLK_ADR adr=0x%012llx", byte50);
			str.append(c);
			break;
		case HIF_CMD_OBLK_ADR:
			sprintf(c, "HIF_CMD_OBLK_ADR adr=0x%012llx", byte50);
			str.append(c);
			break;
		case HIF_CMD_OBLK_FLSH_ADR:
			sprintf(c, "HIF_CMD_OBLK_FLSH_ADR adr=0x%012llx", byte50);
			str.append(c);
			break;
		case HIF_CMD_IBLK_PARAM:
			sprintf(c, "HIF_CMD_IBLK_PARAM size=%d, cnt=%d",
				byte0, byte1);
			str.append(c);
			break;
		case HIF_CMD_OBLK_PARAM:
			sprintf(c, "HIF_CMD_OBLK_PARAM size=%d, cnt=%d, time=0x%07x",
				byte0, byte1, byte52);
			str.append(c);
			break;
		case HIF_CMD_IBLK_RDY:
			sprintf(c, "HIF_CMD_IBLK_RDY ");
			str.append(c);
			if (byte30 % 8) {
				switch (byte30 % 8) {
				case HIF_DQ_NULL:
					sprintf(c, "(NULL)");
					str.append(c);
					break;
				case HIF_DQ_CALL: 
				{
					sprintf(c, "(CALL) argc=%d", (uint8_t)byte30 >> 3);
					str.append(c);

					sprintf(c, "\n  Raw arguments:");
					str.append(c);
					int j = 0;
					for (int i = 0; i < argc; i += 1) {
						sprintf(c, "\n   Arg[%d] = 0x%016llx", i, (long long)pArgv[j++]);
						str.append(c);

						for (int k = 1; k < pArgw[i]; k += 1) {
							sprintf(c, "\n            0x%016llx", (long long)pArgv[j++]);
							str.append(c);
						}
					}
					break;
				}
				case HIF_DQ_HALT:
					sprintf(c, "(HALT)");
					str.append(c);
					break;
				default: assert(0);
				}
			} else {
				sprintf(c, "size=%d", byte30);
				str.append(c);
			}
			break;
		case HIF_CMD_OBLK_AVL:
			sprintf(c, "HIF_CMD_OBLK_AVL");
			str.append(c);
			break;
		default:
			if (ctrlMsg.GetMsgType() < 16) {
				sprintf(c, "HIF_CMD_UNKNOWN(%d, 0x%014llx)",
					ctrlMsg.GetMsgType(),
					(long long unsigned int)ctrlMsg.GetMsgData());
				str.append(c);
			} else {
				sprintf(c, "USER_CMD(%s, 0x%014llx)",
					m_pHtUnitBase->GetSendMsgName(ctrlMsg.GetMsgType()),
					(long long unsigned int)ctrlMsg.GetMsgData());
				str.append(c);
			}
		}

		fflush(stdout);
		fprintf(stderr, "%s\n", str.c_str());
	}

	void CHtUnitLib::RecvMsgDebug(uint8_t msgType, uint64_t msgData)
	{
		uint8_t byte6 = (uint8_t)((msgData >> 48) & 0xffULL);
		uint16_t byte54 = (uint16_t)((msgData >> 32) & 0xffffULL);
		uint32_t byte30 = (uint32_t)( msgData & 0xffffffffULL);

		char c[128];
		string str;

		int ae, au;
		m_pHtHifLib->m_unitMap.FindAeAu(m_unitId, ae, au);

		sprintf(c, " RcvMsg AE%d AU%d ", ae, au);
		str.append(c);

		switch (msgType) {
		case HIF_CMD_OBLK_RDY:
			sprintf(c, "HIF_CMD_OBLK_RDY ");
			str.append(c);
			if (byte30 % 8) {
				switch (byte30 % 8) {
				case HIF_DQ_NULL:
					sprintf(c, "(NULL)");
					str.append(c);
					break;
				case HIF_DQ_CALL:
					sprintf(c, "(RTN) argc=%d", (uint8_t)byte30 >> 3);
					str.append(c);
					break;
				case HIF_DQ_HALT:
					sprintf(c, "(HALT)");
					str.append(c);
					break;
				default: assert(0);
				}
			} else {
				sprintf(c, "size=%d", byte30);
				str.append(c);
			}
			break;
		case HIF_CMD_IBLK_AVL:
			sprintf(c, "HIF_CMD_IBLK_AVL");
			str.append(c);
			break;
		case HIF_CMD_ASSERT: {
			char const * pModuleName = m_pHtUnitBase->GetModuleName(byte6);
			if (pModuleName == 0)
				sprintf(c, "HIF_CMD_ASSERT: module=%d, lineNum=%d, info=0x%08x",
					byte6, byte54, byte30);
			else
				sprintf(c, "HIF_CMD_ASSERT: module=%s, lineNum=%d, info=0x%08x",
					pModuleName, byte54, byte30);
			str.append(c);
			break;
		}
		case HIF_CMD_ASSERT_COLL: {
			char const * pModuleName = m_pHtUnitBase->GetModuleName(byte6);
			if (pModuleName == 0)
				sprintf(c, "HIF_CMD_ASSERT_COLL: module=%d, lineNum=%d, info=0x%08x",
					byte6, byte54, byte30);
			else
				sprintf(c, "HIF_CMD_ASSERT_COLL: module=%s, lineNum=%d, info=0x%08x",
					pModuleName, byte54, byte30);
			str.append(c);
			break;
		}
		default:
			if (msgType < 16) {
				sprintf(c, "HIF_CMD_UNKNOWN(%d, 0x%014llx)",
					msgType,
					(long long unsigned int)msgData);
				str.append(c);
			} else {
				sprintf(c, "USER_CMD(%s, 0x%014llx)",
					m_pHtUnitBase->GetRecvMsgName(msgType),
					(long long unsigned int)msgData);
				str.append(c);
			}
		}

		fflush(stdout);
		fprintf(stderr, "%s\n", str.c_str());
	}

	bool CHtUnitLib::SendCall(int argc, int * pArgw, uint64_t * pArgv)
	{
		LockInBlk();
		FlushInBlk(); // flush input block data

		while (m_inBlkAvailCnt < 2) {
			UnlockInBlk();

			QueOutMsgs();

			LockInBlk();
			if (m_inBlkAvailCnt < 2) {
				UnlockInBlk();
				return false;
			}
		}

		uint64_t idx = 0;
	#ifdef HT_AVL_TEST
		idx = (uint64_t)(m_inBlkIdx & m_inBlkMask) << 48;
	#endif
		NextInBlk();	// force empty block for call msg

		int qwCnt = 0;
		for (int argIdx = 0; argIdx < argc; argIdx += 1)
			qwCnt += pArgw[argIdx];

		int cnt = SendDataInternal(qwCnt, pArgv);
		assert(cnt == qwCnt);

		int size = (qwCnt << 3) | HIF_DQ_CALL;
		SendMsgInternal(HIF_CMD_IBLK_RDY, idx | size, false, argc, pArgw, pArgv);

		FlushInBlk(); // flush call args
		assert(m_inBlkAvailCnt >= 0);

		m_monitor.IncCallCnt();
		UnlockInBlk();
		return true;
	}

	bool CHtUnitLib::RecvReturn(int argc, uint64_t * pArgv)
	{
		ERecvType recvType = MsgLoop();

		if (recvType != eRecvReturn)
			return false;

		if (m_recvRtnArgc == 0) {
			uint8_t msgType = m_outMsgQue.Front().GetMsgType();
			uint64_t msgData = m_outMsgQue.Front().GetMsgData();

			assert(msgType == HIF_CMD_OBLK_RDY && (msgData & 0x7) == HIF_DQ_CALL);
			m_recvRtnArgc = (int)(msgData >> 3);

			assert(argc >= m_recvRtnArgc);
	#ifdef HT_AVL_TEST
			assert((m_outBlkIdx & m_outBlkMask) == ((msgData >> 48) & m_outBlkMask));
	#endif
			NextOutMsgQue();
			NextOutBlk(true);

			if (m_recvRtnArgc == 0) {
				m_monitor.IncRtnCnt();
				return true;
			}
		}

		if (m_recvRtnArgc > 0) {
			uint8_t msgType = m_outMsgQue.Front().GetMsgType();
			uint64_t msgData = m_outMsgQue.Front().GetMsgData();

			if (msgType != HIF_CMD_OBLK_RDY)
				return false;

			assert(msgType == HIF_CMD_OBLK_RDY && (msgData & 0x7) == 0);
			assert((m_recvRtnArgc & 0xff) == ((msgData >> 3) & 0xff));

	#ifdef HT_AVL_TEST
			assert((m_outRdyMask & (1 << ((m_outMsgQue.Front().GetMsgData() >> 48) & m_outBlkMask))) == 0);
			m_outRdyMask |= 1 << ((m_outMsgQue.Front().GetMsgData() >> 48) & m_outBlkMask);
	#endif

			uint64_t * pOutBlk = &m_pOutBlkBase[(m_outBlkIdx & m_outBlkMask) * m_outBlkSize];

			memcpy(pArgv, pOutBlk, argc * sizeof(uint64_t));

	#ifdef HT_AVL_TEST
			assert(m_outRdyMask & (1 << (m_outBlkIdx & m_outBlkMask)));
	#endif

			NextOutMsgQue();
			NextOutBlk(true);

			m_recvRtnArgc = 0;

			m_monitor.IncRtnCnt();
			return true;
		}

		return false;
	}

	bool CHtUnitLib::RecvMsgInternal(uint8_t & msgType, uint64_t & msgData)
	{
		ERecvType recvType = MsgLoop();

		if (recvType != eRecvHostMsg)
			return false;

		msgType = m_outMsgQue.Front().GetMsgType();
		msgData = m_outMsgQue.Front().GetMsgData();

		NextOutMsgQue();

		m_monitor.IncOhmCnt();
		return true;
	}

	void CHtUnitLib::NextOutBlk(bool bLock)
	{
		LockInBlk(bLock);

	#ifdef HT_AVL_TEST
		assert(m_outRdyMask & (1 << (m_outBlkIdx & m_outBlkMask)));
		m_outRdyMask &= ~(1 << (m_outBlkIdx & m_outBlkMask));
	#endif

		SendMsgInternal(HIF_CMD_OBLK_AVL, m_outBlkIdx);

		m_outBlkIdx += 1;
		m_pOutBlk = m_pOutBlkBase + (m_outBlkIdx & m_outBlkMask) * m_outBlkSize;
		m_outBlkRdIdx = 0;

		UnlockInBlk(bLock);
	}

	void CHtUnitLib::SendMsgInternal(uint8_t msgType, uint64_t msgData, bool bLock, int argc, int * pArgw, uint64_t * pArgv)
	{
		LockInBlk(bLock);

		CHtCtrlMsg * pInMsg = m_pCtlInQue + (m_ctlInWrIdx++ & m_ctlQueMask);

		UnlockInBlk(bLock);

		CHtCtrlMsg ctrlMsg(msgType, msgData);

		// wait until slot empty
		while (pInMsg->IsValid())
			usleep(1);

		*pInMsg = ctrlMsg;

		if (g_htDebug > 1)
			SendMsgDebug(msgType, msgData, argc, pArgw, pArgv);
	}

	void CHtUnitLib::HaltLoop()
	{
		switch (MsgLoop(false)) {
		case eRecvIdle:
			usleep(1);
			break;
		case eRecvHalt:
			break;
		case eRecvReturn:
			NextOutMsgQue();
			NextOutBlk(true);
			break;
		case eRecvHostData:
			NextOutMsgQue();
			NextOutBlk(true);
			break;
		case eRecvHostMsg:
			NextOutMsgQue();
			break;
		default:
			assert(0);
		}
	}

	void CHtUnitLib::NextOutMsg()
	{
		assert(m_pOutCtlMsg->IsValid());

		if (m_pOutCtlMsg->GetMsgType() == HIF_CMD_ASSERT ||
			m_pOutCtlMsg->GetMsgType() == HIF_CMD_ASSERT_COLL ||
			g_htDebug > 1)
		{
			RecvMsgDebug(m_pOutCtlMsg->GetMsgType(), m_pOutCtlMsg->GetMsgData());
		}

		m_pOutCtlMsg->Clear();
		m_outCtlRdIdx += 1;
		m_pOutCtlMsg = &m_pCtlOutQue[m_outCtlRdIdx & m_ctlQueMask];
	}

	void CHtUnitLib::QueOutMsgs()
	{
		// handle inbound OBLK_AVAIL messages and queue remainder
		for(;;) {
			CHtCtrlMsg ctrlMsg = *m_pOutCtlMsg;

			if (!ctrlMsg.IsValid() || m_outMsgQue.IsFull())
				break;

			uint8_t msgType = ctrlMsg.GetMsgType();

			if (msgType == HIF_CMD_IBLK_AVL) {
				LockInBlk();
				m_inBlkAvailCnt += 1;
	#ifdef HT_AVL_TEST
				uint64_t msgData = ctrlMsg.GetMsgData();
				assert((m_inAvlMask & (1 << (msgData & m_inBlkMask))) == 0);
				m_inAvlMask |= 1 << (msgData & m_inBlkMask);
	#endif
				UnlockInBlk();
			} else
				m_outMsgQue.Push(ctrlMsg);

			NextOutMsg();
		}
	}

	Ht::ERecvType CHtUnitLib::MsgLoop(bool bUseCb)
	{
		ERecvType rtnRecvType;

		QueOutMsgs();

		for(;;) {
			if (m_outMsgQue.Empty())
				return eRecvIdle;

			CHtCtrlMsg ctrlMsg = m_outMsgQue.Front();

			uint8_t msgType = ctrlMsg.GetMsgType();
			uint64_t msgData = ctrlMsg.GetMsgData();

			switch (msgType) {
			case HIF_CMD_ASSERT:
			case HIF_CMD_ASSERT_COLL:
				NextOutMsgQue();
				continue;
			case HIF_CMD_OBLK_RDY:
	#ifdef HT_AVL_TEST
				if (!m_bOutBlkRdySeen) {
					assert((m_outRdyMask & (1 << ((msgData >> 48) & m_outBlkMask))) == 0);
					m_outRdyMask |= 1 << ((msgData >> 48) & m_outBlkMask);
					m_bOutBlkRdySeen = true;
				}
	#endif
				switch (msgData & 0x7) {
				case HIF_DQ_NULL:
					{
						assert((msgData & ~0x7ULL) == 0);
						if (bUseCb && m_bUsingCallback) {
							Callback(eRecvHostDataMarker);
							continue;
						} else {
							rtnRecvType = eRecvHostDataMarker;
							break;
						}
					}
					break;
				case 0:
					{
						if (m_outBlkDataSize == 0)
							m_outBlkDataSize = (int)(msgData >> 3);

						if (bUseCb && m_bUsingCallback) {
							Callback(m_recvRtnArgc > 0 ? eRecvReturn : eRecvHostData);
							continue;
						} else {
							rtnRecvType = m_recvRtnArgc > 0 ? eRecvReturn : eRecvHostData;
							break;
						}
					}
					break;
				case HIF_DQ_CALL:
					{
						// check if arguments are available
						if ((int)(msgData >> 3) == 0) {
							if (bUseCb && m_bUsingCallback) {
								Callback(eRecvReturn);
								continue;
							} else {
								rtnRecvType = eRecvReturn;
								break;
							}
						}

						if (m_outMsgQue.Size() >= 2) {
							if (bUseCb && m_bUsingCallback) {
								Callback(eRecvReturn);
								continue;
							} else {
								rtnRecvType = eRecvReturn;
								break;
							}
						} else
							return eRecvIdle;
					}
					break;
				case HIF_DQ_HALT:
					{
						NextOutMsgQue();
						NextOutBlk(true);
						rtnRecvType = eRecvHalt;
					}
					break;
				default:
					assert(0);
				}
				break;
			default:
				if (msgType >= 16) {
					if (bUseCb && m_bUsingCallback) {
						Callback(eRecvHostMsg);
						continue;
					} else {
						rtnRecvType = eRecvHostMsg;
						break;
					}
				}

				assert(0);
				break;
			}

			break;
		}

		if (m_outMsgQueLoopCnt <= 1000)
			m_outMsgQueLoopCnt += 1;

		if (m_outMsgQueLoopCnt == 1000) {
			printf("Host side HIF deadlock detected, next receive message type: ");
			switch (rtnRecvType) {
			case eRecvHostMsg: printf("eRecvHostMsg\n"); break;
			case eRecvHostData: printf("eRecvHostData\n"); break;
			case eRecvReturn: printf("eRecvReturn\n"); break;
			case eRecvHalt: printf("eRecvHalt\n"); break;
			case eRecvHostDataMarker: printf("eRecvHostDataMarker\n"); break;
			default: assert(0);
			}
		}

		return rtnRecvType;
	}

	bool CHtUnitLib::SendHostHalt()
	{
		LockInBlk();
		FlushInBlk();

		if (m_inBlkAvailCnt < 1) {
			UnlockInBlk();
			QueOutMsgs();

			LockInBlk();
			if (m_inBlkAvailCnt < 1) {
				UnlockInBlk();
				return false;
			}
		}

		FlushInBlk(false, HIF_DQ_HALT);

		UnlockInBlk();
		return true;
	}

	bool CHtUnitLib::SendHostDataMarker()
	{
		LockInBlk();
		FlushInBlk();

		if (m_inBlkAvailCnt < 1) {
			UnlockInBlk();
			QueOutMsgs();

			LockInBlk();
			if (m_inBlkAvailCnt < 1) {
				UnlockInBlk();
				return false;
			}
		}

		FlushInBlk(false, HIF_DQ_NULL);

		UnlockInBlk();
		return true;
	}

	int CHtUnitLib::SendHostData(int qwCnt, uint64_t * pData)
	{
		return SendDataInternal(qwCnt, pData, true);
	}

	int CHtUnitLib::SendDataInternal(int qwCnt, uint64_t * pData, bool bLock)
	{
		LockInBlk(bLock);

		int sentCnt = 0;
		while (sentCnt < qwCnt) {
			if (m_inBlkAvailCnt == 0) {
				UnlockInBlk();

				QueOutMsgs();

				LockInBlk();
				if (m_inBlkAvailCnt == 0) {
					UnlockInBlk();
					return sentCnt;
				}
			}

			assert(m_inBlkAvailCnt > 0);
	#ifdef HT_AVL_TEST
			assert(m_inAvlMask & (1 << (m_inBlkIdx & m_inBlkMask)));
	#endif

			int cnt = min(qwCnt - sentCnt, m_inBlkSize - m_inBlkWrIdx);
			memcpy(&m_pInBlk[m_inBlkWrIdx], pData + sentCnt, cnt * sizeof(uint64_t));

			m_inBlkWrIdx += cnt;
			sentCnt += cnt;

			assert(m_inBlkWrIdx <= m_inBlkSize);

			if (m_inBlkWrIdx == m_inBlkSize)
				FlushInBlk();
		}

		m_monitor.IncIhdBytes( sentCnt * sizeof(uint64_t) );

		UnlockInBlk(bLock);
		return sentCnt;
	}

	void CHtUnitLib::FlushInBlk(bool bLock, int ctl)
	{
		LockInBlk(bLock);

		if (m_inBlkWrIdx == 0 && ctl == 0) {
			UnlockInBlk(bLock);
			return;
		}

		uint64_t idx = 0;
	#ifdef HT_AVL_TEST
		idx = (uint64_t)(m_inBlkIdx & m_inBlkMask) << 48;
	#endif
		int size = (m_inBlkWrIdx << 3) | ctl;

		NextInBlk();

		SendMsgInternal(HIF_CMD_IBLK_RDY, idx | size);

		UnlockInBlk(bLock);
	}

	void CHtUnitLib::NextInBlk()
	{
		assert(m_inLock != 0);
		assert(m_inBlkAvailCnt > 0);
	#ifdef HT_AVL_TEST
		assert(m_inAvlMask & (1 << (m_inBlkIdx & m_inBlkMask)));
		m_inAvlMask &= ~(1ul << (m_inBlkIdx & m_inBlkMask));
	#endif

		m_inBlkAvailCnt -= 1;
		m_inBlkWrIdx = 0;
		m_inBlkIdx += 1;
		m_pInBlk = m_pInBlkBase + (m_inBlkIdx & m_inBlkMask) * m_inBlkSize;
	}

	bool CHtUnitLib::PeekHostData(uint64_t & data) {
		if (m_outBlkDataSize == 0 && MsgLoop() != eRecvHostData)
			return false;

		assert(m_outBlkDataSize > 0);
		data = m_pOutBlk[m_outBlkRdIdx];
		return true;
	}

	bool CHtUnitLib::RecvHostDataMarker()
	{
		if (m_recvRtnArgc > 0 || m_outBlkDataSize > 0 || MsgLoop() != eRecvHostDataMarker)
			return false;

		NextOutMsgQue();
		NextOutBlk(true);

		return true;
	}

	int CHtUnitLib::RecvHostData(int qwCnt, uint64_t * pData)
	{
		if (m_recvRtnArgc > 0)
			return 0;

		int rcvdCnt = 0;
		while (rcvdCnt < qwCnt && (m_outBlkDataSize > 0 || MsgLoop() == eRecvHostData)) {

			int cnt = min(m_outBlkDataSize, qwCnt - rcvdCnt);
			memcpy(pData + rcvdCnt, m_pOutBlk + m_outBlkRdIdx, cnt * sizeof(uint64_t));

			m_outBlkRdIdx += cnt;
			rcvdCnt += cnt;
			m_outBlkDataSize -= cnt;

			if (m_outBlkDataSize == 0) {
				NextOutMsgQue();
				NextOutBlk(true);
			}
		}

		m_monitor.IncOhdBytes( rcvdCnt * sizeof(uint64_t) );

		return rcvdCnt;
	}

} // namespace Ht
