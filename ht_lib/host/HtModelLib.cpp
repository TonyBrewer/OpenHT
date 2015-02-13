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
#include "HtModelLib.h"

namespace Ht {

	CHtModelHifLibBase * CHtModelHifLibBase::NewHtModelHifLibBase(CHtHifLibBase * pHtHifLibBase) {
		return new CHtModelHifLib(pHtHifLibBase);
	}

	CHtModelHifLib::CHtModelHifLib(CHtHifLibBase * pHtHifLibBase)
	{
		m_allocLock = 0;
		m_bTimerStarted = false;
		m_bTimerFinished = false;
		m_pHtHifLibBase = pHtHifLibBase;

		int unitCnt = m_pHtHifLibBase->GetUnitMap()->GetUnitCnt();

		m_ppHtModelUnits = new CHtModelUnitLib * [unitCnt];
		memset((void *)m_ppHtModelUnits, 0, unitCnt * sizeof(CHtModelUnitLib *));

		m_pHaltSeen = new bool [unitCnt];
		memset((void *)m_pHaltSeen, false, unitCnt * sizeof(bool));
	}

	CHtModelUnitLib::CHtModelUnitLib(CHtModelHifLib * pHtModelHifLib, int unitId)
	{
		m_pHtModelHifLib = pHtModelHifLib;
		m_unitId = unitId;
		m_bFree = false;
		m_bHaltSeen = false;
		m_outLock = 0;

		int aeId, auId;
		m_pHtModelHifLib->m_pHtHifLibBase->GetUnitMap()->FindAeAu(unitId, aeId, auId);

		CHtCtrlMsg * pMsgQueBase = (CHtCtrlMsg *)pHtModelHifLib->GetHtHifLibBase()->GetCtlQueMemBase(aeId);
		m_pInCtlQue = pMsgQueBase + auId * HIF_CTL_QUE_CNT * 2;
		m_pOutCtlQue = pMsgQueBase + auId * HIF_CTL_QUE_CNT * 2 + HIF_CTL_QUE_CNT;

		m_bUsingCallback = false;
		m_ctlQueMask = HIF_CTL_QUE_CNT-1;
		m_pInCtlMsg = m_pInCtlQue;
		m_inBlkIdx = 0;
		m_outBlkIdx = 0;
		m_inCtlRdIdx = 0;
		m_inBlkRdIdx = 0;
		m_outCtlWrIdx = 0;
		m_outBlkWrIdx = 0;
		m_outBlkAvailCnt = 0;
		m_inBlkDataSize = 0;
		m_pInBlkBase = 0;
		m_pInBlk = 0;
		m_pOutBlkBase = 0;
		m_pOutBlk = 0;

	#ifdef HT_AVL_TEST
		m_outBlkMask = 0xf;
		m_inRdyMask = 0;
		m_bInBlkRdySeen = false;
		m_outAvlMask = 0;
	#endif
	}

	CHtModelHifLib::~CHtModelHifLib()
	{
		int unitCnt = m_pHtHifLibBase->GetUnitMap()->GetUnitCnt();

		for (int unitId = 0; unitId < unitCnt; unitId += 1)
			m_ppHtModelUnits[unitId]->SetUsingCallback(false);

		// wait for all units to have seen the halt message
		bool bDone;
		do {
			bDone = true;
			for (int unitId = 0; unitId < unitCnt; unitId += 1) {
				if (m_pHaltSeen[unitId])
					continue;
				switch (m_ppHtModelUnits[unitId]->RecvPoll()) {
				case eRecvIdle:
					break;
				case eRecvHostMsg:
					{
						uint8_t msgType;
						uint64_t msgData;
						m_ppHtModelUnits[unitId]->RecvHostMsg(msgType, msgData);
					}
					break;
				case eRecvHalt:
					m_ppHtModelUnits[unitId]->m_bHaltSeen = true;
					m_pHaltSeen[unitId] = true;
					continue;
				default:
					assert(0);
				}
				bDone = false;
			}
			if (!bDone)
				usleep(100);
		} while (!bDone);

		while (!m_bTimerFinished);

		for (int unitId = 0; unitId < unitCnt; unitId += 1)
			delete m_ppHtModelUnits[unitId];

		delete[] m_ppHtModelUnits;
		delete[] m_pHaltSeen;
	}


	CHtModelUnitLib::CHtModelUnitLib()
	{
	}

	CHtModelUnitLib::~CHtModelUnitLib()
	{
	}

