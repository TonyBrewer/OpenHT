/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "stdafx.h"
#include "Lex.h"

CLex::CTokenTbl CLex::m_tokenTbl;

CLex::CLex(const char *pFileName) : CPreProcess(pFileName)
{
	m_pLine = m_lineBuf;
	m_pLine = 0;
	m_bSingleLine = false;

	GetLine();

	m_tokenTbl.resize(TK_TOKEN_CNT);
	m_tokenTbl[tk_lparen].SetValue(1, "(");
	//m_tokenTbl[tk_period].SetValue(1, ".");
	//m_tokenTbl[tk_minusGreater].SetValue(1, "->");
	//m_tokenTbl[tk_colonColon].SetValue(1, "::");
	//m_tokenTbl[tk_plusPlus].SetValue(2, "++");
	//m_tokenTbl[tk_minusMinus].SetValue(2, "--");
	//m_tokenTbl[tk_typeCast].SetValue(2, "");
	//m_tokenTbl[tk_new].SetValue(2, "new");
	//m_tokenTbl[tk_indirection].SetValue(3, "*");
	//m_tokenTbl[tk_addressOf].SetValue(3, "&");
	m_tokenTbl[tk_unaryPlus].SetValue(3, "+");
	m_tokenTbl[tk_unaryMinus].SetValue(3, "-");
	m_tokenTbl[tk_tilda].SetValue(3, "~");
	m_tokenTbl[tk_bang].SetValue(3, "!");
	m_tokenTbl[tk_andReduce].SetValue(3, "&");
	m_tokenTbl[tk_nandReduce].SetValue(3, "~&");
	m_tokenTbl[tk_orReduce].SetValue(3, "|");
	m_tokenTbl[tk_norReduce].SetValue(3, "~|");
	m_tokenTbl[tk_xorReduce].SetValue(3, "^");
	m_tokenTbl[tk_xnorReduce].SetValue(3, "~^");
	m_tokenTbl[tk_asterisk].SetValue(4, "*");
	m_tokenTbl[tk_slash].SetValue(4, "/");
	m_tokenTbl[tk_percent].SetValue(4, "%");
	m_tokenTbl[tk_plus].SetValue(5, "+");
	m_tokenTbl[tk_minus].SetValue(5, "-");
	m_tokenTbl[tk_lessLess].SetValue(6, "<<");
	//m_tokenTbl[tk_greaterGreater].SetValue(6, ">>");
	m_tokenTbl[tk_less].SetValue(7, "<");
	m_tokenTbl[tk_lessEqual].SetValue(7, "<=");
	m_tokenTbl[tk_greater].SetValue(7, ">");
	m_tokenTbl[tk_greaterEqual].SetValue(7, ">=");
	m_tokenTbl[tk_equalEqual].SetValue(8, "==");
	m_tokenTbl[tk_bangEqual].SetValue(8, "!=");
	m_tokenTbl[tk_ampersand].SetValue(9, "&");
	m_tokenTbl[tk_carot].SetValue(10, "^");
	m_tokenTbl[tk_vbar].SetValue(11, "|");
	m_tokenTbl[tk_ampersandAmpersand].SetValue(12, "&&");
	m_tokenTbl[tk_vbarVbar].SetValue(13, "||");
	m_tokenTbl[tk_question].SetValue(14, "?");
	m_tokenTbl[tk_equal].SetValue(15, "=");
	//m_tokenTbl[tk_plusEqual].SetValue(15, "+=");
	//m_tokenTbl[tk_minusEqual].SetValue(15, "-=");
	//m_tokenTbl[tk_asteriskEqual].SetValue(15, "*=");
	//m_tokenTbl[tk_slashEqual].SetValue(15, "/=");
	//m_tokenTbl[tk_percentEqual].SetValue(15, "%=");
	//m_tokenTbl[tk_greaterGreaterEqual].SetValue(15, ">>=");
	//m_tokenTbl[tk_lessLessEqual].SetValue(15, "<<=");
	//m_tokenTbl[tk_ampersandEqual].SetValue(15, "&=");
	//m_tokenTbl[tk_carotEqual].SetValue(15, "^=");
	//m_tokenTbl[tk_tildaEqual].SetValue(15, "~=");
	//m_tokenTbl[tk_vbarEqual].SetValue(15, "|=");
	m_tokenTbl[tk_comma].SetValue(16, ",");
	m_tokenTbl[tk_exprBegin].SetValue(17, "");
	m_tokenTbl[tk_exprEnd].SetValue(18, "");
	//m_tokenTbl[tk_class].SetValue(-1, "class");
	m_tokenTbl[tk_lbrace].SetValue(-1, "{");
	m_tokenTbl[tk_rbrace].SetValue(-1, "}");
	m_tokenTbl[tk_rparen].SetValue(-1, ")");
	m_tokenTbl[tk_colon].SetValue(-1, ":");
	//m_tokenTbl[tk_semicolon].SetValue(-1, ";");
	m_tokenTbl[tk_lbrack].SetValue(-1, "[");
	m_tokenTbl[tk_rbrack].SetValue(-1, "]");
}

