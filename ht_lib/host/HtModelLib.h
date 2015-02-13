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

#include "HtHifLib.h"
#include "HtModel.h"

namespace Ht {
	class CHtModelHifBase;
	class CHtModelHifLib;
	class CHtModelHifLibBase;

	class CHtModelUnitLib : public CHtModelUnitLibBase {
		friend class CHtModelHifLib;
		friend class CHtModelHifLibBase;
	public:
		int GetUnitId() { return m_unitId; }
		bool RecvHostHalt() { return RecvPoll() == eRecvHalt; }

	public:
		CHtModelUnitLib();
		CHtModelUnitLib(CHtModelHifLib *pHtModelHifLib, int unitId);
		~CHtModelUnitLib();

		void SetUsingCallback(bool bUsingCallback) {
			m_bUsingCallback = bUsingCallback;
		}

		ERecvType RecvPoll(bool bUseCbCall=true, bool bUseCbMsg=true, bool bUseCbData=true) {
			return MsgLoop(bUseCbCall, bUseCbMsg, bUseCbData);
		}

		bool RecvHostMsg(uint8_t & msgType, uint64_t & msgData);
		void SendHostMsg(uint8_t msgType, uint64_t msgData);

		bool PeekHostData(uint64_t & data);
		int  RecvHostData(int qwCnt, uint64_t * pData);
		bool RecvHostDataMarker();
		int  SendHostData(int qwCnt, uint64_t * pData);
		bool SendHostDataMarker();
		void FlushHostData() { FlushOutBlk(true); }

		bool RecvCall(int & argc, uint64_t * pArgv);
		bool SendReturn(int argc, uint64_t * pArgv);

	private:
		void Callback(ERecvType recvType) {
			m_pHtModelUnitBase->RecvCallback(recvType);
		}

		void OutBlkTimer();

		void LockOutBlk(bool bLock=true) { while (bLock && __sync_fetch_and_or(&m_outLock, 1) != 0)
							; assert(m_outLock != 0); }
		void UnlockOutBlk(bool bUnlock=true) { if (bUnlock) { m_outLock = 0; __sync_synchronize(); } }

		ERecvType MsgLoop(bool bUseCbCall=false, bool bUseCbMsg=false, bool bUseCbData=false);

		void FlushOutBlk(bool bLock=false, int ctl=0);
		void QueInMsgs();
		void NextInMsg();
		void NextInBlk(bool bLock=false);
		void NextOutBlk();
		void NextInMsgQue() {
			m_inMsgQue.Pop();
			m_inMsgQueLoopCnt = 0;
	#ifdef HT_AVL_TEST
			m_bInBlkRdySeen = false;
	#endif
		}

		bool SendHostHalt();
		int SendDataInternal(int qwCnt, uint64_t * pData, bool bLock=false);

		void SendMsgInternal(uint8_t msgType, uint64_t msgData, bool bLock=false);
		bool RecvMsgInternal(uint8_t & msgType, uint64_t & msgData);

	private:
		int m_unitId;
		CHtModelHifLib * m_pHtModelHifLib;
		CHtModelUnitBase * m_pHtModelUnitBase;
		bool m_bUsingCallback;
		bool m_bFree;
		bool m_bHaltSeen;

		uint32_t m_ctlQueMask;
		int m_inCtlRdIdx;

		CHtCtrlMsg * m_pInCtlQue;
		CHtCtrlMsg * m_pInCtlMsg;
		CQueue<CHtCtrlMsg, 512> m_inMsgQue;
		uint32_t m_inMsgQueLoopCnt;

		CHtCtrlMsg * m_pOutCtlQue;
		int volatile m_outCtlWrIdx;

		uint64_t * m_pInBlkBase;
		uint64_t * m_pInBlk;
		uint32_t m_inBlkSizeW;
		uint32_t m_inBlkCntW;
		int m_inBlkIdx;
		uint32_t m_inBlkSize;
		uint32_t m_inBlkMask;
		int m_inBlkRdIdx;
		int m_inBlkDataSize;

