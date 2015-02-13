/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
// Htt.cpp : Defines the entry point for the console application.
//

#include "Htt.h"
#include "HttArgs.h"
#include "HttDesign.h"

#if defined(_WIN32) || defined (NO_LIC_CHK)
# define convey_lic_ck_rl(x)
#else
extern "C" {
    extern int convey_lic_ck_rl(const char *);
}
#endif

int main(int argc, char* argv[])
{
    convey_lic_ck_rl("Convey_HT");

    g_httArgs.Parse(argc, argv);

    CHttDesign httDesign;

    httDesign.OpenFiles();

    httDesign.Parse();

    httDesign.CheckWidthRules();

    httDesign.CheckSignalRules();

    httDesign.CheckReferenceRules();

    httDesign.CheckSequenceRules();

    httDesign.ConstructGraph();

    //if (g_httArgs.IsScCode())
    //    httDesign.GenVerilog();

    //if (g_httArgs.IsGenSandbox())
    //    httDesign.GenSandbox();

    if (GetErrorCnt() == 0)
        // Files are deleted if not closed
        httDesign.CloseFiles();

    //printf("CPU time = %4.2f\n", (double)clock() / CLOCKS_PER_SEC);
    return GetErrorCnt();
}