CLex::~CLex(void)
{
}

void
CLex::SetLine(string &lineStr, CLineInfo &lineInfo)
{
	m_pLine = lineStr.c_str();
	m_singleLineInfo = lineInfo;
	m_bSingleLine = true;
}

bool dumpLineBuf = false;

bool
CLex::GetLine()
{
	if (m_bSingleLine)
		return false;
	do {
		if (!CPreProcess::GetLine(m_lineBuf, 1024)) {
			m_lineBuf[0] = '\0';
			m_pLine = m_lineBuf;
			return false;
		}
		m_pLine = m_lineBuf;

		if (dumpLineBuf)
			puts(m_lineBuf);

	} while (m_pLine[0] == '`');
	return true;
}

string
CLex::GetLineBuf()
{
	int len = (int)strlen(m_pLine);
	while (m_pLine[len-1] == '\n' || m_pLine[len-1] == '\r')
		len -= 1;

	string lineBufStr = string(m_pLine, len);

	//AdvanceToNextStatement(m_pLine+len);

	m_pLine = "\n";
	return lineBufStr;
}

CLex::EToken
CLex::GetNextToken()
{
	for (;;) {
		switch (*m_pLine) {
			case '>': m_pLine += 1;
				if (*m_pLine == '=') { m_pLine += 1; return m_token = tk_greaterEqual; }
				return m_token = tk_greater;
			case '<': m_pLine += 1;
				if (*m_pLine == '=') { m_pLine += 1; return m_token = tk_lessEqual; }
				if (*m_pLine == '<') { m_pLine += 1; return m_token = tk_lessLess; }
				return m_token = tk_less;
			case '=': m_pLine += 1;
				if (*m_pLine == '=') { m_pLine += 1; return m_token = tk_equalEqual; }
				return m_token = tk_equal;
			case '!': m_pLine += 1;
				if (*m_pLine == '=') { m_pLine += 1; return m_token = tk_bangEqual; }
				return m_token = tk_bang;
			case '&': m_pLine += 1;
				if (*m_pLine == '&') { m_pLine += 1; return m_token = tk_ampersandAmpersand; }
				return m_token = tk_ampersand;
			case '|': m_pLine += 1;
				if (*m_pLine == '|') { m_pLine += 1; return m_token = tk_vbarVbar; }
				return m_token = tk_vbar;
			case '%': m_pLine += 1; return m_token = tk_percent;
			case '^': m_pLine += 1; return m_token = tk_carot;
			case '*': m_pLine += 1; return m_token = tk_asterisk;
			case '?': m_pLine += 1; return m_token = tk_question;
			case '+': m_pLine += 1; return m_token = tk_plus;
			case '-': m_pLine += 1; return m_token = tk_minus;
			case '(': m_pLine += 1; return m_token = tk_lparen;
			case ')': m_pLine += 1; return m_token = tk_rparen;
			case '[': m_pLine += 1; return m_token = tk_lbrack;
			case ']': m_pLine += 1; return m_token = tk_rbrack;
			case '{': m_pLine += 1; return m_token = tk_lbrace;
			case '}': m_pLine += 1; return m_token = tk_rbrace;
			case '.': m_pLine += 1; return m_token = tk_period;
			case ',': m_pLine += 1; return m_token = tk_comma;
			case ':': m_pLine += 1; return m_token = tk_colon;
			case ';': m_pLine += 1; return m_token = tk_semicolon;
			case '#': m_pLine += 1; return m_token = tk_pound;
			case '~': m_pLine += 1; return m_token = tk_tilda;
			case ' ': m_pLine += 1; continue;
			case '\t': m_pLine += 1; continue;
			case '\n': m_pLine += 1; continue;
			case '\r': m_pLine += 1; continue;
			case '/':
				if (m_pLine[1] == '/') {
					if (!GetLine())
						return m_token = tk_eof;
					continue;
				}
				if (m_pLine[1] == '*') {
					m_pLine += 2;
					while (m_pLine[0] != '*' || m_pLine[1] != '/') {
						if (*m_pLine++ == '\0' && !GetLine())
							return m_token = tk_eof;
					}
					m_pLine += 2;
					continue;
				}
				m_pLine += 1;
				return m_token = tk_slash;
			case '\0':
				if (!GetLine())
					return m_token = tk_eof;
				continue;
			case '"':
				GetString(m_pLine, m_string);
				return m_token = tk_string;
			case '\\':
				{
					char strBuf[256];
					char *pStr = strBuf;
					while (*m_pLine != ' ' && *m_pLine != '\0')
						*pStr++ = *m_pLine++;
					assert(*m_pLine == ' ');
					*pStr++ = *m_pLine++;
					*pStr = '\0';
					m_string = strBuf;
					return m_token = tk_ident;
				}
			default:
				if (isalpha(*m_pLine) || *m_pLine == '_') {
					GetName(m_pLine, m_string);
					return m_token = tk_ident;
				}
				if (isdigit(*m_pLine)) {
					GetNumber(m_pLine, m_string);
					return m_token = tk_num;
				}
				ParseMsg(PARSE_ERROR, "unexpected character '%c'", *m_pLine++);
				SkipTo(tk_semicolon);
				return m_token = tk_unknown;
		}
	}
}

