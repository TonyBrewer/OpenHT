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

#if defined(_WIN32) || defined (NO_LIC_CHK)
# define convey_lic_ck_rl(x)
#else
extern "C" {
    extern int convey_lic_ck_rl(const char *);
}
#endif

void ConveyHtNodeLockedLic();

int main(int argc, char const **argv)
{
#if defined(_WIN32)
	ConveyHtNodeLockedLic();
#else
	convey_lic_ck_rl("Convey_HT");
#endif
	g_appArgs.Parse(argc, argv);

	CDsnInfo dsnInfo;

	for (int i = 0; i < g_appArgs.GetInputFileCnt(); i += 1)
		dsnInfo.loadHtdFile(g_appArgs.GetInputFile(i));

	dsnInfo.LoadDefineList();

	dsnInfo.loadHtiFile(g_appArgs.GetInstanceFile());

	if (CLex::GetParseErrorCnt() > 0)
		CPreProcess::ParseMsg(Fatal, "Previous errors prevent generation of files");

	// initialize and validate dsnInfo
	dsnInfo.InitializeAndValidate();

	dsnInfo.DrawModuleCmdRelationships();

	dsnInfo.GenPersFiles();

	g_appArgs.GetDsnRpt().GenApiEnd();
	g_appArgs.GetDsnRpt().GenCallGraph();

	dsnInfo.ReportRamFieldUsage();

#ifdef _WIN32
	dsnInfo.GenMsvsProjectFiles();
#endif

	return 0;
}

#ifdef _WIN32

#include <stdio.h>
#include <Windows.h>
#include <Iphlpapi.h>
#include <Assert.h>
#pragma comment(lib, "iphlpapi.lib")

void ConveyHtNodeLockedLic()
{
	PIP_ADAPTER_INFO AdapterInfo;
	DWORD dwBufLen = sizeof(AdapterInfo);
	char *mac_addr = (char*)malloc(17);

	AdapterInfo = (IP_ADAPTER_INFO *) malloc(sizeof(IP_ADAPTER_INFO));
	if (AdapterInfo == NULL) {
		printf("Error allocating memory needed to call GetAdaptersinfo\n");

	}

	// Make an initial call to GetAdaptersInfo to get the necessary size into the dwBufLen     variable
	if (GetAdaptersInfo(AdapterInfo, &dwBufLen) == ERROR_BUFFER_OVERFLOW) {

		AdapterInfo = (IP_ADAPTER_INFO *) malloc(dwBufLen);
		if (AdapterInfo == NULL) {
			printf("Error allocating memory needed to call GetAdaptersinfo\n");
		}
	}

	if (GetAdaptersInfo(AdapterInfo, &dwBufLen) == NO_ERROR) {
		PIP_ADAPTER_INFO pAdapterInfo = AdapterInfo;// Contains pointer to current adapter info
		do {
			sprintf(mac_addr, "%02X:%02X:%02X:%02X:%02X:%02X",
				pAdapterInfo->Address[0], pAdapterInfo->Address[1],
				pAdapterInfo->Address[2], pAdapterInfo->Address[3],
				pAdapterInfo->Address[4], pAdapterInfo->Address[5]);

			//printf("Address: %s, mac: %s\n", pAdapterInfo->IpAddressList.IpAddress.String, mac_addr);

			pAdapterInfo = pAdapterInfo->Next;        
		}while(pAdapterInfo);                        
	}
	free(AdapterInfo);
}

#endif
