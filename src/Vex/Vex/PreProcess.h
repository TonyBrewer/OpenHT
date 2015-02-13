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

#define LINE_SIZE	1024

class CPreProcess
{
public:
	CPreProcess(const char *pFileName);
	~CPreProcess(void);

	bool GetLine(char *pLine, int maxLineLen);
	void AdvanceStatementFilePos();

public:
	enum EParseMsgType { PARSE_WARNING, PARSE_ERROR, PARSE_FATAL };
	enum EToken { tk_eof, tk_comma, tk_colon, tk_lparen, tk_rparen, tk_equal, tk_unknown,
		tk_period, tk_ident, tk_lbrack, tk_rbrack, tk_semicolon, tk_num, tk_string,
		tk_lbrace, tk_rbrace, tk_pound, tk_exprBegin, tk_minus, tk_unaryMinus, tk_plus,
		tk_unaryPlus, tk_bang, tk_tilda, tk_question, tk_vbar, tk_andReduce, tk_nandReduce,
		tk_orReduce, tk_norReduce, tk_xorReduce, tk_xnorReduce, tk_asterisk, tk_slash,
		tk_percent, tk_less, tk_lessEqual, tk_greater, tk_greaterEqual, tk_equalEqual,
		tk_bangEqual, tk_ampersand, tk_carot, tk_ampersandAmpersand, tk_vbarVbar, tk_lessLess, tk_exprEnd,
		TK_TOKEN_CNT};

public:
	struct CLineInfo {
		int m_lineNum;
		string m_fileName;
	};

protected:
	CLineInfo &GetLineInfo() { return m_lineInfo; }

	void ParseMsg(EParseMsgType msgType, const char *msgStr, ...);
	void ParseMsg(EParseMsgType msgType, const CLineInfo &lineInfo, const char *msgStr, ...);
	void ParseMsg(EParseMsgType msgType, const CLineInfo &lineInfo, const char *msgStr, va_list marker);

	int GetParseErrorCnt() { return m_parseErrorCnt; }

private:
	bool ExpandString(int &posIdx, const char *&pSrc, char *&pDst);
	bool ExpandString(const char *&pSrc, vector<string> &expandedList);
	int GetNum(const char *&pStr);
	void SkipToRBrace(const char *&pStr);

private:
	FILE			*m_fp;
	FILE			*m_pMsgFp;
	char			m_lineBuf[LINE_SIZE];
	CLineInfo		m_lineInfo;
	int				m_lineFilePos;
	int				m_statementFilePos;
	int				m_statementLineNum;
	size_t			m_expansionWidth;
	int				m_expansionIndex;
	bool			m_bRepositionToLineStart;
	bool			m_bCommentSpansEol;
	int				m_parseErrorCnt;
};
