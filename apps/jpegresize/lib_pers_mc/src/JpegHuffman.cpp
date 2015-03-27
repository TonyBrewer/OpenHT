/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include <stdint.h>
#include <assert.h>
#include <string.h>

#include "JobInfo.h"
#include "JpegHuffman.h"

uint8_t s_dc_lum_bits[17] = { 0,0,1,5,1,1,1,1,1,1,0,0,0,0,0,0,0 };
uint8_t s_dc_lum_val[DC_LUM_CODES] = { 0,1,2,3,4,5,6,7,8,9,10,11 };
uint8_t s_ac_lum_bits[17] = { 0,0,2,1,3,3,2,4,3,5,5,4,4,0,0,1,0x7d };
uint8_t s_ac_lum_val[AC_LUM_CODES]  =
{
  0x01,0x02,0x03,0x00,0x04,0x11,0x05,0x12,0x21,0x31,0x41,0x06,0x13,0x51,0x61,0x07,0x22,0x71,0x14,0x32,0x81,0x91,0xa1,0x08,0x23,0x42,0xb1,0xc1,0x15,0x52,0xd1,0xf0,
  0x24,0x33,0x62,0x72,0x82,0x09,0x0a,0x16,0x17,0x18,0x19,0x1a,0x25,0x26,0x27,0x28,0x29,0x2a,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x43,0x44,0x45,0x46,0x47,0x48,0x49,
  0x4a,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x83,0x84,0x85,0x86,0x87,0x88,0x89,
  0x8a,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xc2,0xc3,0xc4,0xc5,
  0xc6,0xc7,0xc8,0xc9,0xca,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,
  0xf9,0xfa
};
uint8_t s_dc_chroma_bits[17] = { 0,0,3,1,1,1,1,1,1,1,1,1,0,0,0,0,0 };
uint8_t s_dc_chroma_val[DC_CHROMA_CODES]  = { 0,1,2,3,4,5,6,7,8,9,10,11 };
uint8_t s_ac_chroma_bits[17] = { 0,0,2,1,2,4,4,3,4,7,5,4,4,0,1,2,0x77 };
uint8_t s_ac_chroma_val[AC_CHROMA_CODES] =
{
  0x00,0x01,0x02,0x03,0x11,0x04,0x05,0x21,0x31,0x06,0x12,0x41,0x51,0x07,0x61,0x71,0x13,0x22,0x32,0x81,0x08,0x14,0x42,0x91,0xa1,0xb1,0xc1,0x09,0x23,0x33,0x52,0xf0,
  0x15,0x62,0x72,0xd1,0x0a,0x16,0x24,0x34,0xe1,0x25,0xf1,0x17,0x18,0x19,0x1a,0x26,0x27,0x28,0x29,0x2a,0x35,0x36,0x37,0x38,0x39,0x3a,0x43,0x44,0x45,0x46,0x47,0x48,
  0x49,0x4a,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x82,0x83,0x84,0x85,0x86,0x87,
  0x88,0x89,0x8a,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xc2,0xc3,
  0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,
  0xf9,0xfa
};

uint32_t m_huff_codes[4][256];
uint8_t m_huff_code_sizes[4][256];
uint8_t m_huff_bits[4][17];
uint8_t m_huff_val[4][256];

void compute_huffman_table(uint16_t *codes, uint8_t *code_sizes, uint8_t *bits, uint8_t *val)
{
	int i, l, last_p, si;
	uint8_t huff_size[257];
	uint32_t huff_code[257];
	uint32_t code;

	int p = 0;
	for (l = 1; l <= 16; l++)
		for (i = 1; i <= bits[l]; i++)
			huff_size[p++] = (char)l;

	huff_size[p] = 0; last_p = p; // write sentinel

	code = 0; si = huff_size[0]; p = 0;

	while (huff_size[p])
	{
		while (huff_size[p] == si)
			huff_code[p++] = code++;
		code <<= 1;
		si++;
	}

	memset(codes, 0, sizeof(codes[0])*256);
	memset(code_sizes, 0, sizeof(code_sizes[0])*256);
	for (p = 0; p < last_p; p++)
	{
		codes[val[p]]      = huff_code[p];
		code_sizes[val[p]] = huff_size[p];
	}
}

