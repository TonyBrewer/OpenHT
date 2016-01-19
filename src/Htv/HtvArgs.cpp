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
#include "HtvArgs.h"
#include "HtfeErrorMsg.h"

CHtvArgs g_htvArgs;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CHtvArgs::CHtvArgs()
{
	m_bGenFixture = false;
	m_bXilinxFdPrims = false;
	m_bSynXilinxPrims = false;
	m_bIsLedaEnabled = false;
	m_bIsPrmEnabled = false;
	m_bGenSandbox = false;
	m_bKeepHierarchy = false;
	m_bVivado = false;
	m_bQuartus = false;
	m_bAlteraDistRams = false;
}

CHtvArgs::~CHtvArgs()
{

}

#if !defined(VERSION)
#define VERSION "unknown"
#endif
#if !defined(BLDDTE)
#define BLDDTE "unknown"
#endif
#if !defined(VCSREV)
#define VCSREV "unknown"
#endif

void CHtvArgs::Usage(const char *progname)
{
	printf("\nUsage: %s [options] <design>.cpp [<design>.v]\n", progname);
	printf("  where: design.cpp is the C++ input file\n");
	printf("         design.v is the optional output Verilog file\n");
	printf("\n");
	printf("       %s [options] <design>.sc [<design>.h] [<design>.v]\n", progname);
	printf("  where: design.sc is the SystemC input file\n");
	printf("         design.h is the optional output include file\n");
	printf("         design.v is the optional output Verilog file\n");
	printf("\n");
	printf("Version: %s-%s (%s)\n", VERSION, VCSREV, BLDDTE);
	printf("Options:\n");
	printf("  -help, -h\n");
	printf("     Prints this usage message\n");
	printf("  -vdir\n");
	printf("     Directory where verilog is to be written. File name is same as input file\n");
	printf("     name (.cpp or .sc) with a .v suffix\n");

	CHtfeArgs::Usage();

	printf("  -sbx\n");
	printf("     Generates a sandbox in addition to the normal output files\n");
	printf("  -sdrp\n");
	printf("     Generate distributed ram primitives in the output verilog file.\n");
	printf("     Default is to allow Xilinx XST tool to infer the rams.\n");
	printf("\n");
	printf("Output file names can be specified on the command line (.v or .h), or\n");
	printf("  if not specified on the command line will be the same as the input\n");
	printf("  file name with the suffix replaced with .v or .h\n");
}