	void CHtModelHifLib::StartTimer(int oBlkTimerClks)
	{
		if (m_bTimerStarted)
			return;
	
		if (oBlkTimerClks == 0) {
			m_bTimerFinished = true;
			return;
		}

		m_bTimerStarted = true;

		m_oBlkTimerUSec = (oBlkTimerClks + 150-1) / 150;  // assume 6ns clock

		pthread_t timerThread;
		pthread_create(&timerThread, 0, CoprocTimerThreadStart, this);
	}

	void CHtModelHifLib::CoprocTimerThread()
	{
		pthread_detach(pthread_self());
		bool bDone;
		do {
			bDone = true;

			usleep(m_oBlkTimerUSec);

			// check if inbound block queue needs flushed
			for (int i = 0; i < m_pHtHifLibBase->GetUnitMap()->GetUnitCnt(); i += 1)
				if (!m_pHaltSeen[i]) {
					bDone = false;
					if (m_ppHtModelUnits[i] != 0)
						m_ppHtModelUnits[i]->OutBlkTimer();
				}
		} while (!bDone);

		m_bTimerFinished = true;
		pthread_exit(0);
	}

	void CHtModelUnitLib::OutBlkTimer()
	{
		LockOutBlk();
		FlushOutBlk();
		UnlockOutBlk();
	}

	void CHtModelUnitLib::SendHostMsg(uint8_t msgType, uint64_t msgData)
	{
		LockOutBlk();
		FlushOutBlk();
		SendMsgInternal(msgType, msgData); 
		UnlockOutBlk();
	}

	bool CHtModelUnitLib::RecvHostMsg(uint8_t & msgType, uint64_t & msgData)
	{
		bool bRcvd = RecvMsgInternal(msgType, msgData);
		return bRcvd;
	}

	bool CHtModelUnitLib::SendReturn(int argc, uint64_t * pArgv)
	{
		LockOutBlk();
		FlushOutBlk();    // flush input block data

		if (m_outBlkAvailCnt < 2) {
			UnlockOutBlk();

			QueInMsgs();

			LockOutBlk();
			if (m_outBlkAvailCnt < 2) {
				UnlockOutBlk();
				return false;
			}
		}

		uint64_t idx = 0;
	#ifdef HT_AVL_TEST
		idx = ((uint64_t)(m_outBlkIdx & m_outBlkMask) << 48) | (1ull << 36);
	#endif
		NextOutBlk();       // empty out block for call (why?)

		int cnt = SendDataInternal(argc, pArgv);
		assert(cnt == argc);

		int size = (argc << 3) | HIF_DQ_CALL;
		SendMsgInternal(HIF_CMD_OBLK_RDY, idx | size);

		FlushOutBlk();    // flush call args
		assert(m_outBlkAvailCnt >= 0);

		UnlockOutBlk();
		return true;
	}

	bool CHtModelUnitLib::RecvCall(int & argc, uint64_t * pArgv)
	{
		ERecvType recvType = MsgLoop(false, true, true);

		if (recvType != eRecvCall)
			return false;

		uint8_t msgType = m_inMsgQue.Front().GetMsgType();
		uint64_t msgData = m_inMsgQue.Front().GetMsgData();

		assert(msgType == HIF_CMD_IBLK_RDY && (msgData & 0x7) == HIF_DQ_CALL);
		int recv_argc = (int)(msgData >> 3);
		assert(argc >= recv_argc);
	#ifdef HT_AVL_TEST
		assert((m_inBlkIdx & m_inBlkMask) == ((msgData >> 48) & m_inBlkMask));
	#endif

		NextInMsgQue();
		NextInBlk(true);

		if ((int)(msgData >> 3) > 0) {
			assert(m_inMsgQue.Front().GetMsgType() == HIF_CMD_IBLK_RDY && (m_inMsgQue.Front().GetMsgData() & 0x7) == 0);
			assert(((msgData >> 3) & 0xff) == ((m_inMsgQue.Front().GetMsgData() >> 3) & 0xff));

	#ifdef HT_AVL_TEST
			assert((m_inRdyMask & (1 << ((m_inMsgQue.Front().GetMsgData() >> 48) & m_inBlkMask))) == 0);
			m_inRdyMask |= 1 << ((m_inMsgQue.Front().GetMsgData() >> 48) & m_inBlkMask);
	#endif

			uint64_t * pInBlk = &m_pInBlkBase[(m_inBlkIdx & m_inBlkMask) * m_inBlkSize];

			memcpy(pArgv, pInBlk, argc * sizeof(uint64_t));

	#ifdef HT_AVL_TEST
			assert(m_inRdyMask & (1 << (m_inBlkIdx & m_inBlkMask)));
	#endif

			NextInMsgQue();
			NextInBlk(true);
		}

		return true;
	}

