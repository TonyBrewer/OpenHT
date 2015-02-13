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

#include <vector>
#include <algorithm>
using namespace std;

#include "SyscMonLib.h"

namespace Ht {

	CSyscMon::CSyscMon(char const * pPlatform, int unitCnt) {
		m_pSyscMonLib = new CSyscMonLib(pPlatform, unitCnt);
		RegisterModule("HIF", 0, 0, 0, 0, 1);
	}

	int CSyscMon::RegisterModule(char const * pPathName, char const * pSyscPath, int htIdW, int instrW, char const ** pInstrNames, int memPortCnt) {
		m_pSyscMonLib->m_pModList.push_back(new CSyscMonMod(pPathName, pSyscPath, htIdW, instrW, pInstrNames, memPortCnt));
		return (int)m_pSyscMonLib->m_pModList.size()-1;
	}

	void CSyscMon::UpdateValidInstrCnt(int modId, int htInstr) {
		if (m_pSyscMonLib == 0) return;
		assert(modId < (int)m_pSyscMonLib->m_pModList.size());
		CSyscMonMod * pMod = m_pSyscMonLib->m_pModList[modId];
		pMod->m_htValidCnt += 1;
		pMod->m_htInstrValidCnt[htInstr] += 1;
	}

	void CSyscMon::UpdateRetryInstrCnt(int modId, int htInstr) {
		if (m_pSyscMonLib == 0) return;
		assert(modId < (int)m_pSyscMonLib->m_pModList.size());
		CSyscMonMod * pMod = m_pSyscMonLib->m_pModList[modId];
		pMod->m_htRetryCnt += 1;
		pMod->m_htInstrRetryCnt[htInstr] += 1;
	}

	void CSyscMon::UpdateModuleMemReadBytes(int modId, int portId, Int64 bytes) {
		if (m_pSyscMonLib == 0) return;
		assert(modId < (int)m_pSyscMonLib->m_pModList.size());
		CSyscMonMod * pMod = m_pSyscMonLib->m_pModList[modId];
		pMod->m_readMemBytes[portId] += bytes;
	}

	void CSyscMon::UpdateModuleMemReads(int modId, int portId, Int64 cnt) {
		if (m_pSyscMonLib == 0) return;
		assert(modId < (int)m_pSyscMonLib->m_pModList.size());
		CSyscMonMod * pMod = m_pSyscMonLib->m_pModList[modId];
		pMod->m_readMemCnt[portId] += cnt;
	}

	void CSyscMon::UpdateModuleMemRead64s(int modId, int portId, Int64 cnt) {
		if (m_pSyscMonLib == 0) return;
		assert(modId < (int)m_pSyscMonLib->m_pModList.size());
		CSyscMonMod * pMod = m_pSyscMonLib->m_pModList[modId];
		pMod->m_read64MemCnt[portId] += cnt;
	}

	void CSyscMon::UpdateModuleMemWriteBytes(int modId, int portId, Int64 bytes) {
		if (m_pSyscMonLib == 0) return;
		assert(modId < (int)m_pSyscMonLib->m_pModList.size());
		CSyscMonMod * pMod = m_pSyscMonLib->m_pModList[modId];
		pMod->m_writeMemBytes[portId] += bytes;
	}

	void CSyscMon::UpdateModuleMemWrites(int modId, int portId, Int64 cnt) {
		if (m_pSyscMonLib == 0) return;
		assert(modId < (int)m_pSyscMonLib->m_pModList.size());
		CSyscMonMod * pMod = m_pSyscMonLib->m_pModList[modId];
		pMod->m_writeMemCnt[portId] += cnt;
	}

	void CSyscMon::UpdateModuleMemWrite64s(int modId, int portId, Int64 cnt) {
		if (m_pSyscMonLib == 0) return;
		assert(modId < (int)m_pSyscMonLib->m_pModList.size());
		CSyscMonMod * pMod = m_pSyscMonLib->m_pModList[modId];
		pMod->m_write64MemCnt[portId] += cnt;
	}

	void CSyscMon::UpdateActiveClks(int modId, Int64 cnt) {
		if (m_pSyscMonLib == 0) return;
		assert(modId < (int)m_pSyscMonLib->m_pModList.size());
		CSyscMonMod * pMod = m_pSyscMonLib->m_pModList[modId];
		pMod->m_activeClks += cnt;
	}

