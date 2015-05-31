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
#include "DsnInfo.h"
#include "AppArgs.h"

CCoprocInfo g_coprocInfo[] = {
	CCoprocInfo(hc1, "hc1", "hc-1", 1, 1, 1, 1, false, 576),
	CCoprocInfo(hc1ex, "hc1ex", "hc-1ex", 1, 1, 1, 1, false, 1440),
	CCoprocInfo(hc2, "hc2", "hc-2", 8, 8, 1, 1, false, 576),
	CCoprocInfo(hc2ex, "hc2ex", "hc-2ex", 8, 8, 1, 1, false, 1440),
	CCoprocInfo(wx690, "wx690", "wx-690", 8, 8, 8, 8, true, 2940),
	CCoprocInfo(wx2k, "wx2k", "wx-2000", 8, 8, 8, 8, true, 2584/4),
	CCoprocInfo(ma100, "ma100", "ma-100", 8, 8, 8, 8, true, 2500 /*FIXME*/),
	CCoprocInfo(ma400, "ma400", "ma-400", 8, 8, 8, 8, true, 2500 /*FIXME*/),
	CCoprocInfo()
};

CAppArgs g_appArgs;

void
CAppArgs::Usage()
{
	printf("\nUsage: %s [options] <InputFile1> {,<InputFile2>, ...} <OutputFolder>\n", m_progName.c_str());
	printf("\n");
	printf("Version: %s-%s (%s)\n", VERSION, VCSREV, BLDDTE);
	printf("Options:\n");
	printf("  -help, -h        Prints this usage message\n");
	printf("  -cp <name>       Coprocessor (i.e. hc-1, hc-1ex, hc-2, hc-2ex)\n");
	printf("  -ac <num>        AE Count (default 1)\n");
	printf("  -gr              Generate Reports\n");
	printf("  -it              Enable Instruction Tracing\n");
	printf("  -pm              Enable Performance Monitoring\n");
	printf("  -ri              Enable Random Initialization\n");
	printf("  -rr              Enable Random Retry\n");
	printf("  -uc <num>        Unit Count (default 1)\n");
	//printf("  -un <name>       Unit Name (default 'Au')\n");
	//printf("  -en <name>       Entry point for coprocessor (default 'htmain')\n");
	//printf("  -tw <num>        Host thread width (default 0)\n");
	//printf("  -iq <mod>        Module name for input HIF queue\n");
	//printf("  -oq <mod>        Module name for output HIF queue\n");
	printf("  -cl <file>       Name of file for command line options\n");
	//printf("  -if <file>       Name of module instance file\n");
	//printf("  -df <freq>       Default module instance frequency (default 150 mhz)\n");
	printf("  -ub <cnt>        Maximum number of 18Kb block rams per Unit (default: AE BRAM count * 50%% / Units Per AE)\n");
	printf("  -lb <ratio>      Minimum lutToBram ratio for block rams usage (default 100)\n");
	printf("  -ml <scale>      Memory model latency scaling (default 1.0)\n");
	printf("  -mt              Memory tracing enable\n");
	printf("  -vr              Variable report generation enabled\n");
#ifdef _WIN32
	printf("  -pn <name>       Msvs project name\n");
	printf("  -mf <mod>        Generate fixture for module name\n");
#endif
	printf("  -I <directory>   Directory to search for include files\n");
	printf("  -D<defineName>=x Sets <defineName> to the value x for the preprocessor (default value is 1)\n");
}

