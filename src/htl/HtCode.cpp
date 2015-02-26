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
#include "HtCode.h"

CHtFile * CHtFile::m_pHeadOpenFile = 0;
vector<string> CHtFile::m_writeFileList;

void ErrorExit()
{
	// fatal error, close all open files, delete written files and exit
	CHtFile::DeleteWrittenFiles();
	exit(1);
}
