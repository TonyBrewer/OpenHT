/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
// PreProcess.cpp: implementation of the CPreProcess class.
//
//////////////////////////////////////////////////////////////////////

#include "CnyHt.h"
#include "AppArgs.h"
#include "PreProcess.h"

#if !defined(_MSC_VER)
#include <unistd.h>
#include <limits.h>
#endif

CLineInfo CPreProcess::m_lineInfo;
int CPreProcess::m_warningCnt = 0;
int CPreProcess::m_errorCnt = 0;
MTRand_int32 CPreProcess::m_mtRand;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CPreProcess::CPreProcess()
{
	m_pWriteFp = 0;
	m_bPreProc = false;
	m_fileCnt = 0;

	m_fsIdx = -1;
	m_pFs = &m_fileStack[0];
	m_pFs -= 1;

	m_bInComment = false;
	m_bSkipLines = false;

	m_directiveTbl.Insert("define")->SetDirective(dir_define);
	m_directiveTbl.Insert("undef")->SetDirective(dir_undef);
	m_directiveTbl.Insert("include")->SetDirective(dir_include);
	m_directiveTbl.Insert("pragma")->SetDirective(dir_pragma);
	m_directiveTbl.Insert("if")->SetDirective(dir_if);
	m_directiveTbl.Insert("else")->SetDirective(dir_else);
	m_directiveTbl.Insert("elif")->SetDirective(dir_elif);
	m_directiveTbl.Insert("endif")->SetDirective(dir_endif);
	m_directiveTbl.Insert("ifdef")->SetDirective(dir_ifdef);
	m_directiveTbl.Insert("ifndef")->SetDirective(dir_ifndef);

	m_macroTbl.Insert(string("_HTV"));
	m_macroTbl.Insert(string("__LINE__"));

	struct timeval st;
	gettimeofday(&st, NULL);
	int seed = st.tv_sec * 100000 + st.tv_usec;
	m_mtRand.seed(seed);

	char buf[64];
	sprintf(buf, "0x%x", seed);
	string expansion = buf;

	m_macroTbl.Insert(string("__SEED__"))->SetExpansion(expansion);

	//WritePreProcessedInput();
}

CPreProcess::~CPreProcess()
{
	if (m_pWriteFp)
		fclose(m_pWriteFp);
}

bool CPreProcess::Open(const string &name, bool bProcessOnce)
{
	// check if fileName must be replaced
	string fileName = name;
	if (fileName == m_interceptFileName)
		fileName = m_replaceFileName;

	string pathName;
	char fullPathName[PATH_MAX];

	if (m_pFs < m_fileStack) {
#if defined(_MSC_VER)
		m_pFs[1].m_fd = _open(fileName.c_str(), _O_RDONLY | _O_TEXT);
#else
		m_pFs[1].m_fd = open(fileName.c_str(), O_RDONLY);
#endif
		if (m_pFs[1].m_fd < 0)
			return false;

		pathName = fileName;

#ifdef _MSC_VER
		GetFullPathName(pathName.c_str(), 1024, fullPathName, 0);
#else
		realpath(pathName.c_str(), fullPathName);
#endif

	} else {
		// try file stack [0]'s folder name
		pathName = m_fileStack[0].m_lineInfo.m_pathName;
		int lastSlash = pathName.find_last_of("/\\");
		if (false && lastSlash >= 0) {
			// if there is a slash in the filename, replace the existing file name
			//  with the file to open
			pathName.resize(lastSlash);

			pathName += "/" + fileName;
		} else {
			// if there was no slash in the fileName, just use the fileName as
			//  the file to open
			pathName = fileName;
		}

#ifdef _MSC_VER
		GetFullPathName(pathName.c_str(), 1024, fullPathName, 0);
#else
		realpath(pathName.c_str(), fullPathName);
#endif

		if (bProcessOnce) {
			for (unsigned int i = 0; i < m_pragmaOnceList.size(); i += 1)
				if (m_pragmaOnceList[i] == fullPathName)
					return true;
		}

#if defined(_MSC_VER)
		m_pFs[1].m_fd = _open(pathName.c_str(), _O_RDONLY | _O_TEXT);
#else
		m_pFs[1].m_fd = open(pathName.c_str(), O_RDONLY);
#endif
		if (m_pFs[1].m_fd < 0) {
#if defined(_MSC_VER)
			m_pFs[1].m_fd = _open(fileName.c_str(), _O_RDONLY | _O_TEXT);
#else
			m_pFs[1].m_fd = open(fileName.c_str(), O_RDONLY);
#endif
			if (m_pFs[1].m_fd < 0)
				return false;
		}
	}

	m_fileCnt += 1;
	m_fsIdx += 1;
	m_pFs = &m_fileStack[m_fsIdx];
	m_pFs->m_lineInfo.m_pathName = fullPathName;
	m_pFs->m_lineInfo.m_fileName = fullPathName;
	m_pFs->m_lineInfo.m_fileNum = m_fileCnt;

	int pos = m_pFs->m_lineInfo.m_fileName.find_last_of("\\/");
	if (pos >= 0)
		m_pFs->m_lineInfo.m_fileName.erase(0, pos+1);

	m_pFs->m_lineInfo.m_lineNum = 0;

    UpdateStaticLineInfo();

	m_pFs->m_pDefine = 0;
	m_pFs->m_pRdBufPos = m_pFs->m_pRdBufEnd = ""; // force a read
	m_pFs->m_fileRdOffset = 0;  // read position in file 

	m_pFs->m_bSkipStack.resize(0);
	m_pFs->m_bElseStack.resize(0);
	m_pFs->m_bSkipToEndif.resize(0);
	m_pFs->m_bSkipStack.push_back(false);
	m_pFs->m_bElseStack.push_back(false);
	m_pFs->m_bSkipToEndif.push_back(false);

	return true;
}

