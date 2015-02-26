/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "CnyHt.h"
#include "DsnInfo.h"
#include "CxrIntf.h"

const char *
CCxrIntf::GetSrcToDstUc()
{
	if (m_srcToDstUc.size() == 0)
		m_srcToDstUc = m_pSrcMod->m_modName.Uc() + "To" + m_pDstMod->m_modName.Uc();
	return m_srcToDstUc.c_str();
}

const char *
CCxrIntf::GetSignalDstToSrcLc()
{
	if (m_signalDstToSrcLc.size() == 0) {
		m_signalDstToSrcLc = m_pDstMod->m_modName.Lc();
		if (m_pDstMod->m_modInstList.size() > 1) {
			char digit[2] = { (char)('0' + m_dstInstIdx), '\0' };
			m_signalDstToSrcLc += digit;
		}
		m_signalDstToSrcLc += "To";
		m_signalDstToSrcLc += m_pSrcMod->m_modName.Uc();
		if (m_pSrcMod->m_modInstList.size() > 1) {
			char digit[2] = { (char)('0' + m_srcInstIdx), '\0' };
			m_signalDstToSrcLc += digit;
		}
	}
	return m_signalDstToSrcLc.c_str();
}

const char *
CCxrIntf::GetSignalSrcToDstLc()
{
	if (m_signalSrcToDstLc.size() == 0) {
		m_signalSrcToDstLc = m_pSrcMod->m_modName.Lc();
		if (m_pSrcMod->m_modInstList.size() > 1) {
			char digit[2] = { (char)('0' + m_srcInstIdx), '\0' };
			m_signalSrcToDstLc += digit;
		}
		m_signalSrcToDstLc += "To";
		m_signalSrcToDstLc += m_pDstMod->m_modName.Uc();
		if (m_pDstMod->m_modInstList.size() > 1) {
			char digit[2] = { (char)('0' + m_dstInstIdx), '\0' };
			m_signalSrcToDstLc += digit;
		}
	}
	return m_signalSrcToDstLc.c_str();
}

const char *
CCxrIntf::GetSignalNameDstToSrcLc()
{
	if (m_signalNameDstToSrcLc.size() == 0) {
		m_signalNameDstToSrcLc = m_pDstMod->m_modName.Lc();
		m_signalNameDstToSrcLc += "To";
		m_signalNameDstToSrcLc += m_pSrcMod->m_modName.Uc();
	}
	return m_signalNameDstToSrcLc.c_str();
}

const char *
CCxrIntf::GetSignalNameSrcToDstLc()
{
	if (m_signalNameSrcToDstLc.size() == 0) {
		m_signalNameSrcToDstLc = m_pSrcMod->m_modName.Lc();
		m_signalNameSrcToDstLc += "To";
		m_signalNameSrcToDstLc += m_pDstMod->m_modName.Uc();
	}
	return m_signalNameSrcToDstLc.c_str();
}

const char *
CCxrIntf::GetSignalIndexDstToSrc()
{
	if (m_signalIndexDstToSrc.size() == 0) {
		if (m_pDstMod->m_modInstList.size() >= m_pSrcMod->m_modInstList.size() && m_pDstMod->m_modInstList.size() > 1) {

			m_signalIndexDstToSrc += VA("[%d]", m_dstInstIdx);

		} else if (m_pSrcMod->m_modInstList.size() > 1) {

			m_signalIndexDstToSrc += VA("[%d]", m_srcInstIdx);
		}
	}
	return m_signalIndexDstToSrc.c_str();
}

const char *
CCxrIntf::GetSignalIndexSrcToDst()
{
	if (m_signalIndexSrcToDst.size() == 0) {
		if (m_pSrcMod->m_modInstList.size() >= m_pDstMod->m_modInstList.size() && m_pSrcMod->m_modInstList.size() > 1) {

			m_signalIndexSrcToDst += VA("[%d]", m_srcInstIdx);

		} else if (m_pDstMod->m_modInstList.size() > 1) {

			m_signalIndexSrcToDst += VA("[%d]", m_dstInstIdx);
		}
	}
	return m_signalIndexSrcToDst.c_str();
}

///////////////////////////////
// Port name/index

const char *
CCxrIntf::GetPortNameDstToSrcLc()
{
	if (m_portNameDstToSrcLc.size() == 0) {
		m_portNameDstToSrcLc = m_pDstMod->m_modName.Lc();
		m_portNameDstToSrcLc += "To";
		m_portNameDstToSrcLc += m_pSrcMod->m_modName.Uc();
	}
	return m_portNameDstToSrcLc.c_str();
}

const char *
CCxrIntf::GetPortNameSrcToDstLc()
{
	if (m_portNameSrcToDstLc.size() == 0) {
		m_portNameSrcToDstLc = m_pSrcMod->m_modName.Lc();
		m_portNameSrcToDstLc += "To";
		m_portNameSrcToDstLc += m_pDstMod->m_modName.Uc();
	}
	return m_portNameSrcToDstLc.c_str();
}

const char * CCxrIntf::GetPortReplIndex()
{
	if (m_portReplIndex.size() == 0) {
		if (m_cxrDir == CxrIn) {
			if (m_srcReplCnt > 1)
				m_portReplIndex = VA("[%d]", m_srcReplId);
		} else {
			if (m_dstReplCnt > 1)
				m_portReplIndex = VA("[%d]", m_dstReplId);
		}
	}
	return m_portReplIndex.c_str();
}

const char *
CCxrIntf::GetPortReplDecl()
{
	if (m_portReplDecl.size() == 0) {
		if (m_cxrDir == CxrIn) {
			if (m_srcReplCnt > 1)
				m_portReplDecl = VA("[%d]", m_srcReplCnt);
		} else {
			if (m_dstReplCnt > 1)
				m_portReplDecl = VA("[%d]", m_dstReplCnt);
		}
	}
	return m_portReplDecl.c_str();
}

vector<CHtString> & CCxrIntf::GetPortReplDimen()
{
	if (m_cxrDir == CxrIn) {
		if (m_srcReplCnt > 1 && m_portReplDimen.size() == 0) {
			CHtString dimStr = m_srcReplCnt;
			m_portReplDimen.push_back(dimStr);
		}
	} else {
		if (m_dstReplCnt > 1 && m_portReplDimen.size() == 0) {
			CHtString dimStr =m_dstReplCnt;
			m_portReplDimen.push_back(dimStr);
		}
	}
	return m_portReplDimen;
}
