/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
// Htv.cpp : Defines the entry point for the console application.
//

#include "Htv.h"
#include "HtvArgs.h"
#include "HtvDesign.h"

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

    g_htvArgs.Parse(argc, argv);
	
    CHtvDesign htvDesign;

    htvDesign.OpenFiles();

    htvDesign.Parse();

    htvDesign.CheckWidthRules();

    htvDesign.CheckSignalRules();

    htvDesign.CheckReferenceRules();

    htvDesign.CheckSequenceRules();

    htvDesign.PreSynthesis();

    if (g_htvArgs.IsScCode())
        htvDesign.GenVerilog();

    if (g_htvArgs.IsGenSandbox())
        htvDesign.GenSandbox();

    if (GetErrorCnt() == 0)
        // Files are deleted if not closed
        htvDesign.CloseFiles();
	else
		htvDesign.DeleteFiles();

    //printf("CPU time = %4.2f\n", (double)clock() / CLOCKS_PER_SEC);
    return GetErrorCnt();
}
