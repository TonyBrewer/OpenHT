/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#define HT_LIB_SYSC
#include "Ht.h"
#include "SyscMemLib.h"
#include "SyscMonLib.h"

#ifndef COPROC_ADDR_MANGLE
#define COPROC_ADDR_MANGLE 0ULL
#endif

#define DEBUG_SYSC_MEM_

extern uint32_t __CnyMTRand();

void CSyscMem::SyscMem()
{
	if (m_pLib == 0)
		m_pLib = new Ht::CSyscMemLib(this);

	m_pLib->SyscMemLib();
}

namespace Ht {
	CMemTrace g_memTrace;

	extern int avgMemLatency[2];
	deque<CBankReq *> g_bankQue[BANK_CNT+1];	// last que is for host
	extern bool bMemBankModel;
	extern bool bMemTraceEnable;
	extern EHtCoproc htCoproc;
	extern bool g_bMemReqRndFull;

	CBankReq * CBankReq::m_pBlock;
	int CBankReq::m_blockCnt = 0;
	CBankReq * CBankReq::m_pFree = 0;
	int g_seqNum = 0;
	list<CBankReq *> * g_pSyscMemQue[MAX_SYSC_MEM];
	int g_syscMemCnt = 0;
	int Ht::CSyscMemLib::g_memPlacementErrorCnt = 0;

