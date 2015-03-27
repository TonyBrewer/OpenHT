/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
inline ht_uint3 zigZagRow(ht_uint6 k) {
        ht_uint3 row[64] = {
		0, 0, 1, 2, 1, 0, 0, 1,
		2, 3, 4, 3, 2, 1, 0, 0,
		1, 2, 3, 4, 5, 6, 5, 4,
		3, 2, 1, 0, 0, 1, 2, 3,
		4, 5, 6, 7, 7, 6, 5, 4,
		3, 2, 1, 2, 3, 4, 5, 6,
		7, 7, 6, 5, 4, 3, 4, 5,
		6, 7, 7, 6, 5, 6, 7, 7
	};

	return row[k];
}

inline ht_uint3 zigZagCol(ht_uint6 k) {
        ht_uint3 col[64] = {
		0, 1, 0, 0, 1, 2, 3, 2,
		1, 0, 0, 1, 2, 3, 4, 5,
		4, 3, 2, 1, 0, 0, 1, 2,
		3, 4, 5, 6, 7, 6, 5, 4,
		3, 2, 1, 0, 1, 2, 3, 4,
		5, 6, 7, 7, 6, 5, 4, 3,
		2, 3, 4, 5, 6, 7, 7, 6,
		5, 4, 5, 6, 7, 7, 6, 7
	};

	return col[k];
}
