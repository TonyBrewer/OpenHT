/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#pragma once

#include "Ht.h"

inline ht_int10 descale(ht_int21 x, int n) {
	return ( (int)x + (1 << (n-1))) >> n; 
}

inline uint8_t clampPixel(ht_int10 inInt)
{
	if (inInt < 0)
		return 0;
	if (inInt > 255)
		return 255;
	return (uint8_t) inInt;
}
