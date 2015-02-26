/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
// PreProcess.h: interface for the CPreProcess class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#ifdef _WIN32
#pragma warning(disable: 4786) 
#endif

#include <assert.h>
#include "LineInfo.h"
#include "MtRand.h"

#define MAX_FILE_STACK	32
#define RDBUF_SIZE 4096

#define BREAK_LINE_NUM 667

enum EMsgType { Info, Warning, Error, Fatal };

void ErrorMsg(EMsgType msgType, CLineInfo const & lineInfo, const char *pFormat, va_list & args);

struct CIncludeFile {
	CIncludeFile(string fullPath, string name) : m_fullPath(fullPath), m_name(name) {}
	string m_fullPath;
	string m_name;
};

class CPreProcess {
public:
	class CMacro;
public:
	CPreProcess();
	virtual ~CPreProcess();

	bool Open(const string &name, bool bProcessOnce = false);
	void InsertPreProcessorName(const string &name, const string &value) { m_macroTbl.Insert(name, value); }
	bool Reopen();
	void Close();
	bool GetLine(string &line);
	const CLineInfo &GetLineInfo()
	{
		if (m_pFs < m_fileStack)
			return m_fileStack->m_lineInfo;
		else
			return m_pFs->m_lineInfo;
	}
	vector<CIncludeFile> const & GetIncludeList() const { return m_includeList; }
	size_t GetMacroTblSize() { return m_macroTbl.size(); }
	//size_t GetMacroTbl() { return m_macroTbl.size(); }
	void UpdateStaticLineInfo() { m_lineInfo = GetLineInfo(); }
	bool IsScTopology() { return m_pFs->m_lineInfo.m_pathName.substr(m_pFs->m_lineInfo.m_pathName.size() - 3) == ".sc"; }
	bool IsInputFile() { return m_pFs == m_fileStack; }
	int GetFd() { return m_pFs->m_fd; }
	int GetFileOffset(int linePos)
	{
		return m_linePos.GetFileOffset(linePos);
	}
	void VerifyFileOffset(int start, const string &lineBuf);
	string FormatString(const char *msgStr, ...);

	void WritePreProcessedInput()
	{
		m_pWriteFp = fopen("PreProcessed.txt", "w");
	}

	void SetInterceptFileName(string interceptFileName) { m_interceptFileName = interceptFileName; }
	void SetReplaceFileName(string replaceFileName) { m_replaceFileName = replaceFileName; }

public:

	static CLineInfo m_lineInfo;
	static int m_warningCnt;
	static int m_errorCnt;
	static MTRand_int32	m_mtRand;

	void static ParseMsg(EMsgType msgType, const char *pFormat, ...)
	{
		va_list args;
		va_start(args, pFormat);

		ErrorMsg(msgType, CPreProcess::m_lineInfo, pFormat, args);
	}

	void static ParseMsg(EMsgType msgType, CLineInfo const & lineInfo, const char *pFormat, ...)
	{
		va_list args;
		va_start(args, pFormat);

		ErrorMsg(msgType, lineInfo, pFormat, args);
	}

private:
	enum EToken2 {
		tk_exprBegin, tk_bang, tk_num_int, tk_num_hex,
		tk_lparen, tk_rparen, tk_plus,
		tk_minus, tk_slash, tk_asterisk, tk_percent, tk_equalEqual, tk_vbar,
		tk_vbarVbar, tk_carot, tk_ampersand, tk_ampersandAmpersand, tk_bangEqual,
		tk_less, tk_greater, tk_greaterEqual, tk_lessEqual, tk_lessLess,
		tk_greaterGreater, tk_unaryPlus, tk_unaryMinus, tk_tilda,
		tk_identifier, tk_exprEnd, tk_error
	};
	EToken2 m_tk;
	string m_tkString;

private:
	void PerformMacroExpansion(string &lineBuf);
	void ExpandMacro(CMacro *pMacro, vector<string> &argList, string &expansion);
	void PreProcessDefine(string &lineBuf, const char *&pPos);
	void PreProcessUndef(string &lineBuf, const char *&pPos);
	void PreProcessInclude(string &lineBuf, const char *&pPos);
	bool PreProcessIf(string &lineBuf, const char *&pPos);
	bool PreProcessIfdef(string &lineBuf, const char *&pPos);
	bool PreProcessIfndef(string &lineBuf, const char *&pPos);
	bool GetLineWithCommentsStripped(string &line, bool bContinue = false);
	bool GetLineFromFile(string &lineBuf, bool bAppendLine = false);
	void SkipSpace(const char *&pPos);
	string ParseIdentifier(const char *&pPos);
	string ParseString(const char *&pPos);

