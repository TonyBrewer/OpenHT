/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "Htv.h"
#include "HtDesign.h"

namespace ht {
	void HtCntxMap::addDecl(HtDecl * pHtDecl) {
		HtCntxMap_insertPair insertPair;
		insertPair = m_declMap.insert(HtCntxMap_valuePair( pHtDecl->getName(), (HtDecl *)0 ));
		//HtAssert(insertPair.first->second == 0);	// should always be unique
		if (insertPair.first->second == 0)
			insertPair.first->second = pHtDecl;
	}

	void DeclRef::incRdRefCnt(vector<int> * pDimenList, uint16_t bitHigh, uint16_t bitLow)
	{
		vector<int> emptyList;
		if (pDimenList == 0)
			pDimenList = & emptyList;

		for (size_t i = 0; i < m_dimenRefList.size(); i += 1) {
			DimenRef & dimenRef = m_dimenRefList[i];

			// first see if dimenList matches
			assert(pDimenList->size() == dimenRef.m_dimenList.size());
			bool bFound = true;
			for (size_t j = 0; j < dimenRef.m_dimenList.size(); j += 1) {
				if (dimenRef.m_dimenList[j] != (*pDimenList)[j]) {
					bFound = false;
					break;
				}
			}
			if (!bFound) continue;

			// found matching dimenRef, now check for bit range match
			for (size_t j = 0; j < dimenRef.m_rangeRefList.size(); j += 1) {
				RangeRef & rangeRef = dimenRef.m_rangeRefList[j];

				if (rangeRef.m_bitHigh == bitHigh && rangeRef.m_bitLow == bitLow) {
					rangeRef.m_rdRefCnt += 1;
					return;
				}
			}

			// range not found, add a new range
			dimenRef.m_rangeRefList.push_back(RangeRef());
			RangeRef & rangeRef = dimenRef.m_rangeRefList.back();
			rangeRef.m_rdRefCnt += 1;
			return;
		}

		// dimenRef not found, add one
		m_dimenRefList.push_back(DimenRef());
		DimenRef & dimenRef = m_dimenRefList.back();

		// now add first range to dimenRef
		dimenRef.m_dimenList = *pDimenList;
		dimenRef.m_rangeRefList.push_back(RangeRef());
		RangeRef & rangeRef = dimenRef.m_rangeRefList.back();
		rangeRef.m_rdRefCnt += 1;
	}

	void DeclRef::incWrRefCnt(vector<int> * pDimenList, uint16_t bitHigh, uint16_t bitLow)
	{
		vector<int> emptyList;
		if (pDimenList == 0)
			pDimenList = & emptyList;

		for (size_t i = 0; i < m_dimenRefList.size(); i += 1) {
			DimenRef & dimenRef = m_dimenRefList[i];

			// first see if dimenList matches
			assert(pDimenList->size() == dimenRef.m_dimenList.size());
			bool bFound = true;
			for (size_t j = 0; j < dimenRef.m_dimenList.size(); j += 1) {
				if (dimenRef.m_dimenList[j] != (*pDimenList)[j]) {
					bFound = false;
					break;
				}
			}
			if (!bFound) continue;

			// found matching dimenRef, now check for bit range match
			for (size_t j = 0; j < dimenRef.m_rangeRefList.size(); j += 1) {
				RangeRef & rangeRef = dimenRef.m_rangeRefList[j];

				if (rangeRef.m_bitHigh == bitHigh && rangeRef.m_bitLow == bitLow) {
					rangeRef.m_wrRefCnt += 1;
					return;
				}
			}

			// range not found, add a new range
			dimenRef.m_rangeRefList.push_back(RangeRef());
			RangeRef & rangeRef = dimenRef.m_rangeRefList.back();
			rangeRef.m_wrRefCnt += 1;
			return;
		}

		// dimenRef not found, add one
		m_dimenRefList.push_back(DimenRef());
		DimenRef & dimenRef = m_dimenRefList.back();

		// now add first range to dimenRef
		dimenRef.m_dimenList = *pDimenList;
		dimenRef.m_rangeRefList.push_back(RangeRef());
		RangeRef & rangeRef = dimenRef.m_rangeRefList.back();
		rangeRef.m_wrRefCnt += 1;
	}

}

