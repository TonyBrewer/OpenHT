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

enum ECoproc { hcx, hc1, hc1ex, hc2, hc2ex, wx690, wx2k };

class CAppArgs {
public:
	~CAppArgs() {
		if (IsVariableReportEnabled())
			fclose(m_pVarRptFp);

		if (m_pDsnRpt)
			delete m_pDsnRpt;
	}

	void Parse(int argc, char const ** argv);
	void SetHtCoproc(char const * pStr);

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
	bool IsNewGlobalVarEnabled() { return m_bNewGlobalVar; }
	bool IsVariableReportEnabled() { return m_bVariableReport; }
	bool IsModuleUnitNamesEnabled() { return m_bModuleUnitNames; }
	bool IsGlobalWriteHtidEnabled() { return m_bGlobalWriteHtid; }
	bool IsGlobalReadParanEnabled() { return m_bGlobalReadParan; }
	bool IsVcdUserEnabled() { return m_bVcdUser; }
	bool IsVcdAllEnabled() { return m_bVcdAll; }
	FILE * GetVarRptFp() { return m_pVarRptFp; }
	int GetAeCnt() { return m_aeCnt; }
	int GetAeUnitCnt() { return m_aeUnitCnt; }
	int GetHostHtIdW() { return m_hostHtIdW; }
	int GetMax18KbBramPerUnit() { return m_max18KbBramPerUnit; }
	int GetMinLutToBramRatio() { return m_minLutToBramRatio; }
	string GetFxModName() { return m_fxModName; }
	int GetArgMemLatency(int i) { return m_avgMemLatency[i]; }
    vector<string> &GetIncludeDirs() { return m_includeDirs; }
	int GetVcdStartCycle() { return m_vcdStartCycle; }

	CGenHtmlRpt & GetDsnRpt() { return *m_pDsnRpt; }

    int GetPreDefinedNameCnt() { return m_preDefinedNames.size(); }
    string GetPreDefinedName(int i) { return m_preDefinedNames[i].first; }
    string GetPreDefinedValue(int i) { return m_preDefinedNames[i].second; }

    ECoproc GetCoproc() { return m_coproc; }
	char * GetCoprocName();
	char const * GetCoprocAsStr();
	int GetBramsPerAE();

	void GetArgs(int & argc, char const ** &argv) {
		argc = m_argc; argv = m_argv;
	}

	uint32_t GetRtRndIdx(size_t size) { return m_mtRand() % (uint32_t)size; }

	bool IsVcdFilterMatch(string name);

private:
	void ParseArgsFile(char const * pFileName);
	void Usage();
	void ReadVcdFilterFile();
	bool Glob(const char * pName, const char * pFilter);

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
	bool			m_bNewGlobalVar;
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
	int				m_minLutToBramRatio;
	string			m_fxModName;
	int				m_avgMemLatency[2];
    ECoproc         m_coproc;
    bool            m_bModelOnly;
	bool			m_bForkPrivWr;
	bool			m_bDsnRpt;
	CGenHtmlRpt *	m_pDsnRpt;
	bool			m_bVcdUser;
	bool			m_bVcdAll;
	int				m_vcdStartCycle;
	string			m_vcdFilterFile;
	vector<string>	m_vcdFilterList;

	FILE *			m_pVarRptFp;

	MTRand_int32	m_mtRand;
};

extern CAppArgs g_appArgs;
