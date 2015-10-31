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
#include "AppArgs.h"
#include "DsnInfo.h"

void SigHandler(int signo) { HtlAssert(0); }
void InstallSigHandler() { signal(SIGSEGV, SigHandler); }

CDsnInfo * g_pDsnInfo = 0;

int main(int argc, char const **argv)
{
//	InstallSigHandler();

	g_appArgs.Parse(argc, argv);

	g_pDsnInfo = new CDsnInfo;

	for (int i = 0; i < g_appArgs.GetInputFileCnt(); i += 1)
		g_pDsnInfo->loadHtdFile(g_appArgs.GetInputFile(i));

	g_pDsnInfo->LoadDefineList();

	g_pDsnInfo->LoadHtiFile(g_appArgs.GetInstanceFile());

	if (CLex::GetParseErrorCnt() > 0)
		CPreProcess::ParseMsg(Fatal, "Previous errors prevent generation of files");

	// initialize and validate dsnInfo
	g_pDsnInfo->InitializeAndValidate();

	g_pDsnInfo->DrawModuleCmdRelationships();

	g_pDsnInfo->GenPersFiles();

	g_appArgs.GetDsnRpt().GenApiEnd();
	g_appArgs.GetDsnRpt().GenCallGraph();

	g_pDsnInfo->ReportRamFieldUsage();

#ifdef _WIN32
	g_pDsnInfo->GenMsvsProjectFiles();
#endif

	return 0;
}