	void CSyscMon::UpdateActiveCnt(int modId, uint32_t activeCnt) {
		if (m_pSyscMonLib == 0) return;
		assert(modId < (int)m_pSyscMonLib->m_pModList.size());
		CSyscMonMod * pMod = m_pSyscMonLib->m_pModList[modId];
		if (activeCnt > pMod->m_maxActiveCnt)
			pMod->m_maxActiveCnt = activeCnt;
		pMod->m_activeSum += activeCnt;
		pMod->m_activeClks += 1;
	}

	void CSyscMon::UpdateRunningCnt(int modId, uint32_t runningCnt) {
		if (m_pSyscMonLib == 0) return;
		assert(modId < (int)m_pSyscMonLib->m_pModList.size());
		CSyscMonMod * pMod = m_pSyscMonLib->m_pModList[modId];
		pMod->m_runningSum += runningCnt;
	}

	void CSyscMon::UpdateMemReqClocks(bool bHost, Int64 reqClocks) {
		if (m_pSyscMonLib == 0) return;
		if (m_pSyscMonLib->m_minMemClks[bHost] > reqClocks)
			m_pSyscMonLib->m_minMemClks[bHost] = reqClocks;
		if (m_pSyscMonLib->m_maxMemClks[bHost] < reqClocks)
			m_pSyscMonLib->m_maxMemClks[bHost] = reqClocks;
		m_pSyscMonLib->m_totalMemClks[bHost] += reqClocks;
		m_pSyscMonLib->m_totalMemOps[bHost] += 1;
	}

	void CSyscMon::UpdateTotalMemReadBytes(bool bHost, Int64 bytes) {
		if (m_pSyscMonLib == 0) return;
		m_pSyscMonLib->m_totalMemReadBytes[bHost] += bytes;
	}

	void CSyscMon::UpdateTotalMemRead64s(bool bHost, Int64 cnt) {
		if (m_pSyscMonLib == 0) return;
		m_pSyscMonLib->m_totalMemRead64s[bHost] += cnt;
	}

	void CSyscMon::UpdateTotalMemReads(bool bHost, Int64 cnt) {
		if (m_pSyscMonLib == 0) return;
		m_pSyscMonLib->m_totalMemReads[bHost] += cnt;
	}

	void CSyscMon::UpdateTotalMemWriteBytes(bool bHost, Int64 bytes) {
		if (m_pSyscMonLib == 0) return;
		m_pSyscMonLib->m_totalMemWriteBytes[bHost] += bytes;
	}

	void CSyscMon::UpdateTotalMemWrite64s(bool bHost, Int64 cnt) {
		if (m_pSyscMonLib == 0) return;
		m_pSyscMonLib->m_totalMemWrite64s[bHost] += cnt;
	}

	void CSyscMon::UpdateTotalMemWrites(bool bHost, Int64 cnt) {
		if (m_pSyscMonLib == 0) return;
		m_pSyscMonLib->m_totalMemWrites[bHost] += cnt;
	}

	bool CSyscMonLib::HtMonSortByActiveCnt(const CSyscMonMod * pModA, const CSyscMonMod * pModB)
	{
		return pModA->m_activeSum > pModB->m_activeSum;
	}

