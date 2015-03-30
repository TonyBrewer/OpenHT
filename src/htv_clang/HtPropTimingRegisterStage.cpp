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

#define TRS_DEBUG 1

namespace ht {

	// Insert register stages as appropriate to achieve timing
	//   Foreach declaration:
	//     Determine which are user provided registers
	//   Foreach ScMethod
	//     Until all tpe is reached or no progress made
	//       Foreach method statement
	//         if any input variable has a defined register stage, propagate to output
	//   Foreach statement:
	//     Verify that all inputs are in same timing group
	//     At each node in statement:
	//       Check if node's inputs are on same clock, if not add register stage(s)
	//       Check if node's operation would cause output to violate timing
	//         Add register stage to all inputs to avoid violation

	class HtPropTrsUpWard : HtWalkStmtTree
	{
	public:
		HtPropTrsUpWard(HtDesign * pHtDesign, ASTContext &Context)
			: m_pHtDesign(pHtDesign), m_context(Context), m_diags(Context.getDiagnostics()) {}

		void propTrsUpWard();

		bool walkCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid );
		void walkPostCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid );

	private:
		void propTimingRegStage( HtStmtIter & iStmt );

		void propIfStmt( HtExpr * pCond, HtStmt * pThen );
		void propAssignOperator(HtExpr * pLhsExpr, int sgTrs, int sgTpd);
		void propBinaryOperator(int & sgTrs, int & sgTpd, BinaryOperator::Opcode opcode, HtExpr * pLhsExpr, HtExpr * pRhsExpr);
		void propCallExpr( HtExpr * pCallExpr );

		HtDecl * findBaseDecl( HtExpr * pExpr ) {
			return m_pHtDesign->findBaseDecl( pExpr );
		}

	private:
		HtDesign * m_pHtDesign;
		ASTContext & m_context;
		DiagnosticsEngine & m_diags;
		HtTpg * m_pMethodTpGrp;
		bool m_bInIfOrSwitch;
	};

	class HtPropTrsDownWard : HtWalkStmtTree
	{
	public:
		HtPropTrsDownWard(HtDesign * pHtDesign, ASTContext &Context)
			: m_pHtDesign(pHtDesign), m_context(Context), m_diags(Context.getDiagnostics()) {}

		void propTrsDownWard();

		bool walkCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid );
		void walkPostCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid ) {}

	private:

		HtDecl * findBaseDecl( HtExpr * pExpr ) {
			return m_pHtDesign->findBaseDecl( pExpr );
		}

	private:
		HtDesign * m_pHtDesign;
		ASTContext & m_context;
		DiagnosticsEngine & m_diags;
		HtTpg * m_pMethodTpGrp;
	};

	void HtDesign::propTimingRegisterStages(ASTContext &Context)
	{
		HtPropTrsUpWard transform1(this, Context);
		transform1.propTrsUpWard();

		HtPropTrsDownWard transform2(this, Context);
		transform2.propTrsDownWard();

#if TRS_DEBUG
		// dump module declarations
		printf(" PropTimingRegisterStage dump:\n");
		for (HtDeclIter iDecl(getScModuleDeclList()); !iDecl.isEol(); iDecl++ ) {
			HtDecl * pHtDecl = *iDecl;

			if (pHtDecl->getKind() != Decl::Field) continue;

			printf("   %-18s: isReg=%d, sgId=%2d, sgUrs=%2d, minWrTrs=%2d, maxWrTrs=%2d, maxRdTrs=%2d, sgTpd=%2d\n",
				pHtDecl->getName().c_str(), pHtDecl->isRegister(),
				pHtDecl->getTpSgId(), pHtDecl->getTpSgUrs(), pHtDecl->getTpSgMinWrTrs(),
				pHtDecl->getTpSgMaxWrTrs(), pHtDecl->getTpSgMaxRdTrs(), pHtDecl->getTpSgMaxRdTrs());
		}
#endif

		ExitIfError(Context);
	}

	void HtPropTrsUpWard::propTrsUpWard()
	{
		//////////////////////////////////////////////////////////////////////////////////
		// second phase - determine max timing register stages per user register stage
		//     single pass
		//

		m_pHtDesign->initTrsInfo();

		// initialize tps variables, registers to trs=0 and other variables to unknown
		for (HtDeclIter iDecl(m_pHtDesign->getScModuleDeclList()); !iDecl.isEol(); iDecl++ )
			iDecl->initTpSgTrs();

		for (HtDeclIter iDecl(m_pHtDesign->getScMethodDeclList()); !iDecl.isEol(); iDecl++ ) {
			m_pMethodTpGrp = (*iDecl)->getTpGrp();
			assert( (*iDecl)->getBody()->getStmtClass() == Stmt::CompoundStmtClass );
			HtStmtIter iStmt ( (*iDecl)->getBody()->getStmtList() );

			m_pMethodTpGrp->initTrsInfo();

			while ( !iStmt.isEol() )
				propTimingRegStage( iStmt );

			m_pMethodTpGrp->calcTrsInfo();
		}
	}

	void HtPropTrsUpWard::propTimingRegStage( HtStmtIter & iStmt )
	{
		m_bInIfOrSwitch = iStmt->getStmtClass() == Stmt::IfStmtClass;

		walkStmtTree( iStmt, iStmt, false, 0 );
	}

	bool HtPropTrsUpWard::walkCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid )
	{
		Stmt::StmtClass stmtClass = iStmt->getStmtClass();
		switch (stmtClass) {
		case Stmt::MemberExprClass:
		case Stmt::IntegerLiteralClass:
			return false;
		case Stmt::IfStmtClass:
		case Stmt::BinaryOperatorClass:
		case Stmt::ImplicitCastExprClass:
		case Stmt::ParenExprClass:
		case Stmt::CallExprClass:
		case Stmt::CompoundStmtClass:
			return true;
		default:
			assert(0);
			break;
		}
	}

	void HtPropTrsUpWard::walkPostCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid )
	{
		if (iStmt->getTpSgUrs() < 0)
			return;

		Stmt::StmtClass stmtClass = iStmt->getStmtClass();
		switch (stmtClass) {
		case Stmt::MemberExprClass:
			{
				HtExpr * pExpr = iStmt.asExpr();
				HtDecl * pDecl = findBaseDecl( pExpr );

				if (pDecl->getName() == "r_t")
					bool stop = true;
					
				if (isLoa)
					break;

				assert(pDecl->getTpSgMaxWrTrs(true) != TRS_UNKNOWN);

				pExpr->setTpSgTrs( pDecl->getTpSgMaxWrTrs(true) );
				pExpr->setTpSgTpd( pDecl->getTpSgTpd() );

				pDecl->setTpSgRdTrs( pDecl->getTpSgMaxWrTrs(true) );
			}
			break;
		case Stmt::IfStmtClass:
			{
				// walk then and else statement lists
				//   set lhs decl trs

				propIfStmt(iStmt->getCond().asExpr(), iStmt->getThen().asStmt());
				propIfStmt(iStmt->getCond().asExpr(), iStmt->getElse().asStmt());
			}
			break;
		case Stmt::CompoundStmtClass:
			break;
		case Stmt::CallExprClass:
			{
				// propagate trs/tpd from inputs to outputs
				HtExpr * pExpr = iStmt.asExpr();

				propCallExpr( pExpr );
			}
			break;
		case Stmt::BinaryOperatorClass:
			{
				// propagate regStage/subGroup from leaf expr
				HtExpr * pExpr = iStmt.asExpr();
				HtExpr * pLhsExpr = pExpr->getLhs().asExpr();
				HtExpr * pRhsExpr = pExpr->getRhs().asExpr();

				if (pExpr->isAssignStmt()) {

					if (!m_bInIfOrSwitch)
						propAssignOperator(pLhsExpr, pRhsExpr->getTpSgTrs(), pRhsExpr->getTpSgTpd());

				} else {
					int sgTrs, sgTpd;

					propBinaryOperator(sgTrs, sgTpd, pExpr->getBinaryOperator(), pLhsExpr, pRhsExpr);

					pExpr->setTpSgTrs( sgTrs );
					pExpr->setTpSgTpd( sgTpd );
				}
			}
			break;
		case Stmt::ImplicitCastExprClass:
		case Stmt::ParenExprClass:
			{
				// propagate regStage/subGroup from leaf expr
				HtExpr * pExpr = iStmt.asExpr();
				HtExpr * pSubExpr = pExpr->getSubExpr().asExpr();

				assert(pSubExpr->getTpSgUrs() != URS_UNKNOWN);

				pExpr->setTpSgTrs( pSubExpr->getTpSgTrs() );
				pExpr->setTpSgTpd( pSubExpr->getTpSgTpd() );
			}
			break;
		case Stmt::IntegerLiteralClass:
			break;
		default:
			assert(0);
			break;
		}
	}

	void HtPropTrsUpWard::propIfStmt( HtExpr * pCond, HtStmt * pThen )
	{
		FpgaTimingModel * pFpgaTm = m_pHtDesign->getFpgaTimingModel();

		int muxTpd = 200; // mux selection time

		if (pCond->getTpSgTpd() + muxTpd > m_pMethodTpGrp->getCkPeriod() - pFpgaTm->getRegSetup()) {
			// need register stage
			pCond->setTpSgTrs( pCond->getTpSgTrs() + 1);
			pCond->setTpSgTpd( pFpgaTm->getRegCkOut() );
		}

		int condTrs = pCond->getTpSgTrs();
		int condTpd = pCond->getTpSgTpd();

		assert(pCond->getStmtClass() == Stmt::MemberExprClass);
		HtDecl * pCondDecl = findBaseDecl( pCond );
		
		// walk then or else statement list and set trs/tpd

		for (HtStmtIter iStmt(pThen->getStmtList()); !iStmt.isEol(); iStmt++) {
			assert(iStmt->getStmtClass() == Stmt::BinaryOperatorClass);

			HtExpr * pLhsExpr = iStmt->getLhs().asExpr();
			assert(pLhsExpr->getStmtClass() == Stmt::MemberExprClass);
			HtExpr * pRhsExpr = iStmt->getRhs().asExpr();

			int lhsTrs;
			int lhsTpd;

			if (pRhsExpr->getTpSgTrs() < condTrs ||
				pRhsExpr->getTpSgTrs() == condTrs && pRhsExpr->getTpSgTpd() < condTpd)
			{
				lhsTrs = condTrs;
				lhsTpd = condTpd;

			} else if (pRhsExpr->getTpSgTpd() + muxTpd > m_pMethodTpGrp->getCkPeriod() - pFpgaTm->getRegSetup()) {
				// need register stage
				lhsTrs = pRhsExpr->getTpSgTrs() + 1;
				lhsTpd = pFpgaTm->getRegCkOut();

			} else {
				lhsTrs = pRhsExpr->getTpSgTrs();
				lhsTpd = pRhsExpr->getTpSgTpd();
			}

			pCondDecl->setTpSgRdTrs( lhsTrs );

			propAssignOperator(pLhsExpr, lhsTrs, lhsTpd);
		}
	}

	void HtPropTrsUpWard::propAssignOperator(HtExpr * pLhsExpr, int sgTrs, int sgTpd)
	{
		assert(pLhsExpr->getStmtClass() == Stmt::MemberExprClass);
		HtDecl * pLhsDecl = findBaseDecl( (HtExpr *)pLhsExpr );

		pLhsExpr->setTpSgTrs( sgTrs );
		pLhsExpr->setTpSgTpd( sgTpd );

		pLhsDecl->setTpSgWrTrs( sgTrs );
		pLhsDecl->setTpSgTpd( sgTpd );

		HtTpInfo * pTpInfo = pLhsDecl->getTpInfo();
		if (pTpInfo && !pTpInfo->m_isTps) {
			// end point reached

			if (pTpInfo->getTpSgId() >= 0 && pLhsDecl->getTpSgId() >= 0) {
				assert (pTpInfo->getTpSgId() != pLhsDecl->getTpSgId());

				// record trs for urs stage
				m_pMethodTpGrp->setTrsMerge( pLhsExpr->getTpSgId(), pTpInfo->getTpSgId(),
					pLhsExpr->getTpSgUrs(), sgTrs );
			}
		}
	}

	void HtPropTrsUpWard::propBinaryOperator(int & sgTrs, int & sgTpd, 
		BinaryOperator::Opcode opcode, HtExpr * pLhsExpr, HtExpr * pRhsExpr)
	{
		int lhsTpd, rhsTpd;

		FpgaTimingModel * pFpgaTm = m_pHtDesign->getFpgaTimingModel();

		switch (opcode) {
		case BO_EQ:
			lhsTpd = 200;
			rhsTpd = 200;
			break;
		case BO_Add:
			lhsTpd = 200;
			rhsTpd = 200;
			break;
		default:
			assert(0);
		}

		if (pLhsExpr->getTpSgTpd() + lhsTpd > m_pMethodTpGrp->getCkPeriod() - pFpgaTm->getRegSetup()) {
			// need register stage
			pLhsExpr->setTpSgTrs( pLhsExpr->getTpSgTrs() + 1);
			pLhsExpr->setTpSgTpd( pFpgaTm->getRegCkOut() );
		}

		if (pRhsExpr->getTpSgTpd() + rhsTpd > m_pMethodTpGrp->getCkPeriod() - pFpgaTm->getRegSetup()) {
			// need register stage
			pRhsExpr->setTpSgTrs( pRhsExpr->getTpSgTrs() + 1);
			pRhsExpr->setTpSgTpd( pFpgaTm->getRegCkOut() );
		}

		if ( pLhsExpr->getTpSgId() == pRhsExpr->getTpSgId() ) {

			if ( pLhsExpr->getTpSgTrs() > pRhsExpr->getTpSgTrs() ) {

				pRhsExpr->setTpSgTrs( pLhsExpr->getTpSgTrs() );
				pRhsExpr->setTpSgTpd( pFpgaTm->getRegCkOut() );

			} else if ( pLhsExpr->getTpSgTrs() < pRhsExpr->getTpSgTrs() ) {

				pLhsExpr->setTpSgTrs( pRhsExpr->getTpSgTrs() );
				pLhsExpr->setTpSgTpd( pFpgaTm->getRegCkOut() );
			}

			assert( pLhsExpr->getTpSgTrs() == pRhsExpr->getTpSgTrs() );

			sgTrs = pLhsExpr->getTpSgTrs();
			sgTpd = max( pLhsExpr->getTpSgTpd() + lhsTpd, pRhsExpr->getTpSgTpd() + rhsTpd );

		} else {
			assert( pLhsExpr->getTpSgUrs() == pRhsExpr->getTpSgUrs() );

			bool bPrimaryTpgIsLhs = pLhsExpr->getTpSgId() < pRhsExpr->getTpSgId();
			HtExpr * p1stExpr = bPrimaryTpgIsLhs ? pLhsExpr : pRhsExpr;
			HtExpr * p2ndExpr = bPrimaryTpgIsLhs ? pRhsExpr : pLhsExpr;

			if (p1stExpr->getTpSgTpd() < p2ndExpr->getTpSgTpd()) {
				if (p2ndExpr->getTpSgTpd() - p1stExpr->getTpSgTpd() < m_pMethodTpGrp->getCkPeriod() / 2) {

					// if primary is earlier then secondary by small amount the just delay primary
					p1stExpr->setTpSgTpd( p2ndExpr->getTpSgTpd() );

				} else {
					// if primary is earlier then secondary by large amount the register secondary
					p2ndExpr->setTpSgTrs( p2ndExpr->getTpSgTrs() + 1 );
					p2ndExpr->setTpSgTpd( pFpgaTm->getRegCkOut() );
				}
			}

			// record trs delta for urs stage
			m_pMethodTpGrp->setTrsMerge( p1stExpr->getTpSgId(), p2ndExpr->getTpSgId(),
				p1stExpr->getTpSgUrs(), p1stExpr->getTpSgTrs() - p2ndExpr->getTpSgTrs() );

			sgTrs = p1stExpr->getTpSgTrs();
			sgTpd = max( pLhsExpr->getTpSgTpd() + lhsTpd, pRhsExpr->getTpSgTpd() + rhsTpd );
		}
	}

	void HtPropTrsUpWard::propCallExpr( HtExpr * pCallExpr )
	{
		HtDecl * pCalleeDecl = pCallExpr->getCalleeDecl();
		if (pCalleeDecl->getGeneralTemplateDecl())
			pCalleeDecl = pCalleeDecl->getGeneralTemplateDecl();

		FpgaTimingModel * pFpgaTm = m_pHtDesign->getFpgaTimingModel();

		// find base trs for primitive
		int baseTrs = -0x7fffffff;
		HtDeclList_iter_t paramIter = pCalleeDecl->getParamDeclList().begin();
		for (HtStmtIter argExprIter(pCallExpr->getArgExprList()); !argExprIter.isEol(); paramIter++, argExprIter++) {
			HtExpr * pParamExpr = argExprIter.asExpr();
			HtDecl * pParamDecl = *paramIter;

			if (m_pHtDesign->isParamOutput(pParamDecl->getQualType())) {
				// outputs

			} else {
				// verify inputs meet setup, add register if not
				int primTsu = pParamDecl->getTpInfo()->getTpSgTpd();
				if (primTsu == 0)
					primTsu = pFpgaTm->getRegSetup();

				if (pParamExpr->getTpSgTpd() > m_pMethodTpGrp->getCkPeriod() - primTsu) {
					// need register stage
					pParamExpr->setTpSgTrs( pParamExpr->getTpSgTrs() + 1);
					pParamExpr->setTpSgTpd( pFpgaTm->getRegCkOut() );
				}

				assert(pParamDecl->getTpInfo()->getTpSgTrs() >= 0);
				int rsDelta = pParamExpr->getTpSgTrs() - pParamDecl->getTpInfo()->getTpSgTrs();
				baseTrs = max(baseTrs, rsDelta);
			}
		}

		// add register stages to input exprs, assign trs for output exprs
		paramIter = pCalleeDecl->getParamDeclList().begin();
		for (HtStmtIter argExprIter(pCallExpr->getArgExprList()); !argExprIter.isEol(); paramIter++, argExprIter++) {
			HtExpr * pParamExpr = argExprIter.asExpr();
			HtDecl * pParamDecl = *paramIter;

			if (m_pHtDesign->isParamOutput((*paramIter)->getQualType())) {
				// call output
				int sgTrs = pParamDecl->getTpInfo()->getTpSgTrs() + baseTrs;
				int sgTpd = pParamDecl->getTpInfo()->getTpSgTpd();

				propAssignOperator( pParamExpr, sgTrs, sgTpd );

			} else {
				// call input
				int rsDelta = pParamExpr->getTpSgTrs() - pParamDecl->getTpInfo()->getTpSgTrs();
				if (rsDelta < baseTrs) {
					pParamExpr->setTpSgTrs( pParamExpr->getTpSgTrs() + baseTrs - rsDelta);
					pParamExpr->setTpSgTpd( pFpgaTm->getRegCkOut() );
				}
			}
		}
	}


	///////////////////////////////////////////////////////////////////
	// Propagate trs downward from expression output to input members

	void HtPropTrsDownWard::propTrsDownWard()
	{
		HtDeclList_t & moduleDeclList = m_pHtDesign->getScModuleDeclList();

		// walk each ScMethod in design
		for (HtDeclIter iDecl(m_pHtDesign->getScMethodDeclList()); !iDecl.isEol(); iDecl++) {

			m_pMethodTpGrp = (*iDecl)->getTpGrp();
			assert( (*iDecl)->getBody()->getStmtClass() == Stmt::CompoundStmtClass );
			HtStmtIter iStmt ( (*iDecl)->getBody()->getStmtList() );

			HtStmtIter iInsert;
			while ( !iStmt.isEol() )
				walkStmtTree( iStmt, iInsert, false, 0 );
		}
	}

	bool HtPropTrsDownWard::walkCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid )
	{
		Stmt::StmtClass stmtClass = iStmt->getStmtClass();
		switch (stmtClass) {
		case Stmt::MemberExprClass:
			{
				if (isLoa)
					return false;

				HtExpr * pExpr = iStmt.asExpr();
				int rdTrs = pExpr->getTpSgTrs();
					
				HtDecl * pDecl = findBaseDecl( (HtExpr *)pExpr );
				pDecl->setTpSgRdTrs( rdTrs );
			}
			return false;
		case Stmt::IntegerLiteralClass:
		case Stmt::IfStmtClass:
			return false;
		case Stmt::CallExprClass:
		case Stmt::CompoundStmtClass:
			return true;
		case Stmt::BinaryOperatorClass:
			{
				HtExpr * pExpr = iStmt.asExpr();
				HtExpr * pLhsExpr = pExpr->getLhs().asExpr();
				HtExpr * pRhsExpr = pExpr->getRhs().asExpr();

				if (pExpr->isAssignStmt()) {
					// propagate lhs endpoint trs to rhs
					assert(pLhsExpr->getStmtClass() == Stmt::MemberExprClass);
					HtDecl * pLhsDecl = findBaseDecl( (HtExpr *)pLhsExpr );

					if (pLhsDecl->getName() == "r_c")
						bool stop = true;

					HtTpInfo * pTpInfo = pLhsDecl->getTpInfo();
					if (pTpInfo && !pTpInfo->m_isTps) {
						// end point reached

						if (pTpInfo->getTpSgId() >= 0 && pLhsDecl->getTpSgId() >= 0) {
							assert (pTpInfo->getTpSgId() != pLhsDecl->getTpSgId());

							int trsDelta = m_pMethodTpGrp->getTrsDelta(pLhsExpr->getTpSgId(), pTpInfo->getTpSgId(),
								pLhsExpr->getTpSgUrs());

							// propagate trs to rhs
							pRhsExpr->setTpSgTrs( trsDelta );
							pRhsExpr->setTpSgTpd( pTpInfo->getTpSgTpd() );
						}
						return true;
					}

					if (pLhsDecl->isRegister()) {
						// a register is an end point

						// propagate trs to rhs
						pRhsExpr->setTpSgTrs( pLhsDecl->getTpSgMaxWrTrs() );
						pRhsExpr->setTpSgTpd( pLhsDecl->getTpSgTpd() );

						return true;
					}

					// propagate trs to rhs
					pRhsExpr->setTpSgTrs( pLhsExpr->getTpSgTrs() );
					pRhsExpr->setTpSgTpd( pLhsExpr->getTpSgTpd() );
				}
			}
			return true;
		case Stmt::ImplicitCastExprClass:
		case Stmt::ParenExprClass:
			{
				// propagate Trs/Tpd to subexpression
				HtExpr * pSubExpr = iStmt->getSubExpr().asExpr();
				pSubExpr->setTpSgTrs( iStmt->getTpSgTrs() );
				pSubExpr->setTpSgTpd( iStmt->getTpSgTpd() );
			}
			return true;
		default:
			assert(0);
			break;
		}
	}


}
