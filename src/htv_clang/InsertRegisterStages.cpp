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

	// Insert register stages as appropriate to achieve timing
	//   need to decide how to number register stages
	//   need to walk statments and replace registers with appropriate register stage
	//   need to declare new registers
	//   need to connect pipelined reigsters

	////////////////////////////////////////////////////////////////////////////////////
	//  Trs is set prior to entry
	//  Insert timing registers
	//  Declare new variables (r_ and c_)
	//  Insert r_ and c_ assignment statements

	class HtInsertRegisterStages : HtWalkStmtTree
	{
	public:
		HtInsertRegisterStages(HtDesign * pHtDesign, ASTContext &Context)
			: m_pHtDesign(pHtDesign), m_context(Context), m_diags(Context.getDiagnostics()) {}

		void insertRegisterStages();

		bool walkCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid );
		void walkPostCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid );

	private:

		// if statement routines
		void setIfLhsDeclMinTrs(HtStmtIter & iStmt);
		int findIfLhsMinTrs(HtStmtIter & iStmt, bool & bLastTrs);
		void dupIfStmtForMinTrs(HtStmtIter & iOrigStmt, HtStmtIter & iNewStmt, int minTrs);
		HtStmt * dupIfClauseMinTrs( HtStmt * pStmt, int minTrs );
		void insertRegIfStmt(HtStmtIter & iIfStmt, int rdTrs, HtStmtIter & iInsert);

		void insertRegMemberRhsExpr(HtExpr * pRhsExpr, int rdTrs, HtStmtIter & iInsert);
		void insertRegMemberLhsExpr(HtExpr * pLhsExpr, int rdTrs, HtStmtIter & iInsert);

		void insertRegDecl(HtDeclList_t & moduleDeclList);
		void insertRegAssign(HtDeclList_t & moduleDeclList, HtStmtList & methodStmtList);

		HtExpr * findBaseExpr( HtExpr * pExpr );
		HtDecl * findBaseDecl( HtExpr * pExpr );
		
		string genTrsName(HtDecl * pDecl, int rdTrs, bool bIsCVar=false);

	private:
		HtDesign * m_pHtDesign;
		ASTContext & m_context;
		DiagnosticsEngine & m_diags;
		HtTpg * m_pMethodTpGrp;
	};

	void HtDesign::insertRegisterStages(ASTContext &Context)
	{
		HtInsertRegisterStages transform(this, Context);

		transform.insertRegisterStages();

		ExitIfError(Context);
	}

	void HtInsertRegisterStages::insertRegisterStages()
	{
		HtDeclList_t & moduleDeclList = m_pHtDesign->getScModuleDeclList();

		// walk each ScMethod in design
		HtDeclIter iDecl(m_pHtDesign->getScMethodDeclList());
		while ( !iDecl.isEol() ) {

			// create needed staging variables
			insertRegDecl(moduleDeclList);

			m_pMethodTpGrp = (*iDecl)->getTpGrp();
			assert( (*iDecl)->getBody()->getStmtClass() == Stmt::CompoundStmtClass );
			HtStmtIter iStmt ( (*iDecl)->getBody()->getStmtList() );

			HtStmtIter iInsert = iStmt.eol();

			while ( !iStmt.isEol() )
				walkStmtTree( iStmt, iInsert, false, 0 );

			// now walk decl list and generate assignments to new registers
			HtStmtList & methodStmtList = (*iDecl)->getBody()->getStmtList();

			insertRegAssign(moduleDeclList, methodStmtList);

			iDecl++;
		}
	}

	bool HtInsertRegisterStages::walkCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid )
	{
		Stmt::StmtClass stmtClass = iStmt->getStmtClass();
		switch (stmtClass) {
		case Stmt::MemberExprClass:
		case Stmt::IntegerLiteralClass:
		case Stmt::IfStmtClass:
			return false;
		case Stmt::CallExprClass:
		case Stmt::CompoundStmtClass:
		case Stmt::BinaryOperatorClass:
		case Stmt::ImplicitCastExprClass:
		case Stmt::ParenExprClass:
			return true;
		default:
			assert(0);
			break;
		}
	}

	void HtInsertRegisterStages::setIfLhsDeclMinTrs(HtStmtIter & iStmt)
	{
		//// clear minTrs for each lhs decl
		//for (HtStmtIter iThen(iStmt->getThen()->getStmtList()); !iThen.isEol(); iThen++) {
		//	assert(iThen->getStmtClass() == Stmt::BinaryOperatorClass);

		//	HtExpr * pLhsExpr = iThen->getLhs().asExpr();
		//	assert(pLhsExpr->getStmtClass() == Stmt::MemberExprClass);

		//	HtDecl * pLhsDecl = findBaseDecl( (HtExpr *)pLhsExpr );

		//	pLhsDecl->clearSgWrTrs();
		//}

		//for (HtStmtIter iElse(iStmt->getElse()->getStmtList()); !iElse.isEol(); iElse++) {
		//	assert(iElse->getStmtClass() == Stmt::BinaryOperatorClass);

		//	HtExpr * pLhsExpr = iElse->getLhs().asExpr();
		//	assert(pLhsExpr->getStmtClass() == Stmt::MemberExprClass);

		//	HtDecl * pLhsDecl = findBaseDecl( (HtExpr *)pLhsExpr );

		//	pLhsDecl->clearSgWrTrs();
		//}

		// set minTrs for each lhs decl
		//for (HtStmtIter iThen(iStmt->getThen()->getStmtList()); !iThen.isEol(); iThen++) {
		//	assert(iThen->getStmtClass() == Stmt::BinaryOperatorClass);

		//	HtExpr * pLhsExpr = iThen->getLhs().asExpr();
		//	assert(pLhsExpr->getStmtClass() == Stmt::MemberExprClass);

		//	HtDecl * pLhsDecl = findBaseDecl( (HtExpr *)pLhsExpr );

		//	pLhsDecl->setSgWrTrs( pLhsExpr->getSgTrs() );
		//}

		//for (HtStmtIter iElse(iStmt->getElse()->getStmtList()); !iElse.isEol(); iElse++) {
		//	assert(iElse->getStmtClass() == Stmt::BinaryOperatorClass);

		//	HtExpr * pLhsExpr = iElse->getLhs().asExpr();
		//	assert(pLhsExpr->getStmtClass() == Stmt::MemberExprClass);

		//	HtDecl * pLhsDecl = findBaseDecl( (HtExpr *)pLhsExpr );

		//	pLhsDecl->setSgWrTrs( pLhsExpr->getSgTrs() );
		//}
	}

	int HtInsertRegisterStages::findIfLhsMinTrs(HtStmtIter & iStmt, bool &bLastTrs)
	{
		// get minTrs for all lhs decl
		int maxTrs = -1;
		int minTrs = -1;
		for (HtStmtIter iThen(iStmt->getThen()->getStmtList()); !iThen.isEol(); iThen++) {
			assert(iThen->getStmtClass() == Stmt::BinaryOperatorClass);

			HtExpr * pLhsExpr = iThen->getLhs().asExpr();
			assert(pLhsExpr->getStmtClass() == Stmt::MemberExprClass);

			HtDecl * pLhsDecl = findBaseDecl( (HtExpr *)pLhsExpr );

			int lhsTrs = pLhsExpr->getTpSgTrs();

			if (minTrs < 0 || minTrs > lhsTrs)
				minTrs = lhsTrs;

			maxTrs = max(maxTrs, lhsTrs);
		}

		for (HtStmtIter iElse(iStmt->getElse()->getStmtList()); !iElse.isEol(); iElse++) {
			assert(iElse->getStmtClass() == Stmt::BinaryOperatorClass);

			HtExpr * pLhsExpr = iElse->getLhs().asExpr();
			assert(pLhsExpr->getStmtClass() == Stmt::MemberExprClass);

			HtDecl * pLhsDecl = findBaseDecl( (HtExpr *)pLhsExpr );

			int lhsTrs = pLhsExpr->getTpSgTrs();

			if (minTrs < 0 || minTrs > lhsTrs)
				minTrs = lhsTrs;

			maxTrs = max(maxTrs, lhsTrs);
		}

		bLastTrs = minTrs == maxTrs;
		return minTrs;
	}

	void HtInsertRegisterStages::dupIfStmtForMinTrs(HtStmtIter & iOrigStmt, HtStmtIter & iNewStmt, int minTrs)
	{
		HtStmt * pHtStmt = new HtStmt(iOrigStmt.asStmt());
		iNewStmt.insert( pHtStmt );
		iNewStmt--;

		HtExpr * pOrigCondExpr = iOrigStmt->getCond().asExpr();
		HtExpr * pDupCondExpr = new HtExpr( pOrigCondExpr );
		pDupCondExpr->setTpSgTrs( minTrs );
		pHtStmt->setCond( pDupCondExpr );

		assert(pOrigCondExpr->getBase()->getStmtClass() == Stmt::CXXThisExprClass);
		pDupCondExpr->setBase( new HtExpr( pOrigCondExpr->getBase().asExpr() ));

		pHtStmt->setThen( dupIfClauseMinTrs( iOrigStmt->getThen().asStmt(), minTrs ));
		pHtStmt->setElse( dupIfClauseMinTrs( iOrigStmt->getElse().asStmt(), minTrs ));
	}

	HtStmt * HtInsertRegisterStages::dupIfClauseMinTrs( HtStmt * pStmt, int minTrs )
	{
		if (pStmt == 0)
			return 0;

		if (pStmt->getStmtClass() == Stmt::CompoundStmtClass) {
			HtStmt * pCmpdStmt = new HtStmt( pStmt );

			HtStmtIter iStmtIter( pStmt->getStmtList() );
			for ( ; !iStmtIter.isEol(); ) {
				assert(iStmtIter->getStmtClass() == Stmt::BinaryOperatorClass);

				HtExpr * pLhsExpr = iStmtIter->getLhs().asExpr();
				if (pLhsExpr->getTpSgTrs() != minTrs) {
					iStmtIter++;
					continue;
				}

				// move statement to dup if statement
				HtStmtIter iStmt = iStmtIter++;
				HtStmt * pAssignStmt = iStmt.asStmt();
				iStmt.erase();
				pCmpdStmt->getStmtList().append( pAssignStmt );
			}

			return pCmpdStmt;
		}
		assert(0);
		return 0;
	}

	void HtInsertRegisterStages::insertRegIfStmt(HtStmtIter & iIfStmt, int rdTrs, HtStmtIter & iInsert)
	{
		insertRegMemberRhsExpr(iIfStmt->getCond().asExpr(), rdTrs, iInsert);

		for (HtStmtIter iThen(iIfStmt->getThen()->getStmtList()); !iThen.isEol(); iThen++) {
			assert(iThen->getStmtClass() == Stmt::BinaryOperatorClass);

			HtExpr * pRhsExpr = iThen->getRhs().asExpr();
			insertRegMemberRhsExpr(pRhsExpr, rdTrs, iInsert);

			HtExpr * pLhsExpr = iThen->getLhs().asExpr();
			insertRegMemberLhsExpr(pLhsExpr, rdTrs, iInsert);
		}

		for (HtStmtIter iElse(iIfStmt->getElse()->getStmtList()); !iElse.isEol(); iElse++) {
			assert(iElse->getStmtClass() == Stmt::BinaryOperatorClass);

			HtExpr * pRhsExpr = iElse->getRhs().asExpr();
			insertRegMemberRhsExpr(pRhsExpr, rdTrs, iInsert);

			HtExpr * pLhsExpr = iElse->getLhs().asExpr();
			insertRegMemberLhsExpr(pLhsExpr, rdTrs, iInsert);
		}
	}

	string HtInsertRegisterStages::genTrsName(HtDecl * pDecl, int rdTrs, bool bIsCVar)
	{
		// assume name starts with r#_ or c#_ where # is zero or more digits
		//  if c#_ then replace c with r
		//  add $# before _ with # being rdTrs value

		if (rdTrs == 0)
			return pDecl->getName();

		char buf[64];
		sprintf(buf, "$%d", rdTrs);
		string stgStr = buf;

		string name = pDecl->getName();
		if (name[0] == 'c' || name[0] == 'r') {
			int i = 1;
			for ( ; i < name.size() && isdigit(name[i]); i+=1);

			if (i < name.size() && name[i] == '_') {
				name = name.substr(0, i) + stgStr + name.substr(i);
				return name;
			}
		} 
		
		name = (bIsCVar ? "c" : "r") + stgStr + "_" + name;
		return name;
	}

	void HtInsertRegisterStages::walkPostCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid )
	{
		if (iStmt->getTpSgUrs() < 0)
			return;

		Stmt::StmtClass stmtClass = iStmt->getStmtClass();
		switch (stmtClass) {
		case Stmt::MemberExprClass:
			{
				// process variable
				HtExpr * pExpr = iStmt.asExpr();

				if (isLoa)
					break;

				int rdTrs = pExpr->getTpSgTrs();

				insertRegMemberRhsExpr(pExpr, rdTrs, iInsert);
			}
			break;
		case Stmt::IfStmtClass:
			{
				// handle if statement
				//   first clear all lhs decl minTrs
				//   walk then/else statements and set lhs decl minTrs
				//   while (then or else not empty)
				//     find min lhs expr trs
				//     generate if then/else with min trs

				setIfLhsDeclMinTrs(iStmt);

				bool bLastTrs;
				do {
					int rdTrs = findIfLhsMinTrs(iStmt, bLastTrs);

					HtStmtIter iIfStmt = iStmt;
					if (!bLastTrs)
						dupIfStmtForMinTrs(iStmt, iIfStmt, rdTrs);

					insertRegIfStmt(iIfStmt, rdTrs, iInsert);

				} while (!bLastTrs);
			}
			break;
		case Stmt::CompoundStmtClass:
			break;
		case Stmt::CallExprClass:
			{
				// propagate trs/tpd from inputs to outputs
				HtExpr * pExpr = iStmt.asExpr();

				//propCallExpr( pExpr );
			}
			break;
		case Stmt::BinaryOperatorClass:
			{
				// propagate regStage/subGroup from leaf expr
				HtExpr * pExpr = iStmt.asExpr();
				HtExpr * pLhsExpr = pExpr->getLhs().asExpr();
				HtExpr * pRhsExpr = pExpr->getRhs().asExpr();

				if (pExpr->isAssignStmt()) {
					HtExpr * pBaseExpr = findBaseExpr( pLhsExpr );
					HtDecl * pBaseDecl = pBaseExpr->getMemberDecl();

					if (pBaseDecl->isRegister()) {
						int rdTrs = m_pMethodTpGrp->getSgMaxWrTrs( pLhsExpr->getTpSgId(), pLhsExpr->getTpSgUrs() );

						insertRegMemberRhsExpr(pRhsExpr, rdTrs, iInsert);
					}

				} else {
					assert(pExpr->getTpSgTrs() == pLhsExpr->getTpSgTrs());
					assert(pExpr->getTpSgTrs() == pRhsExpr->getTpSgTrs());
				}
			}
			break;
		case Stmt::ImplicitCastExprClass:
		case Stmt::ParenExprClass:
			{
				// propagate regStage/subGroup from leaf expr
				HtExpr * pExpr = iStmt.asExpr();
				HtExpr * pSubExpr = pExpr->getSubExpr().asExpr();

				assert(pSubExpr->getTpSgUrs() != URS_UNKNOWN);
			}
			break;
		case Stmt::IntegerLiteralClass:
			break;
		default:
			assert(0);
			break;
		}
	}

	void HtInsertRegisterStages::insertRegMemberLhsExpr(HtExpr * pExpr, int rdTrs, HtStmtIter & iInsert)
	{
		assert(pExpr->getStmtClass() == Stmt::MemberExprClass);
		HtExpr * pBaseExpr = findBaseExpr( pExpr );
		HtDecl * pBaseDecl = pBaseExpr->getMemberDecl();

		assert(pExpr->getTpSgTrs() == rdTrs);
		assert(pBaseDecl->getTpSgMinWrTrs() <= rdTrs);

		if (pExpr->getTpSgTrs() == rdTrs)
			return;

		assert(0);
	}

	void HtInsertRegisterStages::insertRegMemberRhsExpr(HtExpr * pExpr, int rdTrs, HtStmtIter & iInsert)
	{
		while (pExpr->getStmtClass() == Stmt::ImplicitCastExprClass)
			pExpr = pExpr->getSubExpr().asExpr();

		assert(pExpr->getStmtClass() == Stmt::MemberExprClass);
		HtExpr * pBaseExpr = findBaseExpr( pExpr );
		HtDecl * pBaseDecl = pBaseExpr->getMemberDecl();

		int wrTrs = pBaseDecl->getTpSgMinWrTrs();

		assert(rdTrs >= wrTrs);
		//assert(wrTrs >= 0);

		if (wrTrs < 0 || rdTrs == wrTrs)
			return;

		// need one or more register stages
		//   create variable name
		//   find or create declaration
		//   replace new declaration in memberExpr

		int declMaxTrs = pBaseDecl->getTpSgMaxRdTrs() >= 0 ?
			pBaseDecl->getTpSgMaxRdTrs() : wrTrs;

		string name = genTrsName(pBaseDecl, rdTrs);
		HtDecl * pTrsDecl = m_pHtDesign->getScModuleDecl()->findSubDecl( name );
		assert(pTrsDecl);

		pBaseExpr->setMemberDecl( pTrsDecl );
	}

	void HtInsertRegisterStages::insertRegDecl(HtDeclList_t & moduleDeclList)
	{
		for (HtDeclIter iDecl(moduleDeclList); !iDecl.isEol(); iDecl++) {
			if (iDecl->getTpSgMinWrTrs() == iDecl->getTpSgMaxRdTrs())
				continue;

			// found a decleration that needs staging
			// first declare c$ variables
			for (int cStgIdx = iDecl->getTpSgMinWrTrs()+1; cStgIdx <= iDecl->getTpSgMaxWrTrs(); cStgIdx += 1) {
				string name = genTrsName( *iDecl, cStgIdx, true);
				
				HtDecl * pTrsDecl = new HtDecl( Decl::Field, iDecl->getLineInfo() );
				pTrsDecl->setName( name );
				pTrsDecl->setQualType( iDecl->getQualType() );
				pTrsDecl->setDesugaredQualType( iDecl->getDesugaredQualType() );
				pTrsDecl->setTpGrp( iDecl->getTpGrp() );
				pTrsDecl->setTpGrpId( iDecl->getTpGrpId() );
				pTrsDecl->setTpSgId( iDecl->getTpSgId() );
				pTrsDecl->setTpSgUrs( iDecl->getTpSgUrs() );
				pTrsDecl->setTpSgWrTrs( cStgIdx );
				pTrsDecl->setTpSgTpd( TPD_REG_OUT );
				pTrsDecl->setTpSgRdTrs( cStgIdx );
				m_pHtDesign->getScModuleDecl()->addSubDecl( pTrsDecl );
			}

			// now declare r$ variables
			for (int cStgIdx = iDecl->getTpSgMinWrTrs()+1; cStgIdx <= iDecl->getTpSgMaxRdTrs(); cStgIdx += 1) {
				string name = genTrsName( *iDecl, cStgIdx);
				
				HtDecl * pTrsDecl = new HtDecl( Decl::Field, iDecl->getLineInfo() );
				pTrsDecl->setName( name );
				pTrsDecl->isRegister( iDecl->isRegister() );
				pTrsDecl->setQualType( iDecl->getQualType() );
				pTrsDecl->setDesugaredQualType( iDecl->getDesugaredQualType() );
				pTrsDecl->setTpGrp( iDecl->getTpGrp() );
				pTrsDecl->setTpGrpId( iDecl->getTpGrpId() );
				pTrsDecl->setTpSgId( iDecl->getTpSgId() );
				pTrsDecl->setTpSgUrs( iDecl->getTpSgUrs() );
				pTrsDecl->setTpSgWrTrs( cStgIdx );
				pTrsDecl->setTpSgTpd( TPD_REG_OUT );
				pTrsDecl->setTpSgRdTrs( cStgIdx );
				m_pHtDesign->getScModuleDecl()->addSubDecl( pTrsDecl );
			}
		}
	}

	void HtInsertRegisterStages::insertRegAssign(HtDeclList_t & moduleDeclList, HtStmtList & methodStmtList)
	{
		HtStmtIter iInsert ( methodStmtList );
		HtStmtIter iInsertEnd = iInsert.eol();

		for (HtDeclIter iDecl(moduleDeclList); !iDecl.isEol(); iDecl++) {
			if (iDecl->getTpSgMinWrTrs() == iDecl->getTpSgMaxRdTrs())
				continue;

			// found a decleration that needs staging
			for (int rStgIdx = iDecl->getTpSgMinWrTrs()+1; rStgIdx <= iDecl->getTpSgMaxRdTrs(); rStgIdx += 1) {

				// insert c_ assignments at beginning of module statement list
				if (rStgIdx <= iDecl->getTpSgMaxWrTrs()) {

					string rhsName = genTrsName( *iDecl, rStgIdx);
					string lhsName = genTrsName( *iDecl, rStgIdx, true);

					HtDecl * pLhsDecl = m_pHtDesign->getScModuleDecl()->findSubDecl( lhsName );
					HtDecl * pRhsDecl = m_pHtDesign->getScModuleDecl()->findSubDecl( rhsName );

					HtExpr * pHtLhs = new HtExpr( Stmt::MemberExprClass, pLhsDecl->getQualType(), pRhsDecl->getLineInfo() );
					pHtLhs->setMemberDecl( pLhsDecl );

					HtExpr * pHtRhs = new HtExpr( Stmt::MemberExprClass, pRhsDecl->getQualType(), pRhsDecl->getLineInfo() );
					pHtRhs->setMemberDecl( pRhsDecl );

					HtExpr * pThis = new HtExpr( Stmt::CXXThisExprClass, HtQualType( 0 ), pRhsDecl->getLineInfo() );
					pHtLhs->setBase( pThis );
					pHtRhs->setBase( pThis );

					HtDesign::genAssignStmts(iInsert, pHtLhs, pHtRhs);
				}

				// now insert register assignment statement at end of module statement list
				{
					string lhsName = genTrsName( *iDecl, rStgIdx);

					string rhsName;
					if (rStgIdx <= iDecl->getTpSgMaxWrTrs())
						rhsName = genTrsName( *iDecl, rStgIdx-1, true );
					else
						rhsName = genTrsName( *iDecl, rStgIdx-1 );

					HtDecl * pTrsDecl = m_pHtDesign->getScModuleDecl()->findSubDecl( lhsName );
					HtDecl * pTrsM1Decl = m_pHtDesign->getScModuleDecl()->findSubDecl( rhsName );

					HtExpr * pHtLhs = new HtExpr( Stmt::MemberExprClass, pTrsDecl->getQualType(), pTrsM1Decl->getLineInfo() );
					pHtLhs->setMemberDecl( pTrsDecl );

					HtExpr * pHtRhs = new HtExpr( Stmt::MemberExprClass, pTrsM1Decl->getQualType(), pTrsM1Decl->getLineInfo() );
					pHtRhs->setMemberDecl( pTrsM1Decl );

					HtExpr * pThis = new HtExpr( Stmt::CXXThisExprClass, HtQualType( 0 ), pTrsM1Decl->getLineInfo() );
					pHtLhs->setBase( pThis );
					pHtRhs->setBase( pThis );

					HtStmtIter iSave = iInsertEnd-1;
					HtDesign::genAssignStmts(iInsertEnd, pHtLhs, pHtRhs);
					iInsertEnd = iSave+1;
				}
			}
		}
	}

	HtExpr * HtInsertRegisterStages::findBaseExpr( HtExpr * pExpr )
	{
		// find the base declaration for the reference
		HtExpr * pBaseExpr = 0;
		for (;;) {
			Stmt::StmtClass stmtClass = pExpr->getStmtClass();
			switch (stmtClass) {
			case Stmt::MemberExprClass:
				pBaseExpr = pExpr;
				pExpr = pExpr->getBase().asExpr();
				break;
			case Stmt::ImplicitCastExprClass:
			case Stmt::CXXStaticCastExprClass:
				pExpr = pExpr->getSubExpr().asExpr();
				break;
			case Stmt::CXXThisExprClass:
				return pBaseExpr;
			default:
				assert(0);
			}
		}
		return 0;
	}

	HtDecl * HtInsertRegisterStages::findBaseDecl( HtExpr * pExpr )
	{
		// find the base declaration for the reference
		HtExpr * pBaseExpr = findBaseExpr( pExpr );
		HtDecl * pBaseDecl = pBaseExpr == 0 ? 0 : pBaseExpr->getMemberDecl();

		assert(pBaseDecl == 0 || pBaseDecl->getKind() == Decl::Field);
		return pBaseDecl;
	}
}
