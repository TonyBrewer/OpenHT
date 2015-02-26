/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "CnyHt.h"
#include "Lex.h"

CLex::CTokenTbl *CLex::m_pTokenTbl;

CLex::CLex()
{
	m_tokenTbl.resize(TK_TOKEN_CNT);
	m_tokenTbl[eTkLParen].SetValue(0, "(");
	m_tokenTbl[eTkPeriod].SetValue(1, ".");
	m_tokenTbl[eTkUnaryPlus].SetValue(3, "+");
	m_tokenTbl[eTkUnaryMinus].SetValue(3, "-");
	m_tokenTbl[eTkTilda].SetValue(3, "~");
	m_tokenTbl[eTkBang].SetValue(3, "!");
	m_tokenTbl[eTkAsterisk].SetValue(4, "*");
	m_tokenTbl[eTkSlash].SetValue(4, "/");
	m_tokenTbl[eTkPercent].SetValue(4, "%");
	m_tokenTbl[eTkPlus].SetValue(5, "+");
	m_tokenTbl[eTkMinus].SetValue(5, "-");
	m_tokenTbl[eTkLessLess].SetValue(6, "<<");
	m_tokenTbl[eTkGreaterGreater].SetValue(6, ">>");
	m_tokenTbl[eTkLess].SetValue(7, "<");
	m_tokenTbl[eTkLessEqual].SetValue(7, "<=");
	m_tokenTbl[eTkGreater].SetValue(7, ">");
	m_tokenTbl[eTkGreaterEqual].SetValue(7, ">=");
	m_tokenTbl[eTkEqualEqual].SetValue(8, "==");
	m_tokenTbl[eTkBangEqual].SetValue(8, "!=");
	m_tokenTbl[eTkAmpersand].SetValue(9, "&");
	m_tokenTbl[eTkCarot].SetValue(10, "^");
	m_tokenTbl[eTkVbar].SetValue(11, "|");
	m_tokenTbl[eTkAmpersandAmpersand].SetValue(12, "&&");
	m_tokenTbl[eTkVbarVbar].SetValue(13, "||");
	m_tokenTbl[eTkQuestion].SetValue(14, "?");
	m_tokenTbl[eTkEqual].SetValue(15, "=");
	m_tokenTbl[eTkComma].SetValue(16, ",");
	m_tokenTbl[eTkExprBegin].SetValue(17, "");
	m_tokenTbl[eTkExprEnd].SetValue(18, "");
	m_tokenTbl[eTkLBrace].SetValue(-1, "{");
	m_tokenTbl[eTkRBrace].SetValue(-1, "}");
	m_tokenTbl[eTkRParen].SetValue(-1, ")");
	m_tokenTbl[eTkColon].SetValue(-1, ":");
	m_tokenTbl[eTkSemi].SetValue(-1, ";");
	m_tokenTbl[eTkLBrack].SetValue(-1, "[");
	m_tokenTbl[eTkRBrack].SetValue(-1, "]");

	m_pTokenTbl = &m_tokenTbl;
}

bool CLex::LexOpen(string fileName)
{
	static char nullLine = 0;
	m_pLine = &nullLine;
	m_tk = eTkUnknown;

	bool bOpened = CPreProcess::Open(fileName);
	if (bOpened)
		GetNextTk();
	return bOpened;
}

void CLex::LexClose()
{
	CPreProcess::Close();
}

bool CLex::GetNextLine()
{
	for (;;) {
		if (!GetLine(m_lineBuf))
			return false;

		m_pLine = m_lineBuf.c_str();

		// skip empty lines
		while (*m_pLine == ' ' || *m_pLine == '\t')
			m_pLine += 1;

		if (*m_pLine == '/' && m_pLine[1] == '/')
			continue;

		if (*m_pLine == '\r' || *m_pLine == '\n' || *m_pLine == '\0')
			continue;

		return true;
	}
}

