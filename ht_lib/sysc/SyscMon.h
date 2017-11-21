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
	typedef long long Int64;

	enum EHtCoproc { hc1, hc1ex, hc2, hc2ex, wx690, wx2k, ma100, ma400, wx2vu7p};

	class CSyscMonLib;

	class CSyscMon {
	public:
		CSyscMon(char const * pPlatform, int unitCnt);
		~CSyscMon();

		int RegisterModule(char const * pPathName, char const * pSyscPath, int htIdW, int instrW, char const ** pInstrNames, int memPortCnt);
		void UpdateValidInstrCnt(int modId, int htInstr);
		void UpdateRetryInstrCnt(int modId, int htInstr);
		void UpdateModuleMemReadBytes(int modId, int portId, Int64 bytes);
		void UpdateModuleMemReads(int modId, int portId, Int64 cnt);
		void UpdateModuleMemRead64s(int modId, int portId, Int64 cnt);
		void UpdateModuleMemWriteBytes(int modId, int portId, Int64 bytes);
		void UpdateModuleMemWrites(int modId, int portId, Int64 cnt);
		void UpdateModuleMemWrite64s(int modId, int portId, Int64 cnt);
		void UpdateActiveClks(int modId, Int64 cnt);
		void UpdateActiveCnt(int modId, uint32_t activeCnt);
		void UpdateRunningCnt(int modId, uint32_t runningCnt);

		void UpdateMemReqClocks(bool bHost, Int64 reqClocks);
		void UpdateTotalMemReadBytes(bool bHost, Int64 bytes);
		void UpdateTotalMemRead64s(bool bHost, Int64 cnt);
		void UpdateTotalMemReads(bool bHost, Int64 cnt);
		void UpdateTotalMemWriteBytes(bool bHost, Int64 bytes);
		void UpdateTotalMemWrite64s(bool bHost, Int64 cnt);
		void UpdateTotalMemWrites(bool bHost, Int64 cnt);

	private:
		CSyscMonLib * m_pSyscMonLib;
	};

	extern CSyscMon g_syscMon;

	extern sc_trace_file *g_vcdp;
	extern const char *g_vcdn;
	extern FILE * g_instrTraceFp;
}

