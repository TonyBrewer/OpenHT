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

#include "GenHtmlRpt.h"

#if !defined(VERSION)
#define VERSION "unknown"
#endif
#if !defined(BLDDTE)
#define BLDDTE "unknown"
#endif
#if !defined(VCSREV)
#define VCSREV "unknown"
#endif

enum ECoproc { hcx, hc1, hc1ex, hc2, hc2ex, wx690, wx2k, ma100, ma400 };

// platform capabilities
class CCoprocInfo {
public:
	CCoprocInfo() : m_coproc(hcx), m_pCoprocName(0) {}
	CCoprocInfo(ECoproc coproc, char const * pCoprocAsStr, char const * pCoprocName, int maxHostQwReadCnt,
		int maxHostQwWriteCnt, int maxCoprocQwReadCnt, int maxCoprocQwWriteCnt, bool bVarQwReqCnt,
		int bramsPerAe) :
		m_coproc(coproc),
		m_pCoprocAsStr(pCoprocAsStr),
		m_pCoprocName(pCoprocName),
		m_maxHostQwReadCnt(maxHostQwReadCnt),
		m_maxHostQwWriteCnt(maxHostQwWriteCnt),
		m_maxCoprocQwReadCnt(maxCoprocQwReadCnt),
		m_maxCoprocQwWriteCnt(maxCoprocQwWriteCnt),
		m_bVarQwReqCnt(bVarQwReqCnt),
		m_bramsPerAe(bramsPerAe)
	{
		m_bIsMultiQwSupported = m_maxHostQwReadCnt > 1 || m_maxHostQwWriteCnt > 1 ||
			m_maxCoprocQwReadCnt > 1 || m_maxCoprocQwWriteCnt > 1;
	}

	ECoproc GetCoproc() const { return m_coproc; }
	char const * GetCoprocAsStr() const { return m_pCoprocAsStr; }
	char const * GetCoprocName() const { return m_pCoprocName; }

	int GetMaxHostQwReadCnt() const { return m_maxHostQwReadCnt; }
	int GetMaxHostQwWriteCnt() const { return m_maxHostQwWriteCnt; }
	int GetMaxCoprocQwReadCnt() const { return m_maxCoprocQwReadCnt; }
	int GetMaxCoprocQwWriteCnt() const { return m_maxCoprocQwWriteCnt; }

	bool IsVarQwReqCnt() const { return m_bVarQwReqCnt; }

	int GetBramsPerAE() const { return m_bramsPerAe; }

	bool IsMultiQwSuppported() const { return m_bIsMultiQwSupported; }

private:
	ECoproc m_coproc;
	char const * m_pCoprocAsStr;
	char const * m_pCoprocName;

	int m_maxHostQwReadCnt;		// max QW reads in single request to host memory
	int m_maxHostQwWriteCnt;	// max QW writes in single request from host memory
	int m_maxCoprocQwReadCnt;	// max QW reads in single request to coproc memory
	int m_maxCoprocQwWriteCnt;	// max QW writes in single request from coproc memory

	bool m_bVarQwReqCnt;		// multi QW requests can be variable in size

	int m_bramsPerAe;			// number of block rams per AE

	bool m_bIsMultiQwSupported;
};
extern CCoprocInfo g_coprocInfo[];

class CAppArgs {
public:
	CAppArgs()
	{
		m_coprocId = -1;
	}
	~CAppArgs()
	{
		if (IsVariableReportEnabled())
			fclose(m_pVarRptFp);

		if (m_pDsnRpt)
			delete m_pDsnRpt;
	}

	void Parse(int argc, char const ** argv);
	bool SetHtCoproc(char const * pStr);