bool CPreProcess::Reopen()
{
	// start from beginning for first file
	m_bPreProc = false;
	m_fsIdx = 0;
	m_pFs = &m_fileStack[m_fsIdx];
	m_bInComment = false;
	m_bSkipLines = false;

#if defined(_MSC_VER)
	m_pFs[0].m_fd = _open(m_pFs->m_lineInfo.m_pathName.c_str(), _O_RDONLY | _O_TEXT);
#else
	m_pFs[0].m_fd = open(m_pFs->m_lineInfo.m_pathName.c_str(), O_RDONLY);
#endif

	if (m_pFs[0].m_fd < 0)
		return false;

	m_pFs->m_lineInfo.m_lineNum = 0;

    UpdateStaticLineInfo();

	m_pFs->m_pDefine = 0;
	m_pFs->m_pRdBufPos = m_pFs->m_pRdBufEnd = ""; // force a read
	m_pFs->m_fileRdOffset = 0;  // read position in file 

	m_pFs->m_bSkipStack.resize(0);
	m_pFs->m_bElseStack.resize(0);
	m_pFs->m_bSkipToEndif.resize(0);
	m_pFs->m_bSkipStack.push_back(false);
	m_pFs->m_bElseStack.push_back(false);
	m_pFs->m_bSkipToEndif.push_back(false);

	return true;
}

void
	CPreProcess::Close()
{
#if defined(_MSC_VER)
	_close(m_pFs->m_fd);
#else
	close(m_pFs->m_fd);
#endif
	m_fsIdx -= 1;
	m_pFs = &m_fileStack[m_fsIdx];
}

