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

// walk design and count variable and type references

namespace ht {

	class HtUpdateRefCounts : HtWalkStmtTree
	{
	public:
		HtUpdateRefCounts(HtDesign * pHtDesign, ASTContext &Context)
			: m_pHtDesign(pHtDesign), m_context(Context), m_diags(Context.getDiagnostics()) {}

		void updateRefCounts();
		void updateRefCounts(HtDecl * pHtDecl);

		bool walkCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid );
		void walkPostCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid ) { iStmt++; }

	private:
		HtDesign * m_pHtDesign;
		ASTContext & m_context;
		DiagnosticsEngine & m_diags;
	};

	void HtDesign::updateRefCounts(ASTContext &Context)
	{
		HtUpdateRefCounts transform(this, Context);

		transform.updateRefCounts();

		ExitIfError(Context);
	}

	void HtUpdateRefCounts::updateRefCounts()
	{
		// start with top level declarations
		HtDeclList_t & declList = m_pHtDesign->getTopDeclList();
		for (HtDeclList_iter_t declIter = declList.begin();
			declIter != declList.end(); declIter++)
		{
			updateRefCounts(*declIter);
		}
	}

	void HtUpdateRefCounts::updateRefCounts(HtDecl * pHtDecl)
	{
		// set reference counts to zero for declarations
		pHtDecl->clearRefCnt();

		switch (pHtDecl->getKind()) {
		case Decl::Typedef:
		case Decl::TemplateTypeParm:
		case Decl::Var:
		case Decl::Field:
		case Decl::IndirectField:
		case Decl::ParmVar:
		case Decl::NonTypeTemplateParm:
		case Decl::AccessSpec:
		case Decl::PragmaHtv:
			break;
		case Decl::CXXRecord:
			for (HtDeclList_iter_t declIter = pHtDecl->getSubDeclList().begin();
				declIter != pHtDecl->getSubDeclList().end(); declIter++)
			{
				updateRefCounts(*declIter );
			}
			break;
		case Decl::CXXConstructor:
		case Decl::CXXDestructor:
		case Decl::Function:
		case Decl::CXXMethod:
		case Decl::CXXConversion:
			for (HtDeclList_iter_t declIter = pHtDecl->getParamDeclList().begin();
				declIter != pHtDecl->getParamDeclList().end(); declIter++)
			{
				updateRefCounts(*declIter);
			}
			if (pHtDecl->doesThisDeclarationHaveABody() && pHtDecl->hasBody()) {
				HtStmtIter iStmt ( pHtDecl->getBody() );
				walkStmtTree( iStmt, iStmt, false, 0 );
			}
			break;
		case Decl::ClassTemplate:
		case Decl::FunctionTemplate:
			for (HtDeclList_iter_t declIter = pHtDecl->getParamDeclList().begin();
				declIter != pHtDecl->getParamDeclList().end(); declIter++)
			{
				updateRefCounts(*declIter);
			}
			updateRefCounts(pHtDecl->getTemplateDecl());
			break;
		case Decl::Enum:
			for (HtDeclList_iter_t declIter = pHtDecl->getSubDeclList().begin();
				declIter != pHtDecl->getSubDeclList().end(); declIter++) 
			{
				updateRefCounts(*declIter );
			}
			break;
		case Decl::EnumConstant:
			assert(0);
			break;
		default:
			assert(0);
		}
	}

	bool HtUpdateRefCounts::walkCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid )
	{
		// update ref counts for declarations
		Stmt::StmtClass stmtClass = iStmt->getStmtClass();

		switch (stmtClass) {
		case Stmt::IfStmtClass:
		case Stmt::CompoundStmtClass:
		case Stmt::BinaryOperatorClass:
		case Stmt::CXXBoolLiteralExprClass:
		case Stmt::IntegerLiteralClass:
		case Stmt::ImplicitCastExprClass:
		case Stmt::CXXStaticCastExprClass:
		case Stmt::CXXThisExprClass:
		case Stmt::ParenExprClass:
			break;
		case Stmt::DeclRefExprClass:
			{
				HtExpr * pExpr = iStmt.asExpr();
				if (isLoa)
					pExpr->getRefDecl()->incWrRefCnt();
				else
					pExpr->getRefDecl()->incRdRefCnt();

				if (pExpr->getRefDecl()->getType() && pExpr->getRefDecl()->getType()->getHtDecl())
					pExpr->getRefDecl()->getType()->getHtDecl()->incRdRefCnt();
			}
			break;
		case Stmt::MemberExprClass:
			{
				HtExpr * pExpr = iStmt.asExpr();
				if (isLoa)
					pExpr->getMemberDecl()->incWrRefCnt();
				else
					pExpr->getMemberDecl()->incRdRefCnt();

				if (pExpr->getMemberDecl()->getType() && pExpr->getMemberDecl()->getType()->getHtDecl())
					pExpr->getMemberDecl()->getType()->getHtDecl()->incRdRefCnt();
			}
			break;
		case Stmt::CXXConstructExprClass:
			{
				HtExpr * pExpr = iStmt.asExpr();
				pExpr->getConstructorDecl()->incRdRefCnt();
			}
			break;
		case Stmt::CXXMemberCallExprClass:
			{
				HtExpr * pExpr = iStmt.asExpr();
				pExpr->getMemberDecl()->incRdRefCnt();
			}
			break;
		case Stmt::CallExprClass:
		case Stmt::CXXOperatorCallExprClass:
			{
				HtExpr * pExpr = iStmt.asExpr();
				pExpr->getCalleeDecl()->incRdRefCnt();
			}
			break;
		default:
			assert(0);
		}

		return true;
	}

}
