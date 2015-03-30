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

	void HtWalkStmtTree::walkStmtTree( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid )
	{ // isLoa - is left of assignment
		if (iStmt.isEol())
			return;

		HtStmtIter iNextStmt = iStmt+1;

		bool bWalkSubStmts = walkCallback( iStmt, iInsert, isLoa, pVoid );
		if (!bWalkSubStmts)
			bool stop = true;

		// walk sub statements
		if (bWalkSubStmts) {
			switch (iStmt->getStmtClass()) {
			case Stmt::CompoundStmtClass:
				for (HtStmtIter iSubStmt(iStmt->getStmtList()); !iSubStmt.isEol(); ) {
					walkStmtTree( iSubStmt, iSubStmt, false, pVoid );
				}
				break;
			case Stmt::ForStmtClass:
				walkStmtTree( HtStmtIter(iStmt->getInit()), iStmt, false, pVoid );
				walkStmtTree( HtStmtIter(iStmt->getCond()), iStmt, false, pVoid );
				walkStmtTree( HtStmtIter(iStmt->getInc()), iStmt, false, pVoid );
				walkStmtTree( HtStmtIter(iStmt->getBody()), iStmt, false, pVoid );
				break;
			case Stmt::SwitchStmtClass:
				walkStmtTree( HtStmtIter(iStmt->getCond()), iStmt, false, pVoid );
				walkStmtTree( HtStmtIter(iStmt->getBody()), iStmt, false, pVoid );
				break;
			case Stmt::CaseStmtClass:
				walkStmtTree( HtStmtIter(iStmt->getLhs()), iStmt, false, pVoid );
				walkStmtTree( HtStmtIter(iStmt->getSubStmt()), iStmt, false, pVoid );
				break;
			case Stmt::DefaultStmtClass:
				walkStmtTree( HtStmtIter(iStmt->getSubStmt()), iStmt, false, pVoid );
				break;
			case Stmt::IfStmtClass:
				walkStmtTree( HtStmtIter(iStmt->getCond()), iStmt, false, pVoid );
				walkStmtTree( HtStmtIter(iStmt->getThen()), HtStmtIter(iStmt->getThen()), false, pVoid );
				walkStmtTree( HtStmtIter(iStmt->getElse()), HtStmtIter(iStmt->getElse()), false, pVoid );
				break;
			case Stmt::DeclStmtClass:
				for (HtDeclIter declIter(iStmt->getDeclList()); !declIter.isEol(); declIter++ ) {
					if (declIter->hasInitExpr())
						walkStmtTree( HtStmtIter(declIter->getInitExpr()), iStmt, false, pVoid );
				}
				break;
			case Stmt::BreakStmtClass:
			case Stmt::NullStmtClass:
				break;

			// Expressions
			case Stmt::ConditionalOperatorClass:
				walkStmtTree( HtStmtIter(iStmt->getCond()), iInsert, isLoa, pVoid );
				walkStmtTree( HtStmtIter(iStmt->getThen()), iInsert, isLoa, pVoid );
				walkStmtTree( HtStmtIter(iStmt->getElse()), iInsert, isLoa, pVoid );
				break;
			case Stmt::BinaryOperatorClass:
				if (iStmt.asExpr()->getBinaryOperator() == BO_Assign) {
					walkStmtTree( HtStmtIter(iStmt->getRhs()), iInsert, false, pVoid );
					walkStmtTree( HtStmtIter(iStmt->getLhs()), iInsert, true, pVoid );
				} else {
					walkStmtTree( HtStmtIter(iStmt->getLhs()), iInsert, isLoa, pVoid );
					walkStmtTree( HtStmtIter(iStmt->getRhs()), iInsert, isLoa, pVoid );
				}
				break;
			case Stmt::CompoundAssignOperatorClass:
				walkStmtTree( HtStmtIter(iStmt->getLhs()), iInsert, true, pVoid );
				walkStmtTree( HtStmtIter(iStmt->getRhs()), iInsert, isLoa, pVoid );
				break;
			case Stmt::ParenExprClass:
			case Stmt::UnaryOperatorClass:
			case Stmt::CStyleCastExprClass:
			case Stmt::CXXFunctionalCastExprClass:
			case Stmt::ImplicitCastExprClass:
			case Stmt::CXXStaticCastExprClass:
			case Stmt::MaterializeTemporaryExprClass:
				walkStmtTree( HtStmtIter(iStmt->getSubExpr()), iInsert, isLoa, pVoid );
				break;
			case Stmt::MemberExprClass:
				walkStmtTree( HtStmtIter(iStmt->getBase()), iInsert, isLoa, pVoid );
				break;
			case Stmt::StringLiteralClass:
			case Stmt::IntegerLiteralClass:
			case Stmt::CXXThisExprClass:
			case Stmt::DeclRefExprClass:
			case Stmt::CXXBoolLiteralExprClass:
				break;
			case Stmt::ArraySubscriptExprClass:
				walkStmtTree( HtStmtIter(iStmt->getBase()), iInsert, isLoa, pVoid );
				walkStmtTree( HtStmtIter(iStmt->getIdx()), iInsert, false, pVoid );
				break;
			case Stmt::CXXConstructExprClass:
				{
					HtDeclList_iter_t paramIter = iStmt.asExpr()->getConstructorDecl()->getParamDeclList().begin();
					for (HtStmtIter argExprIter(iStmt.asExpr()->getArgExprList()); !argExprIter.isEol(); paramIter++)
						walkStmtTree( argExprIter, iInsert, isParamTypeLoa((*paramIter)->getQualType()), pVoid );
				}
				break;
			case Stmt::CXXOperatorCallExprClass:
				{
					// first walk base expression
					HtStmtIter argExprIter(iStmt.asExpr()->getArgExprList());
					walkStmtTree( argExprIter, iInsert, false, pVoid );

					// now walk arguments
					HtDeclIter paramIter = iStmt.asExpr()->getCalleeDecl()->getParamDeclList();
					for ( ; !argExprIter.isEol(); paramIter++)
						walkStmtTree( argExprIter, iInsert, isParamTypeLoa((*paramIter)->getQualType()), pVoid );
				}
				break;
			case Stmt::CXXMemberCallExprClass:
				{
					walkStmtTree( HtStmtIter(iStmt->getCalleeExpr()), iInsert, isLoa, pVoid );

					HtDeclList_iter_t paramIter = iStmt.asExpr()->getMemberDecl()->getParamDeclList().begin();
					for (HtStmtIter argExprIter(iStmt.asExpr()->getArgExprList()); !argExprIter.isEol(); paramIter++)
						walkStmtTree( argExprIter, iInsert, isParamTypeLoa((*paramIter)->getQualType()), pVoid );
				}
				break;
			case Stmt::InitListExprClass:
				{
					for (HtStmtIter initExprIter(iStmt.asExpr()->getInitExprList()); !initExprIter.isEol(); )
						walkStmtTree( initExprIter, iInsert, isLoa, pVoid );
				}
				break;
			case Stmt::CallExprClass:
				{
					HtDeclList_iter_t paramIter = iStmt.asExpr()->getCalleeDecl()->getParamDeclList().begin();
					for (HtStmtIter argExprIter(iStmt.asExpr()->getArgExprList()); !argExprIter.isEol(); paramIter++)
						walkStmtTree( argExprIter, iInsert, isParamTypeLoa((*paramIter)->getQualType()), pVoid );
				}
				break;
			case Stmt::CXXTemporaryObjectExprClass:
				break;
			default:
				assert(0);
			}
		}

		walkPostCallback( iStmt, iInsert, isLoa, pVoid );

		iStmt = iNextStmt;
	}

	bool HtWalkStmtTree::isParamTypeLoa(HtQualType qualType)
	{
		if (isParamTypeConst(qualType))
			return false;	// const are roa

		// check if parameter decleration is an input or output
		for (;;) {
			switch (qualType.getType()->getTypeClass()) {
			case Type::LValueReference:
				return true;	// lvalues are loa
				break;
			case Type::TemplateSpecialization:
			case Type::Builtin:
			case Type::Record:
				return false;	// builtin types, struct and templates are roa
			default:
				assert(0);
			}
		}
	}

	bool HtWalkStmtTree::isParamTypeConst(HtQualType qualType)
	{
		// check if parameter decleration is an input or output
		for (;;) {
			if (qualType.isConst())
				return true;

			switch (qualType.getType()->getTypeClass()) {
			case Type::LValueReference:
				return isParamTypeConst(qualType.getType()->getQualType());
				break;
			case Type::TemplateSpecialization:
			case Type::Builtin:
			case Type::Record:
				return false;	// builtin types, struct and templates are roa
			default:
				assert(0);
			}
		}
	}

}
