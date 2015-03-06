/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#pragma once

#include "MyInt.h"

struct HorzResizeMsg {
	HorzResizeMsg( my_uint10 mcuRowFirst, my_uint10 mcuColFirst, bool bEndOfImage )
		: m_mcuRowFirst(mcuRowFirst), m_mcuColFirst(mcuColFirst), m_bEndOfImage(bEndOfImage) {}

	my_uint10	m_mcuRowFirst;
	my_uint10	m_mcuColFirst;
	bool		m_bEndOfImage;
};
