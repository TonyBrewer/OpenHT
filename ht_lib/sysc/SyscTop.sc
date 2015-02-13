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
#include "SyscAeTop.h"

ht_topology_top
SC_MODULE(CSyscTop) {
	sc_signal<uint8_t> aeId[HT_AE_CNT];
	CSyscAeTop * pSyscAeTop[HT_AE_CNT];

	SC_CTOR(CSyscTop) {
		for (int i = 0; i < HT_AE_CNT; i += 1) {
			aeId[i] = i;
			pSyscAeTop[i] = new CSyscAeTop("SyscAeTop[%i]");
			pSyscAeTop[i]->i_aeId(aeId[i]);
			pSyscAeTop[i]->o_mon_valid(mon_valid[i]);
			pSyscAeTop[i]->o_mon_data(mon_data[i]);
			pSyscAeTop[i]->i_mon_stall(mon_stall[i]);
			pSyscAeTop[i]->i_mip_valid(mon_valid[(i+HT_AE_CNT-1)%HT_AE_CNT]);
			pSyscAeTop[i]->i_mip_data(mon_data[(i+HT_AE_CNT-1)%HT_AE_CNT]);
			pSyscAeTop[i]->o_mip_stall(mon_stall[(i+HT_AE_CNT-1)%HT_AE_CNT]);
			pSyscAeTop[i]->i_min_valid(mop_valid[i]);
			pSyscAeTop[i]->i_min_data(mop_data[i]);
			pSyscAeTop[i]->o_min_stall(mop_stall[i]);
			pSyscAeTop[i]->o_mop_valid(mop_valid[(i+HT_AE_CNT+1)%HT_AE_CNT]);
			pSyscAeTop[i]->o_mop_data(mop_data[(i+HT_AE_CNT+1)%HT_AE_CNT]);
			pSyscAeTop[i]->i_mop_stall(mop_stall[(i+HT_AE_CNT+1)%HT_AE_CNT]);
		}
	}
};
