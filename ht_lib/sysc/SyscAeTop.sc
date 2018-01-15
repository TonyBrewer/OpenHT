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
#include "SyscClockHxStub.h"
#if defined(HT_UIO) && defined (HT_VERILOG)
#include "SyscUio.h"
#endif

SC_MODULE(CSyscAeTop) {
	CSyscClock * pSyscClock;
        CSyscClockHxStub * pSyscClockHxStub;
	CSyscDisp * pSyscDisp;
	CPersAeTop * pPersAeTop;
	CSyscMem * pSyscMem[SYSC_AE_MEM_CNT];
#if defined(HT_UIO) && defined (HT_VERILOG)
	CSyscUio * pSyscUio[SYSC_AE_UIO_CNT];
#endif

	SC_CTOR(CSyscAeTop) {

		pSyscClock = new CSyscClock("SyscClock");

		pSyscClockHxStub = new CSyscClockHxStub("SyscClockHxStub");

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
	
#if defined(HT_UIO) && defined (HT_VERILOG)
		for (int i = 0; i < SYSC_AE_UIO_CNT; i += 1) {
			pSyscUio[i] = new CSyscUio("SyscUio[%i]");
			pSyscUio[i]->i_auToPort_uio_Data(auToPort_uio_Data[i]);
			pSyscUio[i]->i_auToPort_uio_Rdy(auToPort_uio_Rdy[i]);
			pSyscUio[i]->i_portToAu_uio_AFull(portToAu_uio_AFull[i]);
			pSyscUio[i]->o_auToPort_uio_AFull(auToPort_uio_AFull[i]);
			pSyscUio[i]->o_portToAu_uio_Data(portToAu_uio_Data[i]);
			pSyscUio[i]->o_portToAu_uio_Rdy(portToAu_uio_Rdy[i]);
		}
#endif

	}
};