		int volatile m_outBlkAvailCnt;
		uint64_t * m_pOutBlkBase;
		uint64_t * volatile m_pOutBlk;
		uint32_t m_outBlkSizeW;
		uint32_t m_outBlkCntW;
		int m_outBlkSize;
		uint32_t m_outBlkMask;
		int volatile m_outBlkWrIdx;
		int volatile m_outBlkIdx;
		uint32_t m_outBlkTimerClks;
		int64_t volatile m_outLock;

		uint32_t m_ctlTimerClks;

	#ifdef HT_AVL_TEST
		uint32_t volatile m_inRdyMask;
		bool m_bInBlkRdySeen;
		uint32_t volatile m_outAvlMask;
	#endif
	};

	class CHtModelHifLib : public CHtModelHifLibBase {
		friend class CHtModelUnitLib;
	public:
		CHtModelHifLib(CHtHifLibBase * pHtHifLibBase);
		~CHtModelHifLib();

		int GetUnitCnt() { return m_pHtHifLibBase->GetUnitMap()->GetUnitCnt(); }

		CHtHifLibBase * GetHtHifLibBase() { return m_pHtHifLibBase; }

		CHtModelUnitLibBase * const * AllocAllUnits() {
			LockAlloc();
			for (int unitId = 0; unitId < GetUnitCnt(); unitId += 1) {
				if (m_ppHtModelUnits[unitId] == 0)
					InitUnit(unitId);
				else if (m_ppHtModelUnits[unitId]->m_bFree)
					m_ppHtModelUnits[unitId]->m_bFree = false;
			}
			UnlockAlloc();
			return (CHtModelUnitLibBase * const *)m_ppHtModelUnits;
		}

		void FreeAllUnits() { /* model does not free units */ }

		CHtModelUnitLib * AllocUnit(CHtModelUnitBase * pHtModelUnitBase) {
			LockAlloc();
			int unitId;
			for (unitId = 0; unitId < GetUnitCnt(); unitId += 1) {
				if (m_ppHtModelUnits[unitId] == 0) {
					InitUnit(unitId);
					m_ppHtModelUnits[unitId]->m_pHtModelUnitBase = pHtModelUnitBase;
					break;
				} else if (m_ppHtModelUnits[unitId]->m_bFree) {
					m_ppHtModelUnits[unitId]->m_bFree = false;
					m_ppHtModelUnits[unitId]->m_pHtModelUnitBase = pHtModelUnitBase;
					break;
				}
			}
			UnlockAlloc();
			return unitId < GetUnitCnt() ? m_ppHtModelUnits[unitId] : 0;
		}

		void FreeUnit(CHtModelUnitLibBase * pHtModelUnitLibBase) {
			// units are currently use once
		}

	private:
		void InitUnit(int unitId);

		void StartTimer(int oBlkTimerClks);

		void CoprocTimerThread();
		static void *CoprocTimerThreadStart(void *pThis) {
			((CHtModelHifLib*)pThis)->CoprocTimerThread();
			return 0;
		}

		void LockAlloc() { while (__sync_fetch_and_or(&m_allocLock, 1) != 0); }
		void UnlockAlloc() { m_allocLock = 0; __sync_synchronize(); }

	protected:
		CHtHifLibBase * m_pHtHifLibBase;
		CHtHifUnitMap * m_pUnitMap;
		CHtModelUnitLib * volatile * m_ppHtModelUnits;
		bool volatile * m_pHaltSeen;
		volatile int64_t m_allocLock;

	private:
		bool m_bTimerStarted;
		bool volatile m_bTimerFinished;
		int m_oBlkTimerUSec;
	};

	inline void CHtModelHifLib::InitUnit(int unitId)
	{
		assert(unitId < GetUnitCnt());
		if (m_ppHtModelUnits[unitId] == 0)
			m_ppHtModelUnits[unitId] = new CHtModelUnitLib(this, unitId);
	}

} // namespace Ht
