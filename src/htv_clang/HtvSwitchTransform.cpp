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
#include "HtvDesign.h"

namespace htv {

	// Transform switch statement
	// - transform list of statements for each case/default such that a single break
	//   statement exists at the end of the list.
	// - provide error if the statements from one case flows into the next (i.e. no break)

	class HtvTransformSwitchStmts : public HtWalkDesign
	{
	public:
		void handleSwitchStmt(HtStmt * pStmt);

		void transformSwitchStmt(HtvStmt * pSwitchStmt);
		void transformCaseStmtListToRemoveBreaks(HtvStmt *pCaseHead);
		void transformCaseIfStmtList(HtvStmt **ppList, HtvStmt *pPostList);
		bool isEmbeddedBreakStmt(HtvStmt *pStmt);
	};

	void HtvDesign::transformSwitchStmts()
	{
		HtvTransformSwitchStmts transform;

		transform.walkDesign( this );
	}

	void HtvTransformSwitchStmts::handleSwitchStmt(HtStmt * pHtSwitchStmt)
	{
		HtvStmt * pSwitchStmt = (HtvStmt *)pHtSwitchStmt;

		// walk statements within switch and perform case statement transformations
		assert(pSwitchStmt->getStmtClass() == Stmt::SwitchStmtClass);

		HtvStmt * pCmpndStmt = pSwitchStmt->getBody();
		assert(pCmpndStmt->getStmtClass() == Stmt::CompoundStmtClass);

		// clang puts all statements within a switch into a single list
		// first transform: add a compound statement foreach group of
		// case/default entry points and put individual statements in
		// the compound statement's list

		HtvStmt * pStmt = pCmpndStmt->getFirstStmt();
		while (pStmt) {
			Stmt::StmtClass stmtClass = pStmt->getStmtClass();

			if ((stmtClass == Stmt::CaseStmtClass || stmtClass == Stmt::DefaultStmtClass)
				&& pStmt->getSubStmt() != 0)
			{
				HtvStmt * pCaseStmt = pStmt;

				// create a compound statement
				HtvStmt * pNewCompound = new HtvStmt(Stmt::CompoundStmtClass);
				pNewCompound->addStmt( pCaseStmt->getSubStmt() );

				pCaseStmt->setSubStmt( pNewCompound );

				HtvStmt * pNextStmt = pStmt->getNextStmt();
				for (;;) {
					if (pNextStmt == 0)
						break;

					stmtClass = pNextStmt->getStmtClass();
					if (stmtClass == Stmt::CaseStmtClass || stmtClass == Stmt::DefaultStmtClass)
						break;

					pStmt = pNextStmt;
					pNextStmt = pStmt->getNextStmt();
					pStmt->setNextStmt(0);

					pNewCompound->addStmt(pStmt);
				}

				// put remainder of switch statements after original case statement
				pCaseStmt->setNextStmt( pNextStmt );

				// resume from case statement
				pStmt = pCaseStmt;
			}

			pStmt = pStmt->getNextStmt();
		}

		// second transform - make sure each case compound statement list has just one break
		// and it is at the end of the compound statement list
		pStmt = pCmpndStmt->getFirstStmt();
		while (pStmt) {
			Stmt::StmtClass stmtClass = pStmt->getStmtClass();

			if ((stmtClass == Stmt::CaseStmtClass || stmtClass == Stmt::DefaultStmtClass)
				&& pStmt->getSubStmt() != 0)
			{
				// found the first of a list of statements
				HtvStmt * pCaseCmpnd = pStmt->getSubStmt();
				transformCaseStmtListToRemoveBreaks(pCaseCmpnd->getFirstStmt());
			}

			pStmt = pStmt->getNextStmt();
		}
	}

	void HtvTransformSwitchStmts::transformCaseStmtListToRemoveBreaks(HtvStmt *pCaseHead)
	{
		// verilog does not allow multiple breaks within a case statement
		//   transform if statements with embedded breaks to eliminate the extra breaks
		// The algorithm merges post if-else statements into the if-else, removing any
		//   internal breaks. The algorithm starts at the end of the statement list and
		//   works to the beginning using recussion.

		HtvStmt *pStIf = pCaseHead;
		while (pStIf && (pStIf->getStmtClass() != Stmt::IfStmtClass || !(isEmbeddedBreakStmt(pStIf->getThen()) ||
			isEmbeddedBreakStmt(pStIf->getElse()))))
			pStIf = pStIf->getNextStmt();

		if (pStIf) {
			// found an if statement, see if there are more if's in the list
			HtvStmt *pStIf2 = pStIf->getNextStmt();
			while (pStIf2 && (pStIf2->getStmtClass() != Stmt::IfStmtClass || !(isEmbeddedBreakStmt(pStIf2->getThen()) ||
				isEmbeddedBreakStmt(pStIf2->getElse()))))
				pStIf2 = pStIf2->getNextStmt();

			if (pStIf2)
				transformCaseStmtListToRemoveBreaks(pStIf2);

			if (isEmbeddedBreakStmt(pStIf)) {
				// found an embedded break statement, find the last break

				HtvStmt *pStmt = pStIf;
				while (pStmt && pStmt->getNextStmt() && pStmt->getNextStmt()->getStmtClass() != Stmt::BreakStmtClass)
					pStmt = pStmt->getNextStmt();

				HtvStmt *pBreak = pStmt->getNextStmt();
				pStmt->setNextStmt(0);

				HtvStmt *pPostList = pStIf->getNextStmt();
				pStIf->setNextStmt(pBreak);

				transformCaseIfStmtList(pStIf->getPThen(), pPostList);
				transformCaseIfStmtList(pStIf->getPElse(), pPostList);

				if (pStIf->getThen() == 0)
					// insert a null statement
					pStIf->setThen( new HtvStmt(Stmt::NullStmtClass) );
			}
		}
	}

	void HtvTransformSwitchStmts::transformCaseIfStmtList(HtvStmt **ppList, HtvStmt *pPostList)
	{
		// Append postList on to pIfList provided pIfList doesn't end in a break
		HtvStmt **ppTail = ppList;
		while (*ppTail && (*ppTail)->getNextStmt())
			ppTail = (*ppTail)->getPNextStmt();

		if (*ppTail == 0)
			*ppTail = pPostList;
		else if ((*ppTail)->getStmtClass() != Stmt::BreakStmtClass)
			(*ppTail)->setNextStmt(pPostList);
		else
			*ppTail = 0;

		transformCaseStmtListToRemoveBreaks(*ppList);
	}

	bool HtvTransformSwitchStmts::isEmbeddedBreakStmt(HtvStmt *pStmt)
	{
		while (pStmt) {
			switch (pStmt->getStmtClass()) {
			case Stmt::SwitchStmtClass:
				// no possibility of an embedded break in these statements
				break;
			case Stmt::BreakStmtClass:
				return true;
			case Stmt::IfStmtClass:
				if (isEmbeddedBreakStmt(pStmt->getThen()) ||
					isEmbeddedBreakStmt(pStmt->getElse()))
					return true;
				break;
			default:
				if (pStmt->isAssignStmt())
					break;
				HtAssert(0);
			}
			pStmt = pStmt->getNextStmt();
		}
		return false;
	}
}