bool CPreProcess::GetLine(string &lineBuf)
{
	for (;;) {  // loop until non-directive line found

		if (!GetLineWithCommentsStripped(lineBuf)) {
			// end of file, check that we are not in the middle of #if
			if (m_pFs->m_bSkipStack.size() != 1) {
				ParseMsg(Error, GetLineInfo(), "EOF - Expected #endif");
				while (m_pFs->m_bSkipStack.size() > 1) {
					m_pFs->m_bSkipStack.pop_back();
					m_pFs->m_bElseStack.pop_back();
					m_pFs->m_bSkipToEndif.pop_back();
				}
			}
			// close file and pop fileStack
			Close();
			if (m_pFs < m_fileStack)
				return false; // end of initial file
			else
				continue;
		}
		// fprintf(stderr, "{%s}\n", lineBuf.c_str());

		// process directives
		const char *pPos = lineBuf.c_str();
		SkipSpace(pPos);
		if (*pPos++ != '#') {
			if (m_bSkipLines)
				continue;
			else {
				PerformMacroExpansion(lineBuf);
				if (m_pWriteFp)
					fprintf(m_pWriteFp, "%s\n", lineBuf.c_str());

				return true;
			}
		}

		// found a directive
		SkipSpace(pPos);
		if (*pPos == '\0')
			continue;  // null directive;

		if (!m_bSkipLines) {
			switch (m_directiveTbl.Find(ParseIdentifier(pPos))) {
			case dir_define:
				PreProcessDefine(lineBuf, pPos);
				break;
			case dir_undef:
				PreProcessUndef(lineBuf, pPos);
				break;
			case dir_include:
				PreProcessInclude(lineBuf, pPos);
				break;
			case dir_if:
				m_bSkipLines |= PreProcessIf(lineBuf, pPos);
				m_pFs->m_bSkipStack.push_back(m_bSkipLines);
				m_pFs->m_bElseStack.push_back(false);	// else has not been seen
				m_pFs->m_bSkipToEndif.push_back(!m_bSkipLines);
				break;
			case dir_ifdef:
				m_bSkipLines |= PreProcessIfdef(lineBuf, pPos);
				m_pFs->m_bSkipStack.push_back(m_bSkipLines);
				m_pFs->m_bElseStack.push_back(false);	// else has not been seen
				m_pFs->m_bSkipToEndif.push_back(!m_bSkipLines);
				break;
			case dir_ifndef:
				m_bSkipLines |= PreProcessIfndef(lineBuf, pPos);
				m_pFs->m_bSkipStack.push_back(m_bSkipLines);
				m_pFs->m_bElseStack.push_back(false);	// else has not been seen
				m_pFs->m_bSkipToEndif.push_back(!m_bSkipLines);
				break;
			case dir_else:
				if (m_pFs->m_bSkipStack.size() <= 1 ||
					m_pFs->m_bElseStack.back() == true) {
						ParseMsg(Error, "Unexpected #else directive");
						break;
				}
				m_bSkipLines = true;
				m_pFs->m_bSkipStack.back() = true;
				m_pFs->m_bElseStack.back() = true;
				m_pFs->m_bSkipToEndif.back() = true;
				break;
			case dir_elif:
				if (m_pFs->m_bSkipStack.size() <= 1 ||
					m_pFs->m_bElseStack.back() == true) {
						ParseMsg(Error, "Unexpected #elif directive");
						break;
				}
				m_bSkipLines = true;
				m_pFs->m_bSkipStack.back() = true;
				m_pFs->m_bElseStack.back() = false;
				m_pFs->m_bSkipToEndif.back() = true;
				break;
			case dir_endif:
				if (m_pFs->m_bSkipStack.size() <= 1) {
					ParseMsg(Error, "Unexpected #endif directive");
					break;
				}
				m_pFs->m_bSkipStack.pop_back();
				m_pFs->m_bElseStack.pop_back();
				m_pFs->m_bSkipToEndif.pop_back();
				break;
			case dir_pragma:
				{
					SkipSpace(pPos);
					string pragmaName = ParseIdentifier(pPos);
					if (pragmaName == "once") {
						//printf("OnceList <= %s\n", m_pFs->m_lineInfo.m_pathName.c_str());
						m_pragmaOnceList.push_back(m_pFs->m_lineInfo.m_pathName);

					} else if (pragmaName == "htl") {

						//pPos -= 10;
						//memcpy((void *)pPos, "pragma_htl", 10);
						lineBuf += "#";
						return true;

					} else if (pragmaName == "message") {
						SkipSpace(pPos);
						while (*pPos != '\0') {
							if (*pPos == '"') {
								pPos++;
								while (*pPos != '"' && *pPos != '\n' && *pPos != '\0') {
									if (*pPos == '\\') {
										pPos+= 1;
										switch (*pPos++) {
										case 'n': putchar('\n'); break;
										}
									} else
										putchar(*pPos++);
								}
								if (*pPos != '\0') pPos += 1;
							} else {
								// check for a macro to expand
								string name = ParseIdentifier(pPos);

								CMacro *pMacro;
								if (pMacro = m_macroTbl.Find(name)) {
									const char *pValueStr = pMacro->GetExpansion().c_str();
									int value;
									GetNextToken(pValueStr, false);
									if (!ParseExpression(pValueStr, value, false))
										fputs(pMacro->GetExpansion().c_str(), stdout);
									else
										printf("0x%x", value);
								} else
									break;
							}
							SkipSpace(pPos);
						}
						putchar('\n');
					}
				}
				break;  // Ignore
			default:
				ParseMsg(Error, "Unknown preprocessing directive");
				return true;
			}
		} else {
			switch (m_directiveTbl.Find(ParseIdentifier(pPos))) {
			case dir_if:
			case dir_ifdef:
			case dir_ifndef:
				m_pFs->m_bSkipStack.push_back(true);
				m_pFs->m_bElseStack.push_back(false);	// else has not been seen
				m_pFs->m_bSkipToEndif.push_back(true);
				break;
			case dir_else:
				if (m_pFs->m_bElseStack.back() == true)
					ParseMsg(Error, "Unexpected #else directive");

				m_pFs->m_bElseStack.back() = true;
				if (m_pFs->m_bSkipStack[m_pFs->m_bSkipStack.size()-2] == false
					&& m_pFs->m_bSkipToEndif.back() == false) 
				{
					m_pFs->m_bSkipStack.back() = false;
					m_bSkipLines = false;
				}

				break;
			case dir_elif:
				if (m_pFs->m_bElseStack.back() == true)
					ParseMsg(Error, "Unexpected #elif directive");

				if (m_pFs->m_bSkipStack[m_pFs->m_bSkipStack.size()-2] == false
					&& m_pFs->m_bSkipToEndif.back() == false) 
				{
					m_pFs->m_bSkipStack.back() = false;
					m_bSkipLines = false;

					// do if
					m_bSkipLines |= PreProcessIf(lineBuf, pPos);
					m_pFs->m_bSkipStack.back() = m_bSkipLines;
					m_pFs->m_bElseStack.back() = false;	// else has not been seen
					m_pFs->m_bSkipToEndif.back() = !m_bSkipLines;
				}
				break;
			case dir_endif:
				m_pFs->m_bElseStack.pop_back();
				m_pFs->m_bSkipStack.pop_back();
				m_pFs->m_bSkipToEndif.pop_back();
				m_bSkipLines = m_pFs->m_bSkipStack.back();
				break;
			default: break;
			}
		}
	}

	PerformMacroExpansion(lineBuf);

	if (m_pWriteFp)
		fprintf(m_pWriteFp, "%s\n", lineBuf.c_str());

	return true;
}

void CPreProcess::SkipSpace(const char *&pPos)
{
	while (*pPos == ' ' || *pPos == '\t')
		pPos += 1;
}

string CPreProcess::ParseIdentifier(const char *&pPos)
{
	string identifier = "";
	const char *pInitPos = pPos;
	if (*pPos != '_' && !isalpha(*pPos))
		return identifier;

	while (*pPos == '_' || isalnum(*pPos))
		pPos += 1;

	identifier.assign(pInitPos, pPos-pInitPos);
	return identifier;
}

string CPreProcess::ParseString(const char *&pPos)
{
	string identifier = "";

	if (*pPos++ != '"')
		return identifier;

	const char *pInitPos = pPos;
	while (*pPos != '\0' && *pPos != '"') pPos += 1;
	if (*pPos != '"')
		return identifier;

	identifier.assign(pInitPos, pPos - pInitPos);
	pPos += 1;
	return identifier;
}

