#include <stdint.h>

#include "JpegCommon.h"
#include "JobInfo.h"
#include "JpegFdct.h"
#include "MyInt.h"

static inline int descale(int64_t x, int n) {
	return ( (int)x + (1 << (n-1))) >> n; 
}

/*
* jfdctint.c
*
* Copyright (C) 1991-1996, Thomas G. Lane.
* This file is part of the Independent JPEG Group's software.
* For conditions of distribution and use, see the accompanying README file.
*
* This file contains a slow-but-accurate integer implementation of the
* forward DCT (Discrete Cosine Transform).
*
* A 2-D DCT can be done by 1-D DCT on each row followed by 1-D DCT
* on each column.  Direct algorithms are also available, but they are
* much more complex and seem not to be any faster when reduced to code.
*
* This implementation is based on an algorithm described in
*   C. Loeffler, A. Ligtenberg and G. Moschytz, "Practical Fast 1-D DCT
*   Algorithms with 11 Multiplications", Proc. Int'l. Conf. on Acoustics,
*   Speech, and Signal Processing 1989 (ICASSP '89), pp. 988-991.
* The primary algorithm described there uses 11 multiplies and 29 adds.
* We use their alternate method with 12 multiplies and 32 adds.
* The advantage of this method is that no data path contains more than one
* multiplication; this allows a very simple and accurate implementation in
* scaled fixed-point arithmetic, with a minimal number of shifts.
*/

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

inline my_int8 rmBias(uint8_t x) { return my_int8(x - 128); }

/*
* Perform the forward DCT on one block of samples.
*/

