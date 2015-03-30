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

#include "HtTpgInfo.h"

namespace ht {

	class HtPath;

	class HtStmt : public HtNode {
		friend class HtExpr;
	public:

		HtStmt(Stmt::StmtClass stmtClass, HtLineInfo & lineInfo) : HtNode(lineInfo) {
			Init();
			m_stmtClass = stmtClass;
			if (m_stmtClass == Stmt::CompoundStmtClass)
				m_stmtList[0].isCompoundStmtList(true);
		}

		HtStmt(Stmt const * pStmt) : HtNode(pStmt->getSourceRange()) {
			Init();
			m_stmtClass = pStmt->getStmtClass();
			if (m_stmtClass == Stmt::CompoundStmtClass)
				m_stmtList[0].isCompoundStmtList(true);
		}

		HtStmt(HtStmt const * pRhs) : HtNode(pRhs) {
			Init();
			m_stmtClass = pRhs->m_stmtClass;
			m_isAssignStmt = pRhs->m_isAssignStmt;
			m_hasSubStmt = pRhs->m_hasSubStmt;
			m_isInitStmt = pRhs->m_isInitStmt;
			m_cntx = pRhs->m_cntx;
			m_pHtPragmaList = pRhs->m_pHtPragmaList;
			m_tpgInfo = pRhs->m_tpgInfo;
		}

		void Init() {
			m_isExpr = false;
			m_isAssignStmt = false;
			m_hasSubStmt = false;
			m_isInitStmt = false;
			m_isCtorMemberInit = false;
			m_pHtPragmaList = 0;
			m_pPath = 0;
		}

		void setStmtClass(Stmt::StmtClass stmtClass) { m_stmtClass = stmtClass; }
		Stmt::StmtClass getStmtClass() { return m_stmtClass; }

		void setSubExpr( HtStmt * pSubExpr ) { 
			if (pSubExpr == 0) return;
			assert(m_stmtList[2].size() == 0);
			m_stmtList[2].push_back( pSubExpr );
		}
		HtStmtList & getSubExpr() { return m_stmtList[2]; }
		int getSubExprIdx() { return 2; }

		void setSubStmt( HtStmt * pSubStmt ) { 
			if (pSubStmt == 0) return;
			assert(m_stmtList[2].size() == 0);
			m_stmtList[2].push_back( pSubStmt );
		}
		HtStmtList & getSubStmt() { return m_stmtList[2]; }
		int getSubStmtIdx() { return 2; }

		void setLhs( HtStmt * pLhs ) { 
			if (pLhs == 0) return;
			assert(m_stmtList[1].size() == 0);
			m_stmtList[1].push_back( pLhs );
		}
		HtStmtList & getLhs() { return m_stmtList[1]; }
		int getLhsIdx() { return 1; }

		void setRhs( HtStmt * pRhs ) {
			if (pRhs == 0) return;
			assert(m_stmtList[2].size() == 0);
			m_stmtList[2].push_back( pRhs );
		}
		HtStmtList & getRhs() { return m_stmtList[2]; }
		int getRhsIdx() { return 2; }

		// IfStmt, SwitchStmt / Conditional Expr
		void setCond( HtStmt * pCond ) {
			if (pCond == 0) return;
			assert(m_stmtList[0].size() == 0);
			m_stmtList[0].push_back( pCond );
		}
		HtStmtList & getCond() { return m_stmtList[0]; }
		int getCondIdx() { return 0; }

		void setThen( HtStmt * pThen ) {
			if (pThen == 0) return;
			m_stmtList[1].clear();
			m_stmtList[1].push_back( pThen );
		}
		HtStmtList & getThen() { return m_stmtList[1]; }
		int getThenIdx() { return 1; }

		void setElse( HtStmt * pElse ) {
			if (pElse == 0) return;
			m_stmtList[2].clear();
			m_stmtList[2].push_back( pElse );
		}
		HtStmtList & getElse() { return m_stmtList[2]; }
		int getElseIdx() { return 2; }

		void setBase( HtStmt * pBase ) {
			if (pBase == 0) return;
			m_stmtList[0].clear();
			m_stmtList[0].push_back( pBase );
		}
		HtStmtList & setBase() { m_stmtList[0].clear(); return m_stmtList[0]; }
		HtStmtList & getBase() { return m_stmtList[0]; }
		int getBaseIdx() { return 0; }

		void setIdx( HtStmt * pIdx ) {
			if (pIdx == 0) return;
			assert(m_stmtList[1].size() == 0);
			m_stmtList[1].push_back( pIdx );
		}
		HtStmtList & getIdx() { return m_stmtList[1]; }
		int getIdxIdx() { return 1; }

		void setCalleeExpr( HtStmt * pCallee ) {
			if (pCallee == 0) return;
			assert(m_stmtList[0].size() == 0);
			m_stmtList[0].push_back( pCallee );
		}
		HtStmtList & getCalleeExpr() { return m_stmtList[0]; }
		int getCalleeExprIdx() { return 0; }

