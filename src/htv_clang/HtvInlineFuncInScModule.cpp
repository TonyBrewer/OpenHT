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

	class HtInlineFuncInScModule
	{
	public:
		HtInlineFuncInScModule(HtDesign * pHtDesign) {
			m_pHtDesign = pHtDesign;
		}

		void inlineFuncInDeclList(HtDeclList_t & declList, HtStmtList_t * pStmtList, HtExpr * pObjExpr, HtDeclIter & declIter);
		HtDecl * inlineFuncInDecl( HtStmtList_t * pStmtList, HtExpr * pObjExpr, HtDecl * pOrigDecl);
		void inlineFuncInStmtList(HtStmtList_t & stmtList, HtDecl * pRtnDecl, HtExpr * pObjExpr, HtStmtIter & stmtIter);
		void inlineFuncInStmt(HtStmtList_t & stmtList, HtDecl * pRtnDecl, HtExpr * pObjExpr, HtStmt * pOrigStmt);
		HtStmt * inlineFuncInStmt( HtDecl * pRtnDecl, HtExpr * pObjExpr, HtStmt * pOrigStmt );
		HtExpr * inlineFuncInExpr(HtStmtList_t & stmtList, HtExpr * pObjExpr, HtExpr * pOrigExpr);
		void declCallArgs( HtStmtList_t & stmtList, HtExpr * pObjExpr, HtDeclIter & paramDeclIter, HtExprIter & argExprIter );
		HtDecl * genVarForFuncReturn( HtStmtList_t & stmtList, HtQualType & qualType, HtLineInfo & lineInfo );

		string findUniqueName( string name, HtLineInfo & lineInfo ) { return m_pHtDesign->findUniqueName( name, lineInfo ); }

		//void handleDeclStmt(HtStmtList_t & stmtList, HtExpr * pObjExpr, HtStmt * pOrigStmt);

	private:
		HtDesign * m_pHtDesign;
		HtDecl * m_pScModuleDecl;
	};

	void HtDesign::inlineFuncInScModule()
	{
		HtInlineFuncInScModule transform(this);

		HtDeclList_t & declList = getTopDeclList();
		HtDeclIter declIter(declList);

		// set new top decl list for transform
		setTopDeclList( new HtDeclList_t );

		// start with top level declarations
		transform.inlineFuncInDeclList( getTopDeclList(), 0, 0, declIter );
	}

	void HtInlineFuncInScModule::inlineFuncInDeclList(HtDeclList_t & declList, HtStmtList_t * pStmtList, HtExpr * pObjExpr, HtDeclIter & declIter)
	{
		for ( ; !declIter.isEol(); declIter++ )
		{
			HtDecl * pOrigDecl = *declIter;

			if (pOrigDecl->hasCopyDecl()) {
				declList.push_back( pOrigDecl->getCopyDecl() );
				continue;
			}

			HtDecl * pNewDecl = inlineFuncInDecl( pStmtList, pObjExpr, *declIter );
			if (pNewDecl)
				declList.push_back( pNewDecl );
		}
	}

	HtDecl * HtInlineFuncInScModule::inlineFuncInDecl( HtStmtList_t * pStmtList, HtExpr * pObjExpr, HtDecl * pOrigDecl)
	{
		if (pOrigDecl->hasCopyDecl())
			return pOrigDecl->getCopyDecl();

		//// first step is to put variables in ScModule in flat name table
		//if (pHtDecl->isScModule()) {
		//	for (HtDeclList_iter declIter = pHtDecl->getSubDeclList().begin();
		//		declIter != pHtDecl->getSubDeclList().end(); declIter++)
		//	{
		//		HtDecl * pSubDecl = declIter();
		//		if (pSubDecl->getKind() != Decl::Field)
		//			continue;

		//		pSubDecl->setFlatName( m_pHtDesign->addFlatName( pSubDecl->getName() ));
		//	}
		//}
		HtDecl * pNewDecl = 0;

		switch (pOrigDecl->getKind()) {
		case Decl::Typedef:
			pNewDecl = new HtDecl( pOrigDecl );
			break;
		case Decl::ClassTemplate:
			pNewDecl = new HtDecl( pOrigDecl );

			for (HtDeclList_iter_t declIter = pOrigDecl->getParamDeclList().begin();
				declIter != pOrigDecl->getParamDeclList().end(); declIter++ )
			{
				pNewDecl->addParamDecl( inlineFuncInDecl( pStmtList, pObjExpr, *declIter ));
			}

			pNewDecl->setTemplateDecl( inlineFuncInDecl( pStmtList, pObjExpr, pOrigDecl->getTemplateDecl() ));
			break;
		case Decl::NonTypeTemplateParm:
			pNewDecl = new HtDecl( pOrigDecl );
			break;
		case Decl::TemplateTypeParm:
			pNewDecl = new HtDecl( pOrigDecl );
			break;
		case Decl::CXXMethod:
			{
				if (!pOrigDecl->isScMethod())
					break;

				assert(pOrigDecl->getParentDecl()->isScModule());
				m_pScModuleDecl = pOrigDecl->getParentDecl();

				pNewDecl = new HtDecl( pOrigDecl );

				pNewDecl->setParentDecl( inlineFuncInDecl( pStmtList, pObjExpr, pOrigDecl->getParentDecl() ));

				if (pOrigDecl->doesThisDeclarationHaveABody()) {
					// Make copy of method body
					HtStmt * pOrigStmt = pOrigDecl->getBody();
					assert(pOrigStmt->getStmtClass() == Stmt::CompoundStmtClass);

					HtStmt * pNewStmt = new HtStmt( Stmt::CompoundStmtClass );
					pNewDecl->setBody( pNewStmt );

					HtStmtIter stmtIter( pOrigStmt->getStmtList() );

					inlineFuncInStmtList( pNewStmt->getStmtList(), 0, 0, stmtIter );
				}
			}
			break;
		case Decl::Var:
			{
				pNewDecl = new HtDecl( pOrigDecl );

				if (pOrigDecl->hasInitExpr())
					pNewDecl->setInitExpr( inlineFuncInExpr( *pStmtList, pObjExpr, pOrigDecl->getInitExpr() ));
			}
			break;
		case Decl::Field:
			{
				pNewDecl = new HtDecl( pOrigDecl );

				if (pOrigDecl->hasBitWidthExpr())
					pNewDecl->setBitWidthExpr( inlineFuncInExpr( *pStmtList, pObjExpr, pOrigDecl->getBitWidthExpr() ));
			}
			break;
		case Decl::IndirectField:
			pNewDecl = new HtDecl( pOrigDecl );
			break;
		case Decl::CXXRecord:
			{
				pNewDecl = new HtDecl( pOrigDecl );

				for (HtBaseSpecifier * pBaseSpec = pOrigDecl->getFirstBaseSpecifier();
					pBaseSpec; pBaseSpec = pBaseSpec->getNextBaseSpec())
				{
					pNewDecl->addBaseSpecifier( new HtBaseSpecifier( pBaseSpec ));
				}

				// add public accessSpec if a class type record
				if (pOrigDecl->getTagKind() == TTK_Class) {
					HtDecl * pAccessDecl = new HtDecl( Decl::AccessSpec );
					pAccessDecl->setAccess( AS_public );
					pNewDecl->addSubDecl( pAccessDecl );
				}

				for (HtDeclList_iter_t declIter = pOrigDecl->getSubDeclList().begin();
					declIter != pOrigDecl->getSubDeclList().end(); declIter++)
				{
					HtDecl * pDecl = inlineFuncInDecl( pStmtList, pObjExpr, *declIter );
					if (pDecl)
						pNewDecl->addSubDecl( pDecl );
				}
			}
			break;
		case Decl::AccessSpec:
		case Decl::CXXConstructor:
		case Decl::CXXDestructor:
		case Decl::Function:
		case Decl::CXXConversion:
			break;
		default:
			assert(0);
		}

		return pNewDecl;
	}

	void HtInlineFuncInScModule::inlineFuncInStmtList(HtStmtList_t & stmtList, HtDecl * pRtnDecl, HtExpr * pObjExpr, HtStmtIter & stmtIter)
	{
		for ( ; !stmtIter.isEol(); stmtIter++ ) {
			HtStmt * pOrigStmt = * stmtIter;

			inlineFuncInStmt( stmtList, pRtnDecl, pObjExpr, pOrigStmt );
		}
	}

	HtStmt * HtInlineFuncInScModule::inlineFuncInStmt( HtDecl * pRtnDecl, HtExpr * pObjExpr, HtStmt * pOrigStmt )
	{
		if (pOrigStmt == 0)
			return 0;

		HtStmt * pStmt = new HtStmt( Stmt::CompoundStmtClass );
		inlineFuncInStmt( pStmt->getStmtList(), pRtnDecl, pObjExpr, pOrigStmt );

		// if only one stmt then we didn't need the compound stmt
		if (pStmt->getStmtList().size() > 1)
			return pStmt;

		assert( pStmt->getStmtList().size() == 1 );

		HtStmt * pNewStmt = * pStmt->getStmtList().begin();
		delete pStmt;
		return pNewStmt;
	}

	void HtInlineFuncInScModule::inlineFuncInStmt( HtStmtList_t & stmtList, HtDecl * pRtnDecl, HtExpr * pObjExpr, HtStmt * pOrigStmt )
	{
		if (pOrigStmt->isAssignStmt()) {
			HtExpr * pInlineExpr = inlineFuncInExpr( stmtList, pObjExpr, (HtExpr *)pOrigStmt );
			if (pInlineExpr != 0) {
				pInlineExpr->isAssignStmt(true);
				stmtList.push_back( pInlineExpr );
			}

			return;
		}

		switch (pOrigStmt->getStmtClass()) {
		case Stmt::CompoundStmtClass:
			{
				HtStmt * pHtStmt = new HtStmt( pOrigStmt );
				HtStmtIter subStmtIter( pOrigStmt->getStmtList() );
				inlineFuncInStmtList( pHtStmt->getStmtList(), pRtnDecl, pObjExpr, subStmtIter );
				stmtList.push_back( pHtStmt );
			}
			break;
		case Stmt::IfStmtClass:
			{
				HtStmt * pHtStmt = new HtStmt( pOrigStmt );
				pHtStmt->setCond( inlineFuncInExpr( stmtList, pObjExpr, pOrigStmt->getCond() ));
				pHtStmt->setThen( inlineFuncInStmt( pRtnDecl, pObjExpr, pOrigStmt->getThen() ));
				pHtStmt->setElse( inlineFuncInStmt( pRtnDecl, pObjExpr, pOrigStmt->getElse() ));
				stmtList.push_back( pHtStmt );
			}
			break;
		case Stmt::SwitchStmtClass:
			{
				HtStmt * pHtStmt = new HtStmt( pOrigStmt );
				pHtStmt->setCond( inlineFuncInExpr( stmtList, pObjExpr, pOrigStmt->getCond() ));
				pHtStmt->setBody( inlineFuncInStmt( pRtnDecl, pObjExpr, pOrigStmt->getBody() ));
				stmtList.push_back( pHtStmt );
			}
			break;
		case Stmt::CaseStmtClass:
			{
				HtStmt * pHtStmt = new HtStmt( pOrigStmt );
				pHtStmt->setLHS( inlineFuncInExpr( stmtList, pObjExpr, pOrigStmt->getLHS() ));
				pHtStmt->setSubStmt( inlineFuncInStmt( pRtnDecl, pObjExpr, pOrigStmt->getSubStmt() ));
				stmtList.push_back( pHtStmt );
			}
			break;
		case Stmt::DefaultStmtClass:
			{
				HtStmt * pHtStmt = new HtStmt( pOrigStmt );
				pHtStmt->setSubStmt( inlineFuncInStmt( pRtnDecl, pObjExpr, pOrigStmt->getSubStmt() ));
				stmtList.push_back( pHtStmt );
			}
			break;
		case Stmt::BreakStmtClass:
			{
				HtStmt * pHtStmt = new HtStmt( pOrigStmt );
				stmtList.push_back( pHtStmt );
			}
			break;
		case Stmt::DeclStmtClass:
			{
				HtStmt * pHtStmt = new HtStmt( pOrigStmt );

				HtDeclList_t & declList = pOrigStmt->getDeclList();
				for (HtDeclList_iter_t declIter = declList.begin();
					declIter != declList.end(); declIter++)
				{
					pHtStmt->addDecl( inlineFuncInDecl( &stmtList, pObjExpr, *declIter ));
				}

				stmtList.push_back( pHtStmt );
			}
			break;
		case Stmt::NullStmtClass:
			{
				HtStmt * pHtStmt = new HtStmt( pOrigStmt );
				stmtList.push_back( pHtStmt );
			}
			break;
		case Stmt::ReturnStmtClass:
			if (pRtnDecl) {
				HtExpr * pHtRhs = inlineFuncInExpr( stmtList, pObjExpr, pOrigStmt->getSubExpr() );

				HtExpr * pHtLhs = new HtExpr( Stmt::DeclRefExprClass );
				pHtLhs->setRefDecl( pRtnDecl );

				HtExpr * pHtAssign = new HtExpr( Stmt::BinaryOperatorClass );
				pHtAssign->setBinaryOperator( BO_Assign );
				pHtAssign->isAssignStmt( true );
				pHtAssign->setRHS( pHtRhs );
				pHtAssign->setLHS( pHtLhs );

				stmtList.push_back( pHtAssign );
			}
			break;
		default:
			assert(0);
		}
	}

	//void HtInlineFuncInScModule::handleDeclStmt(HtStmtList_t & stmtList, HtExpr * pObjExpr, HtStmt * pOrigStmt)
	//{
	//	// found a declaration statement within a ScMethod or called function
	//	// - need to move declarations to ScModule
	//	// - need to turn initialization into init statement here

	//	// step one: create copy of decl statement and mark as a init statement
	//	//   The isInitStmt is used to indicate to perform the init, but don't generate a declaration
	//	HtStmt * pDeclStmt = new HtStmt( pOrigStmt->getStmtClass() );
	//	pDeclStmt->isInitStmt( true );

	//	// walk through individual decls within statement
	//	for (HtDecl ** ppDecl = pOrigStmt->getPFirstDecl(); *ppDecl; ppDecl = (*ppDecl)->getPNextDecl()) {
	//		HtDecl * pHtDecl = *ppDecl;

	//		string flatName = m_pHtDesign->addFlatName( pHtDecl->getName() );

	//		// step two: insert top level decl for each decl
	//		{
	//			// create copy of decl for top level
	//			HtDecl * pTopDecl = new HtDecl( pHtDecl );
	//			pHtDecl->setInlineDecl( pTopDecl );
	//			pTopDecl->setInitExpr(0);

	//			// get unique name for variable
	//			pTopDecl->setFlatName( flatName );

	//			// add to ScModule top list
	//			m_pScModuleDecl->addSubDecl( pTopDecl );
	//		}

	//		// step three: create copy of decl for local statement
	//		if (pHtDecl->getInitExpr()) {
	//			HtDecl * pLocalDecl = new HtDecl( pHtDecl );
	//			pLocalDecl->setInitExpr( inlineFuncInExpr(stmtList, pObjExpr, pHtDecl->getInitExpr() ));

	//			// get unique name
	//			pLocalDecl->setFlatName( flatName );

	//			// add to local declStmt
	//			pDeclStmt->addDecl( pLocalDecl );
	//		}
	//	}

	//	// add decl stmt to list of inlined statements
	//	if (pDeclStmt->getFirstDecl())
	//		stmtList.push_back( pDeclStmt );
	//	else
	//		delete pDeclStmt;
	//}

	HtExpr * HtInlineFuncInScModule::inlineFuncInExpr(HtStmtList_t & stmtList, HtExpr * pObjExpr, HtExpr * pOrigExpr)
	{
		// walk expression in the order of execution
		HtExpr * pNewExpr = 0;

		switch (pOrigExpr->getStmtClass()) {
		case Stmt::CompoundAssignOperatorClass:
			pNewExpr = new HtExpr( pOrigExpr );
			pNewExpr->setLHS( inlineFuncInExpr( stmtList, pObjExpr, pOrigExpr->getLHS() ));
			pNewExpr->setRHS( inlineFuncInExpr( stmtList, pObjExpr, pOrigExpr->getRHS() ));
			break;
		case Stmt::UnaryOperatorClass:
			pNewExpr = new HtExpr( pOrigExpr );
			pNewExpr->setSubExpr( inlineFuncInExpr( stmtList, pObjExpr, pOrigExpr->getSubExpr() ));
			break;
		case Stmt::BinaryOperatorClass:
			pNewExpr = new HtExpr( pOrigExpr );
			pNewExpr->setLHS( inlineFuncInExpr( stmtList, pObjExpr, pOrigExpr->getLHS() ));
			pNewExpr->setRHS( inlineFuncInExpr( stmtList, pObjExpr, pOrigExpr->getRHS() ));
			break;
		case Stmt::ConditionalOperatorClass:
			pNewExpr = new HtExpr( pOrigExpr );
			pNewExpr->setCond( inlineFuncInExpr( stmtList, pObjExpr, pOrigExpr->getCond() ));
			pNewExpr->setLHS( inlineFuncInExpr( stmtList, pObjExpr, pOrigExpr->getLHS() ));
			pNewExpr->setRHS( inlineFuncInExpr( stmtList, pObjExpr, pOrigExpr->getRHS() ));
			break;
		case Stmt::CXXConstructExprClass:
			{
				HtAssert(pOrigExpr->getConstructorDecl() != 0);
				if (pOrigExpr->getConstructorDecl()->getBody() == 0)
					printf("Body definition not found for constructor, %s::%s\n", 
						pOrigExpr->getConstructorDecl()->getParentDecl()->getName().c_str(),
						pOrigExpr->getConstructorDecl()->getName().c_str());
				pNewExpr = new HtExpr( pOrigExpr );

				HtExprList_t & argList = pOrigExpr->getArgExprList();
				for (HtExprList_iter_t argIter = argList.begin();
					argIter != argList.end(); argIter++ )
				{
					pNewExpr->addArgExpr( inlineFuncInExpr( stmtList, pObjExpr, * argIter ));
				}
			}
			break;
		case Stmt::CXXOperatorCallExprClass:
			{
				HtDecl * pCalleeDecl = pOrigExpr->getCalleeDecl();
				HtAssert(pCalleeDecl != 0);

				// find type for call, returns zero if void
				HtDecl * pRtnDecl = 0;
				if (!pOrigExpr->isAssignStmt())
					pRtnDecl = genVarForFuncReturn( stmtList, pOrigExpr->getQualType(), pOrigExpr->getLineInfo() );

				bool bHasParam = pCalleeDecl->getParamDeclList().size() > 0;
				HtDeclIter paramDeclIter(pCalleeDecl->getParamDeclList());
	
				HtExprIter argExprIter(pOrigExpr->getArgExprList());

				// first visit LHS of operator
				HtExpr * pOrigLhsExpr = * argExprIter++;
				HtExpr * pNewBaseExpr = inlineFuncInExpr( stmtList, pObjExpr, pOrigLhsExpr );

				HtStmtList_t * pInlineStmtList = &stmtList;
				if (bHasParam) {
					HtStmt * pHtStmt = new HtStmt( Stmt::CompoundStmtClass );
					stmtList.push_back( pHtStmt );

					pInlineStmtList = & pHtStmt->getStmtList();

					declCallArgs( *pInlineStmtList, pObjExpr, paramDeclIter, argExprIter );
				}

				// now expand operator
				if (pCalleeDecl->getBody() == 0)
					printf("Body definition not found for operator call, %s::%s\n", 
						pCalleeDecl->getParentDecl()->getName().c_str(),
						pCalleeDecl->getName().c_str());
				else
					inlineFuncInStmt( *pInlineStmtList, pRtnDecl, pNewBaseExpr, pCalleeDecl->getBody() );

				if (pRtnDecl) {
					// create expression with return variable
					pNewExpr = new HtExpr( Stmt::DeclRefExprClass );
					pNewExpr->setRefDecl( pRtnDecl );

					return pNewExpr;
				}
			}
			break;
		case Stmt::CXXMemberCallExprClass:
			{
				HtDecl * pCalleeDecl = pOrigExpr->getMemberDecl();
				HtAssert(pCalleeDecl != 0);

				// find type for call, returns zero if void
				HtDecl * pRtnDecl = 0;
				if (!pOrigExpr->isAssignStmt())
					pRtnDecl = genVarForFuncReturn( stmtList, pOrigExpr->getQualType(), pOrigExpr->getLineInfo() );

				HtExpr * pCalleeExpr = pOrigExpr->getCallee();
				HtExpr * pNewBaseExpr = inlineFuncInExpr( stmtList, pObjExpr, pCalleeExpr->getBase() );

				bool bHasParam = pCalleeDecl->getParamDeclList().size() > 0;

				HtStmtList_t * pInlineStmtList = &stmtList;
				if (bHasParam) {
					HtStmt * pHtStmt = new HtStmt( Stmt::CompoundStmtClass );
					stmtList.push_back( pHtStmt );

					pInlineStmtList = & pHtStmt->getStmtList();

					HtExprIter argExprIter(pOrigExpr->getArgExprList());
					HtDeclIter paramDeclIter(pCalleeDecl->getParamDeclList());

					declCallArgs( *pInlineStmtList, pObjExpr, paramDeclIter, argExprIter );
				}

				// now expand operator
				if (pCalleeDecl->getBody() == 0)
					printf("Body definition not found for member call, %s::%s\n", 
						pCalleeDecl->getParentDecl()->getName().c_str(),
						pCalleeDecl->getName().c_str());
				else
					inlineFuncInStmt( *pInlineStmtList, pRtnDecl, pNewBaseExpr, pCalleeDecl->getBody() );

				if (pRtnDecl) {
					// create expression with return variable
					pNewExpr = new HtExpr( Stmt::DeclRefExprClass );
					pNewExpr->setRefDecl( pRtnDecl );

					return pNewExpr;
				}
			}
			break;
		case Stmt::CallExprClass:
			{
				HtDecl * pCalleeDecl = pOrigExpr->getCalleeDecl();
				HtAssert(pCalleeDecl != 0);

				// find type for call, returns zero if void
				HtDecl * pRtnDecl = 0;
				if (!pOrigExpr->isAssignStmt())
					pRtnDecl = genVarForFuncReturn( stmtList, pOrigExpr->getQualType(), pOrigExpr->getLineInfo() );

				HtStmt * pHtStmt = new HtStmt( Stmt::CompoundStmtClass );
				stmtList.push_back( pHtStmt );

				HtStmtList_t & inlineStmtList = pHtStmt->getStmtList();

				HtDeclIter paramDeclIter(pCalleeDecl->getParamDeclList());
				HtExprIter argExprIter(pOrigExpr->getArgExprList());

				declCallArgs( inlineStmtList, pObjExpr, paramDeclIter, argExprIter );

				// now expand call
				if (pCalleeDecl->getBody() == 0)
					printf("Body definition not found for function call, %s::%s\n", 
						pCalleeDecl->getParentDecl()->getName().c_str(),
						pCalleeDecl->getName().c_str());
				else
					inlineFuncInStmt( inlineStmtList, pRtnDecl, 0, pCalleeDecl->getBody() );
			}
			break;
		case Stmt::DeclRefExprClass:
			assert(pOrigExpr->getRefDecl());

			pNewExpr = new HtExpr( pOrigExpr );

			HtDecl * pRefDecl;
			if (pOrigExpr->getRefDecl()->getInlineDecl())
				pRefDecl = pOrigExpr->getRefDecl()->getInlineDecl();
			else
				pRefDecl = pOrigExpr->getRefDecl();

			pNewExpr->setRefDecl( pRefDecl );
			break;
		case Stmt::MemberExprClass:
			pNewExpr = new HtExpr( pOrigExpr );
			pNewExpr->setSubExpr( inlineFuncInExpr( stmtList, pObjExpr, pOrigExpr->getSubExpr() ));

			if (pOrigExpr->getSubExpr()->getStmtClass() == Stmt::CXXThisExprClass)
				pNewExpr->isArrow(false);

			break;
		case Stmt::CXXThisExprClass:
			if (pObjExpr == 0)
				pNewExpr = pOrigExpr;
			else
				pNewExpr = pObjExpr;
			break;
		case Stmt::ImplicitCastExprClass:
		case Stmt::CXXStaticCastExprClass:
			pNewExpr = new HtExpr( pOrigExpr );
			pNewExpr->setSubExpr( inlineFuncInExpr( stmtList, pObjExpr, pOrigExpr->getSubExpr() ));
			break;
		case Stmt::InitListExprClass:
			{
				pNewExpr = new HtExpr( pOrigExpr );

				HtExprList_t & exprList = pOrigExpr->getInitExprList();
				for (HtExprList_iter_t exprIter = exprList.begin();
					exprIter != exprList.end(); exprIter++)
				{
					pNewExpr->addInitExpr( inlineFuncInExpr( stmtList, pObjExpr, *exprIter ) );
				}
			}
			break;
		case Stmt::ArraySubscriptExprClass:
			pNewExpr = new HtExpr( pOrigExpr );
			pNewExpr->setBase( inlineFuncInExpr( stmtList, pObjExpr, pOrigExpr->getBase() ));
			pNewExpr->setIdx( inlineFuncInExpr( stmtList, pObjExpr, pOrigExpr->getIdx() ));
			break;
		case Stmt::MaterializeTemporaryExprClass:
			pNewExpr = new HtExpr( pOrigExpr );
			pNewExpr->setTemporaryExpr( inlineFuncInExpr( stmtList, pObjExpr, pOrigExpr->getTemporaryExpr() ));
			break;
		case Stmt::IntegerLiteralClass:
		case Stmt::CXXBoolLiteralExprClass:
			pNewExpr = new HtExpr( pOrigExpr );
			break;
		case Stmt::ParenExprClass:
			pNewExpr = new HtExpr( pOrigExpr );
			pNewExpr->setSubExpr( inlineFuncInExpr( stmtList, pObjExpr, pOrigExpr->getSubExpr() ));
			break;
		case Stmt::CStyleCastExprClass:
			pNewExpr = new HtExpr( pOrigExpr );
			pNewExpr->setSubExpr( inlineFuncInExpr( stmtList, pObjExpr, pOrigExpr->getSubExpr() ));
			break;
		default:
			assert(0);
		}

		return pNewExpr;
	}

	HtDecl * HtInlineFuncInScModule::genVarForFuncReturn( HtStmtList_t & stmtList, HtQualType & qualType, HtLineInfo & lineInfo )
	{
		HtType * pType = qualType.getType();

		if (pType->getTypeClass() == Type::FunctionProto)
			pType = pType->getQualType().getType();

		if (pType->getTypeClass() == Type::SubstTemplateTypeParm)
			pType = pType->getQualType().getType();

		if (pType->getTypeClass() == Type::Builtin && pType->getBuiltinTypeKind() == BuiltinType::Void)
			pType = 0;

		if (pType) {
			// declare variable
			HtDecl * pDecl = new HtDecl( Decl::Var );
			pDecl->setType( pType );

			pDecl->setName( findUniqueName( "rv", lineInfo ));

			HtStmt * pStmt = new HtStmt( Stmt::DeclStmtClass );
			pStmt->addDecl( pDecl );

			stmtList.push_back( pStmt );

			return pDecl;
		} else
			return 0;
	}

	void HtInlineFuncInScModule::declCallArgs( HtStmtList_t & stmtList, HtExpr * pObjExpr,
		HtDeclIter & paramDeclIter, HtExprIter & argExprIter )
	{
		while ( !argExprIter.isEol() && !paramDeclIter.isEol() )
		{
			HtDecl * pParamDecl = * paramDeclIter;
			HtExpr * pOrigArgExpr = * argExprIter;

			// create copy of decl as local variable
			HtDecl * pNewVarDecl = new HtDecl( pParamDecl, false );
			pNewVarDecl->setKind( Decl::Var );

			if (pNewVarDecl->getType()->getTypeClass() == Type::RValueReference)
				pNewVarDecl->getQualType().setType( pNewVarDecl->getType()->getQualType().getType() );

			pParamDecl->setInlineDecl( pNewVarDecl );

			string paramName = pParamDecl->getName();
			if (paramName.size() == 0)
				paramName = "pv";

			pNewVarDecl->setName( findUniqueName( paramName, pOrigArgExpr->getLineInfo() ));

			//pTopParamDecl->setFlatName( m_pHtDesign->addFlatName( paramName ));
			//m_pScModuleDecl->addSubDecl( pTopParamDecl );

			pNewVarDecl->setInitExpr( inlineFuncInExpr( stmtList, pObjExpr, pOrigArgExpr ) );

			HtStmt * pDeclStmt = new HtStmt( Stmt::DeclStmtClass );
			pDeclStmt->addDecl( pNewVarDecl );

			stmtList.push_back( pDeclStmt );

			paramDeclIter++;
			argExprIter++;
		}
		
		assert(paramDeclIter.isEol());
		assert(argExprIter.isEol());
	}
}
