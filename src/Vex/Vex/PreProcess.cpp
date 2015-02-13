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
#include "PreProcess.h"

CPreProcess::CPreProcess(const char *pFileName)
{
	if (!(m_fp = fopen(pFileName, "rb"))) {
		printf("Could not open verilog expansion input file '%s'\n", pFileName);
		exit(1);
	}

	m_pMsgFp = 0;
	m_statementFilePos = 0;
	m_statementLineNum = 1;
	m_bCommentSpansEol = false;
	m_expansionWidth = 0;
	m_expansionIndex = 0;
	m_bRepositionToLineStart = false;
	m_parseErrorCnt = 0;
	m_lineInfo.m_fileName = pFileName;
	m_lineInfo.m_lineNum = 0;
}

CPreProcess::~CPreProcess(void)
{
	if (m_fp)
		fclose(m_fp);
}

bool
CPreProcess::GetLine(char *pLine, int maxLineLen) {
	for (;;) {
		if (m_bRepositionToLineStart) {
			// replay last statement with next index
			m_bRepositionToLineStart = false;
			fseek(m_fp, m_statementFilePos, SEEK_SET);
			m_lineInfo.m_lineNum = m_statementLineNum-1;
		}

		m_lineFilePos = ftell(m_fp);
		if (!fgets(m_lineBuf, maxLineLen, m_fp))
			return false;

		m_lineInfo.m_lineNum += 1;

		const char *pSrc = m_lineBuf;
		char *pDst = pLine;
		while (*pSrc != '\0') {
			if (*pSrc == '/' || m_bCommentSpansEol) {
				if (pSrc[1] == '*' || m_bCommentSpansEol) {
					if (!m_bCommentSpansEol)
						pSrc += 2;
					while (pSrc[0] != '\0' && (pSrc[0] != '*' || pSrc[1] != '/'))
						pSrc += 1;
					if (pSrc[0] == '\0') {
						m_bCommentSpansEol = true;
						*pDst = '\0';
						return true;
					}
					m_bCommentSpansEol = false;
					pSrc += 2;
					continue;
				} else if (pSrc[1] == '/') {
					*pDst = '\0';
					return true;
				}
			} else if (pSrc[0] == '`') {
				if (pSrc == m_lineBuf)
					break;	// get next line

				pSrc += 1;

				if (*pSrc == '{') {
					//int expWidth = 0;
					vector<string> expandedList;
					if (ExpandString(pSrc, expandedList)) {
						size_t expWidth = expandedList.size();
					//if (ExpandString(expWidth, pSrc, pDst)) {
						if (m_expansionWidth == 0)
							m_expansionWidth = expWidth;
						else if (m_expansionWidth != expWidth) {
							ParseMsg(PARSE_ERROR, "preprocessor unequal expansion widths within statement, %d and %d",
								m_expansionWidth, expWidth);
							m_expansionWidth = m_expansionWidth < expWidth ? m_expansionWidth : expWidth;
						}

						const char *pStr = expandedList[(m_expansionWidth-1)-m_expansionIndex].c_str();
						while (*pStr != '\0')
							*pDst++ = *pStr++;
					}
					continue;
				}
			} else if (pSrc[0] == ';') {
				m_expansionIndex += 1;
				if (m_expansionWidth == 0 || (int)m_expansionWidth == m_expansionIndex) {
					// advance to next statement
					m_expansionIndex = 0;
					m_expansionWidth = 0;
					m_statementFilePos = m_lineFilePos + (int)(pSrc - m_lineBuf) + 1;
					m_statementLineNum = m_lineInfo.m_lineNum;
				} else {
					m_bRepositionToLineStart = true;
					*pDst++ = *pSrc++;
					*pDst = '\0';
					return true;
				}
			}

			*pDst++ = *pSrc++;
		}

		if (*pSrc == '\0') {
			*pDst = '\0';
			return true;
		}
	}
}

void
CPreProcess::AdvanceStatementFilePos()
{
	m_statementFilePos = ftell(m_fp);
	m_statementLineNum = m_lineInfo.m_lineNum+1; // file position is after newline, must add one
}