	bool ParseExpression(const char *&pPos, int &rtnValue, bool bErrMsg = true);
	void EvaluateExpression(EToken2 tk, vector<int> &operandStack,
		vector<int> &operatorStack);
	EToken2 GetNextToken(const char *&pPos, bool bErrMsg = true);
	EToken2 GetToken() { return m_tk; }
	string &GetTokenString() { return m_tkString; }
	int GetTokenValue();
	int GetTokenPrec(EToken2 tk);

private:
	struct CFileStack {
		CLineInfo m_lineInfo;
		int m_fd;
		char m_rdBuf[RDBUF_SIZE];
		const char *m_pRdBufPos;
		const char *m_pRdBufEnd;
		int m_fileRdOffset;
		CMacro *m_pDefine;
		int m_paramCnt;

		// conditional directive state
		vector<bool> m_bSkipStack;
		vector<bool> m_bElseStack;
		vector<bool> m_bSkipToEndif;
	} m_fileStack[MAX_FILE_STACK];

	int m_fsIdx;
	vector<CIncludeFile> m_includeList;	// list of first level include files

	bool m_bPreProc;
	bool m_bInComment;
	CFileStack *m_pFs;
	int m_fileCnt;
	bool m_bSkipLines;
	FILE *m_pWriteFp;
	vector <string>	m_pragmaOnceList;

	struct CLinePosDelta {
		CLinePosDelta() {}
		CLinePosDelta(int linePos, int delta) : m_linePos(linePos), m_delta(delta) {}

		int m_linePos;
		int m_delta;
	};

	string m_interceptFileName;
	string m_replaceFileName;

	class CLinePos {
	public:
		typedef vector<CLinePosDelta> CDeltaList;
		CLinePos() { m_baseOffset = 0; m_deltaList.resize(0); }
		void SetBaseOffset(int baseOffset)
		{
			m_baseOffset = baseOffset; m_deltaList.resize(0);
		}
		bool IsInFile(int linePos)
		{
			// check of char in line was in original file
			CDeltaList::iterator deltaIter;
			for (deltaIter = m_deltaList.begin(); deltaIter < m_deltaList.end(); deltaIter++) {
				if (deltaIter->m_linePos <= linePos &&
					linePos < deltaIter->m_linePos - deltaIter->m_delta)
					return false;
			}
			return true;
		}

		void Insert(int linePos, int length)
		{
			// insert in ordered list, update remaining entries
			bool foundFirst = false;
			CDeltaList::iterator deltaIter;
			CDeltaList::iterator newDeltaPos = m_deltaList.end();
			for (deltaIter = m_deltaList.begin(); deltaIter < m_deltaList.end(); deltaIter++) {
				if (deltaIter->m_linePos < linePos)
					continue;
				if (!foundFirst) {
					foundFirst = true;
					newDeltaPos = deltaIter;
				}
				deltaIter->m_linePos += length;
			}
			newDeltaPos = m_deltaList.insert(newDeltaPos, CLinePosDelta(linePos, -length));
		}
		void Delete(int linePos, int length)
		{
			// insert in ordered list, update remaining entries
			bool foundFirst = false;
			CDeltaList::iterator deltaIter;
			CDeltaList::iterator newDeltaPos = m_deltaList.end();
			for (deltaIter = m_deltaList.begin(); deltaIter < m_deltaList.end(); deltaIter++) {
				if (deltaIter->m_linePos < linePos)
					continue;
				if (!foundFirst) {
					foundFirst = true;
					newDeltaPos = deltaIter;
				}
				deltaIter->m_linePos -= length;
			}
			newDeltaPos = m_deltaList.insert(newDeltaPos, CLinePosDelta(linePos, length));
		}
		int GetFileOffset(int linePos)
		{
			int offset = m_baseOffset + linePos;
			CDeltaList::iterator deltaIter;
			for (deltaIter = m_deltaList.begin(); deltaIter < m_deltaList.end(); deltaIter++) {
				if (linePos >= deltaIter->m_linePos)
					offset += deltaIter->m_delta;
				else
					break;
			}
			return offset;
		}

	private:
		int m_baseOffset;
		CDeltaList m_deltaList;
	} m_linePos;

	/*********************************************************
	** Directive table
	*********************************************************/

	// Directives table
	enum EDirective {
		dir_noMatch, dir_define, dir_include, dir_if,
		dir_ifdef, dir_ifndef, dir_undef, dir_elif, dir_else, dir_endif,
		dir_pragma
	};

	class CDirective {
	public:
		CDirective(const string &name) : m_name(name) {}
		EDirective GetDirective() { return m_directive; }

		string &GetName() { return m_name; }
		CDirective* SetDirective(EDirective directive) { m_directive = directive; return this; }

	private:
		string	m_name;
		EDirective m_directive;
	};

	typedef map<string, CDirective>				DirectiveTblMap;
	typedef pair<string, CDirective>			DirectiveTblMap_ValuePair;
	typedef DirectiveTblMap::iterator			DirectiveTblMap_Iter;
	typedef pair<DirectiveTblMap_Iter, bool>	DirectiveTblMap_InsertPair;

