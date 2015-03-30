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
#include "HtPrintDesign.h"

namespace ht {

	class HtDumpHtvAST : HtWalkStmtTree
	{
	public:
		HtDumpHtvAST(HtDesign * pHtDesign, ASTContext &Context)
			: m_pHtDesign(pHtDesign), m_context(Context), m_diags(Context.getDiagnostics()) {}

		void dumpHtvAST();
		void dump(HtDecl * pDecl);

		bool walkCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid );
		void walkPostCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid ) { iStmt++; }

	private:
		HtDesign * m_pHtDesign;
		ASTContext & m_context;
		DiagnosticsEngine & m_diags;
		string m_indent;
	};

	void HtDesign::dumpHtvAST(ASTContext &Context)
	{
		HtDumpHtvAST transform(this, Context);

		transform.dumpHtvAST();

		ExitIfError(Context);
	}

	void HtDumpHtvAST::dumpHtvAST()
	{
		cout << "TranslationUnitDecl\n";
		m_indent += "  ";

		// start with top level declarations
		HtDeclList_t & declList = m_pHtDesign->getTopDeclList();
		for (HtDeclList_iter_t declIter = declList.begin();
			declIter != declList.end(); declIter++)
		{
			dump(*declIter);
		}
	}

	void HtDumpHtvAST::dump(HtDecl * pHtDecl)
	{
		HtPrintDesign printDesign(m_pHtDesign, m_context);

		// set reference counts to zero for declarations
		Decl::Kind declKind = pHtDecl->getKind();
		switch (declKind) {
		default:
			assert(0);
		case Decl::FunctionTemplate:
			cout << m_indent << "FunctionalTemplateDecl " << pHtDecl->getName() << "\n";
			m_indent += "| ";

			for (HtDeclList_iter_t declIter = pHtDecl->getParamDeclList().begin();
				declIter != pHtDecl->getParamDeclList().end(); declIter++)
			{
				dump(*declIter);
			}

			dump(pHtDecl->getTemplateDecl());
			m_indent.erase(m_indent.length()-2);
			break;
		case Decl::CXXRecord:
			cout << m_indent << "CXXRecordDecl " << pHtDecl->getTagKindAsStr() << " " << pHtDecl->getName();
			if (!pHtDecl->isCompleteDefinition()) {
				cout << "\n";
				break;
			}

			cout << " definition\n";
			m_indent += "| ";

			if (pHtDecl->getType()->isPackedAttr())
				cout << m_indent << "PackedAttr\n";

			for (HtBaseSpecifier * pBaseSpec = pHtDecl->getFirstBaseSpecifier();
				pBaseSpec; pBaseSpec = pBaseSpec->getNextBaseSpec()) 
			{
				cout << m_indent;

				if (pBaseSpec->isVirtual())
					cout << "virtual ";

				cout << pBaseSpec->getAccessSpecName();

				cout << pBaseSpec->getQualType().getType()->getHtDecl()->getName() << "\n";
			}

			for (HtDeclList_iter_t declIter = pHtDecl->getSubDeclList().begin();
				declIter != pHtDecl->getSubDeclList().end(); declIter++)
			{
				dump( *declIter );
			}

			m_indent.erase(m_indent.length()-2);
			break;
		case Decl::ParmVar:
			cout << m_indent << "ParmVarDecl ";
			printDesign.printQualType(pHtDecl->getQualType(), pHtDecl->getName());
			cout << "\n";
			break;
		case Decl::Var:
			cout << m_indent << "VarDecl ";
			printDesign.printQualType(pHtDecl->getQualType(), pHtDecl->getName());
			cout << "\n";
			if (pHtDecl->hasInitExpr()) {
				m_indent += "  ";
				walkCallback( HtStmtIter(pHtDecl->getInitExpr()), HtStmtIter(), false, 0 );
				m_indent.erase(m_indent.length()-2);
			}
			break;
		case Decl::Field:
			cout << m_indent << "FieldDecl ";
			printDesign.printQualType(pHtDecl->getQualType(), pHtDecl->getName());
			if (pHtDecl->hasFieldBitWidth()) {
				cout << " : " << pHtDecl->getFieldBitWidth();
			}
			cout << "\n";
			if (pHtDecl->hasInitExpr()) {
				m_indent += "  ";
				walkCallback( HtStmtIter(pHtDecl->getInitExpr()), HtStmtIter(), false, 0 );
				m_indent.erase(m_indent.length()-2);
			}
			break;
		case Decl::IndirectField:
			cout << m_indent << "IndirectFieldDecl " << pHtDecl->getName() << "\n";
			break;
		case Decl::ClassTemplate:
			cout << m_indent << "ClassTemplateDecl " << pHtDecl->getName() << "\n";
			m_indent += "| ";

			for (HtDeclList_iter_t declIter = pHtDecl->getParamDeclList().begin();
				declIter != pHtDecl->getParamDeclList().end(); declIter++)
			{
				dump(*declIter);
			}

			dump(pHtDecl->getTemplateDecl());
			m_indent.erase(m_indent.length()-2);
			break;
		case Decl::TemplateTypeParm:
			cout << m_indent << "TemplateTypeParmDecl " << (pHtDecl->wasDeclaredWithTypename() ? "typename " : "class ") << pHtDecl->getName() << "\n";
			if (pHtDecl->hasDefaultArgument()) {
				cout << m_indent << "  " << "Default ";
				printDesign.printQualType( pHtDecl->getDefaultArgQualType() );
				cout << "\n";
			}
			break;
		case Decl::AccessSpec:
			cout << m_indent << "AccessSpecDecl " << pHtDecl->getAccessName() << "\n";
			break;
		case Decl::Typedef:
			cout << m_indent << "TypedefDecl ";
			printDesign.printQualType(pHtDecl->getQualType());
			cout << " " << pHtDecl->getName() << "\n";
			break;
		case Decl::NonTypeTemplateParm:
			cout << m_indent << "NonTypeTemplateParmDecl ";
			printDesign.printQualType(pHtDecl->getQualType());
			cout << " " << pHtDecl->getName() << "\n";
			break;
		case Decl::Function:
		case Decl::CXXMethod:
		case Decl::CXXConversion:
		case Decl::CXXConstructor:
		case Decl::CXXDestructor:
			switch (pHtDecl->getKind()) {
			case Decl::CXXConversion:
				cout << m_indent << "CXXConversionDecl ";
				cout << "operator ";
				printDesign.printQualType(pHtDecl->getQualType());
				break;
			case Decl::CXXMethod:
				cout << m_indent << "CXXMethodDecl ";
				if (pHtDecl->isDeclaredInMethod())
					printDesign.printQualType(pHtDecl->getQualType(), pHtDecl->getName());
				else
					printDesign.printQualType(pHtDecl->getQualType(), pHtDecl->getParentDecl()->getName() + "::" + pHtDecl->getName());
				break;
			case Decl::Function:
				cout << m_indent << "FunctionDecl ";
				printDesign.printQualType(pHtDecl->getQualType(), pHtDecl->getName());
				break;
			case Decl::CXXConstructor:
				cout << m_indent << "CXXConstructor " << pHtDecl->getName();
				break;
			case Decl::CXXDestructor:
				cout << m_indent << "CXXDestructor " << pHtDecl->getName();
				break;
			}
			cout << "\n";
			m_indent += "  ";

			if (pHtDecl->getName() == "Mul")
				bool stop = true;

			for (HtDeclList_iter_t declIter = pHtDecl->getParamDeclList().begin();
				declIter != pHtDecl->getParamDeclList().end(); declIter++)
			{
				dump(*declIter);
			}

			if (pHtDecl->doesThisDeclarationHaveABody() && pHtDecl->hasBody()) {
				walkCallback( HtStmtIter(pHtDecl->getBody()), HtStmtIter(), false, 0 );
			}
			m_indent.erase(m_indent.length()-2);
			break;
		case Decl::PragmaHtv:
			cout << m_indent << "PragmaHtvDecl " << HtDesign::getSourceLine( m_context, pHtDecl->getLoc() ) << '\n';
			break;
		//case Decl::FunctionTemplate:
		//case Decl::Enum:
		//case Decl::EnumConstant:
		}
	}

	bool HtDumpHtvAST::walkCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid )
	{
		HtPrintDesign printDesign(m_pHtDesign, m_context);

		// update ref counts for declarations
		Stmt::StmtClass stmtClass = iStmt->getStmtClass();

		switch (stmtClass) {
		default:
			assert(0);
		case Stmt::CompoundStmtClass:
			cout << m_indent << "CompoundStmt\n";
			m_indent += "| ";
			for (HtStmtIter subStmtIter(iStmt->getStmtList()); !subStmtIter.isEol(); subStmtIter++)
			{
				walkCallback( subStmtIter, HtStmtIter(), false, 0 );
			}
			m_indent.erase(m_indent.length()-2);
			break;
		case Stmt::UnaryOperatorClass:
			cout << m_indent << "UnaryOperator '" << iStmt.asExpr()->getOpcodeAsStr() << "'\n";
			m_indent += "  ";
			walkCallback( HtStmtIter(iStmt.asExpr()->getSubExpr()), HtStmtIter(), false, 0 );
			m_indent.erase(m_indent.length()-2);
			break;
		case Stmt::BinaryOperatorClass:
			cout << m_indent << "BinaryOperator '" << iStmt.asExpr()->getOpcodeAsStr() << "'\n";
			m_indent += "| ";
			walkCallback( HtStmtIter(iStmt.asExpr()->getLhs()), HtStmtIter(), false, 0 );
			walkCallback( HtStmtIter(iStmt.asExpr()->getRhs()), HtStmtIter(), false, 0 );
			m_indent.erase(m_indent.length()-2);
			break;
		case Stmt::DeclRefExprClass:
			{
				HtDecl * pRefDecl = iStmt.asExpr()->getRefDecl();
				cout << m_indent << "DeclRefExpr "; 
				printDesign.printQualType(pRefDecl->getQualType(), pRefDecl->getName());
				cout << "\n";
			}
			break;
		case Stmt::CXXBoolLiteralExprClass:
			cout << m_indent << "CXXBoolLiteralExpr " << iStmt.asExpr()->getBooleanLiteralAsStr() << "\n";
			break;
		case Stmt::ImplicitCastExprClass:
			cout << m_indent << "ImplicitCastExpr ";
			printDesign.printQualType(iStmt.asExpr()->getQualType(), string());
			cout << "\n";
			m_indent += "  ";
			walkCallback( HtStmtIter(iStmt.asExpr()->getSubExpr()), HtStmtIter(), false, 0 );
			m_indent.erase(m_indent.length()-2);
			break;
		case Stmt::CXXStaticCastExprClass:
			cout << m_indent << "CXXStaticCastExpr ";
			printDesign.printQualType(iStmt.asExpr()->getQualType(), string());
			cout << "\n";
			m_indent += "  ";
			walkCallback( HtStmtIter(iStmt.asExpr()->getSubExpr()), HtStmtIter(), false, 0 );
			m_indent.erase(m_indent.length()-2);
			break;
		case Stmt::MemberExprClass:
			cout << m_indent << "MemberExpr ";
			switch (iStmt.asExpr()->getMemberDecl()->getKind()) {
			case Decl::Field:
				cout << "(Field) ";
				break;
			case Decl::CXXMethod:
				cout << "(CXXMethod) ";
				break;
			case Decl::CXXConversion:
				cout << "(CXXConversion) ";
				break;
			default:
				assert(0);
			}
			printDesign.printQualType(iStmt.asExpr()->getQualType(), iStmt.asExpr()->getMemberDecl()->getName());
			cout << "\n";

			m_indent += "  ";
			walkCallback( HtStmtIter(iStmt.asExpr()->getBase()), HtStmtIter(), false, 0 );
			m_indent.erase(m_indent.length()-2);
			break;
		case Stmt::CXXThisExprClass:
			cout << m_indent << "CXXThisExpr\n";
			break;
		case Stmt::IfStmtClass:
			cout << m_indent << "IfStmt\n";
			m_indent += "  ";
			walkCallback( HtStmtIter(iStmt->getCond()), HtStmtIter(), false, 0 );
			walkCallback( HtStmtIter(iStmt->getThen()), HtStmtIter(), false, 0 );
			walkCallback( HtStmtIter(iStmt->getElse()), HtStmtIter(), false, 0 );
			m_indent.erase(m_indent.length()-2);
			break;
		case Stmt::CallExprClass:
			cout << m_indent << "CallExpr ";
			printDesign.printQualType(iStmt.asExpr()->getQualType(), string());
			cout << "\n";

			m_indent += "  ";
			walkCallback( HtStmtIter(iStmt.asExpr()->getCalleeExpr()), HtStmtIter(), false, 0 );

			for (HtStmtIter argExprIter(iStmt.asExpr()->getArgExprList()); !argExprIter.isEol(); )
			{
				walkCallback( argExprIter++, HtStmtIter(), false, 0 );
			}
			m_indent.erase(m_indent.length()-2);
			break;
		case Stmt::CXXOperatorCallExprClass:
			cout << m_indent << "CXXOperatorCallExpr ";
			printDesign.printQualType(iStmt.asExpr()->getQualType(), string());
			cout << "\n";

			m_indent += "  ";
			walkCallback( HtStmtIter(iStmt.asExpr()->getCalleeExpr()), HtStmtIter(), false, 0 );

			for (HtStmtIter argExprIter(iStmt.asExpr()->getArgExprList()); !argExprIter.isEol(); )
			{
				walkCallback( argExprIter++, HtStmtIter(), false, 0 );
			}
			m_indent.erase(m_indent.length()-2);
			break;
		case Stmt::ReturnStmtClass:
			cout << m_indent << "ReturnStmt\n";
			m_indent += "  ";
			walkCallback( HtStmtIter(iStmt->getSubExpr()), HtStmtIter(), false, 0 );
			m_indent.erase(m_indent.length()-2);
			break;
		case Stmt::MaterializeTemporaryExprClass:
			cout << m_indent << "MaterializeTemporaryExpr ";
			printDesign.printQualType(iStmt.asExpr()->getQualType(), string());
			cout << "\n";
			m_indent += "  ";
			walkCallback( HtStmtIter(iStmt.asExpr()->getSubExpr()), HtStmtIter(), false, 0 );
			m_indent.erase(m_indent.length()-2);
			break;
		case Stmt::CXXFunctionalCastExprClass:
			cout << m_indent << "CXXFunctionalCastExpr ";
			printDesign.printQualType(iStmt.asExpr()->getQualType(), string());
			cout << "\n";
			m_indent += "  ";
			walkCallback( HtStmtIter(iStmt.asExpr()->getSubExpr()), HtStmtIter(), false, 0 );
			m_indent.erase(m_indent.length()-2);
			break;
		case Stmt::CStyleCastExprClass:
			cout << m_indent << "CStyleCastExpr ";
			printDesign.printQualType(iStmt.asExpr()->getQualType(), string());
			cout << "\n";
			m_indent += "  ";
			walkCallback( HtStmtIter(iStmt.asExpr()->getSubExpr()), HtStmtIter(), false, 0 );
			m_indent.erase(m_indent.length()-2);
			break;
		case Stmt::CXXConstructExprClass:
			cout << m_indent << "CXXConstructExpr ";
			printDesign.printQualType(iStmt.asExpr()->getQualType(), string());
			cout << "\n";

			m_indent += "  ";
			//dump(iStmt.asExpr()->getConstructorDecl());

			for (HtStmtIter argExprIter(iStmt.asExpr()->getArgExprList()); !argExprIter.isEol(); )
			{
				walkCallback( argExprIter++, HtStmtIter(), false, 0 );
			}
			m_indent.erase(m_indent.length()-2);
			break;
		case Stmt::DeclStmtClass:
			cout << m_indent << "DeclStmt\n";
			m_indent += "  ";
			for (HtDeclList_iter_t declIter = iStmt->getDeclList().begin();
				declIter != iStmt->getDeclList().end(); declIter++ )
			{
				if (iStmt->isInitStmt())
					assert(0);
					//printInitDecl(indent, *declIter);
				else
					dump(*declIter);
			}
			m_indent.erase(m_indent.length()-2);
			break;
		case Stmt::CXXDependentScopeMemberExprClass:
			cout << m_indent << "DeclStmt\n";
			m_indent += "  ";
			walkCallback( HtStmtIter(iStmt.asExpr()->getSubExpr()), HtStmtIter(), false, 0 );
			m_indent.erase(m_indent.length()-2);
			break;
		case Stmt::CXXUnresolvedConstructExprClass:
			cout << m_indent << "CXXUnresolvedConstructExpr ";
			printDesign.printQualType(HtType::PreType, iStmt.asExpr()->getQualType());
			cout << "\n";
			m_indent += "  ";
			for (HtStmtIter argExprIter(iStmt.asExpr()->getArgExprList()); !argExprIter.isEol(); )
			{
				walkCallback( argExprIter++, HtStmtIter(), false, 0 );
			}
			m_indent.erase(m_indent.length()-2);
			break;
		case Stmt::ParenExprClass:
			cout << m_indent << "ParenExpr ";
			printDesign.printQualType(HtType::PreType, iStmt.asExpr()->getQualType());
			cout << "\n";
			m_indent += "  ";
			walkCallback( HtStmtIter(iStmt.asExpr()->getSubExpr()), HtStmtIter(), false, 0 );
			m_indent.erase(m_indent.length()-2);
			break;
		case Stmt::CXXMemberCallExprClass:
			cout << m_indent << "CXXMemberCallExpr ";
			printDesign.printQualType(iStmt.asExpr()->getQualType(), string());
			cout << "\n";
			m_indent += "  ";
			walkCallback( HtStmtIter(iStmt.asExpr()->getCalleeExpr()), HtStmtIter(), false, 0 );

			for (HtStmtIter argExprIter(iStmt.asExpr()->getArgExprList()); !argExprIter.isEol(); )
			{
				walkCallback( argExprIter++, HtStmtIter(), false, 0 );
			}
			m_indent.erase(m_indent.length()-2);
			break;
		case Stmt::IntegerLiteralClass:
			cout << m_indent << "IntegerLiteral " << iStmt.asExpr()->getIntegerLiteralAsStr() << "\n";
			break;
		//case Stmt::IntegerLiteralClass:
		}

		return true;
	}

}
