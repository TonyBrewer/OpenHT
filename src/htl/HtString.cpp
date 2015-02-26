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

CDefineTable * CHtString::m_pDefineTable;
CDefineTable * CHtIntParam::m_pDefineTable;

void CHtString::InitValue(CLineInfo const &lineInfo, bool bRequired, int defValue, bool bIsSigned)
{
	if (m_bIsValid)
		return;

	if (m_string.size() == 0 && !bRequired) {

		m_value = defValue;
		m_bIsSigned = bIsSigned;

	} else if (!m_pDefineTable->FindStringValue(lineInfo, m_string, m_value, m_bIsSigned)) {

		m_value = defValue;
		m_bIsSigned = bIsSigned;

		CPreProcess::ParseMsg(Error, lineInfo, "unable to determine value for '%s'", m_string.c_str());
	}

	m_bIsValid = true;
}

void CHtIntParam::SetValue(CLineInfo & lineInfo, string & strValue, bool bRequired, int defValue)
{
	if (strValue.size() == 0 && !bRequired) {

		m_value = defValue;

	} else if (!m_pDefineTable->FindStringValue(lineInfo, strValue, m_value, m_bIsSigned)) {

		m_value = defValue;

		CPreProcess::ParseMsg(Error, lineInfo, "unable to determine value for '%s'", strValue.c_str());
	}

	m_bIsSet = true;

}