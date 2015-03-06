/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include <stdint.h>

#include "JpegCommon.h"
#include "JobInfo.h"
#include "MyInt.h"

static inline int descale(int64_t x, int n) {
	return ( (int)x + (1 << (n-1))) >> n; 
}

/*
* Each IDCT routine is responsible for range-limiting its results and
* converting them to unsigned form (0..MAXJSAMPLE).  The raw outputs could
* be quite far out of range if the input data is corrupt, so a bulletproof
* range-limiting step is required.  We use a mask-and-table-lookup method
* to do the combined operations quickly.  See the comments with
* prepare_range_limit_table (in jdmaster.c) for more info.
*/

#define MAXJSAMPLE	255
#define RANGE_MASK  (MAXJSAMPLE * 4 + 3) /* 2 bits wider than legal samples */

#define CONST_BITS  13
#define PASS1_BITS  2

#define FIX_0_298631336  ((int16_t)  2446)	/* FIX(0.298631336) */
#define FIX_0_390180644  ((int16_t)  3196)	/* FIX(0.390180644) */
#define FIX_0_541196100  ((int16_t)  4433)	/* FIX(0.541196100) */
#define FIX_0_765366865  ((int16_t)  6270)	/* FIX(0.765366865) */
#define FIX_0_899976223  ((int16_t)  7373)	/* FIX(0.899976223) */
#define FIX_1_175875602  ((int16_t)  9633)	/* FIX(1.175875602) */
#define FIX_1_501321110  ((int16_t)  12299)	/* FIX(1.501321110) */
#define FIX_1_847759065  ((int16_t)  15137)	/* FIX(1.847759065) */
#define FIX_1_961570560  ((int16_t)  16069)	/* FIX(1.961570560) */
#define FIX_2_053119869  ((int16_t)  16819)	/* FIX(2.053119869) */
#define FIX_2_562915447  ((int16_t)  20995)	/* FIX(2.562915447) */
#define FIX_3_072711026  ((int16_t)  25172)	/* FIX(3.072711026) */

/*
* Perform dequantization and inverse DCT on one block of coefficients.
*/

uint8_t jpegIdctRangeLimit(int x)
{
	if (x < 0x80)
		return x | 0x80;
	if (x < 0x200)
		return 0xff;
	if (x < 0x380)
		return 0;
	return x & 0x7f;
}

