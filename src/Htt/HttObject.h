/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#pragma once

class CHttObject {
public:
	CHttObject(CHttOperand * pOp=0) : m_pOp(pOp), m_objWidth(0) {}

	CHttOperand * m_pOp;
	int m_objWidth;
	string m_objName;
	string m_objPosName;
	string m_rtnFieldTempVar;
	CHttObject * m_pParamObj;
};
