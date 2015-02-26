/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
// Lex.cpp: implementation of the CHtfeLex class.
//
//////////////////////////////////////////////////////////////////////

#include "Htfe.h"
#include "HtfeArgs.h"

#ifdef __GNUG__
#include <limits.h>
#endif

#include "HtfeLex.h"

CHtfeLex::CTokenTbl *CHtfeLex::m_pTokenTbl;
CLineInfo CHtfeLex::m_lineInfo;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CHtfeLex::CHtfeLex()
{
	m_rsvdTbl.Insert("const")->SetToken(tk_const);
	m_rsvdTbl.Insert("operator")->SetToken(tk_operator);
	m_rsvdTbl.Insert("union")->SetToken(tk_union);
	m_rsvdTbl.Insert("class")->SetToken(tk_class);
	m_rsvdTbl.Insert("static")->SetToken(tk_static);
	m_rsvdTbl.Insert("private")->SetToken(tk_private);
	m_rsvdTbl.Insert("public")->SetToken(tk_public);
	m_rsvdTbl.Insert("struct")->SetToken(tk_struct);
	m_rsvdTbl.Insert("friend")->SetToken(tk_friend);
	m_rsvdTbl.Insert("new")->SetToken(tk_new);
	m_rsvdTbl.Insert("return")->SetToken(tk_return);
	m_rsvdTbl.Insert("for")->SetToken(tk_for);
	m_rsvdTbl.Insert("do")->SetToken(tk_do);
	m_rsvdTbl.Insert("while")->SetToken(tk_while);
	m_rsvdTbl.Insert("include")->SetToken(tk_include);
	m_rsvdTbl.Insert("define")->SetToken(tk_define);
	m_rsvdTbl.Insert("typedef")->SetToken(tk_typedef);
	m_rsvdTbl.Insert("sc_in")->SetToken(tk_sc_in);
	m_rsvdTbl.Insert("sc_out")->SetToken(tk_sc_out);
	m_rsvdTbl.Insert("sc_inout")->SetToken(tk_sc_inout);
	m_rsvdTbl.Insert("sc_signal")->SetToken(tk_sc_signal);
	m_rsvdTbl.Insert("enum")->SetToken(tk_enum);
	m_rsvdTbl.Insert("SC_CTOR")->SetToken(tk_SC_CTOR);
	m_rsvdTbl.Insert("SC_METHOD")->SetToken(tk_SC_METHOD);
	m_rsvdTbl.Insert("sensitive")->SetToken(tk_sensitive);
	m_rsvdTbl.Insert("sensitive_pos")->SetToken(tk_sensitive_pos);
	m_rsvdTbl.Insert("sensitive_neg")->SetToken(tk_sensitive_neg);
	m_rsvdTbl.Insert("if")->SetToken(tk_if);
	m_rsvdTbl.Insert("else")->SetToken(tk_else);
	m_rsvdTbl.Insert("switch")->SetToken(tk_switch);
	m_rsvdTbl.Insert("case")->SetToken(tk_case);
	m_rsvdTbl.Insert("default")->SetToken(tk_default);
	m_rsvdTbl.Insert("break")->SetToken(tk_break);
	m_rsvdTbl.Insert("continue")->SetToken(tk_continue);
	m_rsvdTbl.Insert("goto")->SetToken(tk_goto);
	m_rsvdTbl.Insert("try")->SetToken(tk_try);
	m_rsvdTbl.Insert("catch")->SetToken(tk_catch);
	m_rsvdTbl.Insert("throw")->SetToken(tk_throw);
	m_rsvdTbl.Insert("template")->SetToken(tk_template);
	m_rsvdTbl.Insert("namespace")->SetToken(tk_namespace);
	m_rsvdTbl.Insert("delete")->SetToken(tk_delete);

	m_tokenTbl.resize(TK_TOKEN_CNT);
	m_tokenTbl[tk_lparen].SetValue(0, "(");
	m_tokenTbl[tk_period].SetValue(1, ".");
	m_tokenTbl[tk_minusGreater].SetValue(1, "->");
	m_tokenTbl[tk_colonColon].SetValue(1, "::");
	m_tokenTbl[tk_new].SetValue(3, "new");
	m_tokenTbl[tk_plusPlus].SetValue(3, "++");
	m_tokenTbl[tk_minusMinus].SetValue(3, "--");
	m_tokenTbl[tk_preInc].SetValue(3, "++");
	m_tokenTbl[tk_preDec].SetValue(3, "--");
	m_tokenTbl[tk_postInc].SetValue(3, "++");
	m_tokenTbl[tk_postDec].SetValue(3, "--");
	m_tokenTbl[tk_typeCast].SetValue(3, "");
	m_tokenTbl[tk_indirection].SetValue(3, "*");
	m_tokenTbl[tk_addressOf].SetValue(3, "&");
	m_tokenTbl[tk_unaryPlus].SetValue(3, "+");
	m_tokenTbl[tk_unaryMinus].SetValue(3, "-");
	m_tokenTbl[tk_tilda].SetValue(3, "~");
	m_tokenTbl[tk_bang].SetValue(3, "!");
	m_tokenTbl[tk_asterisk].SetValue(4, "*");
	m_tokenTbl[tk_slash].SetValue(4, "/");
	m_tokenTbl[tk_percent].SetValue(4, "%");
	m_tokenTbl[tk_plus].SetValue(5, "+");
	m_tokenTbl[tk_minus].SetValue(5, "-");
	m_tokenTbl[tk_lessLess].SetValue(6, "<<");
	m_tokenTbl[tk_greaterGreater].SetValue(6, ">>");
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
	m_tokenTbl[tk_plusEqual].SetValue(15, "+=");
	m_tokenTbl[tk_minusEqual].SetValue(15, "-=");
	m_tokenTbl[tk_asteriskEqual].SetValue(15, "*=");
	m_tokenTbl[tk_slashEqual].SetValue(15, "/=");
	m_tokenTbl[tk_percentEqual].SetValue(15, "%=");
	m_tokenTbl[tk_greaterGreaterEqual].SetValue(15, ">>=");
	m_tokenTbl[tk_lessLessEqual].SetValue(15, "<<=");
	m_tokenTbl[tk_ampersandEqual].SetValue(15, "&=");
	m_tokenTbl[tk_carotEqual].SetValue(15, "^=");
	m_tokenTbl[tk_tildaEqual].SetValue(15, "~=");
	m_tokenTbl[tk_vbarEqual].SetValue(15, "|=");
	m_tokenTbl[tk_comma].SetValue(16, ",");
	m_tokenTbl[tk_exprBegin].SetValue(17, "");
	m_tokenTbl[tk_exprEnd].SetValue(18, "");
	m_tokenTbl[tk_class].SetValue(-1, "class");
	m_tokenTbl[tk_lbrace].SetValue(-1, "{");
	m_tokenTbl[tk_rbrace].SetValue(-1, "}");
	m_tokenTbl[tk_rparen].SetValue(-1, ")");
	m_tokenTbl[tk_colon].SetValue(-1, ":");
	m_tokenTbl[tk_semicolon].SetValue(-1, ";");
	m_tokenTbl[tk_lbrack].SetValue(-1, "[");
	m_tokenTbl[tk_rbrack].SetValue(-1, "]");
	m_tokenTbl[tk_static].SetValue(-1, "static");
	m_tokenTbl[tk_private].SetValue(-1, "private");
	m_tokenTbl[tk_public].SetValue(-1, "public");
	m_tokenTbl[tk_struct].SetValue(-1, "struct");
	m_tokenTbl[tk_friend].SetValue(-1, "friend");
	m_tokenTbl[tk_include].SetValue(-1, "include");
	m_tokenTbl[tk_typedef].SetValue(-1, "typedef");
	m_tokenTbl[tk_SC_CTOR].SetValue(-1, "SC_CTOR");
	m_tokenTbl[tk_SC_METHOD].SetValue(-1, "SC_METHOD");
	m_tokenTbl[tk_sensitive].SetValue(-1, "sensitive");
	m_tokenTbl[tk_sensitive_pos].SetValue(-1, "sensitive_pos");
	m_tokenTbl[tk_sensitive_neg].SetValue(-1, "sensitive_neg");
	m_tokenTbl[tk_if].SetValue(-1, "if");
	m_tokenTbl[tk_else].SetValue(-1, "else");
	m_tokenTbl[tk_switch].SetValue(-1, "switch");
	m_tokenTbl[tk_case].SetValue(-1, "case");
	m_tokenTbl[tk_default].SetValue(-1, "default");
	m_tokenTbl[tk_break].SetValue(-1, "break");
	m_tokenTbl[tk_sc_in].SetValue(-1, "sc_in");
	m_tokenTbl[tk_sc_out].SetValue(-1, "sc_out");
	m_tokenTbl[tk_sc_inout].SetValue(-1, "sc_inout");
	m_tokenTbl[tk_sc_signal].SetValue(-1, "sc_signal");

	m_pTokenTbl = &m_tokenTbl;

	m_token = (EToken) -1;
	m_bOpenLine = false;
	m_bTokenRecording = false;
	m_bTokenPlayback = false;
	m_bRemovedToken = false;
	m_tokenPlaybackLevel = -1;

	m_tkDumpFp = 0;
	if (g_pHtfeArgs->IsTkDumpEnabled())
		m_tkDumpFp = fopen("tkDump.txt", "w");
}

