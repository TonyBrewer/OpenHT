/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "Htfe.h"
#include "HtfeDesign.h"

void CHtfeSignal::AddReference(CHtfeInstancePort *pInstancePort) {
	m_refList.push_back(pInstancePort);
	switch (pInstancePort->GetModulePort()->GetPortDir()) {
	case port_in: m_inRefCnt += 1; break;
	case port_out: m_outRefCnt += 1; break;
	case port_inout: m_inoutRefCnt += 1; break;
	default: break; 
	}
}
