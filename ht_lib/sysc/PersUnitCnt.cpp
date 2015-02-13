/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "Ht.h"

void CPersUnitCnt::PersUnitCnt()
{
	bool c_hifToDisp_dispBusy = false;
	for (int i = 0; i<HT_UNIT_CNT; i += 1)
		c_hifToDisp_dispBusy |= i_hifToDisp_busy[i].read() != 0;

	o_unitCnt = HT_UNIT_CNT;
	o_hifToDisp_dispBusy = c_hifToDisp_dispBusy;
}
