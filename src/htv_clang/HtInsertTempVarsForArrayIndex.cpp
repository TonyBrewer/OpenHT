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

	class HtInsertTempVarForArrayIndex : HtWalkStmtTree
	{
	public:
		HtInsertTempVarForArrayIndex(HtDesign * pHtDesign, ASTContext &Context)
			: m_pHtDesign(pHtDesign), m_context(Context), m_diags(Context.getDiagnostics()) {}

		void insertTempVarForArrayIndex();

		bool walkCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid );
		void walkPostCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid ) { iStmt++; }

	private:
		HtDesign * m_pHtDesign;
		ASTContext & m_context;
		DiagnosticsEngine & m_diags;
	};

	void HtDesign::insertTempVarForArrayIndex(ASTContext &Context)
	{
		HtInsertTempVarForArrayIndex transform(this, Context);

		transform.insertTempVarForArrayIndex();

		ExitIfError(Context);
	}

	void HtInsertTempVarForArrayIndex::insertTempVarForArrayIndex()
	{
		HtDeclIter methodIter(m_pHtDesign->getScMethodDeclList());
		while ( !methodIter.isEol() ) {

			HtStmtIter iStmt ( (*methodIter)->getBody() );

			walkStmtTree( iStmt, iStmt, false, 0 );

			methodIter++;
		}
	}

	bool HtInsertTempVarForArrayIndex::walkCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid )
	{
		// find array subscript operator
		//   if subscript expression is not a constant value then
		//    insert a temp variable.

		if (iStmt->getStmtClass() == Stmt::ArraySubscriptExprClass) {
			HtStmtIter idxExpr = iStmt->getIdx();

			if (!idxExpr.asExpr()->isConstExpr()) {

				// found an array index that is not a const value

				// declare temp variable
				HtDecl * pDecl = new HtDecl( Decl::Var, idxExpr->getLineInfo() );
				pDecl->setType( idxExpr.asExpr()->getQualType().getType() );
				pDecl->isTempVar( true );

				pDecl->setName( m_pHtDesign->findUniqueName( "ai", idxExpr->getLineInfo() ));

				pDecl->setInitExpr( idxExpr.asExpr() );

				HtStmt * pStmt = new HtStmt( Stmt::DeclStmtClass, idxExpr->getLineInfo() );
				pStmt->addDecl( pDecl );

				iInsert.insert( pStmt );

				// now set the temp as the array index
				HtExpr * pExpr = new HtExpr( Stmt::DeclRefExprClass, pDecl->getQualType(), idxExpr->getLineInfo() );
				pExpr->setRefDecl( pDecl );

				idxExpr.replace( pExpr );

				iStmt++;
				return false;
			}
		}

		return true;
	}

}