void
CLex::GetName(const char *&pLine, string &name) {
	char nameBuf[256];
	char *pName = nameBuf;
	for (;;) {
		if (isalnum(*pLine) || *pLine == '_' || *pLine == '$') {
			*pName++ = *pLine++;
			continue;
		}
		break;
	}
	*pName = '\0';
	name = nameBuf;
}

void
CLex::GetNumber(const char *&pLine, string &num) {
	char numBuf[256];
	char *pNum = numBuf;
	const char *pLineStart = pLine;
	while (isdigit(*pLine))
		*pNum++ = *pLine++;
	if (strncmp(pLine, "'b", 2) == 0) {
		for (int cnt = 2 ;*pLine == '0' || *pLine == '1' || cnt > 0; cnt -= 1)
			*pNum++ = *pLine++;
	} else if (strncmp(pLine, "'h", 2) == 0) {
		for (int cnt = 2 ;isdigit(*pLine) || *pLine >= 'A' && *pLine <= 'F' || *pLine >= 'a' && *pLine <= 'f' || cnt > 0; cnt -= 1)
			*pNum++ = *pLine++;
	} else if (strncmp(pLine, "'d", 2) == 0) {
		for (int cnt = 2 ;isdigit(*pLine) || cnt > 0; cnt -= 1)
			*pNum++ = *pLine++;
	} else if (pLineStart[0] == '0' && pLineStart[1] == 'x') {
		for (int cnt = 1 ;isdigit(*pLine) || *pLine >= 'A' && *pLine <= 'F' || *pLine >= 'a' && *pLine <= 'f' || cnt > 0; cnt -= 1)
			*pNum++ = *pLine++;
	}

	if (isdigit(*pLine) || isalpha(*pLine))
		ParseMsg(PARSE_ERROR, "unexpected number format, found '%c'", *pLine);

	*pNum = '\0';
	num = numBuf;
}

