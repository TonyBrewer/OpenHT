/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
// ScArgs.cpp: implementation of the ScArgs class.
//
//////////////////////////////////////////////////////////////////////

#include "Htfe.h"
#include "HtfeErrorMsg.h"
#include "HtfeArgs.h"

CHtfeArgs * g_pHtfeArgs;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CHtfeArgs::CHtfeArgs()
{
	g_pHtfeArgs = this;

    m_bScMain = false;
    m_bScHier = false;
    m_bScCode = false;
    m_bIsTkDumpEnabled = false;
    m_bIsStDumpEnabled = false;
    m_bIsLvxEnabled = false;
    m_bWritePreProcessedInput = false;
	m_bMethodSupport = false;
	m_bTemplateSupport = false;
    m_bHtQueCntSynKeep = false;
}

CHtfeArgs::~CHtfeArgs()
{

}

void CHtfeArgs::Usage()
{
    printf("  -I <directory>\n");
    printf("     Directory to search for include files\n");
    printf("  -D<defineName>=x\n");
    printf("     Sets <defineName> to the value x for the preprocessor (default value is 1)\n");
    printf("  -lvx\n");
    printf("     Initialize uninitialized local variables to undefined in the output\n");
    printf("     verilog to allow more aggressive logic reduction. (default is initialize to 0)\n");
}

void CHtfeArgs::Parse(int & argPos, char *argv[])
{
	if (strncmp(argv[argPos], "-D", 2) == 0) {
        m_preDefinedNames.push_back(argv[argPos]+2);
    } else if (strncmp(argv[argPos], "-template", 9) == 0) {
        SetTemplateSupport(true);
    } else if (strncmp(argv[argPos], "-method", 7) == 0) {
        SetMethodSupport(true);
    } else if (strncmp(argv[argPos], "-TkDump", 7) == 0) {
        SetIsTkDumpEnabled();
    } else if (strncmp(argv[argPos], "-StDump", 7) == 0) {
        SetIsStDumpEnabled();
    } else if (strncmp(argv[argPos], "-InUse", 6) == 0) {
        printf("-InUse flag has been depricated\n");
		ErrorExit();
    } else if (strncmp(argv[argPos], "-lvx", 4) == 0) {
        SetIsLvxEnabled();
    } else if (strncmp(argv[argPos], "-wpp", 4) == 0) {
        SetWritePreProcessedInput();
    } else if (strcmp(argv[argPos], "-I") == 0) {
        string path = argv[++argPos];
        if (path.find_last_of("/\\") != path.size()-1)
            path += "/";
        m_includeDirs.push_back(path);
    } else {
        printf("Unknown command line argument '%s'\n", argv[argPos]);
        ErrorExit();
    }
}
