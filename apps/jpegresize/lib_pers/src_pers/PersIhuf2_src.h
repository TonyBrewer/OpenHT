/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
//static ht_int16 huffExtend(ht_int16 r, ht_uint4 s)
//{
//	return r + (((r - (1<<(s-1))) >> 31) & (((-1)<<s) + 1));
//}

static ht_int16 huffExtend(ht_int16 r, ht_uint4 s)
{
	bool bRIsNeg = (r & (1 << (s-1))) != 0;
	ht_int16 mask = (s == 0) ? 0 : ((0xffff << s) | 1);
	ht_int16 rslt = r + (bRIsNeg ? (ht_int16)0 : mask);
	//ht_int16 orig = r + (((r - (1<<(s-1))) >> 31) & (((-1)<<s) + 1));
	//assert(orig == rslt);
	return rslt;
}

static ht_int16 huffExtendOp(ht_int16 r, ht_uint4 s)
{
	bool bRIsNeg = (r & (1 << (s-1))) != 0;
	ht_int16 mask = (s == 0) ? 0 : ((0xffff << s) | 1);
	return bRIsNeg ? (ht_int16)0 : mask;
}

static ht_uint8 peekBits(ht_uint23 bits, ht_uint4 nb)
{
	assert(nb >= 0 && nb <= 8);
	ht_uint8 staging = bits >> 15;
	return staging >> ((8 - nb) & 7);
}