	void CSyscMemLib::SyscMemLib()
	{
		if (bMemTraceEnable) {
			static bool bInit = false;
			if (!bInit) {
				g_memTrace.Open(CMemTrace::Write, "MemTrace.htmt");
				bInit = true;
			}
		}

		sc_time currTime = sc_time_stamp();

		m_pSyscMem->o_xbarToMif_rdRspRdy = 0;
		m_pSyscMem->o_xbarToMif_wrRspRdy = 0;

		bool is_hc1 = htCoproc == hc1 || htCoproc == hc1ex;
		bool is_hc2 = htCoproc == hc2 || htCoproc == hc2ex;
		//bool is_wx  = htCoproc == wx690 || htCoproc == wx2k;

		if (bMemBankModel) {
			// model individual banks of memory
			if (m_pSyscMem->i_mifToXbar_reqRdy.read()) {

				// insert request in delay fifo (mimics flow through delay of memory controller and crossbar)
				if (m_pNewReq == 0) {
					m_pNewReq = new CBankReq;

					m_pNewReq->m_memReq = m_pSyscMem->i_mifToXbar_req.read();
					m_pNewReq->m_startTime = sc_time_stamp();

					bool bCoprocAddress = false;
					bool bValidAddress = g_memTrace.IsValidAddress(m_pNewReq->m_memReq.m_addr, bCoprocAddress);

					if (g_memTrace.IsAddressValidationEnabled() && !bValidAddress) {
						fprintf(stderr, "FATAL: Coprocessor memory request to invalid virtual address\n");
						fprintf(stderr, "   Virtual address: 0x%012llx\n", (long long)m_pNewReq->m_memReq.m_addr);
						fprintf(stderr, "   Request specifier: %s\n", m_pNewReq->m_memReq.m_host ? "host" : "coproc");
						fprintf(stderr, "   Requesting Module and Line: %s %d\n", m_pNewReq->m_memReq.m_file, m_pNewReq->m_memReq.m_line);
						fprintf(stderr, "   Simulation cycle: %lld\n", HT_CYCLE());
						if (g_vcdp) sc_close_vcd_trace_file(g_vcdp);

#						ifdef _WIN32
						assert(0);
#						else
						exit(1);
#						endif
					}

					m_pNewReq->m_host = !bValidAddress || !bCoprocAddress;
					m_pNewReq->m_bank = m_pNewReq->m_host ? BANK_CNT : ((m_pNewReq->m_memReq.m_addr >> 3) & BANK_MASK);

					if (is_hc1)
						m_pNewReq->m_reqQwCnt = 1;
					else
						m_pNewReq->m_reqQwCnt = (m_pNewReq->m_memReq.m_tid & 7) + 1;

					if (m_pNewReq->m_memReq.m_type == MEM_REQ_WR)
						m_pNewReq->m_data[m_wrDataIdx++] = m_pSyscMem->i_mifToXbar_req.read().m_data;

					if ((m_pNewReq->m_memReq.m_tid & 7) > 0 && m_pNewReq->m_host != m_pNewReq->m_memReq.m_host && g_memPlacementErrorCnt++ < 10) {
						fprintf(stderr, "WARNING: Multi-quadward memory read request host/coproc specifier does not match memory placement\n");
						fprintf(stderr, "   Virtual address: 0x%012llx\n", (long long)m_pNewReq->m_memReq.m_addr);
						fprintf(stderr, "   Request specifier: %s\n", m_pNewReq->m_memReq.m_host ? "host" : "coproc");
						fprintf(stderr, "   Memory placement: %s\n", m_pNewReq->m_host ? "host" : "coproc");
						fprintf(stderr, "   Requesting Module and Line: %s %d\n", m_pNewReq->m_memReq.m_file, m_pNewReq->m_memReq.m_line);
					}

					if (is_hc2) {
						if (m_pNewReq->m_reqQwCnt > 1 && (m_pNewReq->m_reqQwCnt != 8 || !m_pNewReq->m_host)) {
							fprintf(stderr, "ERROR: Multi-quadword memory request inappropriate for platform type\n");
							fprintf(stderr, "   Virtual address: 0x%012llx\n", (long long)m_pNewReq->m_memReq.m_addr);
							fprintf(stderr, "   Request quadword count: %d (must be 8)\n", m_pNewReq->m_reqQwCnt);
							fprintf(stderr, "   Request memory placement: %s (must be host)\n", m_pNewReq->m_host ? "host" : "coproc");
							fprintf(stderr, "   Requesting Module and Line: %s %d\n", m_pNewReq->m_memReq.m_file, m_pNewReq->m_memReq.m_line);
							fprintf(stderr, "   Simulation cycle: %lld\n", HT_CYCLE());
							if (g_vcdp) sc_close_vcd_trace_file(g_vcdp);

#							ifdef _WIN32
							assert(0);
#							else
							exit(1);
#							endif
						}
					}

					if (m_pNewReq->m_memReq.m_type == MEM_REQ_RD && (m_pNewReq->m_memReq.m_tid & 7) != 0) {
						// check that multi-quadword reads have reqCnt == 8 && addr aligned to 64B
						assert((m_pNewReq->m_memReq.m_tid & 7) == 7);
						assert((m_pNewReq->m_memReq.m_addr & 0x3f) == 0);
					}

				} else
					m_pNewReq->m_data[m_wrDataIdx++] = m_pSyscMem->i_mifToXbar_req.read().m_data;

				if (m_bMemDump) {
					CMemRdWrReqIntf req = m_pSyscMem->i_mifToXbar_req.read();
					string dbg = m_memName;
					dbg += HtStrFmt("%s: seqNum=%d tid=%x addr=%012llx",
							req.m_type == MEM_REQ_RD ? "RdReq" :
							req.m_type == MEM_REQ_WR ? "WrReq" : "UnKwn",
							(int)m_pNewReq->m_seqNum,
							(int)req.m_tid, (long long)req.m_addr);
					if (req.m_type != MEM_REQ_RD)
						dbg += HtStrFmt(" data=%llx", (long long)req.m_data);
					dbg += HtStrFmt(" host=%d @ %lld\n", (int)req.m_host, HT_CYCLE());
					printf("%s", dbg.c_str()); fflush(stdout);
				}

				if (m_pNewReq->m_memReq.m_type != MEM_REQ_WR || m_wrDataIdx == m_pNewReq->m_reqQwCnt) {

					deque<CBankReq *>::iterator reqIter;
					if (m_pNewReq->m_host) {
						for (reqIter = m_hostReqQue.begin(); reqIter != m_hostReqQue.end(); reqIter++)
							CheckMemReqOrdering(m_pNewReq->m_memReq, (*reqIter)->m_memReq);

						m_hostReqQue.push_back(m_pNewReq);
					} else {
						deque<CBankReq *>::iterator reqIter;
						for (reqIter = m_bankReqQue.begin(); reqIter != m_bankReqQue.end(); reqIter++)
							CheckMemReqOrdering(m_pNewReq->m_memReq, (*reqIter)->m_memReq);

						m_bankReqQue.push_back(m_pNewReq);
					}

					// now check bank queues for ordering problems
					int bank = m_pNewReq->m_bank;
					for (reqIter = g_bankQue[bank].begin(); reqIter != g_bankQue[bank].end(); reqIter++)
						CheckMemReqOrdering(m_pNewReq->m_memReq, (*reqIter)->m_memReq);

					m_pNewReq = 0;
					m_wrDataIdx = 0;
				}
			}

			CBankReq * pReq = 0;
			sc_time bankBusyTime;
			if (!m_hostReqQue.empty() && m_hostReqQue.front()->m_startTime + sc_time(1000 * 100/66, SC_NS) <= currTime) {
				pReq = m_hostReqQue.front();
				m_hostReqQue.pop_front();

				// 700 MB/s for quadword reads => 8B/11.4ns
				// 2.5 GB/s for 64B reads => 64B/25.6ns
				// 10ns simtime == 6.6ns real time
				if ((pReq->m_memReq.m_tid & 7) > 0)
					bankBusyTime = sc_time(25.6 * 100/66, SC_NS);
				else
					bankBusyTime = sc_time(11.4 * 100/66, SC_NS);

			} else if (!m_bankReqQue.empty() && m_bankReqQue.front()->m_startTime + sc_time(400 * 100/66, SC_NS) <= currTime) {
				pReq = m_bankReqQue.front();
				m_bankReqQue.pop_front();

				// 100ns bank busy time, 10ns simtime == 6.6ns real time
				if ((pReq->m_memReq.m_tid & 7) > 0)
					bankBusyTime = sc_time(800 * 100/66, SC_NS);
				else
					bankBusyTime = sc_time(100 * 100/66, SC_NS);
			}

			if (pReq) {
				// schedule request on bank (or host interface)
				int bank = pReq->m_bank;

				sc_time minBusyTime = currTime + bankBusyTime;
				if (g_bankQue[bank].empty()) {
					// host is idle, start operation immediately
					pReq->m_bankAvlTime = minBusyTime;
				} else {
					sc_time minAvlTime = g_bankQue[bank].back()->m_bankAvlTime + bankBusyTime;
					pReq->m_bankAvlTime = max(minAvlTime, minBusyTime);
				}

				g_bankQue[bank].push_back(pReq);

				// walk through list of requests for this port and insert request to maintain order of avail bank times
				list<CBankReq *>::iterator i;
				for (i = m_bankBusyQue.begin(); i != m_bankBusyQue.end(); i++)
					if ((*i)->m_bankAvlTime < pReq->m_bankAvlTime 
						|| ((*i)->m_bankAvlTime == pReq->m_bankAvlTime && (*i)->m_bank < pReq->m_bank))
						break;
				m_bankBusyQue.insert(i, pReq);

				int ae = 0;
				int unit = 0;
				int op = (int)pReq->m_memReq.m_type;
				int host = pReq->m_host;
				char const * file = pReq->m_memReq.m_file;
				int line = pReq->m_memReq.m_line;
				uint64_t startTime = pReq->m_startTime.value() * 66/100;
				uint64_t compTime = pReq->m_bankAvlTime.value() * 66/100;
				uint64_t addr = pReq->m_memReq.m_addr;

				g_memTrace.WriteReq(ae, unit, op, host, file, line, startTime, compTime, addr);
			}

			if (!m_bankBusyQue.empty()) {
				CBankReq * pBankAvl = m_bankBusyQue.back();
				int bank = pBankAvl->m_host ? BANK_CNT : ((pBankAvl->m_memReq.m_addr >> 3) & BANK_MASK);
				CBankReq * pBankReq = g_bankQue[bank].front();

#				ifdef ENABLE_HANG_CHECK
				if (currTime > pBankReq->m_bankAvlTime + sc_time(1000, SC_NS)) {

					// dump all queues to a file
					FILE *fp = fopen("MemDump.txt", "w");

					fprintf(fp, "Hang detected:\n");
					fprintf(fp, "  MC %d: seqNum=%d, start=%lld, comp=%lld\n", m_syscMemId, pBankAvl->m_seqNum, pBankAvl->m_startTime, pBankAvl->m_bankAvlTime);
					fprintf(fp, "  Bank %d: seqNum=%d, start=%lld, comp=%lld\n", bank, pBankReq->m_seqNum, pBankReq->m_startTime, pBankReq->m_bankAvlTime);

					for (int i = 0; i < BANK_CNT+1; i += 1) {
						if (!g_bankQue[i].empty())
							fprintf(fp, "BankQue[%d]\n", i);

						while (!g_bankQue[i].empty()) {
							pBankReq = g_bankQue[bank].front();

							fprintf(fp, "  seqNum=%d, start=%lld, comp=%lld\n", pBankReq->m_seqNum, pBankReq->m_startTime, pBankReq->m_bankAvlTime);

							g_bankQue[bank].pop();
						}
					}

					fprintf(fp, "\n");

					// dump mc queues
					for (int i = 0; i < g_syscMemCnt; i += 1) {
						if (!g_pSyscMemQue[i]->empty())
							fprintf(fp, "MC Que[%d]\n", i);

						while (!g_pSyscMemQue[i]->empty()) {
							pBankReq = g_pSyscMemQue[i]->back();

							fprintf(fp, "  seqNum=%d, start=%lld, comp=%lld\n", pBankReq->m_seqNum, pBankReq->m_startTime, pBankReq->m_bankAvlTime);

							g_pSyscMemQue[i]->pop_back();
						}
					}

					fclose(fp);

#					ifdef _WIN32
					assert(0);
#					else
					exit(1);
#					endif
				}
#				endif

				if (pBankReq == pBankAvl && pBankReq->m_bankAvlTime <= sc_time_stamp()) {
					// operation completed advance queue

					CMemRdWrReqIntf & memReq = pBankReq->m_memReq;

					bool bCoprocAddress = false;
					bool bValidAddress = g_memTrace.IsValidAddress(memReq.m_addr, bCoprocAddress);
					bool bHost = !bValidAddress || !bCoprocAddress;

					if (g_memTrace.IsAddressValidationEnabled() && !bValidAddress) {
						fprintf(stderr, "FATAL: Coprocessor memory request to invalid virtual address\n");
						fprintf(stderr, "   Virtual address: 0x%012llx\n", (long long)memReq.m_addr);
						fprintf(stderr, "   Request specifier: %s\n", memReq.m_host ? "host" : "coproc");
						fprintf(stderr, "   Requesting Module and Line: %s %d\n", memReq.m_file, memReq.m_line);
						fprintf(stderr, "   Simulation cycle: %lld\n", HT_CYCLE());
						if (g_vcdp) sc_close_vcd_trace_file(g_vcdp);

#						ifdef _WIN32
						assert(0);
#						else
						exit(1);
#						endif
					}

					if (memReq.m_type == MEM_REQ_WR) {

						if (!m_pSyscMem->i_mifToXbar_wrRspFull.read()) {

							switch (memReq.m_size) {
							case MEM_REQ_U8:
								*(uint8_t*)((memReq.m_addr ^ COPROC_ADDR_MANGLE) + (m_wrReqQwIdx << 3)) = (uint8_t)pBankReq->m_data[m_wrReqQwIdx];
								break;
							case MEM_REQ_U16:
								*(uint16_t*)((memReq.m_addr ^ COPROC_ADDR_MANGLE) + (m_wrReqQwIdx << 3)) = (uint16_t)pBankReq->m_data[m_wrReqQwIdx];
								break;
							case MEM_REQ_U32:
								*(uint32_t*)((memReq.m_addr ^ COPROC_ADDR_MANGLE) + (m_wrReqQwIdx << 3)) = (uint32_t)pBankReq->m_data[m_wrReqQwIdx];
								break;
							case MEM_REQ_U64:
								*(uint64_t*)((memReq.m_addr ^ COPROC_ADDR_MANGLE) + (m_wrReqQwIdx << 3)) = pBankReq->m_data[m_wrReqQwIdx];
								break;
							default:
								assert(0);
							}

							if (m_wrReqQwIdx == 0) {
								m_pSyscMem->o_xbarToMif_wrRspRdy = 1;
								m_pSyscMem->o_xbarToMif_wrRspTid = (memReq.m_tid & ~7) | (m_wrReqQwIdx & 7);

								if (m_bMemDump) {
									string dbg = m_memName;
									dbg += HtStrFmt("WrRsp: seqNum=%d tid=%x host=%d @ %lld\n",
										(int)pBankReq->m_seqNum, (int)memReq.m_tid, (int)bHost, HT_CYCLE());
									printf("%s", dbg.c_str()); fflush(stdout);
								}
							}

							long long reqClocks = (long long)((sc_time_stamp().value() - pBankReq->m_startTime.value()) / 10000);
							Ht::g_syscMon.UpdateMemReqClocks(memReq.m_host == 1, reqClocks);

							Ht::g_syscMon.UpdateTotalMemWriteBytes(memReq.m_host == 1, 1 << memReq.m_size);
							if (memReq.m_tid & 7)
								Ht::g_syscMon.UpdateTotalMemWrite64s(memReq.m_host == 1, m_wrReqQwIdx == 0 ? 1 : 0);
							else
								Ht::g_syscMon.UpdateTotalMemWrites(memReq.m_host == 1, 1);

							m_wrReqQwIdx += 1;

							if (m_wrReqQwIdx == pBankReq->m_reqQwCnt) {
								g_bankQue[bank].pop_front();
								m_bankBusyQue.pop_back();
								delete pBankReq;
								m_wrReqQwIdx = 0;
							}
						}

					} else {
						// handle read, read64B or an atomic operator

						if (!m_pSyscMem->i_mifToXbar_rdRspFull.read()) {

							uint64_t data = 0;
							switch (memReq.m_size) {
							case MEM_REQ_U8:
								data = *(uint8_t*)((memReq.m_addr + (m_rdReqQwIdx << 3)) ^ COPROC_ADDR_MANGLE);
								break;
							case MEM_REQ_U16:
								data = *(uint16_t*)((memReq.m_addr + (m_rdReqQwIdx << 3)) ^ COPROC_ADDR_MANGLE);
								break;
							case MEM_REQ_U32:
								data = *(uint32_t*)((memReq.m_addr + (m_rdReqQwIdx << 3)) ^ COPROC_ADDR_MANGLE);
								break;
							case MEM_REQ_U64:
								data = *(uint64_t*)((memReq.m_addr + (m_rdReqQwIdx << 3)) ^ COPROC_ADDR_MANGLE);
								break;
							}

							switch (memReq.m_type) {
							case MEM_REQ_RD:
								break;
							case MEM_REQ_SET_BIT_63:
								*(uint64_t*)((memReq.m_addr ^ COPROC_ADDR_MANGLE) + (m_rdReqQwIdx << 3)) = 0x8000000000000000ULL;
								break;
							default:
								assert(0);  // undefined memory type
							}

							m_pSyscMem->o_xbarToMif_rdRspRdy = 1;

							CMemRdRspIntf rdRsp;
							rdRsp.m_tid = (memReq.m_tid & ~7) | (m_rdReqQwIdx & 7);
							rdRsp.m_data = data;

							m_pSyscMem->o_xbarToMif_rdRsp = rdRsp;

							if (m_bMemDump) {
								string dbg = m_memName;
								dbg += HtStrFmt("RdRsp: seqNum=%d tid=%x data=%llx host=%d @ %lld\n",
										(int)pBankReq->m_seqNum, (int)rdRsp.m_tid, (long long)rdRsp.m_data, (int)bHost, HT_CYCLE());
								printf("%s", dbg.c_str()); fflush(stdout);
							}

							long long reqClocks = (long long)((sc_time_stamp().value() - pBankReq->m_startTime.value()) / 10000);
							Ht::g_syscMon.UpdateMemReqClocks(memReq.m_host == 1, reqClocks);

							Ht::g_syscMon.UpdateTotalMemReadBytes(memReq.m_host == 1, 8);
							if (memReq.m_tid & 7)
								Ht::g_syscMon.UpdateTotalMemRead64s(memReq.m_host == 1, m_rdReqQwIdx == 0 ? 1 : 0);
							else
								Ht::g_syscMon.UpdateTotalMemReads(memReq.m_host == 1, 1);

							m_rdReqQwIdx += 1;

							if (m_rdReqQwIdx == pBankReq->m_reqQwCnt) {
								g_bankQue[bank].pop_front();
								m_bankBusyQue.pop_back();
								delete pBankReq;
								m_rdReqQwIdx = 0;
							}
						}

					}
				}
			}

		} else {

			if (m_pSyscMem->i_mifToXbar_reqRdy.read()) {

				if (m_bMemDump) {
					string dbg = m_memName;
					CMemRdWrReqIntf req = m_pSyscMem->i_mifToXbar_req.read();
					dbg += HtStrFmt("%s: tid=%x addr=%012llx",
							req.m_type == MEM_REQ_RD ? "RdReq" :
							req.m_type == MEM_REQ_WR ? "WrReq" : "UnKwn",
							(int)req.m_tid, (long long)req.m_addr);
					if (req.m_type != MEM_REQ_RD)
						dbg += HtStrFmt(" data=%llx", (long long)req.m_data);
					dbg += HtStrFmt(" host=%d @ %lld\n", (int)req.m_host, HT_CYCLE());
					printf("%s", dbg.c_str()); fflush(stdout);
				}

				if (m_pReqTime == 0) {
					m_pReqTime = new CMemRdWrReqTime;
					m_pReqTime->m_req = m_pSyscMem->i_mifToXbar_req.read();
					m_pReqTime->m_time = sc_time_stamp();

					bool bCoprocAddress = false;
					bool bValidAddress = g_memTrace.IsValidAddress(m_pReqTime->m_req.m_addr, bCoprocAddress);
					m_pReqTime->m_host = !bValidAddress || !bCoprocAddress;

					if (g_memTrace.IsAddressValidationEnabled() && !bValidAddress) {
						fprintf(stderr, "FATAL: Coprocessor memory request to invalid virtual address\n");
						fprintf(stderr, "   Virtual address: 0x%012llx\n", (long long)m_pReqTime->m_req.m_addr);
						fprintf(stderr, "   Request specifier: %s\n", m_pReqTime->m_req.m_host ? "host" : "coproc");
						fprintf(stderr, "   Requesting Module and Line: %s %d\n", m_pReqTime->m_req.m_file, m_pReqTime->m_req.m_line);
						fprintf(stderr, "   Simulation cycle: %lld\n", HT_CYCLE());
						if (g_vcdp) sc_close_vcd_trace_file(g_vcdp);

#						ifdef _WIN32
						assert(0);
#						else
						exit(1);
#						endif
					}

					if (is_hc1)
						m_pReqTime->m_reqQwCnt = 1;
					else
						m_pReqTime->m_reqQwCnt = (m_pReqTime->m_req.m_tid & 7) + 1;

					if (m_pReqTime->m_req.m_type == MEM_REQ_WR)
						m_pReqTime->m_data[m_wrDataIdx++] = m_pSyscMem->i_mifToXbar_req.read().m_data;

					if (m_pReqTime->m_reqQwCnt > 1 && m_pReqTime->m_host != m_pReqTime->m_req.m_host && g_memPlacementErrorCnt++ < 10) {
						fprintf(stderr, "WARNING: Multi-quadward memory request host/coproc specifier does not match memory placement\n");
						fprintf(stderr, "   Virtual address: 0x%012llx\n", (long long)m_pReqTime->m_req.m_addr);
						fprintf(stderr, "   Request specifier: %s\n", m_pReqTime->m_req.m_host ? "host" : "coproc");
						fprintf(stderr, "   Memory placement: %s\n", m_pReqTime->m_host ? "host" : "coproc");
						fprintf(stderr, "   Requesting Module and Line: %s %d\n", m_pReqTime->m_req.m_file, m_pReqTime->m_req.m_line);
					}

					if (is_hc2) {
						if (m_pReqTime->m_reqQwCnt > 1 && (m_pReqTime->m_reqQwCnt != 8 || !m_pReqTime->m_host)) {
							fprintf(stderr, "ERROR: Multi-quadword memory request inappropriate for platform type\n");
							fprintf(stderr, "   Virtual address: 0x%012llx\n", (long long)m_pReqTime->m_req.m_addr);
							fprintf(stderr, "   Request quadword count: %d (must be 8)\n", m_pReqTime->m_reqQwCnt);
							fprintf(stderr, "   Request memory placement: %s (must be host)\n", m_pReqTime->m_host ? "host" : "coproc");
							fprintf(stderr, "   Requesting Module and Line: %s %d\n", m_pReqTime->m_req.m_file, m_pReqTime->m_req.m_line);
							fprintf(stderr, "   Simulation cycle: %lld\n", HT_CYCLE());
							if (g_vcdp) sc_close_vcd_trace_file(g_vcdp);

#							ifdef _WIN32
							assert(0);
#							else
							exit(1);
#							endif
						}
					}

					if (m_pReqTime->m_req.m_type == MEM_REQ_RD && (m_pReqTime->m_req.m_tid & 7) != 0) {
						// check that multi-quadword reads have reqCnt == 8 && addr aligned to 64B
						assert((m_pReqTime->m_req.m_tid & 7) == 7);
						assert((m_pReqTime->m_req.m_addr & 0x3f) == 0);
					}
				} else 
					m_pReqTime->m_data[m_wrDataIdx++] = m_pSyscMem->i_mifToXbar_req.read().m_data;

				if (m_pReqTime->m_req.m_type != MEM_REQ_WR || m_wrDataIdx == m_pReqTime->m_reqQwCnt) {
					deque<CMemRdWrReqTime>::iterator reqIter;
					for (reqIter = m_memReqQue.begin(); reqIter != m_memReqQue.end(); reqIter++)
						CheckMemReqOrdering(m_pReqTime->m_req, (*reqIter).m_req);

					// check read / write lists
					if (m_pReqTime->m_req.m_type == MEM_REQ_WR) {
						for (int i = 0; i < MEM_REQ_MAX; i += 1) {
							if (((m_rdReqValid >> i) & 1) != 0)
								CheckMemReqOrdering(m_pReqTime->m_req, m_rdReqList[i].m_req);
						}
					}

					if (m_pReqTime->m_req.m_type != MEM_REQ_WR) {
						for (int i = 0; i < MEM_REQ_MAX; i += 1) {
							if (((m_wrReqValid >> i) & 1) != 0)
								CheckMemReqOrdering(m_pReqTime->m_req, m_wrReqList[i].m_req);
						}
					}

					m_memReqQue.push_back(*m_pReqTime);

					delete m_pReqTime;
					m_pReqTime = 0;
					m_wrDataIdx = 0;
				}
			}

			bool insert = false;
			if (!m_memReqQue.empty()) {
				/* fudged te get desired avg. latency */
				int lat = max(0, m_memReqQue.front().m_req.m_host ?
					avgMemLatency[1] - 32 : avgMemLatency[0] - 63);

				if (m_memReqQue.front().m_time + sc_time(lat * 10, SC_NS) < sc_time_stamp())
					insert = true;
			}
			if (insert) {
				// find an empty slot
				if (m_memReqQue.front().m_req.m_type == MEM_REQ_WR) {
					for (int i = 0; i < MEM_REQ_MAX; i += 1, m_wrReqRr = (m_wrReqRr + 1) % MEM_REQ_MAX) {
						if (((m_wrReqValid >> m_wrReqRr) & 1) == 0) {
							m_wrReqList[m_wrReqRr] = m_memReqQue.front();
							m_wrReqValid |= 1 << m_wrReqRr;
							m_wrReqCnt += 1;
							m_memReqQue.pop_front();
							break;
						}
					}
				} else {
					for (int i = 0; i < MEM_REQ_MAX; i += 1, m_rdReqRr = (m_rdReqRr + 1) % MEM_REQ_MAX) {
						if (((m_rdReqValid >> m_rdReqRr) & 1) == 0) {
							m_rdReqList[m_rdReqRr] = m_memReqQue.front();
							m_rdReqValid |= 1 << m_rdReqRr;
							m_rdReqCnt += 1;
							m_memReqQue.pop_front();
							break;
						}
					}
				}
			}

			// look for work to be done
			int idx = (m_bRdReqHold || m_bWrReqHold) ? m_prevReqIdx : (__CnyMTRand() & (MEM_REQ_MAX - 1));
			m_prevReqIdx = idx;

			if (!m_pSyscMem->i_mifToXbar_rdRspFull.read() && m_rdReqCnt != 0 && m_wrReqCnt != MEM_REQ_MAX && ((m_rdReqValid >> idx) & 1)) {
				CMemRdWrReqIntf memReq = m_rdReqList[idx].m_req;

				bool bCoprocAddress = false;
				bool bValidAddress = g_memTrace.IsValidAddress(memReq.m_addr, bCoprocAddress);
				bool bHost = !bValidAddress || !bCoprocAddress;

				uint64_t data = 0;
				switch (memReq.m_size) {
				case MEM_REQ_U8:
					data = *(uint8_t*)((memReq.m_addr + (m_rdReqQwIdx << 3)) ^ COPROC_ADDR_MANGLE);
					break;
				case MEM_REQ_U16:
					data = *(uint16_t*)((memReq.m_addr + (m_rdReqQwIdx << 3)) ^ COPROC_ADDR_MANGLE);
					break;
				case MEM_REQ_U32:
					data = *(uint32_t*)((memReq.m_addr + (m_rdReqQwIdx << 3)) ^ COPROC_ADDR_MANGLE);
					break;
				case MEM_REQ_U64:
					data = *(uint64_t*)((memReq.m_addr + (m_rdReqQwIdx << 3)) ^ COPROC_ADDR_MANGLE);
					break;
				}

				switch (memReq.m_type) {
				case MEM_REQ_RD:
					break;
				case MEM_REQ_SET_BIT_63:
					*(uint64_t*)(memReq.m_addr ^ COPROC_ADDR_MANGLE) = 0x8000000000000000ULL;
					break;
				default:
					assert(0);  // undefined memory type
				}

				CMemRdRspIntf rdRsp;
				rdRsp.m_tid = (memReq.m_tid & ~7) | (m_rdReqQwIdx & 7);
				rdRsp.m_data = data;

				if (m_bMemDump) {
					string dbg = m_memName;
					dbg += HtStrFmt("RdRsp: tid=%x data=%llx host=%d @ %lld\n",
							(int)rdRsp.m_tid, (long long)rdRsp.m_data, bHost, HT_CYCLE());
					printf("%s", dbg.c_str()); fflush(stdout);
				}

				m_pSyscMem->o_xbarToMif_rdRspRdy = 1;
				m_pSyscMem->o_xbarToMif_rdRsp = rdRsp;

				long long reqClocks = (long long)((sc_time_stamp().value() - m_rdReqList[idx].m_time.value()) / 10000);
				Ht::g_syscMon.UpdateMemReqClocks(bHost, reqClocks);

				Ht::g_syscMon.UpdateTotalMemReadBytes(bHost, 8);
				if (memReq.m_tid & 7)
					Ht::g_syscMon.UpdateTotalMemRead64s(bHost, m_rdReqQwIdx == 0 ? 1 : 0);
				else
					Ht::g_syscMon.UpdateTotalMemReads(bHost, 1);

				m_rdReqQwIdx += 1;

				if (m_rdReqList[idx].m_reqQwCnt ==  m_rdReqQwIdx) {
					m_rdReqCnt -= 1;
					m_rdReqValid &= ~(1 << idx);
					m_rdReqQwIdx = 0;
					m_bRdReqHold = false;
				} else
					m_bRdReqHold = true;
			}

			if (!m_pSyscMem->i_mifToXbar_wrRspFull.read() && m_wrReqCnt != 0 && ((m_wrReqValid >> idx) & 1)) {
				CMemRdWrReqIntf memReq = m_wrReqList[idx].m_req;

				bool bCoprocAddress = false;
				bool bValidAddress = g_memTrace.IsValidAddress(memReq.m_addr, bCoprocAddress);
				bool bHost = !bValidAddress || !bCoprocAddress;

				switch (memReq.m_size) {
				case MEM_REQ_U8:
					*(uint8_t*)((memReq.m_addr ^ COPROC_ADDR_MANGLE) + (m_wrReqQwIdx << 3)) = (uint8_t)m_wrReqList[idx].m_data[m_wrReqQwIdx];
					break;
				case MEM_REQ_U16:
					*(uint16_t*)((memReq.m_addr ^ COPROC_ADDR_MANGLE) + (m_wrReqQwIdx << 3)) = (uint16_t)m_wrReqList[idx].m_data[m_wrReqQwIdx];
					break;
				case MEM_REQ_U32:
					*(uint32_t*)((memReq.m_addr ^ COPROC_ADDR_MANGLE) + (m_wrReqQwIdx << 3)) = (uint32_t)m_wrReqList[idx].m_data[m_wrReqQwIdx];
					break;
				case MEM_REQ_U64:
					*(uint64_t*)((memReq.m_addr ^ COPROC_ADDR_MANGLE) + (m_wrReqQwIdx << 3)) = m_wrReqList[idx].m_data[m_wrReqQwIdx];
					break;
				default:
					assert(0);
				}

				if (m_wrReqQwIdx == 0) {
					if (m_bMemDump) {
						string dbg = m_memName;
						dbg += HtStrFmt("WrRsp: tid=%x host=%d @ %lld\n",
								(int)memReq.m_tid, bHost, HT_CYCLE());
						printf("%s", dbg.c_str()); fflush(stdout);
					}

					m_pSyscMem->o_xbarToMif_wrRspRdy = 1;
					m_pSyscMem->o_xbarToMif_wrRspTid = (memReq.m_tid & ~7) | (m_wrReqQwIdx & 7);
				}

				long long reqClocks = (long long)((sc_time_stamp().value() - m_wrReqList[idx].m_time.value()) / 10000);
				Ht::g_syscMon.UpdateMemReqClocks(bHost, reqClocks);

				Ht::g_syscMon.UpdateTotalMemWriteBytes(bHost, 1 << memReq.m_size);
				if (memReq.m_tid & 7)
					Ht::g_syscMon.UpdateTotalMemWrite64s(bHost, m_wrReqQwIdx == 0 ? 1 : 0);
				else
					Ht::g_syscMon.UpdateTotalMemWrites(bHost, 1);

				m_wrReqQwIdx += 1;

				if (m_wrReqList[idx].m_reqQwCnt == m_wrReqQwIdx) {
					m_wrReqCnt -= 1;
					m_wrReqValid &= ~(1 << idx);
					m_wrReqQwIdx = 0;
					m_bWrReqHold = false;
				} else
					m_bWrReqHold = true;
			}

			if (g_bMemReqRndFull) {
				if (m_reqRndFullCnt == 0) {
					m_reqRndFullCnt = 2 + (__CnyMTRand() % 400);
					m_bReqRndFull = !m_bReqRndFull;
				} else
					m_reqRndFullCnt -= 1;

				m_pSyscMem->o_xbarToMif_reqFull = m_bReqRndFull;
			} else
				m_pSyscMem->o_xbarToMif_reqFull = g_bMemReqRndFull && ((__CnyMTRand() & 1) == 1);

			///////////////////////
			// Registers
			//

			if (m_pSyscMem->i_reset.read()) {
				m_rdReqValid = 0;
				m_wrReqValid = 0;
				m_rdReqRr = 0;
				m_wrReqRr = 0;
				m_rdReqCnt = 0;
				m_wrReqCnt = 0;
				m_bRdReqHold = false;
				m_bWrReqHold = false;
				m_prevReqIdx = 0;
				m_rdReqQwIdx = 0;
				m_wrReqQwIdx = 0;
			}
		}
	}