	bool IsGenReportsEnabled() { return m_bGenReports; }
	int GetInputFileCnt() { return (int)m_inputFileList.size(); }
	string GetInputFile(int i) { return m_inputFileList[i]; }
	string GetOutputFolder() { return m_outputFolder; }
	string GetInstanceFile() { return m_instanceFile; }
	int GetDefaultFreq() { return m_defaultFreqMhz; }
	string GetUnitName() { return m_unitName; }
	string GetProjName() { return m_projName; }
	string GetHtlName() { return m_htlName; }
	string & GetEntryName() { return m_entryName; }
	string & GetIqModName() { return m_iqModName; }
	string & GetOqModName() { return m_oqModName; }
	bool IsPerfMonEnabled() { return m_bPerfMon; }
	bool IsInstrTraceEnabled() { return m_bInstrTrace; }
	bool IsRndRetryEnabled() { return m_bRndRetry; }
	bool IsRndInitEnabled() { return m_bRndInit; }
	bool IsRndTestEnabled() { return m_bRndTest; }
	bool IsMemTraceEnabled() { return m_bMemTrace; }
	bool IsModelOnly() { return m_bModelOnly; }
	bool IsForkPrivWr() { return m_bForkPrivWr; }
	bool IsVariableReportEnabled() { return m_bVariableReport; }
	bool IsModuleUnitNamesEnabled() { return m_bModuleUnitNames; }
	bool IsGlobalWriteHtidEnabled() { return m_bGlobalWriteHtid; }
	bool IsGlobalReadParanEnabled() { return m_bGlobalReadParan; }
	bool IsVcdUserEnabled() { return m_bVcdUser; }
	bool IsVcdAllEnabled() { return m_bVcdAll; }
	bool IsOgv() { return m_bOgv; }
	FILE * GetVarRptFp() { return m_pVarRptFp; }
	int GetAeCnt() { return m_aeCnt; }
	int GetAeUnitCnt() { return m_aeUnitCnt; }
	int GetHostHtIdW() { return m_hostHtIdW; }
	int GetMax18KbBramPerUnit() { return m_max18KbBramPerUnit; }
	int GetMinSliceToBramRatio() { return m_minSliceToBramRatio; }
	string GetFxModName() { return m_fxModName; }
	int GetArgMemLatency(int i) { return m_avgMemLatency[i]; }
	vector<string> &GetIncludeDirs() { return m_includeDirs; }
	int GetVcdStartCycle() { return m_vcdStartCycle; }

	CGenHtmlRpt & GetDsnRpt() { return *m_pDsnRpt; }

	int GetPreDefinedNameCnt() { return m_preDefinedNames.size(); }
	string GetPreDefinedName(int i) { return m_preDefinedNames[i].first; }
	string GetPreDefinedValue(int i) { return m_preDefinedNames[i].second; }

	CCoprocInfo const & GetCoprocInfo() { return g_coprocInfo[m_coprocId]; }
	//ECoproc GetCoproc() { return m_coprocInfo.GetCoproc(); }
	char const * GetCoprocName() { return g_coprocInfo[m_coprocId].GetCoprocName(); }
	char const * GetCoprocAsStr() { return g_coprocInfo[m_coprocId].GetCoprocAsStr(); }
	int GetBramsPerAE() { return g_coprocInfo[m_coprocId].GetBramsPerAE(); }

	void GetArgs(int & argc, char const ** &argv)
	{
		argc = m_argc; argv = m_argv;
	}

	uint32_t GetRtRndIdx(size_t size) { return m_mtRand() % (uint32_t)size; }

	bool IsVcdFilterMatch(string name);

private:
	void ParseArgsFile(char const * pFileName);
	void Usage();
	void ReadVcdFilterFile();
	bool Glob(const char * pName, const char * pFilter);
	void EnvVarExpansion(string & path);

private:
	int				m_argc;
	char const **	m_argv;

	string			m_progName;
	string			m_instanceFile;
	vector<string>	m_inputFileList;
	string			m_outputFolder;
	vector<pair<string, string> >	m_preDefinedNames;
	vector<string>	m_includeDirs;

	int				m_aeCnt;
	bool			m_bGenReports;
	bool			m_bInstrTrace;
	bool			m_bPerfMon;
	bool			m_bRndInit;
	bool			m_bRndRetry;
	bool			m_bRndTest;
	bool			m_bMemTrace;
	bool			m_bVariableReport;
	bool			m_bModuleUnitNames;
	bool			m_bGlobalWriteHtid;
	bool			m_bGlobalReadParan;
	int				m_aeUnitCnt;
	string          m_htlName;
	string			m_projName;
	string			m_unitName;
	string			m_entryName;
	int				m_hostHtIdW;
	string			m_iqModName;
	string			m_oqModName;
	int				m_defaultFreqMhz;
	int				m_max18KbBramPerUnit;
	int				m_minSliceToBramRatio;
	string			m_fxModName;
	int				m_avgMemLatency[2];
	int				m_coprocId;
	bool            m_bModelOnly;
	bool			m_bForkPrivWr;
	bool			m_bDsnRpt;
	CGenHtmlRpt *	m_pDsnRpt;
	bool			m_bVcdUser;
	bool			m_bVcdAll;
	int				m_vcdStartCycle;
	string			m_vcdFilterFile;
	vector<string>	m_vcdFilterList;
	bool			m_bOgv;

	FILE *			m_pVarRptFp;

	MTRand_int32	m_mtRand;
};

extern CAppArgs g_appArgs;
