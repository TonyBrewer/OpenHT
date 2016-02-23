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

using namespace std;

#ifdef HT_SYSC
#include "../sysc/HtMemTrace.h"
#endif

#ifndef HT_NICK
# define HT_NICK pdk
#endif

#define mckstr(s) # s
#define mckxstr(s) mckstr(s)
#define HT_PERS mckxstr(HT_NICK)

#if !defined(HT_MODEL) && !defined(HT_SYSC) && !defined(HT_LIB_HIF) && !defined(HT_LIB_SYSC)
# ifdef CNYOS_API
#  include <convey/usr/cny_comp.h>
#  include <convey/sys/cnysys_arch.h>
# else
#  include <wdm_user.h>
# endif
#endif

#define HUGE_PAGE_SIZE (1 * 1024 * 1024 * 1024)
#if !defined(WIN32) && !defined(CNYOS_API)
#  include <sys/mman.h>
#endif

#define HT_NUMA_SET_MAX 4
#define HT_HIF_AE_CNT_MAX 4

struct CSyscTop;

namespace Ht {

#	ifdef HT_SYSC
	extern CMemTrace g_memTrace;
#	endif

	struct CHtHifParams {
		CHtHifParams() {
			m_ctlTimerUSec = 10;
			m_iBlkTimerUSec = 1000;
			m_oBlkTimerUSec = 1000;
			m_appPersMemSize = 0;
			m_appUnitMemSize = 0;
			m_numaSetCnt = 0;
			m_pHtPers = 0;
			m_bHtHifHugePage = false;
			m_pHtHifHugePageBase = 0;
			memset(m_numaSetUnitCnt, 0, sizeof(uint8_t) * HT_NUMA_SET_MAX);
		};

		uint32_t m_ctlTimerUSec;
		uint32_t m_iBlkTimerUSec;
		uint32_t m_oBlkTimerUSec;
		uint64_t m_appPersMemSize;
		uint64_t m_appUnitMemSize;
		int32_t m_numaSetCnt;
		char const * m_pHtPers;
		bool m_bHtHifHugePage;
		void * m_pHtHifHugePageBase;
		uint8_t m_numaSetUnitCnt[HT_NUMA_SET_MAX];
	};

	enum EHtErrorCode { eHtBadHostAlloc, eHtBadCoprocAlloc, eHtBadHtHifParam, eHtBadDispatch };
	class CHtException : public exception {
	public:
		CHtException(EHtErrorCode code, string const &msg) :
			m_code(code), m_msg(msg) {}

		~CHtException() throw() {}

		const char * what() const throw() {
			return m_msg.c_str();
		}

		string const &GetMsg() {
			return m_msg;
		}
		EHtErrorCode GetCode() {
			return m_code;
		}
	private:
		EHtErrorCode m_code;
		string const m_msg;
	};

	enum EAppMode { eAppRun, eAppSysc, eAppVsim, eAppModel };
	enum ERecvType { eRecvIdle, eRecvBusy, eRecvHalt, eRecvCall, eRecvReturn, eRecvHostData, eRecvHostMsg, eRecvHostDataMarker };

	class CHtHifBase;
	class CHtUnitBase;
	class CHtHifLibBase;
	class CHtUnitLibBase;
	class CHtHifUnitMap;

	class CHtHifLibBase {
		friend class CHtHifBase;
		friend class CHtModelUnitLib;
		friend class CHtModelHifLib;
	protected:
		static CHtHifLibBase * NewHtHifLibBase(CHtHifParams * pHtHifParams, char const * pHtPers, CHtHifBase * pHtHifBase);
		virtual ~CHtHifLibBase() {}
		virtual int GetUnitCnt() = 0;
		virtual void FreeAllUnits() = 0;
		virtual CHtUnitLibBase * AllocUnit(CHtUnitBase * pHtUnitBase, int numaSet) = 0;
		virtual void FreeUnit(CHtUnitLibBase * pHtUnitLibBase) = 0;
		virtual void SendAllHostMsg(uint8_t msgType, uint64_t msgData) = 0;
		virtual uint8_t * GetAppUnitMemBase(int unitId) = 0;
		virtual int GetUnitMHz() = 0;
		virtual uint64_t GetPartNumber() = 0;
		virtual uint8_t * GetCtlQueMemBase(int aeId) = 0;
		virtual CHtHifUnitMap * GetUnitMap() = 0;
	};