void
CAppArgs::ParseArgsFile(char const * pFileName)
{
	// parse file of arguments
	// copy file into a buffer and set \n and \r to space
	// set argc and argv to point to arguments in buffer

	// Get size of file
	int fileSize;
#ifdef WIN32
	struct _stat statBuf;
	if (_stat(pFileName, &statBuf) != 0) {
		printf("Command line file: Could not find file %s\n", pFileName);
		exit(1);
	}

	fileSize = statBuf.st_size;
#else
	struct stat statBuf;
	if (stat(pFileName, &statBuf) != 0) {
		printf("Command line file: Could not find file %s\n", pFileName);
		exit(1);
	}

	fileSize = statBuf.st_size;
#endif

	if (fileSize > 64000) {
		printf("Command line file: file %s too big (limit of 64K bytes)\n", pFileName);
		exit(1);
	}

	char *fileBuf = new char[fileSize + 1];

	FILE *fp = fopen(pFileName, "rb");
	if (fp == 0 || fread(fileBuf, fileSize, 1, fp) != 1) {
		printf("Command line file: could not read file %s\n", pFileName);
		exit(1);
	}
	fileBuf[fileSize] = '\0';
	fclose(fp);

	char const ** argv = new char const *[1000];
	int argc = 0;
	argv[argc++] = m_progName.c_str();

	char *pBuf = fileBuf;
	while (*pBuf != '\0') {
		if (*pBuf == ' ' || *pBuf == '\t' || *pBuf == '\n' || *pBuf == '\r') {
			pBuf += 1;
			continue;
		}

		if (pBuf[0] == '/' && pBuf[1] == '/') {
			while (*pBuf != '\n' && *pBuf != '\r' && *pBuf != '\0') pBuf += 1;
			continue;
		}

		bool bQuotes = *pBuf == '"';
		if (bQuotes)
			pBuf += 1;

		argv[argc++] = pBuf;

		if (bQuotes) {
			while (*pBuf != '"' && *pBuf != '\0') pBuf += 1;
			if (*pBuf == '\0') {
				printf("Command line file: missing closing quote\n");
				exit(1);
			}
		} else
			while (*pBuf != ' ' && *pBuf != '\t' && *pBuf != '\n' && *pBuf != '\r' && *pBuf != '\0') pBuf += 1;

		bool bEol = *pBuf == '\0';

		*pBuf = '\0';

		if (!bEol)
			pBuf += 1;
	}

	Parse(argc, argv);

	// don't delete - HtlAssert needs arguments
	//delete fileBuf;
	//delete argv;
}

bool CAppArgs::SetHtCoproc(char const * pStr)
{
	if (*pStr == '?') {
		if (m_bRndTest) {
			int rndIdx = GetRtRndIdx(6);
			m_coprocId = rndIdx;
			printf("   Rnd Coproc: %s\n", GetCoprocName());
			return true;
		}
		pStr += 1;
	}
	
	for (int i = 0; g_coprocInfo[i].GetCoprocName() != 0; i += 1) {
		if (strcasecmp(pStr, g_coprocInfo[i].GetCoprocName()) == 0) {
			m_coprocId = i;
			return true;
		}
	}

	return false;
}

