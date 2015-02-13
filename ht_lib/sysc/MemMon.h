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

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define MC_CNT 8
#define MC_BANK_CNT 128

class CMemMon {
public:
	CMemMon() {
		memset(m_mcRdCnt, 0, sizeof(uint64_t) * MC_CNT);
		memset(m_mcWrCnt, 0, sizeof(uint64_t) * MC_CNT);
		memset(m_bankRdCnt, 0, sizeof(uint64_t) * MC_CNT * MC_BANK_CNT);
		memset(m_bankWrCnt, 0, sizeof(uint64_t) * MC_CNT * MC_BANK_CNT);
		memset(m_logTable, 0, sizeof(uint64_t) * MC_CNT * MC_BANK_CNT);
		memset(m_invTable, 0, sizeof(uint64_t) * MC_CNT * MC_BANK_CNT);
	}

	~CMemMon() {
		FILE *fp = fopen("MemMon.txt", "w");
		double avg = 0;
		uint64_t max = 0;
		for (int mc = 0; mc < MC_CNT; mc += 1) {
			fprintf(fp, "MC%d Rd=%lld Wr=%lld\n", mc,
				(long long)m_mcRdCnt[mc],
				(long long)m_mcWrCnt[mc]);

			for (int bank = 0; bank < MC_BANK_CNT; bank += 1) {
				if (m_bankRdCnt[mc][bank] > max)
					max = m_bankRdCnt[mc][bank];
				avg += m_bankRdCnt[mc][bank];
			}
		}
		fprintf(fp, "max Rd bank = %lld\n", (long long)max);
		fprintf(fp, "avg Rd bank = %8.3f\n", avg / MC_CNT / MC_BANK_CNT);

		fprintf(fp, "\n\nLOG: ");
		for (int i = 0; i < MC_CNT * MC_BANK_CNT; i += 1)
			fprintf(fp, "%5lld ", (long long)m_logTable[i]);

		fprintf(fp, "\n\nINV: ");
		for (int i = 0; i < MC_CNT * MC_BANK_CNT; i += 1)
			fprintf(fp, "%5lld ", (long long)m_invTable[i]);

		fprintf(fp, "\n");

		fclose(fp);
	}

	void Read(uint64_t addr) {
		int mc = (addr >> 6) & 7;
		int bank = ((addr >> 6) & 0x78) | ((addr >> 3) & 0x7);

		m_mcRdCnt[mc] += 1;
		m_bankRdCnt[mc][bank] += 1;
	}

	void Write(uint64_t addr) {
		int mc = (addr >> 6) & 7;
		int bank = ((addr >> 6) & 0x78) | ((addr >> 3) & 0x7);

		m_mcWrCnt[mc] += 1;
		m_bankWrCnt[mc][bank] += 1;
	}

	void LogTable(int tableIdx) {
		int bank = tableIdx & 0x3ff;

		m_logTable[bank] += 1;
	}

	void InvTable(int tableIdx) {
		int bank = tableIdx & 0x3ff;

		m_invTable[bank] += 1;
	}

private:
	uint64_t m_mcRdCnt[MC_CNT];
	uint64_t m_mcWrCnt[MC_CNT];
	uint64_t m_bankRdCnt[MC_CNT][MC_BANK_CNT];
	uint64_t m_bankWrCnt[MC_CNT][MC_BANK_CNT];
	uint64_t m_logTable[MC_CNT * MC_BANK_CNT];
	uint64_t m_invTable[MC_CNT * MC_BANK_CNT];
};

//extern CMemMon memMon;
