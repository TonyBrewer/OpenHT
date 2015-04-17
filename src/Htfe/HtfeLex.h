/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
// HtfeLex.h: interface for the CHtfeLex class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "HtfePreProcess.h"
#include "HtfeConstValue.h"

class CHtfeLex : protected CPreProcess
{
public:
	class CToken;
	typedef vector<CToken> CTokenList;
public:
	CHtfeLex();
	virtual ~CHtfeLex();

	enum EToken { tk_eof, tk_string, tk_identifier, tk_lbrace, tk_rbrace, tk_slash,
		tk_lparen, tk_rparen, tk_colon, tk_semicolon, tk_asterisk, tk_tilda, tk_comma,
		tk_lbrack, tk_rbrack, tk_class, tk_plus, tk_minus, tk_equal, tk_static,
		tk_private, tk_public, tk_struct, tk_friend, tk_return, tk_for, tk_union,
		tk_question, tk_new, tk_less, tk_greater, tk_period, tk_num_int, tk_num_float,
		tk_include, tk_define, tk_ampersand, tk_typedef, tk_vbar, tk_percent, tk_sc_signal,
		tk_bang, tk_carot, tk_sc_in, tk_sc_out, tk_sc_inout, tk_enum, tk_SC_CTOR,
		tk_SC_METHOD, tk_sensitive, tk_sensitive_pos, tk_sensitive_neg, tk_lessLess,
		tk_colonColon, tk_if, tk_else, tk_equalEqual, tk_switch, tk_case, tk_default,
		tk_break, tk_vbarVbar, tk_ampersandAmpersand, tk_bangEqual, tk_lessEqual,
		tk_greaterEqual, tk_num_hex, tk_greaterGreater, tk_num_string, tk_plusEqual,
		tk_minusEqual, tk_carotEqual, tk_vbarEqual, tk_ampersandEqual, tk_asteriskEqual,
		tk_slashEqual, tk_percentEqual, tk_tildaEqual, tk_lessLessEqual, tk_plusPlus,
		tk_greaterGreaterEqual, tk_minusMinus, tk_unaryMinus, tk_unaryPlus, tk_typeCast,
		tk_indirection, tk_addressOf, tk_exprBegin, tk_exprEnd, tk_minusGreater,
		tk_operator, tk_const, tk_do, tk_while, tk_preInc, tk_postInc, tk_preDec, tk_postDec,
		tk_try, tk_continue, tk_goto, tk_throw, tk_catch, tk_template, tk_namespace,
		tk_delete, tk_toe, tk_eor,
		TK_TOKEN_CNT };

	void OpenLine(const string &line) {
		if (m_bOpenLine) {
			ParseMsg(PARSE_ERROR, "Internal Error: nesting of Lex::OpenLine routine calls");
			return;
		}
		m_bOpenLine = true;

		m_lineBufSave = m_lineBuf;
		m_posSave = m_pPos - m_lineBuf.c_str();
		m_tokenSave = m_token;
		m_stringBufSave = m_stringBuf;

		m_lineBuf = line;
		m_pPos = m_lineBuf.c_str();
	}
	bool Open(const string &fileName);
	bool Reopen() {
		m_token = (CHtfeLex::EToken)-1;
		m_pPos = "";
		return CPreProcess::Reopen();
	}

	size_t RecordTokens();
    void RecordTokensPause();
    size_t RecordTokensResume();
    void RecordTokensErase(size_t pos);
	void RemoveLastRecordedToken();
	int GetTokenPlaybackLevel() { return m_tokenPlaybackLevel; }
    bool IsTokenPlayback() { return m_bTokenPlayback; }
    void FuncDeclPlayback();
    void FuncDeclPlaybackExit();
	void ForLoopTokenPlayback(string &ctrlVarName, bool bUnsignedCtrlVar, int64_t initValue, int64_t exitValue, int64_t incValue);
	void ForLoopExit();