void CPreProcess::PerformMacroExpansion(string &lineBuf)
{
	// find identifiers in line and check for macro names
	const char *pPos = lineBuf.c_str();
	string identifier;

	while (*pPos != '\0') {
		if (*pPos != '_' && !isalpha(*pPos)) {
			pPos += 1;
			continue;
		}

		CMacro *pMacro;
		int startPos = pPos - lineBuf.c_str();
		identifier = ParseIdentifier(pPos);
		if (!(pMacro = m_macroTbl.Find(identifier)))
			continue;

		int endPos = pPos - lineBuf.c_str();

		SkipSpace(pPos);
		if (*pPos != '(' && pMacro->IsParenReqd())
			continue; // macro requires arguments, do not expand

		string expansion = pMacro->GetExpansion();

		if (identifier == "__LINE__") {
			char buf[64];
			(void)sprintf(buf, "%d", GetLineInfo().m_lineNum);
			expansion = buf;
		}

		if (pMacro->IsParenReqd()) {
			HtlAssert(*pPos == '(');
			pPos += 1;
			// get arguments
			vector<string> argList;
			int argId = 0;
			for (;;) {
				while (*pPos == ' ' || *pPos == '\t')
					pPos += 1;
				const char *pStartArgPos = pPos;
				int parenLvl = 0;
				while (parenLvl > 0 || (*pPos != ',' && *pPos != ')')) {
					if (*pPos == '\0') {
						const char *origLineBuf = lineBuf.c_str();
						GetLineWithCommentsStripped(lineBuf, true);
						pPos += lineBuf.c_str() - origLineBuf;
						pStartArgPos += lineBuf.c_str() - origLineBuf;
					} else if (*pPos == '(')
						parenLvl += 1;
					else if (*pPos == ')')
						parenLvl -= 1;
					pPos += 1;
				}
				while (pPos > pStartArgPos && (*(pPos-1) == ' ' || *pPos == '\t'))
					pPos -= 1;
				if (*pPos == '\0') {
					ParseMsg(Error, "Macro argument error");
					return;
				}
				if (pPos != pStartArgPos) {
					string argument(pStartArgPos, pPos - pStartArgPos);

					argList.push_back(argument);	// add to end of list
					argId += 1;
				}

				while (*pPos == ' ' || *pPos == '\t')
					pPos += 1;

				if (*pPos != ',')
					break;

				pPos += 1;
				while (*pPos == ' ' || *pPos == '\t')
					pPos += 1;
			}
			if (*pPos++ != ')' || argId != pMacro->GetParamCnt()) {
				ParseMsg(Error, "Macro argument error");
				return;
			}
			endPos = pPos - lineBuf.c_str();

			// first expand macro with arguments

			ExpandMacro(pMacro, argList, expansion);
		}

		// now replace macro in lineBuf
		lineBuf.replace(startPos, endPos-startPos, expansion);

		m_linePos.Delete(startPos, endPos-startPos);
		m_linePos.Insert(startPos, expansion.length());

		pPos = lineBuf.c_str() + startPos;
	}
}

void CPreProcess::ExpandMacro(CMacro *pMacro, vector<string> &argList, string &expansion)
{
	// find identifiers in line and check for macro names
	const char *pPos = expansion.c_str();
	string identifier;

	while (*pPos != '\0') {
		if (*pPos != '_' && !isalpha(*pPos)) {
			pPos += 1;
			continue;
		}

		//CParam *pParam;
		int startPos = pPos - expansion.c_str();
		identifier = ParseIdentifier(pPos);

		int paramId;
		if ((paramId = pMacro->FindParamId(identifier)) < 0)
			continue;

		int endPos = pPos - expansion.c_str();

		// now replace macro in lineBuf
		expansion.replace(startPos, endPos-startPos,
			argList[paramId]);

		pPos = expansion.c_str() + startPos + argList[paramId].size();
	}
}

void CPreProcess::PreProcessDefine(string &lineBuf, const char *&pPos)
{
	// pPos points just after directive
	SkipSpace(pPos);
	string defineName = ParseIdentifier(pPos);
	if (defineName.length() == 0) {
		ParseMsg(Error, "Missing define name");
		return;
	}
	CMacro *pDefine = m_macroTbl.Insert(defineName);

	if (!pDefine->IsNew()) {
		ParseMsg(Warning, "Redefinition of %s", defineName.c_str());
		ParseMsg(Info, pDefine->GetLineInfo(), "     Original definition");
	}

	// check if define has parameters
	if (*pPos == '(') {
		pDefine->SetIsParenReqd(true);

		for (;;) {
			pPos += 1;
			SkipSpace(pPos);

			if (*pPos == ')')
				break;

			string paramName = ParseIdentifier(pPos);
			if (paramName.length() == 0) {
				ParseMsg(Error, "Parameter name error");
				return;
			}
			if (!pDefine->InsertParam(paramName)) {
				ParseMsg(Error, "Macro has redundant parameter name '%s'", paramName.c_str());
				return;
			}
			SkipSpace(pPos);

			if (*pPos != ',')
				break;
		}

		if (*pPos++ != ')') {
			ParseMsg(Error, "Parameter list error");
			return;
		}
	}

	// save expansion string
	//  white space is deleted before and after string
	SkipSpace(pPos);

	// find last non-space character
	const char *pEndPos = lineBuf.c_str() + lineBuf.length();
	while (pEndPos[-1] == ' ' || pEndPos[-1] == '\t')
		pEndPos -= 1;

	string expansion(pPos, (pEndPos < pPos) ? 0 : pEndPos - pPos);

	//  if string starts with a number, then one space is appended
	if (isdigit(*pPos))
		expansion.resize(pEndPos - pPos + 1, ' ');

	size_t n = expansion.find("__RANDOM__");
	if (n != std::string::npos) {
		char buf[64];
		sprintf(buf, "0x%x", (unsigned int)m_mtRand());
		expansion.replace(n, 10, buf);
	}

	if (defineName == "__SEED__") {
		GetNextToken(pPos);
		int seed;
		if (!ParseExpression(pPos, seed))
			ParseMsg(Error, "#define of __SEED__ must have constant expression");
		else
			m_mtRand.seed(seed);
	}

	pDefine->SetExpansion(expansion);
	pDefine->SetIsNew(false);
	pDefine->SetLineInfo(GetLineInfo());
	pDefine->SetIsFromIncludeFile(m_fsIdx > 0);
}

