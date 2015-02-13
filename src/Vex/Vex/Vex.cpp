/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
// Vex.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Design.h"

#if !defined(VERSION)
  #define VERSION "unknown"
#endif
#if !defined(BLDDTE)
  #define BLDDTE "unknown"
#endif
#if !defined(SVNREV)
  #define SVNREV "unknown"
#endif

void usage() {
	printf("\nUsage: Vex [-leda] [-fix] VexInput.vex VerilogOutput.v\n");
	printf("Version: %s-%s (%s)\n", VERSION, SVNREV, BLDDTE);
}


int main(int argc, char* argv[])
{
	CDesign *pDsn;

#ifndef __GNUG__
	if (argc <= 1) {
		argc = 1;
		char *argv2[10];

		argv2[0] = argv[0];
		argv2[argc++] = "-leda";
		//argv2[argc++] = "-fix";
		argv2[argc++] = "-prp";

		argv2[argc++] = "c:/Users/TBrewer/Documents/Visual Studio 2010/Projects/ZG1 checkout/ZG1 SystemC/ZG1FpPrims.vex";
		argv2[argc++] = "ZG1FpPrims.v";

		//argv2[argc++] = "c:/Users/TBrewer/Documents/Visual Studio 2010/Projects/CnyBwa/src/CnyBwaPrims.vex";
		//argv2[argc++] = "c:/Users/TBrewer/Documents/Visual Studio 2010/Projects/CnyBwa/CnyBwa/verilog/CnyBwaPrims.v";
		argv2[argc] = 0;

		argv = argv2;
	}
#endif

	int argIdx = 1;

	bool bIsLedaEnabled = false;
	bool bIsGenFixtureEnabled = false;
	bool bIsPrpEnabled = false;

	while (argv[argIdx][0] == '-') {
		if (strcmp(argv[argIdx], "-leda") == 0)
			bIsLedaEnabled = true;
		else if (strcmp(argv[argIdx], "-fix") == 0)
			bIsGenFixtureEnabled = true;
		else if (strcmp(argv[argIdx], "-prp") == 0)
			bIsPrpEnabled = true;
		else {
			usage();
			exit(1);
		}
		argIdx += 1;
	}

	if (argc - argIdx != 2) {
		usage();
		exit(1);
	}

	char *pVexName = argv[argIdx++];
	pDsn = new CDesign(pVexName);

	char *pVerilogName = argv[argIdx++];

	pDsn->Parse();

	pDsn->PerformModuleChecks();

	pDsn->GenVppFile(pVerilogName, bIsLedaEnabled);

	if (bIsPrpEnabled)
		pDsn->WritePrpFile(pVexName);

	if (bIsGenFixtureEnabled)
		pDsn->GenFixture(pVerilogName);

	delete pDsn;

	return 0;
}