bool
CPreProcess::ExpandString(const char *&pSrc, vector<string> &expandedList)
{
	// parse expansion specifier

	assert(*pSrc == '{');
	pSrc += 1;

	for (;;) {
		int expCount = 0;
		while (*pSrc == ' ') pSrc += 1;
		const char *pBase = pSrc;
		if (isdigit(*pSrc)) {
			int base = GetNum(pSrc);
			if (*pSrc == ':') {
				// number expansion
				pSrc += 1;
				if (!isdigit(*pSrc) && !(*pSrc == '-' && isdigit(pSrc[1]))) {
					ParseMsg(PARSE_ERROR, "expected increment number in preprocessor expansion");
					SkipToRBrace(pSrc);
					return false;
				}
				int inc = GetNum(pSrc);

				int incBase = 0;
				int incDiv = 1;
				if (*pSrc == '-' || *pSrc == '+') {
					incBase = inc;
					inc = GetNum(pSrc);
					if (*pSrc == '/') {
						pSrc += 1;
						incDiv = GetNum(pSrc);
					}
				}
				if (*pSrc++ != ':') {
					ParseMsg(PARSE_ERROR, "expected ':' after increment number in preprocessor expansion");
					SkipToRBrace(pSrc);
					return false;
				}
				if (!isdigit(*pSrc)) {
					ParseMsg(PARSE_ERROR, "expected width number in preprocessor expansion");
					SkipToRBrace(pSrc);
					return false;
				}
				expCount = GetNum(pSrc);
				if (*pSrc != ',' && *pSrc != '}') {
					ParseMsg(PARSE_ERROR, "expected ',' or '}' after numeric preprocessor expansion");
					SkipToRBrace(pSrc);
					return false;
				}

				char expBuf[64];
				for (int i = expCount-1; i >= 0; i -= 1) {
					int value = base + (incBase + i * inc) / incDiv;
					if (value < 0) {
						ParseMsg(PARSE_ERROR, "negative numeric preprocessor expansion");
						SkipToRBrace(pSrc);
						return false;
					}
					sprintf(expBuf, "%d", value);
					expandedList.push_back(expBuf);
				}
				
			} else if (*pSrc == '{') {
				int startIdx = (int)expandedList.size();
				if (!ExpandString(pSrc, expandedList))
					return false;

				int expCnt = (int)expandedList.size() - startIdx;
				if (*pSrc != ',' && *pSrc != '}') {
					ParseMsg(PARSE_ERROR, "expected ',' or '}' after numeric preprocessor expansion");
					SkipToRBrace(pSrc);
					return false;
				}

				for (int i = 1; i < base; i += 1) {
					for (int j = 0; j < expCnt; j += 1) {
						expandedList.push_back(expandedList[startIdx + j]);
					}
				}
				expCount = expCnt * base;

			} else if (*pSrc == '\'') {
				int width = base;
				pSrc += 1;
				char baseCh = *pSrc++;
				if (baseCh != 'b' && baseCh != 'h') {
					ParseMsg(PARSE_ERROR, "unknown base for constant '%s'", pBase);
					SkipToRBrace(pSrc);
					return false;
				}
				uint64 value = 0;
				while (baseCh == 'b' && (*pSrc == '0' || *pSrc == '1') || baseCh == 'h' && isxdigit(*pSrc)) {
					if (baseCh == 'b')
						value = value * 2 + *pSrc - '0';
					else if (isdigit(*pSrc))
						value = value * 16 + *pSrc - '0';
					else
						value = value * 16 + tolower(*pSrc) - 'a' + 10;
					pSrc += 1;
				}
				for (int i = width-1; i >= 0; i -= 1)
					expandedList.push_back(((value >> i) & 1) == 1 ? "1'b1" : "1'b0");
				expCount = width;
			} else
				pSrc = pBase;
		}

		size_t startIdx = expandedList.size();
		char srcBuf[256];
		char *pBuf = srcBuf;
		while (*pSrc != ',' && *pSrc != '}' && *pSrc != '\0') {
			if (*pSrc == '{') {
				*pBuf = '\0';
				string bufStr = srcBuf;
				pBuf = srcBuf;

				vector<string> localExpList;
				if (!ExpandString(pSrc, localExpList))
					return false;

				if (expCount == 0) {
					expCount = (int)localExpList.size();
					for (int i = 0; i < expCount; i += 1)
						expandedList.push_back(bufStr + localExpList[i]);
				} else {
					if (expCount != (int)localExpList.size()) {
						ParseMsg(PARSE_ERROR, "preprocessor inconsistent expansion widths");
						SkipToRBrace(pSrc);
						return false;
					}
					for (int i = 0; i < expCount; i += 1)
						expandedList[startIdx + i] += bufStr + localExpList[i];
				}
				continue;
			}

			*pBuf++ = *pSrc++;
		}

		*pBuf = '\0';

		if (expCount == 0)
			expandedList.push_back(srcBuf);
		else if (srcBuf[0] != '\0') {
			for (int i = 0; i < expCount; i += 1)
				expandedList[startIdx + i] += srcBuf;
		}

		if (*pSrc == '\0') {
			ParseMsg(PARSE_ERROR, "missing '}' for proprocessor expansion");
			SkipToRBrace(pSrc);
			return false;
		}

		if (*pSrc++ == '}')
			break;
	}

	return true;
}

