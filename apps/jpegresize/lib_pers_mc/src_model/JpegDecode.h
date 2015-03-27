/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#pragma once

#include <stdint.h>
#include <assert.h>

#include <queue>
using namespace std;

#include "JpegCommon.h"
#include "JobInfo.h"
#include "MyInt.h"
#include "JpegDecMsg.h"

void jpegDecode( JobInfo * pJobInfo, queue<JpegDecMsg> &jpegDecMsgQue );

extern int cycles;

class HuffDec {
public:
	HuffDec( JobInfo const * pJobInfo, uint16_t rstIdx )
		: m_pJobInfo(pJobInfo)
	{
		m_pInPtr = pJobInfo->m_dec.m_pRstBase + pJobInfo->m_dec.m_rstInfo[rstIdx].m_offset;
		m_curCi = 0;
		m_bitsLeft = 0;
		m_bitsZero = 0;
		for (int i = 0; i < 4; i+=1)
			m_lastDcValue[i] = 0;
	}

	void setCompId( my_uint2 ci ) {
		m_curCi = ci;
	}
	int getCompId() { return (int) m_curCi; }

	// get next huffman decoded value
	my_uint8 getNext(bool bDc) {
		JobDht const * pDht;
		if (bDc)
			pDht = &m_pJobInfo->m_dec.m_dcDht[ m_pJobInfo->m_dec.m_dcp[m_curCi].m_dcDhtId];
		else
			pDht = &m_pJobInfo->m_dec.m_acDht[ m_pJobInfo->m_dec.m_dcp[m_curCi].m_acDhtId];

		fillBits();

		// first clock of decode (usually only clock)
		cycles += 1;
		my_uint8 t1 = peekBits(HUFF_LOOKAHEAD);
		my_uint12 t2 = pDht->m_dhtTbls[t1].m_lookup;
		my_uint5 nb = (t2 >> HUFF_LOOKAHEAD) & 0xf;
		my_uint8 s1 = t2 & ((1 << HUFF_LOOKAHEAD) - 1);

		my_uint8 s2 = 0;
		if (nb > HUFF_LOOKAHEAD) {
			assert(nb == HUFF_LOOKAHEAD+1);
			my_int18 t3 = (uint64_t)getBits(HUFF_LOOKAHEAD+1);

			// a clock per iteration
			while (t3 > (my_int21)pDht->m_dhtTbls[nb].m_maxCode) {
				cycles += 1;	// for performance monitoring (not in HW)
				t3 = (t3 << 1) | getBits(1);
				nb = nb + 1;
			}

			// last clock
			cycles += 1;
			my_uint8 codeIdx = (my_uint8)(t3 + pDht->m_dhtTbls[nb].m_valOffset);
			s2 = pDht->m_dhtTbls[ codeIdx ].m_huffCode;
		} else
			// first clock when needed
			dropBits(nb);

		return nb > HUFF_LOOKAHEAD ? s2 : s1;
	}

	my_int16 huffExtendDc(my_uint11 r, my_uint5 s)
	{
		bool rIsNeg = (r & (1Ull << (s-1))) != 0;
		my_int16 t = rIsNeg ? 0 : ((-1 << s) + 1);
		return (int64_t)(r + t);
	}

	my_int16 huffExtendAc(my_uint10 r, my_uint4 s)
	{
		bool rIsNeg = (r & (1Ull << (s-1))) != 0;
		my_int16 t = rIsNeg ? 0 : ((-1 << s) + 1);
		return (int64_t)(r + t);
	}

	void fillBits()
	{
		while (m_bitsLeft < 16) {
			bool bSkipZero = false;
			if (m_pInPtr[0] == 0xFF) {
				if (m_pInPtr[1] != 0x00) {
					// fill with zero
					m_bitBuffer <<= 8;
					m_bitsLeft += 8;
					m_bitsZero += 8;
					continue;
				}
				bSkipZero = true;
			}
			m_bitBuffer = (m_bitBuffer << 8) | *m_pInPtr++;
			if (bSkipZero)
				m_pInPtr++;
			m_bitsLeft += 8;
		}
	}
	int peekBits(int nb)
	{
		return (m_bitBuffer >> (m_bitsLeft -  (nb))) & ((1<<nb)-1);
	}
	void dropBits(my_uint5 nb)
	{
		assert(nb <= 8);
		assert(m_bitsLeft - m_bitsZero >= nb);
		m_bitsLeft = m_bitsLeft - nb;
	}
	my_uint16 getBits(my_uint5 nb)
	{
		assert(nb <= 16);
		assert(m_bitsLeft - m_bitsZero >= nb);
		int bits = (m_bitBuffer >> (m_bitsLeft -  (nb))) & ((1<<nb)-1);
		m_bitsLeft = m_bitsLeft - nb;
		return bits;
	}
	int16_t & getLastDcValueRef() { return m_lastDcValue[m_curCi]; }

	void checkEndOfScan() {
		assert( m_pInPtr[0] == 0xFF && (m_pInPtr[1] == 0xD9 || (m_pInPtr[1] >= 0xD0 && m_pInPtr[1] <= 0xD7)));
	};

private:
	JobInfo const * m_pJobInfo;
	uint8_t * m_pInPtr;

	uint32_t m_bitBuffer;
	my_uint6 m_bitsLeft;	// allows a 32 bit buffer
	uint8_t m_bitsZero;

	uint64_t m_curCi : 2;
	int16_t m_lastDcValue[4];
};