	class CHtUnitLibBase {
		friend class CHtUnitBase;
	protected:
		virtual ~CHtUnitLibBase() {}
		virtual int GetUnitId() = 0;
		virtual void SetUsingCallback(bool bUsingCallback) = 0;
		virtual ERecvType RecvPoll(bool bUseCb=true) = 0;
		virtual void SendHostMsg(uint8_t msgType, uint64_t msgData) = 0;
		virtual bool RecvHostMsg(uint8_t &msgType, uint64_t &msgData) = 0;
		virtual bool PeekHostData(uint64_t & data) = 0;
		virtual int RecvHostData(int qwCnt, uint64_t * pData) = 0;
		virtual bool RecvHostDataMarker() = 0;
		virtual int SendHostData(int qwCnt, uint64_t * pData) = 0;
		virtual bool SendHostDataMarker() = 0;
		virtual bool SendHostHalt() = 0;
		virtual void FlushHostData() = 0;
		virtual bool SendCall(int argc, int * pArgw, uint64_t * pArgv) = 0;
		virtual bool RecvReturn(int argc, uint64_t * pArgv) = 0;
		virtual uint8_t * GetUnitMemBase() = 0;
	};

	class CHtHifBase {
		friend class CHtHifLib;
		friend class CHtModelHifLib;
		friend class CHtUnitBase;
	protected:
		CHtHifBase(CHtHifParams * pParams) {
#			if !defined(HT_MODEL) && !defined(HT_SYSC) && !defined(CNYOS_API) && !defined(HT_LIB_HIF) && !defined(HT_LIB_SYSC)
				m_coproc = WDM_INVALID;
#			endif
			m_pHtHifLibBase = CHtHifLibBase::NewHtHifLibBase(pParams, HT_PERS, this);
			m_allocLock = 0;
		}
		~CHtHifBase() {
			if (m_pHtHifLibBase) {
				delete m_pHtHifLibBase;
				m_pHtHifLibBase = 0;
			}

			HtCpRelease();
		}
		CHtUnitLibBase * AllocUnit(CHtUnitBase * pHtUnitBase, int numaSet) {
			return m_pHtHifLibBase->AllocUnit(pHtUnitBase, numaSet);
		}
		void FreeUnit(CHtUnitLibBase * pHtUnitLibBase) {
			m_pHtHifLibBase->FreeUnit(pHtUnitLibBase);
		}

	public:
		int GetAeCnt();
		int GetUnitCnt();
		int GetUnitMHz() { return m_pHtHifLibBase->GetUnitMHz(); }
		uint64_t GetPartNumber() { return m_pHtHifLibBase->GetPartNumber(); }
		uint8_t * GetAppUnitMemBase(int unitId) { return m_pHtHifLibBase->GetAppUnitMemBase(unitId); }

		void SendAllHostMsg(uint8_t msgType, uint64_t msgData) {
			m_pHtHifLibBase->SendAllHostMsg(msgType, msgData);
		}

	private:
		CSyscTop * NewSyscTop();

		void HtCoprocSysc();

		void HtCpInfo(bool *needFlush, volatile bool *busy, uint64_t *aeg2, uint64_t *aeg3);
		void HtCpDispatch(uint64_t *pBase);
		void HtCpDispatchWait(uint64_t *pBase);
		void HtCpRelease();

	public:
		Ht::EAppMode GetAppMode();

		static void * HostMemAlloc(size_t size, bool bEnableSystemcAddressValidation=true);
		static void HostMemFree(void * pMem);
		static void * HostMemAllocAlign(size_t align, size_t size, bool bEnableSystemcAddressValidation=true);
		static void HostMemFreeAlign(void * pMem);
		static void * HostMemAllocHuge(void * pBaseAddr);
		static void HostMemFreeHuge(void * pMem);

		void * MemAlloc(size_t cnt);
		void MemFree(void * pMem);
		void * MemAllocAlign(size_t align, size_t size);
		void MemFreeAlign(void * pMem);
		void * MemSet(void * pDst, int ch, size_t size);
		void * MemCpy(void * pDst, void * pSrc, size_t size);
		size_t GetMemSize();

	protected:
		void LockAlloc() { while (__sync_fetch_and_or(&m_allocLock, 1) != 0); }
		void UnlockAlloc() { m_allocLock = 0; __sync_synchronize(); }
	protected:
		volatile int64_t m_allocLock;
		CHtHifLibBase * m_pHtHifLibBase;
#		if !defined(HT_MODEL) && !defined(HT_SYSC) && !defined(CNYOS_API) && !defined(HT_LIB_HIF) && !defined(HT_LIB_SYSC)
			wdm_coproc_t m_coproc;
#		endif
	};

