/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
// HttArgs.h: interface for the ScArgs class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "HtfeArgs.h"

class CHttArgs : public CHtfeArgs
{
public:
    CHttArgs();
    virtual ~CHttArgs();

    void Usage(const char *progname);
    void Parse(int argc, char *argv[]);

	void GetArgs(int & argc, char ** &argv) {
		argc = m_argc; argv = m_argv;
	}

    void SetGenSandbox(bool bGenSandbox=true) {
        m_bGenSandbox = bGenSandbox;
    }

    string GetInputPathName() { return m_inputPathName; }
    string GetInputFileName() { return m_inputFileName; }
    string GetIncOutFileName() { return m_incOutFileName; }
    string GetCppOutFileName() { return m_cppOutFileName; }

    bool IsGenSandbox() { return m_bGenSandbox; }

private:
	int				m_argc;
	char **			m_argv;
    string          m_inputPathName;
    string          m_inputFileName;
    string          m_incOutFileName;
    string          m_cppOutFileName;
    bool			m_bGenSandbox;
};

extern CHttArgs g_httArgs;
