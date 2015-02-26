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

class CGenHtmlRpt {
public:
	CGenHtmlRpt(string htlPath, string fileName, int argc, char const **argv);
	~CGenHtmlRpt();

	void AddLevel(const char *pFormat, ...);
	void AddItem(const char *pFormat, ...);
	void AddText(const char *pFormat, ...);
	void EndLevel();

	FILE * GetFp() { return m_fp; }

	void GenApiHdr();
	void GenApiEnd();
	void GenCallGraph();

protected:
	FILE * m_fp;
	string m_indent;
	bool m_bPendingIndent;
	bool m_bPendingText;
	int m_maxBoldLevel;
	int m_indentLevel;
	char m_pendingTextStr[1024];
	string m_htmlPath;
};
