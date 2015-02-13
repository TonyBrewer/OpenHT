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
