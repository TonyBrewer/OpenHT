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

	class HtTransformConditionalBase : HtDupStmtTree, HtWalkStmtTree
	{
	public:
		HtTransformConditionalBase(HtDesign * pHtDesign, ASTContext &Context)
			: m_pHtDesign(pHtDesign), m_context(Context), m_diags(Context.getDiagnostics()) {}

		void transformConditionalBase();

		bool walkCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid );
		void walkPostCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid ) { /*iStmt++;*/ }
		HtStmt * dupCallback( HtStmtIter & iSrcStmt, void * pVoid );

		struct WalkParams {
			WalkParams() : m_iAssignExpr(HtStmtList()), m_iDupExpr(HtStmtList()), m_isWalkBaseExpr(false) {}
			WalkParams( HtStmtList & list ) : m_iAssignExpr(list), m_iDupExpr(HtStmtList()), m_isWalkBaseExpr(false) {}

			HtStmtIter m_iAssignExpr;
			HtStmtIter m_iDupExpr;
			bool m_isWalkBaseExpr;
		};

	private:
		HtDesign * m_pHtDesign;
		ASTContext & m_context;
		DiagnosticsEngine & m_diags;
	};

	void HtDesign::transformConditionalBase(ASTContext &Context)
	{
		HtTransformConditionalBase transform(this, Context);

		transform.transformConditionalBase();

		ExitIfError(Context);
	}

	void HtTransformConditionalBase::transformConditionalBase()
	{
		HtDeclIter methodIter(m_pHtDesign->getScMethodDeclList());
		while ( !methodIter.isEol() ) {

			// walk statement list looking for expressions where base
			//  has a conditional expression
			HtStmtIter stmtIter ( (*methodIter)->getBody() );

			HtStmtIter iEmpty;
			WalkParams params;
			walkStmtTree( stmtIter, iEmpty, false, &params );

			methodIter++;
		}
	}

	bool HtTransformConditionalBase::walkCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid )
	{
		WalkParams * pParams = (WalkParams *)pVoid;

		if (pParams->m_isWalkBaseExpr) {
			if (iStmt->getStmtClass() != Stmt::ConditionalOperatorClass)
				return true;

			// duplicate statement tree
			iStmt->setThen( dupStmtTree( pParams->m_iDupExpr, iStmt->getThen().asExpr() ));
			iStmt->setElse( dupStmtTree( pParams->m_iDupExpr, iStmt->getElse().asExpr() ));

			if (isLoa) {
				iStmt->setStmtClass( Stmt::IfStmtClass );
				iStmt->isAssignStmt( true );
			}

			pParams->m_iDupExpr.replace( iStmt.asStmt() );

			WalkParams thenParams( iStmt->getThen() );
			walkStmtTree( HtStmtIter(iStmt->getThen()), iInsert, isLoa, & thenParams );

			WalkParams elseParams( iStmt->getElse() );
			walkStmtTree( HtStmtIter(iStmt->getElse()), iInsert, isLoa, & elseParams );

			iStmt++;
			return false;

		} else {
			if (iStmt->isAssignStmt())
				pParams->m_iAssignExpr = iStmt;

			if (iStmt->getStmtClass() != Stmt::MemberExprClass)
				return true;

			WalkParams walkBaseExprParams;
			walkBaseExprParams.m_isWalkBaseExpr = true;
			walkBaseExprParams.m_iDupExpr = isLoa ? pParams->m_iAssignExpr : iStmt;

			walkStmtTree( HtStmtIter(iStmt->getBase()), iInsert, isLoa, & walkBaseExprParams);

			iStmt++;
			return false;
		}
	}

	HtStmt * HtTransformConditionalBase::dupCallback( HtStmtIter & iSrcStmt, void * pVoid )
	{
		if (iSrcStmt->getStmtClass() == Stmt::ConditionalOperatorClass)
			return (HtStmt *)pVoid;
		else
			return 0;
	}

}
