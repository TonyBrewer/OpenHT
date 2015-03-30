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

	// Inline function calls within the SC_METHODs of an SC_MODULE
	// - recursively inline function calls
	// - must break expressions with function calls into piece evaluated
	//   prior to function, the inlined function, and the piece evaluated
	//   after the function to preserve order of modifications to variables.

	class HtInlineFunctionCalls
	{
	public:
		HtInlineFunctionCalls(HtDesign * pHtDesign, ASTContext &Context)
			: m_pHtDesign(pHtDesign), m_diags(Context.getDiagnostics()) {}

		void inlineFuncInDeclList( HtStmtList & dupStmtList, HtExpr * pBaseExpr, HtDeclIter & declIter );
		void inlineFuncInDecl( HtStmtList & dupStmtList, HtExpr * pBaseExpr, HtDeclIter & declIter );
		void inlineFuncInDecl( HtStmtList & dupStmtList, HtExpr * pBaseExpr, HtDecl * pDecl );

		void inlineFuncInStmt( HtStmtList & dupStmtList, HtStmtList & srcStmtList, HtDecl * pRtnTempDecl, HtExpr * pBaseExpr );

		void handleInitDecl(HtStmtList & insertList, HtDecl * pOrigDecl, HtDecl * pNewDecl, HtExpr * pBaseExpr);

		HtExpr * inlineFuncInExpr(HtStmtList & dupStmtList, HtExpr * pSrcExpr, HtExpr * pBaseExpr);

		bool isReferenceType(HtType * pHtType);

		void pushCallArgs( HtStmtList & dupStmtList, HtExpr * pBaseExpr, HtDeclIter & paramDeclIter, HtStmtIter & argExprIter );
		void popCallArgs( HtDeclIter & paramDeclIter );

		HtDecl * genUniqueExprVar( HtStmtList & dupStmtList, HtQualType & qualType, HtLineInfo & lineInfo );

		string findUniqueName( string name, HtLineInfo & lineInfo ) {
			string uniqueName = m_pHtDesign->findUniqueName( name, lineInfo );
			if (uniqueName == "value$3$59")
				bool stop = true;
			return uniqueName;
		}

	private:
		DiagnosticsEngine & m_diags;
		HtDesign * m_pHtDesign;
		HtDecl * m_pScModuleDecl;
	};

	void HtDesign::inlineFunctionCalls(ASTContext &Context)
	{
		HtInlineFunctionCalls transform(this, Context);

		// start with top level declarations
		HtDeclList_t & declList = getTopDeclList();
		HtDeclIter declIter(declList);

		HtStmtList stmtList;

		transform.inlineFuncInDeclList( stmtList, 0, declIter );

		assert(stmtList.size() == 0);

		ExitIfError(Context);
	}

	void HtInlineFunctionCalls::inlineFuncInDeclList( HtStmtList & stmtList, HtExpr * pBaseExpr, HtDeclIter & declIter )
	{
		while ( !declIter.isEol() )
		{
			inlineFuncInDecl( stmtList, pBaseExpr, declIter );
		}
	}

	void HtInlineFunctionCalls::inlineFuncInDecl( HtStmtList & stmtList, HtExpr * pBaseExpr, HtDecl * pDecl )
	{
		HtDeclList_t declList;
		declList.push_back(pDecl);
		HtDeclIter declIter(declList);

		HtStmtList emptyList;

		inlineFuncInDecl( emptyList, 0, declIter );

		assert(emptyList.size() == 0);
	}

	void HtInlineFunctionCalls::inlineFuncInDecl( HtStmtList & stmtList, HtExpr * pBaseExpr, HtDeclIter & declIter )
	{
		HtDecl * pOrigDecl = *declIter;

		switch (pOrigDecl->getKind()) {
		case Decl::Typedef:
		case Decl::NonTypeTemplateParm:
		case Decl::TemplateTypeParm:
		case Decl::Var:
		case Decl::Field:
		case Decl::IndirectField:
		case Decl::Enum:
			// nothing to do
			declIter++;
			break;
		case Decl::AccessSpec:
		case Decl::CXXConstructor:
		case Decl::CXXDestructor:
		case Decl::Function:
		case Decl::CXXConversion:
			// delete these declarations
			{
				if (declIter->isHtPrim() || declIter->isScCtor()) {
					declIter++;
					break;
				}

				HtDeclIter deleteIter = declIter++;
				deleteIter.erase();
			}
			break;
		case Decl::FunctionTemplate:
		case Decl::ClassTemplate:
			{
				HtStmtList emptyList;
				inlineFuncInDecl( emptyList, 0, pOrigDecl->getTemplateDecl() );
				assert(emptyList.size() == 0);
				declIter++;
			}
			break;
		case Decl::CXXMethod:
			{
				if (!pOrigDecl->isScMethod()) {
					// if not ScMethod then remove from decl list (it will be inlined if used)
					HtDeclIter deleteIter = declIter++;
					deleteIter.erase();
					break;
				}

				assert(pOrigDecl->getParentDecl()->isScModule());
				m_pScModuleDecl = pOrigDecl->getParentDecl();

				if (pOrigDecl->doesThisDeclarationHaveABody()) {
					// Make copy of method body
					if (pOrigDecl->isBeingInlined()) {
						unsigned DiagID = m_diags.getCustomDiagID(DiagnosticsEngine::Error,
							"htt/htv do not support recursion");
						m_diags.Report(pOrigDecl->getLineInfo().getLoc(), DiagID);

					} else {
						HtStmtList inlinedBody;

						pOrigDecl->isBeingInlined( true );
						inlineFuncInStmt( inlinedBody, pOrigDecl->getBody(), 0, 0 );
						pOrigDecl->getBody() = inlinedBody;

						pOrigDecl->isBeingInlined( false );
					}
				}

				declIter++;
			}
			break;
		case Decl::CXXRecord:
			{
				// walk sub decl list
				HtDeclIter subDeclIter(pOrigDecl->getSubDeclList());
				while ( !subDeclIter.isEol() )
				{
					inlineFuncInDecl( stmtList, pBaseExpr, subDeclIter );
				}

				// add public accessSpec if a class type record
				if (pOrigDecl->getTagKind() == TTK_Class) {
					HtDecl * pAccessDecl = new HtDecl( Decl::AccessSpec, pOrigDecl->getLineInfo() );
					pAccessDecl->setAccess( AS_public );
					pOrigDecl->getSubDeclList().push_front( pAccessDecl );
				}

				declIter++;
			}
			break;
		default:
			assert(0);
		}
	}

	void HtInlineFunctionCalls::inlineFuncInStmt( HtStmtList & dupStmtList, HtStmtList & srcStmtList, 
		HtDecl * pRtnTempDecl, HtExpr * pBaseExpr )
	{
		vector<HtDecl *> declList;

		for ( HtStmtIter iSrcStmt(srcStmtList); !iSrcStmt.isEol(); iSrcStmt++ ) {

			if (iSrcStmt->isAssignStmt()) {
				HtExpr * pExpr = inlineFuncInExpr( dupStmtList, iSrcStmt.asExpr(), pBaseExpr );

				Stmt::StmtClass stmtClass = iSrcStmt->getStmtClass();
				if (stmtClass != Stmt::CallExprClass && stmtClass != Stmt::CXXOperatorCallExprClass
					&& stmtClass != Stmt::CXXMemberCallExprClass && stmtClass != Stmt::CXXConstructExprClass)
				{
					dupStmtList.push_back( pExpr );
				}
				continue;
			}

			switch (iSrcStmt->getStmtClass()) {
			case Stmt::CompoundStmtClass:
				{
					HtStmt * pHtStmt = new HtStmt( iSrcStmt );
					inlineFuncInStmt( pHtStmt->getStmtList(), iSrcStmt->getStmtList(), pRtnTempDecl, pBaseExpr );
					dupStmtList.push_back( pHtStmt );
				}
				break;
			case Stmt::ForStmtClass:
				{
					HtStmt * pHtStmt = new HtStmt( iSrcStmt );
					inlineFuncInStmt( pHtStmt->getInit(), iSrcStmt->getInit(), pRtnTempDecl, pBaseExpr );
					inlineFuncInStmt( pHtStmt->getCond(), iSrcStmt->getCond(), pRtnTempDecl, pBaseExpr );
					inlineFuncInStmt( pHtStmt->getInc(), iSrcStmt->getInc(), pRtnTempDecl, pBaseExpr );
					inlineFuncInStmt( pHtStmt->getBody(), iSrcStmt->getBody(), pRtnTempDecl, pBaseExpr );
					dupStmtList.push_back( pHtStmt );
				}
				break;
			case Stmt::IfStmtClass:
				{
					HtStmt * pHtStmt = new HtStmt( iSrcStmt );
					pHtStmt->setCond( inlineFuncInExpr( dupStmtList, iSrcStmt->getCond().asExpr(), pBaseExpr ));
					inlineFuncInStmt( pHtStmt->getThen(), iSrcStmt->getThen(), pRtnTempDecl, pBaseExpr );
					inlineFuncInStmt( pHtStmt->getElse(), iSrcStmt->getElse(), pRtnTempDecl, pBaseExpr );
					dupStmtList.push_back( pHtStmt );
				}
				break;
			case Stmt::SwitchStmtClass:
				{
					HtStmt * pHtStmt = new HtStmt( iSrcStmt );
					pHtStmt->setCond( inlineFuncInExpr( dupStmtList, iSrcStmt->getCond().asExpr(), pBaseExpr ));
					inlineFuncInStmt( pHtStmt->getBody(), iSrcStmt->getBody(), pRtnTempDecl, pBaseExpr );
					dupStmtList.push_back( pHtStmt );
				}
				break;
			case Stmt::CaseStmtClass:
				{
					HtStmt * pHtStmt = new HtStmt( iSrcStmt );
					pHtStmt->setLhs( inlineFuncInExpr( dupStmtList, iSrcStmt->getLhs().asExpr(), pBaseExpr ));
					inlineFuncInStmt( pHtStmt->getSubStmt(), iSrcStmt->getSubStmt(), pRtnTempDecl, pBaseExpr );
					dupStmtList.push_back( pHtStmt );
				}
				break;
			case Stmt::DefaultStmtClass:
				{
					HtStmt * pHtStmt = new HtStmt( iSrcStmt );
					inlineFuncInStmt( pHtStmt->getSubStmt(), iSrcStmt->getSubStmt(), pRtnTempDecl, pBaseExpr );
					dupStmtList.push_back( pHtStmt );
				}
				break;
			case Stmt::BreakStmtClass:
				{
					HtStmt * pHtStmt = new HtStmt( iSrcStmt );
					dupStmtList.push_back( pHtStmt );
				}
				break;
			case Stmt::NullStmtClass:
				{
					HtStmt * pHtStmt = new HtStmt( iSrcStmt );
					dupStmtList.push_back( pHtStmt );
				}
				break;
			case Stmt::ReturnStmtClass:
				{
					assert(pRtnTempDecl);

					HtExpr * pHtRhs = inlineFuncInExpr( dupStmtList, iSrcStmt->getSubExpr().asExpr(), pBaseExpr );

					HtExpr * pHtLhs = 0;
					if (isReferenceType( pRtnTempDecl->getQualType().getType() )) {

						pHtLhs = new HtExpr( Stmt::DeclRefExprClass, pHtRhs->getQualType(), pHtRhs->getLineInfo() );
						pHtLhs->setRefDecl( pRtnTempDecl );

						HtExpr * pHtAssign = new HtExpr( Stmt::BinaryOperatorClass, pHtRhs->getQualType(), pHtRhs->getLineInfo() );
						pHtAssign->setBinaryOperator( BO_Assign );
						pHtAssign->isAssignStmt( true );
						pHtAssign->setRhs( pHtRhs );
						pHtAssign->setLhs( pHtLhs );

						dupStmtList.push_back( pHtAssign );

					} else {
						pHtLhs = new HtExpr( Stmt::MemberExprClass, pRtnTempDecl->getQualType(), pHtRhs->getLineInfo() );
						pHtLhs->setMemberDecl( pRtnTempDecl );

						HtExpr * pThis = new HtExpr( Stmt::CXXThisExprClass, HtQualType( 0 ), pHtRhs->getLineInfo() );
						pHtLhs->setBase( pThis );

						HtStmtIter iInsert(dupStmtList, dupStmtList.end());
						HtDesign::genAssignStmts(iInsert, pHtLhs, pHtRhs);
					}
				}
				break;
			case Stmt::DeclStmtClass:
				{
					HtDeclIter declIter(iSrcStmt->getDeclList());
					while ( !declIter.isEol() )
					{
						HtDecl * pNewDecl = new HtDecl( *declIter );

						if (isReferenceType( (*declIter)->getType() )) {

							HtStmt * pHtStmt = new HtStmt( iSrcStmt );
							dupStmtList.push_back( pHtStmt );

							pHtStmt->addDecl( pNewDecl );

						} else {

							pNewDecl->setDeclKind( Decl::Field );
							pNewDecl->setName( findUniqueName( pNewDecl->getName(), pNewDecl->getLineInfo() ));

							m_pHtDesign->getScModuleDecl()->addSubDecl( pNewDecl );
						}

						handleInitDecl(dupStmtList, *declIter, pNewDecl, pBaseExpr);

						(*declIter)->pushInlineDecl( pNewDecl );

						declList.push_back( *declIter );

						declIter++;
					}
				}
				break;
			default:
				assert(0);
			}
		}

		for (size_t i = 0; i < declList.size(); i += 1)
			declList[i]->popInlineDecl();
	}

	void HtInlineFunctionCalls::handleInitDecl(HtStmtList & insertList, HtDecl * pOrigDecl, 
		HtDecl * pNewDecl, HtExpr * pBaseExpr)
	{
		if (!pOrigDecl->hasInitExpr())
			return;

		HtExpr * pInitExpr = pOrigDecl->getInitExpr().asExpr();

		if (pOrigDecl->getType()->getTypeClass() == Type::ConstantArray) {
			// an array declaration. Must break apart and generate
			// a statement per array element

			vector<int> dimenList;
			vector<HtStmtIter> initExprList;

			HtType * pType = pOrigDecl->getType();
			HtStmtIter initIter( pInitExpr->getInitExprList() );
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
				HtExpr * pDeclRef = 0;
				if (isReferenceType(pNewDecl->getQualType().getType())) {
					pDeclRef = new HtExpr( Stmt::DeclRefExprClass, pNewDecl->getQualType(), pNewDecl->getLineInfo() );
					pDeclRef->setRefDecl( pNewDecl );
				} else {
					pDeclRef = new HtExpr( Stmt::MemberExprClass, pNewDecl->getQualType(), pNewDecl->getLineInfo() );
					pDeclRef->setMemberDecl( pNewDecl );

					HtExpr * pThis = new HtExpr( Stmt::CXXThisExprClass, HtQualType( 0 ), pNewDecl->getLineInfo() );
					pDeclRef->setBase( pThis );
				}

				HtType * pArrayType = pNewDecl->getType();
				HtExpr * pArrayBase = pDeclRef;

				for (size_t i = 0; i < dimenList.size(); i += 1) {
					assert( pArrayType->getTypeClass() == Type::ConstantArray);

					HtType * pIntType = m_pHtDesign->getBuiltinIntType();
					HtExpr * pArrayIdx = new HtExpr( Stmt::IntegerLiteralClass, HtQualType(pIntType), pNewDecl->getLineInfo() );
					pArrayIdx->setIntegerLiteral( idxList[i] );
					pArrayIdx->isConstExpr(true);

					HtExpr * pArraySubExpr = new HtExpr( Stmt::ArraySubscriptExprClass, HtQualType(pArrayType), pNewDecl->getLineInfo() );
					pArraySubExpr->setBase( pArrayBase );
					pArraySubExpr->setIdx( pArrayIdx );

					pArrayBase = pArraySubExpr;
					pArrayType = pArrayType->getType();
				}

				HtStmtIter initExprIter = hasInitList ? initExprList.back() : pOrigDecl->getInitExpr();

				if (initExprIter.asExpr()->getStmtClass() == Stmt::CXXConstructExprClass) {

					inlineFuncInExpr( insertList, initExprIter.asExpr(), pArrayBase );

				} else {
					HtExpr * pAssignStmt = new HtExpr( Stmt::BinaryOperatorClass, HtQualType(pArrayType), pOrigDecl->getLineInfo() );
					pAssignStmt->isAssignStmt( true );
					pAssignStmt->setBinaryOperator( BO_Assign );
					pAssignStmt->setLhs( pArrayBase );
					pAssignStmt->setRhs( inlineFuncInExpr( insertList, initExprIter.asExpr(), pArrayBase ) );

					insertList.push_back( pAssignStmt );
				}

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
			// create lhs expression
			HtExpr * pDeclRef = 0;
			if (isReferenceType(pOrigDecl->getQualType().getType())) {
				pDeclRef = new HtExpr( Stmt::DeclRefExprClass, pOrigDecl->getQualType(), pOrigDecl->getLineInfo() );
				pDeclRef->setRefDecl( pNewDecl );
			} else {
				pDeclRef = new HtExpr( Stmt::MemberExprClass, pOrigDecl->getQualType(), pOrigDecl->getLineInfo() );
				pDeclRef->setMemberDecl( pNewDecl );

				HtExpr * pThis = new HtExpr( Stmt::CXXThisExprClass, HtQualType( 0 ), pOrigDecl->getLineInfo() );
				pDeclRef->setBase( pThis );
			}

			if (pInitExpr->getStmtClass() == Stmt::CXXConstructExprClass) {
				inlineFuncInExpr( insertList, pOrigDecl->getInitExpr().asExpr(), pDeclRef );

			} else {
				HtExpr * pAssignStmt = new HtExpr( Stmt::BinaryOperatorClass, pOrigDecl->getQualType(), pOrigDecl->getLineInfo() );
				pAssignStmt->isAssignStmt( true );
				pAssignStmt->setBinaryOperator( BO_Assign );
				pAssignStmt->setLhs( pDeclRef );
				pAssignStmt->setRhs( inlineFuncInExpr( insertList, pOrigDecl->getInitExpr().asExpr(), pBaseExpr ) );

				insertList.push_back( pAssignStmt );
			}
		}
								
		pOrigDecl->setInitExpr( 0 );
	}

	HtExpr * HtInlineFunctionCalls::inlineFuncInExpr(HtStmtList & dupStmtList, HtExpr * pSrcExpr, HtExpr * pBaseExpr)
	{
		// walk expression in the order of execution
		HtExpr * pNewExpr = 0;

		switch (pSrcExpr->getStmtClass()) {
		case Stmt::CompoundAssignOperatorClass:
			pNewExpr = new HtExpr( pSrcExpr );
			pNewExpr->setLhs( inlineFuncInExpr( dupStmtList, pSrcExpr->getLhs().asExpr(), pBaseExpr ));
			pNewExpr->setRhs( inlineFuncInExpr( dupStmtList, pSrcExpr->getRhs().asExpr(), pBaseExpr ));
			break;
		case Stmt::UnaryOperatorClass:
			if ( pSrcExpr->getUnaryOperator() == UO_Deref
				&& pSrcExpr->getSubExpr().asExpr()->getStmtClass() == Stmt::CXXThisExprClass)
			{
				return inlineFuncInExpr( dupStmtList, pSrcExpr->getSubExpr().asExpr(), pBaseExpr );
			}
			else
			{
				pNewExpr = new HtExpr( pSrcExpr );
				pNewExpr->setSubExpr( inlineFuncInExpr( dupStmtList, pSrcExpr->getSubExpr().asExpr(), pBaseExpr ));
			}
			break;
		case Stmt::BinaryOperatorClass:
			if (pSrcExpr->getOpcodeAsStr() == "+")
				bool stop = true;
			pNewExpr = new HtExpr( pSrcExpr );
			pNewExpr->setLhs( inlineFuncInExpr( dupStmtList, pSrcExpr->getLhs().asExpr(), pBaseExpr ));
			pNewExpr->setRhs( inlineFuncInExpr( dupStmtList, pSrcExpr->getRhs().asExpr(), pBaseExpr ));
			break;
		case Stmt::ConditionalOperatorClass:
			pNewExpr = new HtExpr( pSrcExpr );
			pNewExpr->setCond( inlineFuncInExpr( dupStmtList, pSrcExpr->getCond().asExpr(), pBaseExpr ));
			pNewExpr->setLhs( inlineFuncInExpr( dupStmtList, pSrcExpr->getLhs().asExpr(), pBaseExpr ));
			pNewExpr->setRhs( inlineFuncInExpr( dupStmtList, pSrcExpr->getRhs().asExpr(), pBaseExpr ));
			break;
		case Stmt::CXXConstructExprClass:
			{
				HtDecl * pCtorDecl = pSrcExpr->getConstructorDecl();
				HtAssert(pCtorDecl != 0);

				HtExpr * pNewBaseExpr;
				if (isReferenceType(pSrcExpr->getQualType().getType())) {
					HtDecl * pNewDecl = genUniqueExprVar( dupStmtList, pSrcExpr->getQualType(), pSrcExpr->getLineInfo() );

					pNewBaseExpr = new HtExpr( Stmt::DeclRefExprClass, pNewDecl->getQualType(), pNewDecl->getLineInfo() );
					pNewBaseExpr->setRefDecl( pNewDecl );

				} else {
					HtDecl * pNewDecl = genUniqueExprVar( dupStmtList, pSrcExpr->getQualType(), pSrcExpr->getLineInfo() );

					pNewBaseExpr = new HtExpr( Stmt::MemberExprClass, pNewDecl->getQualType(), pNewDecl->getLineInfo() );
					pNewBaseExpr->setMemberDecl( pNewDecl );

					HtExpr * pThis = new HtExpr( Stmt::CXXThisExprClass, HtQualType( 0 ), pNewDecl->getLineInfo() );
					pNewBaseExpr->setBase( pThis );
				}

				bool bHasParam = pCtorDecl->getParamDeclList().size() > 0;
	
				if (bHasParam) {
					HtDeclIter paramDeclIter(pCtorDecl->getParamDeclList());
					HtStmtIter argExprIter(pSrcExpr->getArgExprList());

					pushCallArgs( dupStmtList, pBaseExpr, paramDeclIter, argExprIter );
				}

				// now expand operator
				if ( !pCtorDecl->hasBody() ) {
					unsigned DiagID = m_diags.getCustomDiagID(DiagnosticsEngine::Error,
						"body definition not found for constructor %0::%1");
					m_diags.Report(DiagID)
						<< pCtorDecl->getParentDecl()->getName() << pCtorDecl->getName();

				} else 
					inlineFuncInStmt( dupStmtList, pCtorDecl->getBody(), 0, pNewBaseExpr );

				if (bHasParam) {
					HtDeclIter paramDeclIter(pCtorDecl->getParamDeclList());

					popCallArgs( paramDeclIter );
				}

				pNewExpr = pNewBaseExpr;
			}
			break;
		case Stmt::CXXTemporaryObjectExprClass:
			{
				HtDecl * pCtorDecl = pSrcExpr->getConstructorDecl();
				HtAssert(pCtorDecl != 0);

				// find type for call, returns zero if void
				//HtDecl * pRtnDecl = genVarForFuncReturn( dupStmtList, pSrcExpr->getQualType(), iSrcStmt->getLineInfo() );

				//HtExpr * pObjExpr1 = new HtExpr( Stmt::DeclRefExprClass, pRtnDecl->getQualType(), pRtnDecl->getLineInfo() );
				//pObjExpr1->setRefDecl( pRtnDecl );

				bool bHasParam = pCtorDecl->getParamDeclList().size() > 0;
	
				if (bHasParam) {
					HtDeclIter paramDeclIter(pCtorDecl->getParamDeclList());
					HtStmtIter argExprIter(pSrcExpr->getArgExprList());

					pushCallArgs( dupStmtList, pBaseExpr, paramDeclIter, argExprIter );
				}

				// now expand operator
				if ( !pCtorDecl->hasBody() ) {
					unsigned DiagID = m_diags.getCustomDiagID(DiagnosticsEngine::Error,
						"body definition not found for constructor %0::%1");
					m_diags.Report(DiagID)
						<< pCtorDecl->getParentDecl()->getName() << pCtorDecl->getName();

				} else 
					inlineFuncInStmt( dupStmtList, pCtorDecl->getBody(), 0, pBaseExpr );

				if (bHasParam) {
					HtDeclIter paramDeclIter(pCtorDecl->getParamDeclList());

					popCallArgs( paramDeclIter );
				}
				//if (pRtnDecl) {
				//	// create expression with return variable
				//	pNewExpr = new HtExpr( Stmt::DeclRefExprClass, pRtnDecl->getQualType(), pRtnDecl->getLineInfo() );
				//	pNewExpr->setRefDecl( pRtnDecl );

				//	return pNewExpr;
				//}
			}
			break;
		case Stmt::CXXOperatorCallExprClass:
			{
				HtDecl * pCalleeDecl = pSrcExpr->getCalleeDecl();
				HtAssert(pCalleeDecl != 0);

				if (pSrcExpr->getLineInfo().m_lineNum == 65)
					bool stop = true;

				// find type for call, returns zero if void
				HtDecl * pRtnDecl = genUniqueExprVar( dupStmtList, pSrcExpr->getQualType(), pSrcExpr->getLineInfo() );

				bool bHasParam = pCalleeDecl->getParamDeclList().size() > 0;
	
				// first visit LHS of operator

				HtStmtIter argExprIter(pSrcExpr->getArgExprList());

				HtExpr * pNewBaseExpr = inlineFuncInExpr( dupStmtList, argExprIter.asExpr(), pBaseExpr );
				argExprIter++;

				if (bHasParam) {
					HtDeclIter paramDeclIter(pCalleeDecl->getParamDeclList());

					pushCallArgs( dupStmtList, pBaseExpr, paramDeclIter, argExprIter );
				}

				// now expand operator
				if ( !pCalleeDecl->hasBody() ) {
					unsigned DiagID = m_diags.getCustomDiagID(DiagnosticsEngine::Error,
						"body definition not found for operator %0::%1");
					m_diags.Report(DiagID)
						<< pCalleeDecl->getParentDecl()->getName() << pCalleeDecl->getName();

				} else 	if (pCalleeDecl->isBeingInlined()) {
					unsigned DiagID = m_diags.getCustomDiagID(DiagnosticsEngine::Error,
						"htt/htv do not support recursion");
					m_diags.Report(pCalleeDecl->getLineInfo().getLoc(), DiagID);

				} else {
					pCalleeDecl->isBeingInlined( true );
					inlineFuncInStmt( dupStmtList, pCalleeDecl->getBody(), pRtnDecl, pNewBaseExpr );
					pCalleeDecl->isBeingInlined( false );
				}

				if (bHasParam)
					popCallArgs( HtDeclIter(pCalleeDecl->getParamDeclList()) );

				if (pRtnDecl) {
					// create expression with return variable
					if (isReferenceType(pRtnDecl->getQualType().getType())) {
						pNewExpr = new HtExpr( Stmt::DeclRefExprClass, pRtnDecl->getQualType(), pRtnDecl->getLineInfo() );
						pNewExpr->setRefDecl( pRtnDecl );
					} else {
						pNewExpr = new HtExpr( Stmt::MemberExprClass, pRtnDecl->getQualType(), pRtnDecl->getLineInfo() );
						pNewExpr->setMemberDecl( pRtnDecl );

						HtExpr * pThis = new HtExpr( Stmt::CXXThisExprClass, HtQualType( 0 ), pRtnDecl->getLineInfo() );
						pNewExpr->setBase( pThis );
					}
				}
			}
			break;
		case Stmt::CXXMemberCallExprClass:
			{
				if (pSrcExpr->getNodeId() == 0x41)
					bool stop = true;

				HtDecl * pCalleeDecl = pSrcExpr->getMemberDecl();
				HtAssert(pCalleeDecl != 0);

				// find type for call, returns zero if void
				HtDecl * pRtnDecl = genUniqueExprVar( dupStmtList, pCalleeDecl->getQualType(), pSrcExpr->getLineInfo() );

				HtExpr * pCalleeExpr = pSrcExpr->getCalleeExpr().asExpr();

				HtExpr * pNewBaseExpr = pCalleeExpr->getBase().asExpr();

				if (pNewBaseExpr->getStmtClass() == Stmt::UnaryOperatorClass &&
					pNewBaseExpr->getUnaryOperator() == UO_Deref &&
					pNewBaseExpr->getSubExpr().asExpr()->getStmtClass() == Stmt::CXXThisExprClass)
				{
					pNewBaseExpr = pBaseExpr;
				}

				bool bHasParam = pCalleeDecl->getParamDeclList().size() > 0;

				if (bHasParam) {
					HtStmtIter argExprIter(pSrcExpr->getArgExprList());
					HtDeclIter paramDeclIter(pCalleeDecl->getParamDeclList());

					pushCallArgs( dupStmtList, pBaseExpr, paramDeclIter, argExprIter );
				}

				// now expand operator
				if ( !pCalleeDecl->hasBody() ) {
					unsigned DiagID = m_diags.getCustomDiagID(DiagnosticsEngine::Error,
						"body definition not found for method %0::%1");
					m_diags.Report(DiagID)
						<< pCalleeDecl->getParentDecl()->getName() << pCalleeDecl->getName();

				} else if (pCalleeDecl->isBeingInlined()) {
					unsigned DiagID = m_diags.getCustomDiagID(DiagnosticsEngine::Error,
						"htt/htv do not support recursion");
					m_diags.Report(pCalleeDecl->getLineInfo().getLoc(), DiagID);
				
				} else {
					pCalleeDecl->isBeingInlined( true );
					inlineFuncInStmt( dupStmtList, pCalleeDecl->getBody(), pRtnDecl, pNewBaseExpr );
					pCalleeDecl->isBeingInlined( false );
				}

				if (bHasParam) {
					HtDeclIter paramDeclIter(pCalleeDecl->getParamDeclList());

					popCallArgs( paramDeclIter );
				}

				if (pRtnDecl) {
					// create expression with return variable
					if (isReferenceType(pRtnDecl->getQualType().getType())) {
						pNewExpr = new HtExpr( Stmt::DeclRefExprClass, pRtnDecl->getQualType(), pRtnDecl->getLineInfo() );
						pNewExpr->setRefDecl( pRtnDecl );
					} else {
						pNewExpr = new HtExpr( Stmt::MemberExprClass, pRtnDecl->getQualType(), pRtnDecl->getLineInfo() );
						pNewExpr->setMemberDecl( pRtnDecl );

						HtExpr * pThis = new HtExpr( Stmt::CXXThisExprClass, HtQualType( 0 ), pRtnDecl->getLineInfo() );
						pNewExpr->setBase( pThis );
					}
				}
			}
			break;
		case Stmt::CallExprClass:
			{
				HtDecl * pCalleeDecl = pSrcExpr->getCalleeDecl();
				HtAssert(pCalleeDecl != 0);

				if (pCalleeDecl->isHtPrim() || pCalleeDecl->getGeneralTemplateDecl() &&
					pCalleeDecl->getGeneralTemplateDecl()->isHtPrim())
				{
					// duplicate expression
					pNewExpr = new HtExpr( pSrcExpr );

					pNewExpr->setCalleeExpr( inlineFuncInExpr( dupStmtList, pSrcExpr->getCalleeExpr().asExpr(), pBaseExpr ));

					for (HtStmtIter argExprIter(pSrcExpr->getArgExprList()); !argExprIter.isEol(); argExprIter++)
						pNewExpr->addArgExpr( inlineFuncInExpr( dupStmtList, argExprIter.asExpr(), pBaseExpr ));

					dupStmtList.push_back( pNewExpr );

				} else {

					// find type for call, returns zero if void
					HtDecl * pRtnDecl = genUniqueExprVar( dupStmtList, pSrcExpr->getQualType(), pSrcExpr->getLineInfo() );

					HtDeclIter paramDeclIter(pCalleeDecl->getParamDeclList());
					HtStmtIter argExprIter(pSrcExpr->getArgExprList());

					pushCallArgs( dupStmtList, pBaseExpr, paramDeclIter, argExprIter );

					// now expand call
					if ( !pCalleeDecl->hasBody() ) {

						if (pCalleeDecl->getParentDecl()) {
							unsigned DiagID = m_diags.getCustomDiagID(DiagnosticsEngine::Error,
								"body definition not found for function %0::%1");
							m_diags.Report(DiagID)
								<< pCalleeDecl->getParentDecl()->getName() << pCalleeDecl->getName();
						} else {
							unsigned DiagID = m_diags.getCustomDiagID(DiagnosticsEngine::Error,
								"body definition not found for function %0");
							m_diags.Report(DiagID)
								<< pCalleeDecl->getName();
						}

					} else if (pCalleeDecl->isBeingInlined()) {
						unsigned DiagID = m_diags.getCustomDiagID(DiagnosticsEngine::Error,
							"htt/htv do not support recursion");
						m_diags.Report(pCalleeDecl->getLineInfo().getLoc(), DiagID);

					} else {
						pCalleeDecl->isBeingInlined( true );
						inlineFuncInStmt( dupStmtList, pCalleeDecl->getBody(), pRtnDecl, 0 );
						pCalleeDecl->isBeingInlined( false );
					}

					popCallArgs( HtDeclIter(pCalleeDecl->getParamDeclList()) );
				}
			}
			break;
		case Stmt::DeclRefExprClass:
			{
				HtDecl * pOrigRefDecl = pSrcExpr->getRefDecl();
				assert(pOrigRefDecl);

				HtDecl * pInlineDecl = pOrigRefDecl->getInlineDecl();

				if (pInlineDecl) {
					if (isReferenceType(pInlineDecl->getQualType().getType())) {
						assert(pInlineDecl->getKind() == Decl::Var);

						pNewExpr = new HtExpr( pSrcExpr );

						HtDecl * pRefDecl = pSrcExpr->getRefDecl()->getInlineDecl();

						pNewExpr->setRefDecl( pRefDecl );
					} else {
						assert(pInlineDecl->getKind() == Decl::Field);

						pNewExpr = new HtExpr( Stmt::MemberExprClass, pOrigRefDecl->getQualType(), pOrigRefDecl->getLineInfo() );
						pNewExpr->setMemberDecl( pInlineDecl );

						HtExpr * pThis = new HtExpr( Stmt::CXXThisExprClass, HtQualType( 0 ), pOrigRefDecl->getLineInfo() );
						pNewExpr->setBase( pThis );
					}
				} else {

					pNewExpr = new HtExpr( pSrcExpr );

					pNewExpr->setRefDecl( pOrigRefDecl );
				}


			}
			break;
		case Stmt::MemberExprClass:
			pNewExpr = new HtExpr( pSrcExpr );
			pNewExpr->setBase( inlineFuncInExpr( dupStmtList, pSrcExpr->getBase().asExpr(), pBaseExpr ));

			if (pSrcExpr->getBase()->getStmtClass() == Stmt::CXXThisExprClass)
				pNewExpr->isArrow(false);

			break;
		case Stmt::CXXThisExprClass:
			if (pBaseExpr == 0 || pBaseExpr->getStmtClass() == Stmt::CXXThisExprClass) {
				assert(pSrcExpr->getStmtClass() == Stmt::CXXThisExprClass);
				pNewExpr = pSrcExpr;
			} else
				pNewExpr = inlineFuncInExpr( dupStmtList, pBaseExpr, 0 );
			break;
		case Stmt::CXXFunctionalCastExprClass:
		case Stmt::ImplicitCastExprClass:
		case Stmt::CXXStaticCastExprClass:
			pNewExpr = new HtExpr( pSrcExpr );
			pNewExpr->setSubExpr( inlineFuncInExpr( dupStmtList, pSrcExpr->getSubExpr().asExpr(), pBaseExpr ));
			break;
		case Stmt::InitListExprClass:
			{
				pNewExpr = new HtExpr( pSrcExpr );

				for (HtStmtIter exprIter(pSrcExpr->getInitExprList()); !exprIter.isEol(); exprIter++)
				{
					pNewExpr->addInitExpr( inlineFuncInExpr( dupStmtList, exprIter.asExpr(), pBaseExpr ) );
				}
			}
			break;
		case Stmt::ArraySubscriptExprClass:
			pNewExpr = new HtExpr( pSrcExpr );
			pNewExpr->setBase( inlineFuncInExpr( dupStmtList, pSrcExpr->getBase().asExpr(), pBaseExpr ));
			pNewExpr->setIdx( inlineFuncInExpr( dupStmtList, pSrcExpr->getIdx().asExpr(), pBaseExpr ));
			break;
		case Stmt::MaterializeTemporaryExprClass:
			{
				// declare variable
				bool stop = true;
				//assert(0);
				//HtDecl * pDecl = new HtDecl( Decl::Var, iSrcStmt->getLineInfo() );
				//pDecl->setType( pSrcExpr->getQualType().getType() );
				//pDecl->isTempVar( true );
				//pDecl->setName( findUniqueName( "te", iSrcStmt->getLineInfo() ));

				//HtStmt * pStmt = new HtStmt( Stmt::DeclStmtClass, iSrcStmt->getLineInfo() );
				//pStmt->addDecl( pDecl );
				//dupStmtList.push_back( pStmt );

				//pTempExpr = new HtExpr( Stmt::DeclRefExprClass, pSrcExpr->getQualType(), iSrcStmt->getLineInfo() );
				//pTempExpr->setRefDecl( pDecl );

				pNewExpr = inlineFuncInExpr( dupStmtList, pSrcExpr->getSubExpr().asExpr(), pBaseExpr );

				//pNewExpr = pTempExpr;
			}
			break;
		case Stmt::StringLiteralClass:
		case Stmt::IntegerLiteralClass:
		case Stmt::CXXBoolLiteralExprClass:
			pNewExpr = new HtExpr( pSrcExpr );
			break;
		case Stmt::ParenExprClass:
			pNewExpr = new HtExpr( pSrcExpr );
			pNewExpr->setSubExpr( inlineFuncInExpr( dupStmtList, pSrcExpr->getSubExpr().asExpr(), pBaseExpr ));
			break;
		case Stmt::CStyleCastExprClass:
			pNewExpr = new HtExpr( pSrcExpr );
			pNewExpr->setSubExpr( inlineFuncInExpr( dupStmtList, pSrcExpr->getSubExpr().asExpr(), pBaseExpr ));
			break;
		default:
			assert(0);
		}

		return pNewExpr;
	}

	HtDecl * HtInlineFunctionCalls::genUniqueExprVar( HtStmtList & dupStmtList, HtQualType & qualType, HtLineInfo & lineInfo )
	{
		HtType * pType = qualType.getType();

		if (pType->getTypeClass() == Type::FunctionProto)
			pType = pType->getQualType().getType();

		if (pType->getTypeClass() == Type::SubstTemplateTypeParm)
			pType = pType->getQualType().getType();

		if (pType->getTypeClass() == Type::Builtin && pType->getBuiltinTypeKind() == BuiltinType::Void)
			pType = 0;

		if (pType) {

			HtDecl * pNewDecl = 0;
			if (isReferenceType(pType)) {

				// declare local variable
				pNewDecl = new HtDecl( Decl::Var, lineInfo );

				HtStmt * pStmt = new HtStmt( Stmt::DeclStmtClass, lineInfo );
				pStmt->addDecl( pNewDecl );

				pNewDecl->setType( pType );
				pNewDecl->isTempVar( true );
				pNewDecl->setName( findUniqueName( "ce", lineInfo ));

				dupStmtList.push_back( pStmt );

			} else {
				// declare member variable
				pNewDecl = new HtDecl( Decl::Field, lineInfo );

				pNewDecl->setType( pType );
				pNewDecl->isTempVar( true );
				pNewDecl->setName( findUniqueName( "ce", lineInfo ));

				m_pHtDesign->getScModuleDecl()->addSubDecl( pNewDecl );
			}

			return pNewDecl;
		} else
			return 0;
	}

	void HtInlineFunctionCalls::pushCallArgs( HtStmtList & dupStmtList, HtExpr * pBaseExpr,
		HtDeclIter & paramDeclIter, HtStmtIter & argExprIter )
	{
		while ( !argExprIter.isEol() && !paramDeclIter.isEol() )
		{
			HtDecl * pParamDecl = * paramDeclIter;

			if (argExprIter->getStmtClass() == Stmt::CXXConstructExprClass) {

				HtExpr * pConstructExpr = inlineFuncInExpr( dupStmtList, argExprIter.asExpr(), pBaseExpr );

				pParamDecl->pushInlineDecl( pConstructExpr->getRefDecl() );

			} else {

				string paramName = pParamDecl->getName();
				if (paramName.size() == 0)
					paramName = "pv";

				HtDecl * pNewDecl;
				if (isReferenceType(pParamDecl->getQualType().getType())) {

					// create copy of decl as local variable
					pNewDecl = new HtDecl( pParamDecl, false );
					pNewDecl->setKind( Decl::Var );

					HtStmt * pDeclStmt = new HtStmt( Stmt::DeclStmtClass, argExprIter->getLineInfo() );
					pDeclStmt->addDecl( pNewDecl );

					pNewDecl->isTempVar( true );
					pNewDecl->setName( findUniqueName( paramName, argExprIter->getLineInfo() ));

					dupStmtList.push_back( pDeclStmt );

				} else {

					// create copy of decl as member of SC_MODULE
					pNewDecl = new HtDecl( Decl::Field, argExprIter->getLineInfo() );
					pNewDecl->setQualType( pParamDecl->getQualType() );
					pNewDecl->setDesugaredQualType( pParamDecl->getDesugaredQualType() );

					pNewDecl->isTempVar( true );
					pNewDecl->setName( findUniqueName( paramName, argExprIter->getLineInfo() ));

					m_pHtDesign->getScModuleDecl()->addSubDecl( pNewDecl );
				}

				pParamDecl->pushInlineDecl( pNewDecl );


				pNewDecl->setInitExpr( inlineFuncInExpr( dupStmtList, argExprIter.asExpr(), pBaseExpr ) );
				
				if (!isReferenceType(pParamDecl->getQualType().getType())) {
					HtExpr thisExpr( Stmt::CXXThisExprClass, HtQualType( 0 ), argExprIter->getLineInfo() );
					handleInitDecl(dupStmtList, pNewDecl, pNewDecl, &thisExpr);//pBaseExpr ? pBaseExpr->getBase().asExpr() : 0);
				}
			}

			paramDeclIter++;
			argExprIter++;
		}
		
		assert(paramDeclIter.isEol());
		assert(argExprIter.isEol());
	}

	void HtInlineFunctionCalls::popCallArgs( HtDeclIter & paramDeclIter )
	{
		while ( !paramDeclIter.isEol() )
		{
			HtDecl * pParamDecl = * paramDeclIter;

			pParamDecl->popInlineDecl();

			paramDeclIter++;
		}
	}

	bool HtInlineFunctionCalls::isReferenceType(HtType * pHtType)
	{
		return pHtType->getTypeClass() == Type::LValueReference
			|| pHtType->getTypeClass() == Type::RValueReference;
	}

}
