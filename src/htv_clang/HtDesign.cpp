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
	int HtNode::m_nextId = 0;
	HtNode::NodePtr HtNode::m_nodes[];
	int HtLineInfo::m_nextFileNum = 1;
	HtLineInfo::HtNameMap_map HtLineInfo::m_pathNameMap;
	clang::SourceManager * HtLineInfo::m_pSourceManager;

	string HtDesign::findUniqueName(string name, HtLineInfo & lineInfo) {
		// unique names have the format name$F$LC, where:
		//   F - fileNum
		//   L - lineNum
		//   C - unique char string, if needed

		// first check if name passed in is already a temp
		string uniqName;
		int pos = name.find('$');
		if (pos > 0) {
			// already a temp, find name without C
			size_t len;
			for (len = name.length(); len > 0; len -= 1) {
				if (isdigit(name[len-1]))
					break;
			}
			uniqName = name.substr(0, len);
		} else {
			char buf[128];
			sprintf(buf, "%s$%d$%d", name.c_str(), lineInfo.m_fileNum, lineInfo.m_lineNum );
			name = buf;
			uniqName = name;
		}

		// find 
		HtNameMap_insertPair insertPair = m_uniqueNameMap.insert(HtNameMap_valuePair( uniqName, 0 ));

		if (insertPair.first->second > 0) {
			// base temp already exists, add char string
			char letters[33] = { 0 };
			int idx = 31;
			for (int i = insertPair.first->second; i > 0; i /= 26)
				letters[idx--] = 'a' + (i-1) % 26;
			uniqName += letters[idx+1];
			insertPair.first->second += 1;

			insertPair = m_uniqueNameMap.insert(HtNameMap_valuePair( uniqName, 0 ));
			assert(insertPair.first->second == 0);
		}
		insertPair.first->second += 1;

		if (uniqName == "a$3$37a")
			bool stop = true;
		return uniqName;
	}

	string HtDesign::getSourceLine(ASTContext &Context, SourceLocation Loc)
	{
		SourceManager & SM = Context.getSourceManager();

		// Decompose the location into a FID/Offset pair.
		std::pair<FileID, unsigned> LocInfo = SM.getDecomposedLoc(Loc);
		FileID FID = LocInfo.first;
		unsigned FileOffset = LocInfo.second;

		// Get information about the buffer it points into.
		bool Invalid = false;
		const char *BufStart = SM.getBufferData(FID, &Invalid).data();
		//if (Invalid)
		//return;

		unsigned LineNo = SM.getLineNumber(FID, FileOffset);
		unsigned ColNo = SM.getColumnNumber(FID, FileOffset);
  
		// Arbitrarily stop showing snippets when the line is too long.
		//static const size_t MaxLineLengthToPrint = 4096;
		//if (ColNo > MaxLineLengthToPrint)
		//return;

		// Rewind from the current position to the start of the line.
		const char *TokPtr = BufStart+FileOffset;
		const char *LineStart = TokPtr-ColNo+1; // Column # is 1-based.

		// Compute the line end.  Scan forward from the error position to the end of
		// the line.
		const char *LineEnd = TokPtr;
		while (*LineEnd != '\n' && *LineEnd != '\r' && *LineEnd != '\0')
		++LineEnd;

		// Arbitrarily stop showing snippets when the line is too long.
		//if (size_t(LineEnd - LineStart) > MaxLineLengthToPrint)
		//return;

		// Copy the line of code into an std::string for ease of manipulation.
		return string(LineStart, LineEnd);
	}

	void ExitIfError(clang::ASTContext &Context)
	{
		DiagnosticsEngine & diags = Context.getDiagnostics();
		if (diags.hasErrorOccurred()) {
			unsigned DiagID = diags.getCustomDiagID(DiagnosticsEngine::Fatal,
				"previous errors prevent further compilation");
			diags.Report(DiagID);
			exit(1);
		}
	}

	void HtDesign::initTrsInfo()
	{
		for (size_t i = 0; i < m_tpgList.size(); i += 1)
			m_tpgList[i]->initTrsInfo();
	}

	void HtDesign::initUrsInfo()
	{
		for (size_t i = 0; i < m_tpgList.size(); i += 1)
			m_tpgList[i]->initUrsInfo();
	}

	int HtDesign::findTpGrpIdFromName(string & name)
	{
		for (size_t i = 0; i < m_tpgList.size(); i += 1) {
			if (m_tpgList[i]->getName() == name)
				return (int)i;
		}
		return -1;
	}
	HtTpg * HtDesign::getTpGrpFromGrpId(int grpId)
	{
		assert(grpId >= 0 && grpId < m_tpgList.size());
		return m_tpgList[grpId];
	}

	void HtDesign::addPragmaTpp( HtDecl * pDecl, HtPragma * pPragma)
	{
		// #pragma htv tpp "lhs trs=0 tsu=0", "rhs trs=0 tsu=0", "rslt trs=2 tco=0"

		string name;

		for (size_t i = 0; i < pPragma->getArgStrings().size(); i += 1) {
			string const & tpArg = pPragma->getArgStrings()[i];
			char const * pStr = tpArg.c_str();

			if (*pStr == '"') pStr++;
			while (*pStr == ' ' || *pStr == ',') pStr++;

			// first argument must be parameter name
			char const * pStart = pStr;
			while (isalpha(*pStr) || isdigit(*pStr) || *pStr == '_' || *pStr == '-')
				pStr += 1;
				
			name = string(pStart, pStr-pStart);

			HtDecl * pParamDecl = 0;

			// find parameter
			for (HtDeclList_iter_t paramIter = pDecl->getParamDeclList().begin(); 
				paramIter != pDecl->getParamDeclList().end(); paramIter++)
			{
				if ((*paramIter)->getName() == name) {
					pParamDecl = *paramIter;
					break;
				}
			}

			if (pParamDecl == 0) {
				unsigned DiagID = m_pDiagEngine->getCustomDiagID(DiagnosticsEngine::Error,
					"tpp error, parameter name not found in function parameters");
				m_pDiagEngine->Report(DiagID);
				continue;
			}

			bool bParamIsOutput = isParamOutput(pParamDecl->getQualType());

			int trs = -1;
			int tpd = -1;

			// now parse trs/tsu/tco
			while (*pStr == ' ' || *pStr == ',') pStr++;

			bool bTsu = false;
			while (*pStr != '"') {
				if (strncmp(pStr, "trs", 3) == 0) {
					pStr += 3;
					while (*pStr == ' ') pStr++;

					if (trs >= 0) {
						unsigned DiagID = m_pDiagEngine->getCustomDiagID(DiagnosticsEngine::Error,
							"tps/tpe error, register stage parameter specified multiple times");
						m_pDiagEngine->Report(DiagID);
						return;
					}

					if (*pStr++ != '=') {
						unsigned DiagID = m_pDiagEngine->getCustomDiagID(DiagnosticsEngine::Error,
							"tps/tpe error while parsing register stage parameter (trs=<integer>)");
						m_pDiagEngine->Report(DiagID);
						return;
					}

					while (*pStr == ' ') pStr++;
					char * pEnd;
					trs = strtol(pStr, &pEnd, 10);

					if (pStr == pEnd || trs < 0) {
						unsigned DiagID = m_pDiagEngine->getCustomDiagID(DiagnosticsEngine::Error,
							"tps/tpe error while parsing register stage parameter (trs=<integer>)");
						m_pDiagEngine->Report(DiagID);
						return;
					}

					pStr = pEnd;
					while (*pStr == ' ' || *pStr == ',') pStr++;
				
				} else if ((bTsu = (strncmp(pStr, "tsu", 3) == 0)) || strncmp(pStr, "tco", 3) == 0) {
					pStr += 3;
					while (*pStr == ' ') pStr++;

					if (bTsu && pPragma->isTps()) {
						unsigned DiagID = m_pDiagEngine->getCustomDiagID(DiagnosticsEngine::Error,
							"tps error while parsing timing parameters, expected (tco=<integer>)");
						m_pDiagEngine->Report(DiagID);
					} else if (!bTsu && pPragma->isTpe()) {
						unsigned DiagID = m_pDiagEngine->getCustomDiagID(DiagnosticsEngine::Error,
							"tpe error while parsing timing parameters, expected (tsu=<integer>)");
						m_pDiagEngine->Report(DiagID);
					}

					if (tpd >= 0) {
						unsigned DiagID = m_pDiagEngine->getCustomDiagID(DiagnosticsEngine::Error,
							"multiple tsu/tco parameters specified");
						m_pDiagEngine->Report(DiagID);
						return;
					}

					if (*pStr++ != '=') {
						if (bTsu) {
							unsigned DiagID = m_pDiagEngine->getCustomDiagID(DiagnosticsEngine::Error,
								"tpe error while parsing setup parameter (tsu=<integer>)");
							m_pDiagEngine->Report(DiagID);
						} else {
							unsigned DiagID = m_pDiagEngine->getCustomDiagID(DiagnosticsEngine::Error,
								"tps error while parsing clock to out parameter (tco=<integer>)");
							m_pDiagEngine->Report(DiagID);
						}
						return;
					}
				
					while (*pStr == ' ') pStr++;
					char * pEnd;
					tpd = strtol(pStr, &pEnd, 10);

					if (pStr == pEnd || tpd < 0) {
						if (bTsu) {
							unsigned DiagID = m_pDiagEngine->getCustomDiagID(DiagnosticsEngine::Error,
								"parsing setup parameter (tsu=<integer>)");
							m_pDiagEngine->Report(DiagID);
						} else {
							unsigned DiagID = m_pDiagEngine->getCustomDiagID(DiagnosticsEngine::Error,
								"parsing clock to out parameter (tco=<integer>)");
							m_pDiagEngine->Report(DiagID);
						}
						return;
					}

					pStr = pEnd;
					while (*pStr == ' ' || *pStr == ',') pStr++;
					
				} else {
					unsigned DiagID = m_pDiagEngine->getCustomDiagID(DiagnosticsEngine::Error,
						"tpp error unexpected parameter name, expected trs, tsu or tco");
					m_pDiagEngine->Report(DiagID);
					return;
				}
			}

			if (trs < 0)
				trs = 0;

			if (tpd < 0)
				tpd = 0;

			int urs = URS_UNKNOWN;
			pParamDecl->setTpInfo( new HtTpInfo(!bTsu, string(), string(), -1, -1, urs, trs, tpd) );
		}
	}

	void HtDesign::addPragmaTpsTpe( HtDecl * pDecl, HtPragma * pPragma)
	{
		// parse pragma string for timing path parameters
		string tpGrpName;
		string tpSgName;
		int urs=-1;
		int tpd=-1;

		if (pDecl->getTpInfo()) {
			unsigned DiagID = m_pDiagEngine->getCustomDiagID(DiagnosticsEngine::Error,
				"tps/tpe error, timing path info for variable previously specified");
			m_pDiagEngine->Report(DiagID);
			return;
		}

		for (size_t i = 0; i < pPragma->getArgStrings().size(); i += 1) {
			string const & tpArg = pPragma->getArgStrings()[i];
			char const * pStr = tpArg.c_str();

			if (*pStr == '"') pStr++;
			while (*pStr == ' ' || *pStr == ',') pStr++;

			if (i == 0) {
				// first argument must be grp.subgrp
				char const * pStart = pStr;
				while (isalpha(*pStr) || isdigit(*pStr) || *pStr == '_' || *pStr == '-')
					pStr += 1;

				if (*pStr != '.') {
					unsigned DiagID = m_pDiagEngine->getCustomDiagID(DiagnosticsEngine::Error,
						"tps/tpe error while looking for timing path group and subgroup names");
					m_pDiagEngine->Report(DiagID);
					return;
				}

				tpGrpName = string(pStart, pStr-pStart);

				pStart = ++pStr;
				while (isalpha(*pStr) || isdigit(*pStr) || *pStr == '_' || *pStr == '-')
					pStr += 1;

				tpSgName = string(pStart, pStr-pStart);

				while (*pStr == ' ' || *pStr == ',') pStr++;
			}
		
			bool bTsu = false;
			while (*pStr != '"') {
				if (strncmp(pStr, "trs", 3) == 0) {
					pStr += 3;
					while (*pStr == ' ') pStr++;

					if (urs >= 0) {
						unsigned DiagID = m_pDiagEngine->getCustomDiagID(DiagnosticsEngine::Error,
							"tps/tpe error, register stage parameter specified multiple times");
						m_pDiagEngine->Report(DiagID);
						return;
					}

					if (*pStr++ != '=') {
						unsigned DiagID = m_pDiagEngine->getCustomDiagID(DiagnosticsEngine::Error,
							"tps/tpe error while parsing register stage parameter (trs=<integer>)");
						m_pDiagEngine->Report(DiagID);
						return;
					}

					while (*pStr == ' ') pStr++;
					char * pEnd;
					urs = strtol(pStr, &pEnd, 10);

					if (pStr == pEnd || urs < 0) {
						unsigned DiagID = m_pDiagEngine->getCustomDiagID(DiagnosticsEngine::Error,
							"tps/tpe error while parsing register stage parameter (trs=<integer>)");
						m_pDiagEngine->Report(DiagID);
						return;
					}

					pStr = pEnd;
					while (*pStr == ' ' || *pStr == ',') pStr++;
				
				} else if ((bTsu = (strncmp(pStr, "tsu", 3) == 0)) || strncmp(pStr, "tco", 3) == 0) {
					pStr += 3;
					while (*pStr == ' ') pStr++;

					if (bTsu && pPragma->isTps()) {
						unsigned DiagID = m_pDiagEngine->getCustomDiagID(DiagnosticsEngine::Error,
							"tps error while parsing timing parameters, expected (tco=<integer>)");
						m_pDiagEngine->Report(DiagID);
					} else if (!bTsu && pPragma->isTpe()) {
						unsigned DiagID = m_pDiagEngine->getCustomDiagID(DiagnosticsEngine::Error,
							"tpe error while parsing timing parameters, expected (tsu=<integer>)");
						m_pDiagEngine->Report(DiagID);
					}

					if (tpd >= 0) {
						unsigned DiagID = m_pDiagEngine->getCustomDiagID(DiagnosticsEngine::Error,
							"multiple tsu/tco parameters specified");
						m_pDiagEngine->Report(DiagID);
						return;
					}

					if (*pStr++ != '=') {
						if (bTsu) {
							unsigned DiagID = m_pDiagEngine->getCustomDiagID(DiagnosticsEngine::Error,
								"tpe error while parsing setup parameter (tsu=<integer>)");
							m_pDiagEngine->Report(DiagID);
						} else {
							unsigned DiagID = m_pDiagEngine->getCustomDiagID(DiagnosticsEngine::Error,
								"tps error while parsing clock to out parameter (tco=<integer>)");
							m_pDiagEngine->Report(DiagID);
						}
						return;
					}
				
					while (*pStr == ' ') pStr++;
					char * pEnd;
					tpd = strtol(pStr, &pEnd, 10);

					if (pStr == pEnd || tpd < 0) {
						if (bTsu) {
							unsigned DiagID = m_pDiagEngine->getCustomDiagID(DiagnosticsEngine::Error,
								"tpe error while parsing setup parameter (tsu=<integer>)");
							m_pDiagEngine->Report(DiagID);
						} else {
							unsigned DiagID = m_pDiagEngine->getCustomDiagID(DiagnosticsEngine::Error,
								"tps error while parsing clock to out parameter (tco=<integer>)");
							m_pDiagEngine->Report(DiagID);
						}
						return;
					}

					pStr = pEnd;
					while (*pStr == ' ' || *pStr == ',') pStr++;
					
				} else {
					unsigned DiagID = m_pDiagEngine->getCustomDiagID(DiagnosticsEngine::Error,
						"tps/tpe error unexpected parameter name, expected trs, tsu or tco");
					m_pDiagEngine->Report(DiagID);
					return;
				}
			}
		}

		if (urs < 0)
			urs = 0;

		if (tpd < 0)
			tpd = 0;

		int trs = 0;

		// add to tpgList
		size_t tpGrpId = addTpGrp( tpGrpName );

		HtTpg * pTpg = getTpGrpFromGrpId( tpGrpId );

		size_t tpSgId = pTpg->addTpDecl( tpSgName, pDecl);

		pDecl->setTpInfo( new HtTpInfo(pPragma->isTps(), tpGrpName, tpSgName, tpGrpId, tpSgId, urs, trs, tpd) );
	}

	size_t HtDesign::addTpGrp(string & name)
	{
		size_t tpGrpId;
		for (tpGrpId = 0; tpGrpId < m_tpgList.size(); tpGrpId += 1) {
			if (m_tpgList[tpGrpId]->getName() == name)
				return tpGrpId;
		}

		HtTpg * pTpg = new HtTpg;
		m_tpgList.push_back(pTpg);
		pTpg->setName( name );

		return tpGrpId;
	}

	bool HtDesign::isParamOutput(HtQualType qualType)
	{
		if (isParamConst(qualType))
			return false;	// const are roa

		// check if parameter decleration is an input or output
		for (;;) {
			switch (qualType.getType()->getTypeClass()) {
			case Type::LValueReference:
				return true;	// lvalues are loa
				break;
			case Type::TemplateSpecialization:
			case Type::Builtin:
			case Type::Record:
				return false;	// builtin types, struct and templates are roa
			default:
				assert(0);
			}
		}
	}

	bool HtDesign::isParamConst(HtQualType qualType)
	{
		// check if parameter decleration is an input or output
		for (;;) {
			if (qualType.isConst())
				return true;

			switch (qualType.getType()->getTypeClass()) {
			case Type::LValueReference:
				return isParamConst(qualType.getType()->getQualType());
				break;
			case Type::TemplateSpecialization:
			case Type::Builtin:
			case Type::Record:
				return false;	// builtin types, struct and templates are roa
			default:
				assert(0);
			}
		}
	}

	HtDecl * HtDesign::findBaseDecl( HtExpr * pExpr )
	{
		// find the base declaration for the reference
		HtDecl * pBaseDecl = 0;
		for (;;) {
			Stmt::StmtClass stmtClass = pExpr->getStmtClass();
			switch (stmtClass) {
			case Stmt::MemberExprClass:
				pBaseDecl = pExpr->getMemberDecl();
				assert(pBaseDecl->getKind() == Decl::Field);
				pExpr = pExpr->getBase().asExpr();
				break;
			case Stmt::ImplicitCastExprClass:
			case Stmt::CXXStaticCastExprClass:
				pExpr = pExpr->getSubExpr().asExpr();
				break;
			case Stmt::CXXThisExprClass:
				return pBaseDecl;
			default:
				assert(0);
			}
		}
		return 0;
	}

	void HtDesign::genAssignStmts(HtStmtIter & iInsert, HtExpr * pHtLhs, HtExpr * pHtRhs)
	{
		HtQualType qualType = pHtLhs->getMemberDecl()->getDesugaredQualType();

		while (qualType.getType()->getTypeClass() == Type::Typedef)
			qualType = qualType.getType()->getHtDecl()->getDesugaredQualType();

		assert(pHtLhs->getStmtClass() == Stmt::MemberExprClass);
		assert(pHtRhs->getStmtClass() == Stmt::MemberExprClass ||
			qualType.getType()->getTypeClass() == Type::Builtin);

		// insert assignment statements to perform an assign from rhs to lhs
		//   note that a structure must be copied one element at a time

		switch (qualType.getType()->getTypeClass()) {
		case Type::Builtin:
			{
				HtExpr * pHtAssign = new HtExpr( Stmt::BinaryOperatorClass, pHtRhs->getQualType(), pHtRhs->getLineInfo() );
				pHtAssign->setBinaryOperator( BO_Assign );
				pHtAssign->isAssignStmt( true );
				pHtAssign->setRhs( pHtRhs );
				pHtAssign->setLhs( pHtLhs );

				iInsert.insert(pHtAssign );
			}
			break;
		case Type::Record:
			{
				HtDecl * pRecordDecl = qualType.getType()->getHtDecl();

				//if (pRecordDecl->getQualType().getType()->getHtDecl()->getKind() == Decl::ClassTemplateSpecialization)
				//	pRecordDecl = pRecordDecl->getQualType().getType()->getHtDecl();

				for (HtCntxMap_constIter iDecl = pRecordDecl->getCntx()->begin();
					iDecl != pRecordDecl->getCntx()->end(); iDecl++)
				{
					HtDecl * pMemberDecl = iDecl->second;
					if (pMemberDecl->getKind() != Decl::Field)
						continue;

					// create MemberExpr for new field for source and dest
					HtExpr * pNewLhs = new HtExpr( Stmt::MemberExprClass, pMemberDecl->getQualType(), pHtLhs->getLineInfo() );
					pNewLhs->setMemberDecl( pMemberDecl );
					pNewLhs->setBase( pHtLhs );

					HtExpr * pNewRhs = new HtExpr( Stmt::MemberExprClass, pMemberDecl->getQualType(), pHtRhs->getLineInfo() );
					pNewRhs->setMemberDecl( pMemberDecl );
					pNewRhs->setBase( pHtRhs );

					genAssignStmts(iInsert, pNewLhs, pNewRhs);
				}
			}
			break;
		default:
			assert(0);
		}
	}

}
