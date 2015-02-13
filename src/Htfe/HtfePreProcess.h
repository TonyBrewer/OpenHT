/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
// HtfePreProcess.h: interface for the CPreProcess class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#ifdef _WIN32
#pragma warning(disable: 4786) 
#endif

#include "HtfeErrorMsg.h"
#include "HtfeLineInfo.h"

#define MAX_FILE_STACK	32
#define RDBUF_SIZE 4096

#define BREAK_LINE_NUM 667

class CPreProcess  
{
	class CDefine;
public:
	CPreProcess();
	virtual ~CPreProcess();

	bool Open(const string &name, bool bProcessOnce=false);
	void InsertPreProcessorName(const string &name) { m_defineTbl.Insert(name.c_str()); }
	bool Reopen();
	void Close();
	bool GetLine(string &line);
	//virtual const CLineInfo &GetLineInfo() = 0;
	const CLineInfo &GetFileLineInfo() {
		if (m_pFs < m_fileStack)
			return m_fileStack->m_lineInfo;
		else
			return m_pFs->m_lineInfo;
	}
	bool IsScTopology() { return m_pFs->m_lineInfo.m_pathName.substr(m_pFs->m_lineInfo.m_pathName.size()-3) == ".sc"; }
	bool IsInputFile() { return m_pFs == m_fileStack; }
	int GetInputFd() { return m_pFs->m_fd; }
	int GetInputFileOffset(int linePos) {
		return m_linePos.GetInputFileOffset(linePos);
	}
	void VerifyFileOffset(int start, const string &lineBuf);
	string FormatString(const char *msgStr, ...);
 	void ParseMsg(EErrorMsgType msgType, const char *msgStr, ...);
    void ParseMsg(EErrorMsgType msgType, const CLineInfo &lineInfo, const char *msgStr, ...);

	void WritePreProcessedInput() {
		m_pWriteFp = fopen("PreProcessed.txt", "w");
	}

    void SetInterceptFileName(string interceptFileName) { m_interceptFileName = interceptFileName; }
    void SetReplaceFileName(string replaceFileName) { m_replaceFileName = replaceFileName; }

private:
	enum EToken2 { tk_exprBegin, tk_bang, tk_num_int, tk_num_hex,
		tk_lparen, tk_rparen, tk_plus,
		tk_minus, tk_slash, tk_asterisk, tk_percent, tk_equalEqual, tk_vbar,
		tk_vbarVbar, tk_carot, tk_ampersand, tk_ampersandAmpersand, tk_bangEqual,
		tk_less, tk_greater, tk_greaterEqual, tk_lessEqual, tk_lessLess,
		tk_greaterGreater, tk_unaryPlus, tk_unaryMinus, tk_tilda,
		tk_identifier, tk_exprEnd, tk_error };
	EToken2 m_tk;
	string m_tkString;

private:
	void PerformMacroExpansion(string &lineBuf);
	void ExpandMacro(CDefine *pMacro, vector<string> &argList, string &expansion);
	void PreProcessDefine(string &lineBuf, const char *&pPos);
	void PreProcessUndef(string &lineBuf, const char *&pPos);
	void PreProcessInclude(string &lineBuf, const char *&pPos);
	bool PreProcessIf(string &lineBuf, const char *&pPos);
	bool PreProcessIfdef(string &lineBuf, const char *&pPos);
	bool PreProcessIfndef(string &lineBuf, const char *&pPos);
	bool GetLineWithCommentsStripped(string &line, bool bContinue=false);
	bool GetLineFromFile(string &lineBuf, bool bAppendLine=false);
	void SkipSpace(const char *&pPos);
	string ParseIdentifier(const char *&pPos);
	string ParseString(const char *&pPos);

	bool ParseExpression(const char *&pPos, int &rtnValue);
	void EvaluateExpression(EToken2 tk, vector<int> &operandStack,
		vector<int> &operatorStack);
	EToken2 GetNextToken(const char *&pPos);
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
		CDefine *m_pDefine;
		int m_paramCnt;