void CPreProcess::PreProcessUndef(string &lineBuf, const char *&pPos)
{
	// pPos points just after directive
	SkipSpace(pPos);
	string undefName = ParseIdentifier(pPos);
	if (undefName.length() == 0) {
		ParseMsg(Error, "Missing undef name");
		return;
	}
	m_macroTbl.Remove(undefName);
}

void CPreProcess::PreProcessInclude(string &lineBuf, const char *&pPos)
{
	// pPos points just after directive
	SkipSpace(pPos);

	if (*pPos == '<')
		return; // ignore system include files

	if (*pPos != '"') {
		ParseMsg(Error, "Include file error");
		return;
	}

	string fileName = ParseString(pPos);

	if (fileName.length() == 0) {
		ParseMsg(Error, "Include file error");
		return;
	}

	for (size_t i = 0; i < g_appArgs.GetIncludeDirs().size(); i += 1) {
		string pathName = g_appArgs.GetIncludeDirs()[i] + fileName;

		if (Open(pathName, true)) {
			if (m_fsIdx == 1) {

				char fullPathName[PATH_MAX];

#				ifdef _MSC_VER
					GetFullPathName(pathName.c_str(), 1024, fullPathName, 0);
#				else
					realpath(pathName.c_str(), fullPathName);
#				endif

				m_includeList.push_back(CIncludeFile(fullPathName, pathName));
			}
			return;
		}
	}

	if (!Open(fileName, true))
		ParseMsg(Error, "Could not open file");
	else if (m_fsIdx == 1) {
		char fullPathName[PATH_MAX];

#		ifdef _MSC_VER
			GetFullPathName(fileName.c_str(), 1024, fullPathName, 0);
#		else
			realpath(fileName.c_str(), fullPathName);
#		endif

		m_includeList.push_back(CIncludeFile(fullPathName, fileName));
	}
}

bool CPreProcess::PreProcessIf(string &lineBuf, const char *&pPos)
{
	// pPos points just after directive
	GetNextToken(pPos); // get first token
	int rtnValue;
	if (!ParseExpression(pPos, rtnValue))
		return false;
	else
		return rtnValue == 0; // return true if skip lines


	bool bNot = false;
	if (*pPos == '!') {
		pPos += 1;
		bNot = true;
		SkipSpace(pPos);
	}

	string identifier = ParseIdentifier(pPos);
	if (identifier.length() == 0) {
		ParseMsg(Error, "Unknown #if directive format");
		return false;
	}

	if (identifier == "defined") {
		SkipSpace(pPos);

		bool bLParen = false;
		if (*pPos == '(') {
			pPos += 1;
			bLParen = true;
			SkipSpace(pPos);
		}

		identifier = ParseIdentifier(pPos);
		if (identifier.length() == 0) {
			ParseMsg(Error, "Unknown IF directive format");
			return false;
		}

		SkipSpace(pPos);
		if (bLParen) {
			if (*pPos++ != ')') {
				ParseMsg(Error, "Unknown IF directive format");
				return false;
			}
			SkipSpace(pPos);
		}

		if (*pPos != '\0') {
			ParseMsg(Error, "Unknown IF directive format");
			return false;
		}

		return (m_macroTbl.Find(identifier) == 0) != bNot;
	} else {
		SkipSpace(pPos);

		if (*pPos == '>')
			return false;

		ParseMsg(Error, "Unknown IF directive format");
		return false;
	}
}

bool CPreProcess::ParseExpression(const char *&pPos, int &rtnValue, bool bErrMsg)
{
	SkipSpace(pPos);

	vector<int> operandStack;
	vector<int> operatorStack;
	operatorStack.push_back(tk_exprBegin);

	EToken2 tk;
	for (;;) {
		// first parse an operand
		tk = GetToken();
		if (tk == tk_error) return false;

		if (tk == tk_bang) {
			EvaluateExpression(tk, operandStack, operatorStack);
			tk = GetNextToken(pPos);
			if (tk == tk_error) return false;
		}

		if (tk == tk_num_int || tk == tk_num_hex) {
			operandStack.push_back(GetTokenValue());
		} else if (GetToken() == tk_identifier) {

			if (GetTokenString() == "defined") {
				if (GetNextToken(pPos) != tk_lparen) {
					ParseMsg(Error, "expected '('");
					return false;
				}
				if (GetNextToken(pPos) != tk_identifier) {
					ParseMsg(Error, "expected an identifier");
					return false;
				}
				if (GetNextToken(pPos) != tk_rparen) {
					ParseMsg(Error, "expected a ')'");
					return false;
				}
				operandStack.push_back(m_macroTbl.Find(GetTokenString()) == 0 ? 0 : 1);

			} else {
				CMacro *pMacro;
				if (!(pMacro = m_macroTbl.Find(GetTokenString())))
					operandStack.push_back(0);
				else {
					// convert macro to an integer value
					const char *pValueStr = pMacro->GetExpansion().c_str();

					//if (GetToken() != tk_rparen) {
					//	ParseMsg(Error, "expected ')'");
					//	return false;
					//}

					GetNextToken(pValueStr);

					if (!ParseExpression(pValueStr, rtnValue))
						return false;
					operandStack.push_back(rtnValue);

					//char *pStopPos;
					//int value = strtol(pValueStr, &pStopPos, 0);
					//while (*pStopPos == ' ')
					//	pStopPos += 1;

					//if (pMacro->GetExpansion().length() != pStopPos - pValueStr) {
					//	ParseMsg(Error, "could not convert macro name to value '%s='%s'\n",
					//		GetTokenString().c_str(), pMacro->GetExpansion().c_str());
					//	return false;
					//}
					//operandStack.push_back(value);
				}
			}

		} else if (GetToken() == tk_lparen) {
			GetNextToken(pPos);

			if (!ParseExpression(pPos, rtnValue))
				return false;
			operandStack.push_back(rtnValue);

			if (GetToken() != tk_rparen) {
				ParseMsg(Error, "expected ')'");
				return false;
			}
		} else {
			if (bErrMsg)
				ParseMsg(Error, "expected an operand");
			return false;
		}

		switch (GetNextToken(pPos)) {
		case tk_exprEnd:
		case tk_rparen:
			EvaluateExpression(tk_exprEnd, operandStack, operatorStack);

			if (operandStack.size() != 1)
				ParseMsg(Error, "Evaluation error, operandStack");
			else if (operatorStack.size() != 0)
				ParseMsg(Error, "Evaluation error, operatorStack");

			rtnValue = operandStack.back();
			return true;
		case tk_plus:
		case tk_minus:
		case tk_slash:
		case tk_asterisk:
		case tk_percent:
		case tk_equalEqual:
		case tk_vbar:
		case tk_vbarVbar:
		case tk_carot:
		case tk_ampersand:
		case tk_ampersandAmpersand:
		case tk_bangEqual:
		case tk_less:
		case tk_greater:
		case tk_greaterEqual:
		case tk_lessEqual:
		case tk_lessLess:
		case tk_greaterGreater:
			EvaluateExpression(GetToken(), operandStack, operatorStack);
			GetNextToken(pPos);
			break;
		default:
			if (bErrMsg)
				ParseMsg(Error, "Unknown operator type");
			return false;
		}
	}
}