void compute_huffman_table(uint16_t *codes, uint8_t *code_sizes, JobDht & jobDht )
{
	int i, l, last_p, si;
	uint8_t huff_size[257];
	uint32_t huff_code[257];
	uint32_t code;

	int p = 0;
	for (l = 1; l <= 16; l++) {
		for (i = 1; i <= jobDht.m_bits[l-1]; i++) {
			huff_size[p++] = (char)l;
		}
	}

	huff_size[p] = 0; last_p = p; // write sentinel

	code = 0; si = huff_size[0]; p = 0;

	while (huff_size[p])
	{
		while (huff_size[p] == si)
			huff_code[p++] = code++;
		code <<= 1;
		si++;
	}

	memset(codes, 0, sizeof(codes[0])*256);
	memset(code_sizes, 0, sizeof(code_sizes[0])*256);
	for (p = 0; p < last_p; p++)
	{
		codes[jobDht.m_dhtTbls[p].m_huffCode]      = huff_code[p];
		code_sizes[jobDht.m_dhtTbls[p].m_huffCode] = huff_size[p];
	}
}


void calcDhtDerivedTbl( JobDht & dht )
{
	int8_t huffSize[257];
	uint32_t huffCode[257];
	uint32_t code;
	int32_t lookBits;

	if (!dht.m_bUsed)
		return;

	// Figure C.1
	int p = 0;
	for (uint8_t l = 1; l <= 16; l += 1) {
		int i = dht.m_bits[l-1];
		while (i--)
			huffSize[p++] = l;
	}
	huffSize[p] = 0;

	// Figure C.2
	code = 0;
	uint8_t si = huffSize[0];
	p = 0;
	while (huffSize[p]) {
		while (huffSize[p] == si)
			huffCode[p++] = code++;

		code <<= 1;
		si += 1;
	}

	// Figure F.15
	p = 0;
	for (int l = 1; l <= 16; l++) {
		if (dht.m_bits[l-1]) {
			dht.m_dhtTbls[l].m_valOffset = (int32_t) p - (int32_t) huffCode[p];
			p += dht.m_bits[l-1];
			dht.m_dhtTbls[l].m_maxCode = huffCode[p-1]; /* maximum code of length l */
		} else {
			dht.m_dhtTbls[l].m_valOffset = 0;
			dht.m_dhtTbls[l].m_maxCode = -1;	/* -1 if no codes of this length */
		}
	}
	dht.m_dhtTbls[17].m_valOffset = 0;
	dht.m_dhtTbls[17].m_maxCode = 0xFFFFFL; /* ensures jpeg_huff_decode terminates */

	/* Compute lookahead tables to speed up decoding.
	* First we set all the table entries to 0, indicating "too long";
	* then we iterate through the Huffman codes that are short enough and
	* fill in all the entries that correspond to bit sequences starting
	* with that code.
	*/

	for (int i = 0; i < (1 << HUFF_LOOKAHEAD); i++)
		dht.m_dhtTbls[i].m_lookup = (HUFF_LOOKAHEAD + 1) << HUFF_LOOKAHEAD;

	p = 0;
	for (int l = 1; l <= HUFF_LOOKAHEAD; l++) {
		for (int i = 1; i <= (int) dht.m_bits[l-1]; i++, p++) {
			/* l = current code's length, p = its index in huffcode[] & huffval[]. */
			/* Generate left-justified code followed by all possible bit sequences */
			assert(p<257);
			lookBits = huffCode[p] << (HUFF_LOOKAHEAD-l);
			for (int ctr = 1 << (HUFF_LOOKAHEAD-l); ctr > 0; ctr--) {
				dht.m_dhtTbls[lookBits].m_lookup = (l << HUFF_LOOKAHEAD) | dht.m_dhtTbls[p].m_huffCode;
				lookBits++;
			}
		}
	}

	for (int i = 0; i < (1 << HUFF_LOOKAHEAD); i++) {
		uint32_t lookup = dht.m_dhtTbls[i].m_lookup;
		assert(lookup < 0x1000);
		uint32_t valueBits = lookup & 0xf;
		uint32_t huffBits = (lookup >> 8) & 0xf;
		uint32_t peekBits = valueBits + huffBits;
		bool bFastDec = peekBits <= 8;
		bool bValueZero = valueBits == 0;
		dht.m_dhtTbls[i].m_lookup |= (bValueZero ? 0x20000 : 0) | (bFastDec ? 0x10000 : 0) | ((peekBits & 0xf) << 12);
	}
}


