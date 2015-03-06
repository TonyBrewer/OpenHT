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

#include "JpegCommon.h"
#include "JobInfo.h"

void jpegDecodeDV( JobInfo * pJobInfo );

class LenStats {
public:
	LenStats() {
		for (int i = 0; i < 64; i += 1)
			m_stats[i] = 0;
	}

	~LenStats() {
		int total = 0;
		for (int i = 0; i < 64; i += 1)
			total += m_stats[i];
		int accum = 0;
		for (int i = 0; i < 64; i += 1)
			if (m_stats[i] > 0) {
				printf("m_stats[%d] = %d (%4.1f%%)\n", i, m_stats[i], (m_stats[i]+accum)*100.0/total);
				accum += m_stats[i];
			}
	}		

	void count(int nb, int s) {
		assert(nb + s < 64);
		m_stats[nb + s] += 1;
	}

private:
	int m_stats[64];
};
//extern LenStats g_lenStats;

class HuffDecDV {
public:
	HuffDecDV( JobInfo const * pJobInfo, uint16_t rstIdx )
		: m_pJobInfo(pJobInfo)
	{
		m_pInPtr = pJobInfo->m_dec.m_pRstBase + pJobInfo->m_dec.m_rstInfo[rstIdx].m_offset;
		m_curCi = 0;
		m_bitsLeft = 0;
		m_bitsZero = 0;
		for (int i = 0; i < 4; i+=1)
			m_lastDcValue[i] = 0;
	}

	void setCompId( int ci ) {
		m_curCi = ci;
	}
	int getCompId() { return (int) m_curCi; }

	// get next huffman decoded value
	int getNext(bool bDc) {
		JobDht const * pDht;
		if (bDc)
			pDht = &m_pJobInfo->m_dec.m_dcDht[ m_pJobInfo->m_dec.m_dcp[m_curCi].m_dcDhtId];
		else
			pDht = &m_pJobInfo->m_dec.m_acDht[ m_pJobInfo->m_dec.m_dcp[m_curCi].m_acDhtId];

		fillBits();
		int t = peekBits(HUFF_LOOKAHEAD);
		t = pDht->m_dhtTbls[t].m_lookup;
		int nb = (t >> HUFF_LOOKAHEAD) & 0xf;

		int s = t & ((1 << HUFF_LOOKAHEAD) - 1);
		if (nb > HUFF_LOOKAHEAD) {
			assert(nb == HUFF_LOOKAHEAD+1);
			t = getBits(HUFF_LOOKAHEAD+1);

			//g_lenStats.count(62, 0);

			while (t > pDht->m_dhtTbls[nb].m_maxCode) {
				t <<= 1;
				t |= getBits(1);
				nb++;
				//g_lenStats.count(63, 0);
			}
			s = pDht->m_dhtTbls[ (int) (t + pDht->m_dhtTbls[nb].m_valOffset) & 0xFF ].m_huffCode;
		} else {
			dropBits(nb);

			//g_lenStats.count(nb, s & 0xf);
		}

		return s;
	}

	int huffExtend(int r, int s)
	{
		return r + (((r - (1<<(s-1))) >> 31) & (((-1)<<s) + 1));
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
	void dropBits(int nb)
	{
		assert(m_bitsLeft - m_bitsZero >= nb);
		m_bitsLeft -= nb;
	}
	int getBits(int nb)
	{
		assert(m_bitsLeft - m_bitsZero >= nb);
		int bits = (m_bitBuffer >> (m_bitsLeft -  (nb))) & ((1<<nb)-1);
		m_bitsLeft -= nb;
		return bits;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       
	}
	int16_t & getLastDcValueRef() { return m_lastDcValue[m_curCi]; }

	void checkEndOfScan() {
		assert( m_pInPtr[0] == 0xFF && (m_pInPtr[1] == 0xD9 || (m_pInPtr[1] >= 0xD0 && m_pInPtr[1] <= 0xD7)));
	}

	void nextRestart() {
	  if (m_pInPtr[0] != 0xff)
	    m_pInPtr++;
	  checkEndOfScan();
	  m_pInPtr += 2;
	  m_curCi = 0;
	  m_bitsLeft = 0;
	  m_bitsZero = 0;
	  for (int i = 0; i < 4; i+=1)
	    m_lastDcValue[i] = 0;
	}

private:
	JobInfo const * m_pJobInfo;
	uint8_t * m_pInPtr;

	uint32_t m_bitBuffer;
	uint8_t m_bitsLeft;
	uint8_t m_bitsZero;

	uint64_t m_curCi : 2;
	int16_t m_lastDcValue[4];
};
