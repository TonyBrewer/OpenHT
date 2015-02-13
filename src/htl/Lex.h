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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <string>
using namespace std;

#include "PreProcess.h"

enum EToken { eTkIdent, eTkEqual, eTkPeriod, eTkLParen, eTkRParen, eTkString, eTkInteger, eTkComma, eTkSemi,
	eTkError, eTkBang, eTkNumInt, eTkNumHex, eTkExprEnd, eTkPlus, eTkMinus, eTkSlash, eTkAsterisk, eTkPercent,
	eTkEqualEqual, eTkVbar, eTkVbarVbar, eTkCarot, eTkAmpersand, eTkAmpersandAmpersand, eTkBangEqual, eTkLess,
	eTkGreater, eTkGreaterEqual, eTkLessEqual, eTkLessLess, eTkGreaterGreater, eTkExprBegin, eTkUnaryPlus,
	eTkUnaryMinus, eTkTilda, eTkPound, eTkLBrack, eTkRBrack, eTkColon, eTkBoolean, eTkIntRange, eTkIntList, eTkQuestion,
	eTkLBrace, eTkRBrace, eTkFTick, eTkBTick, eTkIntSet, eTkIdentRef, eTkParamStr, eTkTypeStr, eTkEof, eTkUnknown };
	
class CLex : public CPreProcess {
public:
	bool LexOpen(string fileName);
	void LexClose();
	bool GetNextLine();
	EToken GetNextTk();
	EToken GetTk() { return m_tk; }
	string &GetTkString() { return m_string; }
	int GetTkInteger();
	static int GetParseErrorCnt() { return m_errorCnt; }
	string GetExprStr(char termCh);
	char const * GetLinePos() { return m_pLine; }
	void SetLinePos(char const * pPos) {
		size_t len = strlen(m_pLine);
		assert(pPos >= m_pLine && m_pLine + len >= pPos);
		m_pLine = pPos;
	}
	bool GetLine(string &line) { return CPreProcess::GetLine(line); }

private:
    string          m_lineBuf;
	char const *    m_pLine;
	string          m_string;
	EToken          m_tk;
};
