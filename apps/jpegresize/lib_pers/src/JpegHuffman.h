/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#pragma once

#include "JobInfo.h"

enum { DC_LUM_CODES = 12, AC_LUM_CODES = 256, DC_CHROMA_CODES = 12, AC_CHROMA_CODES = 256, MAX_HUFF_SYMBOLS = 257, MAX_HUFF_CODESIZE = 32 };

extern uint8_t s_dc_lum_bits[17];
extern uint8_t s_dc_lum_val[DC_LUM_CODES];
extern uint8_t s_ac_lum_bits[17];
extern uint8_t s_ac_lum_val[AC_LUM_CODES];
extern uint8_t s_dc_chroma_bits[17];
extern uint8_t s_dc_chroma_val[DC_CHROMA_CODES];
extern uint8_t s_ac_chroma_bits[17];
extern uint8_t s_ac_chroma_val[AC_CHROMA_CODES];

void compute_huffman_table(uint16_t *codes, uint8_t *code_sizes, uint8_t *bits, uint8_t *val);
void calcDhtDerivedTbl( JobDht & jobDht );
void compute_huffman_table(uint16_t *codes, uint8_t *code_sizes, JobDht & jobDht );
