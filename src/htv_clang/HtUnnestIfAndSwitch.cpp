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

#define DEBUG_UNNEST 0

namespace ht {

	// Remove nested statements (other than simple assignments) from if and switch statements
	//   Objective is to end up with top level non-trivial assignment statements, if and switch
	//   statements with just trivial assignments within.
	// Algorithm is to walk through top level statements. Ignore top level assignments. Move
	//   out embeded statements within If and Switch (including other If and Switch).

	struct HtSubstDecl {
		HtSubstDecl(HtDecl * pDecl, HtDecl * pOrigSubstDecl) : m_pDecl(pDecl), m_pOrigSubstDecl(pOrigSubstDecl) {}

		HtDecl * m_pDecl;
		HtDecl * m_pOrigSubstDecl;
	};

	struct NestedTemp {
		NestedTemp(HtExpr * pLhsOrigExpr, HtExpr * pLhsNewExpr, HtExpr * pRhs, int clauseMask) 
			: m_pLhsOrigExpr(pLhsOrigExpr), m_pLhsNewExpr(pLhsNewExpr), m_pRhsExpr(pRhs), m_clauseMask(clauseMask) {}
		HtExpr * m_pLhsOrigExpr;
		HtExpr * m_pLhsNewExpr;
		HtExpr * m_pRhsExpr;
		uint32_t m_clauseMask;
	};

	class HtUnnestIfAndSwitch : HtWalkStmtTree
	{
	public:
		HtUnnestIfAndSwitch(HtDesign * pHtDesign, ASTContext &Context)
			: m_pHtDesign(pHtDesign), m_context(Context), m_diags(Context.getDiagnostics()) {}

		void unnestIfAndSwitch();