	class CDirectiveTbl {
	public:
		CDirective * Insert(const string &name)
		{
			DirectiveTblMap_InsertPair insertPair;
			insertPair = m_directiveTblMap.insert(DirectiveTblMap_ValuePair(name, CDirective(name)));
			return &insertPair.first->second;
		}
		EDirective Find(const string &name)
		{
			DirectiveTblMap_Iter iter = m_directiveTblMap.find(name);
			if (iter == m_directiveTblMap.end())
				return dir_noMatch;
			CDirective *pDir = &iter->second;
			return pDir == 0 ? dir_noMatch : pDir->GetDirective();
		}
		int GetCount() { return m_directiveTblMap.size(); }
	private:
		DirectiveTblMap		m_directiveTblMap;
	} m_directiveTbl;

public:
	/*********************************************************
	** Macro table
	*********************************************************/
	class CMacro {
	public:
		CMacro(string &str) : m_name(str) { m_bIsNew = true; m_bParenReqd = false; m_bIsFromIncludeFile = true; }
		CMacro(string const &name, string const &expansion) : m_name(name), m_expansion(expansion)
		{
			m_bIsNew = true; m_bParenReqd = false; m_bIsFromIncludeFile = true;
		}

		string const &GetName() const { return m_name; }
		vector<string> const & GetParamList() const { return m_paramList; }
		string const &GetExpansion() const { return m_expansion; }
		void SetExpansion(string &expansion) { m_expansion = expansion; }
		bool InsertParam(string &name)
		{
			if (FindParamId(name) >= 0)
				return false;
			m_paramList.push_back(name);
			return true;
		}
		int FindParamId(string const & name)
		{
			for (size_t i = 0; i < m_paramList.size(); i += 1)
				if (m_paramList[i] == name)
					return i;
			return -1;
		}
		int GetParamCnt() const { return (int)m_paramList.size(); }
		string const &GetParam(int i) const { return m_paramList[i]; }
		bool IsNew() { return m_bIsNew; }
		void SetIsNew(bool bIsNew) { m_bIsNew = bIsNew; }
		bool IsParenReqd() const { return m_bParenReqd; }
		void SetIsParenReqd(bool bParenReqd) { m_bParenReqd = bParenReqd; }
		bool IsFromIncludeFile() const { return m_bIsFromIncludeFile; }
		void SetIsFromIncludeFile(bool bIsFromIncludeFile) { m_bIsFromIncludeFile = bIsFromIncludeFile; }
		void SetLineInfo(const CLineInfo &lineInfo) { m_lineInfo = lineInfo; }
		CLineInfo const & GetLineInfo() const { return m_lineInfo; }

	private:
		string		m_name;
		string		m_expansion;
		vector<string> m_paramList;
		bool		m_bIsNew;
		bool		m_bParenReqd;
		bool		m_bIsFromIncludeFile;
		CLineInfo	m_lineInfo;
	};

public:
	typedef map<string, CMacro>		    MacroMap;
	typedef pair<string, CMacro>		MacroMap_ValuePair;
	typedef MacroMap::iterator			MacroMap_Iter;
	typedef pair<MacroMap_Iter, bool>	MacroMap_InsertPair;

	class CDefineTbl {
	public:
		//CMacro * Insert(const char *pName)
		//{
		//	string name;
		//	string value;
		//	string str = string(pName);
		//	size_t eqPos = str.find('=');
		//	if (eqPos != string::npos) {
		//		name = str.substr(0, eqPos);
		//		value = str.substr(eqPos+1);
		//	} else
		//		name = str;

		//	MacroMap_InsertPair insertPair;
		//	insertPair = m_macroMap.insert(MacroMap_ValuePair(name, CMacro(name)));
		//	insertPair.first->second.SetExpansion(value);

		//	return &insertPair.first->second;
		//}
		CMacro * Insert(string str)
		{
			MacroMap_InsertPair insertPair;
			insertPair = m_macroMap.insert(MacroMap_ValuePair(str, CMacro(str)));
			return &insertPair.first->second;
		}
		CMacro * Insert(string const name, string const value)
		{
			MacroMap_InsertPair insertPair;
			insertPair = m_macroMap.insert(MacroMap_ValuePair(name, CMacro(name, value)));
			return &insertPair.first->second;
		}
		void Remove(string &str)
		{
			MacroMap_Iter iter = m_macroMap.find(str);
			if (iter == m_macroMap.end())
				return;
			m_macroMap.erase(iter);
		}
		CMacro * Find(const string &str)
		{
			MacroMap_Iter iter = m_macroMap.find(str);
			if (iter == m_macroMap.end())
				return 0;
			return &iter->second;
		}
		size_t size() { return m_macroMap.size(); }
		MacroMap_Iter begin() { return m_macroMap.begin(); }
		MacroMap_Iter end() { return m_macroMap.end(); }

	private:
		MacroMap		m_macroMap;

	};

private:
	CDefineTbl m_macroTbl;

public:
	CDefineTbl & GetMacroTbl() { return m_macroTbl; }
};