	bool CHtModelUnitLib::RecvMsgInternal(uint8_t & msgType, uint64_t & msgData)
	{
		ERecvType recvType = MsgLoop();

		if (recvType != eRecvHostMsg)
			return false;

		msgType = m_inMsgQue.Front().GetMsgType();
		msgData = m_inMsgQue.Front().GetMsgData();

		NextInMsgQue();

		return true;
	}

	void CHtModelUnitLib::NextInBlk(bool bLock)
	{
		LockOutBlk(bLock);

	#ifdef HT_AVL_TEST
		assert(m_inRdyMask & (1 << (m_inBlkIdx & m_inBlkMask)));
		m_inRdyMask &= ~(1 << (m_inBlkIdx & m_inBlkMask));
	#endif

		SendMsgInternal(HIF_CMD_IBLK_AVL, m_inBlkIdx);

		m_inBlkIdx += 1;
		m_pInBlk = m_pInBlkBase + (m_inBlkIdx & m_inBlkMask) * m_inBlkSize;
		m_inBlkRdIdx = 0;

		UnlockOutBlk(bLock);
	}

	void CHtModelUnitLib::SendMsgInternal(uint8_t msgType, uint64_t msgData, bool bLock)
	{
		LockOutBlk(bLock);

		CHtCtrlMsg *pOutMsg = m_pOutCtlQue + (m_outCtlWrIdx++ & m_ctlQueMask);
	
		UnlockOutBlk(bLock);

		CHtCtrlMsg ctrlMsg(msgType, msgData);

		// wait until slot empty
		while (pOutMsg->IsValid())
			usleep(1);

		*pOutMsg = ctrlMsg;
	}

	void CHtModelUnitLib::NextInMsg()
	{
		assert(m_pInCtlMsg->IsValid());

		m_pInCtlMsg->Clear();
		m_inCtlRdIdx += 1;
		m_pInCtlMsg = &m_pInCtlQue[m_inCtlRdIdx & m_ctlQueMask];
	}

	void CHtModelUnitLib::QueInMsgs()
	{
		// handle inbound OBLK_AVAIL messages and queue remainer
		for(;;) {
			CHtCtrlMsg ctrlMsg = *m_pInCtlMsg;

			if (!ctrlMsg.IsValid() || m_inMsgQue.IsFull())
				break;

			uint8_t msgType = ctrlMsg.GetMsgType();

			if (msgType ==  HIF_CMD_OBLK_AVL) {
	#ifdef HT_AVL_TEST
				uint64_t msgData = ctrlMsg.GetMsgData();
				if ((m_outAvlMask & (1 << (msgData & m_outBlkMask))) != 0) {
					printf("src = %d\n", ((msgData >> 36) & 0xf));
					assert((m_outAvlMask & (1 << (msgData & m_outBlkMask))) == 0);
				}
	#endif
				LockOutBlk();
				m_outBlkAvailCnt += 1;
	#ifdef HT_AVL_TEST
				m_outAvlMask |= 1 << (msgData & m_outBlkMask);
	#endif
				UnlockOutBlk();
			} else
				m_inMsgQue.Push(ctrlMsg);

			NextInMsg();
		}
	}

