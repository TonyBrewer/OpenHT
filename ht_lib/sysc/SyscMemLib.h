/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
// SyscMemLib.h
//
//////////////////////////////////////////////////////////////////////
#pragma once

#include "HtMemTrace.h"

#define BANK_CNT 1024
#define BANK_MASK (BANK_CNT-1)

#define MAX_SYSC_MEM 64

namespace Ht {
	extern int g_seqNum;

	struct CBankReq {
		CBankReq() {}
		CBankReq(sc_time startTime, sc_time bankAvlTime, const CMemRdWrReqIntf &memReq)
			: m_startTime(startTime), m_bankAvlTime(bankAvlTime), m_memReq(memReq) {}

		sc_time m_startTime;
		sc_time m_bankAvlTime;
		uint16_t m_host;
		short m_bank;
		int m_seqNum;
		CMemRdWrReqIntf m_memReq;
		uint64_t m_data[8];
		int m_reqQwCnt;

	public:
		static CBankReq * m_pFree;
		static CBankReq * m_pBlock;
		static int m_blockCnt;

		void * operator new(size_t size) {
			if (m_pFree) {
				CBankReq * pNew = (CBankReq *)m_pFree;
				m_pFree = (CBankReq *) (*(CBankReq **)m_pFree);
				pNew->m_seqNum = g_seqNum++;
				return pNew;
			}

			if (m_blockCnt == 0)
				m_pBlock = ::new CBankReq [m_blockCnt = 500];

			CBankReq * pNew = m_pBlock++;
			m_blockCnt -= 1;
			pNew->m_seqNum = g_seqNum++;
			return pNew;
		}
		void operator delete(void * pOld) {
			*(CBankReq **)pOld = m_pFree;
			m_pFree = (CBankReq *) pOld;
		}
	};

	extern list<CBankReq *> * g_pSyscMemQue[MAX_SYSC_MEM];
	extern int g_syscMemCnt;

	struct CMemReq {
		CMemReq() {}
		CMemReq(sc_time time, uint16_t tid, uint8_t type, uint64_t addr, uint64_t data)
			: m_time(time), m_tid(tid), m_type(type), m_addr(addr),
			m_data(data), m_queTime(sc_time_stamp()) {}

		sc_time m_time;
		uint16_t m_tid;
		uint8_t m_type;
		uint64_t m_addr;
		uint64_t m_data;
		sc_time m_queTime;
	};

	struct CMemRdWrReqTime {
		CMemRdWrReqIntf m_req;
		sc_time m_time;
		uint32_t m_host;
		int m_reqQwCnt;
		uint64_t m_data[8];
	};

	struct CSyscMemLib {
		CSyscMemLib(CSyscMem * pSyscMem) {
			m_pSyscMem = pSyscMem;
			m_rdReqValid = 0;
			m_wrReqValid = 0;
			m_rdReqRr = 0;
			m_wrReqRr = 0;
			m_rdReqCnt = 0;
			m_wrReqCnt = 0;
			m_rdReqQwIdx = 0;
			m_wrReqQwIdx = 0;
			m_pNewReq = 0;
			m_pReqTime = 0;
			m_wrDataIdx = 0;

			m_reqRndFullCnt = 0;
			m_bReqRndFull = false;

			assert(g_syscMemCnt < MAX_SYSC_MEM);
			m_syscMemId = g_syscMemCnt++;
			g_pSyscMemQue[m_syscMemId] = &m_bankBusyQue;

			char const * ep = getenv("CNY_HTMEM");
			m_bMemDump = ep && strcmp(ep, "1") == 0;
			m_memName = m_pSyscMem->name();
			m_memName.erase(0, m_memName.find_last_of('.')+1);
			m_memName.append(": ");
		}

		void SyscMemLib();

	private:
		CSyscMem * m_pSyscMem;
		deque<CMemRdWrReqTime> m_memReqQue;

		deque<CBankReq *> m_bankReqQue;
		deque<CBankReq *> m_hostReqQue;
		list<CBankReq *> m_bankBusyQue;
		int m_syscMemId;
		CBankReq * m_pNewReq;
		CMemRdWrReqTime * m_pReqTime;
		int m_wrDataIdx;

		static int g_memPlacementErrorCnt;

#		define MEM_REQ_MAX 32
		CMemRdWrReqTime m_rdReqList[MEM_REQ_MAX];
		CMemRdWrReqTime m_wrReqList[MEM_REQ_MAX];

		uint32_t m_rdReqValid;
		uint32_t m_wrReqValid;
		uint32_t m_rdReqRr;
		uint32_t m_wrReqRr;
		uint32_t m_rdReqCnt;
		uint32_t m_wrReqCnt;
		bool m_bRdReqHold;
		bool m_bWrReqHold;
		uint32_t m_prevReqIdx;
		int m_rdReqQwIdx;
		int m_wrReqQwIdx;

		int m_reqRndFullCnt;
		bool m_bReqRndFull;

		bool m_bMemDump;
		string m_memName;

		void CheckMemReqOrdering(CMemRdWrReqIntf & newReq, CMemRdWrReqIntf & queReq);
	};

} // namespace Ht