CHtfeLex::~CHtfeLex()
{

}

bool
CHtfeLex::Open(const string &fileName)
{
	m_pPos = "";
	return CPreProcess::Open(fileName);
}

CHtfeLex::EToken
CHtfeLex::GetNextToken()
{
	char pCh = ' ';
	char rCh = ' ';
	EToken tk;
	if (m_bTokenPlayback && !m_bOpenLine) {

		bool bEndOfRecordedList = m_tokenRecord[m_tokenPlaybackLevel].m_playbackPos + 1 == (int)m_tokenRecord[m_tokenPlaybackLevel].m_tokenList.size();
		if (m_bTokenPlayback && m_bTokenRecording && bEndOfRecordedList)
			return m_token = tk_eor;

		pCh = 'p';
		m_tokenRecord[m_tokenPlaybackLevel].m_playbackPos += 1;
		if (bEndOfRecordedList) {
			// end of recorded token list, increment control value and replay list
			m_tokenRecord[m_tokenPlaybackLevel].m_value += m_tokenRecord[m_tokenPlaybackLevel].m_incValue;
			if (m_tokenRecord[m_tokenPlaybackLevel].m_value < m_tokenRecord[m_tokenPlaybackLevel].m_exitValue)
				m_tokenRecord[m_tokenPlaybackLevel].m_playbackPos = 0;
			else {
				// reached the end of the playback, pop the playback stack
				Assert(m_tokenPlaybackLevel == (int)m_tokenRecord.size()-1);
				m_tokenPlaybackLevel -= 1;
				m_tokenRecord.pop_back();
				if (m_tokenRecord.size() == 0) {
					m_bTokenPlayback = false;

					if (m_bRemovedToken) {
						m_bRemovedToken = false;
						m_token = m_removedToken.GetToken();
						m_stringBuf = m_removedToken.GetString();
						m_lineInfo = m_removedToken.GetLineInfo();
						return GetToken();
					} else
						return GetNextToken();
				}

				GetNextToken();

				if (m_tokenRecord.size() == 0)
					return GetToken();
			}
		}

		m_playbackToken = m_tokenRecord[m_tokenPlaybackLevel].m_tokenList[m_tokenRecord[m_tokenPlaybackLevel].m_playbackPos];

		tk = m_token = m_playbackToken.GetToken();

		if (tk == tk_identifier && m_playbackToken.GetString() == m_tokenRecord[m_tokenPlaybackLevel].m_ctrlVarName) {
			// substitute the constant value
			tk = m_token = tk_num_int;
			char buf[32];
			sprintf(buf, "%d%s", m_tokenRecord[m_tokenPlaybackLevel].m_value,
				m_tokenRecord[m_tokenPlaybackLevel].m_bUnsignedCtrlVar ? "ul" : "");
			m_stringBuf = buf;
		} else if (tk == tk_string) {
			char const *pStr = m_playbackToken.GetRawString().c_str();
			char buf[256];
			char *pBuf = buf;
			char const *pVar = m_tokenRecord[m_tokenPlaybackLevel].m_ctrlVarName.c_str();
			int varLen = m_tokenRecord[m_tokenPlaybackLevel].m_ctrlVarName.size();

			while (*pStr) {
				if (*pStr == '%' && strncmp(pStr+1, pVar, varLen) == 0) {
					char buf2[32];
					sprintf(buf2, "%d", m_tokenRecord[m_tokenPlaybackLevel].m_value);
					char const *pBuf2 = buf2;
					while (*pBuf2) *pBuf++ = *pBuf2++;
					pStr += 1 + varLen;
					continue;
				}
				*pBuf++ = *pStr++;
			}
			*pBuf = '\0';
			m_stringBuf = buf;
		} else if (tk == tk_identifier || tk == tk_num_int || tk == tk_num_hex || tk == tk_num_string)
			m_stringBuf = m_playbackToken.GetString();
	} else
		tk = GetNextToken2();

	if (m_bTokenRecording) {
		rCh = 'r';
		m_tokenRecord.back().m_tokenList.push_back(CToken(tk, GetString(), GetLineInfo()));
	}

	if (m_tkDumpFp) {
		static int tkCnt = 0;
		//if (tkCnt == 13309)
		//	bool stop = true;
		if (tk == tk_identifier || tk == tk_num_int)
			fprintf(m_tkDumpFp, "%d %c%c tk: %s %s\n", tkCnt++, pCh, rCh, m_tokenTbl[tk].GetString().c_str(), GetString().c_str());
		else
			fprintf(m_tkDumpFp, "%d %c%c tk: %s\n", tkCnt++, pCh, rCh, m_tokenTbl[tk].GetString().c_str());
	}

	m_lineInfo = GetLineInfoInternal();

	return tk;
}