void
CAppArgs::Parse(int argc, char const **argv)
{
	m_argc = argc;
	m_argv = argv;

	m_progName = argv[0];
	m_aeCnt = 1;
	m_bGenReports = true;
	m_bPerfMon = true;
	m_bRndInit = true;
	m_bRndRetry = false;
	m_bRndTest = false;
	m_aeUnitCnt = 1;
	m_unitName = "Au";
	//m_entryName = "htmain";
	m_hostHtIdW = 0;
	m_defaultFreqMhz = 150;
	m_max18KbBramPerUnit = -1;
	m_minLutToBramRatio = 40;
	m_bInstrTrace = false;
	m_avgMemLatency[0] = 130;	// CP  : 110 min + ~20%
	m_avgMemLatency[1] = 430;	// Host: 360 min + ~20%
	m_bMemTrace = false;
	m_bModelOnly = false;
	m_bForkPrivWr = true;
	m_bRndTest = false;
	m_bVariableReport = false;
	m_pVarRptFp = 0;
	m_bModuleUnitNames = true;
	m_bGlobalWriteHtid = true;
	m_bGlobalReadParan = true;
	m_bDsnRpt = true;
	m_pDsnRpt = 0;
	m_bVcdUser = false;
	m_bVcdAll = false;
	m_vcdStartCycle = 0;
	m_bOgv = false;

	int argPos;
	bool bClFlag = false;
	for (argPos = 1; argPos < argc; argPos++) {
		if (bClFlag) {
			Usage();
			exit(1);
		}
		if (argv[argPos][0] == '-') {
			if ((strcmp(argv[argPos], "-h") == 0) ||
				(strcmp(argv[argPos], "-help") == 0)) {
				Usage();
				exit(0);
			} else if ((strcmp(argv[argPos], "-cp") == 0)) {
				argPos += 1;
				if (!SetHtCoproc(argv[argPos])) {
					fprintf(stderr, "Unknown value for -cp\n");
					Usage();
					exit(1);
				}
			} else if ((strcmp(argv[argPos], "-ac") == 0)) {
				argPos += 1;
				m_aeCnt = atoi(argv[argPos]);
			} else if ((strcmp(argv[argPos], "-ogv") == 0)) {
				m_bOgv = true;
			} else if ((strcmp(argv[argPos], "-mund") == 0)) {
				m_bModuleUnitNames = false;
			} else if ((strcmp(argv[argPos], "-grpd") == 0)) {
				m_bGlobalReadParan = false;
			} else if ((strcmp(argv[argPos], "-gwhd") == 0)) {
				m_bGlobalWriteHtid = false;
			} else if ((strcmp(argv[argPos], "-it") == 0)) {
				m_bInstrTrace = true;
			} else if ((strcmp(argv[argPos], "-ofj") == 0)) {
				m_bForkPrivWr = false;
			} else if ((strcmp(argv[argPos], "-mo") == 0)) {
				m_bModelOnly = true;
			} else if ((strcmp(argv[argPos], "-ri") == 0)) {
				m_bRndInit = true;
			} else if ((strcmp(argv[argPos], "-nri") == 0)) {
				m_bRndInit = false;
			} else if ((strcmp(argv[argPos], "-rr") == 0)) {
				m_bRndRetry = true;
			} else if ((strncmp(argv[argPos], "-rt", 3) == 0)) {
				m_bRndTest = true;
			} else if ((strcmp(argv[argPos], "-vcd") == 0)) {
				m_bVcdUser = true;
			} else if ((strcmp(argv[argPos], "-vcd_all") == 0)) {
				m_bVcdAll = true;
			} else if ((strcmp(argv[argPos], "-vcd_start") == 0)) {
				argPos += 1;
				m_vcdStartCycle = atoi(argv[argPos]);
			} else if ((strcmp(argv[argPos], "-vcd_filter") == 0)) {
				argPos += 1;
				m_vcdFilterFile = argv[argPos];
			} else if ((strncmp(argv[argPos], "-vr", 3) == 0)) {
				m_bVariableReport = true;
				m_pVarRptFp = fopen("HtPrivRpt.txt", "w");
				if (m_pVarRptFp == 0) {
					fprintf(stderr, "Could not open HtPrivRpt.txt\n");
					exit(1);
				}
			} else if ((strcmp(argv[argPos], "-uc") == 0)) {
				argPos += 1;
				m_aeUnitCnt = atoi(argv[argPos]);
			} else if ((strcmp(argv[argPos], "-un") == 0)) {
				if (argPos == argc - 1) {
					printf("expected parameter for -un command line flag\n");
					Usage();
					exit(1);
				}
				argPos += 1;
				m_unitName = argv[argPos];
#ifdef _WIN32
			} else if ((strcmp(argv[argPos], "-pn") == 0)) {
				if (argPos == argc - 1) {
					printf("expected parameter for -pn command line flag\n");
					Usage();
					exit(1);
				}
				argPos += 1;
				m_projName = argv[argPos];
			} else if ((strcmp(argv[argPos], "-mf") == 0)) {
				if (argPos == argc - 1) {
					printf("expected parameter for -mf command line flag\n");
					Usage();
					exit(1);
				}
				argPos += 1;
				m_fxModName = argv[argPos];

#endif
			} else if ((strcmp(argv[argPos], "-if") == 0)) {
				if (argPos == argc - 1) {
					printf("expected parameter for -if command line flag\n");
					Usage();
					exit(1);
				}
				argPos += 1;
				m_instanceFile = argv[argPos];
			} else if ((strcmp(argv[argPos], "-df") == 0)) {
				if (argPos == argc - 1) {
					printf("expected parameter for -df command line flag\n");
					Usage();
					exit(1);
				}
				argPos += 1;
				if (sscanf(argv[argPos], "%d", &m_defaultFreqMhz) != 1) {
					printf("expected an integer parameter for -df command line flag\n");
					Usage();
					exit(1);
				}
			} else if ((strcmp(argv[argPos], "-ub") == 0)) {
				if (argPos == argc - 1) {
					printf("expected parameter for -ub command line flag\n");
					Usage();
					exit(1);
				}
				argPos += 1;
				if (sscanf(argv[argPos], "%d", &m_max18KbBramPerUnit) != 1) {
					printf("expected an integer parameter for -ub command line flag\n");
					Usage();
					exit(1);
				}
			} else if ((strcmp(argv[argPos], "-mt") == 0)) {
				m_bMemTrace = true;
			} else if ((strcmp(argv[argPos], "-ml") == 0)) {
				if (argPos == argc - 1) {
					printf("expected parameter for -ml command line flag\n");
					Usage();
					exit(1);
				}
				argPos += 1;
				float latScale = 1.0;
				if (sscanf(argv[argPos], "%f", &latScale) != 1) {
					printf("expected a floating point parameter for -ml command line flag\n");
					Usage();
					exit(1);
				}
				if (latScale < 0.0 || latScale > 10.0) {
					printf("-ml parameter outside valid range (0.0-10.0)\n");
					exit(1);
				}
				m_avgMemLatency[0] = (int)(m_avgMemLatency[0] * latScale);
				m_avgMemLatency[1] = (int)(m_avgMemLatency[1] * latScale);
			} else if ((strcmp(argv[argPos], "-lb") == 0)) {
				if (argPos == argc - 1) {
					printf("expected parameter for -lb command line flag\n");
					Usage();
					exit(1);
				}
				argPos += 1;
				if (sscanf(argv[argPos], "%d", &m_minLutToBramRatio) != 1) {
					printf("expected an integer parameter for -lb command line flag\n");
					Usage();
					exit(1);
				}
			} else if ((strcmp(argv[argPos], "-tw") == 0)) {
				if (argPos == argc - 1) {
					printf("expected parameter for -tw command line flag\n");
					Usage();
					exit(1);
				}
				argPos += 1;
				if (sscanf(argv[argPos], "%d", &m_hostHtIdW) != 1) {
					printf("expected an integer parameter for -tw command line flag\n");
					Usage();
					exit(1);
				}
			} else if ((strcmp(argv[argPos], "-en") == 0)) {
				if (argPos == argc - 1) {
					printf("expected parameter for -en command line flag\n");
					Usage();
					exit(1);
				}
				argPos += 1;
				m_entryName = argv[argPos];
			} else if ((strcmp(argv[argPos], "-iq") == 0)) {
				if (argPos == argc - 1) {
					printf("expected parameter for -iq command line flag\n");
					Usage();
					exit(1);
				}
				argPos += 1;
				m_iqModName = argv[argPos];
			} else if ((strcmp(argv[argPos], "-oq") == 0)) {
				if (argPos == argc - 1) {
					printf("expected parameter for -oq command line flag\n");
					Usage();
					exit(1);
				}
				argPos += 1;
				m_oqModName = argv[argPos];
			} else if ((strcmp(argv[argPos], "-cl") == 0)) {
				if (argPos != 1) {
					printf("expected -cl command line flag to be the only flag\n");
					Usage();
					exit(1);
				}
				if (argPos == argc - 1) {
					printf("expected parameter for -cl command line flag\n");
					Usage();
					exit(1);
				}
				argPos += 1;

				m_htlName = argv[argPos];
				int pos = m_htlName.find_last_of("/\\");
				if (pos >= 0)
					m_htlName = m_htlName.substr(pos + 1);

				ParseArgsFile(argv[argPos]);
				bClFlag = true;

#ifdef WIN32
			} else if ((strcmp(argv[argPos], "-wd") == 0)) {
				if (argPos == argc - 1) {
					printf("expected parameter for -wd command line flag\n");
					Usage();
					exit(1);
				}
				argPos += 1;

				string wdPath = argv[argPos];
				EnvVarExpansion(wdPath);

				if (SetCurrentDirectory((char *)wdPath.c_str()) == 0) {
					printf("Unable to set working directory (Error=%d)\n", GetLastError());
					exit(1);
				}
#endif
			} else if (strncmp(argv[argPos], "-D", 2) == 0) {
				// check if define has a value
				char const *pArg = argv[argPos] + 2;
				while (*pArg != '=' && *pArg != '\0') pArg++;
				if (*pArg == '=')
					m_preDefinedNames.push_back(pair<string, string>(string(argv[argPos] + 2, pArg - (argv[argPos] + 2)), string(pArg + 1)));
				else
					m_preDefinedNames.push_back(pair<string, string>(string(argv[argPos] + 2), string()));
			} else if (strcmp(argv[argPos], "-I") == 0) {
				string path = argv[++argPos];
				if (path.find_last_of("/\\") != path.size() - 1)
					path += "/";
				m_includeDirs.push_back(path);
			} else {
				printf("Unknown command line switch: %s\n", argv[argPos]);
				Usage();
				exit(1);
			}
		} else
			break;
	}

	if (!bClFlag && (argc - argPos) < 2) {
		printf("expected an htd file\n");
		Usage();
		exit(1);
	}

	if (bClFlag && argPos != argc) {
		printf("expected -cl command line flag to be the only flag\n");
		Usage();
		exit(1);
	}

	for (; argPos < argc - 1; argPos += 1)
		m_inputFileList.push_back(argv[argPos]);

	if (!bClFlag)
		m_outputFolder = argv[argPos];

	if (m_projName.size() == 0) {
		m_projName = m_htlName;
		int pos = m_htlName.find_last_of(".");
		if (pos >= 0)
			m_projName = m_projName.substr(0, pos);
	}

#ifdef _WIN32
	if (!bClFlag) {
		// check if HtRndTestEnable.txt file exists
		FILE *fp = fp = fopen("../../msvs10/HtRndTest.txt", "r");

		if (fp) {
			char buf[128];

			while (fgets(buf, 128, fp)) {
				if (strncmp(buf, "-cp ", 4) == 0) {
					char *p = buf + 4;
					while (*p != '\n' && *p != '\r' && *p != '\0') p += 1;
					*p = '\0';
					if (!SetHtCoproc(buf + 4)) {
						fprintf(stderr, "Unknown value for -cp\n");
						Usage();
						exit(1);
					}
				} else if (strncmp(buf, "-rt", 3) == 0) {
					m_bRndTest = true;

					int seed = -1;
					FILE *fp = fopen("HtFailSeed.txt", "r");
					if (fp) {
						if (fscanf(fp, "%d", &seed) == 1)
							printf("Previous failed seed found (%d)\n", seed);
						fclose(fp);
					}

					if (seed == -1) {
						struct timeval st;
						gettimeofday(&st, NULL);
						seed = st.tv_usec;
						FILE *fp = fopen("HtRndSeed.txt", "w");
						fprintf(fp, "%d\n", seed);
						fclose(fp);
						printf("New random seed (%d)\n", seed);
					}

					m_mtRand.seed(seed);
				} else if (strncmp(buf, "//", 2) != 0)
					printf("Unknown HtRndTest.txt command\n  %s\n", buf);
			}

			printf("Found HtRndTest.txt - %s\n", m_bRndTest ? "true" : "false");
			fclose(fp);
		}
	}
#endif

	// check if coproc is specified in environment variable
	char * pValue = getenv("HTL_FORCE_COPROC");
	if (pValue) {
		if (!SetHtCoproc(pValue)) {
			fprintf(stderr, "Unknown value for HTL_FORCE_COPROC environment variable, %s\n", pValue);
			exit(1);
		}
		printf("Warning - HTL_FORCE_COPROC forced coproc to %s\n", pValue);
	}

	// validate command line options
	bool bError = false;
	if (m_unitName.size() == 0) {
		printf("Error - Unit name not specified, use command line option '-un <name>'");
		bError = true;
	}

	if (m_coprocId < 0 && !IsModelOnly()) {
		printf("Error - target coprocessor type not specified, use '-cp <target platform>'");
		bError = true;
		exit(1);
	}

	if (m_aeUnitCnt > 16 || m_aeUnitCnt < 1) {
		printf("Error - Value for -uc outside supported range (1-16)\n");
		bError = true;
	}

	// check HT_UNIT_CNT define
	bool bFoundUnitCnt = false;
	for (size_t i = 0; i < m_preDefinedNames.size(); i += 1) {
		if (m_preDefinedNames[i].first == "HT_UNIT_CNT") {
			bFoundUnitCnt = true;

			char * pEnd;
			int tmpCnt = strtol(m_preDefinedNames[i].second.c_str(), &pEnd, 10);
			if (m_preDefinedNames[i].second.c_str() == pEnd) {
				printf("Error -DHT_UNIT_CNT must specify a value\n");
				bError = true;
				break;
			} else if (tmpCnt != m_aeUnitCnt) {
				printf("Error -DHT_UNIT_CNT value does not match -uc value\n");
				bError = true;
				break;
			}
		}
	}
	if (!bFoundUnitCnt)
		m_preDefinedNames.push_back(pair<string, string>("HT_UNIT_CNT", VA("%d", m_aeUnitCnt)));

	if (m_max18KbBramPerUnit == -1)
		m_max18KbBramPerUnit = (int)(GetBramsPerAE() * 0.5 / m_aeUnitCnt);

	if (!bClFlag && m_bDsnRpt) {
		m_pDsnRpt = new CGenHtmlRpt(argv[0], "HtDsnRpt.html", argc, argv);
		m_pDsnRpt->GenApiHdr();
	}

	if (!bClFlag && !m_bModuleUnitNames) {
		printf("Warning - Unit name embedded within a user defined module has been deprecated\n");
		printf("    Use command line flag -mune to enable new syntax\n");
		printf("    Example: PersAuCtl  =>  PersCtl\n");
	}
	if (!bClFlag && !m_bGlobalWriteHtid) {
		printf("Warning - Global variable write API with hidden htId has been deprecated\n");
		printf("    Use command line flag -gwhe to enable new syntax\n");
		printf("    Example: GW_var_field(varAddr2)  =>  GW_var_field(varAddr1, varAddr2)\n");
	}
	if (!bClFlag && !m_bGlobalReadParan) {
		printf("Warning - Global variable read API without parentheses has been deprecated\n");
		printf("    Use command line flag -grpe to enable new syntax\n");
		printf("    Example: GR_var_field  =>  GR_var_field()\n");
	}

	if (!bClFlag && m_vcdFilterFile.size() > 0)
		ReadVcdFilterFile();

	if (bError)
		exit(1);
}

