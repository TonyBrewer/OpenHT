/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#pragma once

#include "Htt.h"
#include "HttArgs.h"

CHttArgs g_httArgs;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CHttArgs::CHttArgs()
{
    m_bGenSandbox = false;
}

CHttArgs::~CHttArgs()
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

void CHttArgs::Usage(const char *progname)
{
    printf("\nUsage: %s [options] <design>.cpp\n", progname);
    printf("  where: design.cpp is the htl .cpp output file\n");
    printf("\n");
    printf("Version: %s-%s (%s)\n", VERSION, VCSREV, BLDDTE);
    printf("Options:\n");
    printf("  -help, -h\n");
    printf("     Prints this usage message\n");

	CHtfeArgs::Usage();

    printf("  -sbx\n");
    printf("     Generates a sandbox in addition to the normal output files\n");
    printf("\n");
}

void CHttArgs::Parse(int argc, char *argv[])
{
	m_argc = argc;
	m_argv = argv;

#ifndef __GNUG__
    if (argc <= 1) {
        argc = 1;
        char *argv2[20];

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

        //SetCurrentDirectory("C:/Users/TBrewer/Documents/Visual Studio 2010/Projects/PacBioCorrector/MemCorruptHttBug/msvs10");
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
  //      argv2[argc++] = "-I";
  //      argv2[argc++] = "../src_pers";
  //      argv2[argc++] = "-I";
  //      argv2[argc++] = "../ht/sysc";
  //      argv2[argc++] = "-I";
  //      argv2[argc++] = "C:/ht/ht_lib";
  //      argv2[argc++] = "../ht/sysc/PersAuBcm.cpp";
  //      argv2[argc++] = "../ht/verilog/PersAuBcm.v";

        SetCurrentDirectory("C:/Ht/tests/clk2x/msvs10");
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
        argv2[argc++] = "../ht/sysc/PersAuClk1x.cpp";

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

    while (argPos < argc) {
        if (argv[argPos][0] == '-') {
            if ((strcmp(argv[argPos], "-h") == 0) ||
                (strcmp(argv[argPos], "-help") == 0)) {
                    Usage(argv[0]);
                    exit(1);
            } else if (strcmp(argv[argPos], "-sbx") == 0) {
                SetGenSandbox(true);

			} else
				CHtfeArgs::Parse(argPos, argv);

        } else {

            string fileName = argv[argPos];
            size_t nameLen = fileName.length();
            string suffix4 = nameLen > 4 ? fileName.substr(fileName.length()-4, 4) : "";
            string suffix3 = nameLen > 3 ? fileName.substr(fileName.length()-3, 3) : "";
            string suffix2 = nameLen > 2 ? fileName.substr(fileName.length()-2, 2) : "";

            if (suffix4 == ".cpp") {

				SetIsScCode();

                if (m_inputPathName.size() != 0) {
                    printf("Command line error: multiple input file names specified (.cpp)\n");
                    exit(1);
                }

                m_inputPathName = argv[argPos];

             } else {
                printf("Command line error: Extra file specified '%s'\n", argv[argPos]);
                exit(1);
            }
        }

        argPos += 1;
    }

    // One legal file combination
    // 1. x.cpp

    if (m_inputPathName.size() == 0) {
        printf("Command line error: input file name not specified\n");
        exit(1);
    }

    m_inputFileName = m_inputPathName;
    int periodPos = m_inputFileName.find_last_of(".");
    m_inputFileName.erase(periodPos);

	m_incOutFileName = m_inputFileName + "_htt.h";
	m_cppOutFileName = m_inputFileName + "_htt.cpp";

    int slashPos = m_inputFileName.find_last_of("/\\");
    if (slashPos > 0) m_inputFileName.erase(0, slashPos+1);
}
