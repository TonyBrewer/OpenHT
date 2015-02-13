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

void CPersXbarStub::PersXbarStub()
{
	o_stubToXbar_reqRdy = false;
	o_stubToXbar_rdRspFull = false;
	o_stubToXbar_wrRspFull = false;

	CMemRdWrReqIntf memReq;
	memReq.m_type = 0;
	memReq.m_addr = 0;
	memReq.m_data = 0;
	memReq.m_tid = 0;

	o_stubToXbar_req = memReq;
}