void jpegIdct ( JobInfo * pJobInfo, int ci, int16_t (&coefBlock)[8][8], uint8_t * outBlock[8] )
{
	int16_t ws[8][8];	/* buffers data between passes */

	/* Pass 1: process columns from input, store into work array. */
	/* Note results are scaled up by sqrt(8) compared to a true IDCT; */
	/* furthermore, we scale the results by 2**PASS1_BITS. */

	int dqtId = pJobInfo->m_dec.m_dcp[ci].m_dqtId;
	int16_t (&quantTbl)[8][8] = pJobInfo->m_dec.m_dqt[dqtId].m_quantTbl;

	for (uint64_t row = 0; row < DCTSIZE; row++) {

		my_int24 a0 = coefBlock[row][0] * quantTbl[row][0];
		my_int24 a1 = coefBlock[row][2] * quantTbl[row][2];
		my_int24 a2 = coefBlock[row][4] * quantTbl[row][4];
		my_int24 a3 = coefBlock[row][6] * quantTbl[row][6];

		my_int27 b = (a1 + a3) * FIX_0_541196100;

		my_int27 c0 = (a0 + a2) << CONST_BITS;
		my_int27 c1 = b + a1 * FIX_0_765366865;  
		my_int27 c2 = (a0 - a2) << CONST_BITS;
		my_int27 c3 = b + a3 * - FIX_1_847759065;

		my_int27 d0 = c0 + c1;
		my_int27 d1 = c0 - c1;
		my_int27 d2 = c2 + c3;
		my_int27 d3 = c2 - c3;

		my_int24 e0 = coefBlock[row][7] * quantTbl[row][7];
		my_int24 e1 = coefBlock[row][5] * quantTbl[row][5];
		my_int24 e2 = coefBlock[row][3] * quantTbl[row][3];
		my_int24 e3 = coefBlock[row][1] * quantTbl[row][1];

		my_int27 f0 = e0 + e3;
		my_int27 f1 = e1 + e2;
		my_int27 f2 = e0 + e2;
		my_int27 f3 = e1 + e3;

		my_int27 g0 = e0 * FIX_0_298631336;
		my_int27 g1 = e1 * FIX_2_053119869;
		my_int27 g2 = e2 * FIX_3_072711026;
		my_int27 g3 = e3 * FIX_1_501321110;

		my_int27 h = (f2 + f3) * FIX_1_175875602;

		my_int27 i0 = f0 * - FIX_0_899976223;
		my_int27 i1 = f1 * - FIX_2_562915447;
		my_int27 i2 = h + f2 * - FIX_1_961570560;
		my_int27 i3 = h + f3 * - FIX_0_390180644;

		my_int27 j0 = g0 + i0 + i2;
		my_int27 j1 = g1 + i1 + i3;
		my_int27 j2 = g2 + i1 + i2;
		my_int27 j3 = g3 + i0 + i3;

		ws[row][0] = (int16_t) descale(d0 + j3, CONST_BITS-PASS1_BITS);
		ws[row][4] = (int16_t) descale(d1 - j0, CONST_BITS-PASS1_BITS);
		ws[row][2] = (int16_t) descale(d3 + j1, CONST_BITS-PASS1_BITS);
		ws[row][6] = (int16_t) descale(d2 - j2, CONST_BITS-PASS1_BITS);
		ws[row][1] = (int16_t) descale(d2 + j2, CONST_BITS-PASS1_BITS);
		ws[row][5] = (int16_t) descale(d3 - j1, CONST_BITS-PASS1_BITS);
		ws[row][3] = (int16_t) descale(d1 + j0, CONST_BITS-PASS1_BITS);
		ws[row][7] = (int16_t) descale(d0 - j3, CONST_BITS-PASS1_BITS);
	}

	/* Pass 2: process rows from work array, store into output array. */
	/* Note that we must descale the results by a factor of 8 == 2**3, */
	/* and also undo the PASS1_BITS scaling. */

	for (uint64_t col = 0; col < DCTSIZE; col += 1) {

		my_int28 k = (ws[2][col] + ws[6][col]) * FIX_0_541196100;

		my_int28 m0 = (ws[0][col] + ws[4][col]) << CONST_BITS;
		my_int28 m1 = (ws[0][col] - ws[4][col]) << CONST_BITS;
		my_int28 m2 = k + ws[6][col] * - FIX_1_847759065;
		my_int28 m3 = k + ws[2][col] * FIX_0_765366865; 

		my_int28 n0 = m0 + m3;
		my_int28 n3 = m0 - m3;
		my_int28 n1 = m1 + m2;
		my_int28 n2 = m1 - m2;

		my_int17 p0 = ws[7][col] + ws[1][col];
		my_int17 p1 = ws[5][col] + ws[3][col];
		my_int17 p2 = ws[7][col] + ws[3][col];
		my_int17 p3 = ws[5][col] + ws[1][col];

		my_int28 q = (p2 + p3) * FIX_1_175875602;

		my_int28 r0 = ws[7][col] * FIX_0_298631336;
		my_int28 r1 = ws[5][col] * FIX_2_053119869;
		my_int28 r2 = ws[3][col] * FIX_3_072711026;
		my_int28 r3 = ws[1][col] * FIX_1_501321110;

		my_int28 s1 = p0 * - FIX_0_899976223;
		my_int28 s2 = p1 * - FIX_2_562915447;
		my_int28 s3 = q + p2 * - FIX_1_961570560;
		my_int28 s4 = q + p3 * - FIX_0_390180644;

		my_int28 t0 = r0 + s1 + s3;
		my_int28 t1 = r1 + s2 + s4;
		my_int28 t2 = r2 + s2 + s3;
		my_int28 t3 = r3 + s1 + s4;

		/* Final output stage */

		outBlock[0][col] = jpegIdctRangeLimit((int) descale(n0 + t3, CONST_BITS+PASS1_BITS+3) & RANGE_MASK);
		outBlock[7][col] = jpegIdctRangeLimit((int) descale(n0 - t3, CONST_BITS+PASS1_BITS+3) & RANGE_MASK);
		outBlock[1][col] = jpegIdctRangeLimit((int) descale(n1 + t2, CONST_BITS+PASS1_BITS+3) & RANGE_MASK);
		outBlock[6][col] = jpegIdctRangeLimit((int) descale(n1 - t2, CONST_BITS+PASS1_BITS+3) & RANGE_MASK);
		outBlock[2][col] = jpegIdctRangeLimit((int) descale(n2 + t1, CONST_BITS+PASS1_BITS+3) & RANGE_MASK);
		outBlock[5][col] = jpegIdctRangeLimit((int) descale(n2 - t1, CONST_BITS+PASS1_BITS+3) & RANGE_MASK);
		outBlock[3][col] = jpegIdctRangeLimit((int) descale(n3 + t0, CONST_BITS+PASS1_BITS+3) & RANGE_MASK);
		outBlock[4][col] = jpegIdctRangeLimit((int) descale(n3 - t0, CONST_BITS+PASS1_BITS+3) & RANGE_MASK);
	}
}
