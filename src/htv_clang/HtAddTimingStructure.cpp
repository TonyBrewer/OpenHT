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

	class HtPathDst;

	class HtAddTimingPathLinks : HtWalkStmtTree
	{
	public:
		HtAddTimingPathLinks(HtDesign * pHtDesign, ASTContext &Context)
			: m_pHtDesign(pHtDesign), m_context(Context), m_diags(Context.getDiagnostics()) {}

		void addTimingPathLinks();

		bool walkCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid );
		void walkPostCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid ) { iStmt++; }

	private:
		HtDesign * m_pHtDesign;
		ASTContext & m_context;
		DiagnosticsEngine & m_diags;
	};

	void HtDesign::addTimingPathLinks(ASTContext &Context)
	{
		HtAddTimingPathLinks transform(this, Context);

		transform.addTimingPathLinks();

		ExitIfError(Context);
	}

	void HtAddTimingPathLinks::addTimingPathLinks()
	{
		HtDeclIter methodIter(m_pHtDesign->getScMethodDeclList());
		while ( !methodIter.isEol() ) {

			HtStmtIter iStmt ( (*methodIter)->getBody() );

			walkStmtTree( iStmt, iStmt, false, 0 );

			methodIter++;
		}
	}

	bool HtAddTimingPathLinks::walkCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid )
	{
		if (iStmt->getStmtClass() == Stmt::BinaryOperatorClass && iStmt.asExpr()->getBinaryOperator() == BO_Assign
			|| iStmt->getStmtClass() == Stmt::CompoundAssignOperatorClass)
		{
			return true;
		}

		if (pathDst.getListIdx() >= 0) {
			if (iStmt->getPath() == 0)
				iStmt->setPath(new HtPath(iStmt->getLineInfo()));

			HtPath * pPath = iStmt->getPath();

			if (isLoa) {
				// assignment or left of assignment, path points toward leaf
				pPath->addSrc(pathDst.getStmtIter());
				pPath->addDst(HtPathDst(iStmt, -1));

			} else {
				pPath->addSrc(iStmt);
				pPath->addDst(pathDst);
			}

			if (iStmt->getStmtClass() == Stmt::DeclRefExprClass || iStmt->getStmtClass() == Stmt::MemberExprClass) {

				HtDecl * pDecl = iStmt->getStmtClass() == Stmt::DeclRefExprClass
					? iStmt.asExpr()->getRefDecl() : iStmt.asExpr()->getMemberDecl();

				if (pDecl->getPath() == 0)
					pDecl->setPath(new HtPath(iStmt->getLineInfo()));

				HtPath * pDeclPath = pDecl->getPath();
				if (isLoa) {	// left of assignment, src
					pDeclPath->addSrc(iStmt);
				} else {		// right of assignment, dst
					pDeclPath->addDst(HtPathDst(iStmt, -1));
				}
				bool stop = true;
			}
		}

		return true;
	}

}
