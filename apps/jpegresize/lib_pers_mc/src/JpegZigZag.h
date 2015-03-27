/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#pragma once

#include <stdint.h>

struct JpegZigZag {
	uint8_t m_r;
	uint8_t m_c;
};

extern JpegZigZag jpegZigZag[64];