CHtfeLex::EToken
CHtfeLex::GetNextToken2()
{
	const char *pInitPos;
	bool bNumString;

	if (m_token == tk_eof)
		return m_token;

	SkipWhite();

	while (*m_pPos == '\0') {
		if (m_bOpenLine) {
			m_lineBuf = m_lineBufSave;
			m_pPos = m_lineBuf.c_str() + m_posSave;
			m_token = m_tokenSave;
			m_stringBuf = m_stringBufSave;
			m_bOpenLine = false;
			return m_token;
		}

		// get a new line
		if (!GetLine(m_lineBuf))
			return m_token = tk_eof;  // end of file

		m_pPos = m_lineBuf.c_str();

		SkipWhite();
	}

	switch (*m_pPos) {
		case '"':
			m_pPos += 1;
			pInitPos = m_pPos;
			bNumString = true;
			while (*m_pPos != '"' && *m_pPos != '\0') {
				if (*m_pPos != '0' && *m_pPos != '1')
					bNumString = false;
				m_pPos += 1;
			}
			m_stringBuf.assign(pInitPos, m_pPos-pInitPos);

			if (*m_pPos++ != '"') {
				m_pPos = "";
				ParseMsg(PARSE_ERROR, "String is not closed");
			}

			return m_token = bNumString ? tk_num_string : tk_string;
		case '|':
			m_pPos += 1;
			if (*m_pPos == '|') {
				m_pPos += 1;
				return m_token = tk_vbarVbar;
			}
			if (*m_pPos == '=') {
				m_pPos += 1;
				return m_token = tk_vbarEqual;
			}
			return m_token = tk_vbar;
		case '!':
			m_pPos += 1;
			if (*m_pPos == '=') {
				m_pPos += 1;
				return m_token = tk_bangEqual;
			}
			return m_token = tk_bang;
		case '%':
			m_pPos += 1;
			if (*m_pPos == '=') {
				m_pPos += 1;
				return m_token = tk_percentEqual;
			}
			return m_token = tk_percent;
		case ',':
			m_pPos += 1;
			return m_token = tk_comma;
		case '[':
			m_pPos += 1;
			return m_token = tk_lbrack;
		case ']':
			m_pPos += 1;
			return m_token = tk_rbrack;
		case '*':
			m_pPos += 1;
			if (*m_pPos == '=') {
				m_pPos += 1;
				return m_token = tk_asteriskEqual;
			}
			return m_token = tk_asterisk;
		case '~':
			m_pPos += 1;
			if (*m_pPos == '=') {
				m_pPos += 1;
				return m_token = tk_tildaEqual;
			}
			return m_token = tk_tilda;
		case ':':
			m_pPos += 1;
			if (*m_pPos == ':') {
				m_pPos += 1;
				return m_token = tk_colonColon;
			}
			return m_token = tk_colon;
		case ';':
			m_pPos += 1;
			return m_token = tk_semicolon;
		case '{':
			m_pPos += 1;
			return m_token = tk_lbrace;
		case '}':
			m_pPos += 1;
			return m_token = tk_rbrace;
		case '(':
			m_pPos += 1;
			return m_token = tk_lparen;
		case ')':
			m_pPos += 1;
			return m_token = tk_rparen;
		case '/':
			m_pPos += 1;
			if (*m_pPos == '=') {
				m_pPos += 1;
				return m_token = tk_slashEqual;
			}
			return m_token = tk_slash;
		case '+':
			m_pPos += 1;
			if (*m_pPos == '+') {
				m_pPos += 1;
				return m_token = tk_plusPlus;
			}
			if (*m_pPos == '=') {
				m_pPos += 1;
				return m_token = tk_plusEqual;
			}
			return m_token = tk_plus;
		case '-':
			m_pPos += 1;
			if (*m_pPos == '=') {
				m_pPos += 1;
				return m_token = tk_minusEqual;
			}
			if (*m_pPos == '-') {
				m_pPos += 1;
				return m_token = tk_minusMinus;
			}
			if (*m_pPos == '>') {
				m_pPos += 1;
				return m_token = tk_minusGreater;
			}
			return m_token = tk_minus;
		case '=':
			m_pPos += 1;
			if (*m_pPos == '=') {
				m_pPos += 1;
				return m_token = tk_equalEqual;
			}
			return m_token = tk_equal;
		case '?':
			m_pPos += 1;
			return m_token = tk_question;
		case '>':
			m_pPos += 1;
			if (*m_pPos == '=') {
				m_pPos += 1;
				return m_token = tk_greaterEqual;
			}
			if (*m_pPos == '>') {
				m_pPos += 1;
				if (*m_pPos == '=') {
					m_pPos += 1;
					return m_token = tk_greaterGreaterEqual;
				}
				return m_token = tk_greaterGreater;
			}
			return m_token = tk_greater;
		case '<':
			m_pPos += 1;
			if (*m_pPos == '<') {
				m_pPos += 1;
				if (*m_pPos == '=') {
					m_pPos += 1;
					return m_token = tk_lessLessEqual;
				}
				return m_token = tk_lessLess;
			}
			if (*m_pPos == '=') {
				m_pPos += 1;
				return m_token = tk_lessEqual;
			}
			return m_token = tk_less;
		case '.':
			m_pPos += 1;
			return m_token = tk_period;
		case '&':
			m_pPos += 1;
			if (*m_pPos == '&') {
				m_pPos += 1;
				return m_token = tk_ampersandAmpersand;
			}
			if (*m_pPos == '=') {
				m_pPos += 1;
				return m_token = tk_ampersandEqual;
			}
			return m_token = tk_ampersand;
		case '^':
			m_pPos += 1;
			if (*m_pPos == '=') {
				m_pPos += 1;
				return m_token = tk_carotEqual;
			}
			return m_token = tk_carot;
		default:
			if (isalpha(*m_pPos) || *m_pPos == '_' || *m_pPos == '$') {
				pInitPos = m_pPos;
				while (isalnum(*m_pPos) || *m_pPos == '_' || *m_pPos == '$') {
					static bool bErrorMsg = false;
					if (*m_pPos == '$' && bErrorMsg == false) {
						ParseMsg(PARSE_ERROR, "identifiers with $ character are not supported");
						bErrorMsg = true;
					}

					m_pPos += 1;
				}
				m_stringBuf.assign(pInitPos, m_pPos-pInitPos);

				// check if identifier is a reserved word
				return m_token = m_rsvdTbl.Find(m_stringBuf);
			}
			char ch = 0;
			if (*m_pPos == '\'') {
				m_pPos += 1;
				if (*m_pPos == '\\') {
					m_pPos += 1;
					switch (*m_pPos) {
					case 'n': ch = '\n'; break;
					case 'r': ch = '\r'; break;
					case '\\': ch = '\\'; break;
					case 't': ch = '\t'; break;
					case 'v': ch = '\v'; break;
					case 'b': ch = '\b'; break;
					case 'f': ch = '\f'; break;
					case 'a': ch = '\a'; break;
					case '?': ch = '\?'; break;
					case '\'': ch = '\''; break;
					case '"': ch = '\"'; break;
					default: 
						{
							if (isdigit(*m_pPos) && (*m_pPos - '0' < 8)) {
								int v = 0;
								for (int i = 0; i < 3 && isdigit(*m_pPos) && (*m_pPos - '0' < 8); i += 1)
									// octal
									v = v * 8 + *m_pPos++ - '0';
								if (v > 255)
									ParseMsg(PARSE_ERROR, "octal digit too large");
								ch = (char)v;
								m_pPos -= 1;
							} else if (*m_pPos == 'x') {
								char buf[3];
								buf[0] = m_pPos[1];
								buf[1] = m_pPos[2];
								buf[2] = 0;

								char * pEnd;
								int v = strtol(buf, &pEnd, 16);
								m_pPos += pEnd - buf;
								if (pEnd - buf == 0)
									ParseMsg(PARSE_ERROR, "illegal hex digit");
								else if (v > 255)
									ParseMsg(PARSE_ERROR, "hex digit too large");
							} else
								ParseMsg(PARSE_ERROR, "illegal escape sequence");
						}
					}
					m_pPos += 1;
				} else {
					ch = *m_pPos++;
				}
				if (*m_pPos++ != '\'')
					ParseMsg(PARSE_ERROR, "illegal character constant");

				char str[8];
				sprintf(str, "0x%02x", ch);
				m_stringBuf.assign(str, strlen(str));
				return m_token = tk_num_hex;
			}
			if (isdigit(*m_pPos)) {
				pInitPos = m_pPos;
				if (*m_pPos == '0' && m_pPos[1] == 'x') {
					m_pPos += 2;
					while (isxdigit(*m_pPos)) m_pPos += 1;

					if (tolower(*m_pPos) == 'u') m_pPos += 1;
					if (tolower(*m_pPos) == 'l') m_pPos += 1;
					if (tolower(*m_pPos) == 'l') m_pPos += 1;

					m_stringBuf.assign(pInitPos, m_pPos-pInitPos);
					return m_token = tk_num_hex;
				} else {
					while (isdigit(*m_pPos)) m_pPos += 1;
					if (*m_pPos == '.') {
						m_pPos += 1;
						while (isdigit(*m_pPos)) m_pPos += 1;
						if ((*m_pPos == 'i' || *m_pPos == 'I') && m_pPos[1] == '6' && m_pPos[2] == '4')
							m_pPos += 3;
						m_stringBuf.assign(pInitPos, m_pPos-pInitPos);
						return m_token = tk_num_float;

					} else {
						if (tolower(*m_pPos) == 'u') m_pPos += 1;
						if (tolower(*m_pPos) == 'l') m_pPos += 1;
						if (tolower(*m_pPos) == 'l') m_pPos += 1;

						m_stringBuf.assign(pInitPos, m_pPos-pInitPos);
						return m_token = tk_num_int;
					}
				}
			}

			ParseMsg(PARSE_ERROR, "unexpected input");
			m_pPos += 1;
			return GetNextToken();
	}
}

