/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#pragma once

#include <stdint.h>

#define JPEG_ENC_CONST_BITS  13
#define JPEG_ENC_PASS1_BITS  2
#define JPEG_ENC_RECIP_BITS	8

void jpegFdctDV (uint8_t (&inBlock)[8][8], int16_t (&coefBlock)[8][8]);
