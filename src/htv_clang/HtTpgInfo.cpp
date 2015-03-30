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

#define RS_DELTA_UNKNOWN 0x7fffffff

	size_t HtTpg::addTpDecl(string &tpSgName, HtDecl * pDecl) 
	{
		m_tpDeclList.push_back(pDecl);

		size_t i;
		for (i = 0; i < m_sgNameList.size(); i += 1)
			if (m_sgNameList[i] == tpSgName)
				return i;
		m_sgNameList.push_back(tpSgName);
		return i;
	}

	bool HtTpg::areAllTpeReached()
	{
		for (size_t i = 0; i < m_tpDeclList.size(); i += 1)
			if (!m_tpDeclList[i]->getTpInfo()->m_isTps && m_tpDeclList[i]->getTpSgUrs() == URS_UNKNOWN)
				return false;
		return true;
	}

	void HtTpg::initUrsInfo()
	{
		assert(m_pSgUrsMatrix == 0);
		size_t size = m_sgNameList.size() * m_sgNameList.size();
		m_pSgUrsMatrix = new int [size];
		for (size_t i = 0; i < size; i += 1)
			m_pSgUrsMatrix[i] = RS_DELTA_UNKNOWN;

		for (size_t i = 0; i < m_sgNameList.size(); i += 1)
			m_pSgUrsMatrix[i * m_sgNameList.size() + i] = 0;
	}

	bool HtTpg::isSgUrsConsistent(int sg1Id, int sg1Rs, int sg2Id, int sg2Rs)
	{
		if (getUrsDelta(sg1Id, sg2Id) != RS_DELTA_UNKNOWN)
			return getUrsDelta(sg1Id, sg2Id) == sg1Rs - sg2Rs;

		setUrsDelta(sg1Id, sg2Id, sg1Rs - sg2Rs);
		setUrsDelta(sg2Id, sg1Id, sg2Rs - sg1Rs);

		return true;
	}

	bool HtTpg::areAllSgConnected(DiagnosticsEngine &diag)
	{
		// fill in connections where possible
		bool bProgress;
		do {
			bProgress = false;

			for (size_t i = 0; i < m_sgNameList.size(); i += 1) {
				for (size_t j = 0; j < m_sgNameList.size(); j += 1) {
					if (getUrsDelta(i, j) != RS_DELTA_UNKNOWN)
						continue;

					for (size_t k = 0; k < m_sgNameList.size(); k += 1) {
						if (getUrsDelta(i, k) == RS_DELTA_UNKNOWN || getUrsDelta(k, j) == RS_DELTA_UNKNOWN)
							continue;

						int delta = getUrsDelta(i, k) + getUrsDelta(k, j);
						setUrsDelta(i, j, delta);
						setUrsDelta(j, i, -delta);

						bProgress = true;
						break;
					}
				}
			}

		} while (bProgress);

		// now check connectivity
		bool bFullyConnected = true;
		for (size_t i = 0; i < m_sgNameList.size(); i += 1) {
			for (size_t j = 0; j < i; j += 1) {
				if (m_pSgUrsMatrix[j * m_sgNameList.size() + i] == RS_DELTA_UNKNOWN) {
					bFullyConnected = false;
					char errorMsg[256];
					sprintf(errorMsg, "subgroups of timing path group '%s' are not fully connected (%s and %s)",
						m_name.c_str(), m_sgNameList[j].c_str(), m_sgNameList[i].c_str());
					unsigned DiagID = diag.getCustomDiagID(DiagnosticsEngine::Error, errorMsg);
					diag.Report(DiagID);
				}
			}
		}
		return bFullyConnected;
	}

	void HtTpg::initTrsInfo()
	{
		m_sgTrsMaxList.resize(m_ursMax+1, 0);

		size_t sgCnt = m_sgNameList.size();
		for (int ursIdx = 0; ursIdx <= m_ursMax; ursIdx += 1) {
			int * pTrsMax = m_sgTrsMaxList[ursIdx] = new int [sgCnt];
			for (size_t i = 0; i < sgCnt; i += 1)
				pTrsMax[i] = -0x7fffffff;
		}

		m_sgTrsMatrixList.resize(m_ursMax+1, 0);

		size_t sg2Cnt = m_sgNameList.size() * m_sgNameList.size();
		for (int ursIdx = 0; ursIdx <= m_ursMax; ursIdx += 1) {
			int * pTrsMatrix = m_sgTrsMatrixList[ursIdx] = new int [sg2Cnt];
			for (size_t i = 0; i < sg2Cnt; i += 1)
				pTrsMatrix[i] = -0x7fffffff;

			for (size_t i = 0; i < sgCnt; i += 1)
				pTrsMatrix[i * sgCnt + i] = 0;
		}
	}

	void HtTpg::calcTrsInfo()
	{
		for (int ursIdx = 0; ursIdx < m_ursMax+1; ursIdx += 1) { 
			// fill in connections where possible
			bool bProgress;
			do {
				bProgress = false;

				for (size_t i = 0; i < m_sgNameList.size(); i += 1) {
					for (size_t j = 0; j < m_sgNameList.size(); j += 1) {
						if (getTrsDelta(i, j, ursIdx) != RS_DELTA_UNKNOWN)
							continue;

						for (size_t k = 0; k < m_sgNameList.size(); k += 1) {
							if (getTrsDelta(i, k, ursIdx) == RS_DELTA_UNKNOWN || getTrsDelta(k, j, ursIdx) == RS_DELTA_UNKNOWN)
								continue;

							int delta = getTrsDelta(i, k, ursIdx) + getTrsDelta(k, j, ursIdx);
							setTrsDelta(i, j, ursIdx, delta);
							setTrsDelta(j, i, ursIdx, -delta);

							bProgress = true;
							break;
						}
					}
				}

			} while (bProgress);
		}
	}
}