		bool walkCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid );
		void walkPostCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid ) { /*iStmt++;*/ }

	private:
		void processTopStmt( HtStmtIter & iStmt );
		void processIfStmt( HtStmtIter & iStmt, int clauseIdx, HtStmtIter & iInsert, vector<NestedTemp> * pNestedTempList );
		void processSwitchStmt( HtStmtIter & iStmt, HtStmtIter & iInsert );
		void processStmtList( bool bIsCond, int clauseIdx, HtStmtIter & iStmt, HtStmtIter & iInsert, vector<NestedTemp> * pNestedTempList );
		void processAssignStmt( HtStmtIter & iStmt, int clauseIdx, HtStmtIter & iInsert );
		void insertNestedTempInitStmts( HtStmtIter & iInsert, vector<NestedTemp> * pNestedTempList );
		void insertNestedTempStmts( HtStmtIter & iInsert, vector<NestedTemp> * pNestedTempList );

		HtExpr * findSubstDecl( HtStmtIter & iStmt );
		//HtDecl * findBaseDecl( HtStmtIter & iStmt );

		string findUniqueName( string name, HtLineInfo & lineInfo ) {
			string uniqueName = m_pHtDesign->findUniqueName( name, lineInfo );
			return uniqueName;
		}

	private:
		HtDesign * m_pHtDesign;
		ASTContext & m_context;
		DiagnosticsEngine & m_diags;

		int m_ifSwitchCnt;
		vector<HtSubstDecl> * m_pSubstDeclList;
		vector<NestedTemp> * m_pNestedTempList;
		int m_clauseIdx;
	};

	void HtDesign::unnestIfAndSwitch(ASTContext &Context)
	{
		updateRefCounts( Context );

		HtUnnestIfAndSwitch transform(this, Context);

		transform.unnestIfAndSwitch();

		ExitIfError(Context);
	}

	void HtUnnestIfAndSwitch::unnestIfAndSwitch()
	{
		m_ifSwitchCnt = 0;

		// iterate through methods
		HtDeclIter methodIter(m_pHtDesign->getScMethodDeclList());
		while ( !methodIter.isEol() ) {

			assert( (*methodIter)->getBody()->getStmtClass() == Stmt::CompoundStmtClass );
			HtStmtIter iStmt ( (*methodIter)->getBody()->getStmtList() );

			while ( !iStmt.isEol() )
				processTopStmt( iStmt );

			methodIter++;
		}
	}

	void HtUnnestIfAndSwitch::processTopStmt( HtStmtIter & iStmt )
	{
		Stmt::StmtClass stmtClass = iStmt->getStmtClass();
		if (stmtClass == Stmt::IfStmtClass) {

			processIfStmt( iStmt, -1, iStmt, 0 );

		} else if (stmtClass == Stmt::SwitchStmtClass) {

			processSwitchStmt( iStmt, iStmt );

		} else if (iStmt->isAssignStmt()) {

			assert(m_ifSwitchCnt == 0);
			iStmt++;

		} else
			assert(0);

		return;
	}

	void HtUnnestIfAndSwitch::processIfStmt( HtStmtIter & iStmt, int clauseIdx,
		HtStmtIter & iInsert, vector<NestedTemp> * pOuterTempList )
	{
		m_ifSwitchCnt += 1;

#if DEBUG_UNNEST
		printf("processIfStmt entry: cnt=%d\n", m_ifSwitchCnt);
		for (size_t i = 0; pOuterTempList != 0 && i < pOuterTempList->size(); i += 1) {
			NestedTemp & temp = (*pOuterTempList)[i];
			printf(" outerTemp[%d]: orig %s, new %s, rhs %s, mask=%d\n", (int)i,
				m_pHtDesign->findBaseDecl( temp.m_pLhsOrigExpr )->getName().c_str(),
				m_pHtDesign->findBaseDecl( temp.m_pLhsNewExpr )->getName().c_str(),
				m_pHtDesign->findBaseDecl( temp.m_pRhsExpr )->getName().c_str(),
				temp.m_clauseMask);
		}
		printf("\n");
#endif

		// list of rhs temps for then/else statements
		vector<NestedTemp> nestedTempList;
		vector<NestedTemp> * pInnerTempList = &nestedTempList;

		HtStmtIter iCond ( iStmt->getCond() );
		processStmtList( true, -1, iCond, iInsert, 0 );

#if DEBUG_UNNEST
		printf("processIfStmt before then: cnt=%d\n", m_ifSwitchCnt);
#endif
		HtStmtIter iThen ( iStmt->getThen() );
		processStmtList( false, 0, iThen, iInsert, pInnerTempList );

#if DEBUG_UNNEST
		printf("processIfStmt after then: cnt=%d\n", m_ifSwitchCnt);
		for (size_t i = 0; i < pInnerTempList->size(); i += 1) {
			NestedTemp & temp = (*pInnerTempList)[i];
			printf(" innerTemp[%d]: orig %s, new %s, rhs %s, mask=%d\n", (int)i,
				m_pHtDesign->findBaseDecl( temp.m_pLhsOrigExpr )->getName().c_str(),
				m_pHtDesign->findBaseDecl( temp.m_pLhsNewExpr )->getName().c_str(),
				m_pHtDesign->findBaseDecl( temp.m_pRhsExpr )->getName().c_str(),
				temp.m_clauseMask);
		}
		printf("\n");

		printf("processIfStmt before else: cnt=%d\n", m_ifSwitchCnt);
#endif
		HtStmtIter iElse ( iStmt->getElse() );
		processStmtList( false, 1, iElse, iInsert, pInnerTempList );

#if DEBUG_UNNEST
		printf("processIfStmt after else: cnt=%d\n", m_ifSwitchCnt);
		for (size_t i = 0; i < pInnerTempList->size(); i += 1) {
			NestedTemp & temp = (*pInnerTempList)[i];
			printf(" innerTemp[%d]: orig %s, new %s, rhs %s, mask=%d\n", (int)i,
				m_pHtDesign->findBaseDecl( temp.m_pLhsOrigExpr )->getName().c_str(),
				m_pHtDesign->findBaseDecl( temp.m_pLhsNewExpr )->getName().c_str(),
				m_pHtDesign->findBaseDecl( temp.m_pRhsExpr )->getName().c_str(),
				temp.m_clauseMask);
		}
		printf("\n");
#endif

		if (m_ifSwitchCnt > 1) {
			// insert initialization assignment statements
			insertNestedTempInitStmts( iInsert, pInnerTempList );

			// move statement
			iInsert.insert( iStmt.asStmt() );
			HtStmtIter iEraseStmt = iStmt++;
			iEraseStmt.erase();

			// transfer nested temps to outer list
			for (size_t i = 0; i < pInnerTempList->size(); i += 1) {
				NestedTemp & innerTemp = (*pInnerTempList)[i];

				size_t j;
				bool bFoundNestedTemp = false;
				for (j = 0; j < pOuterTempList->size(); j += 1) {
					NestedTemp & outerTemp = (*pOuterTempList)[j];
					HtDecl * pInnerBaseDecl = m_pHtDesign->findBaseDecl( innerTemp.m_pLhsOrigExpr );
					HtDecl * pOuterBaseDecl = m_pHtDesign->findBaseDecl( outerTemp.m_pLhsOrigExpr );

					bFoundNestedTemp = pOuterBaseDecl == pInnerBaseDecl;
					if (bFoundNestedTemp) {
						if (clauseIdx >= 0)
							outerTemp.m_clauseMask |= 1 << clauseIdx;
						break;
					}
				}

				//if (j < pOuterTempList->size())
				//	continue;

				HtDecl * pInnerDecl = innerTemp.m_pLhsNewExpr->getMemberDecl();

				HtExpr * pLhsTemp = 0;
				if (m_ifSwitchCnt != 2) {

					HtDecl * pNewOuterDecl;
					if (bFoundNestedTemp) {
						pNewOuterDecl = (*pOuterTempList)[j].m_pLhsNewExpr->getMemberDecl();
					} else {
						// create temp for outer statement list
						pNewOuterDecl = new HtDecl( Decl::Field, pInnerDecl->getLineInfo() );
						pNewOuterDecl->setType( pInnerDecl->getQualType().getType() );
						pNewOuterDecl->setFieldBitWidth( pInnerDecl->getFieldBitWidth() );
						pNewOuterDecl->isTempVar( true );
						pNewOuterDecl->setName( findUniqueName( pInnerDecl->getName(), pInnerDecl->getLineInfo() ));
						m_pHtDesign->getScModuleDecl()->addSubDecl( pNewOuterDecl );
					}

					// create lhs of assignment with new temp var
					pLhsTemp = new HtExpr( Stmt::MemberExprClass, pNewOuterDecl->getQualType(), pNewOuterDecl->getLineInfo() );
					pLhsTemp->setMemberDecl( pNewOuterDecl );

					HtExpr * pThis = new HtExpr( Stmt::CXXThisExprClass, HtQualType( 0 ), pLhsTemp->getLineInfo() );
					pLhsTemp->setBase( pThis );

					if (!bFoundNestedTemp)
						pOuterTempList->push_back(NestedTemp( innerTemp.m_pLhsNewExpr, pLhsTemp, innerTemp.m_pRhsExpr, 1 << clauseIdx ));
				}


				// create assignment statement
				HtExpr * pHtAssign = new HtExpr( Stmt::BinaryOperatorClass, pInnerDecl->getQualType(), pInnerDecl->getLineInfo() );
				pHtAssign->setBinaryOperator( BO_Assign );
				pHtAssign->isAssignStmt( true );
				iStmt.insert(pHtAssign);

				pHtAssign->setLhs( m_ifSwitchCnt == 2 ? innerTemp.m_pRhsExpr : pLhsTemp );
				pHtAssign->setRhs( innerTemp.m_pLhsNewExpr );
			}
		} else
			iStmt++;

#if DEBUG_UNNEST
		printf("processIfStmt exit: cnt=%d\n", m_ifSwitchCnt);
		for (size_t i = 0; i < pInnerTempList->size(); i += 1) {
			NestedTemp & temp = (*pInnerTempList)[i];
			printf(" innerTemp[%d]: orig %s, new %s, rhs %s, mask=%d\n", (int)i,
				m_pHtDesign->findBaseDecl( temp.m_pLhsOrigExpr )->getName().c_str(),
				m_pHtDesign->findBaseDecl( temp.m_pLhsNewExpr )->getName().c_str(),
				m_pHtDesign->findBaseDecl( temp.m_pRhsExpr )->getName().c_str(),
				temp.m_clauseMask);
		}
		printf("\n");
#endif

		m_ifSwitchCnt -= 1;
	}

	void HtUnnestIfAndSwitch::processSwitchStmt( HtStmtIter & iStmt, HtStmtIter & iInsert )
	{
		m_ifSwitchCnt += 1;

		assert(0);

		m_ifSwitchCnt -= 1;
	}
		
	void HtUnnestIfAndSwitch::insertNestedTempInitStmts( HtStmtIter & iInsert, vector<NestedTemp> * pNestedTempList )
	{
		for (size_t i = 0; i < pNestedTempList->size(); i += 1) {
			NestedTemp & nestedTemp = (*pNestedTempList)[i];

			// when clause mask == 0x3 then the variable was assigned in both then and else clauses
			//   in this case an initialization is not required
			if (nestedTemp.m_clauseMask == 0x3)
				continue;

			HtDecl * pTempDecl = nestedTemp.m_pRhsExpr->getMemberDecl();

			// create assignment statement
			HtExpr * pHtAssign = new HtExpr( Stmt::BinaryOperatorClass, pTempDecl->getQualType(), pTempDecl->getLineInfo() );
			pHtAssign->setBinaryOperator( BO_Assign );
			pHtAssign->isAssignStmt( true );
			iInsert.insert(pHtAssign);

			pHtAssign->setLhs( nestedTemp.m_pLhsNewExpr );
			pHtAssign->setRhs( nestedTemp.m_pRhsExpr );
		}
	}

	void HtUnnestIfAndSwitch::insertNestedTempStmts( HtStmtIter & iInsert, vector<NestedTemp> * pNestedTempList )
	{
		for (size_t i = 0; i < pNestedTempList->size(); i += 1) {
			NestedTemp & nestedTemp = (*pNestedTempList)[i];

			HtDecl * pTempDecl = nestedTemp.m_pRhsExpr->getMemberDecl();

			// create assignment statement
			HtExpr * pHtAssign = new HtExpr( Stmt::BinaryOperatorClass, pTempDecl->getQualType(), pTempDecl->getLineInfo() );
			pHtAssign->setBinaryOperator( BO_Assign );
			pHtAssign->isAssignStmt( true );
			iInsert.insert(pHtAssign);

			pHtAssign->setLhs( nestedTemp.m_pRhsExpr );
			pHtAssign->setRhs( nestedTemp.m_pLhsNewExpr );
		}
	}

	void HtUnnestIfAndSwitch::processStmtList( bool bIsCond, int clauseIdx, HtStmtIter & iStmt,
		HtStmtIter & iInsert, vector<NestedTemp> * pNestedTempList )
	{
		if (iStmt.isEol())
			return;

		HtExpr * pExpr = iStmt.asExpr();

		vector<HtSubstDecl> substDeclList;

		if (bIsCond) {
			// single variable or member expressions are okay as is
			if (pExpr->getStmtClass() != Stmt::MemberExprClass) {

				assert(0);

			}

		} else {
			// walk state list

			vector<HtSubstDecl> * pSaveSubstDeclList = m_pSubstDeclList;
			m_pSubstDeclList = & substDeclList;

			for ( ; !iStmt.isEol(); ) {

				if (iStmt->isAssignStmt()) {

					vector<NestedTemp> * pSaveNestedList = m_pNestedTempList;
					m_pNestedTempList = pNestedTempList;

					processAssignStmt( iStmt, clauseIdx, iInsert );

					m_pNestedTempList = pSaveNestedList;
					continue;
				}

				Stmt::StmtClass stmtClass = iStmt->getStmtClass();
				switch (stmtClass) {
				case Stmt::CompoundStmtClass:
					{
						HtStmtIter iList ( iStmt->getStmtList() );
						processStmtList( false, clauseIdx, iList, iInsert, pNestedTempList );
						iStmt++;
					}
					break;
				case Stmt::IfStmtClass:
					{
						processIfStmt( iStmt, clauseIdx, iInsert, pNestedTempList );
					}
					break;
				case Stmt::SwitchStmtClass:
					{
						processSwitchStmt( iStmt, iInsert );
						iStmt++;
					}
					break;
				default:
					assert(0);
				}
			}

			m_pSubstDeclList = pSaveSubstDeclList;
		}

		// remove substitutions set within statement list
		while (substDeclList.size() > 0) {
			HtSubstDecl & substDecl = substDeclList.back();

			substDecl.m_pDecl->setSubstDecl(substDecl.m_pOrigSubstDecl);

			substDeclList.pop_back();
		}
	}

	void HtUnnestIfAndSwitch::processAssignStmt( HtStmtIter & iStmt, int clauseIdx, HtStmtIter & iInsert )
	{
		// process an assignment or member call statement
		//   original statement processed with walkStmtTree
		//     create temps for LHS variables, insert assignment statements in place of original statement
		//     substitute previous temps for RHS variables
		//   move processed original statement to top level

		HtStmtIter iOrigStmt = iStmt;

		// variables are handled while scanning statement
		m_clauseIdx = clauseIdx;
		walkStmtTree( iStmt, iOrigStmt, false, 0 );

		// move statement to outside of if/switch statement
		iInsert.insert( iOrigStmt.asStmt() );
		iOrigStmt.erase();
	}

	bool HtUnnestIfAndSwitch::walkCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid )
	{
		Stmt::StmtClass stmtClass = iStmt->getStmtClass();
		switch (stmtClass) {
		default:
			assert(0);
			break;
		case Stmt::CallExprClass:
		case Stmt::BinaryOperatorClass:
		case Stmt::ImplicitCastExprClass:
		case Stmt::IntegerLiteralClass:
			break;
		case Stmt::MemberExprClass:
			{
				HtDecl * pBaseDecl = m_pHtDesign->findBaseDecl( iStmt.asExpr() );
				HtDecl * pFieldDecl = iStmt.asExpr()->getMemberDecl();

				if (isLoa) {
					if (pBaseDecl->getWrRefCnt() > 1) {//!pBaseDecl->isTempVar()) {
						// lhs var needs a temp
						HtDecl * pNewDecl = new HtDecl( Decl::Field, pBaseDecl->getLineInfo() );
						pNewDecl->setType( pFieldDecl->getQualType().getType() );
						pNewDecl->setFieldBitWidth( pFieldDecl->getFieldBitWidth() );
						pNewDecl->isTempVar( true );
						pNewDecl->setName( findUniqueName( pFieldDecl->getName(), pBaseDecl->getLineInfo() ));
						m_pHtDesign->getScModuleDecl()->addSubDecl( pNewDecl );

						// create assignment statement
						HtExpr * pHtAssign = new HtExpr( Stmt::BinaryOperatorClass, pNewDecl->getQualType(), pNewDecl->getLineInfo() );
						pHtAssign->setBinaryOperator( BO_Assign );
						pHtAssign->isAssignStmt( true );
						iInsert.insert(pHtAssign);

						// create rhs of assignment with new temp var
						HtExpr * pHtRhs = new HtExpr( Stmt::MemberExprClass, pNewDecl->getQualType(), pNewDecl->getLineInfo() );
						pHtRhs->setMemberDecl( pNewDecl );
						pHtAssign->setRhs( pHtRhs );

						HtExpr * pThis = new HtExpr( Stmt::CXXThisExprClass, HtQualType( 0 ), pHtRhs->getLineInfo() );
						pHtRhs->setBase( pThis );

						if (m_ifSwitchCnt == 1) {
							// move original lhs expression as lhs for new assignment
							pHtAssign->setLhs( iStmt.asExpr() );

						} else {
							// need temp for nested if/switch assignment
							//   first check if a temp was previously created
							size_t i;
							bool bFoundNestedTemp = false;
							for (i = 0; i < m_pNestedTempList->size(); i += 1) {
								NestedTemp &nestedTemp = (*m_pNestedTempList)[i];

								HtDecl * pRhsBaseDecl = m_pHtDesign->findBaseDecl( iStmt.asExpr() );
								HtDecl * pNestedTempDecl = m_pHtDesign->findBaseDecl( nestedTemp.m_pRhsExpr );

								bFoundNestedTemp = pNestedTempDecl == pRhsBaseDecl;
								if (bFoundNestedTemp) {
									if (m_clauseIdx >= 0)
										nestedTemp.m_clauseMask |= 1 << m_clauseIdx;
									break;
								}
							}

							HtDecl * pNewNestedDecl;
							if (bFoundNestedTemp) {
								pNewNestedDecl = (*m_pNestedTempList)[i].m_pLhsNewExpr->getMemberDecl();
							} else {
								pNewNestedDecl = new HtDecl( Decl::Field, pBaseDecl->getLineInfo() );
								pNewNestedDecl->setType( pFieldDecl->getQualType().getType() );
								pNewNestedDecl->setFieldBitWidth( pFieldDecl->getFieldBitWidth() );
								pNewNestedDecl->isTempVar( true );
								pNewNestedDecl->setName( findUniqueName( pFieldDecl->getName(), pBaseDecl->getLineInfo() ));
								m_pHtDesign->getScModuleDecl()->addSubDecl( pNewNestedDecl );
							}

							// create lhs of assignment with new temp var
							HtExpr * pLhsTemp = new HtExpr( Stmt::MemberExprClass, pNewNestedDecl->getQualType(), pNewNestedDecl->getLineInfo() );
							pLhsTemp->setMemberDecl( pNewNestedDecl );
							pHtAssign->setLhs( pLhsTemp );

							HtExpr * pThis = new HtExpr( Stmt::CXXThisExprClass, HtQualType( 0 ), pLhsTemp->getLineInfo() );
							pLhsTemp->setBase( pThis );

							//printf("orig: %s, new: %s, rhs: %s\n",
							//	m_pHtDesign->findBaseDecl( iStmt.asExpr() )->getName().c_str(),
							//	m_pHtDesign->findBaseDecl( pLhsTemp )->getName().c_str(),
							//	m_pHtDesign->findBaseDecl( iStmt.asExpr() )->getName().c_str() );

							if (!bFoundNestedTemp)
								m_pNestedTempList->push_back(NestedTemp( iStmt.asExpr(), pLhsTemp, iStmt.asExpr(), 1 << m_clauseIdx ));
						}

						// create new expression for original statement output
						HtExpr * pOrigLhs = new HtExpr( Stmt::MemberExprClass, pNewDecl->getQualType(), pNewDecl->getLineInfo() );
						pOrigLhs->setMemberDecl( pNewDecl );

						HtExpr * pOrigLhsThis = new HtExpr( Stmt::CXXThisExprClass, HtQualType( 0 ), pHtRhs->getLineInfo() );
						pOrigLhs->setBase( pOrigLhsThis );

						iStmt.replace( pOrigLhs );

						m_pSubstDeclList->push_back( HtSubstDecl( pFieldDecl, pFieldDecl->getSubstDecl() ));
						pFieldDecl->setSubstDecl( pNewDecl );
					}
				} else {
					// check if member or base has substitute declaration specified

					HtExpr * pExpr = findSubstDecl( iStmt );

					if (pExpr && pExpr->getStmtClass() == Stmt::MemberExprClass)
						pExpr->setMemberDecl( pExpr->getMemberDecl()->getSubstDecl() );
				}

				iStmt++;
				return false;
			}
			break;
		}
		return true;
	}

	HtExpr * HtUnnestIfAndSwitch::findSubstDecl( HtStmtIter & iStmt )
	{
		HtExpr * pExpr = iStmt.asExpr();
		for (;;) {
			Stmt::StmtClass stmtClass = pExpr->getStmtClass();
			switch (stmtClass) {
			case Stmt::MemberExprClass:
				assert(pExpr->getMemberDecl()->getKind() == Decl::Field);
				if (pExpr->getMemberDecl()->getSubstDecl())
					return pExpr;
				pExpr = pExpr->getBase().asExpr();
				break;
			case Stmt::ImplicitCastExprClass:
			case Stmt::CXXStaticCastExprClass:
				pExpr = pExpr->getSubExpr().asExpr();
				break;
			case Stmt::CXXThisExprClass:
				return 0;
			default:
				assert(0);
			}
		}
		return 0;
	}
}