	EToken GetNextToken();
	EToken GetNextToken2();
	EToken GetToken() { return m_token; }
	string &GetString() { return m_stringBuf; }
	CConstValue & GetConstValue();
	bool GetSint32ConstValue(int &value);
	//int64_t GetTokenValue(bool bAllowInt64=true);
	int GetInputFileOffset() {
		const char *pBegin = m_lineBuf.c_str();
		const char *pEnd = pBegin + m_lineBuf.size();
		if (pBegin <= m_pPos && m_pPos < pEnd)
			return CPreProcess::GetInputFileOffset(m_pPos - pBegin);
		else
			return CPreProcess::GetInputFileOffset(m_lineBuf.size());
	}
	string GetTokenString() {
		string tokenStr = m_tokenTbl[m_token].GetString();
		return (tokenStr.empty()) ? m_stringBuf : tokenStr;
	}
	static bool IsEqualOperator(EToken token) {
		string tkStr = GetTokenString(token);
		char last = tkStr[tkStr.length()-1];
		return (last == '=') && token != tk_lessEqual && token != tk_greaterEqual &&
			token != tk_equalEqual && token != tk_bangEqual;
	}
	static string &GetTokenString(EToken token) { return (*m_pTokenTbl)[token].GetString(); }
	static int GetTokenPrec(EToken token) { return (*m_pTokenTbl)[token].GetPrecedence(); }
	static bool IsTokenAssocLR(EToken token) { return (*m_pTokenTbl)[token].IsTokenAssocLR(); }
	static CLineInfo & GetLineInfo() { return m_lineInfo; }

private:
	const CLineInfo &GetLineInfoInternal() {
		if (m_bTokenPlayback)
			return m_playbackToken.GetLineInfo();

		return CPreProcess::GetFileLineInfo();
	}

public:

	struct CTokenInfo {
		void SetValue(int precedence, const char *pTokenStr) {
			m_precedence = precedence; m_tokenStr = pTokenStr;
		}
		string &GetString() { return m_tokenStr; }
		int GetPrecedence() { return m_precedence; }
		bool IsTokenAssocLR() { return m_precedence != 3 && m_precedence != 14 &&
			m_precedence != 15; }
		string m_tokenStr;
		int m_precedence;
	};
	typedef vector<CTokenInfo> CTokenTbl;

	class CToken {
	public:
		CToken() {}
		CToken(EToken token, string &s) { m_token = token; m_string = s; }
		CToken(EToken token) { m_token = token; }
		CToken(EToken token, string &s, const CLineInfo &lineInfo)
		{ m_token = token; m_string = s; m_lineInfo = lineInfo; }
		EToken GetToken() const { return m_token; }
		const string GetString() const {
			if (m_token == tk_string)
				return "\"" + m_string + "\"";
			else
				return m_string;
		}
		const string &GetRawString() const { return m_string; }
		const CLineInfo &GetLineInfo() { return m_lineInfo; }
	private:
		EToken m_token;
		string m_string;
		CLineInfo m_lineInfo;
	};

private:
	void SkipWhite();


private:
	string			m_lineBuf;
	EToken			m_token;
	string			m_stringBuf;
	const char *	m_pPos;

	bool			m_bOpenLine;
	string			m_lineBufSave;
	int				m_posSave;
	EToken			m_tokenSave;
	string			m_stringBufSave;
	CConstValue		m_constValue;

	CTokenTbl		m_tokenTbl;
	static CTokenTbl *m_pTokenTbl;

	/*********************************************************
	** Reserved word table
	*********************************************************/

	class CRsvd
	{
	public:
		CRsvd(const string &name) : m_name(name) {}
		void SetToken(EToken token) { m_token = token; }
		EToken GetToken() { return m_token; }
	private:
		string	m_name;
		EToken	m_token;
	};

	typedef map<string, CRsvd>			RsvdTblMap;
	typedef pair<string, CRsvd>			RsvdTblMap_ValuePair;
	typedef RsvdTblMap::iterator		RsvdTblMap_Iter;
	typedef pair<RsvdTblMap_Iter, bool>	RsvdTblMap_InsertPair;

	class CRsvdTbl  
	{
	public:
		CRsvd * Insert(const string &name) {
			RsvdTblMap_InsertPair insertPair;
			insertPair = m_signalTblMap.insert(RsvdTblMap_ValuePair(name, CRsvd(name)));
			return &insertPair.first->second;
		}
		EToken Find(const string &name) {
			RsvdTblMap_Iter iter = m_signalTblMap.find(name);
			if (iter == m_signalTblMap.end())
				return tk_identifier;
			CRsvd *pRsvd = &iter->second;
			return pRsvd ? pRsvd->GetToken() : tk_identifier;
		}
		int GetCount() { return m_signalTblMap.size(); }
	private:
		RsvdTblMap		m_signalTblMap;
	} m_rsvdTbl;

	/*********************************************************
	** Token recording
	*********************************************************/

private:
	struct CTokenRecord {
		CTokenRecord() {}

		int				m_value;
		int				m_exitValue;
		int				m_incValue;
		int				m_playbackPos;
		string			m_ctrlVarName;
		bool			m_bUnsignedCtrlVar;

		vector<CToken>		m_tokenList;
	};

private:
	bool					m_bTokenRecording;
	bool					m_bTokenPlayback;
	int						m_tokenPlaybackLevel;
	vector<CTokenRecord>	m_tokenRecord;
	CToken					m_playbackToken;

	bool					m_bRemovedToken;
	CToken					m_removedToken;

	static CLineInfo		m_lineInfo;

	FILE *					m_tkDumpFp;
};
