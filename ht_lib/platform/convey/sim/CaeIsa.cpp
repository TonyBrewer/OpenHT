/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include <stdio.h>
#include "CaeSim.h"
#include "CaeIsa.h"

#define AEUIE 0

#define M48 0xffffffffffffULL;
#define M32 0xffffffffULL;
#define M16 0xffffULL;


void
CCaeIsa::InitPers()
{
    SetAegCnt(5);
    for (int aeId = 0; aeId < CAE_AE_CNT; aeId += 1)
	for (int i = 0; i < 5; i += 1)
	    WriteAeg(aeId, i, 0);
    //SetPersNum(HT_SIG);
}


void
CCaeIsa::CaepInst(int aeId, int opcode, int immed, uint32 inst, uint64 scalar)
{
    // F7,0,20-3F
    switch (opcode) {
	default: {
	    printf("Default case hit - opcode = %x\n", opcode);
	    for (int aeId = 0; aeId < CAE_AE_CNT; aeId += 1)
		SetException(aeId, AEUIE);
	}

	// CAEP00 - Run it
	case 0x20: {
	    //uint64_t base = ReadAeg(aeId, 0) & M48;
	}
    }

    return;
}
