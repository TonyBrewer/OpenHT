/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#pragma once

#include <stdint.h>
#include "JobInfo.h"

class JpegHuffDec {
public:
	JpegHuffDec( JobInfo const * pJobInfo, uint8_t * pInPtr, uint8_t * pRstPtr )
		: m_pJobInfo(pJobInfo)
	{
		m_pInPtr = pInPtr;
		m_pStartPtr = pInPtr;
		m_pOutPtr = pRstPtr;
		m_outBits = 0;
		m_curCi = 0;
		m_inBits = 0;
		m_inBitsZero = 0;
		for (int i = 0; i < 4; i+=1)
			m_lastDcValue[i] = 0;
	}

	void setStartPtr (uint8_t * pInPtr) {
		m_pInPtr = pInPtr;
		m_pStartPtr = m_pInPtr;
		//m_outBits = 0;
		//m_curCi = 0;
		m_inBits = 0;
		m_inBitsZero = 0;
		//for (int i = 0; i < 4; i+=1)
		//	m_lastDcValue[i] = 0;
	}

	int getBitPos() {
		return ((int)(m_pInPtr - m_pStartPtr) << 3) - m_inBits;
	}
	uint8_t * getInPtr() {
		return m_pInPtr;
	}
	uint8_t * getOutPtr() {
		return m_pOutPtr;
	}

	void setCompId( int ci ) {
		m_curCi = ci;
	}
	int getCompId() { return (int) m_curCi; }

	// get next huffman decoded value
	int getNext(bool bDc, bool bCopy) {
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
			t = getBits(HUFF_LOOKAHEAD+1, bCopy);

			while (t > pDht->m_dhtTbls[nb].m_maxCode) {
				t <<= 1;
				t |= getBits(1, bCopy);
				nb++;
			}
			s = pDht->m_dhtTbls[ (int) (t + pDht->m_dhtTbls[nb].m_valOffset) & 0xFF ].m_huffCode;
		} else {
			getBits(nb, bCopy);
		}

		return s;
	}

	int huffExtend(int r, int s)
	{
		return r + (((r - (1<<(s-1))) >> 31) & (((-1)<<s) + 1));
	}

	void fillBits()
	{
		while (m_inBits < 16) {
			bool bSkipZero = false;
			if (m_pInPtr[0] == 0xFF) {
				if (m_pInPtr[1] != 0x00) {
					// fill with zero
					m_inBuffer <<= 8;
					m_inBits += 8;
					m_inBitsZero += 8;
					continue;
				}
				bSkipZero = true;
			}
			m_inBuffer = (m_inBuffer << 8) | *m_pInPtr++;
			if (bSkipZero)
				m_pInPtr++;
			m_inBits += 8;
		}
	}
	int peekBits(int nb)
	{
		return (m_inBuffer >> (m_inBits -  (nb))) & ((1<<nb)-1);
	}
	//void dropBits(int nb)
	//{
	//	assert(m_inBits - m_inBitsZero >= nb);
	//	m_inBits -= nb;
	//}
	int getBits(int nb, bool bCopy)
	{
		assert(m_inBits - m_inBitsZero >= nb);
		int bits = (m_inBuffer >> (m_inBits -  (nb))) & ((1<<nb)-1);
		m_inBits -= nb;

		if (bCopy)
			putBits(bits, nb);

		return bits;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       
	}
	int16_t & getLastDcValueRef() { return m_lastDcValue[m_curCi]; }

	void checkEndOfScan() {
		assert( m_pInPtr[0] == 0xFF && (m_pInPtr[1] == 0xD9 || (m_pInPtr[1] >= 0xD0 && m_pInPtr[1] <= 0xD7)));
	};

	// encoding routines for first DC values in MCU row
	void putNbits(int nbits, bool bDc) {

		JobEncDht const * pDht;
		if (bDc)
			pDht = &m_pJobInfo->m_enc.m_dcDht[ m_pJobInfo->m_dec.m_dcp[m_curCi].m_dcDhtId];
		else
			pDht = &m_pJobInfo->m_enc.m_acDht[ m_pJobInfo->m_dec.m_dcp[m_curCi].m_acDhtId];

		int code = pDht->m_huffCode[nbits];
		int size = pDht->m_huffSize[nbits];

		putBits(code, size);
	}
	void putBits(int code, int size) {
		code &= (((int) 1)<<size) - 1;	// mask bits

		m_outBuffer = (m_outBuffer << size) | code;
		m_outBits += size;

		emptyBits();
	}
	void emptyBits() {
		while (m_outBits >= 8) {
			uint8_t byte = (m_outBuffer >> (m_outBits - 8)) & 0xff;
			*m_pOutPtr++ = byte;
			m_outBits -= 8;

			if (byte == 0xff) {
				*m_pOutPtr++ = 0;
			}

			//if (m_pOutPtr == (uint8_t*)0x4000026)
			//	bool stop = true;
		}
	}
	uint8_t * addRstMarker() {
		if (m_outBits!=0) {
			int fillBits = 8 - (m_outBits & 7);
			putBits(0, fillBits);
		}
		*m_pOutPtr++ = 0xFF;
		*m_pOutPtr++ = 0xD0;
		return m_pOutPtr;
	}

private:
	JobInfo const * m_pJobInfo;
	uint8_t * m_pStartPtr;

	uint8_t * m_pInPtr;
	uint32_t m_inBuffer;
	uint8_t m_inBits;
	uint8_t m_inBitsZero;

	uint8_t * m_pOutPtr;
	uint8_t m_outBits;
	uint32_t m_outBuffer;

	uint64_t m_curCi : 2;
	int16_t m_lastDcValue[4];
};

void jpegDecodeRst( JobInfo * pJobInfo, JpegHuffDec &huffDec );
void jpegDecodeChangeRst( JobInfo * pJobInfo, JpegHuffDec &huffDec,
		int &mcuRow, int &mcuCol  );
