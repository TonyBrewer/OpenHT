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

	class HtInsertTempVarPrePostIncDec : HtWalkStmtTree
	{
	public:
		HtInsertTempVarPrePostIncDec(HtDesign * pHtDesign, ASTContext &Context)
			: m_pHtDesign(pHtDesign), m_context(Context), m_diags(Context.getDiagnostics()) {}

		void insertTempVarPrePostIncDec();

		bool walkCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid );
		void walkPostCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid ) { iStmt++; }

	private:
		HtDesign * m_pHtDesign;
		ASTContext & m_context;
		DiagnosticsEngine & m_diags;
	};

	void HtDesign::insertTempVarPrePostIncDec(ASTContext &Context)
	{
		HtInsertTempVarPrePostIncDec transform(this, Context);

		transform.insertTempVarPrePostIncDec();

		ExitIfError(Context);
	}

	void HtInsertTempVarPrePostIncDec::insertTempVarPrePostIncDec()
	{
		HtDeclIter methodIter(m_pHtDesign->getScMethodDeclList());
		while ( !methodIter.isEol() ) {

			HtStmtIter iStmt ( (*methodIter)->getBody() );

			walkStmtTree( iStmt, iStmt, false, 0 );

			methodIter++;
		}
	}

	bool HtInsertTempVarPrePostIncDec::walkCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid )
	{
		// find pre/post inc/dec replace with a temp var and
		//   insert term temp var assignment

		if (iStmt->getStmtClass() == Stmt::UnaryOperatorClass
			&& iStmt != iInsert // ignore var++; type statements
			&& (iStmt.asExpr()->getUnaryOperator() == UO_PostInc ||
				iStmt.asExpr()->getUnaryOperator() == UO_PostDec ||
				iStmt.asExpr()->getUnaryOperator() == UO_PreInc ||
				iStmt.asExpr()->getUnaryOperator() == UO_PreInc))
		{
			// declare temp variable
			HtDecl * pDecl = new HtDecl( Decl::Var, iStmt->getLineInfo() );
			pDecl->setType( iStmt.asExpr()->getQualType().getType() );
			pDecl->isTempVar( true );

			pDecl->setName( m_pHtDesign->findUniqueName( "pp", iStmt->getLineInfo() ));

			pDecl->setInitExpr( iStmt.asExpr() );

			HtStmt * pStmt = new HtStmt( Stmt::DeclStmtClass, iStmt->getLineInfo() );
			pStmt->addDecl( pDecl );

			iInsert.insert( pStmt );

			// now set the temp as the array index
			HtExpr * pExpr = new HtExpr( Stmt::DeclRefExprClass, HtQualType(pDecl->getType()), pDecl->getLineInfo() );
			pExpr->setRefDecl( pDecl );

			iStmt.replace( pExpr );

			iStmt++;
			return false;
		}

		return true;
	}

}