void CHtvArgs::Parse(int argc, char *argv[])
{
	m_argc = argc;
	m_argv = argv;

#ifdef WIN32
	if (argc <= 1) {
		argc = 1;
		char *argv2[20];

		argv2[0] = argv[0];

		SetCurrentDirectory("C:/OpenHT/tests/dsn1/msvs12/");
		argv2[argc++] = "-I";
		argv2[argc++] = "C:/OpenHT/ht_lib";
		argv2[argc++] = "-I";
		argv2[argc++] = "C:/OpenHT/ht_lib/sysc";
		argv2[argc++] = "-I";
		argv2[argc++] = "../src";
		argv2[argc++] = "-I";
		argv2[argc++] = "../src_pers";
		argv2[argc++] = "-I";
		argv2[argc++] = "../ht/sysc";
		//argv2[argc++] = "-DHT_ASSERT";
		//argv2[argc++] = "-vivado";
		//argv2[argc++] = "../ht/sysc/PersAuTop.sc";
		//argv2[argc++] = "../ht/sysc/PersAuTop.h";
		//argv2[argc++] = "../ht/verilog/PersAuTop.v";
		argv2[argc++] = "../ht/sysc/PersInc.cpp";
		//argv2[argc++] = "C:/OpenHT_ngv/ht_lib/sysc/PersUnitCnt.cpp";
		argv2[argc++] = "../ht/verilog/PersInc.v";

		argv2[argc] = 0;

		argv = argv2;
	}
#endif

	int argPos = 1;
	string verilogDir;

	while (argPos < argc) {
		if (argv[argPos][0] == '-') {
			if ((strcmp(argv[argPos], "-h") == 0) ||
				(strcmp(argv[argPos], "-help") == 0)) {
					Usage(argv[0]);
					ErrorExit();
			} else if (strncmp(argv[argPos], "-DesignPrefix=", 14) == 0) {
				SetDesignPrefix(argv[argPos]+14);
			} else if (strncmp(argv[argPos], "-FD_prims", 9) == 0) {
				SetXilinxFdPrims(true);
			} else if (strncmp(argv[argPos], "-gfx", 4) == 0 || strncmp(argv[argPos], "-fx", 3) == 0) {
				SetGenFixture(true);
			} else if (strcmp(argv[argPos], "-sxp") == 0) {
				SetSynXilinxPrims(true);
			} else if (strcmp(argv[argPos], "-vivado") == 0) {
				SetVivadoEnabled(true);
			} else if (strcmp(argv[argPos], "-quartus") == 0) {
				SetQuartusEnabled(true);
			} else if (strcmp(argv[argPos], "-adr") == 0) {
				SetAlteraDistRams(true);
			} else if (strcmp(argv[argPos], "-sbx") == 0) {
				SetGenSandbox(true);
			} else if (strncmp(argv[argPos], "-leda", 5) == 0) {
				SetIsLedaEnabled();
			} else if (strncmp(argv[argPos], "-prm", 4) == 0) {
				SetIsPrmEnabled();
			} else if (strcmp(argv[argPos], "-vdir") == 0) {
				verilogDir = argv[++argPos];

			} else
				CHtfeArgs::Parse(argPos, argv);

		} else {

			string fileName = argv[argPos];
			size_t nameLen = fileName.length();
			string suffix4 = nameLen > 4 ? fileName.substr(fileName.length()-4, 4) : "";
			string suffix3 = nameLen > 3 ? fileName.substr(fileName.length()-3, 3) : "";
			string suffix2 = nameLen > 2 ? fileName.substr(fileName.length()-2, 2) : "";

			if (suffix3 == ".sc") {

				SetIsScHier();

				if (m_inputPathName.size() != 0) {
					printf("Command line error: multiple input file names specified (.sc or .cpp)\n");
					ErrorExit();
				}

				m_inputPathName = argv[argPos];

			} else if (suffix4 == ".cpp") {

				SetIsScCode();

				if (m_inputPathName.size() != 0) {
					printf("Command line error: multiple input file names specified (.sc or .cpp)\n");
					ErrorExit();
				}

				m_inputPathName = argv[argPos];

			} else if (suffix2 == ".v") {

				if (m_vFileName.size() != 0) {
					printf("Command line error: multiple output file names specified (.v)\n");
					ErrorExit();
				}

				m_vFileName = argv[argPos];

			} else if (suffix2 == ".h") {

				if (m_incFileName.size() != 0) {
					printf("Command line error: multiple output file names specified (.h)\n");
					ErrorExit();
				}

				m_incFileName = argv[argPos];

			} else {
				printf("Command line error: Extra file specified '%s'\n", argv[argPos]);
				ErrorExit();
			}
		}

		argPos += 1;
	}

#ifdef _WIN32
#define strcasecmp _stricmp
#endif

	char * pValue = getenv("HTV_FORCE_VENDOR");
	if (pValue) {
		if (strcasecmp(pValue, "Altera") == 0) {
			SetVivadoEnabled(false);
			SetQuartusEnabled(true);
		} else if (strcasecmp(pValue, "Xilinx") == 0) {
			SetVivadoEnabled(true);
			SetQuartusEnabled(false);
		} else {
			fprintf(stderr, "Unknown value for HTL_FORCE_COPROC environment variable, %s\n", pValue);
			exit(1);
		}
		printf("Warning - HTV_FORCE_VENDOR set to %s\n", pValue);
	}


	// Two legal file combinations
	// 1. x.cpp z.v
	// 2. x.sc y.h z.v

	if (m_inputPathName.size() == 0) {
		printf("Command line error: input file name not specified (.sc or .v)\n");
		ErrorExit();
	}

	m_inputFileName = m_inputPathName;
	int periodPos = m_inputFileName.find_last_of(".");
	m_inputFileName.erase(periodPos);
	int slashPos = m_inputFileName.find_last_of("/\\");
	if (slashPos > 0) m_inputFileName.erase(0, slashPos+1);

	if (verilogDir.size() != 0 && m_vFileName.size() != 0) {
		printf("Command line error: -vdir and <verilogFile> specified (only one allowed)\n");
		ErrorExit();
	}

	// output files are optional, create output file names if not provided
	if (verilogDir.size() > 0) {
		m_vFileName = m_inputPathName;

		int lastPeriodPos = m_vFileName.find_last_of(".");
		if (lastPeriodPos > 0)
			m_vFileName.erase(lastPeriodPos+1);

		int lastSlashPos = m_vFileName.find_last_of("\\/");
		if (lastSlashPos > 0)
			m_vFileName.erase(0, lastSlashPos);

		m_vFileName = verilogDir + "/" + m_vFileName + "v";

	} else if (m_vFileName.size() == 0) {
		m_vFileName = m_inputPathName;

		int lastPeriodPos = m_vFileName.find_last_of(".");
		if (lastPeriodPos > 0)
			m_vFileName.erase(lastPeriodPos+1);

		m_vFileName += "v";
	}

	if (m_incFileName.size() == 0) {
		m_incFileName = m_inputPathName;

		int lastPeriodPos = m_incFileName.find_last_of(".");
		if (lastPeriodPos > 0)
			m_incFileName.erase(lastPeriodPos+1);

		m_incFileName += "h";
	}
}
