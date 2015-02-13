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

namespace Ht {

#	ifndef HT_MODEL_NO_UNIT_THREADS
#	define HT_MODEL_UNIT_THREADS
#	endif

	class CHtHifBase;
	class CHtModelUnitBase;
	class CHtModelHifLibBase;
	class CHtModelUnitLibBase;

	extern void HtCoprocModel(Ht::CHtHifLibBase *);

	class CHtModelHifLibBase {
		friend class CHtModelHif;
		friend class CHtModelHifBase;
	protected:
		static CHtModelHifLibBase * NewHtModelHifLibBase(CHtHifLibBase * pHtHifLibBase);
		virtual ~CHtModelHifLibBase() {}
		virtual int GetUnitCnt() { assert(0); return 0; }
		virtual CHtModelUnitLibBase * const * AllocAllUnits() { assert(0); return 0; }
		virtual void FreeAllUnits() { assert(0); }
		virtual CHtModelUnitLibBase * AllocUnit(CHtModelUnitBase * pHtModelUnitBase) { assert(0); return 0; }
		virtual void FreeUnit(CHtModelUnitLibBase * pHtModelUnitLibBase) { assert(0); }
	};

	class CHtModelUnitLibBase {
		friend class CHtModelUnitBase;
	protected:
		virtual ~CHtModelUnitLibBase() {}
		virtual void SetUsingCallback(bool bUsingCallback) = 0;
		virtual ERecvType RecvPoll(bool bUseCbCall, bool bUseCbMsg, bool bUseCbData) = 0;
		virtual void SendHostMsg(uint8_t msgType, uint64_t msgData) = 0;
		virtual bool RecvHostMsg(uint8_t &msgType, uint64_t &msgData) = 0;
		virtual int SendHostData(int qwCnt, uint64_t * pData) = 0;
		virtual bool SendHostDataMarker() = 0;
		virtual void FlushHostData() = 0;
		virtual bool PeekHostData(uint64_t & data) = 0;
		virtual int RecvHostData(int qwCnt, uint64_t * pData) = 0;
		virtual bool RecvHostDataMarker() = 0;
		virtual bool RecvHostHalt() = 0;
		virtual bool RecvCall(int & argc, uint64_t * pArgv) = 0;
		virtual bool SendReturn(int argc, uint64_t * pArgv) = 0;
	};

	class CHtModelHifBase {
		friend class CHtModelUnitBase;
	protected:
		CHtModelHifBase(CHtHifLibBase * pHtHifLibBase) {
			m_pHtModelHifLibBase = CHtModelHifLibBase::NewHtModelHifLibBase(pHtHifLibBase);
		}
		~CHtModelHifBase() {
			delete m_pHtModelHifLibBase;
		}
		CHtModelUnitLibBase * AllocUnit(CHtModelUnitBase * pHtModelUnitBase) {
			return m_pHtModelHifLibBase->AllocUnit(pHtModelUnitBase);
		}
		void FreeUnit(CHtModelUnitLibBase * pHtModelUnitLibBase) {
			m_pHtModelHifLibBase->FreeUnit(pHtModelUnitLibBase);
		}
		int GetUnitCntBase() {
			return m_pHtModelHifLibBase->GetUnitCnt();
		}

	protected:
		CHtModelHifLibBase * m_pHtModelHifLibBase;
	};

	class CHtModelUnitBase {
		friend class CHtModelUnitLib;
	protected:
		CHtModelUnitBase(CHtModelHifBase * pHtModelHifBase) {
			m_pHtModelHifBase = pHtModelHifBase;
			m_pHtModelUnitLibBase = m_pHtModelHifBase->AllocUnit(this);
		}

	public:
		virtual ~CHtModelUnitBase() {
			m_pHtModelHifBase->FreeUnit(m_pHtModelUnitLibBase);
		}

	public:
		void SetUsingCallback( bool bUsingCallback ) {
			m_pHtModelUnitLibBase->SetUsingCallback(bUsingCallback);
		}

		Ht::ERecvType RecvPoll(bool bUseCbCall=true, bool bUseCbMsg=true, bool bUseCbData=true) {
			return m_pHtModelUnitLibBase->RecvPoll(bUseCbCall, bUseCbMsg, bUseCbData);
		}

		bool RecvHostHalt() {
			return m_pHtModelUnitLibBase->RecvHostHalt();
		}

		virtual void RecvCallback( Ht::ERecvType ) { assert(0); }

	protected:
		void SendHostMsg(uint8_t msgType, uint64_t msgData) {
			m_pHtModelUnitLibBase->SendHostMsg(msgType, msgData);
		}

		bool RecvHostMsg(uint8_t &msgType, uint64_t &msgData) {
			return m_pHtModelUnitLibBase->RecvHostMsg(msgType, msgData);
		}

		int SendHostData(int qwCnt, uint64_t * pData) {
			return m_pHtModelUnitLibBase->SendHostData(qwCnt, pData);
		}

		bool SendHostDataMarker() {
			return m_pHtModelUnitLibBase->SendHostDataMarker();
		}

		void FlushHostData() {
			m_pHtModelUnitLibBase->FlushHostData();
		}

		bool PeekHostData(uint64_t & data) {
			return m_pHtModelUnitLibBase->PeekHostData(data);
		}

		int RecvHostData(int qwCnt, uint64_t * pData) {
			return m_pHtModelUnitLibBase->RecvHostData(qwCnt, pData);
		}

		bool RecvHostDataMarker() {
			return m_pHtModelUnitLibBase->RecvHostDataMarker();
		}

		bool RecvCall(int & argc, uint64_t * pArgv) {
			return m_pHtModelUnitLibBase->RecvCall(argc, pArgv);
		}

		bool SendReturn(int argc, uint64_t * pArgv) {
			return m_pHtModelUnitLibBase->SendReturn(argc, pArgv);
		}

	protected:
		CHtModelUnitLibBase * m_pHtModelUnitLibBase;

	private:
		CHtModelHifBase * m_pHtModelHifBase;
	};

	class CHtModelHif : public Ht::CHtModelHifBase {
	public:
		CHtModelHif(Ht::CHtHifLibBase * pHtHifLibBase) : Ht::CHtModelHifBase(pHtHifLibBase) {}
		int GetUnitCnt() { return GetUnitCntBase(); }
		template<typename T> void StartUnitThreads();
		void WaitForUnitThreads();
	private:
		pthread_t * m_pthreads;
		Ht::CHtModelUnitBase ** m_ppUnits;
	};

#	ifdef HT_MODEL_UNIT_THREADS
	template<typename T> inline void CHtModelHif::StartUnitThreads() {
		int unitCnt = GetUnitCnt();

		// start threads
		m_pthreads = new pthread_t [unitCnt];
		m_ppUnits = new Ht::CHtModelUnitBase * [unitCnt];
		for (int i = 0; i < unitCnt; i += 1) {
			m_ppUnits[i] = new T(this);
			assert(m_ppUnits[i]);
			pthread_create(&m_pthreads[i], 0, T::StartUnitThread, m_ppUnits[i]);
		}
	}

	inline void CHtModelHif::WaitForUnitThreads() {
		// wait for threads to finish
		int unitCnt = GetUnitCnt();
		void * status;
		for (int i = 0; i < unitCnt; i += 1) {
			if (m_ppUnits[i]) {
				int pthread_join_rtn = pthread_join(m_pthreads[i], &status);
				assert(pthread_join_rtn == 0);
				delete m_ppUnits[i];
				m_ppUnits[i] = 0;
			}
		}
		delete m_pthreads;
	}
#	endif

} // namespace Ht