void
	CPreProcess::EvaluateExpression(EToken2 tk, vector<int> &operandStack,
	vector<int> &operatorStack)
{
	int op1, op2, rslt;

	for (;;) {
		int tkPrec = GetTokenPrec(tk);
		EToken2 stackTk = (EToken2)operatorStack.back();
		int stackPrec = GetTokenPrec(stackTk);

		if (tkPrec > stackPrec /*|| tkPrec == stackPrec && IsTokenAssocLR(tk)*/) {
			operatorStack.pop_back();

			if (stackTk == tk_exprBegin) {
				HtlAssert(operandStack.size() == 1 && operatorStack.size() == 0);
				return;
			} else if (stackTk == tk_unaryPlus || stackTk == tk_unaryMinus ||
				stackTk == tk_tilda || stackTk == tk_bang) {

					// get operand off stack
					op1 = operandStack.back();
					operandStack.pop_back();

					switch (stackTk) {
					case tk_unaryPlus:
						rslt = op1;
						break;
					case tk_unaryMinus:
						rslt = -op1;
						break;
					case tk_tilda:
						rslt = ~op1;
						break;
					case tk_bang:
						rslt = !op1;
						break;
					default:
						HtlAssert(0);
					}
			} else {

				// binary operators
				int depth = operandStack.size();
				HtlAssert(depth >= 2);
				op2 = operandStack.back();
				operandStack.pop_back();
				op1 = operandStack.back();
				operandStack.pop_back();

				switch (stackTk) {
				case tk_plus:
					rslt = op1 + op2;
					break;
				case tk_minus:
					rslt = op1 - op2;
					break;
				case tk_slash:
					rslt = op1 / op2;
					break;
				case tk_asterisk:
					rslt = op1 * op2;
					break;
				case tk_percent:
					rslt = op1 % op2;
					break;
				case tk_vbar:
					rslt = op1 | op2;
					break;
				case tk_carot:
					rslt = op1 ^ op2;
					break;
				case tk_ampersand:
					rslt = op1 & op2;
					break;
				case tk_lessLess:
					rslt = op1 << op2;
					break;
				case tk_greaterGreater:
					rslt = op1 >> op2;
					break;
				case tk_equalEqual:
					rslt = op1 == op2;
					break;
				case tk_vbarVbar:
					rslt = op1 || op2;
					break;
				case tk_ampersandAmpersand:
					rslt = op1 && op2;
					break;
				case tk_bangEqual:
					rslt = op1 != op2;
					break;
				case tk_less:
					rslt = op1 < op2;
					break;
				case tk_greater:
					rslt = op1 > op2;
					break;
				case tk_greaterEqual:
					rslt = op1 >= op2;
					break;
				case tk_lessEqual:
					rslt = op1 <= op2;
					break;
				default:
					HtlAssert(0);
				}
			}

			operandStack.push_back(rslt);

		} else {
			// push operator on stack
			operatorStack.push_back(tk);
			break;
		}
	}
}

int
	CPreProcess::GetTokenPrec(EToken2 tk) {
		switch (tk) {
		case tk_unaryPlus:
		case tk_unaryMinus:
		case tk_tilda:
		case tk_bang:
			return 3;
		case tk_asterisk:
		case tk_slash:
		case tk_percent:
			return 4;
		case tk_plus:
		case tk_minus:
			return 5;
		case tk_lessLess:
		case tk_greaterGreater:
			return 6;
		case tk_less:
		case tk_lessEqual:
		case tk_greater:
		case tk_greaterEqual:
			return 7;
		case tk_equalEqual:
		case tk_bangEqual:
			return 8;
		case tk_ampersand:
			return 9;
		case tk_carot:
			return 10;
		case tk_vbar:
			return 11;
		case tk_ampersandAmpersand:
			return 12;
		case tk_vbarVbar:
			return 13;
		case tk_exprBegin:
			return 17;
		case tk_exprEnd:
			return 18;
		default:
			HtlAssert(0);
			return 0;
		}
}