void jpegFdct (uint8_t (&inBlock)[8][8], int16_t (&coefBlock)[8][8])
{
	my_int16 ws[DCTSIZE][DCTSIZE];	/* buffers data between passes */

	/* Pass 1: process rows. */
	/* Note results are scaled up by sqrt(8) compared to a true DCT; */
	/* furthermore, we scale the results by 2**JPEG_ENC_PASS1_BITS. */

	for (uint64_t row = 0; row < DCTSIZE; row += 1) {
		my_int9 a0 = rmBias(inBlock[row][0]) + rmBias(inBlock[row][7]);
		my_int9 a1 = rmBias(inBlock[row][1]) + rmBias(inBlock[row][6]);
		my_int9 a2 = rmBias(inBlock[row][2]) + rmBias(inBlock[row][5]);
		my_int9 a3 = rmBias(inBlock[row][3]) + rmBias(inBlock[row][4]);

		my_int10 b0 = a0 + a3;
		my_int10 b3 = a0 - a3;
		my_int10 b1 = a1 + a2;
		my_int10 b2 = a1 - a2;

		my_int24 c = (b2 + b3) * FIX_0_541196100;

		my_int13 d0 = (b0 + b1) << JPEG_ENC_PASS1_BITS;
		my_int13 d1 = (b0 - b1) << JPEG_ENC_PASS1_BITS;
		my_int25 d2 = c + b3 * FIX_0_765366865;
		my_int25 d3 = c + b2 * - FIX_1_847759065;

		ws[row][0] = (int16_t) d0;
		ws[row][4] = (int16_t) d1;
		ws[row][2] = (int16_t) descale(d2, JPEG_ENC_CONST_BITS-JPEG_ENC_PASS1_BITS);
		ws[row][6] = (int16_t) descale(d3, JPEG_ENC_CONST_BITS-JPEG_ENC_PASS1_BITS);

		my_int9 e0 = rmBias(inBlock[row][3]) - rmBias(inBlock[row][4]);
		my_int9 e1 = rmBias(inBlock[row][2]) - rmBias(inBlock[row][5]);
		my_int9 e2 = rmBias(inBlock[row][1]) - rmBias(inBlock[row][6]);
		my_int9 e3 = rmBias(inBlock[row][0]) - rmBias(inBlock[row][7]);

		my_int10 f4 = e0 + e3;
		my_int10 f5 = e1 + e2;
		my_int10 f6 = e0 + e2;
		my_int10 f7 = e1 + e3;

		my_int25 g = (f6 + f7) * FIX_1_175875602;

		my_int24 h0 = e0 * FIX_0_298631336;
		my_int24 h1 = e1 * FIX_2_053119869;
		my_int24 h2 = e2 * FIX_3_072711026;
		my_int24 h3 = e3 * FIX_1_501321110;

		my_int25 i0 = f4 * - FIX_0_899976223;
		my_int25 i1 = f5 * - FIX_2_562915447;
		my_int25 i2 = g + f6 * - FIX_1_961570560;
		my_int25 i3 = g + f7 * - FIX_0_390180644;

		my_int27 j0 = h0 + i0 + i2;
		my_int27 j1 = h1 + i1 + i3;
		my_int27 j2 = h2 + i1 + i2;
		my_int27 j3 = h3 + i0 + i3;

		ws[row][7] = (int16_t) descale(j0, JPEG_ENC_CONST_BITS-JPEG_ENC_PASS1_BITS);
		ws[row][5] = (int16_t) descale(j1, JPEG_ENC_CONST_BITS-JPEG_ENC_PASS1_BITS);
		ws[row][3] = (int16_t) descale(j2, JPEG_ENC_CONST_BITS-JPEG_ENC_PASS1_BITS);
		ws[row][1] = (int16_t) descale(j3, JPEG_ENC_CONST_BITS-JPEG_ENC_PASS1_BITS);
	}

	/* Pass 2: process columns.
	* We remove the JPEG_ENC_PASS1_BITS scaling, but leave the results scaled up
	* by an overall factor of 8.
	*/

	for (uint64_t col = 0; col < DCTSIZE; col += 1) {
		my_int17 k0 = ws[0][col] + ws[7][col];
		my_int17 k1 = ws[1][col] + ws[6][col];
		my_int17 k2 = ws[2][col] + ws[5][col];
		my_int17 k3 = ws[3][col] + ws[4][col];

		my_int18 m0 = k0 + k3;
		my_int18 m3 = k0 - k3;
		my_int18 m1 = k1 + k2;
		my_int18 m2 = k1 - k2;

		my_int31 n = (m2 + m3) * FIX_0_541196100;

		my_int19 p0 = m0 + m1;
		my_int19 p1 = m0 - m1;
		my_int31 p2 = n + m3 * FIX_0_765366865;
		my_int31 p3 = n + m2 * - FIX_1_847759065;

		coefBlock[0][col] = (int16_t) descale(p0, JPEG_ENC_PASS1_BITS);
		coefBlock[4][col] = (int16_t) descale(p1, JPEG_ENC_PASS1_BITS);
		coefBlock[2][col] = (int16_t) descale(p2, JPEG_ENC_CONST_BITS+JPEG_ENC_PASS1_BITS);
		coefBlock[6][col] = (int16_t) descale(p3, JPEG_ENC_CONST_BITS+JPEG_ENC_PASS1_BITS);

		my_int17 q0 = ws[0][col] - ws[7][col];
		my_int17 q1 = ws[1][col] - ws[6][col];
		my_int17 q2 = ws[2][col] - ws[5][col];
		my_int17 q3 = ws[3][col] - ws[4][col];

		my_int18 r0 = q3 + q0;
		my_int18 r1 = q2 + q1;
		my_int18 r2 = q3 + q1;
		my_int18 r3 = q2 + q0;

		my_int31 s = (r2 + r3) * FIX_1_175875602;

		my_int31 t0 = q3 * FIX_0_298631336;
		my_int31 t1 = q2 * FIX_2_053119869;
		my_int31 t2 = q1 * FIX_3_072711026;
		my_int31 t3 = q0 * FIX_1_501321110;

		my_int31 u0 = r0 * - FIX_0_899976223;
		my_int31 u1 = r1 * - FIX_2_562915447;
		my_int31 u2 = s + r2 * - FIX_1_961570560;
		my_int31 u3 = s + r3 * - FIX_0_390180644;

		my_int31 v0 = t0 + u0 + u2;
		my_int31 v1 = t1 + u1 + u3;
		my_int31 v2 = t2 + u1 + u2;
		my_int31 v3 = t3 + u0 + u3;

		coefBlock[7][col] = (int16_t) descale(v0, JPEG_ENC_CONST_BITS+JPEG_ENC_PASS1_BITS);
		coefBlock[5][col] = (int16_t) descale(v1, JPEG_ENC_CONST_BITS+JPEG_ENC_PASS1_BITS);
		coefBlock[3][col] = (int16_t) descale(v2, JPEG_ENC_CONST_BITS+JPEG_ENC_PASS1_BITS);
		coefBlock[1][col] = (int16_t) descale(v3, JPEG_ENC_CONST_BITS+JPEG_ENC_PASS1_BITS);
	}
}
