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

#define RD_BUF_SIZE     512

class CFileBkup
{
public:
    CFileBkup(const string pathName, const string fileName, const string fileExt);
    ~CFileBkup(void);

    void Print(const char *msgStr, ...);
    void Write(const char *pBuf);
    void Close() { fclose(m_fp); }
    void PrintHeader(const char *pComment);

private:
    string  m_pathName;
    string  m_fileName;
    string  m_fileExt;

    FILE    *m_fp;
    int     m_rdBufIdx;
    char    m_rdBuf[RD_BUF_SIZE];
    long	m_fpos;
    bool    m_bWrFile;
};
