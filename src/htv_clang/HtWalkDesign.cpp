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
	void HtWalkDesign::walkDesign(HtDesign * pHtDesign)
	{
		// start with top level declarations
		HtDecl ** ppHtDecl = pHtDesign->getTopDeclList().getPFirst();
		while (*ppHtDecl)
		{
			HtDecl * pOrigDecl = *ppHtDecl;

			handleTopDecl(ppHtDecl);
			walkDecl(ppHtDecl);

			// stay on current declaration if original was deleted
			if (*ppHtDecl == pOrigDecl)
				ppHtDecl = (*ppHtDecl)->getPNextDecl();
		}
	}

	void HtWalkDesign::walkDecl(HtDecl ** ppHtDecl)
	{
		HtDecl * pHtDecl = * ppHtDecl;

		switch ((*ppHtDecl)->getKind()) {
		case Decl::CXXMethod:
			handleCXXMethodDecl(ppHtDecl);
			if ((*ppHtDecl)->hasBody())
				walkStmt((*ppHtDecl)->getPBody());
			break;
		case Decl::Function:
			handleFunctionDecl(ppHtDecl);
			if (*ppHtDecl != pHtDecl) break;	// check if original node was deleted

			if ((*ppHtDecl)->hasBody())
				walkStmt((*ppHtDecl)->getPBody());
			break;
		case Decl::CXXConstructor:
		case Decl::CXXDestructor:
		case Decl::CXXConversion:
			if ((*ppHtDecl)->hasBody())
				walkStmt((*ppHtDecl)->getPBody());
			break;
		case Decl::CXXRecord:
			{
				handleCXXRecordDecl(ppHtDecl);

				// walk members of record
				HtDecl ** ppHtSubDecl = (*ppHtDecl)->getPFirstSubDecl();
				while (*ppHtSubDecl)
				{
					HtDecl * pOrigDecl = *ppHtSubDecl;

					walkDecl(ppHtSubDecl);

					// stay on current declaration if original was deleted
					if (*ppHtSubDecl == pOrigDecl)
						ppHtSubDecl = (*ppHtSubDecl)->getPNextDecl();
				}
			}
			break;
		case Decl::Var:
		case Decl::Typedef:
		case Decl::Field:
		case Decl::IndirectField:
		case Decl::ParmVar:
		case Decl::ClassTemplate:
		case Decl::NonTypeTemplateParm:
		case Decl::TemplateTypeParm:
		case Decl::AccessSpec:
			break;
		default:
			assert(0);
		}
	}

	void HtWalkDesign::walkStmt(HtStmt ** ppHtStmt)
	{
		HtStmt * pHtStmt = *ppHtStmt;

		if (pHtStmt == 0)
			return;

		handleStmt( ppHtStmt );

		switch (pHtStmt->getStmtClass()) {
		case Stmt::DeclStmtClass:
			handleDeclStmt(ppHtStmt);
			if (*ppHtStmt != pHtStmt) break;

			for (HtDecl ** ppHtDecl = pHtStmt->getPFirstDecl(); *ppHtDecl; ppHtDecl = (*ppHtDecl)->getPNextDecl()) {
				walkDecl(ppHtDecl);
				if (*ppHtDecl == 0) break;
			}
			break;
		case Stmt::NullStmtClass:
			break;
		case Stmt::CompoundStmtClass:
			{
				HtStmt ** ppHtSubStmt = pHtStmt->getPFirstStmt();
				while (*ppHtSubStmt)
				{
					HtStmt * pOrigStmt = *ppHtSubStmt;

					walkStmt(ppHtSubStmt);

					// stay on current statement if original statement was deleted
					if (*ppHtSubStmt == pOrigStmt)
						ppHtSubStmt = (*ppHtSubStmt)->getPNextStmt();
				}
			}
			break;
		case Stmt::IfStmtClass:
			walkExpr(pHtStmt->getPCond());
			walkStmt(pHtStmt->getPThen());
			walkStmt(pHtStmt->getPElse());
			break;
		case Stmt::SwitchStmtClass:
			handleSwitchStmt(ppHtStmt);
			walkExpr(pHtStmt->getPCond());
			walkStmt(pHtStmt->getPBody());
			break;
		case Stmt::CaseStmtClass:
			walkExpr(pHtStmt->getPLHS());
			walkStmt(pHtStmt->getPSubStmt());
			break;
		case Stmt::DefaultStmtClass:
			walkStmt(pHtStmt->getPSubStmt());
			break;
		case Stmt::BreakStmtClass:
			break;
		case Stmt::ReturnStmtClass:
			walkExpr(pHtStmt->getPSubExpr());
			break;
		case Stmt::CXXOperatorCallExprClass:
		case Stmt::CXXMemberCallExprClass:
		case Stmt::ImplicitCastExprClass:
		case Stmt::CXXStaticCastExprClass:
		case Stmt::CStyleCastExprClass:
		case Stmt::MemberExprClass:
		case Stmt::CXXThisExprClass:
		case Stmt::BinaryOperatorClass:
		case Stmt::CXXBoolLiteralExprClass:
		case Stmt::IntegerLiteralClass:
		case Stmt::ParenExprClass:
		case Stmt::CallExprClass:
		case Stmt::CompoundAssignOperatorClass:
			assert(pHtStmt->isAssignStmt());
			walkExpr((HtExpr**)(ppHtStmt));
			break;
		default:
			assert(0);
		}
	}

	void HtWalkDesign::walkExpr(HtExpr ** ppHtExpr)
	{
	}
}
