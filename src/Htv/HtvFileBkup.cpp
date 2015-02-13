/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "Htv.h"
#include "HtvFileBkup.h"
#include "HtfeErrorMsg.h"

CFileBkup::CFileBkup(const string pathName, const string fileName, const string fileExt)
{
    m_pathName = pathName;
    m_fileName = fileName;
    m_fileExt = fileExt;

    // Open file for reading
    string name = pathName + fileName + "." + fileExt;

	if (m_fp = fopen(name.c_str(), "rb")) {
		m_rdBufIdx = m_fp ? RD_BUF_SIZE : 0;
		m_bWrFile = false;
		m_fpos = 0;
	} else {
		if (!(m_fp = fopen(name.c_str(), "wb"))) {
			fprintf(stderr, "Could not open '%s' for write\n", name.c_str());
			ErrorExit();
		}
		m_bWrFile = true;
	}
}

CFileBkup::~CFileBkup(void)
{
}

void
CFileBkup::Print(const char *msgStr, ...)
{
	va_list marker;
	va_start( marker, msgStr );

	char buf[256];
	vsprintf(buf, msgStr, marker);

	Write(buf);
}

void
CFileBkup::Write(const char *pWrBuf)
{
    while (*pWrBuf && !m_bWrFile) {
		if (m_fp) {
			if (m_rdBufIdx == RD_BUF_SIZE) {
				m_fpos = ftell(m_fp);
				fread(m_rdBuf, RD_BUF_SIZE, 1, m_fp);
				m_rdBufIdx = 0;
			}
			if (*pWrBuf == m_rdBuf[m_rdBufIdx]) {
				m_rdBufIdx += 1;
				pWrBuf += 1;
				continue;
			}
			fclose(m_fp);
		}
        m_bWrFile = true;
        m_fpos += m_rdBufIdx;

        // file being written is different than original file
        //   rename original file as a backup
		time_t seconds;
		time(&seconds);

        char timeBuf[32];
		sprintf(timeBuf, "_%d.", (int)seconds);

        string origName = m_pathName + m_fileName + "." + m_fileExt;
        string bkupName = m_pathName + "HtvBkup/" + m_fileName + timeBuf + m_fileExt;
		rename(origName.c_str(), bkupName.c_str());

        //   copy backup file to original file name to start of difference
        FILE *srcFp = fopen(bkupName.c_str(), "rb");
        m_fp = fopen(origName.c_str(), "wb");

        while (m_fpos > 0) {
            size_t rdLen = (size_t)(m_fpos > RD_BUF_SIZE ? RD_BUF_SIZE : m_fpos);
            m_fpos -= rdLen;

            fread(m_rdBuf, rdLen, 1, srcFp);
            fwrite(m_rdBuf, rdLen, 1, m_fp);
        }
        fclose(srcFp);
        break;
    }

    fwrite(pWrBuf, strlen(pWrBuf), 1, m_fp);
}

void
CFileBkup::PrintHeader(const char *pComment)
{
	Print("// %s\n", pComment);
	Print("//\n");
	Print("//////////////////////////////////////////////////////////////////////\n");
	Print("\n");
}
