/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
// LineInfo.h: interface for the CLineInfo class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

struct CLineInfo {
	int m_lineNum;
	int m_fileNum;
	string m_pathName;
	string m_fileName;
};
