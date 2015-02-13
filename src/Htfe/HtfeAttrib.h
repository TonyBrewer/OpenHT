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

/*********************************************************
** HtAttrib
*********************************************************/

struct CHtAttrib {
    CHtAttrib(string &name, string &inst, int msb, int lsb, string &value, CLineInfo &lineInfo) 
		: m_name(name), m_inst(inst), m_msb(msb), m_lsb(lsb), m_value(value), m_lineInfo(lineInfo) {}

    string m_name;
    string m_inst;
	int	m_msb;
	int m_lsb;
    string m_value;
    CLineInfo m_lineInfo;
};