CPreProcess::EToken2 CPreProcess::GetNextToken(const char *&pPos, bool bErrMsg) 
{
	const char *pInitPos;

	while (*pPos == ' ' || *pPos == '\t') pPos += 1;
	switch (*pPos++) {
	case '!':
		if (*pPos == '=') {
			pPos += 1;
			return m_tk = tk_bangEqual;
		}
		return m_tk = tk_bang;
	case '|':
		if (*pPos == '|') {
			pPos += 1;
			return m_tk = tk_vbarVbar;
		}
		return m_tk = tk_vbar;
	case '&':
		if (*pPos == '&') {
			pPos += 1;
			return m_tk = tk_ampersandAmpersand;
		}
		return m_tk = tk_ampersand;
	case '(':
		return m_tk = tk_lparen;
	case ')':
		return m_tk = tk_rparen;
	case '+':
		return m_tk = tk_plus;
	case '-':
		return m_tk = tk_minus;
	case '/':
		return m_tk = tk_slash;
	case '*':
		return m_tk = tk_asterisk;
	case '%':
		return m_tk = tk_percent;
	case '^':
		return m_tk = tk_carot;
	case '~':
		return m_tk = tk_tilda;
	case '=':
		if (*pPos++ == '=')
			return m_tk = tk_equalEqual;
		return tk_error;
	case '<':
		if (*pPos == '<') {
			pPos += 1;
			return m_tk = tk_lessLess;
		}
		if (*pPos == '=') {
			pPos += 1;
			return m_tk = tk_lessEqual;
		}
		return m_tk = tk_less;
	case '>':
		if (*pPos == '>') {
			pPos += 1;
			return m_tk = tk_greaterGreater;
		}
		if (*pPos == '=') {
			pPos += 1;
			return m_tk = tk_greaterEqual;
		}
		return m_tk = tk_greater;
	case '\0':
		return m_tk = tk_exprEnd;
	default:
		pPos -= 1;
		if (isalpha(*pPos) || *pPos == '_') {
			pInitPos = pPos;
			while (isalnum(*pPos) || *pPos == '_') pPos += 1;
			m_tkString.assign(pInitPos, pPos-pInitPos);
			return m_tk = tk_identifier;
		}
		if (isdigit(*pPos)) {
			pInitPos = pPos;
			if (*pPos == '0' && pPos[1] == 'x') {
				pPos += 2;
				while (isxdigit(*pPos)) pPos += 1;
				m_tkString.assign(pInitPos, pPos-pInitPos);
				return m_tk = tk_num_hex;
			} else {
				while (isdigit(*pPos)) pPos += 1;
				m_tkString.assign(pInitPos, pPos-pInitPos);
				return m_tk = tk_num_int;
			}
		}
		if (bErrMsg)
			ParseMsg(Error, "unknown preprocess token type");
		return tk_error;
	}
}

int CPreProcess::GetTokenValue()
{
	const char *pStr = GetTokenString().c_str();
	unsigned int value = 0;

	if (m_tk == tk_num_int) {
		while (*pStr) {
			if (value >= 0xffffffff / 10) {
				ParseMsg(Error, "integer too large");
				return 0;
			}
			value = value * 10 + *pStr - '0';
			pStr += 1;
		}
		return value;
	} else if (m_tk == tk_num_hex) {
		while (*pStr) {
			if (value >= 0xffffffff / 16) {
				ParseMsg(Error, "integer too large");
				return 0;
			}
			if (*pStr >= '0' && *pStr <= '9')
				value = value * 16 + *pStr - '0';
			else if (*pStr >= 'A' && *pStr <= 'F')
				value = value * 16 + *pStr - 'A' + 10;
			else if (*pStr >= 'a' && *pStr <= 'f')
				value = value * 16 + *pStr - 'a' + 10;
			pStr += 1;
		}
		return value;
	} else {
		HtlAssert(0);
		return 0;
	}
}

bool
	CPreProcess::PreProcessIfdef(string &lineBuf, const char *&pPos)
{
	// pPos points just after directive
	SkipSpace(pPos);

	string identifier = ParseIdentifier(pPos);
	if (identifier.length() == 0) {
		ParseMsg(Error, "Unknown #ifdef directive format");
		return false;
	}

	SkipSpace(pPos);
	if (*pPos != '\0') {
		ParseMsg(Error, "Unknown #ifdef directive format");
		return false;
	}

	return m_macroTbl.Find(identifier) == 0; // skip lines if true
}

bool
	CPreProcess::PreProcessIfndef(string &lineBuf, const char *&pPos)
{
	// pPos points just after directive
	SkipSpace(pPos);

	string identifier = ParseIdentifier(pPos);
	if (identifier.length() == 0) {
		ParseMsg(Error, "Unknown #ifmdef directive format");
		return false;
	}

	SkipSpace(pPos);
	if (*pPos != '\0') {
		ParseMsg(Error, "Unknown #ifmdef directive format");
		return false;
	}

	return m_macroTbl.Find(identifier) != 0; // skip lines if true
}

