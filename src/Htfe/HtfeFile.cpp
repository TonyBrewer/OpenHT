/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
// HtFile.cpp: implementation of the CHtFile class.
//
//////////////////////////////////////////////////////////////////////

#include "Htfe.h"
#include "HtfeErrorMsg.h"
#include "HtfeFile.h"

#ifndef _MSC_VER
#include <sys/types.h>
#include <unistd.h>
#define _lseek lseek
#define _read read
#define _unlink unlink
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CHtFile::CHtFile()
{
  m_indentLevel = 0;
  m_bNewLine = false;
  m_bDoNotSplitLine = false;
  m_dstFp = 0;
  m_lineCol = 0;
  m_lineNum = 1;
  m_maxLineCol = 70;
  m_bLineBuffering = false;
  m_lineListIdx = 0;
  m_bVarInit = false;
  m_bInTask = false;
}

bool
CHtFile::Open(const string &outputFileName, char *pMode)
{
	m_fileName = outputFileName;
	m_dstFp = fopen(outputFileName.c_str(), pMode);

	int lastSlash = outputFileName.find_last_of("/\\");
	if (lastSlash > 0)
		m_folder = outputFileName.substr(0,lastSlash);
	else
		m_folder = ".";

	return m_dstFp != 0;
}

void CHtFile::Close() {
	Assert(m_dstFp);
	fclose(m_dstFp);
	m_dstFp = 0;
}

void CHtFile::Delete() {
	if (m_dstFp) {
		fclose(m_dstFp);
		_unlink(m_fileName.c_str());
	}
	m_dstFp = 0;
}

void
CHtFile::Dup(int srcFd, int startOffset, int endOffset)
{
  int origOffset = _lseek(srcFd, 0, SEEK_CUR);

  _lseek(srcFd, startOffset, SEEK_SET);

  char buffer[4096];
  for (int offset = startOffset; offset < endOffset; ) {
    int readSize = endOffset - offset;
    if (readSize > 4096)
      readSize = 4096;

    int readBytes = _read(srcFd, buffer, readSize);
    if (readBytes <= 0)
      break;
    
    fwrite(buffer, 1, readBytes, m_dstFp);

    offset += readBytes;
  }

  _lseek(srcFd, origOffset, SEEK_SET);
}

void CHtFile::PrintVarInit(const char *format, ...)
{
	int savedIndentLevel = GetIndentLevel();
	SetIndentLevel(m_varInitIndentLevel);
	m_bVarInit = true;

	va_list marker;
	va_start(marker, format);     /* Initialize variable arguments. */
	Print_(format, marker);

	m_bVarInit = false;
	SetIndentLevel(savedIndentLevel);
}

int CHtFile::Print(const char *format, ...)
{
	va_list marker;
	va_start(marker, format);     /* Initialize variable arguments. */
	return Print_(format, marker);
}

int CHtFile::Print_(const char *format, va_list marker)
{
	if (m_indentLevel == -1) {
		int r = vfprintf(m_dstFp, format, marker);
		for (const char *pCh = format; *pCh; pCh += 1)
			if (*pCh == '\n')
				m_lineNum += 1;
		return r;
	} else {
		char buf[4096];
		int r = vsprintf(buf, format, marker);

		// a newline in the input forces a new line in the output
		// the formated string can be split where a space exists or at the boundary

		bool bEol = false;
		char *pStrStart = buf;
		for (;;) {
			if (m_bNewLine) {
				int level = m_indentLevel;
				for (int i = 0; i < level; i += 1)
					Putc(' ');
				m_lineCol = m_indentLevel;
				m_bNewLine = false;
			}

			// find next string terminated by a ' ', '\t', '\n' or '\0'
			char *pStrEnd = pStrStart;
			if (pStrStart[0] == '/' && pStrStart[1] == '/') {
				// comments must be handled together to avoid wrapping to a newline
				while (*pStrEnd != '\n' && *pStrEnd != '\r' && *pStrEnd != '\0')
					pStrEnd++;
			} else {
				while (*pStrEnd != ' ' && *pStrEnd != '\t' && *pStrEnd != '\n' && *pStrEnd != '\r' && *pStrEnd != '\0')
					pStrEnd++;
			}

			m_bNewLine = *pStrEnd == '\n' || *pStrEnd == '\r';
			bEol = *pStrEnd == '\0';
			bool bSpace = *pStrEnd == ' ';
			bool bTab = *pStrEnd == '\t';
			*pStrEnd = '\0';

			// check if string can fit in current line
			int strLen = strlen(pStrStart);
			if (!m_bDoNotSplitLine && m_indentLevel < m_lineCol && m_lineCol + strLen > m_maxLineCol) {
				// string does not fit, start new line
				Putc('\n');
				m_lineNum += 1;
				for (int i = 0; i < m_indentLevel+1; i += 1)
					Putc(' ');
				m_lineCol = m_indentLevel+1;
			}

			if (strLen > 0)
				Puts(pStrStart);
			m_lineCol += strLen;

			if (!m_bNewLine && bSpace) Putc(' ');
			if (!m_bNewLine && bTab) Putc('\t');

			pStrStart = pStrEnd+1;

			if (m_bNewLine) {
				Putc('\n');
				m_lineNum += 1;
			}

			while (!bEol && *pStrStart == '\n') {
				pStrStart += 1;
				Putc('\n');
				m_lineNum += 1;
			}

			if (bEol || *pStrStart == '\0') {
				m_bDoNotSplitLine = false;
				return r;
			}
		}
	}
}

void CHtFile::Putc(char ch)
{
	if (m_bVarInit) {
		m_varInitBuffer += ch;
		if (ch == '\n') {
			m_varInitLineList.push_back(m_varInitBuffer);
			m_varInitBuffer.clear();
		}
	} else if (m_bLineBuffering) {
		m_lineBuffer += ch;
		if (ch == '\n') {
			m_lineList[m_lineListIdx].push_back(m_lineBuffer);
			m_lineBuffer.clear();
		}
	} else 
		fputc(ch, m_dstFp);
}

void CHtFile::Puts(char * pStr)
{
	if (m_bVarInit)
		m_varInitBuffer += pStr;
	else if (m_bLineBuffering)
		m_lineBuffer += pStr;
	else
		fputs(pStr, m_dstFp);
}

void CHtFile::FlushLineBuffer()
{
	for (size_t i = 0; i < m_lineList[m_lineListIdx].size(); i += 1)
		fputs(m_lineList[m_lineListIdx][i].c_str(), m_dstFp);

	m_lineList[m_lineListIdx].clear();
}

void CHtFile::VarInitClose()
{
	if (m_bInTask) {
		// first insert task's temp var declarations
		m_lineList[m_varInitListIdx].insert(m_lineList[m_varInitListIdx].begin() + m_varInitLineIdx,
			m_lineList[2].begin(), m_lineList[2].end());
		m_varInitLineIdx += m_lineList[2].size();
		m_lineList[2].clear();
	//	SetLineBuffer(0);
	}

	m_lineList[m_varInitListIdx].insert(m_lineList[m_varInitListIdx].begin() + m_varInitLineIdx,
		m_varInitLineList.begin(), m_varInitLineList.end());
	m_varInitLineList.clear();
}
