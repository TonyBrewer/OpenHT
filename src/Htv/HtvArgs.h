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

#include "HtfeArgs.h"

class CHtvArgs : public CHtfeArgs
{
public:
    CHtvArgs();
    virtual ~CHtvArgs();

    void Usage(const char *progname);
    void Parse(int argc, char *argv[]);

	void GetArgs(int & argc, char ** &argv) {
		argc = m_argc; argv = m_argv;
	}

    void SetDesignPrefix(const string &designPrefixString) {
        m_designPrefixString = designPrefixString;
    }
    void SetXilinxFdPrims(bool bFdPrims) {
        m_bXilinxFdPrims = bFdPrims;
    }
    void SetSynXilinxPrims(bool bSynXilinxPrims) {
        m_bSynXilinxPrims = bSynXilinxPrims;
    }
	void SetVivadoEnabled(bool bVivado)
	{
		m_bVivado = bVivado;
	}
	void SetQuartusEnabled(bool bQuartus)
	{
		m_bQuartus = bQuartus;
		SetAlteraDistRams(bQuartus);
	}
	void SetAlteraDistRams(bool bAlteraDistRams)
	{
		m_bAlteraDistRams = bAlteraDistRams;
	}
    void SetGenFixture(bool bGenFixture) {
        m_bGenFixture = bGenFixture;
    }
    void SetGenSandbox(bool bGenSandbox=true) {
        m_bGenSandbox = bGenSandbox;
    }

    string GetDesignPrefixString() { return m_designPrefixString; }
    string GetInputPathName() { return m_inputPathName; }
    string GetInputFileName() { return m_inputFileName; }
    string GetIncFileName() { return m_incFileName; }
    string GetVFileName() { return m_vFileName; }
    string GetCppFileName() { return m_cppFileName; }
    bool GetXilinxFdPrims() { return m_bXilinxFdPrims; }

    bool IsGenFixture() { return m_bGenFixture; }
    bool IsGenSandbox() { return m_bGenSandbox; }
    bool IsSynXilinxPrims() { return m_bSynXilinxPrims; }
	bool IsAlteraDistRams() { return m_bAlteraDistRams; }
    bool IsXilinxFdPrims() { return m_bXilinxFdPrims; }
	bool IsVivadoEnabled() { return m_bVivado; }
	bool IsQuartusEnabled() { return m_bQuartus; }

    bool IsLedaEnabled() { return m_bIsLedaEnabled; }
    void SetIsLedaEnabled() { m_bIsLedaEnabled = true; }

    bool IsPrmEnabled() { return m_bIsPrmEnabled; }
    void SetIsPrmEnabled() { m_bIsPrmEnabled = true; }

    bool IsKeepHierarchyEnabled() { return m_bKeepHierarchy && !m_bQuartus; }

private:
	int				m_argc;
	char **			m_argv;
    string          m_incFileName;
    string          m_cppFileName;
    string          m_vFileName;
    string          m_rptFileName;
    string          m_designPrefixString;
    string          m_inputPathName;
    string          m_inputFileName;
    bool	        m_bXilinxFdPrims;
    bool            m_bGenFixture;
    bool			m_bGenSandbox;
    bool            m_bSynXilinxPrims;
    bool			m_bIsLedaEnabled;
    bool			m_bIsPrmEnabled;
    bool            m_bKeepHierarchy;
	bool			m_bVivado;
	bool			m_bQuartus;
	bool			m_bAlteraDistRams;
};

extern CHtvArgs g_htvArgs;
