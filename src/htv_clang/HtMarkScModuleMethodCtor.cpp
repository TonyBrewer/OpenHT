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

	class HtMarkScModuleMethodCtor
	{
	public:
		HtMarkScModuleMethodCtor(HtDesign * pHtDesign, ASTContext &Context) 
			: m_pHtDesign(pHtDesign), m_context(Context), m_diags(Context.getDiagnostics())
		{
			m_foundScModule = false;
			m_foundScCtor = false;
			m_foundScMethod = false;
			m_pHtPragmaList = 0;
		}

		void walkDesign();
		void walkStmtList(HtStmtIter & stmtIter);
		void handleTopDecl(HtDeclList_t &declList, HtDeclList_iter_t & declIter);
		void handleCXXMemberDecl(HtDeclList_t &declList, HtDeclList_iter_t & declIter);
		void handlePragmaHtv(HtDecl * pPragmaDecl);
		string findPragmaArgString(HtStmtIter & list);

	private:
		HtDesign * m_pHtDesign;
		ASTContext & m_context;
		DiagnosticsEngine & m_diags;
		bool m_foundScModule;
		bool m_foundScCtor;
		bool m_foundScMethod;
		vector<HtPragma *> * m_pHtPragmaList;
	};

	void HtDesign::markScModuleMethodCtor(ASTContext &Context)
	{
		HtMarkScModuleMethodCtor transform( this, Context );

		transform.walkDesign();
		
		ExitIfError(Context);
	}

	void HtMarkScModuleMethodCtor::walkDesign()
	{
		// start with top level declarations
		HtDeclList_t & declList = m_pHtDesign->getTopDeclList();
		for (HtDeclList_iter_t declIter = declList.begin();
			declIter != declList.end(); )
		{
			HtDecl * pHtDecl = *declIter;

			handleTopDecl(declList, declIter);

			if (declIter != declList.end() && pHtDecl == *declIter)
				declIter++;
		}

		if (!m_foundScModule) {
			unsigned DiagID = m_diags.getCustomDiagID(DiagnosticsEngine::Error,
				"did not find SC_MODULE");
			m_diags.Report(DiagID);

		} else if (!m_foundScCtor) {
			unsigned DiagID = m_diags.getCustomDiagID(DiagnosticsEngine::Error,
				"did not find SC_CTOR in SC_CTOR");
			m_diags.Report(DiagID);

		} else if (!m_foundScMethod) {
			unsigned DiagID = m_diags.getCustomDiagID(DiagnosticsEngine::Error,
				"did not find SC_METHOD in SC_METHOD");
			m_diags.Report(DiagID);
		}
	}

	void HtMarkScModuleMethodCtor::handleTopDecl(HtDeclList_t &declList, HtDeclList_iter_t & declIter)
	{
		HtDecl * pDecl = *declIter;

		if (pDecl->getKind() == Decl::PragmaHtv) {

			// found HTV pragma
			handlePragmaHtv(pDecl);

			HtDeclList_iter_t eraseDecl = declIter++;
			declList.erase(eraseDecl);

			return; // avoid pragma list test

		}

		if (m_pHtPragmaList) {
			pDecl->setPragmaList(m_pHtPragmaList);

			for (size_t i = 0; i < m_pHtPragmaList->size(); i += 1) {
				HtPragma * pPragma = (*m_pHtPragmaList)[i];

				if (pPragma->isTpp())
					m_pHtDesign->addPragmaTpp( *declIter, pPragma );
			}

			if (pDecl->isScModule()) {
				m_foundScModule = true;
				m_pHtDesign->setScModuleDecl(pDecl);
			}
			m_pHtPragmaList = 0;
		}
		
		if (pDecl->getKind() == Decl::CXXRecord) {

			// walk members of record
			HtDeclList_t & subDeclList = pDecl->getSubDeclList();
			for (HtDeclList_iter_t subDeclIter = subDeclList.begin();
				subDeclIter != subDeclList.end(); )
			{
				HtDecl * pSubDecl = *subDeclIter;

				handleCXXMemberDecl(subDeclList, subDeclIter);

				if (subDeclIter != subDeclList.end() && pSubDecl == *subDeclIter)
					subDeclIter++;
			}
		}
	}

	void HtMarkScModuleMethodCtor::handleCXXMemberDecl(HtDeclList_t &declList, HtDeclList_iter_t & declIter)
	{
		HtDecl * pDecl = *declIter;

		if (pDecl->getKind() == Decl::PragmaHtv) {

			// found HTV pragma
			handlePragmaHtv(pDecl);

			HtDeclList_iter_t eraseDecl = declIter++;
			declList.erase(eraseDecl);

			return;
		}

		if (m_pHtPragmaList) {

			if ((*declIter)->getKind() == Decl::Field) {
				int firstFieldLine = (*declIter)->getLineInfo().getBeginLine();
				int firstFieldCol = (*declIter)->getLineInfo().getBeginColumn();

				while (declIter != declList.end() && (*declIter)->getKind() == Decl::Field
					&& (*declIter)->getLineInfo().getBeginColumn() == firstFieldCol
					&& (*declIter)->getLineInfo().getBeginLine() == firstFieldLine)
				{
					(*declIter)->setPragmaList( m_pHtPragmaList );

					for (size_t i = 0; i < m_pHtPragmaList->size(); i += 1) {
						HtPragma * pPragma = (*m_pHtPragmaList)[i];

						if (pPragma->isTps() || pPragma->isTpe())
							m_pHtDesign->addPragmaTpsTpe( *declIter, pPragma );
					}

					declIter++;
				}
			} else
				(*declIter)->setPragmaList( m_pHtPragmaList );

			if (pDecl->isScModule())
				m_pHtDesign->setScModuleDecl(pDecl);
			m_pHtPragmaList = 0;
		}

		if (pDecl->isScCtor()) {

			if (pDecl->getKind() != Decl::CXXConstructor) {
				unsigned DiagID = m_diags.getCustomDiagID(DiagnosticsEngine::Error,
					"expected a SC_CTOR");
				m_diags.Report((*declIter)->getLineInfo().getLoc(), DiagID);
				return;
			}

			m_foundScCtor = true;

			m_pHtDesign->setScCtorDecl(pDecl);

			if ((*declIter)->hasBody()) {
				HtStmtIter bodyStmtIter( (*declIter)->getBody() );
				walkStmtList( bodyStmtIter );
			}

			return;
		}
	}

	void HtMarkScModuleMethodCtor::handlePragmaHtv(HtDecl * pPragmaDecl)
	{
		unsigned DiagID = m_diags.getCustomDiagID(DiagnosticsEngine::Error,
			"unexpected pragma htv format");

		ArrayRef<Token const> tokens = pPragmaDecl->getPragmaTokens();

		ArrayRef<Token const>::const_iterator I = tokens.begin();
		if (I->getKind() != tok::identifier) {
			m_diags.Report(pPragmaDecl->getLineInfo().getLoc(), DiagID);
			return;
		}

		string name = string(I->getIdentifierInfo()->getNameStart(), I->getIdentifierInfo()->getLength());
		I++;

		HtPragma * pHtPragma = 0;

		if (name == "SC_MODULE") {
			pHtPragma = new HtPragma(HtPragma::ScModule);
		} else if (name == "SC_CTOR") {
			pHtPragma = new HtPragma(HtPragma::ScCtor);
		} else if (name == "SC_METHOD") {
			pHtPragma = new HtPragma(HtPragma::ScMethod);
		} else if (name == "prim") {
			pHtPragma = new HtPragma(HtPragma::HtPrim);
		} else if (name == "tps") {
			pHtPragma = new HtPragma(HtPragma::HtTps);

			if (I->getKind() == tok::string_literal) {
				string pragmaArg(I->getLiteralData(), I->getLength());
				pHtPragma->addArgString(pragmaArg);
				I++;
			}

		} else if (name == "tpe") {
			pHtPragma = new HtPragma(HtPragma::HtTpe);

			if (I->getKind() == tok::string_literal) {
				string pragmaArg(I->getLiteralData(), I->getLength());
				pHtPragma->addArgString(pragmaArg);
				I++;
			}

		} else if (name == "tpp") {
			pHtPragma = new HtPragma(HtPragma::HtTpp);

			// parse timing arguments
			if (I->getKind() != tok::string_literal) {
				delete pHtPragma;
				unsigned DiagID1 = m_diags.getCustomDiagID(DiagnosticsEngine::Error,
					"htv timing pragma requires at least one timing specifier");
				m_diags.Report(pPragmaDecl->getLineInfo().getLoc(), DiagID1);
				return;
			}

			while (I->getKind() == tok::string_literal) {
				string pragmaArg(I->getLiteralData(), I->getLength());
				pHtPragma->addArgString(pragmaArg);
				I++;

				if (I->getKind() == tok::comma)
					I++;
			}

		} else {
			m_diags.Report(pPragmaDecl->getLineInfo().getLoc(), DiagID);
			return;
		}

		if (I != tokens.end()) {
			// extra info at end of line
			delete pHtPragma;
			m_diags.Report(pPragmaDecl->getLineInfo().getLoc(), DiagID);
			return;
		}

		if (pHtPragma && !m_pHtPragmaList)
			m_pHtPragmaList = new vector<HtPragma *>;

		m_pHtPragmaList->push_back(pHtPragma);
	}

	string HtMarkScModuleMethodCtor::findPragmaArgString(HtStmtIter & argIter)
	{
		HtExpr * pArgExpr = argIter.asExpr();
		if (pArgExpr->getStmtClass() != Stmt::ImplicitCastExprClass) {
			unsigned DiagID = m_diags.getCustomDiagID(DiagnosticsEngine::Error,
				"unexpected ht pragma argument structure");
			m_diags.Report(pArgExpr->getLineInfo().getLoc(), DiagID);
			return 0;
		}

		HtStmtList & stringLiteral = pArgExpr->getSubExpr();
		if (stringLiteral->getStmtClass() != Stmt::StringLiteralClass) {
			unsigned DiagID = m_diags.getCustomDiagID(DiagnosticsEngine::Error,
				"unexpected ht pragma argument type");
			m_diags.Report(pArgExpr->getLineInfo().getLoc(), DiagID);
			return 0;
		}

		return stringLiteral.asExpr()->getStringLiteralAsStr();
	}

	void HtMarkScModuleMethodCtor::walkStmtList(HtStmtIter & iStmt)
	{
		for ( ; !iStmt.isEol(); )
		{
			if (iStmt->getStmtClass() == Stmt::DeclStmtClass) {
				HtDeclList_iter_t declIter = iStmt->getDeclList().begin();

				if ((*declIter)->getKind() != Decl::PragmaHtv) {
					iStmt++;
					continue;
				}

				handlePragmaHtv(*declIter);

				// remove decl stmt and mark next statemenmt as a ScMethod
				iStmt->getDeclList().erase(declIter);
				HtStmtIter eraseIter = iStmt++;
				eraseIter.erase();

				continue;
			}

			if (m_pHtPragmaList) {
				iStmt->setPragmaList(m_pHtPragmaList);
				m_pHtPragmaList = 0;
			}

			if (iStmt->isScMethod()) {

				if (iStmt->getStmtClass() != Stmt::CXXMemberCallExprClass) {
					unsigned DiagID = m_diags.getCustomDiagID(DiagnosticsEngine::Error,
						"expected a SC_METHOD");
					m_diags.Report(iStmt->getLineInfo().getLoc(), DiagID);
					continue;
				}

				HtExpr * pHtExpr = iStmt.asExpr()->getCalleeExpr().asExpr();
				if (pHtExpr == 0 || pHtExpr->getStmtClass() != Stmt::MemberExprClass) {
					unsigned DiagID = m_diags.getCustomDiagID(DiagnosticsEngine::Error,
						"expected a SC_METHOD");
					m_diags.Report(iStmt->getLineInfo().getLoc(), DiagID);
					continue;
				}

				HtDecl * pHtDecl = pHtExpr->getMemberDecl();
				if (pHtDecl == 0 || pHtDecl->getKind() != Decl::CXXMethod) {
					unsigned DiagID = m_diags.getCustomDiagID(DiagnosticsEngine::Error,
						"expected a SC_METHOD");
					m_diags.Report(iStmt->getLineInfo().getLoc(), DiagID);
					continue;
				}

				pHtDecl->isScMethod(true);

				m_pHtDesign->addScMethodDecl( pHtDecl );

				m_foundScMethod = true;
			}

			if (iStmt->getStmtClass() == Stmt::CompoundStmtClass) {
				HtStmtList & subStmtList = iStmt->getStmtList();
				HtStmtIter subStmtIter( subStmtList );
				while (!subStmtIter.isEol())
				{
					size_t listSize = subStmtList.size();

					walkStmtList( subStmtIter );

					// stay on current statement if original statement was deleted
					if (listSize == subStmtList.size())
						subStmtIter++;
				}
			}

			iStmt++;
		}
	}
}

