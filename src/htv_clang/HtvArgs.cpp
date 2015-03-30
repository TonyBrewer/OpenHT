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

#ifdef _WIN32
#include <windows.h>
#endif

CHtvArgs g_htvArgs;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CHtvArgs::CHtvArgs()
{
    m_bScMain = false;
    m_bScHier = false;
    m_bScCode = false;
    m_bIsTkDumpEnabled = false;
    m_bIsStDumpEnabled = false;
    m_bIsLvxEnabled = false;
    m_bWritePreProcessedInput = false;
	m_bMethodSupport = false;
	m_bTemplateSupport = false;
    m_bIsInUseEnabled = false;
    m_bHtQueCntSynKeep = false;

	m_bGenFixture = false;
    m_bXilinxFdPrims = false;
    m_bSynXilinxPrims = false;
    m_bSynDistRamPrims = false;
    m_bIsLedaEnabled = false;
    m_bIsPrmEnabled = false;
    m_bGenSandbox = false;
    m_bKeepHierarchy = false;

	m_bDumpHtvAST = false;
	m_bDumpClangAST = false;
	m_bDumpPosAndWidth = false;
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
#if !defined(SVNREV)
#define SVNREV "unknown"
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
    printf("Version: %s-%s (%s)\n", VERSION, SVNREV, BLDDTE);
    printf("Options:\n");
    printf("  -help, -h\n");
    printf("     Prints this usage message\n");
    printf("  -vdir\n");
    printf("     Directory where verilog is to be written. File name is same as input file\n");
    printf("     name (.cpp or .sc) with a .v suffix\n");

    printf("  -I <directory>\n");
    printf("     Directory to search for include files\n");
    printf("  -D<defineName>=x\n");
    printf("     Sets <defineName> to the value x for the preprocessor (default value is 1)\n");

    printf("  -lvx\n");
    printf("     Initialize uninitialized local variables to undefined in the output\n");
    printf("     verilog to allow more aggressive logic reduction. (default is initialize to 0)\n");
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

void CHtvArgs::Parse(int argc, char const * argv[])
{
	m_argc = argc;
	m_argv = argv;

#ifndef __GNUG__
    if (argc <= 1) {
        argc = 1;
        char const * argv2[20];

        //if (!SetCurrentDirectory("C:/Users/TBrewer/Documents/Visual Studio 2010/Projects/CnyHt/tests/dsn1/msvs10"))
        //if (!SetCurrentDirectory("C:/Users/TBrewer/Documents/My Projects/DpFma - Copy/DpFma"))
        //	exit(1);

        argv2[0] = argv[0];
        //argv2[argc++] = "-DHWT_ASSERT";

        //argv2[argc++] = "-FD_prims";
        //argv2[argc++] = "-DesignPrefix=ae";
        //argv2[argc++] = "DpFma.cpp";
        //argv2[argc++] = "DpFma.v";

        //// Lexis Nexis PersMu
        //SetCurrentDirectory("C:/Users/TBrewer/Documents/Visual Studio 2010/Projects/LexisNexis/PersMu10/msvs10");
        ////argv2[argc++] = "-sbx";
        //argv2[argc++] = "-D_SYSTEM_C";
        //argv2[argc++] = "-I";
        //argv2[argc++] = "../src";
        //argv2[argc++] = "-I";
        //argv2[argc++] = "../ht/sysc";
        //argv2[argc++] = "-I";
        //argv2[argc++] = "c:/Ht/ht_lib";
        //argv2[argc++] = "-D__SYSTEM_C__";
        ////argv2[argc++] = "../ht/sysc/PersMuTop.sc";
        ////argv2[argc++] = "../ht/sysc/PersMuTop.h";
        ////argv2[argc++] = "../ht/verilog/PersMuTop.v";
        //argv2[argc++] = "../ht/sysc/PersMuRead.cpp";
        //argv2[argc++] = "../ht/verilog/PersMuRead.v";

        //SetCurrentDirectory("C:/Users/TBrewer/Documents/Visual Studio 2010/Projects/CnyMemcached/Mcd1b/msvs10");
        //argv2[argc++] = "-D_SYSTEM_C";
        //argv2[argc++] = "-I";
        //argv2[argc++] = "../src";
        //argv2[argc++] = "-I";
        //argv2[argc++] = "../ht/sysc";
        //argv2[argc++] = "-I";
        //argv2[argc++] = "C:/Ht/ht_lib";
        ////argv2[argc++] = "../ht/sysc/PersSuTop.sc";
        ////argv2[argc++] = "../ht/sysc/PersSuTop.h";
        ////argv2[argc++] = "../ht/verilog/PersSuTop.v";
        //argv2[argc++] = "../ht/sysc/PersAuAcc.cpp";
        //argv2[argc++] = "../ht/verilog/PersAuAcc.v";

        //SetCurrentDirectory("C:/Users/TBrewer/Documents/Visual Studio 2010/Projects/CnyHt/tests/jacobi/msvs10");
        //argv2[argc++] = "-DUSE_SYSTEM_C_MODEL";
        //argv2[argc++] = "-I";
        //argv2[argc++] = "../src";
        //argv2[argc++] = "-I";
        //argv2[argc++] = "../ht/sysc";
        //argv2[argc++] = "-I";
        //argv2[argc++] = "../../ht_lib";
        //argv2[argc++] = "-D__SYSTEM_C__";
        ////argv2[argc++] = "../ht/sysc/PersSuTop.sc";
        ////argv2[argc++] = "../ht/sysc/PersSuTop.h";
        ////argv2[argc++] = "../ht/verilog/PersSuTop.v";
        //argv2[argc++] = "../ht/sysc/PersIuStencil.cpp";
        //argv2[argc++] = "../ht/verilog/PersIuStencil.v";

        //SetCurrentDirectory("C:/Users/TBrewer/Documents/Visual Studio 2010/Projects/Levenshtein/msvs10");
        //argv2[argc++] = "-D_SYSTEM_C";
        //argv2[argc++] = "-I";
        //argv2[argc++] = "../src";
        //argv2[argc++] = "-I";
        //argv2[argc++] = "../ht/sysc";
        //argv2[argc++] = "-I";
        //argv2[argc++] = "C:/Ht/ht_lib";
        ////argv2[argc++] = "../ht/sysc/PersSuTop.sc";
        ////argv2[argc++] = "../ht/sysc/PersSuTop.h";
        ////argv2[argc++] = "../ht/verilog/PersSuTop.v";
        //argv2[argc++] = "../ht/sysc/PersAuPipe.cpp";
        //argv2[argc++] = "../ht/verilog/PersAuPipe.v";

        //SetCurrentDirectory("C:/Users/TBrewer/Documents/Visual Studio 2010/Projects/PacBioCorrector/MemCorruptHtvBug/msvs10");
        //argv2[argc++] = "-D_SYSTEM_C";
        //argv2[argc++] = "-I";
        //argv2[argc++] = "../src";
        //argv2[argc++] = "-I";
        //argv2[argc++] = "../ht/sysc";
        //argv2[argc++] = "-I";
        //argv2[argc++] = "C:/Ht/ht_lib";
        ////argv2[argc++] = "../ht/sysc/PersSuTop.sc";
        ////argv2[argc++] = "../ht/sysc/PersSuTop.h";
        ////argv2[argc++] = "../ht/verilog/PersSuTop.v";
        //argv2[argc++] = "../ht/sysc/PersAuBed.cpp";
        //argv2[argc++] = "../ht/verilog/PersAuBed.v";

  //      SetCurrentDirectory("C:/Users/TBrewer/Documents/Visual Studio 2010/Projects/CnyBcm/lib_pers/msvs10");
  //      argv2[argc++] = "-DHT_SYSC";
		////argv2[argc++] = "-DHT_ASSERT";
		//argv2[argc++] = "-DBLACK_BOX";
  //      argv2[argc++] = "-I";
  //      argv2[argc++] = "../src_pers";
  //      argv2[argc++] = "-I";
  //      argv2[argc++] = "../ht/sysc";
  //      argv2[argc++] = "-I";
  //      argv2[argc++] = "C:/ht/ht_lib";
  //      argv2[argc++] = "../ht/sysc/PersAuBcm.cpp";
  //      argv2[argc++] = "../ht/verilog/PersAuBcm.v";

  //      SetCurrentDirectory("C:/Ht/tests/template/msvs10");
  //      argv2[argc++] = "-D_SYSTEM_C";
		//argv2[argc++] = "-template";
		////argv2[argc++] = "-DHT_ASSERT";
  //      argv2[argc++] = "-I";
  //      argv2[argc++] = "../src_pers";
  //      argv2[argc++] = "-I";
  //      argv2[argc++] = "../ht/sysc";
  //      argv2[argc++] = "-I";
  //      argv2[argc++] = "C:/ht/ht_lib";
  //      //argv2[argc++] = "-wpp";
  //      //argv2[argc++] = "../ht/sysc/PersAuTop.sc";
  //      //argv2[argc++] = "../ht/sysc/PersAuTop.h";
  //      //argv2[argc++] = "../ht/verilog/PersAuTop.v";
  //      //argv2[argc++] = "C:/ht/ht_lib/sysc/PersUnitCnt.cpp";
  //      argv2[argc++] = "../ht/sysc/PersAuTst.cpp";
  //      argv2[argc++] = "../ht/verilog/PersAuTst.v";

        SetCurrentDirectory("C:/Ht/tests/bug_fatal/msvs10");
        argv2[argc++] = "-D_SYSTEM_C";
		argv2[argc++] = "-method";
		//argv2[argc++] = "-DHT_ASSERT";
        argv2[argc++] = "-I";
        argv2[argc++] = "../src_pers";
        argv2[argc++] = "-I";
        argv2[argc++] = "../ht/sysc";
        argv2[argc++] = "-I";
        argv2[argc++] = "C:/ht/ht_lib";
        //argv2[argc++] = "-wpp";
        //argv2[argc++] = "../ht/sysc/PersAuTop.sc";
        //argv2[argc++] = "../ht/sysc/PersAuTop.h";
        //argv2[argc++] = "../ht/verilog/PersAuTop.v";
        //argv2[argc++] = "C:/ht/ht_lib/sysc/PersUnitCnt.cpp";
        argv2[argc++] = "../ht/sysc/PersAuReg.cpp";
        argv2[argc++] = "../ht/verilog/PersAuReg.v";

        //SetCurrentDirectory("C:/Ht/examples/ArithUnit");
        //argv2[argc++] = "-DesignPrefix=ae";
        ////argv2[argc++] = "-FD_prims";
        //argv2[argc++] = "Pass.cpp";
        //argv2[argc++] = "verilog/Pass.v";

        //SetCurrentDirectory("C:/Users/TBrewer/Documents/OmpDpMisc");
        ////argv2[argc++] = "-ilf";
        //argv2[argc++] = "-D_SYSTEM_C";
        ////argv2[argc++] = "../ht/sysc/PersSuTop.sc";
        ////argv2[argc++] = "../ht/sysc/PersSuTop.h";
        ////argv2[argc++] = "../ht/verilog/PersSuTop.v";
        //argv2[argc++] = "OmpDpMisc.cpp";
        //argv2[argc++] = "OmpDpMisc.v";

        //SetCurrentDirectory("C:/Users/TBrewer/Documents/Visual Studio 2010/Projects/CnyBwa/redux6/msvs10");
        //argv2[argc++] = "-D_SYSTEM_C";
        //argv2[argc++] = "-I";
        //argv2[argc++] = "../src";
        //argv2[argc++] = "-I";
        //argv2[argc++] = "../ht/sysc";
        //argv2[argc++] = "-I";
        //argv2[argc++] = "C:/Ht/ht_lib";
        ////argv2[argc++] = "../ht/sysc/PersSuTop.sc";
        ////argv2[argc++] = "../ht/sysc/PersSuTop.h";
        ////argv2[argc++] = "../ht/verilog/PersSuTop.v";
        ////argv2[argc++] = "C:/ht/ht_lib/sysc/PersXbarStub.cpp";
        //argv2[argc++] = "../ht/sysc/PersAuOcc.cpp";
        //argv2[argc++] = "../ht/verilog/PersAuOcc.v";

        //SetCurrentDirectory("C:/Users/TBrewer/Documents/Visual Studio 2010/Projects/CnyBwa/redux/Stk/StkFx");
        //argv2[argc++] = "-ilf";
        //argv2[argc++] = "-D_SYSTEM_C";
        //argv2[argc++] = "-I";
        //argv2[argc++] = "../src";
        //argv2[argc++] = "-I";
        //argv2[argc++] = "../ht/sysc";
        //argv2[argc++] = "-I";
        //argv2[argc++] = "C:/Ht/ht_lib";
        ////argv2[argc++] = "FixtureTop.sc";
        ////argv2[argc++] = "FixtureTop.h";
        ////argv2[argc++] = "FixtureTop.v";
        //argv2[argc++] = "../ht/sysc/PersAuStk.cpp";
        //argv2[argc++] = "../ht/verilog/PersAuStk.v";

        //SetCurrentDirectory("C:/Users/TBrewer/Documents/Visual Studio 2010/Projects/CnyBwa/CnyBwa");
        //argv2[argc++] = "-I";
        //argv2[argc++] = "../src";
        ////argv2[argc++] = "../ht/sysc/PersSuTop.sc";
        ////argv2[argc++] = "../ht/sysc/PersSuTop.h";
        ////argv2[argc++] = "../ht/verilog/PersSuTop.v";
        //argv2[argc++] = "../src/PersAuPipe.cpp";
        //argv2[argc++] = "verilog/PersAuPipe.v";

        // CnyBwa
        //SetCurrentDirectory("C:/Users/TBrewer/Documents/Visual Studio 2010/Projects/CnyBwa/CnyBwa");
        //argv2[argc++] = "-I";
        //argv2[argc++] = "../src";
        //argv2[argc++] = "-D__SYSTEM_C__";
        //argv2[argc++] = "../src/PersFourAuTop.sc";
        //argv2[argc++] = "../src/PersFourAuTop.h";
        //argv2[argc++] = "verilog/PersFourAuTop.v";

        //// IndexBench
        //SetCurrentDirectory("C:/Users/TBrewer/Documents/Visual Studio 2010/Projects/GoogleIndexBench - v3_Hif/PersIndexBench");
        //argv2[argc++] = "-I";
        //argv2[argc++] = "Src";
        //argv2[argc++] = "-D__SYSTEM_C__";
        //argv2[argc++] = "-DAE_UNIT_PRESENT_CNT=1";
        ////argv2[argc++] = "Src/PersAeTop.sc";
        ////argv2[argc++] = "Src/PersAeTop.h";
        ////argv2[argc++] = "verilog/PersAeTop.v";
        //argv2[argc++] = "Src/PersIuScr.cpp";
        //argv2[argc++] = "verilog/PersIuScr.v";

        //// SystemC Example
        //SetCurrentDirectory("C:/Users/TBrewer/Documents/My Projects/SystemC/examples/ArithUnit/ArithUnit");
        //argv2[argc++] = "-DesignPrefix=ae";
        //argv2[argc++] = "-D__SYSTEM_C__";
        //argv2[argc++] = "FixtureTop.sc";
        //argv2[argc++] = "FixtureTop.h";
        //argv2[argc++] = "verilog/FixtureTop.v";

        //// DpMisc
        //SetCurrentDirectory("C:/Users/TBrewer/Documents/My Projects/DpMisc - MSVS10/DpMisc");
        //argv2[argc++] = "-FD_prims";
        //argv2[argc++] = "-DesignPrefix=ae";
        //argv2[argc++] = "-D__SYSTEM_C__";
        ////argv2[argc++] = "FixtureTop.sc";
        ////argv2[argc++] = "FixtureTop.h";
        ////argv2[argc++] = "FixtureTop.v";
        //argv2[argc++] = "DpMisc.cpp";
        //argv2[argc++] = "DpMisc.v";

        //// DpFma
        //SetCurrentDirectory("C:/Users/TBrewer/Documents/My Projects/DpFma - Copy/DpFma");
        //argv2[argc++] = "-DesignPrefix=ae";
        //argv2[argc++] = "-D__SYSTEM_C__";
        //argv2[argc++] = "FixtureTop.sc";
        //argv2[argc++] = "FixtureTop.h";
        //argv2[argc++] = "FixtureTop.v";
        ////argv2[argc++] = "Src/PersIuQnd.cpp";
        ////argv2[argc++] = "verilog/PersIuQnd.v";

        //argv2[argc++] = "-GenFixture";
        //argv2[argc++] = "-StDump";
        //argv2[argc++] = "-GenSandbox";
        //argv2[argc++] = "-sbx";
        //argv2[argc++] = "-lvx";
        //argv2[argc++] = "-prm";
        //argv2[argc++] = "-vdir";
        //argv2[argc++] = "verilog";


        argv2[argc] = 0;

        argv = argv2;

        //return main(argc, argv2);
    }
#endif

    int argPos = 1;
    //bool bScFile = false;
    std::string verilogDir;

	m_frontEndArgs.push_back("-fno-cxx-exceptions");

    while (argPos < argc) {
        if (argv[argPos][0] == '-') {
            if ((strcmp(argv[argPos], "-h") == 0) ||
                (strcmp(argv[argPos], "-help") == 0)) {
                    Usage(argv[0]);
                    exit(1);
            } else if (strncmp(argv[argPos], "-DesignPrefix=", 14) == 0) {
                SetDesignPrefix(argv[argPos]+14);
            } else if (strncmp(argv[argPos], "-FD_prims", 9) == 0) {
                SetXilinxFdPrims(true);
            } else if (strncmp(argv[argPos], "-gfx", 4) == 0 || strncmp(argv[argPos], "-fx", 3) == 0) {
                SetGenFixture(true);
            } else if (strncmp(argv[argPos], "-sxp", 4) == 0) {
                SetSynXilinxPrims(true);
            } else if (strncmp(argv[argPos], "-sdrp", 5) == 0) {
                SetSynDistRamPrims(true);
            } else if (strcmp(argv[argPos], "-sbx") == 0) {
                SetGenSandbox(true);
            } else if (strncmp(argv[argPos], "-leda", 5) == 0) {
                SetIsLedaEnabled();
            } else if (strncmp(argv[argPos], "-prm", 4) == 0) {
                SetIsPrmEnabled();
            } else if (strcmp(argv[argPos], "-vdir") == 0) {
                verilogDir = argv[++argPos];
			} else if (strncmp(argv[argPos], "-template", 9) == 0) {
				SetTemplateSupport(true);
			} else if (strncmp(argv[argPos], "-method", 7) == 0) {
				SetMethodSupport(true);
			} else if (strncmp(argv[argPos], "-TkDump", 7) == 0) {
				SetIsTkDumpEnabled();
			} else if (strncmp(argv[argPos], "-StDump", 7) == 0) {
				SetIsStDumpEnabled();
			} else if (strncmp(argv[argPos], "-InUse", 6) == 0) {
				SetIsInUseEnabled(true);
			} else if (strncmp(argv[argPos], "-lvx", 4) == 0) {
				SetIsLvxEnabled();
			} else if (strncmp(argv[argPos], "-wpp", 4) == 0) {
				SetWritePreProcessedInput();
			} else if (strncmp(argv[argPos], "-DumpClangAST", 13) == 0) {
				setDumpClangAST();
			} else if (strncmp(argv[argPos], "-DumpHtvAST", 11) == 0) {
				setDumpHtvAST();
			} else if (strncmp(argv[argPos], "-PrintHtvAST", 12) == 0) {
				setDumpHtvAST(false);
			} else if (strncmp(argv[argPos], "-DumpPosAndWidth", 16) == 0) {
				setDumpPosAndWidth();

            } else {

				// Clang frontend arguments
				if (strncmp(argv[argPos], "-D", 2) == 0) {
					m_frontEndArgs.push_back(argv[argPos]);
					if (argv[argPos][2] == '\0') {
						argPos += 1;
						m_frontEndArgs.push_back(argv[argPos]);
					}
				} else if (strncmp(argv[argPos], "-I", 2) == 0) {
					m_frontEndArgs.push_back(argv[argPos]);
					if (argv[argPos][2] == '\0') {
						argPos += 1;
						m_frontEndArgs.push_back(argv[argPos]);
					}
				} else {
					printf("Unknown command line argument '%s'\n", argv[argPos]);
					exit(1);
				}
			}

        } else {

            std::string fileName = argv[argPos];
            size_t nameLen = fileName.length();
            std::string suffix4 = nameLen > 4 ? fileName.substr(fileName.length()-4, 4) : "";
            std::string suffix3 = nameLen > 3 ? fileName.substr(fileName.length()-3, 3) : "";
            std::string suffix2 = nameLen > 2 ? fileName.substr(fileName.length()-2, 2) : "";

            if (suffix3 == ".sc") {

				SetIsScHier();

                if (m_inputPathName.size() != 0) {
                    printf("Command line error: multiple input file names specified (.sc or .cpp)\n");
                    exit(1);
                }

                m_inputPathName = argv[argPos];
				m_sourceFiles.push_back(m_inputPathName);

            } else if (suffix4 == ".cpp") {

				SetIsScCode();

                if (m_inputPathName.size() != 0) {
                    printf("Command line error: multiple input file names specified (.sc or .cpp)\n");
                    exit(1);
                }

                m_inputPathName = argv[argPos];
				m_sourceFiles.push_back(m_inputPathName);

            } else if (suffix2 == ".v") {

                if (m_vFileName.size() != 0) {
                    printf("Command line error: multiple output file names specified (.v)\n");
                    exit(1);
                }

                m_vFileName = argv[argPos];

            } else if (suffix2 == ".h") {

                if (m_incFileName.size() != 0) {
                    printf("Command line error: multiple output file names specified (.h)\n");
                    exit(1);
                }

                m_incFileName = argv[argPos];

            } else {
                printf("Command line error: Extra file specified '%s'\n", argv[argPos]);
                exit(1);
            }
        }

        argPos += 1;
    }

    // Two legal file combinations
    // 1. x.cpp z.v
    // 2. x.sc y.h z.v

    if (m_inputPathName.size() == 0) {
        printf("Command line error: input file name not specified (.sc or .v)\n");
        exit(1);
    }

    m_inputFileName = m_inputPathName;
    int periodPos = m_inputFileName.find_last_of(".");
    m_inputFileName.erase(periodPos);
    int slashPos = m_inputFileName.find_last_of("/\\");
    if (slashPos > 0) m_inputFileName.erase(0, slashPos+1);

    if (verilogDir.size() != 0 && m_vFileName.size() != 0) {
        printf("Command line error: -vdir and <verilogFile> specified (only one allowed)\n");
        exit(1);
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