void CAppArgs::ReadVcdFilterFile()
{
	FILE *fp;
	if (!(fp = fopen(m_vcdFilterFile.c_str(), "r"))) {
		printf("Could not open -vcdFilterFile %s\n", m_vcdFilterFile.c_str());
		exit(1);
	}

	char buf[1024];
	while (fgets(buf, 1024, fp)) {
		char *pStr = buf;
		while (*pStr != '\n' && *pStr != '\0')
			pStr += 1;
		*pStr = '\0';
		if (pStr == buf)
			continue;
		m_vcdFilterList.push_back(buf);
	}

	fclose(fp);
}

bool CAppArgs::IsVcdFilterMatch(string name)
{
	if (m_vcdFilterList.size() == 0)
		return true;

	for (size_t i = 0; i < m_vcdFilterList.size(); i += 1)
		if (Glob(name.c_str(), m_vcdFilterList[i].c_str()))
			return true;

	return false;
}

bool CAppArgs::Glob(const char * pName, const char * pFilter)
{
	while (pFilter[0] != '\0') {
		if (pFilter[0] == '?') {
			if (pName[0] == '\0')
				return false;

			pFilter += 1;
			pName += 1;
		} else if (pFilter[0] == '*') {
			if (Glob(pName, pFilter + 1))
				return true;
			if (pName[0] == '\0')
				return false;
			pName += 1;
		} else {
			if (pFilter[0] != pName[0])
				return false;

			pFilter += 1;
			pName += 1;
		}
	}

	return pFilter[0] == pName[0];
}

void CAppArgs::EnvVarExpansion(string & path)
{
	// search path for $( ), replace with environment variable value

	for (int i = 0;; i += 1) {
		if (path[i] == '\0') break;
		if (path[i] != '$' || path[i + 1] != '(') continue;

		int j;
		for (j = i + 2; path[j] != '\0' && path[j] != ')'; j += 1);

		if (path[i] == '\0') break;

		// found macro to expand

		string envName = string(path.c_str() + i + 2, j - i - 2);

		char * pValue = getenv(envName.c_str());

		if (pValue == 0) break;

		string value = pValue;

		path.replace(i, j - i + 1, value);

		i += value.size() - 1;
	}
}