	class CHtUnitBase {
	protected:
		CHtUnitBase(CHtHifBase * pHtHifBase, int numaSet) {
			m_pHtHifBase = pHtHifBase;
			m_pHtUnitLibBase = m_pHtHifBase->AllocUnit(this, numaSet);
		}

		uint8_t * GetUnitMemBase() {
			return m_pHtUnitLibBase->GetUnitMemBase();
		}

	public:
		virtual ~CHtUnitBase() {
			m_pHtHifBase->FreeUnit(m_pHtUnitLibBase);
		}

		void SetUsingCallback( bool bUsingCallback ) {
			m_pHtUnitLibBase->SetUsingCallback(bUsingCallback);
		}

	public:
		int GetUnitId() {
			return m_pHtUnitLibBase->GetUnitId();
		}

		ERecvType RecvPoll(bool bUseCb=true) {
			return m_pHtUnitLibBase->RecvPoll(bUseCb);
		}

		virtual void RecvCallback( Ht::ERecvType ) { assert(0); }

		virtual char const * GetModuleName(int modId) { assert(0); return 0; }
		virtual char const * GetSendMsgName(int msgId) { assert(0); return 0; }
		virtual char const * GetRecvMsgName(int msgId) { assert(0); return 0; }

	protected:
		void SendHostMsg(uint8_t msgType, uint64_t msgData) {
			m_pHtUnitLibBase->SendHostMsg(msgType, msgData);
		}

		bool RecvHostMsg(uint8_t &msgType, uint64_t &msgData) {
			return m_pHtUnitLibBase->RecvHostMsg(msgType, msgData);
		}

		int SendHostData(int qwCnt, uint64_t * pData) {
			return m_pHtUnitLibBase->SendHostData(qwCnt, pData);
		}

		bool SendHostDataMarker() {
			return m_pHtUnitLibBase->SendHostDataMarker();
		}

		void FlushHostData() {
			m_pHtUnitLibBase->FlushHostData();
		}

		bool PeekHostData(uint64_t & data) {
			return m_pHtUnitLibBase->PeekHostData(data);
		}

		int RecvHostData(int qwCnt, uint64_t * pData) {
			return m_pHtUnitLibBase->RecvHostData(qwCnt, pData);
		}

		bool RecvHostDataMarker() {
			return m_pHtUnitLibBase->RecvHostDataMarker();
		}

		bool SendCall(int & argc, int * pArgw, uint64_t * pArgv) {
			return m_pHtUnitLibBase->SendCall(argc, pArgw, pArgv);
		}

		bool RecvReturn(int argc, uint64_t * pArgv) {
			return m_pHtUnitLibBase->RecvReturn(argc, pArgv);
		}

	protected:
		CHtUnitLibBase * m_pHtUnitLibBase;

	private:
		CHtHifBase * m_pHtHifBase;
	};

	class CHtHif : public Ht::CHtHifBase {
	public:
		static void* operator new(std::size_t sz){
			try {
				return ::operator new(sz);
			}
			catch (std::bad_alloc) {
				throw CHtException(eHtBadHostAlloc, string("CHtHif::operator new() failed"));
			}
		}

		CHtHif(CHtHifParams * pParams = 0) : CHtHifBase(pParams) {
			m_pthreads = 0;
			m_ppUnits = 0;
		}
		template<typename T> void StartUnitThreads();
		void WaitForUnitThreads();
	private:
		pthread_t * m_pthreads;
		Ht::CHtUnitBase ** m_ppUnits;
	};

	template<typename T> inline void CHtHif::StartUnitThreads() {
		int unitCnt = GetUnitCnt();

		// start threads
		m_pthreads = new pthread_t [unitCnt];
		m_ppUnits = new Ht::CHtUnitBase * [unitCnt];
		for (int i = 0; i < unitCnt; i += 1) {
			m_ppUnits[i] = new T(this);
			assert(m_ppUnits[i]);
			pthread_create(&m_pthreads[i], 0, T::StartUnitThread, m_ppUnits[i]);
		}
	}

	inline void CHtHif::WaitForUnitThreads() {
		// wait for threads to finish
		int unitCnt = GetUnitCnt();
		void * status;
		if (m_ppUnits) {
			for (int i = 0; i < unitCnt; i += 1) {
				if (m_ppUnits[i]) {
					int pthread_join_rtn = pthread_join(m_pthreads[i], &status);
					assert(pthread_join_rtn == 0);
					delete m_ppUnits[i];
					m_ppUnits[i] = 0;
				}
			}
			delete m_ppUnits;
			m_ppUnits = 0;
		}
		if (m_pthreads) {
			delete m_pthreads;
			m_pthreads = 0;
		}
	}

} // namespace Ht
