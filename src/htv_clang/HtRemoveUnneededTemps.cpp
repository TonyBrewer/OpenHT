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

	// Remove temp variables that have a single load and the load is in an assignment statement
	// Also remove temp variables (and assignment statement) if temp is not used
	//   Assigned but not used temps are caused by function returns that are not used
	// Algorithm walks AST and marks temp variables that must be kept by marking temp
	//   variables used as if/switch conditional expressions. Once tree is walked, it is
	//   walked a second time to remove temps that were not marked
	// The algorithm is repeated until no temps can be removed. 

	class HtRemoveUnneededTemps : HtWalkStmtTree
	{
	public:
		HtRemoveUnneededTemps(HtDesign * pHtDesign, ASTContext &Context)
			: m_pHtDesign(pHtDesign), m_context(Context), m_diags(Context.getDiagnostics()) {}

		bool removeUnneededTemps();

		bool walkCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid );
		void walkPostCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid ) { /*iStmt++;*/ }

	private:
		HtDecl * findBaseDecl( HtStmtIter & iStmt );

	private:
		HtDesign * m_pHtDesign;
		ASTContext & m_context;
		DiagnosticsEngine & m_diags;

		bool m_bMarkTempPhase;
		bool m_bInIfSwitchCond;
	};

	void HtDesign::removeUnneededTemps(ASTContext &Context)
	{
		bool bRemovedTemp = true;
		while (bRemovedTemp) {
			HtRemoveUnneededTemps transform(this, Context);

			bRemovedTemp = transform.removeUnneededTemps();
		}

		ExitIfError(Context);
	}

	bool HtRemoveUnneededTemps::removeUnneededTemps()
	{
		m_bInIfSwitchCond = false;

		HtDeclIter methodIter(m_pHtDesign->getScMethodDeclList());
		while ( !methodIter.isEol() ) {

			m_bMarkTempPhase = true;
			HtStmtIter iStmt ( (*methodIter)->getBody() );
			walkStmtTree( iStmt, iStmt, false, 0 );

			m_bMarkTempPhase = false;
			HtStmtIter iStmt2 ( (*methodIter)->getBody() );
			walkStmtTree( iStmt2, iStmt2, false, 0 );

			methodIter++;
		}

		// now remove deleted field declarations
		bool bRemovedTemp = false;
		HtDeclIter iDecl(m_pHtDesign->getScModuleDeclList());
		while ( !iDecl.isEol() ) {
			HtDecl * pHtDecl = *iDecl;

			if (pHtDecl->isTempVar() && !pHtDecl->isKeepTempVar()) {
				HtDeclIter iOrigDecl = iDecl++;
				iOrigDecl.erase();
				bRemovedTemp = true;
			} else
				iDecl++;
		}

		return bRemovedTemp;
	}

	bool HtRemoveUnneededTemps::walkCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid )
	{
		if (m_bMarkTempPhase) {
			// first phase we mark temps that can not be removed

			switch (iStmt->getStmtClass()) {
			case Stmt::IfStmtClass:

				m_bInIfSwitchCond = true;
				walkStmtTree( HtStmtIter(iStmt->getCond()), iInsert, false, 0 );
				m_bInIfSwitchCond = false;

				walkStmtTree( HtStmtIter(iStmt->getThen()), iInsert, false, 0 );
				walkStmtTree( HtStmtIter(iStmt->getElse()), iInsert, false, 0 );

				iStmt++;
				return false;
			case Stmt::SwitchStmtClass:
				assert(0);
				break;
			case Stmt::MemberExprClass:
				HtDecl * pBaseDecl = findBaseDecl( iStmt );
				bool bBuiltinType = iStmt.asExpr()->getMemberDecl()->getDesugaredQualType().getType()->getTypeClass() == Type::Builtin;
				if (pBaseDecl->getName() == "value$2$59")
					bool stop = true;
				if (isLoa)
					pBaseDecl->isKeepTempVar(false);
				else
					pBaseDecl->isKeepTempVar(m_bInIfSwitchCond || !bBuiltinType);
				return false;
			}

		} else {
			// second phase we delete temp (and assignment to temp) that are not marked

			if (iStmt->isAssignStmt()) {
				if (iStmt->getStmtClass() == Stmt::BinaryOperatorClass &&
					iStmt.asExpr()->getBinaryOperator() == BO_Assign)
				{
					HtExpr * pLhs = iStmt.asExpr()->getLhs().asExpr();
					assert(pLhs->getStmtClass() == Stmt::MemberExprClass);
					HtDecl * pBaseDecl = findBaseDecl( HtStmtIter(iStmt.asExpr()->getLhs()) );

					if (pBaseDecl->isTempVar() && !pBaseDecl->isKeepTempVar()) {
						// found a temp to delete

						HtExpr * pRhs = iStmt.asExpr()->getRhs().asExpr();
						pBaseDecl->setSubstExpr(pRhs);

						HtStmtIter iOrigStmt = iStmt++;
						iOrigStmt.erase();

						return false;
					}
				}
			}

			if (!isLoa && iStmt->getStmtClass() == Stmt::MemberExprClass) {
				// check if temp substitution is needed
				HtDecl * pBaseDecl = findBaseDecl( iStmt );

				if (pBaseDecl->isTempVar() && !pBaseDecl->isKeepTempVar()) {
					iStmt.replace(pBaseDecl->getSubstExpr());
					walkStmtTree( HtStmtIter(iStmt), iInsert, isLoa, 0 );

					iStmt++;
					return false;
				}
			}
		}

		return true;
	}

	HtDecl * HtRemoveUnneededTemps::findBaseDecl( HtStmtIter & iStmt )
	{
		// find the base declaration for the reference
		HtStmtIter iStmt2 = iStmt;
		HtDecl * pBaseDecl = 0;
		for (;;) {
			Stmt::StmtClass stmtClass = iStmt2->getStmtClass();
			switch (stmtClass) {
			case Stmt::MemberExprClass:
				pBaseDecl = iStmt2.asExpr()->getMemberDecl();
				assert(pBaseDecl->getKind() == Decl::Field);
				iStmt2 = HtStmtIter(iStmt2.asExpr()->getBase());
				break;
			case Stmt::ImplicitCastExprClass:
			case Stmt::CXXStaticCastExprClass:
				iStmt2 = HtStmtIter(iStmt2.asExpr()->getSubExpr());
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
