/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#pragma once

namespace ht {

	// register stage constants
	#define URS_UNKNOWN -2
	#define URS_ISCONST -1
	#define TRS_UNKNOWN -2
	#define TPD_UNKNOWN -2
	#define GRP_UNKNOWN -2
	#define SG_UNKNOWN -2

	#define TPD_REG_OUT 300	// ps

	class HtDecl;

	class HtTpg {
	public:

		HtTpg() {
			m_ursMax = 0;
			m_pSgUrsMatrix = 0;
		}

		int getCkPeriod() { return 6666; }	// 150 Mhz

		void setName(string name) { m_name = name; }
		string getName() { return m_name; }

		void setUrsMax(int urs) { m_ursMax = max(m_ursMax, urs); }

		int getUrsDelta(int sgIdx1, int sgIdx2) {
			return m_pSgUrsMatrix[sgIdx1 * m_sgNameList.size() + sgIdx2];
		}
		void setUrsDelta(int sgIdx1, int sgIdx2, int ursDelta) {
			m_pSgUrsMatrix[sgIdx1 * m_sgNameList.size() + sgIdx2] = ursDelta;
		}
		int getTrsDelta(int sgIdx1, int sgIdx2, int sgUrs) {
			int * pTrsMatrix = m_sgTrsMatrixList[sgUrs];
			return pTrsMatrix[sgIdx1 * m_sgNameList.size() + sgIdx2];
		}
		void setTrsDelta(int sgIdx1, int sgIdx2, int sgUrs, int trsDelta) {
			int * pTrsMatrix = m_sgTrsMatrixList[sgUrs];
			pTrsMatrix[sgIdx1 * m_sgNameList.size() + sgIdx2] = trsDelta;
		}
		void setTrsMerge(int sgIdx1, int sgIdx2, int sgUrs, int trsDelta) {
			if (sgIdx1 > sgIdx2) {
				int tmp = sgIdx1;
				sgIdx1 = sgIdx2;
				sgIdx2 = tmp;
				trsDelta = -trsDelta;
			}

			int maxTrsDelta = max(getTrsDelta(sgIdx1, sgIdx2, sgUrs), trsDelta);
			setTrsDelta(sgIdx1, sgIdx2, sgUrs, maxTrsDelta);
			setTrsDelta(sgIdx2, sgIdx1, sgUrs, -maxTrsDelta);
		}
		void setSgMaxWrTrs(int sgIdx, int sgUrs, int trs) {
			int * pTrsMax = m_sgTrsMaxList[sgUrs];
			pTrsMax[sgIdx] = max(pTrsMax[sgIdx], trs);
		}
		int getSgMaxWrTrs(int sgIdx, int sgUrs) {
			int * pTrsMax = m_sgTrsMaxList[sgUrs];
			return pTrsMax[sgIdx];
		}

		void initUrsInfo();
		size_t addTpDecl(string &tpSgName, HtDecl * pDecl);
		bool areAllTpeReached();
		bool areAllSgConnected(DiagnosticsEngine &diag);
		bool isSgUrsConsistent(int sg1Id, int sg1Rs, int sg2Id, int sg2Rs);
		int findTpSgIdFromName(string sgName) {
			for (size_t i = 0; i < m_sgNameList.size(); i += 1)
				if (m_sgNameList[i] == sgName)
					return (int)i;
			return -1;
		}

		void initTrsInfo();
		void calcTrsInfo();

	private:
		string m_name;
		vector<HtDecl *> m_tpDeclList;
		vector<string> m_sgNameList;
		int m_ursMax;
		int * m_pSgUrsMatrix;
		vector<int *> m_sgTrsMaxList;
		vector<int *> m_sgTrsMatrixList;
	};
	typedef vector<HtTpg *> HtTpgList_t;

	struct HtTpgInfo {
		HtTpgInfo() {
			m_sgId = -1;
			m_sgUrs = URS_UNKNOWN;
			m_sgTrs = URS_UNKNOWN;
			m_sgTpd = 0;
			m_sgTsd = 0;
			m_isSgUrsComplete = false;
		}

		void setSgTpd( int sgTpd ) { m_sgTpd = sgTpd; }
		void setSgTsd( int sgTsd ) { m_sgTsd = sgTsd; }
		int getSgTpd() { return m_sgTpd + m_sgTsd; }

		bool m_isSgUrsComplete;
		int m_sgId;
		int m_sgUrs;
		int m_sgTrs;
	private:
		int m_sgTpd;
		int m_sgTsd;	// select delay
	};
}
