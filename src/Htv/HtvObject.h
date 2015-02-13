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

class CHtvObject {
public:
	CHtvObject(CHtvOperand * pOp=0) : m_pOp(pOp) {}

	CHtvOperand * m_pOp;
};