CConstValue &
CHtfeLex::GetConstValue()
{
	// convert string to CConstValue container
	char **ppStr=0;
	const char *pStr = GetString().c_str();

	bool bIsSigned = true;
	bool bValueTooLarge = false;
	uint64_t value = 0;
	bool bNeg = false;
	if (m_token == tk_num_int) {
		uint64_t lastValue = 0;
		if (bNeg = *pStr == '-')
			pStr += 1;
		while (isdigit(*pStr)) {
			value = value * 10 + *pStr - '0';
			bValueTooLarge |= value / 10 != lastValue;
			lastValue = value;
			pStr += 1;
		}
	} else if (m_token == tk_num_hex) {
		bIsSigned = false;
		Assert(*pStr == '0' && (pStr[1] == 'x' || pStr[1] == 'X'));
		pStr += 2;
		while (isxdigit(*pStr)) {
			if (value > ULLONG_MAX / 16)
				bValueTooLarge = true;
			if (*pStr >= '0' && *pStr <= '9')
				value = value * 16 + *pStr - '0';
			else if (*pStr >= 'A' && *pStr <= 'F')
				value = value * 16 + *pStr - 'A' + 10;
			else if (*pStr >= 'a' && *pStr <= 'f')
				value = value * 16 + *pStr - 'a' + 10;
			pStr += 1;
		}
	} else if (m_token == tk_num_string)
		value = strtol(GetString().c_str(), ppStr, 2);
	else {
		Assert(0);
		return m_constValue;
	}

	if (pStr[0] == 'u' || pStr[0] == 'U') {
		pStr += 1;
		bIsSigned = false;
	}

	bool bIsLongLong = false;
	if (pStr[0] == 'l' && pStr[1] == 'l' || pStr[0] == 'L' && pStr[1] == 'L') {
		pStr += 2;
		bIsLongLong = true;
	}
	else if (pStr[0] == 'l' || pStr[0] == 'L') {
		pStr += 1;
		bIsLongLong = true;
	}

	if (pStr[0] == 'u' || pStr[0] == 'U') {
		pStr += 1;
		bIsSigned = false;
	}

	if (bIsSigned)
		if (bIsLongLong) {
			if (value & 0x8000000000000000LL)
				bValueTooLarge = true;
			if (bNeg) value = ~value + 1;
			m_constValue.SetSint64() = value;
		} else {
			if (value & 0xffffffff80000000LL)
				bValueTooLarge = true;
			if (bNeg) value = ~value + 1;
			m_constValue.SetSint32() = (int32_t)value;
		}
	else
		if (bIsLongLong) {
			if (bNeg) value = ~value + 1;
			m_constValue.SetUint64() = value;
		} else {
			if (value & 0xffffffff00000000LL)
				bValueTooLarge = true;
			if (bNeg) value = ~value + 1;
			m_constValue.SetUint32() = (uint32_t)value;
		}

	if (bValueTooLarge)
		ParseMsg(PARSE_ERROR, "integer too large (%s)", GetString().c_str());

	return m_constValue;
}

