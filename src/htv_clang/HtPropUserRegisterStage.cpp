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

#define URS_DEBUG 1

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

	class HtPropUserRegisterStages : HtWalkStmtTree
	{
	public:
		HtPropUserRegisterStages(HtDesign * pHtDesign, ASTContext &Context)
			: m_pHtDesign(pHtDesign), m_context(Context), m_diags(Context.getDiagnostics()) {}

		void propUserRegisterStages();

		bool walkCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid );
		void walkPostCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid );

	private:
		void propUserRegStage( HtStmtIter & iStmt );
		void checkInputExpr( HtStmt * pInExpr, bool & bIsUnknownInput, bool & bIsKnownInput, HtStmt * &pRsExpr);
		void updateOutputDecl( HtStmt * pOutExpr, HtStmt * pRsExpr );
		void updateOutputStmt( HtStmt * pOutExpr, HtStmt * pRsExpr, bool isPreRsComplete );

		bool isRegisterName(const string &name);

		HtDecl * findBaseDecl( HtExpr * pExpr );

	private:
		HtDesign * m_pHtDesign;
		ASTContext & m_context;
		DiagnosticsEngine & m_diags;

		HtTpg * m_pMethodTpGrp;
		bool m_bMethodTpError;
		bool m_bMethodTpProgress;
	};

	void HtDesign::propUserRegisterStages(ASTContext &Context)
	{
		HtPropUserRegisterStages transform(this, Context);

		transform.propUserRegisterStages();

		ExitIfError(Context);
	}

	void HtPropUserRegisterStages::propUserRegisterStages()
	{
		// Initialize Urs state
		m_pHtDesign->initUrsInfo();

		// Determine which variables are registers
		// Set regester stage to undetermined (-1)
		for (HtDeclIter iDecl(m_pHtDesign->getScModuleDeclList()); !iDecl.isEol(); iDecl++ ) {
			HtDecl * pHtDecl = *iDecl;

			pHtDecl->isRegister( isRegisterName( pHtDecl->getName() ));

			pHtDecl->setTpGrp(0);
			pHtDecl->setTpGrpId(GRP_UNKNOWN);
			pHtDecl->setTpSgId(SG_UNKNOWN);
			pHtDecl->setTpSgUrs(URS_UNKNOWN);
		}

		//   Foreach ScMethod
		//     Until all tpe is reached or no progress made
		//       Foreach method statement
		//         if any input variable has a defined register stage, propagate to output

		for (HtDeclIter iDecl = HtDeclIter(m_pHtDesign->getScMethodDeclList()); !iDecl.isEol(); iDecl++ ) {
			assert( (*iDecl)->getBody()->getStmtClass() == Stmt::CompoundStmtClass );

			m_pMethodTpGrp = 0;
			m_bMethodTpError = false;
			bool bTimingPathClosed;

			do {
				m_bMethodTpProgress = false;
				HtStmtIter iStmt ( (*iDecl)->getBody()->getStmtList() );

				while ( !iStmt.isEol() )
					propUserRegStage( iStmt );

				if (m_bMethodTpError)
					break;

				bTimingPathClosed = m_pMethodTpGrp->areAllTpeReached();

			} while (!bTimingPathClosed && m_bMethodTpProgress);

			m_pMethodTpGrp->areAllSgConnected(m_diags);

			(*iDecl)->setTpGrp(m_pMethodTpGrp);
		}

#if URS_DEBUG
		// dump module declarations
		printf(" PropUserRegisterStage dump:\n");
		for (HtDeclIter iDecl(m_pHtDesign->getScModuleDeclList()); !iDecl.isEol(); iDecl++ ) {
			HtDecl * pHtDecl = *iDecl;

			if (pHtDecl->getKind() != Decl::Field) continue;

			printf("   %-20s: isReg=%2d, grpId=%2d, sgId=%2d, sgUrs=%2d\n",
				pHtDecl->getName().c_str(), pHtDecl->isRegister(),
				pHtDecl->getTpGrpId(), pHtDecl->getTpSgId(), pHtDecl->getTpSgUrs());
		}
#endif
	}

	void HtPropUserRegisterStages::propUserRegStage( HtStmtIter & iStmt )
	{
		walkStmtTree( iStmt, iStmt, false, 0 );
	}

	bool HtPropUserRegisterStages::isRegisterName(const string &name)
	{
		const char *pName = name.c_str();
		if (*pName++ != 'r')
			return false;

		while (isdigit(*pName)) pName++;

		return *pName == '_';
	}

	bool HtPropUserRegisterStages::walkCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid )
	{
		Stmt::StmtClass stmtClass = iStmt->getStmtClass();
		switch (stmtClass) {
		case Stmt::MemberExprClass:
		case Stmt::IntegerLiteralClass:
			return false;
		case Stmt::IfStmtClass:
			{
				if (iStmt->isTpSgUrsComplete())
					// sub expressions have regStage set
					return false;
			}
			return true;
		case Stmt::BinaryOperatorClass:
		case Stmt::ParenExprClass:
		case Stmt::ImplicitCastExprClass:
		case Stmt::CallExprClass:
		case Stmt::CompoundStmtClass:
			{
				if (iStmt->isTpSgUrsComplete())
					// sub expressions have regStage set
					return false;
			}
			return true;
		default:
			assert(0);
			break;
		}
	}

	void HtPropUserRegisterStages::walkPostCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid )
	{
		if (iStmt->isTpSgUrsComplete())
			return;

		Stmt::StmtClass stmtClass = iStmt->getStmtClass();
		switch (stmtClass) {
		case Stmt::MemberExprClass:
			{
				// process variable
				HtExpr * pExpr = iStmt.asExpr();

				if (isLoa)
					break;

				HtDecl * pDecl = findBaseDecl( pExpr );
				HtTpInfo * pTpInfo = pDecl->getTpInfo();

				if (pTpInfo && pTpInfo->m_isTps) {
					HtTpg * pTpGrp = m_pHtDesign->getTpGrpFromGrpId( pTpInfo->getTpGrpId() );
					HtAssert(pTpGrp);

					if (m_pMethodTpGrp == 0)
						m_pMethodTpGrp = pTpGrp;
					else if (m_pMethodTpGrp != pTpGrp) {
						char errorMsg[256];
						sprintf(errorMsg, "multiple timing path groups found in method (%s and %s)",
							m_pMethodTpGrp->getName().c_str(), pTpGrp->getName().c_str());
						unsigned DiagID = m_diags.getCustomDiagID(DiagnosticsEngine::Error, errorMsg);
						m_diags.Report(pExpr->getLineInfo().getLoc(), DiagID);
					}

					int tpGrpId = pTpInfo->getTpGrpId();
					int tpSgId = pTpInfo->getTpSgId();
					int tpSgUrs = pTpInfo->getTpSgUrs();

					assert(tpGrpId >= 0);
					assert(tpSgId  >= 0);
					assert(tpSgUrs >= 0);

					pExpr->setTpGrpId( tpGrpId );
					pExpr->setTpSgId( tpSgId );
					pExpr->setTpSgUrs( tpSgUrs );
					pExpr->isTpSgUrsComplete( true );

					pDecl->setTpGrp( m_pHtDesign->getTpGrpFromGrpId( tpGrpId ) );
					pDecl->setTpGrpId( tpGrpId );
					pDecl->setTpSgId( tpSgId );
					pDecl->setTpSgUrs( tpSgUrs );

					m_bMethodTpProgress = true;
					break;
				}

				if (pDecl->getTpSgUrs() != URS_UNKNOWN) {
					pExpr->setTpGrpId( pDecl->getTpGrpId() );
					pExpr->setTpSgId( pDecl->getTpSgId() );
					pExpr->setTpSgUrs( pDecl->getTpSgUrs() );
					pExpr->isTpSgUrsComplete( true );
				}
			}
			break;
		case Stmt::IfStmtClass:
			{
				bool bIsUnknownInput = false;
				bool bIsKnownInput = false;
				HtStmt * pRsStmt = 0;

				checkInputExpr(iStmt->getCond().asStmt(), bIsUnknownInput, bIsKnownInput, pRsStmt);
				checkInputExpr(iStmt->getThen().asStmt(), bIsUnknownInput, bIsKnownInput, pRsStmt);
				checkInputExpr(iStmt->getElse().asStmt(), bIsUnknownInput, bIsKnownInput, pRsStmt);

				updateOutputStmt(iStmt.asStmt(), pRsStmt, !bIsUnknownInput);
			}
			break;
		case Stmt::CompoundStmtClass:
			{
				bool bIsUnknownInput = false;
				bool bIsKnownInput = false;
				HtStmt * pRsStmt = 0;

				for (HtStmtIter subStmtIter(iStmt->getStmtList()); !subStmtIter.isEol(); subStmtIter++)
				{
					checkInputExpr(subStmtIter.asStmt(), bIsUnknownInput, bIsKnownInput, pRsStmt);
				}

				updateOutputStmt(iStmt.asStmt(), pRsStmt, !bIsUnknownInput);
			}
			break;
		case Stmt::CallExprClass:
			{
				// propagate regStage/subGroup from inputs to outputs
				HtExpr * pExpr = iStmt.asExpr();

				bool bIsUnknownInput = false;
				bool bIsKnownInput = false;
				HtStmt * pRhsExpr = 0;

				// check inputs
				HtDeclList_iter_t paramIter = pExpr->getCalleeDecl()->getParamDeclList().begin();
				for (HtStmtIter argExprIter(pExpr->getArgExprList()); !argExprIter.isEol(); paramIter++, argExprIter++) {
					HtExpr * pParamExpr = argExprIter.asExpr();

					if (isParamTypeLoa((*paramIter)->getQualType())) {
						// call output

					} else {
						// call input
						checkInputExpr(pParamExpr, bIsUnknownInput, bIsKnownInput, pRhsExpr);
					}
				}

				// update outputs
				if (bIsKnownInput) {
					// at least one input has a known regStage, assign outputs
					HtDeclList_iter_t paramIter = pExpr->getCalleeDecl()->getParamDeclList().begin();
					for (HtStmtIter argExprIter(pExpr->getArgExprList()); !argExprIter.isEol(); paramIter++, argExprIter++) {
						HtExpr * pParamExpr = argExprIter.asExpr();

						if (isParamTypeLoa((*paramIter)->getQualType())) {
							// call output
							updateOutputDecl(pParamExpr, pRhsExpr);
						} else {
							// call input
						}
					}
				}

				updateOutputStmt(pExpr, pRhsExpr, !bIsUnknownInput);
			}
			break;
		case Stmt::BinaryOperatorClass:
			{
				// propagate regStage/subGroup from leaf expr
				HtExpr * pExpr = iStmt.asExpr();
				HtExpr * pLhsExpr = pExpr->getLhs().asExpr();
				HtExpr * pRhsExpr = pExpr->getRhs().asExpr();

				if (pExpr->isAssignStmt()) {
					bool bIsUnknownInput = false;
					bool bIsKnownInput = false;
					HtStmt * pRsltExpr = 0;

					checkInputExpr(pRhsExpr, bIsUnknownInput, bIsKnownInput, pRsltExpr);

					updateOutputDecl(pLhsExpr, pRhsExpr);
					updateOutputStmt(pExpr, pRsltExpr, !bIsUnknownInput);

				} else {
					bool bIsUnknownInput = false;
					bool bIsKnownInput = false;
					HtStmt * pRsltExpr = 0;

					checkInputExpr(pLhsExpr, bIsUnknownInput, bIsKnownInput, pRsltExpr);
					checkInputExpr(pRhsExpr, bIsUnknownInput, bIsKnownInput, pRsltExpr);

					updateOutputStmt(pExpr, pRsltExpr, !bIsUnknownInput);
				}
			}
			break;
		case Stmt::ImplicitCastExprClass:
		case Stmt::ParenExprClass:
			{
				// propagate regStage/subGroup from leaf expr
				HtExpr * pExpr = iStmt.asExpr();
				HtExpr * pSubExpr = pExpr->getSubExpr().asExpr();

				if (pSubExpr->getTpSgUrs() == URS_UNKNOWN)
					break;

				updateOutputStmt(pExpr, pSubExpr, true);
			}
			break;
		case Stmt::IntegerLiteralClass:
			{
				HtExpr * pExpr = iStmt.asExpr();
				updateOutputStmt(pExpr, 0, true);
				pExpr->setTpSgUrs( URS_ISCONST );
			}
			break;
		default:
			assert(0);
			break;
		}
	}

	void HtPropUserRegisterStages::checkInputExpr( HtStmt * pInExpr, 
		bool & bIsUnknownInput, bool & bIsKnownInput, HtStmt * &pRsExpr)
	{
		bIsUnknownInput |= pInExpr->getTpSgUrs() == URS_UNKNOWN;

		if (pRsExpr && pRsExpr->getTpSgId() >= 0 && pInExpr->getTpSgId() >= 0) {
			if (pRsExpr->getTpSgId() != pInExpr->getTpSgId())
				m_pMethodTpGrp->isSgUrsConsistent(pRsExpr->getTpSgId(), pRsExpr->getTpSgUrs(),
					pInExpr->getTpSgId(), pInExpr->getTpSgUrs());
		}

		if (pInExpr->getTpSgUrs() != URS_UNKNOWN) {
			bIsKnownInput = true;

			// check if subgroups or regstage differs, ignore const
			if (pRsExpr == 0 || pRsExpr->getTpSgUrs() == URS_ISCONST ||
				pInExpr->getTpSgUrs() != URS_ISCONST && pInExpr->getTpSgUrs() < pRsExpr->getTpSgUrs())
			{
				pRsExpr = pInExpr;
			}
		}
	}

	void HtPropUserRegisterStages::updateOutputDecl( HtStmt * pOutStmt, HtStmt * pRsExpr )
	{
		assert(pOutStmt->getStmtClass() == Stmt::MemberExprClass);
		HtDecl * pOutDecl = findBaseDecl( (HtExpr *)pOutStmt );

		HtTpg * pTpGrp = 0;
		if (pRsExpr->getTpSgUrs() >= 0)
			pTpGrp = m_pHtDesign->getTpGrpFromGrpId( pRsExpr->getTpGrpId() );

		m_bMethodTpProgress |= pOutDecl->getTpSgUrs() == URS_UNKNOWN;

		int newSgUrs = 0;
		if (pOutDecl->isRegister() && pRsExpr->getTpSgUrs() >= 0) {
			newSgUrs = pRsExpr->getTpSgUrs() + 1;
			pTpGrp->setUrsMax(newSgUrs);
		} else
			newSgUrs = pRsExpr->getTpSgUrs();

		if (pOutDecl->getTpSgId() >= 0 && pRsExpr->getTpSgId() >= 0) {
			assert(pOutDecl->getTpSgId() == pRsExpr->getTpSgId());
		} else if (pOutDecl->getTpSgId() < 0) {
			if (pRsExpr->getTpGrpId() >= 0)
				pOutDecl->setTpGrp( m_pHtDesign->getTpGrpFromGrpId( pRsExpr->getTpGrpId() ) );
			pOutDecl->setTpGrpId( pRsExpr->getTpGrpId() );
			pOutDecl->setTpSgId( pRsExpr->getTpSgId() );
		}

		if (pOutDecl->getTpSgUrs() == URS_UNKNOWN ||
			pOutDecl->getTpSgUrs() == URS_ISCONST && pRsExpr->getTpSgUrs() != URS_UNKNOWN)
		{
			pOutDecl->setTpSgUrs( newSgUrs );
		}

		pOutStmt->setTpGrpId( pOutDecl->getTpGrpId() );
		pOutStmt->setTpSgId( pOutDecl->getTpSgId() );
		pOutStmt->setTpSgUrs( pOutDecl->getTpSgUrs() );

		HtTpInfo * pTpInfo = pOutDecl->getTpInfo();
		if (pTpInfo && !pTpInfo->m_isTps) {
			// an end point
			if (pTpInfo->getTpSgId() >= 0 && pOutDecl->getTpSgId() >= 0) {
				if (pTpInfo->getTpSgId() != pOutDecl->getTpSgId())
					pTpGrp->isSgUrsConsistent(pTpInfo->getTpSgId(), pTpInfo->getTpSgUrs(),
						pOutDecl->getTpSgId(), pOutDecl->getTpSgUrs());
			}
		}
	}

	void HtPropUserRegisterStages::updateOutputStmt( HtStmt * pOutStmt, HtStmt * pRsExpr, bool isPreRsComplete )
	{
		m_bMethodTpProgress |= isPreRsComplete || pOutStmt->getTpSgUrs() == URS_UNKNOWN;

		if (pRsExpr) {
			pOutStmt->setTpGrpId( pRsExpr->getTpGrpId() );
			pOutStmt->setTpSgId( pRsExpr->getTpSgId() );
			pOutStmt->setTpSgUrs( pRsExpr->getTpSgUrs() );
		}
		pOutStmt->isTpSgUrsComplete( isPreRsComplete );
	}

	HtDecl * HtPropUserRegisterStages::findBaseDecl( HtExpr * pExpr )
	{
		// find the base declaration for the reference
		HtDecl * pBaseDecl = 0;
		for (;;) {
			Stmt::StmtClass stmtClass = pExpr->getStmtClass();
			switch (stmtClass) {
			case Stmt::MemberExprClass:
				pBaseDecl = pExpr->getMemberDecl();
				assert(pBaseDecl->getKind() == Decl::Field);
				pExpr = pExpr->getBase().asExpr();
				break;
			case Stmt::ImplicitCastExprClass:
			case Stmt::CXXStaticCastExprClass:
				pExpr = pExpr->getSubExpr().asExpr();
				break;
			case Stmt::CXXThisExprClass:
				return pBaseDecl;
			default:
				assert(0);
			}
		}
		return 0;
	}
}
