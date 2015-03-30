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

	// Inline variable references
	// - Pass2: generate reference init assignments for inlined function
	//    Function returns are replaced with integer indicate appropriate
	//    ref init selection. All non-const indexing for ref init statements
	//    are assigned to a temp variable.
	// - Pass3: inline references and remove ref decl

	class HtInlineVarReferences : HtDupStmtTree, HtWalkStmtTree
	{
	public:
		HtInlineVarReferences(HtDesign * pHtDesign, ASTContext &Context)
			: m_pHtDesign(pHtDesign), m_context(Context), m_diags(Context.getDiagnostics()) {}

		void inlineVarReferences();
		void handleInitDecl(HtStmtIter & insertIter, HtDecl * pHtDecl);
		void visitStmt( HtStmtList & );

		string findUniqueName( string name, HtLineInfo & lineInfo ) { return m_pHtDesign->findUniqueName( name, lineInfo ); }

		bool walkCallback( HtStmtIter & iSrcStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid );
		void walkPostCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid ) { /*iStmt++;*/ }
		HtStmt * dupCallback( HtStmtIter & iStmt, void * pVoid );

	private:
		HtDesign * m_pHtDesign;
		ASTContext & m_context;
		DiagnosticsEngine & m_diags;
	};

	void HtDesign::inlineVarReferences(ASTContext &Context)
	{
		HtInlineVarReferences transform(this, Context);

		transform.inlineVarReferences();

		ExitIfError(Context);
	}

	void HtInlineVarReferences::inlineVarReferences()
	{
		HtDeclIter methodIter(m_pHtDesign->getScMethodDeclList());
		while ( !methodIter.isEol() ) {

			// walk statement list looking for initialization of ref variable 
			//   from within inlined function and useage of reference variable
			visitStmt( (*methodIter)->getBody() );

			methodIter++;
		}
	}

	void HtInlineVarReferences::visitStmt( HtStmtList & stmtList )
	{
		HtStmtIter iStmt( stmtList );
		while ( !iStmt.isEol() ) {

			if (iStmt->isAssignStmt()) {
				HtExpr * pExpr = iStmt.asExpr();
				if (pExpr->getBinaryOperator() == BO_Assign 
					&& pExpr->getLhsExpr()->getStmtClass() == Stmt::DeclRefExprClass
					&& (pExpr->getLhsExpr()->getRefDecl()->getQualType().getType()->getTypeClass() == Type::LValueReference
					|| pExpr->getLhsExpr()->getRefDecl()->getQualType().getType()->getTypeClass() == Type::RValueReference))
				{
					// Found a ref var initialization statement from an inlined function call
					//   move the expression to the ref var decl as the init expression
					assert( !pExpr->getLhsExpr()->getRefDecl()->hasInitExpr() );
					pExpr->getLhsExpr()->getRefDecl()->setInitExpr( pExpr->getRhsExpr() );

					// remove ref expr from stmt list
					HtStmtIter removeStmt = iStmt++;
					removeStmt.erase();
				}
				else
				{
					walkStmtTree( iStmt, iStmt, false );
				}
				continue;
			}

			switch (iStmt->getStmtClass()) {
			case Stmt::CompoundStmtClass:
				{
					visitStmt( iStmt->getStmtList() );

					iStmt++;
				}
				break;
			case Stmt::ForStmtClass:
				{
					walkStmtTree( HtStmtIter(iStmt->getInit()), iStmt, true );
					walkStmtTree( HtStmtIter(iStmt->getCond()), iStmt, true );
					walkStmtTree( HtStmtIter(iStmt->getInc()), iStmt, true );
					visitStmt( iStmt->getBody() );
					iStmt++;
				}
				break;
			case Stmt::IfStmtClass:
				{
					walkStmtTree( HtStmtIter(iStmt->getCond()), iStmt, true );
					visitStmt( iStmt->getThen() );
					visitStmt( iStmt->getElse() );
					iStmt++;
				}
				break;
			case Stmt::SwitchStmtClass:
				{
					walkStmtTree( HtStmtIter(iStmt->getCond()), iStmt, true );
					visitStmt( iStmt->getBody() );
					iStmt++;
				}
				break;
			case Stmt::CaseStmtClass:
				{
					walkStmtTree( HtStmtIter(iStmt->getLhs()), iStmt, true );
					visitStmt( iStmt->getSubStmt() );
					iStmt++;
				}
				break;
			case Stmt::DefaultStmtClass:
				{
					visitStmt( iStmt->getSubStmt() );
					iStmt++;
				}
				break;
			case Stmt::BreakStmtClass:
			case Stmt::NullStmtClass:
				iStmt++;
				break;
			case Stmt::DeclStmtClass:
				{
					HtDeclIter declIter(iStmt->getDeclList());

					// move var decl to ScModule decl list, turn initializers into assignment statements
					HtStmtIter insertIter = iStmt;
					insertIter++;	// insert after original statement
				
					while ( !declIter.isEol() ) {

						HtDecl * pDecl = *declIter;

						if (pDecl->getQualType().getType()->getTypeClass() != Type::LValueReference
							&& pDecl->getQualType().getType()->getTypeClass() != Type::RValueReference)
						{
							// move declaration to top level, keep initialization local

							if (!pDecl->isTempVar())
								pDecl->setName( findUniqueName( pDecl->getName(), pDecl->getLineInfo() ));

							m_pHtDesign->getScModuleDecl()->addSubDecl( pDecl );

							handleInitDecl(insertIter, pDecl);
						}

						declIter++;
					}

					// remove ref declarations from stmt list
					HtStmtIter removeStmt = iStmt++;
					removeStmt.erase();
				}
				break;
			default:
				assert(0);
			}
		}
	}

	void HtInlineVarReferences::handleInitDecl(HtStmtIter & insertIter, HtDecl * pDecl)
	{
		if (!pDecl->hasInitExpr())
			return;

		HtExpr * pInitExpr = pDecl->getInitExpr().asExpr();

		if (pDecl->getType()->getTypeClass() == Type::ConstantArray) {
			// an array declaration. Must break apart and generate
			// a statement per array element

			vector<int> dimenList;
			vector<HtStmtIter> initExprList;

			HtType * pType = pDecl->getType();
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
				HtExpr * pDeclRef = new HtExpr( Stmt::DeclRefExprClass, pDecl->getQualType(), pDecl->getLineInfo() );
				pDeclRef->setRefDecl( pDecl );

				HtType * pArrayType = pDecl->getType();
				HtExpr * pArrayBase = pDeclRef;

				for (size_t i = 0; i < dimenList.size(); i += 1) {
					assert( pArrayType->getTypeClass() == Type::ConstantArray);

					HtType * pIntType = m_pHtDesign->getBuiltinIntType();
					HtExpr * pArrayIdx = new HtExpr( Stmt::IntegerLiteralClass, HtQualType(pIntType), pDecl->getLineInfo() );
					pArrayIdx->setIntegerLiteral( idxList[i] );

					HtExpr * pArraySubExpr = new HtExpr( Stmt::ArraySubscriptExprClass, HtQualType(pArrayType), pDecl->getLineInfo() );
					pArraySubExpr->setBase( pArrayBase );
					pArraySubExpr->setIdx( pArrayIdx );

					pArrayBase = pArraySubExpr;
					pArrayType = pArrayType->getType();
				}

				HtExpr * pAssignStmt = new HtExpr( Stmt::BinaryOperatorClass, HtQualType(pArrayType), pDecl->getLineInfo() );
				pAssignStmt->isAssignStmt( true );
				pAssignStmt->setBinaryOperator( BO_Assign );
				pAssignStmt->setLhs( pArrayBase );
				pAssignStmt->setRhs( hasInitList ? initExprList.back().asExpr() : pInitExpr );

				insertIter.insert( pAssignStmt );

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
			// create assignment statement from init expression
			HtExpr * pDeclRef = new HtExpr( Stmt::DeclRefExprClass, pDecl->getQualType(), pDecl->getLineInfo() );
			pDeclRef->setRefDecl( pDecl );

			HtExpr * pAssignStmt = new HtExpr( Stmt::BinaryOperatorClass, pDecl->getQualType(), pDecl->getLineInfo() );
			pAssignStmt->isAssignStmt( true );
			pAssignStmt->setBinaryOperator( BO_Assign );
			pAssignStmt->setLhs( pDeclRef );
			pAssignStmt->setRhs( pInitExpr );

			insertIter.insert( pAssignStmt );
		}
								
		pDecl->setInitExpr( 0 );
	}

	bool HtInlineVarReferences::walkCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid )
	{
		if (iStmt->getStmtClass() != Stmt::DeclRefExprClass)
			return true;

		HtExpr * pSrcExpr = iStmt.asExpr();
		if (pSrcExpr->getRefDecl()->getQualType().getType()->getTypeClass() != Type::LValueReference
			&& pSrcExpr->getRefDecl()->getQualType().getType()->getTypeClass() != Type::RValueReference)
			return true;

		// we found a reference, inline referenced variable
		assert(pSrcExpr->getRefDecl()->hasInitExpr());

		// remove decl ref
		HtStmtIter eraseIter = iStmt++;
		eraseIter.erase();

		iStmt.insert( dupStmtTree( HtStmtIter(pSrcExpr->getRefDecl()->getInitExpr()), 0 ));

		return false;
	}

	HtStmt * HtInlineVarReferences::dupCallback( HtStmtIter & iSrcStmt, void * pVoid )
	{
		if (iSrcStmt->getStmtClass() != Stmt::DeclRefExprClass)
			return 0;

		HtExpr * pRefInitExpr = iSrcStmt.asExpr();
		if (pRefInitExpr->getRefDecl()->getQualType().getType()->getTypeClass() != Type::LValueReference
			&& pRefInitExpr->getRefDecl()->getQualType().getType()->getTypeClass() != Type::RValueReference)
			return iSrcStmt;

		// we found a reference, inline referenced variable
		assert(pRefInitExpr->getRefDecl()->hasInitExpr());

		return dupStmtTree( HtStmtIter(pRefInitExpr->getRefDecl()->getInitExpr()), 0 );
	}

}
