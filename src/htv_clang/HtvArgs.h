/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
// HtvArgs.h: interface for the ScArgs class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

class CHtvArgs
{
public:
    CHtvArgs();
    virtual ~CHtvArgs();

    void Usage(const char *progname);
    void Parse(int argc, char const * argv[]);

	void GetArgs(int & argc, char const ** &argv) {
		argc = m_argc; argv = m_argv;
	}

	std::vector<std::string> & GetFrontEndArgs() { return m_frontEndArgs; }
	std::vector<std::string> & GetSourceFiles() { return m_sourceFiles; }

    bool IsScHier() { return m_bScHier; }
    bool IsScMain() { return m_bScMain; }
    bool IsScCode() { return m_bScCode; }

	bool IsMethodSupport() { return m_bMethodSupport; }
	bool IsTemplateSupport() { return m_bTemplateSupport; }
    bool IsTkDumpEnabled() { return m_bIsTkDumpEnabled; }
    bool IsWritePreProcessedInput() { return m_bWritePreProcessedInput; }
    bool IsStDumpEnabled() { return m_bIsStDumpEnabled; }
    bool IsLvxEnabled() { return m_bIsLvxEnabled; }
    bool IsInUseEnabled() { return m_bIsInUseEnabled; }
    bool IsHtQueCntSynKeepEnabled() { return m_bHtQueCntSynKeep; }

	void SetIsScHier() { m_bScHier = true; }
	void SetIsScCode() { m_bScCode = true; }

	void SetDesignPrefix(const std::string &designPrefixString) {
        m_designPrefixString = designPrefixString;
    }
    void SetXilinxFdPrims(bool bFdPrims) {
        m_bXilinxFdPrims = bFdPrims;
    }
    void SetSynXilinxPrims(bool bSynXilinxPrims) {
        m_bSynXilinxPrims = bSynXilinxPrims;
    }
    void SetSynDistRamPrims(bool bSynDistRamPrims) {
        m_bSynDistRamPrims = bSynDistRamPrims;
    }
    void SetGenFixture(bool bGenFixture) {
        m_bGenFixture = bGenFixture;
    }
    void SetGenSandbox(bool bGenSandbox=true) {
        m_bGenSandbox = bGenSandbox;
    }

	void setDumpHtvAST(bool bDumpHtvAST=true) {
		m_bDumpHtvAST = bDumpHtvAST;
	}

	void setDumpClangAST(bool bDumpClangAST=true) {
		m_bDumpClangAST = bDumpClangAST;
	}

	void setDumpPosAndWidth(bool bDumpPosAndWidth=true) {
		m_bDumpPosAndWidth = bDumpPosAndWidth;
	}

    std::string GetDesignPrefixString() { return m_designPrefixString; }
    std::string GetInputPathName() { return m_inputPathName; }
    std::string GetInputFileName() { return m_inputFileName; }
    std::string GetIncFileName() { return m_incFileName; }
    std::string GetVFileName() { return m_vFileName; }
    std::string GetCppFileName() { return m_cppFileName; }
    bool GetXilinxFdPrims() { return m_bXilinxFdPrims; }

    bool IsGenFixture() { return m_bGenFixture; }
    bool IsGenSandbox() { return m_bGenSandbox; }
    bool IsSynXilinxPrims() { return m_bSynXilinxPrims; }
    bool IsSynDistRamPrims() { return m_bSynDistRamPrims; }
    bool IsXilinxFdPrims() { return m_bXilinxFdPrims; }

    bool IsLedaEnabled() { return m_bIsLedaEnabled; }
    void SetIsLedaEnabled() { m_bIsLedaEnabled = true; }

    bool IsPrmEnabled() { return m_bIsPrmEnabled; }
    void SetIsPrmEnabled() { m_bIsPrmEnabled = true; }

    bool IsKeepHierarchyEnabled() { return m_bKeepHierarchy; }

	bool isDumpHtvAST() { return m_bDumpHtvAST; }
	bool isDumpClangAST() { return m_bDumpClangAST; }
	bool isDumpPosAndWidth() { return m_bDumpPosAndWidth; }

private:
	void SetMethodSupport(bool bMethod) { m_bMethodSupport = bMethod; }
	void SetTemplateSupport(bool bTemplate) { m_bTemplateSupport = bTemplate; }
    void SetIsTkDumpEnabled() { m_bIsTkDumpEnabled = true; }
    void SetWritePreProcessedInput() { m_bWritePreProcessedInput = true; }
    void SetIsStDumpEnabled() { m_bIsStDumpEnabled = true; }
    void SetIsLvxEnabled() { m_bIsLvxEnabled = true; }
    void SetIsInUseEnabled(bool bIsInUseEnabled) { m_bIsInUseEnabled = bIsInUseEnabled; }

private:
	int				m_argc;
	char const **	m_argv;

    std::vector<std::string>	m_frontEndArgs;
	std::vector<std::string>	m_sourceFiles;

    bool            m_bScHier;
    bool            m_bScMain;
    bool            m_bScCode;

    bool			m_bIsStDumpEnabled;
    bool			m_bIsLvxEnabled;	// local variable X initialization else 0
	bool			m_bIsTkDumpEnabled;
    bool            m_bWritePreProcessedInput;
	bool			m_bMethodSupport;
	bool			m_bTemplateSupport;
    bool			m_bIsInUseEnabled;	// generate memory InUse methods
    bool            m_bHtQueCntSynKeep;

    bool	        m_bGenFiles;
    std::string     m_incFileName;
    std::string     m_cppFileName;
    std::string     m_vFileName;
    std::string     m_rptFileName;
    std::string     m_designPrefixString;
    std::string     m_inputPathName;
    std::string     m_inputFileName;
    bool	        m_bXilinxFdPrims;
    bool            m_bGenFixture;
    bool			m_bGenSandbox;
    bool            m_bSynXilinxPrims;
    bool			m_bSynDistRamPrims;
    bool			m_bIsLedaEnabled;
    bool			m_bIsPrmEnabled;
    bool            m_bKeepHierarchy;

	bool            m_bDumpHtvAST;
	bool            m_bDumpClangAST;
	bool            m_bDumpPosAndWidth;

};

extern CHtvArgs g_htvArgs;
