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

	class HtFoldConstantExpr : HtWalkStmtTree
	{
	public:
		HtFoldConstantExpr(HtDesign * pHtDesign, ASTContext &Context)
			: m_pHtDesign(pHtDesign), m_context(Context), m_diags(Context.getDiagnostics()) {}

		void foldConstantExpr();

		bool walkCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid );
		void walkPostCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid ) { iStmt++; }

	private:
		HtDesign * m_pHtDesign;
		ASTContext & m_context;
		DiagnosticsEngine & m_diags;
	};

	void HtDesign::foldConstantExpr(ASTContext &Context)
	{
		HtFoldConstantExpr transform(this, Context);

		transform.foldConstantExpr();

		ExitIfError(Context);
	}

	void HtFoldConstantExpr::foldConstantExpr()
	{
		HtDeclIter methodIter(m_pHtDesign->getScMethodDeclList());
		while ( !methodIter.isEol() ) {

			HtStmtIter iStmt ( (*methodIter)->getBody() );

			walkStmtTree( iStmt, iStmt, false, 0 );

			methodIter++;
		}
	}

	bool HtFoldConstantExpr::walkCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid )
	{
		// check if an expression can be replaced with a single contant value

		if (!iStmt->isExpr())
			return true;

		HtExpr * pExpr = iStmt.asExpr();

		if (!pExpr->isConstExpr())
			return true;

		Stmt::StmtClass stmtClass = pExpr->getStmtClass();
		if (stmtClass == Stmt::IntegerLiteralClass
			|| stmtClass == Stmt::FloatingLiteralClass)
		{
			iStmt++;
			return false;
		}

		// found one, reduce to a literal class
		if (pExpr->getEvalResult().Val.isInt()) {
			HtExpr * pNewExpr = new HtExpr( Stmt::IntegerLiteralClass, pExpr->getQualType(), pExpr->getLineInfo());
			pNewExpr->setIntegerLiteral(pExpr->getEvalResult().Val);
			pNewExpr->isConstExpr( true );

			iStmt.replace(pNewExpr);
		} else if (pExpr->getEvalResult().Val.isLValue()
			|| pExpr->getEvalResult().Val.isStruct()
			|| pExpr->getEvalResult().Val.isUnion()) {
			return true;
		} else
			assert(0);

		iStmt++;
		return false;
	}
}