EToken CLex::GetNextTk()
{
	if (m_tk == eTkEof)
		return m_tk;

	while (*m_pLine == ' ' || *m_pLine == '\t') m_pLine += 1;

	switch (*m_pLine) {
	case '(':
		m_pLine += 1;
		return m_tk = eTkLParen;
	case ')':
		m_pLine += 1;
		return m_tk = eTkRParen;
	case '=':
		m_pLine += 1;
		return m_tk = eTkEqual;
	case ',':
		m_pLine += 1;
		return m_tk = eTkComma;
	case '\'':
		m_pLine++;
		return eTkFTick;
	case '`':
		m_pLine++;
		return eTkBTick;
	case ';':
		m_pLine += 1;
		return m_tk = eTkSemi;
	case '.':
		m_pLine += 1;
		return m_tk = eTkPeriod;
	case '#':
		m_pLine += 1;
		return m_tk = eTkPound;
	case '[':
		m_pLine += 1;
		return m_tk = eTkLBrack;
	case ']':
		m_pLine += 1;
		return m_tk = eTkRBrack;
	case '{':
		m_pLine += 1;
		return m_tk = eTkLBrace;
	case '}':
		m_pLine += 1;
		return m_tk = eTkRBrace;
	case '-':
		m_pLine += 1;
		return m_tk = eTkMinus;
	case '+':
		m_pLine += 1;
		return m_tk = eTkPlus;
	case '*':
		m_pLine += 1;
		return m_tk = eTkAsterisk;
	case '%':
		m_pLine += 1;
		return m_tk = eTkPercent;
	case '<':
		m_pLine += 1;
		return m_tk = eTkLess;
	case '>':
		m_pLine += 1;
		return m_tk = eTkGreater;
	case ':':
		m_pLine += 1;
		return m_tk = eTkColon;
	case '?':
		m_pLine += 1;
		return m_tk = eTkQuestion;
	case '|':
		m_pLine += 1;
		return m_tk = eTkVbar;
	case '&':
		m_pLine += 1;
		return m_tk = eTkAmpersand;
	case '^':
		m_pLine += 1;
		return m_tk = eTkCarot;
	case '\r':
	case '\n':
	case '\0':
		if (GetNextLine())
			return m_tk = GetNextTk();
		else
			return m_tk = eTkEof;
	case '/':
		if (m_pLine[1] == '/') {
			if (GetNextLine())
				return m_tk = GetNextTk();
			else
				return m_tk = eTkEof;
		} else {
			m_pLine += 1;
			return m_tk = eTkSlash;
		}
		break;
	default:
		char const * pStart = m_pLine;
		if (isdigit(*m_pLine)) {
			while (isdigit(*m_pLine))
				m_pLine += 1;

			if (!isalpha(*m_pLine) && *m_pLine != '_') {
				m_string = string(pStart, m_pLine - pStart);
				return m_tk = eTkInteger;
			}
		}
		if (isalpha(*m_pLine) || *m_pLine == '_') {
			// identifier
			while (isalpha(*m_pLine) || *m_pLine == '_' || isdigit(*m_pLine))
				m_pLine += 1;
			m_string = string(pStart, m_pLine - pStart);
			return m_tk = eTkIdent;
		}
		if (*m_pLine == '"') {
			m_pLine += 1;
			pStart = m_pLine;
			while (*m_pLine != '"' && *m_pLine != '\0')
				m_pLine += 1;
			if (*m_pLine != '"')
				ParseMsg(Error, "Expected a closing quote for string");
			m_string = string(pStart, m_pLine - pStart);
			if (*m_pLine != '\0')
				m_pLine += 1;
			return m_tk = eTkString;
		}
		ParseMsg(Fatal, "Unknown input");
		m_pLine += 1;
		return eTkUnknown;
	}

	// should not reach this point
	HtlAssert(0);
	return eTkSemi;
}

int CLex::GetTkInteger()
{
	return atoi(m_string.c_str());
}

// get an HTD file parameter string
// retrn a string with chars starting from m_pLine to next ',' or ')'
//  string may wrap to next line
string CLex::GetExprStr(char termCh)
{
	string paramStr;
	char const * pPos = m_pLine;
	CLineInfo lineInfo = m_lineInfo;

	while (isspace(*pPos)) pPos += 1;
	char const * pEnd = pPos;

	if (*pPos == '"') {
		static bool bWarn = true;
		if (bWarn) {
			ParseMsg(Warning, "parameters enclosed in quotes is deprecated");
			bWarn = false;
		}

		pPos += 1;
		pEnd = pPos;
		for (;;) {
			if (*pEnd == '"') {
				paramStr += string(pPos, pEnd - pPos);
				m_pLine = pEnd + 1;
				return paramStr;
			}

			if (*pEnd == '\0') {
				paramStr += string(pPos, pEnd - pPos) + " ";
				if (!GetNextLine())
					ParseMsg(Fatal, lineInfo, "found eof while parsing parameter");
				pEnd = pPos = m_pLine;
				continue;
			}

			pEnd += 1;
		}
	} else {
		int parenCnt = 0;
		for (;;) {
			if (*pEnd == '(')
				parenCnt += 1;
			else if (*pEnd == ')') {
				if (parenCnt == 0) {
					paramStr += string(pPos, pEnd - pPos);
					m_pLine = pEnd;
					return paramStr;
				} else
					parenCnt -= 1;
			}

			if (*pEnd == termCh && parenCnt == 0) {
				paramStr += string(pPos, pEnd - pPos);
				m_pLine = pEnd;
				return paramStr;
			}

			if (*pEnd == '\0') {
				paramStr += string(pPos, pEnd - pPos) + " ";
				if (!GetNextLine())
					ParseMsg(Fatal, lineInfo, "found eol while parsing parameter");
				pEnd = pPos = m_pLine;
				continue;
			}

			pEnd += 1;
		}
	}
}