bool CPreProcess::GetLineWithCommentsStripped(string &lineBuf, bool bContinue)
{
	size_t scanPos = 0;
	size_t commentPos = 0;

	do {
		for (;;) {
			if (!GetLineFromFile(lineBuf, bContinue || m_bInComment)) {
				if (bContinue)
					ParseMsg(Error, "File ended with continuation");
				else
					return false; // End of file
			}
			//printf("Line: %s\n", lineBuf.c_str());
			if (lineBuf.length() > 0 && lineBuf[lineBuf.length()-1] == '\\') {
				lineBuf.resize(lineBuf.length()-1);

				m_linePos.Delete(lineBuf.length(), 1);

				bContinue = true;
			} else
				break;
		}

		// scan line for comments
		do {
			if (!m_bInComment) {
				scanPos = lineBuf.find_first_of('/', scanPos);
				if (scanPos != string::npos) {
					if (lineBuf[scanPos+1] == '/') {
						// double slash comment found
						size_t len = lineBuf.length() - scanPos - 1;
						m_linePos.Delete(scanPos+1, len);

						lineBuf[scanPos] = ' ';
						lineBuf.resize(scanPos+1);
						break;
					} else if (lineBuf[scanPos+1] == '*') {
						commentPos = scanPos;
						scanPos += 2;
						m_bInComment = true;
					} else
						scanPos += 1;
				} else
					scanPos = lineBuf.length();
			}
			if (m_bInComment) {
				scanPos = lineBuf.find_first_of('*', scanPos);
				//if (scanPos < 0 || scanPos == lineBuf.length()-1) {
				if (scanPos == string::npos || scanPos == lineBuf.length()-1) {
					scanPos = lineBuf.length();
				} else if (lineBuf[scanPos+1] == '/') {
					// found end of comment
					m_bInComment = false;
					scanPos += 2;
					//VerifyFileOffset(commentPos, lineBuf);
					size_t len = scanPos - commentPos;
					lineBuf.replace(commentPos, len, len, ' ');
					//	  commentPos += len; // needed for VerifyFileOffset()
				} else
					scanPos += 1;
			}
		} while (scanPos < lineBuf.length());

	} while (m_bInComment);

	// comments have been replaces with spaces
	//VerifyFileOffset(commentPos, lineBuf);
	return true;
}

bool CPreProcess::GetLineFromFile(string &lineBuf, bool bAppendLine)
{
	int rdBytes;
	// copy bytes from m_rdBuf to line until end-of-line is found
	// may require refilling m_rdBuf

	if (!bAppendLine) {
		lineBuf.resize(0);

		// initialize line position info
		m_linePos.SetBaseOffset(m_pFs->m_fileRdOffset);
	}

	while (m_pFs->m_pRdBufPos == m_pFs->m_pRdBufEnd) {
		// fill buffer
#if defined(_MSC_VER)
		rdBytes = _read(m_pFs->m_fd, m_pFs->m_rdBuf, RDBUF_SIZE);
#else
		rdBytes = read(m_pFs->m_fd, m_pFs->m_rdBuf, RDBUF_SIZE);
#endif
		if (rdBytes <= 0)
			return false; // end of file
		// end-of-file, pop file stack

		m_pFs->m_pRdBufPos = m_pFs->m_rdBuf;
		m_pFs->m_pRdBufEnd = m_pFs->m_rdBuf + rdBytes;
	}

	const char *pPos = m_pFs->m_pRdBufPos;
	const char *pEnd = m_pFs->m_pRdBufEnd;

	// read buffer has data in it, copy characters to the line buffer
	const char *pInitPos = pPos;
	while (pPos == pEnd || *pPos != '\n') {
		if (pPos == pEnd) {
			// refill read buffer, first transfer scanned bytes to line buffer
			lineBuf.append(pInitPos, pPos-pInitPos);
#if defined(_MSC_VER)
			rdBytes = _read(m_pFs->m_fd, m_pFs->m_rdBuf, RDBUF_SIZE);
#else
			rdBytes = read(m_pFs->m_fd, m_pFs->m_rdBuf, RDBUF_SIZE);
#endif

			if (rdBytes < 0) {
				ParseMsg(Error, "File read error");
				return false;
			}

			m_pFs->m_fileRdOffset += pPos - pInitPos;
			pPos = m_pFs->m_pRdBufPos = m_pFs->m_rdBuf;
			pEnd = m_pFs->m_pRdBufEnd = m_pFs->m_rdBuf + rdBytes;
			pInitPos = pPos;

			if (pPos == pEnd) {
#if defined(_MSC_VER)
				HtlAssert(_eof(m_pFs->m_fd) == 1);
#else
				// FIXME - check eof status
#endif
				// end of file, break out of copy loop
				break;
			}

			continue;
		}
		if (*pPos == '\r') {
			// copy scanned rdBuf to lineBuf
			lineBuf.append(pInitPos, pPos-pInitPos);
			m_pFs->m_fileRdOffset += pPos - pInitPos + 1;

			m_linePos.Delete(lineBuf.length(), 1);
			pInitPos = pPos += 1;
		} else
			pPos += 1;
	}

	m_pFs->m_lineInfo.m_lineNum += 1;

    UpdateStaticLineInfo();

	if (pPos != pInitPos) {
		// this won't happen when '\r' preceeds '\n'
		lineBuf.append(pInitPos, pPos-pInitPos);
		m_pFs->m_fileRdOffset += pPos - pInitPos;
	}

	if (pPos < pEnd && *pPos == '\n') {
		pPos += 1;
		m_pFs->m_fileRdOffset += 1;
		m_linePos.Delete(lineBuf.length(), 1);
	}

	m_pFs->m_pRdBufPos = pPos;

	return true;
}

#include <stdarg.h>

string CPreProcess::FormatString(const char *msgStr, ...)
{
	va_list marker;
	va_start( marker, msgStr );

	char buf[1024];
	vsprintf(buf, msgStr, marker);

	return string(buf);
}