int
CLex::GetInteger(int &width)
{
	const char *pNum = m_string.c_str();
	int value = 0;
	while (isdigit(*pNum))
		value = value * 10 + *pNum++ - '0';

	if (*pNum == '\0') {
		width = -1;
		return value;
	}

	width = value;

	if (pNum[0] == '\'') {
		if (tolower(pNum[1]) == 'h') {
			if (value > 64) {
				ParseMsg(PARSE_ERROR, "constant greater than 64 bits");
				return 0;
			}
			pNum += 2;
			value = 0;
			while (isdigit(*pNum) || tolower(*pNum) >= 'a' && tolower(*pNum) <= 'f')
				value = value * 16 + (isdigit(*pNum) ? (*pNum++ - '0') : (tolower(*pNum++) - 'a' + 10));
		} else if (tolower(pNum[1]) == 'b') {
			if (value > 64) {
				ParseMsg(PARSE_ERROR, "constant greater than 64 bits");
				return 0;
			}
			pNum += 2;
			value = 0;
			while (*pNum == '0' || *pNum == '1')
				value = value * 2 + *pNum++ - '0';
		}
	} else if (pNum[0] == 'x') {
		pNum += 1;
		value = 0;
		while (isdigit(*pNum) || tolower(*pNum) >= 'a' && tolower(*pNum) <= 'f')
			value = value * 16 + (isdigit(*pNum) ? (*pNum++ - '0') : (tolower(*pNum++) - 'a' + 10));
	}

	if (*pNum != '\0')
		ParseMsg(PARSE_ERROR, "unknown constant format");

	return value;
}

void
CLex::GetString(const char *&pLine, string &str) {
	char strBuf[1024];
	char *pStr = strBuf;
	assert(*pLine == '"');
	pLine += 1;
	while (*pLine != '"' && *pLine != '\0') {
		if (*pLine == '\\' && pLine[1] == '\n' && !GetLine())
			assert(0);
		*pStr = *pLine++;
		if (pStr < strBuf+1022)
			pStr += 1;
	}
	*pStr = '\0';
	if (*pLine == '\0')
		ParseMsg(PARSE_ERROR, "string does not have closing \"");

	if (strBuf+1022 == pStr) {
		ParseMsg(PARSE_ERROR, "string length limit exceeded");
		strBuf[0] = '\0';
	}
	pLine += 1;
	str = strBuf;
}

void
CLex::SkipTo(EToken token)
{
	for (int lvl = 0;;) {
		EToken tk = GetNextToken();
		if (tk == tk_eof)
			return;
		if (token == tk) {
			if (lvl == 0)
				return;
			else
				lvl -= 1;
		}
		if (token == tk_rbrace && tk == tk_lbrace)
			lvl += 1;
	}
}

void
CLex::SkipTo(EToken token1, EToken token2)
{
	for (int lvl = 0;;) {
		EToken tk = GetNextToken();
		if (tk == tk_eof)
			return;
		if (token1 == tk || token2 == tk) {
			if (lvl == 0)
				return;
			else
				lvl -= 1;
		}
		if ((token1 == tk_rbrace || token2 == tk_rbrace) && tk == tk_lbrace)
			lvl += 1;
	}
}

void
CLex::ParseMsg(EParseMsgType msgType, const char *msgStr, ...)
{
	va_list marker;
	va_start( marker, msgStr );

	CLineInfo lineInfo = GetLineInfo();

	CPreProcess::ParseMsg(msgType, lineInfo, msgStr, marker);
}

void
CLex::ParseMsg(EParseMsgType msgType, const CLineInfo &lineInfo, const char *msgStr, ...)
{
	va_list marker;
	va_start( marker, msgStr );

	CPreProcess::ParseMsg(msgType, lineInfo, msgStr, marker);
}