	CSyscMon::~CSyscMon() {

		vector<CSyscMonMod *> sortedModList = m_pSyscMonLib->m_pModList;
		std::sort(sortedModList.begin(), sortedModList.end(), CSyscMonLib::HtMonSortByActiveCnt);

		// Generate the report
		FILE * monFp;
		if (!(monFp = fopen("HtMonRpt.txt", "w"))) {
			fprintf(stderr, "Could not open HtMonRpt.txt\n");
			exit(1);
		}

		long long sim_cyc = (long long)sc_time_stamp().value()/10000;

		fprintf(monFp, "Simulated Cycles	: %8lld\n", sim_cyc);
		fprintf(monFp, "Platform		: %8s\n", m_pSyscMonLib->m_pPlatform);
		fprintf(monFp, "Unit Count		: %8d\n", m_pSyscMonLib->m_unitCnt);

		fprintf(monFp, "\n#\n");
		fprintf(monFp, "# Memory Summary\n");
		fprintf(monFp, "#\n");
		fprintf(monFp, "Host Latency (cyc)	: %8lld / %9lld / %8lld  (Min / Avg / Max)\n",
			m_pSyscMonLib->m_minMemClks[1], m_pSyscMonLib->m_totalMemClks[1] / max(1LL, m_pSyscMonLib->m_totalMemOps[1]), m_pSyscMonLib->m_maxMemClks[1]);
		fprintf(monFp, "CP Latency (cyc)	: %8lld / %9lld / %8lld  (Min / Avg / Max)\n",
			min(m_pSyscMonLib->m_minMemClks[0], m_pSyscMonLib->m_maxMemClks[0]),
			m_pSyscMonLib->m_totalMemClks[0] / max(1LL, m_pSyscMonLib->m_totalMemOps[0]), m_pSyscMonLib->m_maxMemClks[0]);
		fprintf(monFp, "Host Operations		: %8lld / %9lld\t\t   (Read / Write)\n",
			m_pSyscMonLib->m_totalMemReads[1] + m_pSyscMonLib->m_totalMemRead64s[1],
			m_pSyscMonLib->m_totalMemWrites[1] + m_pSyscMonLib->m_totalMemWrite64s[1]);
		fprintf(monFp, "CP Operations		: %8lld / %9lld\t\t   (Read / Write)\n",
			m_pSyscMonLib->m_totalMemReads[0] + m_pSyscMonLib->m_totalMemRead64s[0], 
			m_pSyscMonLib->m_totalMemWrites[0] + m_pSyscMonLib->m_totalMemWrite64s[0]);
		fprintf(monFp, "Host Efficiency		: %7.2f%% / %8.2f%%\t\t   (Read / Write)\n",
			100.0 * m_pSyscMonLib->m_totalMemReadBytes[1] / (64 * m_pSyscMonLib->m_totalMemReads[1] + 64 * m_pSyscMonLib->m_totalMemRead64s[1]),
			100.0 * m_pSyscMonLib->m_totalMemWriteBytes[1] / (64 * m_pSyscMonLib->m_totalMemWrites[1] + 64 * m_pSyscMonLib->m_totalMemWrite64s[1]));
		fprintf(monFp, "CP Efficiency		: %7.2f%% / %8.2f%%\t\t   (Read / Write)\n",
			100.0 * m_pSyscMonLib->m_totalMemReadBytes[0] / max(1LL, (64 * m_pSyscMonLib->m_totalMemReads[0] + 64 * m_pSyscMonLib->m_totalMemRead64s[0])),
			100.0 * m_pSyscMonLib->m_totalMemWriteBytes[0] / max(1LL, (64 * m_pSyscMonLib->m_totalMemWrites[0] + 64 * m_pSyscMonLib->m_totalMemWrite64s[0])));
		long long rq_cyc, rs_cyc;
		rq_cyc  = m_pSyscMonLib->m_totalMemReads[1] + m_pSyscMonLib->m_totalMemRead64s[1];
		rq_cyc += m_pSyscMonLib->m_totalMemWrites[1] + m_pSyscMonLib->m_totalMemWrite64s[1] * 8;
		rs_cyc  = m_pSyscMonLib->m_totalMemReads[1] + m_pSyscMonLib->m_totalMemRead64s[1] * 8;
		rs_cyc += m_pSyscMonLib->m_totalMemWrites[1] + m_pSyscMonLib->m_totalMemWrite64s[1];
		fprintf(monFp, "Host Utiliztion		: %7.2f%% / %8.2f%%\t\t   (Req / Resp)\n",
			100.0 * rq_cyc / sortedModList[0]->m_activeClks / m_pSyscMonLib->m_unitCnt,
			100.0 * rs_cyc / sortedModList[0]->m_activeClks / m_pSyscMonLib->m_unitCnt);
		rq_cyc  = m_pSyscMonLib->m_totalMemReads[0] + m_pSyscMonLib->m_totalMemRead64s[0];
		rq_cyc += m_pSyscMonLib->m_totalMemWrites[0] + m_pSyscMonLib->m_totalMemWrite64s[0] * 8;
		rs_cyc  = m_pSyscMonLib->m_totalMemReads[0] + m_pSyscMonLib->m_totalMemRead64s[0] * 8;
		rs_cyc += m_pSyscMonLib->m_totalMemWrites[0] + m_pSyscMonLib->m_totalMemWrite64s[0];
		fprintf(monFp, "CP Utiliztion		: %7.2f%% / %8.2f%%\t\t   (Req / Resp)\n",
			100.0 * rq_cyc / sortedModList[0]->m_activeClks / m_pSyscMonLib->m_unitCnt,
			100.0 * rs_cyc / sortedModList[0]->m_activeClks / m_pSyscMonLib->m_unitCnt);

		fprintf(monFp, "\n#\n");
		fprintf(monFp, "%-35s %10s %10s %10s %10s\n",
			"# Memory Operations", "Read   ", "ReadMw  ", "Write  ", "WriteMw ");
		fprintf(monFp, "%-35s %10s %10s %10s %10s\n",
			"#", "----------", "----------", "----------", "----------");
		fprintf(monFp, "%-35s %10lld %10lld %10lld %10lld\n",
			"<total>", m_pSyscMonLib->m_totalMemReads[0] + m_pSyscMonLib->m_totalMemReads[1],
			m_pSyscMonLib->m_totalMemRead64s[0] + m_pSyscMonLib->m_totalMemRead64s[1],
			m_pSyscMonLib->m_totalMemWrites[0] + m_pSyscMonLib->m_totalMemWrites[1],
			m_pSyscMonLib->m_totalMemWrite64s[0] + m_pSyscMonLib->m_totalMemWrite64s[1]);

		for (size_t i = 0; i < m_pSyscMonLib->m_pModList.size(); i += 1) {
			CSyscMonMod * pMod = m_pSyscMonLib->m_pModList[i];

			Int64 readMemCnt = 0;
			Int64 read64MemCnt = 0;
			Int64 writeMemCnt = 0;
			Int64 write64MemCnt = 0;

			for (int j = 0; j < pMod->m_memPortCnt; j += 1) {
				readMemCnt += pMod->m_readMemCnt[j];
				read64MemCnt += pMod->m_read64MemCnt[j];
				writeMemCnt += pMod->m_writeMemCnt[j];
				write64MemCnt += pMod->m_write64MemCnt[j];
			}

			fprintf(monFp, "%-35s %10lld %10lld %10lld %10lld\n",
				pMod->m_pathName.c_str(), readMemCnt, read64MemCnt, writeMemCnt, write64MemCnt);

			if (pMod->m_memPortCnt > 1)
				for (int j = 0; j < pMod->m_memPortCnt; j += 1) {
					char buf[64];
					sprintf(buf, "   Memory Port %d", j);
					fprintf(monFp, "%-35s %10lld %10lld %10lld %10lld\n",
						buf, pMod->m_readMemCnt[j], pMod->m_read64MemCnt[j], pMod->m_writeMemCnt[j], pMod->m_write64MemCnt[j]);
				}
		}

		fprintf(monFp, "\n#\n");
		fprintf(monFp, "%-35s %10s %10s %10s %10s\n",
			"# Thread Utilization", "Avg. Run ", "Avg. Alloc", "Max. Alloc", "Available");
		fprintf(monFp, "%-35s %10s %10s %10s %10s\n",
			"#", "----------", "----------", "----------", "----------");

		for (size_t i = 0; i < m_pSyscMonLib->m_pModList.size(); i += 1) {
			CSyscMonMod * pMod = m_pSyscMonLib->m_pModList[i];

			fprintf(monFp, "%-35s %10lld %10lld %10lld %10lld\n",
				pMod->m_pathName.c_str(),
				pMod->m_runningSum / max(1LL, pMod->m_activeClks),
				pMod->m_activeSum / max(1LL, pMod->m_activeClks),
				pMod->m_maxActiveCnt, pMod->m_availableCnt);
		}

		fprintf(monFp, "\n#\n");
		fprintf(monFp, "%-35s %10s %10s %10s %10s\n",
			"# Module Utilization", "Valid  ", "Retry  ", "Active Cyc", "Act. Util");
		fprintf(monFp, "%-35s %10s %10s %10s %10s\n",
			"#", "----------", "----------", "----------", "----------");

		for (size_t i = 0; i < m_pSyscMonLib->m_pModList.size(); i += 1) {
			CSyscMonMod * pMod = m_pSyscMonLib->m_pModList[i];

			fprintf(monFp, "%-35s %10lld %10lld %10lld %9.2f%%\n",
				pMod->m_pathName.c_str(), pMod->m_htValidCnt, pMod->m_htRetryCnt, pMod->m_activeClks,
				100.0 * pMod->m_activeClks / sim_cyc);
			for (int i = 0; i < (1 << pMod->m_modInstrW); i += 1) {
				if (pMod->m_htInstrValidCnt[i] > 0 || pMod->m_htInstrRetryCnt[i] > 0)
					fprintf(monFp, "  %-33s %10lld %10lld\n",
						pMod->m_pInstrNames[i], pMod->m_htInstrValidCnt[i], pMod->m_htInstrRetryCnt[i]);
			}
		}

		fclose(monFp);

		CSyscMonLib * pTmp = m_pSyscMonLib;
		m_pSyscMonLib = 0;

		delete pTmp;
	}
}

