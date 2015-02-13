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

#include <vector>
using namespace std;

#include "SyscMon.h"

namespace Ht {

	class CSyscMonMod {
		friend class CSyscMon;
		friend class CSyscMonLib;
	public:
		CSyscMonMod(char const * pPathName, char const * pSyscPath, int htIdW, int modInstrW, char const ** pInstrNames, int memPortCnt) {
			m_pathName = pPathName;
			m_memPortCnt = memPortCnt;

			int syscPathLen;
			if (pSyscPath && (syscPathLen = (int)strlen(pSyscPath)) > 1 && pSyscPath[syscPathLen-1] == ']') {
				int pos = syscPathLen-1;
				while (pos > 0 && pSyscPath[pos] != '[') pos -= 1;
				if (pSyscPath[pos] > 0)
					m_pathName += &pSyscPath[pos];
			}

			m_modInstrW = modInstrW;
			m_pInstrNames = pInstrNames;
			m_htValidCnt = 0;
			m_htInstrValidCnt.resize(1 << modInstrW, 0);
			m_htRetryCnt = 0;
			m_htInstrRetryCnt.resize(1 << modInstrW, 0);
			m_readMemCnt.resize(m_memPortCnt, 0);
			m_read64MemCnt.resize(m_memPortCnt, 0);
			m_readMemBytes.resize(m_memPortCnt, 0);
			m_writeMemCnt.resize(m_memPortCnt, 0);
			m_write64MemCnt.resize(m_memPortCnt, 0);
			m_writeMemBytes.resize(m_memPortCnt, 0);
			m_maxActiveCnt = 0;
			m_availableCnt = 1ull << htIdW;
			m_activeSum = 0;
			m_activeClks = 0;
			m_runningSum = 0;
		}

	private:
		string m_pathName;
		int m_memPortCnt;
		int m_modInstrW;
		char const ** m_pInstrNames;
		Int64 m_htValidCnt;
		vector<Int64> m_htInstrValidCnt;
		Int64 m_htRetryCnt;
		vector<Int64> m_htInstrRetryCnt;
		vector<Int64> m_readMemCnt;
		vector<Int64> m_read64MemCnt;
		vector<Int64> m_readMemBytes;
		vector<Int64> m_writeMemCnt;
		vector<Int64> m_write64MemCnt;
		vector<Int64> m_writeMemBytes;
		Int64 m_maxActiveCnt;
		Int64 m_availableCnt;
		Int64 m_activeSum;
		Int64 m_activeClks;
		Int64 m_runningSum;
	};

	class CSyscMonLib {
		friend class CSyscMon;
	public:
		CSyscMonLib(char const * pPlatform, int unitCnt) {
			m_pPlatform = pPlatform;
			m_unitCnt = unitCnt;

			for (int i = 0; i < 2; i += 1) {
				m_minMemClks[i] = 0x7fffffffffffffffLL;
				m_maxMemClks[i] = 0;
				m_totalMemClks[i] = 0;
				m_totalMemOps[i] = 0;
				m_totalMemReads[i] = 0;
				m_totalMemWrites[i] = 0;
				m_totalMemRead64s[i] = 0;
				m_totalMemWrite64s[i] = 0;
				m_totalMemReadBytes[i] = 0;
				m_totalMemWriteBytes[i] = 0;
			}
		}

	public:
		static bool HtMonSortByActiveCnt(const CSyscMonMod * pModA, const CSyscMonMod * pModB);

	private:
		char const * m_pPlatform;
		int m_unitCnt;

		vector<CSyscMonMod *> m_pModList;

		Int64 m_minMemClks[2];
		Int64 m_maxMemClks[2];
		Int64 m_totalMemClks[2];
		Int64 m_totalMemOps[2];
		Int64 m_totalMemReads[2];
		Int64 m_totalMemWrites[2];
		Int64 m_totalMemRead64s[2];
		Int64 m_totalMemWrite64s[2];
		Int64 m_totalMemReadBytes[2];
		Int64 m_totalMemWriteBytes[2];
	};
}
