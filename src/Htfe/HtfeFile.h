/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
// CppFile.h: interface for the CHtFile class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

class CHtFile  
{
public:
	CHtFile();
	virtual ~CHtFile() {}

	bool IsOpen() { return m_dstFp != 0; }
	bool Open(const string &outputFileName, char *pMode="wb");
	void Close();
	void Delete();
	void Dup(int srcFd, int startOffset, int endOffset);
	int Print(const char *format, ...);
	void SetDoNotSplitLine() { m_bDoNotSplitLine = true; }
	void Flush() { fflush(m_dstFp); }
	void SetIndentLevel(int lvl) { m_indentLevel = lvl; }
	int GetIndentLevel() { return m_indentLevel; }
	void IncIndentLevel() { m_indentLevel += 1; }
	void DecIndentLevel() { m_indentLevel -= 1; }
	string &GetFolder() { return m_folder; }
    void SetMaxLineCol(int maxLineCol) { m_maxLineCol = maxLineCol; }
    int GetMaxLineCol() { return m_maxLineCol; }
	int GetLineNum() { return m_lineNum; }
	void SetLineBuffer(int lineListIdx) { m_lineListIdx = lineListIdx; }
	void SetLineBuffering(bool bEnable) { m_bLineBuffering = bEnable; }
	bool GetLineBuffering() { return m_bLineBuffering; }
	void FlushLineBuffer();

	void SetHtPrimLines(bool bPrimBuffering) {
		if (bPrimBuffering) {
			SetLineBuffer(1);
			m_prePrimLevel = GetIndentLevel();
			SetIndentLevel(1);
		} else {
			SetLineBuffer(0);
			SetIndentLevel(m_prePrimLevel);
		}
	}

	void SetVarDeclLines(bool bVarDeclLines) {
		SetLineBuffering(!bVarDeclLines);
		if (bVarDeclLines) {
			m_preVarDeclLevel = GetIndentLevel();
			SetIndentLevel(1);
		} else {
			SetIndentLevel(m_preVarDeclLevel);
		}
	}

	void Putc(char ch);
	void Puts(char * pStr);

private:
	FILE    *m_dstFp;
	int		m_preVarDeclLevel;
	int		m_prePrimLevel;
	int     m_indentLevel;
	bool    m_bNewLine;
	bool	m_bDoNotSplitLine;
	int		m_lineNum;
	int     m_lineCol;
    int     m_maxLineCol;
	string	m_fileName;
	string  m_folder;
	bool			m_bLineBuffering;
	string			m_lineBuffer;
	int				m_lineListIdx;
	vector<string>	m_lineList[2];
};