	ERecvType CHtModelUnitLib::MsgLoop(bool bUseCbCall, bool bUseCbMsg, bool bUseCbData)
	{
		ERecvType rtnRecvType;

		if (m_bHaltSeen)
			return eRecvHalt;

		QueInMsgs();

		for(;;) {
			if (m_inMsgQue.Empty())
				return eRecvIdle;

			CHtCtrlMsg ctrlMsg = m_inMsgQue.Front();

			uint8_t msgType = ctrlMsg.GetMsgType();
			uint64_t msgData = ctrlMsg.GetMsgData();

			switch (msgType) {
			case HIF_CMD_IBLK_ADR:
				m_pInBlk = m_pInBlkBase = (uint64_t *)msgData;
				NextInMsgQue();
				continue;
			case HIF_CMD_OBLK_ADR:
				m_pOutBlk = m_pOutBlkBase = (uint64_t *)msgData;
				NextInMsgQue();
				continue;
			case HIF_CMD_IBLK_PARAM:
				m_inBlkSizeW = msgData & 0xff;
				m_inBlkCntW = (msgData >> 8) & 0xff;
				m_inBlkSize = 1 << (m_inBlkSizeW-3);
				m_inBlkMask = (1 << m_inBlkCntW)-1;
				NextInMsgQue();
				continue;
			case HIF_CMD_OBLK_PARAM:
				m_outBlkSizeW = msgData & 0xff;
				m_outBlkCntW = (msgData >> 8) & 0xff;
				m_outBlkTimerClks = (msgData >> 16) & 0xffffffff;
				m_outBlkSize = 1 << (m_outBlkSizeW-3);
				m_outBlkMask = (1 << m_outBlkCntW)-1;
				m_pHtModelHifLib->StartTimer(m_outBlkTimerClks);
				NextInMsgQue();
				continue;
			case HIF_CMD_CTL_PARAM:
				m_ctlTimerClks = msgData & 0xffffffff;
				NextInMsgQue();
				continue;
			case HIF_CMD_OBLK_FLSH_ADR:
				NextInMsgQue();
				continue;
			case HIF_CMD_IBLK_RDY:
	#ifdef HT_AVL_TEST
				if (!m_bInBlkRdySeen) {
					assert((m_inRdyMask & (1 << ((msgData >> 48) & m_inBlkMask))) == 0);
					m_inRdyMask |= 1 << ((msgData >> 48) & m_inBlkMask);
					m_bInBlkRdySeen = true;
				}
	#endif

				switch (msgData & 0x7) {
				case HIF_DQ_NULL:
					{
						assert((msgData & ~0x7ULL) == 0);
						if (bUseCbData && m_bUsingCallback) {
							Callback(eRecvHostDataMarker);
							continue;
						} else
							rtnRecvType = eRecvHostDataMarker;
					}
					break;
				case 0:
					{
						if (m_inBlkDataSize == 0)
							m_inBlkDataSize = (int)(msgData >> 3);

						if (bUseCbData && m_bUsingCallback) {
							Callback(eRecvHostData);
							continue;
						} else
							rtnRecvType = eRecvHostData;
					}
					break;
				case HIF_DQ_CALL:
					{
						// check if arguments are available
						if ((int)(msgData >> 3) == 0) {
							if (bUseCbCall && m_bUsingCallback) {
								Callback(eRecvCall);
								continue;
							} else {
								rtnRecvType = eRecvCall;
								break;
							}
						}

						if (m_inMsgQue.Size() >= 2) {
							if (bUseCbCall && m_bUsingCallback) {
								Callback(eRecvCall);
								continue;
							} else
								rtnRecvType = eRecvCall;
						} else
							return eRecvIdle;
					}
					break;
				case HIF_DQ_HALT:
					{
						NextInMsgQue();
						NextInBlk(true);
						while (!SendHostHalt())
							QueInMsgs();
						m_bHaltSeen = true;
						rtnRecvType = eRecvHalt;
					}
					break;
				default:
					assert(0);
				}
				break;
			default:
				if (msgType >= 16) {
					if (bUseCbMsg && m_bUsingCallback) {
						Callback(eRecvHostMsg);
						continue;
					} else
						rtnRecvType = eRecvHostMsg;
				} else
					assert(0);
				break;
			}

			break;
		}

		if (m_inMsgQueLoopCnt <= 1000)
			m_inMsgQueLoopCnt += 1;

		if (m_inMsgQueLoopCnt == 1000) {
			printf("Model side HIF deadlock detected, next receive message type: ");
			switch (rtnRecvType) {
			case eRecvHostMsg: printf("eRecvHostMsg\n"); break;
			case eRecvHostData: printf("eRecvHostData\n"); break;
			case eRecvCall: printf("eRecvCall\n"); break;
			case eRecvHalt: printf("eRecvHalt\n"); break;
			case eRecvHostDataMarker: printf("eRecvHostDataMarker\n"); break;
			default: assert(0);
			}
		}

		return rtnRecvType;
	}

	bool CHtModelUnitLib::SendHostHalt()
	{
		LockOutBlk();
		FlushOutBlk();

		if (m_outBlkAvailCnt < 1) {
			UnlockOutBlk();
			QueInMsgs();

			LockOutBlk();
			if (m_outBlkAvailCnt < 1) {
				UnlockOutBlk();
				return false;
			}
		}

		FlushOutBlk(false, HIF_DQ_HALT);

		UnlockOutBlk();
		return true;
	}

