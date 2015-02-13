/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */

#include "Htt.h"
#include "HttDesign.h"

CHttDesign::CHttDesign()
{
	m_genCppStartOffset = 0;
}

void CHttDesign::OpenFiles()
{
	m_pathName = g_httArgs.GetInputPathName();
	int slashPos = m_pathName.find_last_of("\\/");
	if (slashPos > 0)
		m_pathName.erase(slashPos+1);
	else
		m_pathName = "";

	if (g_httArgs.IsScHier()) {
		printf("  Inappropriate input for Htt (files with suffix .sc)\n");
		exit(1);
	}

	if (!OpenInputFile(g_httArgs.GetInputPathName())) {
		printf("  Could not open input file '%s', exiting\n", g_httArgs.GetInputPathName().c_str());
		exit(1);
	}

	if (!m_cppFile.Open(g_httArgs.GetCppOutFileName().c_str(), "wt")) {
		printf("Could not open cpp file '%s'\n", g_httArgs.GetCppOutFileName().c_str());
		exit(1);
	}

	if (!m_incFile.Open(g_httArgs.GetIncOutFileName().c_str(), "wt")) {
		printf("Could not open .h file '%s'\n", g_httArgs.GetIncOutFileName().c_str());
		exit(1);
	}
}

void CHttDesign::CloseFiles()
{
	m_cppFile.Close();
	m_incFile.Close();
}

void CHttDesign::HandleInputFileInit()
{
	if (g_httArgs.IsScHier())
		m_incFile.Dup(GetInputFd(), m_genCppStartOffset, GetInputFileOffset());
}

void CHttDesign::HandleScMainInit()
{
	m_cppFile.Dup(GetInputFd(), m_genCppStartOffset, GetInputFileOffset());
}

void CHttDesign::HandleScMainStart()
{
	m_genCppStartOffset = GetInputFileOffset();
}
