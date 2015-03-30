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

	class HtArrangeCaseStmts : HtWalkStmtTree
	{
	public:
		HtArrangeCaseStmts(HtDesign * pHtDesign, ASTContext &Context)
			: m_pHtDesign(pHtDesign), m_context(Context), m_diags(Context.getDiagnostics()) {}

		void arrangeCaseStmts();

		bool walkCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid );
		void walkPostCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid ) { iStmt++; }

		void insertCompoundStmt( HtStmtList & stmtList );

	private:
		HtDesign * m_pHtDesign;
		ASTContext & m_context;
		DiagnosticsEngine & m_diags;
	};

	void HtDesign::arrangeCaseStmts(ASTContext &Context)
	{
		HtArrangeCaseStmts transform(this, Context);

		transform.arrangeCaseStmts();

		adjustCompoundStmts( Context );

		ExitIfError(Context);
	}

	void HtArrangeCaseStmts::arrangeCaseStmts()
	{
		HtDeclIter methodIter(m_pHtDesign->getScMethodDeclList());
		while ( !methodIter.isEol() ) {

			HtStmtIter stmtIter ( (*methodIter)->getBody() );

			HtStmtIter iEmpty;
			walkStmtTree( stmtIter, iEmpty, false, 0 );

			methodIter++;
		}
	}

	bool HtArrangeCaseStmts::walkCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid )
	{
		switch (iStmt->getStmtClass()) {
		case Stmt::CaseStmtClass:
		case Stmt::DefaultStmtClass:
			{
				HtStmtIter iStmt2 = iStmt;
				iStmt2++;

				HtStmtIter iSubStmt = HtStmtIter(iStmt->getSubStmt());

				if (iSubStmt->getStmtClass() == Stmt::CaseStmtClass ||
					iSubStmt->getStmtClass() == Stmt::DefaultStmtClass)
				{
					iStmt2.insert(iSubStmt);
					iSubStmt.erase();
					break;
				}

				// ensure case/default has a compound stmt
				insertCompoundStmt( iStmt->getSubStmt() );

				HtStmtList & iCompoundStmt = iStmt->getSubStmt();
				HtStmtList & iCompoundList = iCompoundStmt->getStmtList();
				
				// move remaining statments until next case/default into compound stmt
				while (!iStmt2.isEol()
					&& iStmt2->getStmtClass() != Stmt::CaseStmtClass
					&& iStmt2->getStmtClass() != Stmt::DefaultStmtClass)
				{
					iCompoundList.push_back(iStmt2.asStmt());
					HtStmtIter iDeleteStmt = iStmt2++;
					iDeleteStmt.erase();
				}

				// check if last stmt is a break
				HtStmtList * pList = &iCompoundList;
				while (pList->back().asStmt()->getStmtClass() == Stmt::CompoundStmtClass)
					pList = &(pList->back().asStmt()->getStmtList());

				HtStmt * pStmt = pList->back().asStmt();

				iStmt2 = iStmt;
				iStmt2++;

				if (pStmt->getStmtClass() != Stmt::BreakStmtClass) {
					if (iStmt2.isEol()) {
						if (pStmt->getStmtClass() != Stmt::NullStmtClass) {
							// just add a break statement
							HtStmt * pBreakStmt = new HtStmt( Stmt::BreakStmtClass, iStmt->getLineInfo() );
							pList->push_back( pBreakStmt );
						} else {
							// just add a break statement
							pStmt->setStmtClass( Stmt::BreakStmtClass );
						}
					} else {
						// error - must end case/default with a break
						unsigned DiagID = m_diags.getCustomDiagID(DiagnosticsEngine::Error,
							"htv/htt requires case/default statement list to end with a break");
						m_diags.Report(pStmt->getLineInfo().getLoc(), DiagID);
					}
				}
			}
			break;
		default:
			break;
		}

		return true;
	}

	void HtArrangeCaseStmts::insertCompoundStmt( HtStmtList & stmtList )
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

}
