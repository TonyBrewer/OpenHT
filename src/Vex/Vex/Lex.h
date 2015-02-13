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

#include "PreProcess.h"

class CLex : public CPreProcess
{
public:
	CLex(const char *pFileName);
	~CLex(void);

public:
	struct CTokenInfo {
		void SetValue(int precedence, const char *pTokenStr) {
			m_precedence = precedence; m_tokenStr = pTokenStr;
		}
		string &GetString() { return m_tokenStr; }
		int GetPrecedence() { return m_precedence; }
		bool IsTokenAssocLR() { return m_precedence != 2 && m_precedence != 14 &&
			m_precedence != 15; }

		string m_tokenStr;
		int m_precedence;
	};

	typedef vector<CTokenInfo> CTokenTbl;

public:
	EToken GetNextToken();
	EToken GetToken() { return m_token; }
	string &GetString() { return m_string; }
	int GetInteger(int &width);
	void SkipTo(EToken);
	void SkipTo(EToken, EToken);
	void SetLine(string &lineStr, CLineInfo &lineInfo);

	static string &GetTokenString(EToken token) { return m_tokenTbl[token].GetString(); }
	static int GetTokenPrec(EToken token) { return m_tokenTbl[token].GetPrecedence(); }
	static bool IsTokenAssocLR(EToken token) { return m_tokenTbl[token].IsTokenAssocLR(); }

	void ParseMsg(EParseMsgType msgType, const char *msgStr, ...);
	void ParseMsg(EParseMsgType msgType, const CLineInfo &lineInfo, const char *msgStr, ...);

	CLineInfo &GetLineInfo() { return m_bSingleLine ? m_singleLineInfo : CPreProcess::GetLineInfo(); }
	string GetLineBuf();

public:
	static CTokenTbl	m_tokenTbl;

private:
	struct CToken {
		CToken(EToken token, int width, string &str) : m_token(token), m_width(width), m_string(str) {}

		EToken	m_token;
		int		m_width;		// expansion width
		string	m_string;
	};

private:
	bool GetLine();
	void GetName(const char *&pLine, string &name);
	void GetNumber(const char *&pLine, string &name);
	void GetString(const char *&pLine, string &name);

private:
	const char		*m_pLine;
	char			m_lineBuf[LINE_SIZE];
	EToken			m_token;
	string			m_string;
	bool			m_bSingleLine;
	CLineInfo		m_singleLineInfo;
};
