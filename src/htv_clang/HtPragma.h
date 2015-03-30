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

class HtPragma {
public:
	enum HtPragmaType { HtPrim, HtClk, HtTpp, ScModule, ScCtor, HtTps, HtTpe, ScMethod };

public:
	HtPragma(HtPragmaType type) : m_pragmaType(type) {}

	void addArgString(string & str) {
		m_argStrings.push_back(str);
	}

	vector<string> const & getArgStrings() { return m_argStrings; }

	bool isHtPrim() { return m_pragmaType == HtPrim; }
	bool isTpp() { return m_pragmaType == HtTpp; }
	bool isScModule() { return m_pragmaType == ScModule; }
	bool isScCtor() { return m_pragmaType == ScCtor; }
	bool isScMethod() { return m_pragmaType == ScMethod; }
	bool isTps() { return m_pragmaType == HtTps; }
	bool isTpe() { return m_pragmaType == HtTpe; }

private:
	HtPragmaType m_pragmaType;
	vector<string> m_argStrings;
};
