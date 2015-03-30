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

	class HtDeadCodeElimination : HtWalkStmtTree
	{
	public:
		HtDeadCodeElimination(HtDesign * pHtDesign, ASTContext &Context)
			: m_pHtDesign(pHtDesign), m_context(Context), m_diags(Context.getDiagnostics()) {}

		void deadCodeElimination();

		bool walkCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid );
		void walkPostCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid ) { iStmt++; }

	private:
		HtDesign * m_pHtDesign;
		ASTContext & m_context;
		DiagnosticsEngine & m_diags;
	};

	void HtDesign::deadCodeElimination(ASTContext &Context)
	{
		HtDeadCodeElimination transform(this, Context);

		transform.deadCodeElimination();

		ExitIfError(Context);
	}

	void HtDeadCodeElimination::deadCodeElimination()
	{
		HtDeclIter methodIter(m_pHtDesign->getScMethodDeclList());
		while ( !methodIter.isEol() ) {

			HtStmtIter iStmt ( (*methodIter)->getBody() );

			walkStmtTree( iStmt, iStmt, false, 0 );

			methodIter++;
		}
	}

	bool HtDeadCodeElimination::walkCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid )
	{
		// check if an expression or statement can be reduced due to constant values

		if (iStmt->getStmtClass() == Stmt::IfStmtClass
			|| iStmt->getStmtClass() == Stmt::ConditionalOperatorClass)
		{
			if (!iStmt->getCond().asExpr()->isConstExpr())
				return true;

			if (iStmt->getCond().asExpr()->getBooleanLiteralAsValue())
				// cond == true
				iStmt.replace(iStmt->getThen().asStmt());
			else
				// cond == false
				iStmt.replace(iStmt->getElse().asStmt());
		}

		if (iStmt->getStmtClass() == Stmt::NullStmtClass
			&& iStmt.listSize() > 1)
		{
			// delete null statement
			HtStmtIter iOrigStmt = iStmt++;
			iOrigStmt.erase();

			walkCallback(iStmt, iInsert, isLoa, pVoid);
		}		

		return true;
	}
}