bool
CHtfeLex::GetSint32ConstValue(int &value)
{
	// convert string to 32-bit integer
	CConstValue constValue = GetConstValue();

	value = 1;
	if (constValue.GetSint64() > 0x7fffffff)
		return false;

	value = (int)constValue.GetSint64();
	return true;
}

void
CHtfeLex::SkipWhite()
{
	while (*m_pPos == ' ' || *m_pPos == '\t') m_pPos += 1;
}

size_t
CHtfeLex::RecordTokens()
{
	// create a list of tokens until ForLoopTokenPlayback is called
    m_tokenRecord.push_back(CTokenRecord());

	m_bTokenRecording = true;

    return m_tokenRecord.back().m_tokenList.size();
}

void
CHtfeLex::RecordTokensPause()
{
    m_bTokenRecording = false;
}

size_t
CHtfeLex::RecordTokensResume()
{
    m_bTokenRecording = true;

    return m_tokenRecord.back().m_tokenList.size();
}

void
CHtfeLex::RecordTokensErase(size_t pos)
{
    m_tokenRecord.back().m_tokenList.erase(m_tokenRecord.back().m_tokenList.begin()+pos, m_tokenRecord.back().m_tokenList.end());
    m_bTokenRecording = false;
}

void CHtfeLex::RemoveLastRecordedToken()
{
	if (m_tokenRecord.size() == 1) {
		m_bRemovedToken = true;
		m_removedToken = m_tokenRecord.back().m_tokenList.back();
	} else if (m_token != tk_eor)
		m_tokenRecord[m_tokenPlaybackLevel].m_playbackPos -= 1;

	if (m_token != tk_eor) {
		size_t size = m_tokenRecord.back().m_tokenList.size();
		m_tokenRecord.back().m_tokenList.erase(m_tokenRecord.back().m_tokenList.begin() + size - 1, m_tokenRecord.back().m_tokenList.end());
	}
}