	bool CHtModelUnitLib::SendHostDataMarker()
	{
		LockOutBlk();
		FlushOutBlk();

		if (m_outBlkAvailCnt < 1) {
			UnlockOutBlk();	
			QueInMsgs();

			LockOutBlk();
			if (m_outBlkAvailCnt < 1) {
				UnlockOutBlk();	
				return false;
			}
		}

		FlushOutBlk(false, HIF_DQ_NULL);

		UnlockOutBlk();
		return true;
	}

	int CHtModelUnitLib::SendHostData(int qwCnt, uint64_t * pData)
	{
		return SendDataInternal(qwCnt, pData, true);
	}

	int CHtModelUnitLib::SendDataInternal(int qwCnt, uint64_t * pData, bool bLock)
	{
		LockOutBlk(bLock);

		int sentCnt = 0;
		while (sentCnt < qwCnt) {
			if (m_outBlkAvailCnt == 0) {
				UnlockOutBlk();
				QueInMsgs();

				LockOutBlk();
				if (m_outBlkAvailCnt == 0) {
					UnlockOutBlk();
					return sentCnt;
				}
			}

			assert(m_outBlkAvailCnt > 0);
	#ifdef HT_AVL_TEST
			assert(m_outAvlMask & (1 << (m_outBlkIdx & m_outBlkMask)));
	#endif

			int cnt = min(qwCnt - sentCnt, m_outBlkSize - m_outBlkWrIdx);
			memcpy(&m_pOutBlk[m_outBlkWrIdx], pData + sentCnt, cnt * sizeof(uint64_t));

			m_outBlkWrIdx += cnt;
			sentCnt += cnt;

			assert(m_outBlkWrIdx <= m_outBlkSize);

			if (m_outBlkWrIdx == m_outBlkSize)
				FlushOutBlk();
		}

		UnlockOutBlk(bLock);
		return sentCnt;
	}

	void CHtModelUnitLib::FlushOutBlk(bool bLock, int ctl)
	{
		LockOutBlk(bLock);

		if (m_outBlkWrIdx == 0 && ctl == 0) {
			UnlockOutBlk(bLock);
			return;
		}

		uint64_t idx = 0;
	#ifdef HT_AVL_TEST
		idx = ((uint64_t)(m_outBlkIdx & m_outBlkMask) << 48) | (2ull << 36);
	#endif
		int size = (m_outBlkWrIdx << 3) | ctl;

		NextOutBlk();

		SendMsgInternal(HIF_CMD_OBLK_RDY, idx | size);

		UnlockOutBlk(bLock);
	}

	void CHtModelUnitLib::NextOutBlk()
	{
		assert(m_outLock != 0);
		assert(m_outBlkAvailCnt > 0);

	#ifdef HT_AVL_TEST
		assert(m_outAvlMask & (1 << (m_outBlkIdx & m_outBlkMask)));
		m_outAvlMask &= ~(1ul << (m_outBlkIdx & m_outBlkMask));
	#endif

		m_outBlkAvailCnt -= 1;
		m_outBlkWrIdx = 0;
		m_outBlkIdx += 1;
		m_pOutBlk = m_pOutBlkBase + (m_outBlkIdx & m_outBlkMask) * m_outBlkSize;
	}

	bool CHtModelUnitLib::PeekHostData(uint64_t & data) {
		if (m_inBlkDataSize == 0 && MsgLoop(true, true, false) != eRecvHostData)
			return false;

		assert(m_inBlkDataSize > 0);
		data = m_pInBlk[m_inBlkRdIdx];
		return true;
	}

	bool CHtModelUnitLib::RecvHostDataMarker()
	{
		if (m_inBlkDataSize > 0 || MsgLoop(true, true, false) != eRecvHostDataMarker)
			return false;

		NextInMsgQue();
		NextInBlk(true);

		return true;
	}

	int CHtModelUnitLib::RecvHostData(int qwCnt, uint64_t * pData)
	{
		int recvCnt = 0;
		while (recvCnt < qwCnt && (m_inBlkDataSize > 0 || MsgLoop(true, true, false) == eRecvHostData)) {

			int cnt = min(m_inBlkDataSize, qwCnt - recvCnt);
			memcpy(pData + recvCnt, m_pInBlk + m_inBlkRdIdx, cnt * sizeof(uint64_t));

			m_inBlkRdIdx += cnt;
			m_inBlkDataSize -= cnt;
			recvCnt += cnt;

			if (m_inBlkDataSize == 0) {
				NextInMsgQue();
				NextInBlk(true);
			}
		}

		return recvCnt;
	}
} // namespace Ht
