/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#define HT_LIB_SYSC
#include "SyscDisp.h"

#ifndef _HTV
namespace Ht {
	struct CHtCtrlMsg;
	extern CHtCtrlMsg *g_pCtrlIntfQues[];
	extern int g_coprocAeRunningCnt;
}
#endif

void CSyscDisp::SyscDisp()
{
	/////////////////////////
	// Combinatorial logic

	sc_uint<4> c_dispState = r_dispState;

	///////////////////////
	// Registers
	//
	if (r_dispState < 10) // 10 clocks of reset
		c_dispState = r_dispState + 1;

	bool busy = i_hifToDisp_dispBusy.read() == 1;
	bool idle = !i_hifToDisp_dispBusy.read();

	if (busy && r_dispState == 10) {
		c_dispState = 11;
	}

	if (idle && r_dispState == 11) {
		c_dispState = 12;
		Ht::g_coprocAeRunningCnt -= 1;
	}

	r_dispState = i_reset.read() ? (sc_uint<4>)0 : c_dispState;

	uint64_t ctlIntfQue = (uint64_t)Ht::g_pCtrlIntfQues[i_aeId.read()];
	r_ctlQueWidth = ctlIntfQue >> 48;
	r_ctlQueBase  = ctlIntfQue & 0x0000FFFFFFFFFFFFLL;

	///////////////////////
	// Update outputs
	//

	o_dispToHif_ctlQueWidth = r_ctlQueWidth;
	o_dispToHif_ctlQueBase = r_ctlQueBase;
	o_dispToHif_dispStart = r_dispState == 9;
}