		// conditional directive state
		vector<bool> m_bSkipStack;
		vector<bool> m_bElseStack;
		vector<bool> m_bSkipToEndif;
	} m_fileStack[MAX_FILE_STACK];

	bool m_bPreProc;
	bool m_bInComment;
	CFileStack *m_pFs;
	int m_fileCnt;
	bool m_bSkipLines;
	FILE *m_pWriteFp;
	vector <string>	m_pragmaOnceList;

	int m_parseErrorCnt;
	int m_parseWarningCnt;

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
		void SetBaseOffset(int baseOffset) {
			m_baseOffset = baseOffset; m_deltaList.resize(0);
		}
		bool IsInFile(int linePos) {
			// check of char in line was in original file
			CDeltaList::iterator deltaIter;
			for (deltaIter = m_deltaList.begin(); deltaIter < m_deltaList.end(); deltaIter++) {
				if (deltaIter->m_linePos <= linePos &&
					linePos < deltaIter->m_linePos - deltaIter->m_delta)
					return false;
			}
			return true;
		}

		void Insert(int linePos, int length) {
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
		void Delete(int linePos, int length) {
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
		int GetInputFileOffset(int linePos) {
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
	enum EDirective { dir_noMatch, dir_define, dir_include, dir_if,
		dir_ifdef, dir_ifndef, dir_undef, dir_elif, dir_else, dir_endif,
		dir_pragma };

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

	class CDirectiveTbl  
	{
	public:
		CDirective * Insert(const string &name) {
			DirectiveTblMap_InsertPair insertPair;
			insertPair = m_directiveTblMap.insert(DirectiveTblMap_ValuePair(name, CDirective(name)));
			return &insertPair.first->second;
		}
		EDirective Find(const string &name) {
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

	/*********************************************************
	** Param table
	*********************************************************/

	class CParam {
	public:
		CParam(const string &name) : m_name(name) { m_paramId = -1; }
		string &GetName() { return m_name; }
		int GetParamId() const 
		{ return m_paramId; }
		void SetParamId(int paramId)
		{ m_paramId = paramId; }

	private:
		string	m_name;
		int       m_paramId;
	};

	typedef map<string, CParam>				ParamTblMap;
	typedef pair<string, CParam>			ParamTblMap_ValuePair;
	typedef ParamTblMap::iterator			ParamTblMap_Iter;
	typedef pair<ParamTblMap_Iter, bool>	ParamTblMap_InsertPair;

	class CParamTbl  
	{
	public:
		CParamTbl() { m_bVariadic = false; }

		CParam * Insert(const string &name) {
			ParamTblMap_InsertPair insertPair;
			insertPair = m_paramTblMap.insert(ParamTblMap_ValuePair(name, CParam(name)));
			m_bVariadic |= name == "__VA_ARGS__";
			return &insertPair.first->second;
		}
		CParam * Find(const string &name) {
			ParamTblMap_Iter iter = m_paramTblMap.find(name);
			if (iter == m_paramTblMap.end())
				return 0;
			return &iter->second;
		}
		int GetCount() { return m_paramTblMap.size(); }
		bool IsVariadic() { return m_bVariadic; }
	private:
		ParamTblMap		m_paramTblMap;
		bool			m_bVariadic;
	};

	/*********************************************************
	** Define table
	*********************************************************/

	class CDefineTbl;
	class CDefine
	{
	public:
		CDefine(string &str) : m_name(str) { m_bIsNew = true; m_bParenReqd = false; }
		string &GetExpansion()
			{ return m_expansion; }
		void SetExpansion(string &expansion)
			{ m_expansion = expansion; }
		bool InsertParam(string &name, int paramId) {
			CParam * pParam = m_paramTbl.Insert(name);
			if (pParam->GetParamId() >= 0)
				return false;
			pParam->SetParamId(paramId);
			return true;
		}
		CParam *FindParam(const string &name)
			{ return dynamic_cast<CParam*>(m_paramTbl.Find(name)); }
		int GetParamCnt() { return m_paramTbl.GetCount(); }
		bool IsVariadicArg(int argId) { return m_paramTbl.IsVariadic() && m_paramTbl.GetCount()-1 == argId; }
		bool IsNew() { return m_bIsNew; }
		void SetIsNew(bool bIsNew) { m_bIsNew = bIsNew; }
		bool IsParenReqd() { return m_bParenReqd; }
		void SetIsParenReqd(bool bParenReqd) { m_bParenReqd = bParenReqd; }
		void SetLineInfo(const CLineInfo &lineInfo) { m_lineInfo = lineInfo; }
		const CLineInfo & GetLineInfo() { return m_lineInfo; }

	private:
		string		m_name;
		string		m_expansion;
		CParamTbl   m_paramTbl;
		bool		m_bIsNew;
		bool		m_bParenReqd;
		CLineInfo	m_lineInfo;
	};

	typedef map<string, CDefine>		DefineMap;
	typedef pair<string, CDefine>		DefineMap_ValuePair;
	typedef DefineMap::iterator			DefineMap_Iter;
	typedef pair<DefineMap_Iter, bool>	DefineMap_InsertPair;

	class CDefineTbl  
	{
	public:
		CDefine * Insert(const char *pName)
		{
			string name;
			string value;
			string str = string(pName);
			size_t eqPos = str.find('=');
			if (eqPos != string::npos) {
				name = str.substr(0, eqPos);
				value = str.substr(eqPos+1);
			} else
				name = str;

			DefineMap_InsertPair insertPair;
			insertPair = m_defineMap.insert(DefineMap_ValuePair(name, CDefine(name)));
			insertPair.first->second.SetExpansion(value);

			return &insertPair.first->second;
		}
		CDefine * Insert(string &str)
		{
			DefineMap_InsertPair insertPair;
			insertPair = m_defineMap.insert(DefineMap_ValuePair(str, CDefine(str)));
			return &insertPair.first->second;
		}
		void Remove(string &str) {
			DefineMap_Iter iter = m_defineMap.find(str);
			if (iter == m_defineMap.end())
				return;
			m_defineMap.erase(iter);
		}
		CDefine * Find(const string &str) {
			DefineMap_Iter iter = m_defineMap.find(str);
			if (iter == m_defineMap.end())
				return 0;
			return &iter->second;
		}

	private:
		DefineMap		m_defineMap;

	} m_defineTbl;
};
