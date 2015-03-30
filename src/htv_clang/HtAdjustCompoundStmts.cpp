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

	class HtAdjustCompoundStmts : HtWalkStmtTree
	{
	public:
		HtAdjustCompoundStmts(HtDesign * pHtDesign, ASTContext &Context)
			: m_pHtDesign(pHtDesign), m_context(Context), m_diags(Context.getDiagnostics()) {}

		void adjustCompoundStmts();

		bool walkCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid );
		void walkPostCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid );

		void insertCompoundStmt( HtStmtList & stmtList );

	private:
		HtDesign * m_pHtDesign;
		ASTContext & m_context;
		DiagnosticsEngine & m_diags;

		vector<Stmt::StmtClass> m_stmtClassStk;
	};

	void HtDesign::adjustCompoundStmts(ASTContext &Context)
	{
		HtAdjustCompoundStmts transform(this, Context);

		transform.adjustCompoundStmts();

		ExitIfError(Context);
	}

	void HtAdjustCompoundStmts::adjustCompoundStmts()
	{
		HtDeclIter methodIter(m_pHtDesign->getScMethodDeclList());
		while ( !methodIter.isEol() ) {

			HtStmtIter iStmt ( (*methodIter)->getBody() );

			HtStmtIter iEmpty;

			m_stmtClassStk.push_back( Stmt::NullStmtClass );
			walkStmtTree( iStmt, iEmpty, false, 0 );
			m_stmtClassStk.pop_back();

			assert(m_stmtClassStk.empty());

			methodIter++;
		}
	}

	bool HtAdjustCompoundStmts::walkCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid )
	{
		Stmt::StmtClass stmtClass = m_stmtClassStk.back();

		switch (iStmt->getStmtClass()) {
		case Stmt::CompoundStmtClass:

			if (stmtClass != Stmt::NullStmtClass
				&& stmtClass != Stmt::SwitchStmtClass
				&& stmtClass != Stmt::CaseStmtClass
				&& stmtClass != Stmt::DefaultStmtClass
				&& stmtClass != Stmt::IfStmtClass)
			{
				// move sub statements up and remove compound stmt
				HtStmtIter origStmt = iStmt++;
				iStmt.insert( origStmt->getStmtList() );

				iStmt = origStmt;
				iStmt++;

				origStmt.erase();

				// must process iStmt since previous iStmt was deleted
				walkCallback( iStmt, iInsert, isLoa, pVoid );

				return true;
			}
			break;
		case Stmt::IfStmtClass:
			if (iStmt->getThen().size() > 1)
				insertCompoundStmt(iStmt->getThen());
			if (iStmt->getElse().size() > 1)
				insertCompoundStmt(iStmt->getElse());
			if (iStmt->getElse().size() == 1
				&& iStmt->getElse().asStmt()->getStmtClass() == Stmt::CompoundStmtClass
				&& iStmt->getElse().asStmt()->getStmtList().size() == 0)
			{
				// remove empty compound statement on else clause
				iStmt->getElse().clear();
			}
			break;
		default:
			break;
		}
			
		m_stmtClassStk.push_back( iStmt->getStmtClass() );

		return true;
	}

	void HtAdjustCompoundStmts::insertCompoundStmt( HtStmtList & stmtList )
	{
		// create a compound statement
		HtStmt * pStmt = new HtStmt( Stmt::CompoundStmtClass, stmtList->getLineInfo() );

		// move existing statements to compound list
		HtStmtList_iter_t iter = pStmt->getStmtList().begin();
		pStmt->getStmtList().insert( iter, stmtList.begin(), stmtList.end() );

		// clear original stmtList
		stmtList.clear();

		// insert new compound stmt on original list
		stmtList.push_back(pStmt);
	}

	void HtAdjustCompoundStmts::walkPostCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid )
	{
		m_stmtClassStk.pop_back();
		iStmt++;
	}

}
