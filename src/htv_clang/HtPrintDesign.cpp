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

	void HtDesign::printDesign(ASTContext &Context)
	{
		HtPrintDesign transform( this, Context );

		transform.printDesign();
	}

	void HtPrintDesign::printDesign()
	{
		// print fileNum / pathName table
		HtLineInfo::HtNameMap_map & pathNameMap = HtLineInfo::getPathNameMap();
		string * pPathNameTbl = new string [pathNameMap.size()];

		for (HtLineInfo::HtNameMap_iter pathIter = pathNameMap.begin();
			pathIter != pathNameMap.end(); pathIter++)
		{
			pPathNameTbl[pathIter->second-1] = pathIter->first;
		}

		cout << "// List of fileNum / pathName used for temporary variables\n";

		for (size_t i = 0; i < pathNameMap.size(); i += 1)
			cout << "//  " << i+1 << "  " << pPathNameTbl[i] << "\n";

		cout << "\n";

		// Standard include files
		cout << "#define SC_NO_DATA_TYPES\n";
		cout << "#include <systemc.h>\n";
		cout << "\n";

		cout << "#define HT_NO_PRIVATE\n";
		cout << "#include <HtIntTypes.h>\n";
		cout << "\n";

		// start with top level declarations
		HtDeclList_t & declList = m_pHtDesign->getTopDeclList();
		for (HtDeclList_iter_t declIter = declList.begin();
			declIter != declList.end(); declIter++)
		{
			if ( !(*declIter)->getLineInfo().isInMainFile())
				continue;

			printDecl("", *declIter);
		}
	}

	void HtPrintDesign::printDecl(string indent, HtDecl * pHtDecl)
	{
		if (pHtDecl->isImplicit())
			return;

		switch (pHtDecl->getKind()) {
		case Decl::Typedef:
			if (pHtDecl->getRdRefCnt() > 0) {
				cout << indent << "typedef ";
				printQualType(pHtDecl->getQualType());
				cout << " " << pHtDecl->getName() << ";\n";
			}
			break;
		case Decl::CXXRecord:
			if (pHtDecl->isScModule()) {
				cout << indent << "SC_MODULE(" << pHtDecl->getName() << ")";
			} else {
				cout << indent << pHtDecl->getTagKindAsStr() << " " << pHtDecl->getName();

				if (pHtDecl->getFirstBaseSpecifier()) {
					cout << " : ";

					for (HtBaseSpecifier * pBaseSpec = pHtDecl->getFirstBaseSpecifier(); pBaseSpec; pBaseSpec = pBaseSpec->getNextBaseSpec()) {

						if (pBaseSpec->isVirtual())
							cout << "virtual ";

						cout << pBaseSpec->getAccessSpecName();

						cout << pBaseSpec->getQualType().getType()->getHtDecl()->getName();

						if (pBaseSpec->getNextBaseSpec())
							cout << ", ";
					}
				}
			}

			if (pHtDecl->getSubDeclList().size() > 0) {
				cout << " {\n";
				for (HtDeclList_iter_t declIter = pHtDecl->getSubDeclList().begin();
					declIter != pHtDecl->getSubDeclList().end(); declIter++)
				{
					printDecl(indent + "  ", *declIter );
				}
				cout << indent << "};\n\n";
			} else
				cout << ";\n";
			break;
		case Decl::Var:
		case Decl::Field:
			cout << indent;
			printQualType(pHtDecl->getQualType(), pHtDecl->getName());
			if (pHtDecl->hasFieldBitWidth()) {
				cout << " : " << pHtDecl->getFieldBitWidth();
			}
			if (pHtDecl->hasInitExpr()) {// && !pHtDecl->getInitExpr()->isImplicitNode()) {
				cout << " = ";
				printExpr( HtStmtIter(pHtDecl->getInitExpr()) );
			}
			cout << ";\n";
			break;
		case Decl::IndirectField:
			break;
		case Decl::ParmVar:
			printQualType(pHtDecl->getQualType(), pHtDecl->getName());
			break;
		case Decl::CXXConstructor:
		case Decl::CXXDestructor:
			if (pHtDecl->isScCtor()) {
				cout << indent << "SC_CTOR(" << pHtDecl->getName() << ")";
			} else {
				cout << indent << pHtDecl->getName() << "(";
				for (HtDeclList_iter_t declIter = pHtDecl->getParamDeclList().begin();
					declIter != pHtDecl->getParamDeclList().end(); )
				{
					printDecl("", *declIter);
					declIter++;
					if (declIter != pHtDecl->getParamDeclList().end())
						cout << ", ";
				}
				cout << ")";
			}
			if (pHtDecl->hasBody()) {
				cout << " ";
				printStmt( indent, pHtDecl->getBody() );
			} else
				cout << ";\n";
			break;
		case Decl::Function:
		case Decl::CXXMethod:
		case Decl::CXXConversion:
			cout << indent;
			switch (pHtDecl->getKind()) {
			case Decl::CXXConversion:
				cout << "operator ";
				printQualType(pHtDecl->getQualType());
				break;
			case Decl::CXXMethod:
				if (pHtDecl->isDeclaredInMethod())
					printQualType(pHtDecl->getQualType(), pHtDecl->getName());
				else
					printQualType(pHtDecl->getQualType(), pHtDecl->getParentDecl()->getName() + "::" + pHtDecl->getName());
				break;
			case Decl::Function:
				printQualType(pHtDecl->getQualType(), pHtDecl->getName());
				break;
			}
			cout << " (";
			for (HtDeclList_iter_t declIter = pHtDecl->getParamDeclList().begin();
				declIter != pHtDecl->getParamDeclList().end(); )
			{
				printDecl("", *declIter);
				declIter++;
				if (declIter != pHtDecl->getParamDeclList().end())
					cout << ", ";
			}
			cout << ")";
			if (pHtDecl->doesThisDeclarationHaveABody() && pHtDecl->hasBody()) {
				cout << " ";
				printStmt(indent, pHtDecl->getBody());
			} else
				cout << ";\n";
			break;
		case Decl::ClassTemplate:
		case Decl::FunctionTemplate:
			cout << "template";
			if (pHtDecl->getParamDeclList().size() > 0) {
				cout << "<";
				for (HtDeclList_iter_t declIter = pHtDecl->getParamDeclList().begin();
					declIter != pHtDecl->getParamDeclList().end(); )
				{
					printDecl("", *declIter);
					declIter++;
					if (declIter != pHtDecl->getParamDeclList().end())
						cout << ", ";
				}
				cout << ">";
			}
			cout << "\n";
			printDecl(indent + "  ", pHtDecl->getTemplateDecl());
			break;
		case Decl::NonTypeTemplateParm:
			printQualType(pHtDecl->getQualType());
			cout << " " << pHtDecl->getName();
			break;
		case Decl::TemplateTypeParm:
			cout << (pHtDecl->wasDeclaredWithTypename() ? "typename " : "class ") << pHtDecl->getName();
			if (pHtDecl->hasDefaultArgument()) {
				cout << "=";
				printQualType( pHtDecl->getDefaultArgQualType() );
			}
			break;
		case Decl::AccessSpec:
			cout << indent.substr(0, indent.size()-2) << pHtDecl->getAccessName() << ":\n";
			break;
		case Decl::Enum:
			cout << indent << "enum " << pHtDecl->getName();

			if (pHtDecl->getSubDeclList().size() > 0) {
				cout << " { ";
				for (HtDeclList_iter_t declIter = pHtDecl->getSubDeclList().begin();
					declIter != pHtDecl->getSubDeclList().end(); )
				{
					printDecl(indent + "  ", *declIter );
					declIter++;
					if (declIter != pHtDecl->getSubDeclList().end())
						cout << ", ";
				}
				cout << indent << " }";
			}
			cout << ";\n\n";
			break;
		case Decl::EnumConstant:
			cout << pHtDecl->getName();
			if (pHtDecl->hasInitExpr()) {
				cout << "=";
				printExpr(HtStmtIter(pHtDecl->getInitExpr()));
			}
			break;
		case Decl::PragmaHtv:
			cout << HtDesign::getSourceLine( m_context, pHtDecl->getLoc() ) << '\n';
			break;
		default:
			assert(0);
		}
	}

	void HtPrintDesign::printStmt(string indent, HtStmtList & stmtList)
	{
		for (HtStmtIter stmtIter(stmtList); !stmtIter.isEol(); stmtIter++ )
			printStmt(indent, stmtIter );
	}

	void HtPrintDesign::printStmt(string indent, HtStmtIter & iStmt)
	{
		if (iStmt->isCtorMemberInit())
			return;

		switch (iStmt->getStmtClass()) {
		case Stmt::DeclStmtClass:
			for (HtDeclList_iter_t declIter = iStmt->getDeclList().begin();
				declIter != iStmt->getDeclList().end(); declIter++ )
			{
				if (iStmt->isInitStmt())
					printInitDecl(indent, *declIter);
				else
					printDecl(indent, *declIter);
			}
			break;
		case Stmt::NullStmtClass:
			cout << indent << ";\n";
			break;
		case Stmt::CompoundStmtClass:
			cout << indent << "{\n";
			for (HtStmtIter subStmtIter(iStmt->getStmtList()); !subStmtIter.isEol(); subStmtIter++)
			{
				printStmt(indent + "  ", subStmtIter );
			}
			cout << indent << "}\n";
			break;
		case Stmt::ForStmtClass:
			{
				cout << indent << "for (";
				printStmt("", iStmt->getInit());
				printExpr( HtStmtIter(iStmt->getCond()) );
				cout << "; ";
				printExpr( HtStmtIter(iStmt->getInc()) );
				cout << ")\n";
				printStmt( indent + "  ", iStmt->getBody() );
			}
			break;
		case Stmt::IfStmtClass:
			{
				cout << indent << "if (";
				printExpr( HtStmtIter(iStmt->getCond()) );
				cout << ")\n";
				printStmt(indent + "  ", iStmt->getThen());
				if (!iStmt->getElse().empty()) {
					cout << indent << "else\n";
					printStmt(indent + "  ", iStmt->getElse());
				}
			}
			break;
		case Stmt::SwitchStmtClass:
			cout << indent << "switch (";
			printExpr( HtStmtIter(iStmt->getCond()) );
			cout << ")\n";
			printStmt(indent, iStmt->getBody());
			break;
		case Stmt::CaseStmtClass:
			if (iStmt->getNodeId() == 0x16b4)
				bool stop = true;
			cout << indent << "case ";
			printExpr( HtStmtIter(iStmt->getLhs()) );
			cout << ":\n";
			printStmt(indent + "  ", iStmt->getSubStmt());
			break;
		case Stmt::DefaultStmtClass:
			cout << indent << "default:\n";
			printStmt(indent + "  ", iStmt->getSubStmt());
			break;
		case Stmt::BreakStmtClass:
			cout << indent << "break;\n";
			break;
		case Stmt::ReturnStmtClass:
			cout << indent << "return ";
			printExpr( HtStmtIter(iStmt->getSubExpr()) );
			cout << ";\n";
			break;
		//case Stmt::CXXOperatorCallExprClass:
		//case Stmt::CXXMemberCallExprClass:
		//case Stmt::ImplicitCastExprClass:
		//case Stmt::CXXStaticCastExprClass:
		//case Stmt::CStyleCastExprClass:
		//case Stmt::MemberExprClass:
		//case Stmt::CXXThisExprClass:
		//case Stmt::UnaryOperatorClass:
		//case Stmt::BinaryOperatorClass:
		//case Stmt::CXXBoolLiteralExprClass:
		//case Stmt::IntegerLiteralClass:
		//case Stmt::ParenExprClass:
		//case Stmt::CallExprClass:
		//case Stmt::CompoundAssignOperatorClass:
		//case Stmt::DeclRefExprClass:
		default:
			//assert(iStmt->isAssignStmt());
			cout << indent;
			printExpr( iStmt );
			cout << ";\n";
			break;
		}
	}

	void HtPrintDesign::printExprList(HtStmtList & exprList, bool needParanIfNotLValue)
	{
		for (HtStmtIter exprIter(exprList); !exprIter.isEol(); exprIter++ )
			printExpr( exprIter, needParanIfNotLValue );
	}

	void HtPrintDesign::printExpr(HtStmtIter & iStmt, bool needParanIfNotLValue)
	{
		Stmt::StmtClass exprKind = iStmt->getStmtClass();
		switch (exprKind) {
		case Stmt::ParenExprClass:
			cout << "( ";
			printExpr( HtStmtIter(iStmt.asExpr()->getSubExpr()), false);
			cout << " )";
			break;
		case Stmt::UnaryOperatorClass:
			{
				if (needParanIfNotLValue) cout << "(";
				switch (iStmt.asExpr()->getUnaryOperator()) {
				case UO_PreInc:
				case UO_PreDec:
				case UO_LNot:
				case UO_Deref:
					cout << iStmt.asExpr()->getOpcodeAsStr();
					printExpr( HtStmtIter(iStmt.asExpr()->getSubExpr()) );
					break;
				case UO_PostInc:
				case UO_PostDec:
					printExpr( HtStmtIter(iStmt.asExpr()->getSubExpr()) );
					cout << iStmt.asExpr()->getOpcodeAsStr();
					break;
				default:
					assert(0);
				}
				if (needParanIfNotLValue) cout << ")";
			}
			break;
		case Stmt::BinaryOperatorClass:
		case Stmt::CompoundAssignOperatorClass:
			{
				if (needParanIfNotLValue) cout << "(";
				printExpr( HtStmtIter(iStmt.asExpr()->getLhs()) );
				cout << " " << iStmt.asExpr()->getOpcodeAsStr() << " ";
				printExpr( HtStmtIter(iStmt.asExpr()->getRhs()) );
				if (needParanIfNotLValue) cout << ")";
			}
			break;
		case Stmt::ConditionalOperatorClass:
			{
				if (needParanIfNotLValue) cout << "(";
				printExpr( HtStmtIter(iStmt.asExpr()->getCond()) );
				cout << " ? ";
				printExpr( HtStmtIter(iStmt.asExpr()->getLhs()) );
				cout << " : ";
				printExpr( HtStmtIter(iStmt.asExpr()->getRhs()) );
				if (needParanIfNotLValue) cout << ")";
			}
			break;
		case Stmt::MemberExprClass:
			{
				Stmt::StmtClass stmtClass = iStmt->getBase()->getStmtClass();
				bool bThis = stmtClass == Stmt::CXXThisExprClass;

				if (!bThis) {
					bool needParanIfNotLValue = true;
					printExpr( HtStmtIter(iStmt.asExpr()->getBase()), needParanIfNotLValue);
				}

				if (!iStmt.asExpr()->isImplicitNode()) {
					switch (iStmt.asExpr()->getMemberDecl()->getKind()) {
					case Decl::CXXConversion:
						if (!bThis)
							cout << (iStmt.asExpr()->isArrow() ? "->" : ".");
						cout << "operator " << iStmt.asExpr()->getMemberDecl()->getName();
						break;
					case Decl::CXXMethod:
					case Decl::Field:
						if (iStmt.asExpr()->getMemberDecl()->getName().size() > 0) { // anonamous struct/unions are not printed
							if (!bThis)
								cout << (iStmt.asExpr()->isArrow() ? "->" : ".");
							cout << iStmt.asExpr()->getMemberDecl()->getName();
						}
						break;
					//case Decl::Function:
					//	printQualType(pHtDecl->getQualType(), pHtDecl->getName());
					//	break;
					default:
						assert(0);
					}
				}
			}
			break;
		case Stmt::DeclRefExprClass:
			{
				cout << iStmt.asExpr()->getRefDecl()->getName();
			}
			break;
		case Stmt::CXXBoolLiteralExprClass:
			{
				cout << iStmt.asExpr()->getBooleanLiteralAsStr();
			}
			break;
		case Stmt::IntegerLiteralClass:
			{
				cout << iStmt.asExpr()->getIntegerLiteralAsStr();
			}
			break;
		case Stmt::StringLiteralClass:
			{
				cout << "\"" << iStmt.asExpr()->getStringLiteralAsStr() << "\"";
			}
			break;
		case Stmt::FloatingLiteralClass:
			{
				cout << iStmt.asExpr()->getFloatingLiteralAsStr();
			}
			break;
		case Stmt::ImplicitCastExprClass:
		case Stmt::CXXStaticCastExprClass:
			{
				printExpr( HtStmtIter(iStmt.asExpr()->getSubExpr()), needParanIfNotLValue);
			}
			break;
		case Stmt::CXXDependentScopeMemberExprClass:
			{
				// CStyle casts are handled as CXXConstructExpr subExpr
				printExpr(  HtStmtIter(iStmt.asExpr()->getSubExpr()) );
			}
			break;
		case Stmt::CXXFunctionalCastExprClass:
			{
				// CStyle casts are handled as CXXConstructExpr subExpr
				printExpr(  HtStmtIter(iStmt.asExpr()->getSubExpr()), needParanIfNotLValue );
			}
			break;
		case Stmt::CStyleCastExprClass:
			{
				// CStyle casts are handled as CXXConstructExpr subExpr
				printExpr( HtStmtIter(iStmt.asExpr()->getSubExpr()), needParanIfNotLValue);
			}
			break;
		case Stmt::ArraySubscriptExprClass:
			{
				printExpr( HtStmtIter(iStmt.asExpr()->getBase()) );
				cout << "[";
				printExpr( HtStmtIter(iStmt.asExpr()->getIdx()) );
				cout << "]";
			}
			break;
		case Stmt::CXXConstructExprClass:
			{
				if (iStmt.asExpr()->isElidable()) {
					HtStmtList & argExprList = iStmt.asExpr()->getArgExprList();
					if (!argExprList.empty())
						printExpr( HtStmtIter(argExprList) );
				} else {
					printQualType(HtType::PreType, iStmt.asExpr()->getQualType());
					cout << "(";
					for (HtStmtIter argExprIter(iStmt.asExpr()->getArgExprList()); !argExprIter.isEol(); )
					{
						printExpr( argExprIter++ );
						if ( !argExprIter.isEol() )
							cout << ", ";
					}
					cout << ")";
				}
			}
			break;
		case Stmt::CXXUnresolvedConstructExprClass:
			{
				printQualType(HtType::PreType, iStmt.asExpr()->getQualType());
				cout << "(";
				for (HtStmtIter argExprIter(iStmt.asExpr()->getArgExprList()); !argExprIter.isEol(); )
				{
					printExpr( argExprIter++ );
					if ( !argExprIter.isEol() )
						cout << ", ";
				}
				cout << ")";
			}
			break;
		case Stmt::CallExprClass:
			{
				HtDecl * pDecl = iStmt.asExpr()->getCalleeDecl();

				vector<HtDecl *> scopeVec;
				pDecl = pDecl->getParentDecl();
				while (pDecl && pDecl != m_pHtDesign->getScModuleDecl()) {
					scopeVec.push_back(pDecl);
					pDecl = pDecl->getParentDecl();
				}

				for (size_t i = 0; i < scopeVec.size(); i += 1) {
					printQualType(scopeVec[i]->getQualType());
					cout << "::";
				}

				cout << iStmt.asExpr()->getCalleeDecl()->getName() << "(";
				for (HtStmtIter argExprIter(iStmt.asExpr()->getArgExprList()); !argExprIter.isEol(); )
				{
					printExpr( argExprIter++ );
					if ( !argExprIter.isEol() )
						cout << ", ";
				}
				cout << ")";
			}
			break;
		case Stmt::MaterializeTemporaryExprClass:
			{
				printExpr(  HtStmtIter(iStmt.asExpr()->getSubExpr()) );
			}
			break;
		case Stmt::CXXMemberCallExprClass:
			{
				if (iStmt.asExpr()->isScMethod()) {
					cout << "SC_METHOD(" << iStmt.asExpr()->getMemberDecl()->getName() << ")";
				} else {
					printExpr( HtStmtIter(iStmt.asExpr()->getCalleeExpr()) );
					if (!iStmt.asExpr()->getCalleeExpr().asExpr()->isImplicitNode()) {
						cout << "(";
						for (HtStmtIter argExprIter(iStmt.asExpr()->getArgExprList()); !argExprIter.isEol(); )
						{
							printExpr( argExprIter++ );
							if ( !argExprIter.isEol() )
								cout << ", ";
						}
						cout << ")";
					}
				}
			}
			break;
		case Stmt::CXXOperatorCallExprClass:
			{
				HtStmtIter argExprIter( iStmt.asExpr()->getArgExprList() );
				printExpr( argExprIter++ );
				cout << " " << iStmt.asExpr()->getCalleeDecl()->getName().substr(8) << " ";
				printExpr( argExprIter );
			}
			break;
		case Stmt::CXXThisExprClass:
			cout << "this";
			break;
		case Stmt::CXXTemporaryObjectExprClass:
			printQualType( iStmt.asExpr()->getQualType() );
			cout << "()";
			break;
		case Stmt::InitListExprClass:
			{
				cout << "{ ";
				HtStmtList & initExprList = iStmt.asExpr()->getInitExprList();
				for (HtStmtIter initExprIter(initExprList); !initExprIter.isEol(); )
				{
					printExpr( initExprIter++ );
					if ( !initExprIter.isEol() )
						cout << ", ";
				}
				cout << " }";
			}
			break;
		case Stmt::SubstNonTypeTemplateParmExprClass:
			printExpr( HtStmtIter(iStmt->getReplacement()) );
			break;
		default:
			assert(0);
		}
	}

	void HtPrintDesign::printQualType(HtQualType & qualType, string identStr)
	{
		printQualType(HtType::PreType, qualType);	//  uint64_t const *
		if (identStr.size() > 0)
			cout << " " << identStr;
		printQualType(HtType::PostType, qualType);	//  [4]
	}

	void HtPrintDesign::printQualType(HtType::HtPrintType printFlag, HtQualType & qualType)
	{
		switch (qualType.getType()->getTypeClass()) {
		case Type::Builtin:
			if (printFlag == HtType::PreType)
				cout << qualType.getType()->getHtDecl()->getName();
			break;
		case Type::DependentSizedArray:
			if (printFlag == HtType::PostType) {
				cout << "[";
				printExpr( HtStmtIter(qualType.getType()->getSizeExpr()) );
				cout << "]";
			}
			printQualType(printFlag, qualType.getType()->getQualType());
			break;
		case Type::ConstantArray:
			if (printFlag == HtType::PostType)
				cout << "[" << qualType.getType()->getSize().toString(10, false) << "]";
			printQualType(printFlag, qualType.getType()->getQualType());
			break;
		case Type::Pointer:
			printQualType(printFlag, qualType.getType()->getQualType());
			if (printFlag == HtType::PreType)
				cout << " *";
			break;
		case Type::LValueReference:
			printQualType(printFlag, qualType.getType()->getQualType());
			if (printFlag == HtType::PreType)
				cout << " &";
			if (printFlag == HtType::BaseType && qualType.isConst())
				cout << " const";
			break;
		case Type::RValueReference:
			printQualType(printFlag, qualType.getType()->getQualType());
			if (printFlag == HtType::PreType)
				cout << " &&";
			break;
		case Type::Elaborated:
		case Type::Record:
			if (printFlag == HtType::PreType) {
				HtDecl * pDecl1 = qualType.getType()->getHtDecl();
				while (pDecl1->getName().size() == 0) {
					pDecl1 = pDecl1->getParentDecl();
					assert(pDecl1 && pDecl1->getKind() == Decl::CXXRecord);
				}
				cout << pDecl1->getName();

				HtDecl * pDecl = qualType.getType()->getHtDecl();
				if (pDecl->getKind() == Decl::ClassTemplateSpecialization) {
					cout << "<";
					for (HtTemplateArgument * pTemplateArg = pDecl->getFirstTemplateArg();
						pTemplateArg; pTemplateArg = pTemplateArg->getNextTemplateArg())
					{
						printTemplateArg( pTemplateArg );
						if (pTemplateArg->getNextTemplateArg())
							cout << ", ";
					}
					cout << ">";
				}
			}
			break;
		case Type::FunctionProto:
			if (printFlag == HtType::PreType)
				printQualType(printFlag, qualType.getType()->getQualType());
			break;
		case Type::Typedef:
			if (printFlag == HtType::PreType)
				cout << qualType.getType()->getHtDecl()->getName();
			break;
		case Type::TemplateSpecialization:
			if (printFlag == HtType::PreType) {
				cout << qualType.getType()->getTemplateDecl()->getName() << "<";
				for (HtTemplateArgument * pTemplateArg = qualType.getType()->getFirstTemplateArg();
					pTemplateArg; pTemplateArg = pTemplateArg->getNextTemplateArg())
				{
					printTemplateArg( pTemplateArg );
					if (pTemplateArg->getNextTemplateArg())
						cout << ", ";
				}
				cout << ">";
			}
			break;
		case Type::TemplateTypeParm:
			if (printFlag == HtType::PreType)
				cout << qualType.getType()->getTemplateTypeParmName();
			break;
		case Type::SubstTemplateTypeParm:
			printQualType(printFlag, qualType.getType()->getQualType());
			break;
		case Type::Decayed:
			printQualType(printFlag, qualType.getType()->getQualType());
			break;
		case Type::InjectedClassName:
			printQualType(printFlag, qualType.getType()->getQualType());
			break;
		case Type::Paren:
			if (printFlag == HtType::PreType) {
				printQualType(printFlag, qualType.getType()->getQualType());
				cout << " (";
			}
			if (printFlag == HtType::PostType) {
				cout << ") ";
				printQualType(printFlag, qualType.getType()->getQualType());
			}
			break;
		case Type::Enum:
			if (printFlag == HtType::PreType)
				cout << qualType.getType()->getHtDecl()->getName();
			break;
		default:
			assert(0);
		}

		if (printFlag == HtType::PreType) {
			if (qualType.isConst())
				cout << " const";
		}
	}

	void HtPrintDesign::printTemplateArg(HtTemplateArgument * pHtTemplateArg)
	{
		switch (pHtTemplateArg->getKind()) {
		case TemplateArgument::Expression:
			printExpr( HtStmtIter(pHtTemplateArg->getExpr()) );
			break;
		case TemplateArgument::Type:
			printQualType( pHtTemplateArg->getQualType() );
			break;
		case TemplateArgument::Integral:
			cout << pHtTemplateArg->getIntegerLiteralAsStr();
			break;
		default:
			assert(0);
		}
	}

	void HtPrintDesign::printInitDecl(string indent, HtDecl * pHtDecl)
	{
		if (!pHtDecl->hasInitExpr())
			return;

		HtStmtIter initExpr = HtStmtIter(pHtDecl->getInitExpr());

		if (pHtDecl->getType()->getTypeClass() == Type::ConstantArray) {
			// an array declaration. Must break apart and generate
			// a statement per array element

			vector<int> dimenList;
			vector<HtStmtIter> initExprList;

			HtType * pType = pHtDecl->getType();
			HtStmtIter initIter( initExpr.asExpr()->getInitExprList() );
			//HtExpr * pInitList = pInitExpr->getFirstInitExpr();
			bool hasInitList = !initIter.isEol();

			while (pType->getTypeClass() == Type::ConstantArray) {
				dimenList.push_back( (int)pType->getSize().getZExtValue() );

				if (hasInitList) {
					initExprList.push_back( initIter );

					if (initIter->getStmtClass() == Stmt::InitListExprClass)
						initIter = HtStmtIter( initIter.asExpr()->getInitExprList() );
				}

				pType = pType->getType();
			}

			// iterate through dimensions generating an initialization statement for each
			vector<int> idxList(dimenList.size());	// create same size list with zero values

			size_t dimIdx;
			for (;;) {
				cout << indent << pHtDecl->getName();

				for (size_t i = 0; i < dimenList.size(); i += 1)
					cout << "[" << idxList[i] << "]";

				cout << " = ";

				printExpr(hasInitList ? initExprList.back() : initExpr);

				cout << ";\n";

				// increment the dimension indecies and advance the init expr list
				for (dimIdx = dimenList.size()-1; dimIdx >= 0; dimIdx -= 1) {
					idxList[dimIdx] += 1;

					if (hasInitList)
						initExprList[dimIdx]++;

					if (idxList[dimIdx] != dimenList[dimIdx] && (!hasInitList || !initExprList[dimIdx].isEol()))
						break;
				}

				if (dimIdx < 0)
					break;

				for (dimIdx += 1 ; dimIdx < dimenList.size(); dimIdx += 1) {
					idxList[dimIdx] = 0;
					if (hasInitList)
						initExprList[dimIdx] = HtStmtIter( initExprList[dimIdx-1].asExpr()->getInitExprList() );
				}
			}

		} else {
			cout << indent << pHtDecl->getName();
			cout << " = ";
			printExpr( HtStmtIter(pHtDecl->getInitExpr()) );
			cout << ";\n";
		}
	}
}
