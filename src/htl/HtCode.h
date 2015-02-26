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

#include <vector>
#include <string>
#include <queue>

#ifndef _MSC_VER
#include <sys/types.h>
#include <unistd.h>
#define _lseek lseek
#define _read read
#define _unlink unlink
#endif

using namespace std;

// Class that maintains a list of all open files and a list of files that have been written
//  If an error occurs while generating output files then the open files are closed and
//  then all files there were written are deleted.

class CHtFile {
public:
	CHtFile() : m_pFile(0), m_bOpenedFile(false) {}
	CHtFile(FILE *fp) : m_bOpenedFile(false)
	{
		m_pFile = fp;
	}
	CHtFile(string name, char const * pMode) : m_bOpenedFile(false)
	{
		FileOpen(name, pMode);
	}

	~CHtFile()
	{
		if (m_bOpenedFile && m_pFile)
			FileClose();
	}

	operator FILE * () { return m_pFile; }

	FILE * GetFile() { return m_pFile; }

	void FileOpen(string name, char const *pMode)
	{
		HtlAssert(strcmp(pMode, "w") == 0 || strcmp(pMode, "r") == 0);

		bool bWrite = strcmp(pMode, "w") == 0;

		if (!(m_pFile = fopen(name.c_str(), pMode))) {
			fprintf(stderr, "Could not open file %s for %s", name.c_str(), bWrite ? "writing" : "reading");
			ErrorExit();
		}

		m_pNextOpenFile = m_pHeadOpenFile;
		m_pHeadOpenFile = this;
		m_bOpenedFile = true;

		if (bWrite)
			m_writeFileList.push_back(name);
	}

	void FileClose()
	{
		HtlAssert(m_pFile != 0);
		fclose(m_pFile);
		m_pFile = 0;

		// search open file list
		CHtFile ** ppFile = &m_pHeadOpenFile;
		while (*ppFile && *ppFile != this)
			ppFile = &(*ppFile)->m_pNextOpenFile;

		HtlAssert(*ppFile != 0);

		// remove from open file list
		*ppFile = (*ppFile)->m_pNextOpenFile;
	}

	static void CloseOpenFiles()
	{
		CHtFile * pFile = m_pHeadOpenFile;
		while (pFile) {
			pFile->FileClose();
			pFile = pFile->m_pNextOpenFile;
		}
	}

	static void DeleteWrittenFiles()
	{

		// first close all open files
		CloseOpenFiles();

		// now unlink files that were written
		for (size_t i = 0; i < m_writeFileList.size(); i += 1)
			_unlink(m_writeFileList[i].c_str());
	}

private:
	FILE * m_pFile;
	bool m_bOpenedFile;
	CHtFile * m_pNextOpenFile;

private:
	static vector<string> m_writeFileList;
	static CHtFile * m_pHeadOpenFile;
};

// Buffer lines of generated code until time to write them to a file

class CHtCode : public CHtFile {
public:
	CHtCode()
	{
		m_bFile = false;
	}
	CHtCode(FILE *fp) : CHtFile(fp)
	{
		m_bFile = true;
	}
	void Open(string name)
	{
		FileOpen(name, "w");
		m_bFile = true;
	}
	void Close()
	{
		HtlAssert(m_bFile == true);
		FileClose();
	}
	void Append(const char *format, ...);
	void Write(FILE *fp, string indent = "");
	void NewLine()
	{
		if (m_code.size() > 0)
			Append("\n");
	}
	bool Empty() { return m_code.size() == 0; }

private:
	bool			m_bFile;
	queue<string>	m_code;
};

inline void CHtCode::Append(const char *format, ...)
{
	va_list marker;

	va_start(marker, format);     /* Initialize variable arguments. */

	if (m_bFile)
		vfprintf(GetFile(), format, marker);
	else {
		char buf[4096];
		vsprintf(buf, format, marker);

		//if (strncmp(buf, "\tuint8_t c_final_flag[8];\n", 16) == 0)
		//	bool stop = true;

		bool bNewLine = strcmp(buf, "\n") == 0;

		if (!bNewLine || !m_code.empty() && m_code.back() != "\n")
			m_code.push(buf);
	}
}

inline void CHtCode::Write(FILE *fp, string indent)
{
	if (m_code.size() == 0 || m_code.size() == 1 && m_code.front() == "\n")
		return;

	bool bEmptyLine = false;
	while (!m_code.empty()) {
		if (!bEmptyLine || m_code.front() != "\n") {
			if (indent.length() != 0 && m_code.front().c_str()[0] != '#') {
				fwrite(indent.c_str(), indent.length(), 1, fp);
			}
			fwrite(m_code.front().c_str(), m_code.front().size(), 1, fp);
		}
		bEmptyLine = m_code.front() == "\n";
		m_code.pop();
	}
	if (!bEmptyLine)
		fwrite("\n", 1, 1, fp);
}
