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
	char const * pEnd = pPos;
	CLineInfo lineInfo = m_lineInfo;

	while (isspace(*pPos)) pPos += 1;

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
				paramStr += string(pPos, pEnd-pPos);
				m_pLine = pEnd + 1;
				return paramStr;
			}

			if (*pEnd == '\0') {
				paramStr += string(pPos, pEnd-pPos) + " ";
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
			if (*pEnd == termCh || parenCnt == 0 && *pEnd == ')') {
				paramStr += string(pPos, pEnd-pPos);
				m_pLine = pEnd;
				return paramStr;
			}

			if (*pEnd == '(')
				parenCnt += 1;
			else if (*pEnd == ')')
				parenCnt -= 1;

			if (*pEnd == '\0') {
				paramStr += string(pPos, pEnd-pPos) + " ";
				if (!GetNextLine())
					ParseMsg(Fatal, lineInfo, "found eof while parsing parameter");
				pEnd = pPos = m_pLine;
				continue;
			}

			pEnd += 1;
		}
	}
}

