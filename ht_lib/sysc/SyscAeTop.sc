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
#include "PersAeTop.h"

SC_MODULE(CSyscAeTop) {
	CSyscClock * pSyscClock;
	CSyscDisp * pSyscDisp;
	CPersAeTop * pPersAeTop;
	CSyscMem * pSyscMem[SYSC_AE_MEM_CNT];

	SC_CTOR(CSyscAeTop) {

		pSyscClock = new CSyscClock("SyscClock");

		pSyscDisp = new CSyscDisp("SyscDisp");

		pPersAeTop = new CPersAeTop("PersAeTop");
	
		for (int i = 0; i < SYSC_AE_MEM_CNT; i += 1) {
			pSyscMem[i] = new CSyscMem("SyscMem[%i]");
			pSyscMem[i]->i_mifToXbar_rdRspFull(mifToXbar_rdRspFull[i]);
			pSyscMem[i]->i_mifToXbar_req(mifToXbar_req[i]);
			pSyscMem[i]->i_mifToXbar_reqRdy(mifToXbar_reqRdy[i]);
			pSyscMem[i]->i_mifToXbar_wrRspFull(mifToXbar_wrRspFull[i]);
			pSyscMem[i]->o_xbarToMif_rdRsp(xbarToMif_rdRsp[i]);
			pSyscMem[i]->o_xbarToMif_rdRspRdy(xbarToMif_rdRspRdy[i]);
			pSyscMem[i]->o_xbarToMif_reqFull(xbarToMif_reqFull[i]);
			pSyscMem[i]->o_xbarToMif_wrRspRdy(xbarToMif_wrRspRdy[i]);
			pSyscMem[i]->o_xbarToMif_wrRspTid(xbarToMif_wrRspTid[i]);
		}
	}
};