void jpegEncodeGenDht( JobDht & dht, int32_t (&huffCnts)[257] )
{
#define MAX_CLEN 32		/* assumed maximum initial code length */
	uint8_t huffSize[256];
	uint8_t bits[MAX_CLEN+1];	/* bits[k] = # of symbols with code length k */
	int others[257];		/* next symbol in current branch of tree */
	int c1, c2;
	int p, i, j;
	long v;

	memset(bits, 0, sizeof(bits));
	memset(huffSize, 0, sizeof(huffSize));
	memset(others, 0xff, sizeof(others));
  
	huffCnts[256] = 1;		/* make sure 256 has a nonzero count */
	/* Including the pseudo-symbol 256 in the Huffman procedure guarantees
	* that no real symbol is given code-value of all ones, because 256
	* will be placed last in the largest codeword category.
	*/

	/* Huffman's basic algorithm to assign optimal code lengths to symbols */

	for (;;) {
		/* Find the smallest nonzero frequency, set c1 = its symbol */
		/* In case of ties, take the larger symbol number */
		c1 = -1;
		v = 1000000000L;
		for (i = 0; i <= 256; i++) {
			if (huffCnts[i] && huffCnts[i] <= v) {
				v = huffCnts[i];
				c1 = i;
			}
		}

		/* Find the next smallest nonzero frequency, set c2 = its symbol */
		/* In case of ties, take the larger symbol number */
		c2 = -1;
		v = 1000000000L;
		for (i = 0; i <= 256; i++) {
			if (huffCnts[i] && huffCnts[i] <= v && i != c1) {
				v = huffCnts[i];
				c2 = i;
			}
		}

		/* Done if we've merged everything into one frequency */
		if (c2 < 0)
			break;

		/* Else merge the two counts/trees */
		huffCnts[c1] += huffCnts[c2];
		huffCnts[c2] = 0;

		/* Increment the codeSize of everything in c1's tree branch */
		huffSize[c1]++;
		while (others[c1] >= 0) {
			c1 = others[c1];
			huffSize[c1]++;
		}

		others[c1] = c2;		/* chain c2 onto c1's tree branch */

		/* Increment the codeSize of everything in c2's tree branch */
		huffSize[c2]++;
		while (others[c2] >= 0) {
			c2 = others[c2];
			huffSize[c2]++;
		}
	}

	/* Now count the number of symbols of each code length */
	for (i = 0; i <= 256; i++) {
		if (huffSize[i]) {
			/* The JPEG standard seems to think that this can't happen, */
			/* but I'm paranoid... */
			assert (huffSize[i] <= MAX_CLEN);

			bits[huffSize[i]]++;
		}
	}

	/* JPEG doesn't allow symbols with code lengths over 16 bits, so if the pure
	* Huffman procedure assigned any such lengths, we must adjust the coding.
	* Here is what the JPEG spec says about how this next bit works:
	* Since symbols are paired for the longest Huffman code, the symbols are
	* removed from this length category two at a time.  The prefix for the pair
	* (which is one bit shorter) is allocated to one of the pair; then,
	* skipping the BITS entry for that prefix length, a code word from the next
	* shortest nonzero BITS entry is converted into a prefix for two code words
	* one bit longer.
	*/

	for (i = MAX_CLEN; i > 16; i--) {
		while (bits[i] > 0) {
			j = i - 2;		/* find length of new prefix to be used */
			while (bits[j] == 0)
				j--;

			bits[i] -= 2;		/* remove two symbols */
			bits[i-1]++;		/* one goes in this length */
			bits[j+1] += 2;		/* two new symbols in this length */
			bits[j]--;		/* symbol of this length is now a prefix */
		}
	}

	/* Remove the count for the pseudo-symbol 256 from the largest codelength */
	while (bits[i] == 0)		/* find largest codelength still in use */
		i--;
	bits[i]--;

	/* Return final symbol counts (only for lengths 0..16) */
	memcpy(dht.m_bits, bits, sizeof(dht.m_bits));

	/* Return a list of the symbols sorted by code length */
	/* It's not real clear to me why we don't need to consider the codelength
	* changes made above, but the JPEG spec seems to think this works.
	*/
	p = 0;
	for (i = 1; i <= MAX_CLEN; i++) {
		for (j = 0; j <= 255; j++) {
			if (huffSize[j] == i) {
				dht.m_dhtTbls[p].m_huffCode = (uint8_t) j;
				p++;
			}
		}
	}
}