		void setReplacement( HtStmt * pReplacement ) {
			if (pReplacement == 0) return;
			assert(m_stmtList[0].size() == 0);
			m_stmtList[0].push_back( pReplacement );
		}
		HtStmtList & getReplacement() { return m_stmtList[0]; }

		void setInit( HtStmt * pInit ) {
			if (pInit == 0) return;
			assert(m_stmtList[2].size() == 0);
			m_stmtList[2].push_back( pInit );
		}
		HtStmtList & getInit() { return m_stmtList[2]; }
		int getInitIdx() { return 2; }

		void setInc( HtStmt * pInc ) {
			if (pInc == 0) return;
			assert(m_stmtList[3].size() == 0);
			m_stmtList[3].push_back( pInc );
		}
		HtStmtList & getInc() { return m_stmtList[3]; }

		void setBody( HtStmt * pBody ) {
			if (pBody == 0) return;
			assert(m_stmtList[1].size() == 0);
			m_stmtList[1].push_back( pBody );
		}
		HtStmtList & getBody() { return m_stmtList[1]; }
		int getBodyIdx() { return 1; }

		void isAssignStmt(bool is) { m_isAssignStmt = is; }
		bool isAssignStmt() { return m_isAssignStmt; }

		void isCtorMemberInit(bool is) { m_isCtorMemberInit = is; }
		bool isCtorMemberInit() { return m_isCtorMemberInit; }

		void isInitStmt(bool is) { m_isInitStmt = is; }
		bool isInitStmt() { return m_isInitStmt; }

		void addDecl( HtDecl * pHtDecl ) { m_declList.push_back( pHtDecl ); m_cntx.addDecl( pHtDecl ); }
		HtDeclList_t & getDeclList() { return m_declList; }

		void addStmt( HtStmt * pHtStmt ) {
			if (pHtStmt == 0) return;
			m_stmtList[0].push_back(pHtStmt); 
		}
		HtStmtList & getStmtList() { return m_stmtList[0]; }

		HtCntxMap * getCntx() { return & m_cntx; }

		bool isExpr() { return m_isExpr; }

		void setPath(HtPath * pPath) { m_pPath = pPath; }
		HtPath * getPath() { return m_pPath; }

		void setPragmaList(vector<HtPragma *> * pHtPragmaList) { m_pHtPragmaList = pHtPragmaList; }
		bool isScMethod() {
			if (m_pHtPragmaList == 0) return false;
			for (size_t i = 0; i < m_pHtPragmaList->size(); i += 1)
				if ((*m_pHtPragmaList)[i]->isScMethod())
					return true;
			return false;
		}

		void setTpGrpId(int tpGrpId) { m_tpgInfo.m_tpGrpId = tpGrpId; }
		int getTpGrpId() { return m_tpgInfo.m_tpGrpId; }

		void setTpSgId(int tpSgId) { m_tpgInfo.m_tpSgId = tpSgId; }
		int getTpSgId() { return m_tpgInfo.m_tpSgId; }

		void setTpSgUrs(int tpSgUrs) { m_tpgInfo.m_tpSgUrs = tpSgUrs; }
		int getTpSgUrs() { return m_tpgInfo.m_tpSgUrs; }

		void setTpSgTrs(int tpSgTrs) { m_tpgInfo.m_tpSgTrs = tpSgTrs; }
		int getTpSgTrs() { return m_tpgInfo.m_tpSgTrs; }

		void setTpSgTpd(int tpSgTpd) { m_tpgInfo.m_tpSgTpd = tpSgTpd; }
		int getTpSgTpd() { return m_tpgInfo.m_tpSgTpd; }

		void isTpSgUrsComplete(bool is) { m_tpgInfo.m_isTpSgUrsComplete = is; }
		bool isTpSgUrsComplete() { return m_tpgInfo.m_isTpSgUrsComplete; }

	private:
		bool m_isExpr;

		Stmt::StmtClass m_stmtClass;

		HtStmtList m_stmtList[4];

		bool m_isAssignStmt;
		bool m_hasSubStmt;
		bool m_isInitStmt;
		bool m_isCtorMemberInit;
		
		HtCntxMap m_cntx;

		HtDeclList_t m_declList;

		vector<HtPragma *> * m_pHtPragmaList;

		HtPath * m_pPath;	// interconnect for output of this statement node

		struct ExprTpgInfo {	// TPG info
			int m_tpGrpId;
			int m_tpSgId;
			int m_tpSgUrs;
			int m_tpSgTrs;	// output stage for expression
			int m_tpSgTpd;
			bool m_isTpSgUrsComplete;

			ExprTpgInfo() {
				m_tpGrpId = GRP_UNKNOWN;
				m_tpSgId = SG_UNKNOWN;
				m_tpSgUrs = URS_UNKNOWN;
				m_tpSgTrs = TRS_UNKNOWN;
				m_tpSgTpd = TPD_UNKNOWN;
				m_isTpSgUrsComplete = false;
			}
		} m_tpgInfo;
	};
}