bool
CPreProcess::ExpandString(int &posIdx, const char *&pSrc, char *&pDst)
{
	// parse expansion specifier

	assert(*pSrc == '{');
	pSrc += 1;

	if (isdigit(*pSrc)) {
		const char *pBase = pSrc;
		int base = GetNum(pSrc);
		if (*pSrc == ':') {
			// number expansion
			pSrc += 1;
			if (!isdigit(*pSrc)) {
				ParseMsg(PARSE_ERROR, "expected increment number in preprocessor expansion");
				SkipToRBrace(pSrc);
				return false;
			}
			int inc = GetNum(pSrc);
			if (*pSrc++ != ':') {
				ParseMsg(PARSE_ERROR, "expected ':' after increment number in preprocessor expansion");
				SkipToRBrace(pSrc);
				return false;
			}
			if (!isdigit(*pSrc)) {
				ParseMsg(PARSE_ERROR, "expected width number in preprocessor expansion");
				SkipToRBrace(pSrc);
				return false;
			}
			int width = GetNum(pSrc);
			if (*pSrc++ != '}') {
				ParseMsg(PARSE_ERROR, "expected closing '}' in preprocessor expansion");
				SkipToRBrace(pSrc);
				return false;
			}

			if (posIdx <= m_expansionIndex && m_expansionIndex < posIdx + width) {
				int value = base + (m_expansionIndex - posIdx) * inc;
				char numBuf[32];
				char *pNum = numBuf+32;
				*--pNum = '\0';
				do {
					*--pNum = '0' + value % 10;
					value /= 10;
				} while (value > 0);
				while (*pNum)
					*pDst++ = *pNum++;
			}

			posIdx += width;
			*pDst = '\0';
			return true;
		} if (*pSrc == '{') {
			if (!ExpandString(posIdx, pSrc, pDst))
				return false;
		} else
			pSrc = pBase;
	}

	for (;;) {
		char *pStart = pDst;
		int startPosIdx = posIdx;
		while (*pSrc != ',' && *pSrc != '}' && *pSrc != '\0') {
			if (*pSrc == '{') {
				if (!ExpandString(posIdx, pSrc, pDst))
					return false;
				posIdx -= 1;
			}
			*pDst++ = *pSrc++;
		}
		if (*pSrc == '\0') {
			ParseMsg(PARSE_ERROR, "missing '}' for proprocessor expansion");
			SkipToRBrace(pSrc);
			return false;
		}
		posIdx += 1;
		if (m_expansionIndex < startPosIdx || posIdx <= m_expansionIndex)
			pDst = pStart;
		if (*pSrc++ == '}')
			break;
	}

	*pDst = '\0';
	return true;
}

int
CPreProcess::GetNum(const char *&pStr)
{
	int value = 0;
	int sign = 1;
	if (*pStr == '-') {
		sign = -1;
		pStr += 1;
	} else if (*pStr == '+')
		pStr += 1;

	while (isdigit(*pStr))
		value = value * 10 + *pStr++ - '0';
	return sign * value;
}

void
CPreProcess::SkipToRBrace(const char *&pStr)
{
	while (*pStr != '}' && *pStr != '\0')
		pStr += 1;
	if (*pStr == '}')
		pStr += 1;
}

void
CPreProcess::ParseMsg(EParseMsgType msgType, const char *msgStr, ...)
{
	va_list marker;
	va_start( marker, msgStr );

	CLineInfo lineInfo = GetLineInfo();

	ParseMsg(msgType, lineInfo, msgStr, marker);
}

void
CPreProcess::ParseMsg(EParseMsgType msgType, const CLineInfo &lineInfo, const char *msgStr, ...)
{
	va_list marker;
	va_start( marker, msgStr );

	ParseMsg(msgType, lineInfo, msgStr, marker);
}

void
CPreProcess::ParseMsg(EParseMsgType msgType, const CLineInfo &lineInfo, const char *msgStr, va_list marker)
{
	m_parseErrorCnt += 1;

	char buf1[256];
	sprintf(buf1, "%s(%d) : %s:",
	lineInfo.m_fileName.c_str(), lineInfo.m_lineNum, msgType == PARSE_WARNING ? "Warning" : "Error");

	char buf2[256];
	vsprintf(buf2, msgStr, marker);

	printf("%s %s\n", buf1, buf2);

	if (m_pMsgFp)
		fprintf(m_pMsgFp, "%s %s\n", buf1, buf2);

	if (msgType == PARSE_FATAL) {
		printf("%s(%d) : Fatal: unable to proceed due to previous errors\n",
			lineInfo.m_fileName.c_str(), lineInfo.m_lineNum);
		exit(0);
	}

	if (m_parseErrorCnt > 100) {
		printf("%s(%d) : Error: maximum error count exceeded\n",
			lineInfo.m_fileName.c_str(), lineInfo.m_lineNum);
		exit(0);
	}
}
