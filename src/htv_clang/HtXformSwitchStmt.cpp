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

	// Transform switch statement
	// - transform list of statements for each case/default such that a single break
	//   statement exists at the end of the list.
	// - provide error if the statements from one case flows into the next (i.e. no break)
	class HtXformSwitchStmts : HtWalkStmtTree
	{
	public:
		HtXformSwitchStmts(HtDesign * pHtDesign, ASTContext &Context)
			: m_pHtDesign(pHtDesign), m_context(Context), m_diags(Context.getDiagnostics()) {}

		void xformSwitchStmts();
		bool transformCaseStmtListToRemoveBreaks(HtStmtIter iStmt, HtStmtIter iPost);
		bool hasEmbeddedBreak(HtStmtIter iStmt);

		bool walkCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid );
		void walkPostCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid ) { iStmt++; }

	private:
		HtDesign * m_pHtDesign;
		ASTContext & m_context;
		DiagnosticsEngine & m_diags;
	};

	void HtDesign::xformSwitchStmts(ASTContext &Context)
	{
		HtXformSwitchStmts transform(this, Context);

		transform.xformSwitchStmts();

		adjustCompoundStmts( Context );

		ExitIfError(Context);
	}

	void HtXformSwitchStmts::xformSwitchStmts()
	{
		HtDeclIter methodIter(m_pHtDesign->getScMethodDeclList());
		while ( !methodIter.isEol() ) {

			HtStmtIter stmtIter ( (*methodIter)->getBody() );

			HtStmtIter iEmpty;
			walkStmtTree( stmtIter, iEmpty, false, 0 );

			methodIter++;
		}
	}


	bool HtXformSwitchStmts::walkCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid )
	{
		switch (iStmt->getStmtClass()) {
		case Stmt::CaseStmtClass:
		case Stmt::DefaultStmtClass:
			{
				// make sure each case compound statement list has just one break
				// and it is at the end of the compound statement list
				HtStmtIter iSubStmt = HtStmtIter(iStmt->getSubStmt());

				if (iSubStmt.isEol())
					break;

				assert(iSubStmt->getStmtClass() == Stmt::CompoundStmtClass);

				HtStmtIter iEmpty;
				bool hasBreak = transformCaseStmtListToRemoveBreaks( HtStmtIter(iSubStmt->getStmtList()), iEmpty );

				if (!hasBreak) {
					unsigned DiagID = m_diags.getCustomDiagID(DiagnosticsEngine::Error,
						"case/default statement lists must end with a break statement");
					m_diags.Report(iSubStmt->getLineInfo().getLoc(), DiagID);
				}

				HtStmt * pBreak = new HtStmt( Stmt::BreakStmtClass, iSubStmt->getLineInfo() );
				iSubStmt.appendToEol(pBreak);
			}
			break;
		default:
			break;
		}

		return true;
	}

	bool HtXformSwitchStmts::transformCaseStmtListToRemoveBreaks(HtStmtIter iStmt, HtStmtIter iPost)
	{
		//   transform if statements with embedded breaks to eliminate the extra breaks
		// The algorithm merges post if-else statements into the if-else, removing any
		//   internal breaks. The algorithm starts at the end of the statement list and
		//   works to the beginning using recussion.

		if (!iStmt.isEol() && iStmt->getStmtClass() == Stmt::CompoundStmtClass)
			iStmt = HtStmtIter(iStmt->getStmtList());

		while (!iStmt.isEol()
			&& iStmt->getStmtClass() != Stmt::IfStmtClass
			&& iStmt->getStmtClass() != Stmt::BreakStmtClass)
		{
			iStmt++;
		}

		if (iStmt.isEol()) {
			iStmt.appendToEol(iPost, iPost.eol());
			return false;
		}

		if (iStmt->getStmtClass() == Stmt::BreakStmtClass) {
			iStmt.eraseToEol();	// delete break and any dead code after break
			return true;
		}

		HtStmtIter iNext = iStmt+1;
		bool isPostBreak = transformCaseStmtListToRemoveBreaks( iNext, iPost );

		bool hasThenBreak = hasEmbeddedBreak( HtStmtIter(iStmt->getThen()) );
		bool hasElseBreak = hasEmbeddedBreak( HtStmtIter(iStmt->getElse()) );

		if (hasThenBreak || hasElseBreak) {
			iNext = iStmt+1;	// must update iNext
			transformCaseStmtListToRemoveBreaks( HtStmtIter(iStmt->getThen()), iNext );
			transformCaseStmtListToRemoveBreaks( HtStmtIter(iStmt->getElse()), iNext );

			iNext.eraseToEol();
		}

		return isPostBreak;
	}

	bool HtXformSwitchStmts::hasEmbeddedBreak(HtStmtIter iStmt)
	{
		if (!iStmt.isEol() && iStmt->getStmtClass() == Stmt::CompoundStmtClass)
			iStmt = HtStmtIter(iStmt->getStmtList());

		while (!iStmt.isEol()
			&& iStmt->getStmtClass() != Stmt::IfStmtClass
			&& iStmt->getStmtClass() != Stmt::BreakStmtClass)
		{
			iStmt++;
		}

		if (iStmt.isEol())
			return false;

		if (iStmt->getStmtClass() == Stmt::BreakStmtClass)
			return true;

		HtStmtIter iNext = iStmt+1;
		bool hasPostBreak = hasEmbeddedBreak( iNext );

		bool hasThenBreak = hasEmbeddedBreak( HtStmtIter(iStmt->getThen()) );
		bool hasElseBreak = hasEmbeddedBreak( HtStmtIter(iStmt->getElse()) );

		return hasPostBreak || hasThenBreak || hasElseBreak;
	}

}
