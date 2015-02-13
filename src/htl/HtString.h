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

#include "CnyHt.h"

class CDefineTable;

struct CHtString {

	CHtString() { m_bIsValid = false; }
	CHtString(string s) : m_string(s) { m_bIsValid = false; }

	operator string & () { return m_string; }

	static void SetDefineTable(CDefineTable * pDefineTable) { m_pDefineTable = pDefineTable; }

	void operator = (string a) { m_string = a; }
	bool operator == (string a) { return m_string == a; }
	bool operator == (CHtString a) { return m_string == a.m_string; }
	bool operator != (CHtString a) { return m_string != a.m_string; }
	void operator += (char a) { m_string += " "; m_string[m_string.size()-1] = a; }
	char operator [] (size_t idx) { return m_string[idx]; }

	size_t size() const { return m_string.size(); }
	const char * c_str() { return m_string.c_str(); }

	string & AsStr() { return m_string; }
	int AsInt() const { HtlAssert(m_bIsValid); return m_value; }

	bool IsValid() { return m_bIsValid; }
	void InitValue( CLineInfo const &lineInfo, bool bRequired=true, int defValue=0, bool bIsSigned=true );
	void SetValue(int value) { m_bIsValid = true; m_value = value; }

	string Upper() const {
		string tmp = m_string;
		for (size_t i = 0; i < tmp.size(); i += 1)
			tmp[i] = toupper(tmp[i]);
		return tmp;
	}
	string Uc() const {
		string tmp = m_string;
		if (tmp.size() > 0) tmp[0] = toupper(tmp[0]);
		return tmp;
	}
	string Lc() const {
		string tmp = m_string;
		if (tmp.size() > 0) tmp[0] = tolower(tmp[0]);
		return tmp;
	}

private:
	static CDefineTable	*m_pDefineTable;

private:
	string		m_string;
	bool		m_bIsValid;
	int			m_value;
	bool		m_bIsSigned;
};