void
CHtfeLex::FuncDeclPlayback()
{
	m_tokenRecord.back().m_value = 0;
	m_tokenRecord.back().m_exitValue = 0;
	m_tokenRecord.back().m_incValue = 0;
	m_tokenRecord.back().m_playbackPos = -1;

	m_bTokenRecording = false;

    if (m_tokenRecord.back().m_tokenList.size() > 0) {
	    m_bTokenPlayback = true;
	    m_tokenPlaybackLevel += 1;
    } else
	    m_tokenRecord.pop_back();

	GetNextToken();
}

void
CHtfeLex::FuncDeclPlaybackExit()
{
	Assert(m_tokenPlaybackLevel == -1);
	m_tokenRecord.pop_back();
	m_bTokenRecording = false;

    if (m_bTokenPlayback) {
	    m_bTokenPlayback = false;
        GetNextToken();
    }
}

void
CHtfeLex::ForLoopTokenPlayback(string &ctrlVarName, bool bUnsignedCtrlVar, int64_t initValue, int64_t exitValue, int64_t incValue)
{
	m_tokenRecord.back().m_ctrlVarName = ctrlVarName;
	m_tokenRecord.back().m_bUnsignedCtrlVar = bUnsignedCtrlVar;
	m_tokenRecord.back().m_value = (int)initValue;
	m_tokenRecord.back().m_exitValue = (int)exitValue;
	m_tokenRecord.back().m_incValue = (int)incValue;
	m_tokenRecord.back().m_playbackPos = -1;

	m_bTokenRecording = false;
	m_bTokenPlayback = true;
	m_tokenPlaybackLevel += 1;

	GetNextToken();
}

void
CHtfeLex::ForLoopExit()
{
	Assert(m_tokenPlaybackLevel == -1);
	m_tokenRecord.pop_back();
	m_bTokenRecording = false;
	m_bTokenPlayback = false;

	if (m_bRemovedToken) {
		m_bRemovedToken = false;
		m_token = m_removedToken.GetToken();
		m_stringBuf = m_removedToken.GetString();
		m_lineInfo = m_removedToken.GetLineInfo();
	} else
		GetNextToken();
}
