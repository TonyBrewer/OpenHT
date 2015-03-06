/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#define DEC_SECTION		0

#define DEC_RSTINFO_QCNT	512
#define DEC_RSTINFO_QOFF	0x3

#define DEC_DHT_HUFF_QCNT		256
#define DEC_DCDHT0_HUFF_QOFF	0x210
#define DEC_DCDHT1_HUFF_QOFF	0x318
#define DEC_ACDHT0_HUFF_QOFF	0x420
#define DEC_ACDHT1_HUFF_QOFF	0x528

#define DEC_DHT_MAX_QCNT		32
#define DEC_DCDHT0_MAX_QOFF		0x210
#define DEC_DCDHT1_MAX_QOFF		0x318
#define DEC_ACDHT0_MAX_QOFF		0x420
#define DEC_ACDHT1_MAX_QOFF		0x528

#define DEC_DHT_VAL_QCNT		32
#define DEC_DCDHT0_VAL_QOFF		0x210
#define DEC_DCDHT1_VAL_QOFF		0x318
#define DEC_ACDHT0_VAL_QOFF		0x420
#define DEC_ACDHT1_VAL_QOFF		0x528

#define DEC_DHT_LOOK_QCNT		256
#define DEC_DCDHT0_LOOK_QOFF	0x210
#define DEC_DCDHT1_LOOK_QOFF	0x318
#define DEC_ACDHT0_LOOK_QOFF	0x420
#define DEC_ACDHT1_LOOK_QOFF	0x528

#define DEC_DQT_QCNT		16
#define DEC_DQT0_QOFF		0x628
#define DEC_DQT1_QOFF		0x638
#define DEC_DQT2_QOFF		0x648
#define DEC_DQT3_QOFF		0x658

#define DEC_DCP0_QOFF		0x668
#define DEC_DCP1_QOFF		0x66a
#define DEC_DCP2_QOFF		0x66c


#define ENC_SECTION		3

#define ENC_OFFSET		0xa3580

#define ENC_DHT_HUFF_QCNT	64
#define ENC_DCDHT0_HUFF_QOFF	0x8
#define ENC_DCDHT1_HUFF_QOFF	0x68
#define ENC_ACDHT0_HUFF_QOFF	0xc8
#define ENC_ACDHT1_HUFF_QOFF	0x128

#define ENC_DHT_SIZE_QCNT	32
#define ENC_DCDHT0_SIZE_QOFF	0x48
#define ENC_DCDHT1_SIZE_QOFF	0xa8
#define ENC_ACDHT0_SIZE_QOFF	0x108
#define ENC_ACDHT1_SIZE_QOFF	0x168

#define ENC_DQT_QCNT		16
#define ENC_DQT0_QOFF		0x188
#define ENC_DQT1_QOFF		0x198
#define ENC_DQT2_QOFF		0x1a8
#define ENC_DQT3_QOFF		0x1b8

#define ENC_ECP0_QOFF		0x1c8
#define ENC_ECP1_QOFF		0x1ca
#define ENC_ECP2_QOFF		0x1cc

#define ENC_RSTCNT_QOFF		0x1d0
#define ENC_RSTBASE_QOFF	0x1d1
#define ENC_RSTOFF_QOFF		0x1d2
#define ENC_RSTLEN_DCNT		512
#define ENC_RSTLEN_DOFF		0x3a5
