/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#pragma once

#include <queue>
using namespace std;

#include "MyInt.h"
#include "JobInfo.h"
#include "VertResizeMsg.h"

void jpegEncode( JobInfo * pJobInfo, queue<VertResizeMsg> &vertResizeMsgQue );

class HuffEnc {
public:
	HuffEnc( JobInfo * pJobInfo, my_uint10 rstIdx )
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

	void setCompId( my_uint2 ci ) {
		m_curCi = ci;
	}
	my_uint2 getCompId() { return m_curCi; }

	void putBits(my_int16 data, my_uint5 size) {
		uint16_t code = data & ((1ull << size) - 1u);	// mask bits

		m_bitBuffer = (uint32_t)((m_bitBuffer << size) | code);
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
	void putNbits(my_uint10 nbits, bool bDc) {

		JobEncDht const * pDht;
		if (bDc)
			pDht = &m_pJobInfo->m_enc.m_dcDht[ m_pJobInfo->m_enc.m_ecp[m_curCi].m_dcDhtId];
		else
			pDht = &m_pJobInfo->m_enc.m_acDht[ m_pJobInfo->m_enc.m_ecp[m_curCi].m_acDhtId];

		my_int16 code = (int16_t)pDht->m_huffCode[nbits];
		my_uint5 size = pDht->m_huffSize[nbits];

		putBits(code, size);
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

	my_int16 & getLastDcValueRef() { return m_lastDcValue[m_curCi]; }

private:
	JobInfo * m_pJobInfo;
	my_uint10 m_rstIdx;
	uint8_t * m_pOutPtr;
	int m_rstLength;

	uint32_t m_bitBuffer;
	my_uint6 m_bitsPresent;

	my_uint2 m_curCi;
	my_int16 m_lastDcValue[4];
};

