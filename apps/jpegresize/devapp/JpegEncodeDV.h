/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#pragma once

#include "JobInfo.h"

void jpegEncodeDV( JobInfo * pJobInfo );

class HuffEncDV {
public:
	HuffEncDV( JobInfo * pJobInfo, uint16_t rstIdx )
		: m_pJobInfo(pJobInfo), m_rstIdx(rstIdx)
	{
		m_pOutPtr = pJobInfo->m_enc.m_pRstBase + rstIdx * pJobInfo->m_enc.m_rstOffset;
		m_curCi = 0;
		m_bitBuffer = 0;
		m_bitsPresent = 0;
		m_rstLength = 0;
		for (int i = 0; i < 4; i+=1)
			m_lastDcValue[i] = 0;
	}

	void setCompId( int ci ) {
		m_curCi = ci;
	}
	int getCompId() { return (int) m_curCi; }

	void putBits(int code, int size) {
		code &= (((int) 1)<<size) - 1;	// mask bits

		m_bitBuffer = (m_bitBuffer << size) | code;
		m_bitsPresent += size;

		emptyBits();
	}
	void emptyBits() {
		while (m_bitsPresent >= 8) {
			uint8_t byte = (m_bitBuffer >> (m_bitsPresent - 8)) & 0xff;
			*m_pOutPtr++ = byte;
			m_bitsPresent -= 8;
			m_rstLength += 1;

			if (byte == 0xff) {
				*m_pOutPtr++ = 0;
				m_rstLength += 1;
			}
		}
	}
	void putNbits(int nbits, bool bDc) {

		JobEncDht const * pDht;
		if (bDc)
			pDht = &m_pJobInfo->m_enc.m_dcDht[ m_pJobInfo->m_enc.m_ecp[m_curCi].m_dcDhtId];
		else
			pDht = &m_pJobInfo->m_enc.m_acDht[ m_pJobInfo->m_enc.m_ecp[m_curCi].m_acDhtId];

		int code = pDht->m_huffCode[nbits];
		int size = pDht->m_huffSize[nbits];

		putBits(code, size);
	}
	int16_t calcNbits(int tmp) {
		int16_t nbits = 0;
		while (tmp) {
			nbits++;
			tmp >>= 1;
		}
		return nbits;
	}
	void flushBits() {
		emptyBits();
		if (m_bitsPresent > 0) {
			m_bitBuffer <<= 8 - m_bitsPresent;
			m_bitsPresent = 8;
		}
		emptyBits();
		m_pJobInfo->m_enc.m_rstLength[m_rstIdx] = m_rstLength;
	}

	int16_t & getLastDcValueRef() { return m_lastDcValue[m_curCi]; }

private:
	JobInfo * m_pJobInfo;
	uint16_t m_rstIdx;
	uint8_t * m_pOutPtr;
	int m_rstLength;

	uint32_t m_bitBuffer;
	uint8_t m_bitsPresent;

	uint64_t m_curCi : 2;
	int16_t m_lastDcValue[4];
};

