/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
// HtfeArgs.h: interface for the ScArgs class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

class CHtfeArgs
{
public:
    CHtfeArgs();
    virtual ~CHtfeArgs();

    void Usage();
    void Parse(int & argPos, char *argv[]);

	void SetIsScHier() { m_bScHier = true; }
	void SetIsScCode() { m_bScCode = true; }

private:
	void SetMethodSupport(bool bMethod) { m_bMethodSupport = bMethod; }
	void SetTemplateSupport(bool bTemplate) { m_bTemplateSupport = bTemplate; }
    void SetIsTkDumpEnabled() { m_bIsTkDumpEnabled = true; }
    void SetWritePreProcessedInput() { m_bWritePreProcessedInput = true; }
    void SetIsStDumpEnabled() { m_bIsStDumpEnabled = true; }
    void SetIsLvxEnabled() { m_bIsLvxEnabled = true; }

public:
	virtual void GetArgs(int & argc, char ** &argv) = 0;

    int GetPreDefinedNameCnt() { return m_preDefinedNames.size(); }
    string GetPreDefinedName(int i) { return m_preDefinedNames[i]; }

    vector<string> &GetIncludeDirs() { return m_includeDirs; }

    bool IsScHier() { return m_bScHier; }
    bool IsScMain() { return m_bScMain; }
    bool IsScCode() { return m_bScCode; }

	bool IsMethodSupport() { return m_bMethodSupport; }
	bool IsTemplateSupport() { return m_bTemplateSupport; }
    bool IsTkDumpEnabled() { return m_bIsTkDumpEnabled; }
    bool IsWritePreProcessedInput() { return m_bWritePreProcessedInput; }
    bool IsStDumpEnabled() { return m_bIsStDumpEnabled; }
    bool IsLvxEnabled() { return m_bIsLvxEnabled; }
    bool IsHtQueCntSynKeepEnabled() { return m_bHtQueCntSynKeep; }

private:
    vector<string>	m_preDefinedNames;
    vector<string>	m_includeDirs;

    bool            m_bScHier;
    bool            m_bScMain;
    bool            m_bScCode;

    bool			m_bIsStDumpEnabled;
    bool			m_bIsLvxEnabled;	// local variable X initialization else 0
	bool			m_bIsTkDumpEnabled;
    bool            m_bWritePreProcessedInput;
	bool			m_bMethodSupport;
	bool			m_bTemplateSupport;
    bool            m_bHtQueCntSynKeep;
};

extern CHtfeArgs * g_pHtfeArgs;
