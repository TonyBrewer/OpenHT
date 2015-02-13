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

#include <stdarg.h>
#include <string>

using namespace std;

class HtStrFmt {
public:
	HtStrFmt(char const * pFmt, ... ) {
		va_list args;
		va_start( args, pFmt );
		char buf[4096];
		vsprintf(buf, pFmt, args);
		m_str = string(buf);
	}

	operator string() { return m_str; }
	char const * c_str() { return m_str.c_str(); }

private:
	string m_str;
};
