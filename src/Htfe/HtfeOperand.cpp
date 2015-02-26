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

int CHtfeOperand::GetSubFieldWidth()
{
    int subFieldWidth = GetMember()->GetWidth();

    CScSubField *pSubField = GetSubField();
    for ( ; pSubField; pSubField = pSubField->m_pNext)
        subFieldWidth = pSubField->m_subFieldWidth;

    return subFieldWidth;
}

CHtfeIdent * CHtfeOperand::GetType()
{
	if (m_pType)
		return m_pType;
	else if (m_pMember)
		return m_pMember->IsType() ? m_pMember : m_pMember->GetType();
	else if (!m_bIsLeaf && m_operatorToken == CHtfeLex::tk_typeCast)
		return m_pOperand1->GetType();
	else if (IsConstValue()) {
		CConstValue const & value = GetConstValue();
		switch (value.GetConstType()) {
		case CConstValue::SINT64: return CHtfeDesign::m_pInt64Type;
		case CConstValue::SINT32: return CHtfeDesign::m_pIntType;
		case CConstValue::UINT64: return CHtfeDesign::m_pUInt64Type;
		case CConstValue::UINT32: return CHtfeDesign::m_pUIntType;
		default: Assert(0);
		}
	}
	return 0;
}