	void CSyscMemLib::CheckMemReqOrdering(CMemRdWrReqIntf & newReq, CMemRdWrReqIntf & queReq)
	{
		// check for ordering problems
		//   New read with queued write to same address
		//   New write with queued read to same address
		//   New atomic with queued read or write to same address
		//   New read or write with queued atomic to same address

		static int errorCnt = 0;
		const int maxErrorCnt = 5;
		if (!newReq.m_orderChk || !queReq.m_orderChk || errorCnt > maxErrorCnt)
			return;

		uint32_t newSize = 0;
		bool bNewRead = false;
		bool bNewWrite = false;
		bool bNewAtomic = false;
		char const * pNewReqStr = "";
		switch (newReq.m_type) {
		case MEM_REQ_RD:
			bNewRead = true;
			newSize = ((newReq.m_tid & 7)+1) * 8;
			pNewReqStr = "Read";
			break;
		case MEM_REQ_WR:
			bNewWrite = true;
			newSize = ((newReq.m_tid & 7)+1) * 8;
			pNewReqStr = "Write";
			break;
		case MEM_REQ_SET_BIT_63:
			bNewAtomic = true;
			newSize = 8;
			pNewReqStr = "Atomic";
			break;
		default:
			assert(0);  // undefined memory type
		}

		uint32_t queSize = 0;
		bool bQueRead = false;
		bool bQueWrite = false;
		bool bQueAtomic = false;
		char const * pQueReqStr = "";
		switch (queReq.m_type) {
		case MEM_REQ_RD:
			bQueRead = true;
			queSize = ((queReq.m_tid & 7)+1) * 8;
			pQueReqStr = "Read";
			break;
		case MEM_REQ_WR:
			bQueWrite = true;
			queSize = ((queReq.m_tid & 7)+1) * 8;
			pQueReqStr = "Write";
			break;
		case MEM_REQ_SET_BIT_63:
			bQueAtomic = true;
			queSize = 8;
			pQueReqStr = "Atomic";
			break;
		default:
			assert(0);  // undefined memory type
		}

		// first check if referenced ranges overlap
		uint64_t newStart = newReq.m_addr;
		uint64_t newEnd = newStart + newSize;

		uint64_t queStart = queReq.m_addr;
		uint64_t queEnd = queStart + queSize;

		if (newStart >= queEnd || newEnd <= queStart)
			return; // no overlap

		// check if operations cause ordering problems
		if ((bNewRead && bQueWrite) ||
			(bNewWrite && bQueRead) ||
			(bNewAtomic && (bQueRead || bQueWrite)) ||
			((bNewRead || bNewWrite) && bQueAtomic))
		{
			// problem detected
			printf("Error: memory request ordering problem detected\n");
			printf("   New %s request from file '%s', line %d, time %lld\n",
				pNewReqStr, newReq.m_file, newReq.m_line, (long long)newReq.m_time/10000);
			printf("       address 0x%012llx, size %dB\n", (long long)newReq.m_addr, newSize);
			printf("   Queued %s request from file '%s', line %d, time %lld\n",
				pQueReqStr, queReq.m_file, queReq.m_line, (long long)queReq.m_time/10000);
			printf("       address 0x%012llx, size %dB\n", (long long)queReq.m_addr, queSize);
			printf("\n");
			fflush(stdout);

			if (++errorCnt == maxErrorCnt)
				printf("No further memory request ordering problems will be reported\n");
		}
	}

} // namespace Ht
