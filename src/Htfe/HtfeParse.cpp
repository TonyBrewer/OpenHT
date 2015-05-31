/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
// Parse.cpp: Parse SystemC design file inserting info into CScDesign
//
//////////////////////////////////////////////////////////////////////

#include "Htfe.h"
#include "HtfeArgs.h"
#include "HtfeDesign.h"

void CHtfeDesign::Parse()
{
	InitTables();

	if (g_pHtfeArgs->IsWritePreProcessedInput())
		WritePreProcessedInput();

	// fill in command line defined names
	for (int i = 0; i < g_pHtfeArgs->GetPreDefinedNameCnt(); i += 1)
		InsertPreProcessorName(g_pHtfeArgs->GetPreDefinedName(i));

	GetNextToken();
	do {
		switch (GetToken()) {
		case tk_typedef:
			ParseTypeDef(GetTopHier());
			break;
		case tk_static:
		case tk_identifier:

			if (GetString() == "sc_fixture_top" && GetNextToken() == tk_identifier && GetString() == "SC_MODULE")
				ParseScModule(true, true);
			else if (GetString() == "sc_fixture" && GetNextToken() == tk_identifier && GetString() == "SC_MODULE")
				ParseScModule(true, false);
			else if ((GetString() == "sc_topology_top" || GetString() == "ht_topology_top") && GetNextToken() == tk_identifier && GetString() == "SC_MODULE") {
				static bool bWarnHtTop = true;
				if (bWarnHtTop && GetString() == "sc_topology_top") {
					bWarnHtTop = false;
					ParseMsg(PARSE_WARNING, "sc_topology_top is depricated, use ht_topology_top");
				}

				ParseScModule(false, true);
			} else if (GetString() == "SC_MODULE")
				ParseScModule(false, false);
			else {
				//ParseMsg(PARSE_WARNING, "%s = ParseIdentifierStatement(%s)",
				//		ParseIdentifierStatement(&m_topHier, false),
				//		GetString());
				ParseIdentifierStatement(GetTopHier(), false, true);
			}
			break;
		case tk_semicolon:
			GetNextToken(); // empty statement
			break;
		case tk_template:
			if (!g_pHtfeArgs->IsTemplateSupport())
				ParseMsg(PARSE_FATAL, "templates are not supported");

			ParseTemplateDecl(GetTopHier());
			break;
		case tk_namespace:
			ParseMsg(PARSE_FATAL, "namespaces are not supported");
			break;
		case tk_class:
		case tk_struct:
		case tk_union:
			ParseStructDecl(GetTopHier());
			break;
		case tk_enum:
			ParseEnumDecl(GetTopHier());
			break;
		default:
			ParseMsg(PARSE_ERROR, "unknown statement type");
			SkipTo(tk_semicolon);
			GetNextToken();
			break;
		}

	} while (GetToken() != tk_eof);

	if (GetErrorCnt() != 0) {
		ParseMsg(PARSE_ERROR, "unable to generate verilog due to previous errors");
		ErrorExit();
	}
}

void CHtfeDesign::ParseTypeDef(CHtfeIdent *pHier)
{
	// Insert type name in typeTbl
	GetNextToken();

	CHtfeTypeAttrib typeAttrib;
	CHtfeIdent *pType = ParseTypeDecl(pHier, typeAttrib);
	if (pType == 0)
		return;

	if (typeAttrib.m_bIsStatic)
		ParseMsg(PARSE_ERROR, "static type specifier not supported");

	if (GetToken() != tk_identifier) {
		ParseMsg(PARSE_ERROR, "expected identifier");
		SkipTo(tk_semicolon);
		GetNextToken();
		return;
	}

	CHtfeIdent *pTypeDef = pHier->InsertType(GetString());
	pTypeDef->SetType(pType);
	pTypeDef->SetIsTypeDef();
	pTypeDef->SetWidth(pType->GetWidth());

	if (GetNextToken() != tk_semicolon) {
		ParseMsg(PARSE_ERROR, "syntax error, expected ';'");
		SkipTo(tk_semicolon);
		GetNextToken();
		return;
	}
	GetNextToken();
}

CHtfeIdent * CHtfeDesign::ParseVariableRef(const CHtfeIdent *pHier, const CHtfeIdent *&pHier2, bool bRequired, bool bIgnoreIndexing, bool bLeftOfEqual)
{
	if (GetToken() != tk_identifier) {
		ParseMsg(PARSE_ERROR, "expected an identifier");
		SkipTo(tk_semicolon);
		return 0;
	}

	CHtfeIdent *pIdent = 0;
	string name = GetString();

	// look for identifier in heirarchy of tables
	for (pHier2 = pHier; pHier2; pHier2 = pHier2->GetPrevHier()) {
		pIdent = pHier2->FindIdent(name);

		if (pIdent)
			break;
	}

	while (pIdent && GetNextToken() == tk_colonColon) {

		if (pIdent->GetId() != CHtfeIdent::id_class || !pIdent->IsModule()) {
			ParseMsg(PARSE_ERROR, "'%s' : is not a class or namespace name", name.c_str());
			return 0;
		}

		if (GetNextToken() != tk_identifier) {
			ParseMsg(PARSE_ERROR, "'%s' : illegal token on right side of '::'", GetTokenString().c_str());
			return 0;
		}

		pHier2 = pIdent;
		name = GetString();
		pIdent = pIdent->FindIdent(name);
	}

	if (bIgnoreIndexing)
		return pIdent;

	if (pIdent && pIdent->GetDimenCnt() > 0) {
		// array indexing
		vector<int>	dimenList;
		string arrayName = pIdent->GetName();
		for (size_t i = 0; i < pIdent->GetDimenCnt(); i += 1) {
			if (GetToken() != tk_lbrack) {
				ParseMsg(PARSE_ERROR, "expected [ constant index ]");
				GetNextToken();
				return 0;
			}
			GetNextToken();
			CConstValue value;
			if (!ParseConstExpr(value) || GetToken() != tk_rbrack) {
				ParseMsg(PARSE_ERROR, "expected [ constant index ]");
				GetNextToken();
				return 0;
			}
			if (value.GetSint64() < 0/* || value.GetSint64() > pIdent->GetDimenSize(i)*/) {
				ParseMsg(PARSE_ERROR, "index value(%d) out of range (0-%d)\n",
					(int)value.GetSint64(), pIdent->GetDimenSize(i));
				value = CConstValue(0);
			}

			dimenList.push_back((int)value.GetSint64());

			char buf[16];
			sprintf(buf, "[%d]", (int)value.GetSint64());
			arrayName += buf;

			GetNextToken();
		}

		CHtfeIdent *pArrayIdent = pIdent->FindIdent(arrayName);
		if (!pArrayIdent) {
			if (!pIdent->IsAutoDecl()) {
				if (!m_bDeadStatement)
					ParseMsg(PARSE_ERROR, "invalid reference to arrayed identifier '%s'", arrayName.c_str());
			} else {
				UpdateVarArrayDecl(pIdent, dimenList);

				pArrayIdent = pIdent->FindIdent(arrayName);
			}
		}

		Assert(!pArrayIdent || pArrayIdent->GetFlatIdent() != 0);

		return pArrayIdent;
	}

	if (!pIdent) {
		if (bRequired)
			ParseMsg(PARSE_ERROR, "'%s' : undeclared identifier", name.c_str());

		return 0;
	}

	if (pIdent->IsModule())
		return pIdent;

	return pIdent;
}

void CHtfeDesign::ParseVariableIndexing(CHtfeIdent *pHier, const CHtfeIdent *pIdent, vector<CHtfeOperand *> &indexList)
{
	if (pIdent && !pIdent->GetPrevHier()->IsUserDefinedType() && pIdent->GetDimenCnt() > 0) {
		// array indexing
		for (size_t i = 0; i < pIdent->GetDimenCnt(); i += 1) {
			if (GetToken() != tk_lbrack) {
				ParseMsg(PARSE_ERROR, "expected [ index ]");
				GetNextToken();
				return;
			}
			GetNextToken();

			CHtfeOperand *pOperand;
			if (pIdent->GetPortDir() == port_in || pIdent->GetPortDir() == port_out) {
				CConstValue value;
				if (!ParseConstExpr(value)) {
					ParseMsg(PARSE_ERROR, "expected [ const index ]");
					return;
				}

				pOperand = HandleNewOperand();
				pOperand->InitAsConstant(GetLineInfo(), value);

			} else
				pOperand = ParseExpression(pHier);

			indexList.push_back(pOperand);

			GetNextToken();
		}
	}
}

int CHtfeDesign::ParseConstantIndexing(CHtfeIdent *pHier, const CHtfeIdent *pIdent, vector<int> &indexList)
{
	int elemIdx = 0;
	indexList.resize(pIdent->GetDimenCnt(), 0);
	if (pIdent && pIdent->GetDimenCnt() > 0) {
		// array indexing
		for (size_t i = 0; i < pIdent->GetDimenCnt(); i += 1) {
			if (GetToken() != tk_lbrack) {
				ParseMsg(PARSE_ERROR, "expected [ index ]");
				GetNextToken();
				return 0;
			}
			GetNextToken();

			CConstValue value;
			if (!ParseConstExpr(value)) {
				ParseMsg(PARSE_ERROR, "expected [ const index ]");
				return 0;
			}

			if (value.GetSint64() < 0 || value.GetSint64() >= (int64_t)pIdent->GetDimenSize(i)) {
				ParseMsg(PARSE_ERROR, "index value(%d) out of range (0-%d)\n",
					(int)value.GetSint64(), pIdent->GetDimenSize(i)-1);
			} else {
				indexList[i] = value.GetSint32();
				elemIdx *= pIdent->GetDimenSize(i);
				elemIdx += value.GetSint32();
			}

			GetNextToken();
		}
	}
	return elemIdx;
}

CHtfeStatement * CHtfeDesign::ParseCompoundStatement(CHtfeIdent *pHier, bool bNewNameSpace, bool bFoldConstants)
{
	CHtfeStatement *pStatement = 0;
	CHtfeStatement **ppStatement = &pStatement;

	if (GetToken() != tk_lbrace) {
		ParseMsg(PARSE_ERROR, "Expected a '{'");
		return pStatement;
	}

	// provide a new new namespace
	if (bNewNameSpace) {
		char buf[64];
		sprintf(buf, "Unnamed$%d", pHier->GetNextUnnamedId());
		CHtfeIdent *pNewHier = pHier->InsertIdent(buf, false);

		pNewHier->SetId(CHtfeIdent::id_nameSpace);
		pNewHier->SetIsLocal(pHier->IsLocal());
		pHier = pNewHier;
	}

	GetNextToken();
	while (GetToken() != tk_rbrace && GetToken() != tk_eof) {

		*ppStatement = ParseStatement(pHier, bFoldConstants);

		while (*ppStatement)
			ppStatement = (*ppStatement)->GetPNext();
	}

	GetNextToken();
	return pStatement;
}

CHtfeStatement * CHtfeDesign::ParseDoStatement(CHtfeIdent *pHier, bool bNewNameSpace)
{
	CHtfeStatement *pStatement = 0;
	CHtfeStatement **ppStatement = &pStatement;

	if (GetNextToken() != tk_lbrace) {
		ParseMsg(PARSE_ERROR, "Expected a '{'");
		return pStatement;
	}

	// provide a new new namespace
	if (bNewNameSpace) {
		char buf[64];
		sprintf(buf, "Unnamed$%d", pHier->GetNextUnnamedId());
		CHtfeIdent *pNewHier = pHier->InsertIdent(buf, false);

		pNewHier->SetId(CHtfeIdent::id_nameSpace);
		pNewHier->SetIsLocal(pHier->IsLocal());
		pHier = pNewHier;
	}

	GetNextToken();
	while (GetToken() != tk_rbrace && GetToken() != tk_eof) {

		*ppStatement = ParseStatement(pHier);

		while (*ppStatement)
			ppStatement = (*ppStatement)->GetPNext();
	}

	if (GetNextToken() != tk_while || GetNextToken() != tk_lparen || GetNextToken() != tk_identifier 
		|| GetNextToken() != tk_rparen || GetNextToken() != tk_semicolon) {
			ParseMsg(PARSE_ERROR, "Expected } while (false);");
			SkipTo(tk_semicolon);
	}

	static bool bErrorMsg = false;
	if (GetString() != "false" && !bErrorMsg) {
		ParseMsg(PARSE_ERROR, "do while statements with conditional expression other than 'false' are not supported");
		bErrorMsg = true;
	}
	GetNextToken();

	return pStatement;
}

CHtfeStatement * CHtfeDesign::ParseStatement(CHtfeIdent *pHier, bool bFoldConstants)
{
	CHtfeStatement *pStatement = 0;
	m_statementCount += 1;

	switch (GetToken()) {
	case tk_template:
		ParseMsg(PARSE_FATAL, "templates are not supported");
		break;
	case tk_namespace:
		ParseMsg(PARSE_FATAL, "namespaces are not supported");
		break;
	case tk_class:
	case tk_struct:
	case tk_union:
		{
			int savedCount = m_parseSwitchCount;
			m_parseSwitchCount = 0;

			ParseStructDecl(pHier);

			m_parseSwitchCount = savedCount;

			GetNextToken();
		}
		break;
	case tk_enum:
		ParseEnumDecl(pHier);
		break;
	case tk_sc_signal:
	case tk_identifier:
	case tk_const:
	case tk_static:
		if (GetString() == "sc_attrib" || GetString() == "ht_attrib") {
			static bool bWarnHtAttrib = true;
			if (bWarnHtAttrib && GetString() == "sc_attrib") {
				bWarnHtAttrib = false;
				ParseMsg(PARSE_WARNING, "sc_attrib is depricated, use ht_attrib");
			}

			ParseScAttrib(pHier);
		} else {
			CHtfeIdent *pIdent = 0;
			const CHtfeIdent *pHier2;
			for (pHier2 = pHier; pHier2 && pIdent == 0; pHier2 = pHier2->GetPrevHier())
				pIdent = pHier2->FindIdent(GetString());

			if (pIdent && pIdent->GetId() == CHtfeIdent::id_variable && pIdent->GetType()->IsHtQueue())
				pStatement = ParseHtQueueStatement(pHier);
			else if (pIdent && pIdent->GetId() == CHtfeIdent::id_variable && pIdent->GetType()->IsHtMemory())
				pStatement = ParseHtMemoryStatement(pHier);
			else
				pStatement = ParseIdentifierStatement(pHier, true, false, bFoldConstants);
		}
		break;
	case tk_plusPlus:
	case tk_minusMinus:
		pStatement = ParseIdentifierStatement(pHier, true, false);
		break;
	case tk_if:
		m_statementNestLevel += 1;
		pStatement = ParseIfStatement(pHier);
		m_statementNestLevel -= 1;
		break;
	case tk_for:
		{
			ParseForStatement();
			int playbackLevel = GetTokenPlaybackLevel();

			pStatement = ParseStatement(pHier);

			if (playbackLevel == GetTokenPlaybackLevel()) {
				CHtfeStatement **ppStatement = &pStatement;

				while (*ppStatement)
					ppStatement = (*ppStatement)->GetPNext();

				while (playbackLevel == GetTokenPlaybackLevel() && GetErrorCnt() == 0) {
					*ppStatement = ParseStatement(pHier);

					while (*ppStatement)
						ppStatement = (*ppStatement)->GetPNext();
				}
			}
		}
		break;
	case tk_do:
		pStatement = ParseDoStatement(pHier);
		break;
	case tk_while:
		ParseMsg(PARSE_FATAL, "while statements are not supported");
		break;
	case tk_try:
		ParseMsg(PARSE_FATAL, "try statements are not supported");
		break;
	case tk_catch:
		ParseMsg(PARSE_FATAL, "catch statements are not supported");
		break;
	case tk_throw:
		ParseMsg(PARSE_FATAL, "throw statements are not supported");
		break;
	case tk_continue:
		ParseMsg(PARSE_FATAL, "continue statements are not supported");
		break;
	case tk_goto:
		ParseMsg(PARSE_FATAL, "goto statements are not supported");
		break;
	case tk_switch:
		m_statementNestLevel += 1;
		pStatement = ParseSwitchStatement(pHier);
		m_statementNestLevel -= 1;
		break;
	case tk_break:
		if (m_parseSwitchCount == 0)
			ParseMsg(PARSE_ERROR, "break only allowed in switch statements");

		pStatement = HandleNewStatement();
		pStatement->Init(st_break, GetLineInfo());

		if (GetNextToken() != tk_semicolon) {
			ParseMsg(PARSE_ERROR, "expected a ';'");
			SkipTo(tk_semicolon);
		}
		GetNextToken();
		break;
	case tk_return:
		{
			GetNextToken();

			if (m_parseSwitchCount > 0)
				ParseMsg(PARSE_ERROR, "return statement within a switch statement not supported");

			pStatement = HandleNewStatement();
			pStatement->Init(st_return, GetLineInfo());

			CHtfeIdent *pFunc = pHier->FindHierFunction();
			if (pFunc->GetType() != m_pVoidType)
				pStatement->SetExpr(ParseExpression(pHier));

			if (GetToken() != tk_semicolon) {
				ParseMsg(PARSE_ERROR, "expected a ';'");
				SkipTo(tk_semicolon);
			}

			GetNextToken();

			m_parseReturnCount += 1;

			if (pHier->IsReturnRef() && m_parseReturnCount == 2)
				ParseMsg(PARSE_ERROR, "functions that return a reference can have only one return statement");
		}
		break;
	case tk_lbrace:
		m_statementCount -= 1;  // compound statements are not counted
		pStatement = ParseCompoundStatement(pHier, true, bFoldConstants);
		break;
	case tk_rbrace:
		m_statementCount -= 1;  // compound statements are not counted
		break;
	case tk_semicolon:
		m_statementCount -= 1;  // null statements are not counted
		GetNextToken();
		break;
	default:
		ParseMsg(PARSE_ERROR, "Unknown statement type");
		SkipTo(tk_semicolon);
		break;
	}
	return pStatement;
}

void CHtfeDesign::ParseCtorStatement(CHtfeIdent *pHier)
{
	CHtfeTypeAttrib typeAttrib;
	CHtfeIdent *pType = ParseTypeDecl(pHier, typeAttrib, false);
	if (pType != 0) {
		ParseMsg(PARSE_ERROR, "CTOR type declarations not allowed");
		SkipTo(tk_semicolon);
		GetNextToken();
		return;
	}

	ParseCtorExpression(pHier);

	if (GetToken() != tk_semicolon) {
		ParseMsg(PARSE_ERROR, "Expected a ';'");
		SkipTo(tk_semicolon);
	}
	GetNextToken();
}

void CHtfeDesign::ParseCtorExpression(CHtfeIdent *pHier)
{
	// Allowed statements are:
	const CHtfeIdent *pHier2;
	CHtfeIdent *pBaseIdent = ParseVariableRef(pHier, pHier2, false, true);

	if (pBaseIdent == 0) {
		ParseMsg(PARSE_ERROR, "undeclared identifier '%s'", GetString().c_str());
		SkipTo(tk_semicolon);
		return;
	}

	vector<int> indexList;
	int elemIdx = ParseConstantIndexing(pHier, pBaseIdent, indexList);

	string name = pBaseIdent->GetDimenElemName(elemIdx);

	CHtfeIdent *pIdent;
	if (pBaseIdent->GetDimenCnt() > 0)
		pIdent = pBaseIdent->FindIdent(name);
	else
		pIdent = pBaseIdent;

	if (pIdent == 0) {
		ParseMsg(PARSE_ERROR, "undeclared identifier '%s'", GetString().c_str());
		SkipTo(tk_semicolon);
		return;
	}

	if (!pIdent->IsReference() && pIdent->GetPortDir() != port_signal) {
		ParseMsg(PARSE_ERROR, "expected a signal or instance identifier");
		SkipTo(tk_semicolon);
		return;
	}

	if (pIdent->GetPortDir() == port_signal) {
		if (GetToken() != tk_equal) {
			ParseMsg(PARSE_ERROR, "Illegal CTOR statement");
			SkipTo(tk_semicolon);
			return;
		}
		GetNextToken();

		CConstValue value;
		if (!ParseConstExpr(value) || value.GetSint64() > 0x7fffffff || value.GetSint64() < 0) {
			ParseMsg(PARSE_ERROR, "CTOR constant expression must evaluate in range (0-0x7fffffff)");
			value = CConstValue(0);
		}

		pIdent->SetIsConst();
		pIdent->SetConstValue((int)value.GetSint64());
		pBaseIdent->SetWriteRef(elemIdx);

		if (GetToken() != tk_semicolon) {
			ParseMsg(PARSE_ERROR, "CTOR assignment requires a constant expression");
			SkipTo(tk_semicolon);
			return;
		}

	} else if (GetToken() == tk_equal) {
		GetNextToken();

		if (GetToken() != tk_new || GetNextToken() != tk_identifier) {
			ParseMsg(PARSE_ERROR, "Illegal CTOR statement");
			SkipTo(tk_semicolon);
			return;
		}

		const CHtfeIdent *pHier2;
		CHtfeIdent *pClass = ParseVariableRef(pHier, pHier2);

		if (pClass == 0) {
			ParseMsg(PARSE_ERROR, "undeclared class '%s'", GetString().c_str());
			SkipTo(tk_semicolon);
			return;
		}

		if (pClass->GetName() != pIdent->GetType()->GetName())
			ParseMsg(PARSE_ERROR, "type mismatch for 'new' statement, '%s' and '%s'",
			pIdent->GetType()->GetName().c_str(), pClass->GetName().c_str());

		if (GetToken() != tk_lparen || GetNextToken() != tk_string || GetNextToken() != tk_rparen) {
			ParseMsg(PARSE_ERROR, "Expected an instance name parameter");
			SkipTo(tk_semicolon);
			return;
		}

		pIdent->SetInstanceName(GetString());
		pIdent->SetIndexList(indexList);

		pBaseIdent->SetInstanceName(elemIdx, GetString());

		GetNextToken();

	} else if (GetToken() == tk_minusGreater) {

		// instance port / signal name specification
		if (GetNextToken() != tk_identifier) {
			ParseMsg(PARSE_ERROR, "expected a port name");
			SkipTo(tk_semicolon);
			return;
		}

		const CHtfeIdent *pHier2;
		CHtfeIdent *pPort = ParseVariableRef(pIdent->GetType(), pHier2, false);

		if (!pPort || !pPort->IsPort()) {
			ParseMsg(PARSE_ERROR, "'%s', identifier does not evaluate to a port name", GetString().c_str());
			SkipTo(tk_semicolon);
			return;
		}

		if (GetToken() != tk_lparen || GetNextToken() != tk_identifier/* || GetNextToken() != tk_rparen*/) {
			ParseMsg(PARSE_ERROR, "expected '( signal )'");
			SkipTo(tk_semicolon);
			return;
		}

		CHtfeIdent *pInstancePort = pIdent->InsertIdent(pPort->GetName(), false);
		if (pInstancePort->GetId() != CHtfeIdent::id_new)
			ParseMsg(PARSE_ERROR, "duplicate port, '%s'", pPort->GetName().c_str());

		pInstancePort->SetId(CHtfeIdent::id_instancePort);
		pInstancePort->SetIndexList(*pPort->GetIndexList());

		CHtfeIdent *pSignal = ParseVariableRef(pHier, pHier2, false);

		if (!pSignal) {
			// Automatically insert a signal
			CHtfeIdent * pBaseType = pPort->GetType()->GetType();

			CHtfeIdent * pSignalType;
			EPortDir portDir;
			if (GetString().substr(0,2) == "i_") {
				portDir = port_in;
				pSignalType = CreateUniquePortTemplateType("sc_in", pBaseType, false);
			} else if (GetString().substr(0,2) == "o_") {
				portDir = port_out;
				pSignalType = CreateUniquePortTemplateType("sc_out", pBaseType, false);
			} else {
				portDir = port_signal;
				pSignalType = CreateUniquePortTemplateType("sc_signal", pBaseType, false);
			}

			Assert(pHier->GetPrevHier()->GetId() == CHtfeIdent::id_class);
			pSignal = pHier->GetPrevHier()->InsertIdent(GetString());

			if (pSignal->GetId() == CHtfeIdent::id_new) {
				pSignal->SetId(CHtfeIdent::id_variable);
				pSignal->SetPortDir(portDir);
				pSignal->SetType(pSignalType);
				pSignal->SetLineInfo(GetLineInfo());
			}

			if (GetNextToken() == tk_lbrack) {
				pSignal->SetIsAutoDecl();
				pSignal->SetIsArrayIdent();
				pSignal->SetFlatIdentTbl(pSignal->GetFlatIdent()->GetFlatIdentTbl());
				pSignal->SetType(pSignalType);
				pSignal = ParseVarArrayRef(pSignal);
			}
		}

		pInstancePort->SetSignalIdent(pSignal);

		if (GetToken() != tk_rparen) {
			ParseMsg(PARSE_ERROR, "expected '( signal )'");
			SkipTo(tk_semicolon);
			return;
		}
		GetNextToken();

		pSignal->SetIsPrimOutput(pPort->GetPortDir() == port_out);

		pInstancePort->SetPortSignal(pSignal);

		CHtfeIdent *pPortType = pPort->GetType()->GetType();
		CHtfeIdent *pSignalType = pPort->GetType()->GetType();
		if (pSignalType == 0)
			pSignalType = pPort->GetType();

		if (pPortType != pSignalType)
			ParseMsg(PARSE_ERROR, "port type '%s' does not match signal type '%s'",
			pPortType->GetName().c_str(), pSignalType->GetName().c_str());

	} else {
		ParseMsg(PARSE_ERROR, "illegal CTOR statement");
		SkipTo(tk_semicolon);
		return;
	}
}

void CHtfeDesign::ParseForStatement()
{
	if (GetNextToken() != tk_lparen) {
		ParseMsg(PARSE_ERROR, "expected a '('");
		SkipTo(tk_semicolon);
		GetNextToken();
		return;
	}

	if (GetNextToken() != tk_identifier || GetString() != "int" 
		&& GetString() != "int32_t" && GetString() != "uint32_t" && GetString() != "unsigned") {
			ParseMsg(PARSE_ERROR, "expected for loop control variable int|int32_t|uint32_t|unsigned declaration");
			SkipTo(tk_rparen);
			GetNextToken();
			return;
	}

	bool bUnsignedCtrlVar = GetString() == "uint32_t" || GetString() == "unsigned";

	if (GetNextToken() != tk_identifier || GetNextToken() != tk_equal) {
		ParseMsg(PARSE_ERROR, "expected for loop control variable assignment");
		SkipTo(tk_rparen);
		GetNextToken();
		return;
	}

	string ctrlVarName = GetString();

	GetNextToken();

	CConstValue initValue;
	if (!ParseConstExpr(initValue)) {
		ParseMsg(PARSE_ERROR, "expected for loop control variable constant assignment");
		SkipTo(tk_rparen);
		GetNextToken();
		return;
	}

	if (GetToken() != tk_semicolon) {
		ParseMsg(PARSE_ERROR, "expected a ;");
		SkipTo(tk_rparen);
		GetNextToken();
		return;
	}

	// Increment for loop expression
	if (GetNextToken() != tk_identifier || GetString() != ctrlVarName || GetNextToken() != tk_less) {
		ParseMsg(PARSE_ERROR, "expected loop termination expression to be 'control variable' < 'constant value'");
		SkipTo(tk_rparen);
		GetNextToken();
		return;
	}

	GetNextToken();

	CConstValue exitValue;
	if (!ParseConstExpr(exitValue)) {
		ParseMsg(PARSE_ERROR, "expected for loop compare to a constant assignment");
		SkipTo(tk_rparen);
		GetNextToken();
		return;
	}

	if (GetToken() != tk_semicolon) {
		ParseMsg(PARSE_ERROR, "expected a ;");
		SkipTo(tk_rparen);
		GetNextToken();
		return;
	}

	// Increment for loop expression
	if (GetNextToken() != tk_identifier || GetString() != ctrlVarName || (GetNextToken() != tk_plusEqual && GetToken() != tk_plusPlus)) {
		ParseMsg(PARSE_ERROR, "expected loop increment expression to be 'control variable' += 'constant value'");
		SkipTo(tk_rparen);
		GetNextToken();
		return;
	}

	CConstValue incValue;
	if (GetToken() == tk_plusEqual) {

		GetNextToken();

		if (!ParseConstExpr(incValue)) {
			ParseMsg(PARSE_ERROR, "expected for loop increment by a constant value");
			SkipTo(tk_rparen);
			GetNextToken();
			return;
		}
	} else {
		GetNextToken();
		incValue = 1;
	}

	if (GetToken() != tk_rparen) {
		ParseMsg(PARSE_ERROR, "expected a )");
		SkipTo(tk_rparen);
		GetNextToken();
		return;
	}

	// begin recording tokens 
	RecordTokens();

	// get tokens for the next single statement (could be a compound statement)
	GetNextToken();
	SkipStatement();
	RemoveLastRecordedToken();

	if (initValue.GetSint64() < exitValue.GetSint64()) {
		// playback the token list
		ForLoopTokenPlayback(ctrlVarName, bUnsignedCtrlVar, initValue.GetSint64(), exitValue.GetSint64(), incValue.GetSint64());

	} else {
		// exit loop
		ForLoopExit();
	}
}


void CHtfeDesign::SkipStatement()
{
	switch (GetToken()) {
	case tk_if:
		SkipIfStatement();
		break;
	case tk_for:
		SkipForStatement();
		break;
	case tk_switch:
		SkipSwitchStatement();
		break;
	case tk_lbrace:
		SkipTo(tk_rbrace);
		GetNextToken();
		break;
	case tk_do:
	case tk_class:
	case tk_struct:
	case tk_union:
	case tk_enum:
	case tk_sc_signal:
	case tk_identifier:
	case tk_const:
	case tk_static:
	case tk_plusPlus:
	case tk_minusMinus:
	case tk_break:
	case tk_return:
		SkipTo(tk_semicolon);
		GetNextToken();
		break;
	case tk_semicolon:
		GetNextToken();
		break;
	case tk_template:
		ParseMsg(PARSE_FATAL, "templates are not supported");
		break;
	case tk_namespace:
		ParseMsg(PARSE_FATAL, "namespaces are not supported");
		break;
	case tk_while:
		ParseMsg(PARSE_FATAL, "while statements are not supported");
		break;
	case tk_try:
		ParseMsg(PARSE_FATAL, "try statements are not supported");
		break;
	case tk_catch:
		ParseMsg(PARSE_FATAL, "catch statements are not supported");
		break;
	case tk_throw:
		ParseMsg(PARSE_FATAL, "throw statements are not supported");
		break;
	case tk_continue:
		ParseMsg(PARSE_FATAL, "continue statements are not supported");
		break;
	case tk_goto:
		ParseMsg(PARSE_FATAL, "goto statements are not supported");
		break;
	default:
		ParseMsg(PARSE_ERROR, "Unknown statement type");
		SkipTo(tk_semicolon);
		break;
	}
}

void CHtfeDesign::SkipIfStatement()
{
	GetNextToken();
	SkipTo(tk_rparen);
	GetNextToken();
	SkipStatement();

	if (GetToken() == tk_else) {
		GetNextToken();
		SkipStatement();
	}
}

void CHtfeDesign::SkipForStatement()
{
	GetNextToken();
	SkipTo(tk_rparen);
	GetNextToken();
	SkipStatement();
}

void CHtfeDesign::SkipSwitchStatement()
{
	GetNextToken();
	SkipTo(tk_rparen);
	GetNextToken();
	SkipStatement();
}

CHtfeStatement * CHtfeDesign::ParseIdentifierStatement(CHtfeIdent *pHier, bool bAssignmentAllowed, bool bStaticAllowed, bool bFoldConstants)
{
	CHtfeStatement *pStatement = 0;
	CHtfeTypeAttrib typeAttrib;
	CHtfeIdent *pType;

#ifdef _WIN32
	if (GetLineInfo().m_lineNum == 15)
		bool stop = true;
#endif

	if (GetToken() == tk_identifier && (GetString() == "sc_dist_que" || GetString() == "ht_dist_que")) {
		static bool bWarnDistQue = true;
		if (bWarnDistQue && GetString() == "sc_dist_que") {
			bWarnDistQue = false;
			ParseMsg(PARSE_WARNING, "sc_dist_que is depricated, use ht_dist_que");
		}

		pType = ParseHtQueueDecl(pHier);
	} else if (GetToken() == tk_identifier && (GetString() == "sc_block_que" || GetString() == "ht_block_que")) {
		static bool bWarnBlockQue = true;
		if (bWarnBlockQue && GetString() == "sc_block_que") {
			bWarnBlockQue = false;
			ParseMsg(PARSE_WARNING, "sc_block_que is depricated, use ht_block_que");
		}

		pType = ParseHtQueueDecl(pHier);
	} else if (GetToken() == tk_identifier && (GetString() == "sc_dist_ram" || GetString() == "ht_dist_ram")) {
		static bool bWarnDistRam = true;
		if (bWarnDistRam && GetString() == "sc_dist_ram") {
			bWarnDistRam = false;
			ParseMsg(PARSE_WARNING, "sc_dist_ram is depricated, use ht_dist_ram");
		}

		pType = ParseHtDistRamDecl(pHier);
	} else if (GetToken() == tk_identifier && (GetString() == "sc_block_ram" || GetString() == "ht_block_ram")) {
		static bool bWarnBlockRam = true;
		if (bWarnBlockRam && GetString() == "sc_block_ram") {
			bWarnBlockRam = false;
			ParseMsg(PARSE_WARNING, "sc_block_ram is depricated, use ht_block_ram");
		}

		pType = ParseHtBlockRamDecl(pHier);
	} else if (GetToken() == tk_identifier && GetString() == "ht_mrd_block_ram") {

		pType = ParseHtAsymBlockRamDecl(pHier, true);
	} else if (GetToken() == tk_identifier && GetString() == "ht_mwr_block_ram") {

		pType = ParseHtAsymBlockRamDecl(pHier, false);
	} else if (GetToken() == tk_identifier && (GetString() == "struct" || GetString() == "class")) {
		ParseStructDecl(pHier);
		GetNextToken();
		return 0;
	} else if (GetToken() == tk_identifier &&
		(GetString() == "sc_bit" || GetString() == "sc_logic" || GetString() == "sc_bv" || GetString() == "sc_lv" ||
		GetString() == "sc_fixed" || GetString() == "sc_ufixed" || GetString() == "sc_fix" || GetString() == "sc_ufix"))
	{
		ParseMsg(PARSE_ERROR, "systemc type %s is not supported", GetString().c_str());
		SkipTo(tk_semicolon);
		GetNextToken();
		return 0;
	} else {
		pType = ParseTypeDecl(pHier, typeAttrib, false);

		if (typeAttrib.m_bIsStatic && !bStaticAllowed)
			ParseMsg(PARSE_ERROR, "static type specifier not supported");
	}

	bool bIsReference = false;

	if (pType) {
		if (GetToken() == tk_semicolon)
			return 0;

		if (GetToken() == tk_asterisk) {
			static bool bErrorMsg = false;
			if (!g_pHtfeArgs->IsScHier() && bErrorMsg == false) {
				ParseMsg(PARSE_ERROR, "variable declared as an address is not supported");
				bErrorMsg = true;
			}
			bIsReference = true;
			GetNextToken();
		}

		if (GetToken() == tk_ampersand) {
			static bool bErrorMsg = false;
			if (bErrorMsg == false) {
				ParseMsg(PARSE_ERROR, "variable declared as a reference is not supported");
				bErrorMsg = true;
			}
			GetNextToken();
		}

		// declaring a local variable
		if (GetToken() != tk_identifier) {
			ParseMsg(PARSE_ERROR, "expected an identifier");
			SkipTo(tk_semicolon);
			GetNextToken();
			return 0;
		}

		if (GetString() == "sc_main")
			ParseScMain();
		else
			pStatement = ParseVariableDecl(pHier, pType, typeAttrib, bIsReference);

		return pStatement;
	}

	if (GetString() == "ht_attrib") {
		ParseScAttrib(pHier);
		return 0;
	}

	if (!bAssignmentAllowed) {
		ParseMsg(PARSE_ERROR, "unsupported statement / declaration");
		SkipTo(tk_semicolon);
		GetNextToken();
		return 0;
	}

	pStatement = HandleNewStatement();
	pStatement->Init(st_assign, GetLineInfo());

	pStatement->SetExpr(ParseExpression(pHier, false, bFoldConstants, true));

	// check for trival statement 'var;'
	CHtfeOperand *pExpr = pStatement->GetExpr();
	if (pExpr->IsLeaf() && (pExpr->IsVariable() || pExpr->IsConstValue()))
		return 0;

	// check for var == var2;
	EToken tk = pExpr->GetOperator();
	if (!pExpr->IsLeaf() && tk != tk_equal && tk != tk_preInc && tk != tk_preDec
		&& tk != tk_postInc && tk != tk_postDec && !(tk == tk_period && pExpr->GetOperand2()->IsFunction()))
		ParseMsg(PARSE_ERROR, "Unsupported statement / declaration");

	HandleOverloadedAssignment(pHier, pStatement);

	HandleUserDefinedConversion(pHier, pStatement);

	HandleRegisterAssignment(pHier, pStatement);

	HandleSimpleIncDecStatement(pStatement);

	if (pExpr) {
		if (pExpr->GetOperand1()) {
			CHtfeIdent *pIdent = pExpr->GetOperand1()->GetMember();
			if (pIdent && pIdent->IsVariable())
				pIdent->SetIsConstVar(false);
		}

		HandleScPrimMember(pHier, pStatement);
	} else
		pStatement = 0;

	if (GetToken() != tk_semicolon) {
		ParseMsg(PARSE_ERROR, "expected a ';'");
		SkipTo(tk_semicolon);
	}
	GetNextToken();

	return pStatement;
}

void CHtfeDesign::HandleOverloadedAssignment(CHtfeIdent *pHier, CHtfeStatement *pStatement)
{
	CHtfeOperand *pExpr = pStatement->GetExpr();
	if (pExpr == 0)
		return;

	EToken tk = pExpr->GetOperator();
	if (tk != tk_equal)
		return;

	CHtfeOperand *pOp1 = pExpr->GetOperand1();

	CHtfeIdent * pOp1Type = pOp1->GetType();
	if (!pOp1Type || !pOp1Type->IsStruct())
		return;

	CHtfeIdent * pFunc = pOp1Type->FindIdent("operator=");
	if (!pFunc)
		return;

	CHtfeOperand *pOp2 = pExpr->GetOperand2();
	CHtfeIdent * pOp2Type = pOp2->GetType();

	if (pOp2->GetMember() && pOp2->GetMember()->IsPort())
		pOp2Type = pOp2Type->GetType();

	CHtfeIdent * pParamIdent = pFunc->GetParamIdent(0);
	CHtfeIdent * pParamType = pParamIdent->GetType();

	if (pParamType->IsStruct() != (pOp2Type && pOp2Type->IsStruct()))
		return;

	if (pParamType->IsStruct() && pParamType != pOp2Type)
		return;

	vector<CHtfeOperand *> argsList;
	argsList.push_back(pOp2);

	CHtfeOperand *pTempOp2 = HandleNewOperand();
	pTempOp2->InitAsIdentifier(GetLineInfo(), pFunc/*->GetFlatIdent()*/);
	pExpr->SetOperand2(pTempOp2);

	pTempOp2->SetParamList(argsList);

	pExpr->SetOperator(tk_period);
}

void CHtfeDesign::HandleUserDefinedConversion(CHtfeIdent *pHier, CHtfeStatement * pStatement)
{
	// handle methods with "operator type() { ... }
	// walk expression tree and replace leaf nodes with function call where appropriate

}

void CHtfeDesign::HandleRegisterAssignment(CHtfeIdent *pHier, CHtfeStatement * pStatement)
{
	// transform register assignements with non-trivial rhs to two statements
	CHtfeOperand *pExpr = pStatement->GetExpr();
	if (pExpr == 0)
		return;

	CHtfeOperand *pOp1 = pExpr->GetOperand1();
	CHtfeOperand *pOp2 = pExpr->GetOperand2();

	// return if lhs not a register
	if (pOp1 == 0 || pOp1->GetMember() == 0 || !pOp1->GetMember()->IsRegister())
		return;

	// return if rhs is a leaf node
	if (pOp2 == 0 || pOp2->IsLeaf())
		return;

	// create temp variable
	string tempName = pOp1->GetMember()->GetName();
	tempName[0] = 'c';
	tempName += "$Reg";

	string uniqueSuffix = "";

	CHtfeIdent *pIdent;
	for(;;) {
		pIdent = pHier->InsertIdent(tempName + uniqueSuffix);

		if (pIdent->GetId() != CHtfeIdent::id_new) {
			if (uniqueSuffix.size() == 0)
				uniqueSuffix = "$a";
			else
				uniqueSuffix[1] += 1;
		} else
			break;
	}
	pIdent->SetId(CHtfeIdent::id_variable);
	pIdent->SetType(pOp1->GetMember()->GetType());
	pIdent->SetLineInfo(GetLineInfo());

	// put temp into original statement
	CHtfeOperand *pTempReg1 = HandleNewOperand();
	pTempReg1->InitAsIdentifier(GetLineInfo(), pIdent->GetFlatIdent());

	pExpr->SetOperand1(pTempReg1);

	// split assignment into two statements
	CHtfeStatement * pStatement2 = HandleNewStatement();
	pStatement2->Init(st_assign, GetLineInfo());

	pStatement->SetNext(pStatement2);

	CHtfeOperand *pTempReg2 = HandleNewOperand();
	pTempReg2->InitAsIdentifier(GetLineInfo(), pIdent->GetFlatIdent());

	CHtfeOperand *pAssign = HandleNewOperand();
	pAssign->InitAsOperator(GetLineInfo(), tk_equal, pOp1, pTempReg2);
	pStatement2->SetExpr(pAssign);
}

void CHtfeDesign::HandleSimpleIncDecStatement(CHtfeStatement *pStatement)
{
	CHtfeOperand * pExpr = pStatement->GetExpr();
	EToken tk = pExpr->GetOperator();

	if (tk != tk_preInc && tk != tk_postInc && tk != tk_preDec && tk != tk_postDec)
		return;

	// Simple var++ / ++var / var-- / --var statement
	//   Transform to var = var + 1, or var = var - 1
	CHtfeOperand *pOne = HandleNewOperand();
	pOne->InitAsConstant(pExpr->GetLineInfo(), CConstValue((uint32_t) 1));

	CHtfeOperand *pVar = pExpr->GetOperand1();

	CHtfeOperand *pAddSub = pExpr;
	pAddSub->SetOperator(tk == tk_preInc || tk == tk_postInc ? tk_plus : tk_minus);
	pAddSub->SetOperand2(pOne);
	pAddSub->SetType(pVar->GetType());

	CHtfeOperand *pTmpVar = HandleNewOperand();
	pTmpVar->InitAsIdentifier(pStatement->GetLineInfo(), pVar->GetMember());
	pTmpVar->SetType(pVar->GetType());
	pTmpVar->SetIndexList(pVar->GetIndexList());
	*pTmpVar->GetSubFieldP() = pVar->GetSubField();

	CHtfeOperand *pEqual = HandleNewOperand();
	pEqual->InitAsOperator(pExpr->GetLineInfo(), tk_equal, pTmpVar, pAddSub);
	pEqual->SetType(pVar->GetType());

	// link replicated operands so common index equations can be handled
	pVar->SetLinkedOp(pTmpVar);
	pTmpVar->SetLinkedOp(pVar);

	pStatement->SetExpr(pEqual);
}

void CHtfeDesign::HandleIntegerConstantPropagation(CHtfeOperand * pExpr)
{
	// propagate integer constant assignments to left side of equal variable
	if (pExpr->IsLeaf() || pExpr->GetOperator() != tk_equal)
		return;

	CHtfeOperand *pOp1 = pExpr->GetOperand1();
	CHtfeOperand *pOp2 = pExpr->GetOperand2();

	if (!pOp1 || !pOp2 || !pOp1->GetMember())
		return;

	CConstValue value;
	bool bOp2Const = EvalConstantExpr(pOp2, value);

	CHtfeIdent * pIdent = pOp1->GetMember();

	if (bOp2Const && pOp1->IsVariable() && pOp1->GetDimenCnt() == 0
		&& pIdent->GetType()->GetWidth() >= value.GetMinWidth() && value.IsPos()
		&& pOp1->GetSubField() == 0 && pIdent->GetDeclStatementLevel() == m_statementNestLevel)
	{
		pIdent->SetIsConstVar(true);
		pIdent->SetConstValue(value);
	} else
		// once a non-constant value is assigned a non-constant, it can not be used as a constant
		pIdent->SetIsConstVar(false);
}

void CHtfeDesign::HandleScPrimMember(CHtfeIdent *pHier, CHtfeStatement *pStatement)
{
	if (pStatement->GetStType() != st_assign)
		return;

	CHtfeOperand *pExpr = pStatement->GetExpr();
	CHtfeIdent *pFunc;

	if (pExpr == 0)
		return;
	else if (pExpr->IsLeaf())
		pFunc = pExpr->GetMember();
	else
		return;

	if (pFunc == 0 || pFunc->GetId() != CHtfeIdent::id_function || !pFunc->IsScPrim())
		return;

	if (pHier->GetPrevHier() == GetTopHier())
		return;

	//if (!pHier->FindHierMethod()) {
	if (!pHier->FindHierModule()) {
		ParseMsg(PARSE_ERROR, "ht_prims can only be used within method");
		return;
	}

	pStatement->SetIsScPrim();

	vector<CHtfeOperand *> &paramList = pExpr->GetParamList();
	for (unsigned i = 0; i < paramList.size(); i += 1) {
		if (!paramList[i])
			continue;

		if (!paramList[i]->IsLeaf()) {
			ParseMsg(PARSE_ERROR, "argument can not be an expression for an ht_prim");
			return;
		}

		if (paramList[i]->IsConstValue())
			continue;

		CHtfeOperand * pArgOp = paramList[i];
		CHtfeIdent *pArg = pArgOp->GetMember();

		// check for local variables
		if (pArg->IsScLocal()) {
			ParseMsg(PARSE_ERROR, "arguments for sc_prims can not be declared with sc_local, %s\n",
				pArg->GetName().c_str());
			ParseMsg(PARSE_ERROR, pArg->GetLineInfo(), "    sc_local declaration");
		}

		// insert copies of input variables
		if (pFunc->GetParamIsConst(i)) {
			pArg->SetIsScPrimInput();
		} else {
			pArg->SetIsHtPrimOutput(pArgOp);

			if (pFunc->GetParamIsScState(i))
				pArg->SetIsScState();
		}
	}
}

CHtfeStatement * CHtfeDesign::ParseIfStatement(CHtfeIdent *pHier)
{
	CHtfeStatement *pIf = HandleNewStatement();
	pIf->Init(st_if, GetLineInfo());

	if (GetNextToken() != tk_lparen) {
		ParseMsg(PARSE_ERROR, "Expected a '('");
		SkipTo(tk_semicolon);
		GetNextToken();
		return 0;
	}
	GetNextToken();

	m_pIfExprOperand = ParseExpression(pHier);

	if (m_pIfExprOperand)
		FoldConstantExpr(m_pIfExprOperand);

	if (GetToken() != tk_rparen) {
		ParseMsg(PARSE_ERROR, "Expected a ')'");
		SkipTo(tk_semicolon);
		GetNextToken();
		return 0;
	}
	GetNextToken();

	m_statementCount = 0;
	m_bMemWriteStatement = false;

	bool bDeadIfClause = m_pIfExprOperand && m_pIfExprOperand->IsConstValue() && m_pIfExprOperand->GetConstValue().GetUint64() == 0;
	bool bDeadElseClause = m_pIfExprOperand && m_pIfExprOperand->IsConstValue() && m_pIfExprOperand->GetConstValue().GetUint64() != 0;

	pIf->SetExpr(m_pIfExprOperand);

	bool bPrevDeadSt = m_bDeadStatement;
	m_bDeadStatement = bPrevDeadSt || bDeadIfClause;	// prevent errors on dead statements

	CHtfeStatement *pIfClause = ParseStatement(pHier);
	pIf->SetCompound1(pIfClause);

	if (m_bMemWriteStatement) {
		if (m_statementNestLevel != 1 || m_statementCount != 1 || GetToken() == tk_else)
			ParseMsg(PARSE_ERROR, "unknown memory write syntax, unable to generate verilog");
	}

	CHtfeStatement *pElseClause = 0;
	if (GetToken() == tk_else) {
		GetNextToken();
		m_bDeadStatement = bPrevDeadSt || bDeadElseClause;

		pElseClause = ParseStatement(pHier);
		pIf->SetCompound2(pElseClause);
	}

	m_bDeadStatement = bPrevDeadSt;

	if (!bDeadIfClause && !bDeadElseClause)
		return pIf;
	else if (!bDeadIfClause)
		return pIfClause;
	else
		return pElseClause;
}

CHtfeStatement * CHtfeDesign::ParseSwitchStatement(CHtfeIdent *pHier)
{
	CHtfeStatement *pSwitch = HandleNewStatement();
	pSwitch->Init(st_switch, GetLineInfo());

	CHtfeOperand *pSwitchOperand = 0;

	if (GetNextToken() != tk_lparen) {
		ParseMsg(PARSE_ERROR, "Expected a '('");
		SkipTo(tk_lbrace);
	} else {
		GetNextToken();

		pSwitchOperand = ParseExpression(pHier);
		SetExpressionWidth(pSwitchOperand);

		pSwitch->SetExpr(pSwitchOperand);

		if (GetToken() != tk_rparen) {
			ParseMsg(PARSE_ERROR, "Expected a ')'");
			SkipTo(tk_lbrace);
		} else
			GetNextToken();
	}

	if (GetToken() != tk_lbrace) {
		ParseMsg(PARSE_ERROR, "Expected a '{'");
		SkipTo(tk_semicolon);
		GetNextToken();
		return pSwitch;
	}

	GetNextToken();

	if (GetToken() != tk_case && GetToken() != tk_default) {
		ParseMsg(PARSE_ERROR, "Expected a case or default label");
		SkipTo(tk_rbrace);
		GetNextToken();
		return pSwitch;
	}

	m_parseSwitchCount += 1;

	CHtfeStatement **ppSwitchTail = pSwitch->GetPCompound1();
	CHtfeStatement **ppCaseTail = 0;
	bool bDefaultSeen = false;

	// Handle case where switch expression is a constant
	// This is required to avoid problems with out of range indexies
	//   on dead statements (unavoidable when unrolling for loops)
	bool bConstSwitch = pSwitchOperand && pSwitchOperand->IsConstValue();
	CHtfeStatement * pCaseStatement = 0;
	CHtfeStatement * pDefaultStatement = 0;

	do {
		while (GetToken() == tk_case || GetToken() == tk_default) {
			CHtfeStatement *pCase;
			CHtfeOperand *pCaseOperand = 0;
			bool bDefault = GetToken() == tk_default;
			if (bDefault) {
				if (bDefaultSeen) {
					ParseMsg(PARSE_ERROR, "multiple switch default entries not allowed");
					SkipTo(tk_semicolon);
					break;
				}
				bDefaultSeen = true;

				pCase = HandleNewStatement();
				pCase->Init(st_default, GetLineInfo());

				GetNextToken();
			} else {
				// case
				GetNextToken();

				pCaseOperand = ParseExpression(pHier);

				if (!pCaseOperand->IsConstValue())
					ParseMsg(PARSE_ERROR, "expected case value to be constant expression");

				pCase = HandleNewStatement();
				pCase->Init(st_case, GetLineInfo());
				pCase->SetExpr(pCaseOperand);
			}

			*ppSwitchTail = pCase;
			ppSwitchTail = pCase->GetPNext();
			ppCaseTail = pCase->GetPCompound1();

			if (GetToken() != tk_colon) {
				ParseMsg(PARSE_ERROR, "Expected a ':'");
				SkipTo(tk_semicolon);
			}
			GetNextToken();

			if (GetToken() == tk_case || GetToken() == tk_default)
				continue;

			bool bBreakSeen = false;
			bool bBreakWarned = false;

			CHtfeStatement *pStatement = 0;
			while (GetToken() != tk_case && GetToken() != tk_default && GetToken() != tk_rbrace && GetToken() != tk_eof) {
				pStatement = ParseStatement(pHier);
				if (pStatement != 0) {
					*ppCaseTail = pStatement;
					while (pStatement) {
						if (bBreakSeen) {
							if (!bBreakWarned) {
								ParseMsg(PARSE_WARNING, "statements after break are ignored");
								bBreakWarned = true;
								*ppCaseTail = 0;
							}
							break;
						}

						if (pStatement->GetStType() == st_break)
							bBreakSeen = true;

						ppCaseTail = pStatement->GetPNext();
						pStatement = pStatement->GetNext();
					}
				}
			}

			if (GetToken() == tk_case || GetToken() == tk_default) {
				pStatement = pCase->GetCompound1();
				while (pStatement && pStatement->GetNext())
					pStatement = pStatement->GetNext();
				if (!pStatement || pStatement->GetStType() != st_break)
					ParseMsg(PARSE_ERROR, "expected break at end of case/default statement list");
			} else if (GetToken() != tk_rbrace)
				ParseMsg(PARSE_ERROR, "expected case, default, or }");

			TransformCaseStatementListToRemoveBreaks(pCase->GetCompound1());

			if (bDefault)
				pDefaultStatement = pCase->GetCompound1();

			else if (bConstSwitch && pCaseOperand->IsConstValue() && pCaseOperand->GetConstValue().GetSint64() == pSwitchOperand->GetConstValue().GetSint64())
				pCaseStatement = pCase->GetCompound1();
		}
	} while (GetToken() != tk_rbrace && GetToken() != tk_eof);

	GetNextToken();
	m_parseSwitchCount -= 1;

	if (bConstSwitch) {
		CHtfeStatement * pConstStmt = pCaseStatement ? pCaseStatement : pDefaultStatement;

		// last statement is a st_break, must remove it from list of statements
		CHtfeStatement ** ppStmt = &pConstStmt;
		while (*ppStmt && (*ppStmt)->GetStType() != st_break)
			ppStmt = (*ppStmt)->GetPNext();

		if ((ppStmt == 0 || *ppStmt == 0) && GetErrorCnt() > 0)
			ParseMsg(PARSE_FATAL, "Previous errors prohibit further parsing");

		*ppStmt = 0;
		return pConstStmt;
	} else
		return pSwitch;
}

void CHtfeDesign::TransformCaseStatementListToRemoveBreaks(CHtfeStatement *pCaseHead)
{
	// verilog does not allow multiple breaks within a case statement
	//   transform if statements with embedded breaks to eliminate the extra breaks
	// The algorithm merges post if-else statements into the if-else, removing any
	//   internal breaks. The algorithm starts at the end of the statement list and
	//   works to the beginning using recussion.

	CHtfeStatement *pStIf = pCaseHead;
	while (pStIf && (pStIf->GetStType() != st_if || !(IsEmbeddedBreakStatement(pStIf->GetCompound1()) ||
		IsEmbeddedBreakStatement(pStIf->GetCompound2()))))
		pStIf = pStIf->GetNext();

	if (pStIf) {
		// found an if statement, see if there are more if's in the list
		CHtfeStatement *pStIf2 = pStIf->GetNext();
		while (pStIf2 && (pStIf2->GetStType() != st_if || !(IsEmbeddedBreakStatement(pStIf2->GetCompound1()) ||
			IsEmbeddedBreakStatement(pStIf2->GetCompound2()))))
			pStIf2 = pStIf2->GetNext();

		if (pStIf2)
			TransformCaseStatementListToRemoveBreaks(pStIf2);

		if (IsEmbeddedBreakStatement(pStIf)) {
			// found an embedded break statement, find the last break

			CHtfeStatement *pStatement = pStIf;
			while (pStatement && pStatement->GetNext() && pStatement->GetNext()->GetStType() != st_break)
				pStatement = pStatement->GetNext();

			CHtfeStatement *pBreak = pStatement->GetNext();
			pStatement->SetNext(0);

			CHtfeStatement *pPostList = pStIf->GetNext();
			pStIf->SetNext(pBreak);

			TransformCaseIfStatementList(pStIf->GetPCompound1(), pPostList);
			TransformCaseIfStatementList(pStIf->GetPCompound2(), pPostList); 
		}
	}
}

void CHtfeDesign::TransformCaseIfStatementList(CHtfeStatement **ppList, CHtfeStatement *pPostList)
{
	// Append postList on to pIfList provided pIfList doesn't end in a break
	CHtfeStatement **ppTail = ppList;
	while (*ppTail && (*ppTail)->GetNext())
		ppTail = (*ppTail)->GetPNext();

	if (*ppTail == 0)
		*ppTail = pPostList;
	else if ((*ppTail)->GetStType() != st_break)
		(*ppTail)->SetNext(pPostList);
	else
		*ppTail = 0;

	TransformCaseStatementListToRemoveBreaks(*ppList);
}

bool CHtfeDesign::IsEmbeddedBreakStatement(CHtfeStatement *pStatement)
{
	while (pStatement) {
		switch (pStatement->GetStType()) {
		case st_assign:
		case st_switch:
			// no possibility of an embedded break in these statements
			break;
		case st_break:
			return true;
		case st_if:
			if (IsEmbeddedBreakStatement(pStatement->GetCompound1()) ||
				IsEmbeddedBreakStatement(pStatement->GetCompound2()))
				return true;
			break;
		default:
			Assert(0);
		}
		pStatement = pStatement->GetNext();
	}
	return false;
}

void CHtfeDesign::ParseScAttrib(CHtfeIdent *pHier)
{
	if (GetNextToken() != tk_lparen || GetNextToken() != tk_identifier) {
		ParseMsg(PARSE_ERROR, "Expected attribute name");
		SkipTo(tk_semicolon);
		GetNextToken();
		return;
	}

	string attribName = GetString();

	if (GetNextToken() != tk_comma || GetNextToken() != tk_identifier && GetToken() != tk_string) {
		ParseMsg(PARSE_ERROR, "Expected attribute instance name");
		SkipTo(tk_semicolon);
		GetNextToken();
		return;
	}

	string instName = GetString();

	const CHtfeIdent *pHier2 = pHier;
	CHtfeIdent * pAttribIdent = ParseVariableRef(pHier, pHier2, true, true);

	if (pAttribIdent == 0 || !pAttribIdent->IsVariable() && !pAttribIdent->IsCtor() && !pAttribIdent->IsModule()) {
		ParseMsg(PARSE_ERROR, "expect attribute instance to be either a variable or SC_MODULE name");
		SkipTo(tk_semicolon);
		GetNextToken();
		return;
	}

	if (pAttribIdent->IsCtor())
		pAttribIdent = pAttribIdent->GetPrevHier();

	// check for array indexing
	vector<int>	dimenList;
	for (size_t i = 0; i < pAttribIdent->GetDimenCnt(); i += 1) {
		if (GetToken() != tk_lbrack) {
			ParseMsg(PARSE_ERROR, "expected [ constant index ]");
			SkipTo(tk_semicolon);
			GetNextToken();
			return;
		}
		GetNextToken();
		CConstValue value;
		if (!ParseConstExpr(value) || GetToken() != tk_rbrack) {
			ParseMsg(PARSE_ERROR, "expected [ constant index ]");
			SkipTo(tk_semicolon);
			GetNextToken();
			return;
		}
		if (value.GetSint64() < 0 || value.GetSint64() > (int64_t)pAttribIdent->GetDimenSize(i)) {
			ParseMsg(PARSE_ERROR, "index value(%d) out of range (0-%d)\n",
				(int)value.GetSint64(), pAttribIdent->GetDimenSize(i));
			value = CConstValue(0);
		}

		char buf[16];
		sprintf(buf, "$%lld", (long long)value.GetSint64());

		instName += buf;

		GetNextToken();
	}

	// check for bit range
	int msb = -1;
	int lsb = -1;

	if (GetToken() == tk_lbrack) {
		CConstValue value;

		GetNextToken();
		if (!ParseConstExpr(value)) {
			ParseMsg(PARSE_ERROR, "expected bit range constant");
			SkipTo(tk_semicolon);
			GetNextToken();
			return;
		}

		lsb = msb = value.GetSint32();
		if (GetToken() == tk_colon) {
			GetNextToken();
			if (!ParseConstExpr(value)) {
				ParseMsg(PARSE_ERROR, "expected bit range constant");
				SkipTo(tk_semicolon);
				GetNextToken();
				return;
			}
		}
		lsb = value.GetSint32();

		if (GetToken() != tk_rbrack) {
			ParseMsg(PARSE_ERROR, "expected closing brack for bit range");
			SkipTo(tk_semicolon);
			GetNextToken();
			return;
		}
		GetNextToken();

		if (msb > pAttribIdent->GetWidth()-1 || lsb > msb || lsb) {
			ParseMsg(PARSE_ERROR, "invalid bit range, variable width is %d\n", pAttribIdent->GetWidth());
			SkipTo(tk_semicolon);
			GetNextToken();
			return;
		}
	}

	if (GetToken() != tk_comma || GetNextToken() != tk_string && GetToken() != tk_num_string) {
		ParseMsg(PARSE_ERROR, "Expected attribute value string");
		SkipTo(tk_semicolon);
		GetNextToken();
		return;
	}

	string value = GetString();

	if (value.c_str()[0] == '"') {
		char buf[128];
		strcpy(buf, value.c_str());
		buf[strlen(buf)-1] = '\0';
		value = buf+1;
	}

	if (GetNextToken() != tk_rparen || GetNextToken() != tk_semicolon) {
		ParseMsg(PARSE_ERROR, "Expected ')'");
		SkipTo(tk_semicolon);
	}

	pAttribIdent->AddHtAttrib(attribName, instName, msb, lsb, value, GetLineInfo());

	GetNextToken();
}

void CHtfeDesign::ParseScModule(bool bScFixture, bool bScTopologyTop)
{
	// parse a SystemC class
	if (GetNextToken() != tk_lparen || GetNextToken() != tk_identifier
		|| GetNextToken() != tk_rparen) {
			ParseMsg(PARSE_ERROR, "Expected SC_MODULE name");
			if (GetNextToken() == tk_lbrace) {
				SkipTo(tk_rbrace);
				GetNextToken();
			}
			return;
	}

	CHtfeIdent *pClass = m_pTopHier->InsertType(GetString());
	if (pClass->GetId() != CHtfeIdent::id_new) {
		ParseMsg(PARSE_ERROR, "Identifier redeclared");
		SkipTo(tk_rbrace);
		GetNextToken();
		return;
	}
	pClass->SetId(CHtfeIdent::id_class);
	pClass->SetIsModule();
	pClass->SetPrevHier(m_pTopHier);
	pClass->NewFlatTable();
	pClass->SetIsScTopology(IsScTopology());
	pClass->SetIsScFixture(bScFixture);
	pClass->SetIsScFixtureTop(bScFixture && bScTopologyTop);
	pClass->SetIsScTopologyTop(bScTopologyTop);

	if (IsInputFile()) {
		pClass->SetIsInputFile();

		HandleInputFileInit();
	}

	if (GetNextToken() != tk_lbrace)
		ParseMsg(PARSE_ERROR, "Expected '{'");

	GetNextToken();

	EToken tk;

	for (;;) {
		switch (tk = GetToken()) {
		case tk_sc_in:
			ParsePortDecl(pClass, port_in);
			break;
		case tk_sc_out:
			ParsePortDecl(pClass, port_out);
			break;
		case tk_sc_inout:
			ParsePortDecl(pClass, port_inout);
			break;
		case tk_sc_signal:
			ParsePortDecl(pClass, port_signal);
			break;
		case tk_enum:
			ParseEnumDecl(pClass);
			break;
		case tk_SC_CTOR:
			ParseModuleCtor(pClass);
			break;
		case tk_identifier:
			ParseIdentifierStatement(pClass, false, false);
			break;
		case tk_tilda:
			ParseDestructor(pClass);
			break;
		case tk_template:
			ParseMsg(PARSE_FATAL, "templates are not supported");
			break;
		case tk_namespace:
			ParseMsg(PARSE_FATAL, "namespaces are not supported");
			break;
		case tk_class:
		case tk_struct:
		case tk_union:
			ParseStructDecl(pClass);
			GetNextToken();
			break;
		case tk_typedef:
			ParseTypeDef(pClass);
			break;
		case tk_rbrace:
			if (GetNextToken() != tk_semicolon) {
				ParseMsg(PARSE_ERROR, "Expected a ';'");
				SkipTo(tk_semicolon);
			}
			GetNextToken();
			HandleGenerateScModule(pClass);
			return;
		case tk_eof:
			ParseMsg(PARSE_ERROR, "Unexpected end-of-file");
			return;
		default:
			ParseMsg(PARSE_ERROR, "unknown statement type");
			SkipTo(tk_semicolon);
			GetNextToken();
			break;
		}
	}
}

void CHtfeDesign::ParsePortDecl(CHtfeIdent *pHier, EPortDir portDir)
{
	if (GetNextToken() != tk_less) {
		ParseMsg(PARSE_ERROR, "Expected '<'");
		SkipTo(tk_semicolon);
		return;
	}

	GetNextToken();

	CHtfeTypeAttrib typeAttrib;
	CHtfeIdent *pBaseType = ParseTypeDecl(pHier, typeAttrib);
	if (!pBaseType) {
		SkipTo(tk_semicolon);
		return;
	}

	if (typeAttrib.m_bIsStatic)
		ParseMsg(PARSE_ERROR, "static type specifier not supported");

	if (GetToken() != tk_greater) {
		ParseMsg(PARSE_ERROR, "Expected '>'");
		SkipTo(tk_semicolon);
		return;
	}

	// create unique name for this width
	string portName;
	switch (portDir) {
	case port_in: portName = "sc_in"; break;
	case port_out: portName = "sc_out"; break;
	case port_inout: portName = "sc_inout"; break;
	case port_signal: portName = "sc_signal"; break;
	default: Assert(0);
	}

	CHtfeIdent * pType = CreateUniquePortTemplateType(portName, pBaseType, typeAttrib.m_bIsConst);

	if (GetNextToken() == tk_identifier && (GetString() == "sc_noload" || GetString() == "ht_noload")) {
		static bool bWarnHtNoLoad = true;
		if (bWarnHtNoLoad && GetString() == "sc_noload") {
			bWarnHtNoLoad = false;
			ParseMsg(PARSE_WARNING, "sc_noload is depricated, use ht_noload");
		}

		typeAttrib.m_bIsHtNoLoad = true;
		GetNextToken();
	}

	if (GetToken() != tk_identifier) {
		ParseMsg(PARSE_ERROR, "Expected a port or signal name");
		SkipTo(tk_semicolon);
		return;
	}

	CHtfeIdent *pPort = pHier->InsertIdent(GetString());

	if (pPort->GetId() != CHtfeIdent::id_new) {
		ParseMsg(PARSE_ERROR, "identifier redeclared '%s'\n", GetString().c_str());
		return;
	}

	pPort->SetId(CHtfeIdent::id_variable);
	pPort->SetPortDir(portDir);
	pPort->SetType(pType);
	pPort->SetLineInfo(GetLineInfo());
	pPort->SetIsNoLoad(typeAttrib.m_bIsHtNoLoad);

	if (GetNextToken() == tk_lbrack) {
		//pPort->NewFlatTable();
		//pPort->SetFlatIdentTbl(pPort->GetFlatIdent()->GetFlatIdentTbl());
		ParseVarArrayDecl(pPort);
	}

	if (pHier->IsModule() && IsRegisterName(pPort->GetName()))
		pPort->SetIsRegister();

	if (GetToken() != tk_semicolon) {
		ParseMsg(PARSE_ERROR, "Expected a semicolon");
		SkipTo(tk_semicolon);
	}

	GetNextToken(); // parse semicolon
}

void CHtfeDesign::ParseVarArrayDecl(CHtfeIdent *pIdent)
{
	bool bArrayTbl = pIdent->GetPortDir() == port_in || pIdent->GetPortDir() == port_out ||
		pIdent->GetPortDir() == port_signal || pIdent->GetType() && pIdent->GetType()->IsModule();

	if (bArrayTbl) {
		pIdent->SetIsArrayIdent();
		pIdent->NewFlatTable();
		//pIdent->SetFlatIdentTbl(pIdent->GetFlatIdent()->GetFlatIdentTbl());
	}

	vector<int> emptyList;
	vector<int> dimenList;
	while (GetToken() == tk_lbrack) {
		GetNextToken();

		CConstValue dimenSize = 0;
		if (dimenList.size() > 0 || GetToken() != tk_rbrack) {
			if (!ParseConstExpr(dimenSize)) {
				ParseMsg(PARSE_ERROR, "expected a constant dimension");
				SkipTo(tk_semicolon);
				return;
			}

			if (dimenSize.GetSint64() == 0) {
				ParseMsg(PARSE_ERROR, "dimension size must be greater than or equal to one");
				dimenSize.SetSint32() = 1;
			}
		}

		emptyList.push_back(0);
		dimenList.push_back((int)dimenSize.GetSint64());

		if (GetToken() != tk_rbrack) {
			ParseMsg(PARSE_ERROR, "expected a ]");
			SkipTo(tk_semicolon);
			return;
		}

		GetNextToken();
	}

	if (bArrayTbl) {
		pIdent->SetDimenList(emptyList);
		UpdateVarArrayDecl(pIdent, dimenList);
	} else
		pIdent->SetDimenList(dimenList);
}

void CHtfeDesign::ParseMemberArrayDecl(CHtfeIdent *pIdent, int structBitPos)
{
	pIdent->SetIsArrayIdent();

	vector<int> dimenList;
	while (GetToken() == tk_lbrack) {
		GetNextToken();

		CConstValue dimenSize;
		if (!ParseConstExpr(dimenSize)) {
			ParseMsg(PARSE_ERROR, "expected a constant dimension");
			SkipTo(tk_semicolon);
			return;
		}

		dimenList.push_back((int)dimenSize.GetSint64());

		if (GetToken() != tk_rbrack) {
			ParseMsg(PARSE_ERROR, "expected a ]");
			SkipTo(tk_semicolon);
			return;
		}

		GetNextToken();
	}
	pIdent->SetDimenList(dimenList);

	pIdent->InsertArrayElements(structBitPos);
}

CHtfeIdent * CHtfeDesign::ParseVarArrayRef(CHtfeIdent *pIdent)
{
	vector<int> emptyList;
	vector<int> dimenList;
	while (GetToken() == tk_lbrack) {
		GetNextToken();

		CConstValue dimenSize;
		if (!ParseConstExpr(dimenSize)) {
			ParseMsg(PARSE_ERROR, "expected a constant dimension");
			SkipTo(tk_semicolon);
			return 0;
		}

		emptyList.push_back(0);
		dimenList.push_back((int)dimenSize.GetSint64());

		if (GetToken() != tk_rbrack) {
			ParseMsg(PARSE_ERROR, "expected a ]");
			SkipTo(tk_semicolon);
			return 0;
		}

		GetNextToken();
	}
	if (pIdent->GetDimenCnt() == 0)
		pIdent->SetDimenList(emptyList);

	string heirVarName = pIdent->GetName();

	char idxStr[16];
	for (size_t i = 0; i < dimenList.size(); i += 1) {
		sprintf(idxStr, "%d", dimenList[i]);
		//_itoa(dimenList[i],idxStr,10);
		heirVarName.append("[").append(idxStr).append("]");
	}

	if (pIdent->IsAutoDecl())
		UpdateVarArrayDecl(pIdent, dimenList);

	CHtfeIdent * pArrayIdent = pIdent->FindIdent(heirVarName);

	Assert(pArrayIdent->GetFlatIdent());
	return pArrayIdent;
}

void CHtfeDesign::UpdateVarArrayDecl(CHtfeIdent *pIdent, vector<int> &dimenList)
{
	Assert(dimenList.size() == pIdent->GetDimenCnt());

	// IsAutoDecl - signal is automatically declared for .sc topology files
	//   in this case, we are actually parsing a reference. The index values
	//   need increment by one.

	vector<int>	iterList;
	size_t i;
	for (i = 0; i < dimenList.size(); i += 1) {
		iterList.push_back(0);

		if (pIdent->IsAutoDecl())
			dimenList[i] += 1;
	}

	do {
		for (i = 0; i < dimenList.size(); i += 1) {
			if (iterList[i] >= pIdent->GetDimen(i))
				break;
		}
		if (i < dimenList.size()) {
			string systemCName = pIdent->GetName();
			string verilogName = pIdent->GetName();

			char idxStr[16];
			int linearIdx = 0;
			vector<int> indexList;
			indexList.resize(dimenList.size());

			for (i = 0; i < dimenList.size(); i += 1) {
				indexList[i] = iterList[i];
				linearIdx = linearIdx * dimenList[i] + iterList[i];

				//_itoa(iterList[i],idxStr,10);
				sprintf(idxStr, "%d", iterList[i]);
				systemCName.append("[").append(idxStr).append("]");
				verilogName.append("$").append(idxStr);
			}

			CHtfeIdent *pSignal = pIdent->InsertIdent(systemCName, verilogName);

			pSignal->SetPrevHier(pIdent);
			pSignal->SetIndexList(indexList);
			pSignal->SetLinearIdx(linearIdx);

			// set flags appropriate for a port/signal
			pSignal->SetId(CHtfeIdent::id_variable);
			pSignal->SetPortDir(pIdent->GetPortDir());
			pSignal->SetType(pIdent->GetType());
			pSignal->SetLineInfo(pIdent->GetLineInfo());
		}
		for (i = 0; i < dimenList.size(); i += 1) {
			iterList[i] += 1;
			if (iterList[i] == dimenList[i])
				iterList[i] = 0;
			else
				break;
		}
	} while (i < dimenList.size());

	pIdent->SetDimenList(dimenList);
}

CHtfeIdent * CHtfeDesign::CreateUniquePortTemplateType(string portName, CHtfeIdent * pBaseType, bool bIsConst)
{
	string typeName = portName + "<" + pBaseType->GetName() + "> ";
	CHtfeIdent *pType = m_pTopHier->InsertType(typeName);
	pType->SetType(pBaseType);

	if (pType->GetId() == CHtfeIdent::id_new) {
		pType->SetId(CHtfeIdent::id_class);
		pType->SetUserConvType(pBaseType);
		pType->SetWidth(pBaseType->GetWidth());

		// initialize builtin functions
		CHtfeIdent *pFunc = pType->InsertIdent("pos", false);
		pFunc->SetId(CHtfeIdent::id_function);
		pFunc->SetType(pBaseType);

		pFunc = pType->InsertIdent("neg", false);
		pFunc->SetId(CHtfeIdent::id_function);
		pFunc->SetType(pBaseType);

		pFunc = pType->InsertIdent(m_readString, false);
		pFunc->SetId(CHtfeIdent::id_function);
		pFunc->SetType(pBaseType);

		pFunc = pType->InsertIdent(m_writeString, false);
		pFunc->SetId(CHtfeIdent::id_function);
		pFunc->SetType(pType);
		CHtfeParam tmp = CHtfeParam(pBaseType, bIsConst);
		pFunc->AppendParam(tmp);
	}
	return pType;
}

// Parse a template definition
//   record list of tokens for playback when template is instanciated

void CHtfeDesign::ParseTemplateDecl(CHtfeIdent * pHier)
{
	if (GetNextToken() != tk_less) {
		ParseMsg(PARSE_ERROR, "expected '<' after template keyword");
		return;
	}

	while (GetNextToken() != tk_greater) {

	}
}

// Parse a struct or class declaration
//   All methods and public/private are ignored (i.e. only members info is recorded)
//   startBitPos - starting bit position for struct/class/union
//     named structures start numbering from zero, unnamed start numbering from startBitPos
//     it is not know if it is named or unnamed until the end of the structure, so
//     always start from zero and go back and add startBitPos if needed.

int CHtfeDesign::ParseStructDecl(CHtfeIdent * pHier, int startBitPos)
{
	bool bIsNamedStruct;
	string structName;
	int structBitPos = 0;

	bool bIsUnion = GetToken() == tk_union;

	if (bIsNamedStruct = (GetNextToken() == tk_identifier)) {
		structName = GetString();
		GetNextToken();
	} else {
		char buf[16];
		sprintf(buf, "Unnamed$%d", pHier->GetNextUnnamedId());
		structName = buf;
	}

	CHtfeIdent * pStruct = pHier->InsertIdent(structName, false);

	if (pStruct->GetId() == CHtfeIdent::id_new) {
		pStruct->SetId(CHtfeIdent::id_struct);
		pStruct->SetIsStruct();
		pStruct->SetIsUserDefinedType();

	} else if (pStruct->GetId() != CHtfeIdent::id_struct) {
		ParseMsg(PARSE_ERROR, "identifier previously declared");
		return 0;		
	}

	if (GetToken() == tk_colon) {
		// base class, ignore
		static bool bErrorMsg = false;
		if (bErrorMsg == false) {
			ParseMsg(PARSE_ERROR, "derived classes (a.k.a. inheritance) is not supported");
			bErrorMsg = true;
		}

		if (GetNextToken() == tk_public || GetToken() == tk_private) {
			if (GetNextToken() != tk_colon)
				ParseMsg(PARSE_ERROR, "expected a :");
			else
				GetNextToken();
		}

		if (GetToken() != tk_identifier)
			ParseMsg(PARSE_ERROR, "expected base class name");

		GetNextToken();
	}

	if (GetToken() == tk_semicolon) {
		// struct declared, but not defined
		return 0;
	} 

	if (GetToken() != tk_lbrace) {
		ParseMsg(PARSE_ERROR, "expected {");
		return 0;
	}

	if (pStruct->GetIdentTbl().GetCount() > 0) {
		ParseMsg(PARSE_ERROR, "struct previously defined");
		return 0;
	}

	size_t recordStart = RecordTokens();

	GetNextToken();
	int maxStructBitPos = 0;	// unions return maximum bit width of all declared variables
	while (GetToken() != tk_rbrace && GetToken() != tk_eof) {
		// parse body of struct/class

		CHtfeIdent *pType;
		switch (GetToken()) {
		case tk_friend:
			RecordTokensErase(recordStart);
			recordStart = RecordTokensResume();
			GetNextToken();
			continue;
		case tk_tilda:
		{
			static bool bErrorMsg = false;
			if (bErrorMsg == false) {
				ParseMsg(PARSE_FATAL, "class destructors are not supported in this release");
				bErrorMsg = true;
			}

			GetNextToken();
			break;
		}
		case tk_operator:
		{
			GetNextToken();
			pType = ParseTypeDecl(pHier);

			if (GetToken() == tk_lparen) {
				RecordStructMethod(pStruct);
				break;
			}

			ParseMsg(PARSE_ERROR, "expected () after type");
			SkipTo(tk_rbrace);
			GetNextToken();

			RecordTokensErase(recordStart);
			break;
		}
		case tk_identifier:
			{
				pType = ParseTypeDecl(pHier);
				if (!pType || pType->GetId() != CHtfeIdent::id_struct && pType->GetId() != CHtfeIdent::id_class &&
					pType->GetId() != CHtfeIdent::id_intType)
				{
					ParseMsg(PARSE_ERROR, "expected a type name");
					SkipTo(tk_rbrace);
					GetNextToken();

					RecordTokensErase(recordStart);
					break;
				}

				if (GetToken() == tk_lparen) {
					static bool bErrorMsg = false;
					if (bErrorMsg == false) {
						ParseMsg(PARSE_FATAL, "class constructors are not supported in this release");
						bErrorMsg = true;
					}

					RecordStructMethod(pStruct);	// skip constructor/destructor
					break;
				}

				if (GetToken() == tk_asterisk) {
					static bool bErrorMsg = false;
					if (bErrorMsg == false) {
						ParseMsg(PARSE_ERROR, "member variable declared as an address is not supported in this release");
						bErrorMsg = true;
					}
					GetNextToken();
				}

				bool bIsReference = false;
				if (GetToken() == tk_ampersand) {
					bIsReference = true;
					GetNextToken();
				}

				if (GetToken() == tk_operator)
					SkipTo(tk_lparen);
				else if (GetToken() != tk_identifier && GetToken() != tk_operator) {
					ParseMsg(PARSE_ERROR, "expected a method or member name");
					SkipTo(tk_rbrace);
					GetNextToken();

					RecordTokensErase(recordStart);
					break;
				} else
					GetNextToken();

				if (GetToken() == tk_lparen) {
					RecordStructMethod(pStruct);
					break;
				}

				if (bIsReference) {
					static bool bErrorMsg = false;
					if (bErrorMsg == false) {
						ParseMsg(PARSE_ERROR, "member variable declared as a reference is not supported in this release");
						bErrorMsg = true;
					}
				}


				// wasn't a function declaration, erase tokens from list
				RecordTokensErase(recordStart);

				// Declare variable(s)
				for (;;) {
					CHtfeIdent * pMember = pStruct->InsertIdent(GetString(), false);

					if (pMember->GetId() != CHtfeIdent::id_new) {
						ParseMsg(PARSE_ERROR, "identifier previously declared");
						SkipTo(tk_semicolon);
						break;
					}

					// ParseMemberArrayDecl needs type to determine width of member
					pMember->SetType(pType);

					if (GetToken() == tk_lbrack)
						ParseMemberArrayDecl(pMember, structBitPos);

					pMember->SetId(CHtfeIdent::id_variable);
					pMember->SetLineInfo(GetLineInfo());

					int fieldWidth = pType->GetWidth();

					if (GetToken() == tk_colon) {
						// bit field
						GetNextToken();
						CConstValue value;
						if (!ParseConstExpr(value)) {
							//if (GetNextToken() != tk_num_int) {
							ParseMsg(PARSE_ERROR, "expected a bit field width");
							SkipTo(tk_semicolon);
							break;
						}
						fieldWidth = value.GetSint32();
						if (fieldWidth > (1 << 10)) {
							ParseMsg(PARSE_ERROR, "unsupported field width (1-1024)");
							fieldWidth = 1;
						}
						//GetNextToken();
					}

					pMember->SetStructPos(structBitPos);

					int elemCnt;
					if (pMember->GetDimenCnt() == 0) {
						elemCnt = 1;
						pMember->SetWidth(fieldWidth);
					} else
						elemCnt = pMember->GetIdentTbl().GetCount();

					structBitPos += fieldWidth * elemCnt;

					if (GetToken() == tk_comma) {
						GetNextToken();

						if (GetToken() != tk_identifier) {
							ParseMsg(PARSE_ERROR, "expected a member name");
							SkipTo(tk_rbrace);
							GetNextToken();
							break;
						}

						GetNextToken();
						continue;
					}
					break;
				}

				if (GetToken() != tk_semicolon)
					ParseMsg(PARSE_ERROR, "expected ;");
			}
			break;
		case tk_public:
		case tk_private:
			RecordTokensErase(recordStart);
			if (GetNextToken() != tk_colon)
				ParseMsg(PARSE_ERROR, "expected :");
			break;
		case tk_template:
			ParseMsg(PARSE_FATAL, "templates are not supported");
			break;
		case tk_namespace:
			ParseMsg(PARSE_FATAL, "namespaces are not supported");
			break;
		case tk_static:
			ParseMsg(PARSE_FATAL, "static members are not supported");
			break;
		case tk_union:
			RecordTokensErase(recordStart);
			structBitPos += ParseStructDecl(pStruct, structBitPos);
			break;
		case tk_struct:
		case tk_class:
			RecordTokensErase(recordStart);
			structBitPos += ParseStructDecl(pStruct, structBitPos);
			break;
		default:
			RecordTokensErase(recordStart);
			ParseMsg(PARSE_ERROR, "unsupported struct feature");
			SkipTo(tk_semicolon);
			break;
		}

		recordStart = RecordTokensResume();
		GetNextToken();

		if (maxStructBitPos < structBitPos)
			maxStructBitPos = structBitPos;

		if (bIsUnion)
			structBitPos = 0;
	}

	RecordTokensErase(recordStart);

	if (GetToken() != tk_rbrace) {
		ParseMsg(PARSE_ERROR, "expected }");
		return 0;
	}

	FuncDeclPlayback();

	while (IsTokenPlayback())
		ParseStructMethod(pStruct);

	pStruct->SetWidth(maxStructBitPos);
	pStruct->SetOpWidth(maxStructBitPos);

	if (GetToken() == tk_semicolon) {
		// No variables were declared

		if (bIsNamedStruct)
			// struct/class/union declaration only
			return 0;
		else {
			// Unnamed struct/class/union bit fields are variables, add in to pHier at startBitPos
			CHtfeIdentTblIter fieldIter(pStruct->GetIdentTbl());
			for (fieldIter.Begin(); !fieldIter.End() && !fieldIter->IsPort(); fieldIter++) {
				if (fieldIter->GetId() != CHtfeIdent::id_variable)
					continue;

				CHtfeIdent *pMember = pHier->InsertIdent(fieldIter->GetName(), false);
				pMember->SetId(CHtfeIdent::id_variable)->SetWidth(fieldIter->GetWidth())->SetType(fieldIter->GetType());
				pMember->SetStructPos(startBitPos + fieldIter->GetStructPos());

				if (fieldIter->IsArrayIdent()) {
					pMember->SetIsArrayIdent();
					pMember->SetDimenList(fieldIter->GetDimenList());
					pMember->InsertArrayElements(pMember->GetStructPos());
				}
			}

			return maxStructBitPos;
		}
	}

	int totalBitWidth = 0;
	while (GetToken() == tk_identifier) {
		// declaration of variables of type struct/class/union
		CHtfeIdent *pIdent = pHier->InsertIdent(GetString());

		if (pIdent->GetId() != CHtfeIdent::id_new) {
			ParseMsg(PARSE_ERROR, "redefinition of variable name");
			SkipTo(tk_semicolon);
			return 0;
		}

		bool bIsArrayDecl = false;
		if (GetNextToken() == tk_lbrack) {
			bIsArrayDecl = true;
			ParseVarArrayDecl(pIdent);
		}

		pIdent->SetId(CHtfeIdent::id_variable);
		pIdent->SetType(pStruct);
		pIdent->SetLineInfo(GetLineInfo());

		// set struct starting position and width
		if (!bIsArrayDecl) {
			// not an array
			pIdent->SetStructPos(startBitPos);
			pIdent->SetWidth(maxStructBitPos);
			totalBitWidth += maxStructBitPos;
		} else {
			// an array
			CHtfeIdentTblIter elemIter(pIdent->GetIdentTbl());
			for (elemIter.Begin(); !elemIter.End(); elemIter++) {
				elemIter->SetWidth(maxStructBitPos);
				elemIter->SetStructPos(startBitPos + elemIter->GetLinearIdx() * maxStructBitPos);

				totalBitWidth += maxStructBitPos;
			}
		}

		if (GetToken() == tk_comma)
			GetNextToken();
		else
			break;
	}

	if (GetToken() != tk_semicolon) {

		ParseMsg(PARSE_ERROR, "expected a ;");
		SkipTo(tk_semicolon);
	}

	return totalBitWidth;
}

// Record a structure method for parsing after variables are declared
void CHtfeDesign::RecordStructMethod(CHtfeIdent *pStruct)
{
	// Should enters with lparen token
	if (GetToken() != tk_lparen) {
		ParseMsg(PARSE_ERROR, "expected (");
		return;
	}

	// skip over argument declarations
	SkipTo(tk_rparen);

	if (GetNextToken() == tk_const)
		GetNextToken();

	if (GetToken() == tk_semicolon) {
		// method was declared, but not defined
		return;
	}

	if (GetToken() == tk_colon)
		// skip over member initialization
		SkipTo(tk_lbrace);

	if (GetToken() != tk_lbrace) {
		ParseMsg(PARSE_ERROR, "expected {");
		return;
	}

	if (GetNextToken() != tk_rbrace)
		SkipTo(tk_rbrace);

	RecordTokensPause();
}

void CHtfeDesign::ParseStructMethod(CHtfeIdent *pHier)
{
	CHtfeIdent * pType;
	string funcName;
	bool bIsReturnRef = false;
	bool bIsUserConversion = false;
	bool bIsOverloadedOperator = false;
	CHtfeLex::EToken overloadedOperator = tk_equal;

	if (GetToken() == tk_operator) {
		GetNextToken();

		pType = ParseTypeDecl(pHier);
		if (!pType) {
			ParseMsg(PARSE_ERROR, "expected a type name");
			SkipTo(tk_rbrace);
			GetNextToken();
			return;
		}

		if (GetToken() == tk_ampersand) {
			bIsReturnRef = true;
			GetNextToken();
		}

		funcName = "operator$" + pType->GetName();

		bIsUserConversion = true;

	} else {

		pType = ParseTypeDecl(pHier);
		if (!pType || pType->GetId() != CHtfeIdent::id_struct && pType->GetId() != CHtfeIdent::id_class && pType->GetId() != CHtfeIdent::id_intType) {
			ParseMsg(PARSE_ERROR, "expected a type name");
			SkipTo(tk_rbrace);
			GetNextToken();
			return;
		}

		if (GetToken() == tk_ampersand) {
			bIsReturnRef = true;
			GetNextToken();
		}

		if (GetToken() == tk_operator) {
			EToken tk = GetNextToken();
			if (tk != tk_equal && tk != tk_plusEqual && tk != tk_minusEqual && tk != tk_asteriskEqual
				&& tk != tk_slashEqual && tk != tk_percentEqual && tk != tk_ampersandEqual 
				&& tk != tk_vbarEqual && tk != tk_lessLessEqual && tk != tk_greaterGreaterEqual
				&& tk != tk_carotEqual) 
			{
				ParseMsg(PARSE_ERROR, "unsupported overloaded operator");
				SkipTo(tk_rbrace);
				GetNextToken();
				return;
			}

			funcName = "operator" + CHtfeLex::GetTokenString(tk);

			bIsOverloadedOperator = true;
			overloadedOperator = GetToken();

		} else if (GetToken() != tk_identifier) {

			ParseMsg(PARSE_ERROR, "expected function name");
			SkipTo(tk_rbrace);
			GetNextToken();
			return;

		} else
			funcName = GetString();

		GetNextToken();
	}

	CHtfeIdent * pMember = pHier->InsertIdent(funcName, true, true);
	pMember->SetId(CHtfeIdent::id_function);
	pMember->SetType(pType);
	pMember->SetIsReturnRef(bIsReturnRef);
	pMember->SetLineInfo(GetLineInfo());
	pMember->AllocRefInfo();

	if (bIsOverloadedOperator) {
		pMember->SetIsOverloadedOperator(true);
		pMember->SetOverloadedOperator(overloadedOperator);
	} else if (bIsUserConversion)
		pMember->SetIsUserConversion(true);

	// Should enters with lparen token
	if (GetToken() != tk_lparen) {
		ParseMsg(PARSE_ERROR, "expected (");
		GetNextToken();
		return;
	}

	ParseFunctionDecl(pHier, pHier, pMember);
}

// Parse type without the returned flags
CHtfeIdent * CHtfeDesign::ParseTypeDecl(CHtfeIdent *pHier, bool bRequired)
{
	CHtfeTypeAttrib typeAttrib;
	CHtfeIdent * pType = ParseTypeDecl(pHier, typeAttrib, bRequired);

	if (typeAttrib.m_bIsStatic)
		ParseMsg(PARSE_ERROR, "static type specifier not supported");

	return pType;
}

// Parse type searching heirarchy of name spaces
CHtfeIdent * CHtfeDesign::ParseTypeDecl(CHtfeIdent * pHier, CHtfeTypeAttrib & typeAttrib, bool bRequired)
{
	CHtfeIdent *pType = 0;
	unsigned int typeMask = 0;
	const int UNSIGNED = 2;
	const int INT = 4;
	const int SHORT = 8;
	const int CHAR = 0x10;
	const int FLOAT = 0x20;
	const int DOUBLE = 0x40;
	const int LONG = 0x80;
	const int LONGLONG = 0x100;
	const int INT8 = 0x200;
	const int UINT8 = 0x400;
	const int INT16 = 0x800;
	const int UINT16 = 0x1000;
	const int INT32 = 0x2000;
	const int UINT32 = 0x4000;
	const int INT64 = 0x8000;
	const int UINT64 = 0x10000;

	typeAttrib.m_bIsConst = false;
	typeAttrib.m_bIsHtPrim = false;
	typeAttrib.m_bIsHtNoLoad = false;
	typeAttrib.m_bIsHtState = false;
	typeAttrib.m_bIsHtLocal = false;

	static bool bWarnHtPrim = true;
	static bool bWarnHtNoLoad = true;
	static bool bWarnHtState = true;
	static bool bWarnHtLocal = true;

	while (GetToken() == tk_identifier || GetToken() == tk_const || GetToken() == tk_static) {
		// first check built in integer types
		if (GetToken() == tk_const) {
			typeAttrib.m_bIsConst = true;
			GetNextToken();
			continue;
		}

		if (GetToken() == tk_static) {
			typeAttrib.m_bIsStatic = true;
			GetNextToken();
			continue;
		}

		if (GetString() == "unsigned") {
			typeMask |= UNSIGNED;
			GetNextToken();
			continue;
		}

		if (GetString() == "char") {
			typeMask |= CHAR;
			GetNextToken();
			continue;
		}

		if (GetString() == "short") {
			typeMask |= SHORT;
			GetNextToken();
			continue;
		}

		if (GetString() == "int") {
			typeMask |= INT;
			GetNextToken();
			continue;
		}

		if (GetString() == "long") {
			if (typeMask & LONG) {
				typeMask &= ~LONG;
				typeMask |= LONGLONG;
			} else
				typeMask |= LONG;
			GetNextToken();
			continue;
		}

		if (GetString() == "float") {
			static bool bErrorMsg = false;
			if (bErrorMsg == false) {
				ParseMsg(PARSE_ERROR, "floating point variables are not supported");
				bErrorMsg = true;
			}
			typeMask |= FLOAT;
			GetNextToken();
			continue;
		}

		if (GetString() == "double") {
			static bool bErrorMsg = false;
			if (bErrorMsg == false) {
				ParseMsg(PARSE_ERROR, "floating point variables are not supported");
				bErrorMsg = true;
			}
			typeMask |= DOUBLE;
			GetNextToken();
			continue;
		}

		if (GetString() == "sc_prim" || GetString() == "ht_prim") {
			if (bWarnHtPrim && GetString() == "sc_prim") {
				bWarnHtPrim = false;
				ParseMsg(PARSE_WARNING, "sc_prim is depricated, use ht_prim");
			}

			typeAttrib.m_bIsHtPrim = true;
			GetNextToken();
			continue;
		}

		if (GetString() == "int8_t") {
			typeMask |= INT8;
			GetNextToken();
			continue;
		}

		if (GetString() == "uint8_t") {
			typeMask |= UINT8;
			GetNextToken();
			continue;
		}

		if (GetString() == "int16_t") {
			typeMask |= INT16;
			GetNextToken();
			continue;
		}

		if (GetString() == "uint16_t") {
			typeMask |= UINT16;
			GetNextToken();
			continue;
		}

		if (GetString() == "int32_t") {
			typeMask |= INT32;
			GetNextToken();
			continue;
		}

		if (GetString() == "uint32_t") {
			typeMask |= UINT32;
			GetNextToken();
			continue;
		}

		if (GetString() == "int64_t") {
			typeMask |= INT64;
			GetNextToken();
			continue;
		}

		if (GetString() == "uint64_t") {
			typeMask |= UINT64;
			GetNextToken();
			continue;
		}

		if (GetString() == "sc_state" || GetString() == "ht_state") {
			if (bWarnHtState && GetString() == "sc_state") {
				bWarnHtState = false;
				ParseMsg(PARSE_WARNING, "sc_state is depricated, use ht_state");
			}

			typeAttrib.m_bIsHtState = true;
			if (GetNextToken() == tk_struct || GetToken() == tk_class) {
				GetNextToken();		
			}
			continue;
		}

		if (GetString() == "sc_local" || GetString() == "ht_local") {
			if (bWarnHtLocal && GetString() == "sc_local") {
				bWarnHtLocal = false;
				ParseMsg(PARSE_WARNING, "sc_local is depricated, use ht_local");
			}

			typeAttrib.m_bIsHtLocal = true;
			GetNextToken();
			continue;
		}

		if (ParseHtPrimClkList(typeAttrib)) {
			GetNextToken();
			continue;
		}

		if (GetString() == "inline") {
			GetNextToken();
			continue;
		}

		if (GetString() == "volatile") {
			GetNextToken();
			continue;
		}

		if (GetString() == "sc_noload" || GetString() == "ht_noload") {
			if (bWarnHtNoLoad && GetString() == "sc_noload") {
				bWarnHtNoLoad = false;
				ParseMsg(PARSE_WARNING, "sc_noload is depricated, use ht_noload");
			}

			typeAttrib.m_bIsHtNoLoad = true;
			GetNextToken();
			continue;
		}

		if (typeMask != 0)
			break;

		for (const CHtfeIdent *pHier2 = pHier; pHier2 && !pType; pHier2 = pHier2->GetPrevHier())
			pType = pHier2->FindIdent(GetString());

		if (pType && pType->IsTypeDef())
			pType = pType->GetType();

		if (pType && pType->GetId() != CHtfeIdent::id_struct && pType->GetId() != CHtfeIdent::id_class && 
			pType->GetId() != CHtfeIdent::id_enumType && pType->GetId() != CHtfeIdent::id_intType)
			pType = 0;

		break;
	}

	if (pType == 0 && (typeAttrib.m_bIsConst || typeMask != 0)) {
		if (typeAttrib.m_bIsConst && typeMask == 0)
			return m_pIntType;

		switch (typeMask) {
		case UNSIGNED:
			return m_pUIntType;
		case CHAR:
			return m_pCharType;
		case CHAR | UNSIGNED:
			return m_pUCharType;
		case SHORT:
			return m_pShortType;
		case SHORT | UNSIGNED:
			return m_pUShortType;
		case INT:
			return m_pIntType;
		case INT | UNSIGNED:
			return m_pUIntType;
		case LONG:
		case LONGLONG:
			return m_pLongType;
		case LONG | UNSIGNED:
		case LONGLONG | UNSIGNED:
			return m_pULongType;
		case INT8:
			return m_pInt8Type;
		case UINT8:
			return m_pUInt8Type;
		case INT16:
			return m_pInt16Type;
		case UINT16:
			return m_pUInt16Type;
		case INT32:
			return m_pInt32Type;
		case UINT32:
			return m_pUInt32Type;
		case INT64:
			return m_pInt64Type;
		case UINT64:
			return m_pUInt64Type;
		case FLOAT:
			return m_pFloatType;
		case DOUBLE:
			return m_pDoubleType;
		default:
			ParseMsg(PARSE_ERROR, "illegal type");
			return 0;
		}
	}

	// look for identifier in heirarchy of type tables
	string name = GetString();
	while (pType && GetNextToken() == tk_colonColon && pType->GetId() == CHtfeIdent::id_class) {

		if (pType->GetId() != CHtfeIdent::id_class) {
			ParseMsg(PARSE_ERROR, "'%s' : is not a class or namespace name", name.c_str());
			return 0;
		}

		if (GetNextToken() != tk_identifier) {
			ParseMsg(PARSE_ERROR, "'%s' : illegal token on right side of '::'", GetTokenString().c_str());
			return 0;
		}

		name = GetString();
		pType = pType->FindIdent(name);
	}

	if (typeAttrib.m_bIsHtState) {
		if (!pType) {
			// declare new type
			if (GetToken() != tk_identifier) {
				ParseMsg(PARSE_ERROR, "Expected a ht_state type name");
				return 0;
			}

			pType = m_pTopHier->InsertType(GetString());
			if (pType->GetId() == CHtfeIdent::id_new) {
				pType->SetId(CHtfeIdent::id_class);
				pType->SetIsScState();
			}

			if (GetNextToken() == tk_lbrace) {
				SkipTo(tk_rbrace);
				GetNextToken();
			}
		} else {
			// previously declared
			if (GetToken() == tk_lbrace) {
				SkipTo(tk_rbrace);
				GetNextToken();
			}
		}
	}

	if (!pType || (pType->GetId() != CHtfeIdent::id_struct &&
		pType->GetId() != CHtfeIdent::id_class &&
		pType->GetId() != CHtfeIdent::id_enumType &&
		pType->GetId() != CHtfeIdent::id_intType))
	{
		if (bRequired) {
			ParseMsg(PARSE_ERROR, "Expected a type name");
			GetNextToken();
		}
		return 0;
	}

	if (pType == m_pScIntType || pType == m_pScUintType || pType == m_pScBigUintType || pType == m_pScBigIntType) {

		if (GetToken() != tk_less) {
			ParseMsg(PARSE_ERROR, "Expected a < (%s<W>)", pType->GetName().c_str());
			return 0;
		}

		GetNextToken();
		CConstValue width;
		if (!ParseConstExpr(width)) {
			ParseMsg(PARSE_ERROR, "expected a constant for W (%s<W>)", pType->GetName().c_str());
			SkipTo(tk_semicolon);
			return 0;
		}

		if (GetToken() != tk_greater) {
			ParseMsg(PARSE_ERROR, "expected a > (%s<W>)", pType->GetName().c_str());
			SkipTo(tk_semicolon);
			return 0;
		}

		CHtfeIdent * pBaseType = pType;

		// create unique name for this width
		if (pBaseType == m_pScIntType || pBaseType == m_pScUintType) {
			pType = CreateUniqueScIntType(pType, (int)width.GetSint64());

			if (width.GetSint64() > 64)
				ParseMsg(PARSE_ERROR, "type width exceeds maximum value (64), %s", pType->GetName().c_str());
			if (width.GetSint64() == 0)
				ParseMsg(PARSE_ERROR, "type width of zero not valid, %s", pType->GetName().c_str());

		} else if (pBaseType == m_pScBigIntType || pBaseType == m_pScBigUintType) {

			pType = CreateUniqueScBigIntType(pType, (int)width.GetSint64());

			pType->SetWidth( (int)width.GetSint64() );
			pType->SetOpWidth( (int)width.GetSint64() );
		}
#if 0
		// If this check is enabled, local state is limited to 512 bits
		// total.
		else {
			if (width.GetSint64() > 512)
				ParseMsg(PARSE_ERROR, "type width exceeds maximum value (512), %s", pType->GetName().c_str()); 
		}
#endif

		GetNextToken();
	}

	while (GetToken() == tk_identifier || GetToken() == tk_const || GetToken() == tk_static) {

		if (GetToken() == tk_const) {
			typeAttrib.m_bIsConst = true;
			GetNextToken();
			continue;
		}

		if (GetToken() == tk_static) {
			typeAttrib.m_bIsStatic = true;
			GetNextToken();
			continue;
		}

		if (GetString() == "sc_prim" || GetString() == "ht_prim") {
			if (bWarnHtPrim && GetString() == "sc_prim") {
				bWarnHtPrim = false;
				ParseMsg(PARSE_WARNING, "sc_prim is depricated, use ht_prim");
			}

			typeAttrib.m_bIsHtPrim = true;
			GetNextToken();
			continue;
		}

		if (GetString() == "inline") {
			GetNextToken();
			continue;
		}

		if (GetString() == "sc_noload" || GetString() == "ht_noload") {
			if (bWarnHtNoLoad && GetString() == "sc_noload") {
				bWarnHtNoLoad = false;
				ParseMsg(PARSE_WARNING, "sc_noload is depricated, use ht_noload");
			}

			typeAttrib.m_bIsHtNoLoad = true;
			GetNextToken();
			continue;
		}

		if (GetString() == "sc_state" || GetString() == "ht_state") {
			if (bWarnHtState && GetString() == "sc_state") {
				bWarnHtState = false;
				ParseMsg(PARSE_WARNING, "sc_state is depricated, use ht_state");
			}

			typeAttrib.m_bIsHtState = true;
			GetNextToken();
			continue;
		}

		if (GetString() == "sc_local" || GetString() == "ht_local") {
			if (bWarnHtLocal && GetString() == "sc_local") {
				bWarnHtLocal = false;
				ParseMsg(PARSE_WARNING, "sc_local is depricated, use ht_local");
			}

			typeAttrib.m_bIsHtLocal = true;
			GetNextToken();
			continue;
		}

		if (ParseHtPrimClkList(typeAttrib)) {
			GetNextToken();
			continue;
		}

		break;
	}

	return pType;
}

bool CHtfeDesign::ParseHtPrimClkList(CHtfeTypeAttrib & typeAttrib)
{
	static bool bWarnHtClock = true;

	if (GetString() == "sc_clock" || GetString() == "sc_clk" || GetString() == "ht_clock" || GetString() == "ht_clk") {
		if (bWarnHtClock && (GetString() == "sc_clock" || GetString() == "sc_clk")) {
			bWarnHtClock = false;
			ParseMsg(PARSE_WARNING, "sc_clock is depricated, use ht_clock");
		}

		if (GetNextToken() != tk_lparen) {
			ParseMsg(PARSE_ERROR, "Expected a '('");
			return 0;
		}
		
		bool bNonHtlClk = false;

		do {
			if (GetNextToken() != tk_string) {
				ParseMsg(PARSE_ERROR, "Expect clock port name");
				return 0;
			}

			bool bHtlClk = GetString() == "intfClk1x" || GetString() == "intfClk2x" || GetString() == "intfClk4x" ||
				GetString() == "clk1x" || GetString() == "clk2x" || GetString() == "clk4x";

			if (bNonHtlClk || !bHtlClk && typeAttrib.m_htPrimClkList.size() > 0) {
				ParseMsg(PARSE_ERROR, "multiple clocks within the set of intfClk1x, intfClk2x, intfClk4x, clk1x, clk2x, clk4x");
				ParseMsg(PARSE_INFO, "or a single clock not in the set can be specified");

				SkipTo(tk_rparen);
				break;
			}

			typeAttrib.m_htPrimClkList.push_back(GetString());

		} while (GetNextToken() == tk_comma);

		if (GetToken() != tk_rparen) {
			ParseMsg(PARSE_ERROR, "Expected a ')'");
			return 0;
		}

		return true;

	} else
		return false;
}

CHtfeIdent * CHtfeDesign::CreateUniqueScIntType(CHtfeIdent *pBaseType, int width)
{
	if (width == 0)
		ParseMsg(PARSE_FATAL, "Invalid type width (0)");

	char widthName[16];
	sprintf(widthName, "%d", width);
	string typeName = pBaseType->GetName() + "<" + widthName + "> ";
	CHtfeIdent * pType = m_pTopHier->InsertType(typeName);

	if (pType->GetId() == CHtfeIdent::id_new) {
		pType->SetId(CHtfeIdent::id_class);
		pType->SetWidth(width);
		pType->SetOpWidth(pBaseType->GetOpWidth());
		pType->SetIsParenOverloaded();
		pType->SetIsSigned(pBaseType->IsSigned());
		pType->SetUserConvType(pBaseType->GetUserConvType());

		// initialize builtin functions
		CHtfeIdent *pFunc = CHtfeDesign::NewIdent();
		pType->SetParenOperator(pFunc);
		pFunc->SetName(m_operatorParenString);
		pFunc->SetId(CHtfeIdent::id_function);
		pFunc->SetType(pType);
		CHtfeParam tmp1 = CHtfeParam(m_pIntType, true);
		pFunc->AppendParam(tmp1);
		CHtfeParam tmp2 = CHtfeParam(m_pIntType, true);
		pFunc->AppendParam(tmp2);

		// builtin members
		pType->AddBuiltinMember(m_rangeString, CHtfeIdent::fid_range, pType, m_pIntType, m_pIntType);
		pType->AddBuiltinMember(m_andReduceString, CHtfeIdent::fid_and_reduce, m_pBoolType);
		pType->AddBuiltinMember(m_nandReduceString, CHtfeIdent::fid_nand_reduce, m_pBoolType);
		pType->AddBuiltinMember(m_orReduceString, CHtfeIdent::fid_or_reduce, m_pBoolType);
		pType->AddBuiltinMember(m_norReduceString, CHtfeIdent::fid_nor_reduce, m_pBoolType);
		pType->AddBuiltinMember(m_xorReduceString, CHtfeIdent::fid_xor_reduce, m_pBoolType);
		pType->AddBuiltinMember(m_xnorReduceString, CHtfeIdent::fid_xnor_reduce, m_pBoolType);
	} 
	return pType;
}

CHtfeIdent * CHtfeDesign::CreateUniqueScBigIntType(CHtfeIdent *pBaseType, int width)
{
	if (width == 0)
		ParseMsg(PARSE_FATAL, "Invalid type width (0)");

	char widthName[16];
	sprintf(widthName, "%d", width);
	string typeName = pBaseType->GetName() + "<" + widthName + "> ";
	CHtfeIdent * pType = m_pTopHier->InsertType(typeName);

	if (pType->GetId() == CHtfeIdent::id_new) {
		pType->SetId(CHtfeIdent::id_class);
		pType->SetWidth(width);
		pType->SetOpWidth(pBaseType->GetOpWidth());
		pType->SetIsParenOverloaded();
		pType->SetIsSigned(pBaseType->IsSigned());
		pType->SetUserConvType(pBaseType->GetUserConvType());

		// initialize builtin functions
		CHtfeIdent *pFunc = CHtfeDesign::NewIdent();
		pType->SetParenOperator(pFunc);
		pFunc->SetName(m_operatorParenString);
		pFunc->SetId(CHtfeIdent::id_function);
		pFunc->SetType(pType);
		CHtfeParam tmp1 = CHtfeParam(m_pIntType, true);
		pFunc->AppendParam(tmp1);
		CHtfeParam tmp2 = CHtfeParam(m_pIntType, true);
		pFunc->AppendParam(tmp2);

		// builtin members
		pType->AddBuiltinMember(m_rangeString, CHtfeIdent::fid_range, pType, m_pIntType, m_pIntType);
		pType->AddBuiltinMember(m_andReduceString, CHtfeIdent::fid_and_reduce, m_pBoolType);
		pType->AddBuiltinMember(m_nandReduceString, CHtfeIdent::fid_nand_reduce, m_pBoolType);
		pType->AddBuiltinMember(m_orReduceString, CHtfeIdent::fid_or_reduce, m_pBoolType);
		pType->AddBuiltinMember(m_norReduceString, CHtfeIdent::fid_nor_reduce, m_pBoolType);
		pType->AddBuiltinMember(m_xorReduceString, CHtfeIdent::fid_xor_reduce, m_pBoolType);
		pType->AddBuiltinMember(m_xnorReduceString, CHtfeIdent::fid_xnor_reduce, m_pBoolType);

		// constructor
		pType->AddNullConstructor(m_pIntType);
		pType->AddNullConstructor(m_pBigIntBaseType);

		// user defined conversion members
		pType->AddNullUserConversion(m_pBigIntBaseType);

		// overloaded operator members - diadic
		pType->AddNullConvOverloadedOperator(tk_equal, pType, m_pIntType);
		pType->AddNullConvOverloadedOperator(tk_plusEqual, pType, m_pIntType);
		pType->AddNullConvOverloadedOperator(tk_minusEqual, pType, m_pIntType);
		pType->AddNullConvOverloadedOperator(tk_lessLessEqual, pType, m_pIntType);
		pType->AddNullConvOverloadedOperator(tk_greaterGreaterEqual, pType, m_pIntType);
		pType->AddNullConvOverloadedOperator(tk_vbarEqual, pType, m_pIntType);
		pType->AddNullConvOverloadedOperator(tk_ampersandEqual, m_pIntType, m_pIntType);
		pType->AddNullConvOverloadedOperator(tk_carotEqual, m_pIntType, m_pIntType);
		pType->AddNullConvOverloadedOperator(tk_asteriskEqual, pType, m_pIntType);
		pType->AddNullConvOverloadedOperator(tk_slashEqual, pType, m_pIntType);
		pType->AddNullConvOverloadedOperator(tk_percentEqual, pType, m_pIntType);

		pType->AddNullConvOverloadedOperator(tk_equal, pType, m_pBigIntBaseType);
		pType->AddNullConvOverloadedOperator(tk_plusEqual, pType, m_pBigIntBaseType);
		pType->AddNullConvOverloadedOperator(tk_minusEqual, pType, m_pBigIntBaseType);
		pType->AddNullConvOverloadedOperator(tk_lessLessEqual, pType, m_pBigIntBaseType);
		pType->AddNullConvOverloadedOperator(tk_greaterGreaterEqual, pType, m_pBigIntBaseType);
		pType->AddNullConvOverloadedOperator(tk_vbarEqual, pType, m_pBigIntBaseType);
		pType->AddNullConvOverloadedOperator(tk_ampersandEqual, m_pIntType, m_pBigIntBaseType);
		pType->AddNullConvOverloadedOperator(tk_carotEqual, m_pIntType, m_pBigIntBaseType);
		pType->AddNullConvOverloadedOperator(tk_asteriskEqual, pType, m_pBigIntBaseType);
		pType->AddNullConvOverloadedOperator(tk_slashEqual, pType, m_pBigIntBaseType);
		pType->AddNullConvOverloadedOperator(tk_percentEqual, pType, m_pBigIntBaseType);

		pType->AddNullConvOverloadedOperator(tk_plus, pType, m_pIntType);
		pType->AddNullConvOverloadedOperator(tk_minus, pType, m_pIntType);
		pType->AddNullConvOverloadedOperator(tk_lessLess, pType, m_pIntType);
		pType->AddNullConvOverloadedOperator(tk_greaterGreater, pType, m_pIntType);
		pType->AddNullConvOverloadedOperator(tk_vbar, pType, m_pIntType);
		pType->AddNullConvOverloadedOperator(tk_ampersand, m_pIntType, m_pIntType);
		pType->AddNullConvOverloadedOperator(tk_carot, m_pIntType, m_pIntType);
		pType->AddNullConvOverloadedOperator(tk_asterisk, pType, m_pIntType);
		pType->AddNullConvOverloadedOperator(tk_slash, pType, m_pIntType);
		pType->AddNullConvOverloadedOperator(tk_percent, pType, m_pIntType);

		pType->AddNullConvOverloadedOperator(tk_plus, pType, m_pBigIntBaseType);
		pType->AddNullConvOverloadedOperator(tk_minus, pType, m_pBigIntBaseType);
		pType->AddNullConvOverloadedOperator(tk_lessLess, pType, m_pBigIntBaseType);
		pType->AddNullConvOverloadedOperator(tk_greaterGreater, pType, m_pBigIntBaseType);
		pType->AddNullConvOverloadedOperator(tk_vbar, pType, m_pBigIntBaseType);
		pType->AddNullConvOverloadedOperator(tk_ampersand, m_pIntType, m_pBigIntBaseType);
		pType->AddNullConvOverloadedOperator(tk_carot, m_pIntType, m_pBigIntBaseType);
		pType->AddNullConvOverloadedOperator(tk_asterisk, pType, m_pBigIntBaseType);
		pType->AddNullConvOverloadedOperator(tk_slash, pType, m_pBigIntBaseType);
		pType->AddNullConvOverloadedOperator(tk_percent, pType, m_pBigIntBaseType);

		pType->AddNullConvOverloadedOperator(tk_vbarVbar, m_pBoolType, m_pIntType);
		pType->AddNullConvOverloadedOperator(tk_ampersandAmpersand, m_pBoolType, m_pIntType);
		pType->AddNullConvOverloadedOperator(tk_equalEqual, m_pBoolType, m_pIntType);
		pType->AddNullConvOverloadedOperator(tk_bangEqual, m_pBoolType, m_pIntType);
		pType->AddNullConvOverloadedOperator(tk_less, m_pBoolType, m_pIntType);
		pType->AddNullConvOverloadedOperator(tk_greater, m_pBoolType, m_pIntType);
		pType->AddNullConvOverloadedOperator(tk_lessEqual, m_pBoolType, m_pIntType);
		pType->AddNullConvOverloadedOperator(tk_greaterEqual, m_pBoolType, m_pIntType);

		pType->AddNullConvOverloadedOperator(tk_vbarVbar, m_pBoolType, m_pBigIntBaseType);
		pType->AddNullConvOverloadedOperator(tk_ampersandAmpersand, m_pBoolType, m_pBigIntBaseType);
		pType->AddNullConvOverloadedOperator(tk_equalEqual, m_pBoolType, m_pBigIntBaseType);
		pType->AddNullConvOverloadedOperator(tk_bangEqual, m_pBoolType, m_pBigIntBaseType);
		pType->AddNullConvOverloadedOperator(tk_less, m_pBoolType, m_pBigIntBaseType);
		pType->AddNullConvOverloadedOperator(tk_greater, m_pBoolType, m_pBigIntBaseType);
		pType->AddNullConvOverloadedOperator(tk_lessEqual, m_pBoolType, m_pBigIntBaseType);
		pType->AddNullConvOverloadedOperator(tk_greaterEqual, m_pBoolType, m_pBigIntBaseType);
	}
	return pType;
}

void CHtfeIdent::AddNullConstructor(CHtfeIdent * pParamType)
{
	CHtfeIdent * pFunc = InsertIdent("Constructor$", false, true);
	pFunc->SetId(CHtfeIdent::id_function);
	pFunc->SetType(CHtfeDesign::m_pVoidType);
	pFunc->AppendParam(CHtfeParam(pParamType, true));
	pFunc->SetIsConstructor(true);
	pFunc->SetNullConvOperator(true);
}

void CHtfeIdent::AddBuiltinMember(string const & name, CHtfeIdent::EFuncId funcId, CHtfeIdent * pRtnType)
{
	CHtfeIdent * pFunc = InsertIdent(name, false);
	pFunc->SetId(CHtfeIdent::id_function);
	pFunc->SetFuncId(funcId);
	pFunc->SetType(pRtnType);
}

void CHtfeIdent::AddBuiltinMember(string const & name, CHtfeIdent::EFuncId funcId, CHtfeIdent * pRtnType, CHtfeIdent * pParam1Type, CHtfeIdent * pParam2Type)
{
	CHtfeIdent * pFunc = InsertIdent(name, false);
	pFunc->SetId(CHtfeIdent::id_function);
	pFunc->SetFuncId(funcId);
	pFunc->SetType(pRtnType);
	pFunc->AppendParam(CHtfeParam(pParam1Type, true));
	pFunc->AppendParam(CHtfeParam(pParam2Type, true));
}

void CHtfeIdent::AddNullUserConversion(CHtfeIdent * pConvType)
{
	string name = "operator$" + pConvType->GetName();
	CHtfeIdent * pFunc = InsertIdent(name, false);
	pFunc->SetId(CHtfeIdent::id_function);
	pFunc->SetType(pConvType);
	pFunc->SetIsUserConversion(true);
	pFunc->SetNullConvOperator(true);
}

void CHtfeIdent::AddNullConvOverloadedOperator(CHtfeLex::EToken eToken, CHtfeIdent * pRtnType, CHtfeIdent * pParamType)
{
	string funcName = "operator" + CHtfeLex::GetTokenString(eToken);
	CHtfeIdent * pFunc = InsertIdent(funcName, false, true);
	pFunc->SetId(CHtfeIdent::id_function);
	pFunc->SetType(pRtnType);
	pFunc->AppendParam(CHtfeParam(pParamType, true));
	pFunc->SetIsOverloadedOperator(true);
	pFunc->SetOverloadedOperator(eToken);
	pFunc->SetNullConvOperator(true);
}

void CHtfeIdent::AddNullConvOverloadedOperator(CHtfeLex::EToken eToken, CHtfeIdent * pRtnType, CHtfeIdent * pParam1Type, CHtfeIdent * pParam2Type)
{
	string funcName = "operator" + CHtfeLex::GetTokenString(eToken);
	CHtfeIdent * pFunc = InsertIdent(funcName, false, true);
	pFunc->SetId(CHtfeIdent::id_function);
	pFunc->SetType(pRtnType);
	pFunc->AppendParam(CHtfeParam(pParam1Type, true));
	pFunc->AppendParam(CHtfeParam(pParam2Type, true));
	pFunc->SetIsOverloadedOperator(true);
	pFunc->SetOverloadedOperator(eToken);
	pFunc->SetNullConvOperator(true);
}

void CHtfeDesign::ParseEnumDecl(CHtfeIdent *pHier)
{
	if (GetNextToken() != tk_identifier) {
		ParseMsg(PARSE_ERROR, "Expected an enum name");
		SkipTo(tk_semicolon);
		return;
	}

	CHtfeIdent *pType = pHier->InsertType(GetString());
	pType->SetId(CHtfeIdent::id_enumType);
	pType->SetIsType();
	pType->SetWidth(1);
	pType->SetOpWidth(32);
	pType->SetUserConvType(m_pIntType);

	if (GetNextToken() == tk_semicolon)
		return;

	if (GetToken() != tk_lbrace) {
		ParseMsg(PARSE_ERROR, "Expected '{' or ';'");
		SkipTo(tk_semicolon);
		return;
	}

	GetNextToken();

	vector<int> enumList;
	int enumValue;
	for (enumValue = 0;; enumValue += 1) {
		if (GetToken() != tk_identifier) {
			ParseMsg(PARSE_ERROR, "Expected an enum identifier");
			SkipTo(tk_semicolon);
			return;
		}

		CHtfeIdent *pIdent = pHier->InsertIdent(GetString());

		if (pIdent->GetId() != CHtfeIdent::id_new)
			ParseMsg(PARSE_ERROR, "Enum identifier already used");

		if (GetNextToken() == tk_equal) {
			if (GetNextToken() != tk_num_int && GetToken() != tk_num_hex) {
				ParseMsg(PARSE_ERROR, "expected an integer enum value");
				SkipTo(tk_comma, tk_rbrace);
			} else {
				// convert string to 32-bit integer
				CConstValue constValue = GetConstValue();
				int64_t newValue = constValue.GetSint64();

				if (newValue >= 0x80000000 || newValue < 0)
					ParseMsg(PARSE_ERROR, "enum value out of range 0-0x7fffffff");

				for (size_t i = 0; i < enumList.size(); i += 1)
					if (newValue == enumList[i])
						ParseMsg(PARSE_ERROR, "enum value(%d) matches previous enum value", newValue);
				
				enumValue = (int)newValue;

				GetNextToken();
			}
		}

		if (pIdent->GetId() == CHtfeIdent::id_new) {
			pIdent->SetId(CHtfeIdent::id_enum);
			pIdent->SetConstValue(enumValue);
			pIdent->SetType(pType);
			pIdent->SetLineInfo(GetLineInfo());

			enumList.push_back(enumValue);
		}

		switch (GetToken()) {
		case tk_comma:
			GetNextToken();
			continue;
		case tk_rbrace:
			{
				if (GetNextToken() != tk_semicolon) {
					ParseMsg(PARSE_ERROR, "Expected a ';'");
					SkipTo(tk_semicolon);
				}
				GetNextToken();

				int maxEnumValue = 0;
				for (size_t i = 0; i < enumList.size(); i += 1)
					if (enumList[i] > maxEnumValue)
						maxEnumValue = enumList[i];

				CConstValue v((uint32_t)maxEnumValue);
				pType->SetWidth(v.GetMinWidth());
			}
			return;
		default:
			ParseMsg(PARSE_ERROR, "Expected '}' or ','");
			SkipTo(tk_semicolon);
			return;
		}
	}
}

CHtfeStatement * CHtfeDesign::ParseVariableDecl(CHtfeIdent *pHier, CHtfeIdent *pType, CHtfeTypeAttrib const & typeAttrib, bool bIsReference)
{
	CHtfeStatement * pHeadStatement = 0;
	CHtfeStatement ** ppStatement = & pHeadStatement;

	// parse comma separated list of variable declarations
	for (;;) {
		if (GetToken() != tk_identifier) {
			ParseMsg(PARSE_ERROR, "Expected a variable name");
			SkipTo(tk_semicolon);
			GetNextToken();
			return 0;
		}

		const CHtfeIdent *pHier2 = pHier;
		CHtfeIdent *pIdent = ParseVariableRef(pHier, pHier2, false, true);

		if (pType) {

			if (pIdent) {
				if (pIdent->GetId() != CHtfeIdent::id_function) {
					if (pHier == pHier2) {
						ParseMsg(PARSE_ERROR, "'%s' identifier redeclared", pIdent->GetName().c_str());
						ParseMsg(PARSE_ERROR, pIdent->GetLineInfo(), "    original variable declaration");
						SkipTo(tk_semicolon);
						GetNextToken();
						return 0;
					}

					// variable with same name exists but not in the local scope
					pIdent = 0;
				}
			} else
				GetNextToken();

			if (pIdent == 0) {

				pIdent = pHier->InsertIdent(GetString());

				pHier2 = pHier;

				if (pIdent->GetId() != CHtfeIdent::id_new) {
					ParseMsg(PARSE_ERROR, "earlier errors prohibit further parsing");
					SkipTo(tk_eof);
					return 0;
				}

				pIdent->SetId(CHtfeIdent::id_variable);
				pIdent->SetType(pType);
				pIdent->SetLineInfo(GetLineInfo());
				pIdent->SetDeclStatementLevel(m_statementNestLevel);

				if (GetToken() == tk_lbrack)
					ParseVarArrayDecl(pIdent);

				pIdent->AllocRefInfo();

				if (pType->IsHtDistQue()) {
					pIdent->SetIsHtDistQue();
					pIdent->SetIsRegister();
					HtQueueVarDecl(pHier, pIdent);
				} else if (pType->IsHtBlockQue()) {
					pIdent->SetIsHtBlockQue();
					pIdent->SetIsRegister();
					HtQueueVarDecl(pHier, pIdent);
				} else if (pType->IsHtDistRam()) {
					pIdent->SetIsHtDistRam();
					pIdent->SetIsRegister();
					HtMemoryVarDecl(pHier, pIdent);
				} else if (pType->IsHtBlockRam()) {
					pIdent->SetIsHtBlockRam();
					pIdent->SetIsRegister();
					HtMemoryVarDecl(pHier, pIdent);
				} else if (pType->IsHtMrdBlockRam()) {
					pIdent->SetIsHtMrdBlockRam();
					pIdent->SetIsRegister();
					HtMemoryVarDecl(pHier, pIdent);
				} else if (pType->IsHtMwrBlockRam()) {
					pIdent->SetIsHtMwrBlockRam();
					pIdent->SetIsRegister();
					HtMemoryVarDecl(pHier, pIdent);
				} else {
					pIdent->SetIsReference(bIsReference);
					pIdent->SetIsNoLoad(typeAttrib.m_bIsHtNoLoad);
					pIdent->SetIsScPrim(typeAttrib.m_bIsHtPrim);
					pIdent->SetIsScLocal(typeAttrib.m_bIsHtLocal);
					pIdent->SetHtPrimClkList(typeAttrib.m_htPrimClkList);

					if (typeAttrib.m_bIsHtPrim && pType != m_pVoidType)
						ParseMsg(PARSE_ERROR, "ht_prim functions can not return a value");
				}

				if (GetToken() == tk_colonColon)
					ParseMsg(PARSE_FATAL, "'%s' undeclared heirarchy identifier", pIdent->GetName().c_str());
			}

		} else if (!pIdent) {
			ParseMsg(PARSE_ERROR, "Unknown identifier");
			SkipTo(tk_semicolon);
			GetNextToken();
			return 0;
		}

		if (pIdent && pIdent->GetDimenCnt() > 0 && pIdent->GetDimenList()[0] == 0 && GetToken() != tk_equal) {
			// first dimen was not specified and an initializer is not present
			ParseMsg(PARSE_ERROR, "Leading dimension was not specified and an initializer is not present");
			pIdent->GetDimenList()[0] = 1; // avoid issues
		}

		if (pHier->IsModule() && GetToken() != tk_lparen && IsRegisterName(pIdent->GetName()))
			pIdent->SetIsRegister();

		bool bLBrack = false;
		bool bLParen = false;
		vector<int> dimenList;

		static bool bErrorMsg = false;
		if (pHier->GetName().size() == 0 && GetToken() != tk_lparen && bErrorMsg == false) {
			ParseMsg(PARSE_ERROR, "global namespace variables are not supported");
			bErrorMsg = true;
		}

		// parse initializer statement if present
		for (;;) {
			CHtfeIdent *pIdent2;
			switch (GetToken()) {
			case tk_lbrack:
				// array
				bLBrack = true;
				if (bLParen)
					ParseMsg(PARSE_ERROR, "arrays of functions are not supported");

				if (GetNextToken() == tk_identifier && (pIdent2 = pHier->FindIdent(GetString())) &&
					pIdent2->GetId() == CHtfeIdent::id_enum) {

						dimenList.push_back((int)pIdent2->GetConstValue().GetSint64());

				} else if (GetToken() == tk_num_int || GetToken() == tk_num_hex) {

					int idxDeclValue;
					if (!GetSint32ConstValue(idxDeclValue) || idxDeclValue > 0x10000) {
						ParseMsg(PARSE_ERROR, "exceeded maximum array size (0x10000)");
						idxDeclValue = 0;
					}

					dimenList.push_back(idxDeclValue);

				} else {
					ParseMsg(PARSE_ERROR, "Expected an array specifier");
					SkipTo(tk_semicolon);
					GetNextToken();
					return 0;
				}

				if (GetNextToken() != tk_rbrack) {
					ParseMsg(PARSE_ERROR, "syntax error, expected a ']'");
					SkipTo(tk_semicolon);
					return 0;
				}
				GetNextToken();

				break;
			case tk_lparen:
				bLParen = true;
				if (bLBrack)
					ParseMsg(PARSE_ERROR, "arrays of functions is not supported");

				if (!ParseFunctionDecl(pHier, pHier2, pIdent))
					return 0;

				break;
			case tk_comma:
				{
					if (bLBrack) {
						pIdent->SetDimenList(dimenList);
						dimenList.clear();
					}

					// variable was not initialized, initialize to unknown 'hx if declared within a method
					CHtfeIdent *pHier2 = pHier->FindHierMethodOrFunction();
					if (pHier2) {
						CHtfeStatement *pUnknownStatement = InitializeToUnknown(pHier, pIdent);
						pHier2->InsertStatementList(pUnknownStatement);
					}
				}
				break;
			case tk_equal:
				{
					if (!pHier->IsLocal()/* || bIsArrayDecl*/) {

						ParseMsg(PARSE_ERROR, "Illegal assignment");
						SkipTo(tk_semicolon);
						GetNextToken();
						return 0;
					}

					if (pIdent) {
						if (pIdent->IsRegister())
							pIdent->SetClockIdent(pHier->GetClockIdent());

					} else
						ParseMsg(PARSE_ERROR, "unknown assignment identifier");

					GetNextToken();

					CHtfeStatement * pStatement = *ppStatement = ParseVarDeclInitializer(pHier, pIdent);
					while (pStatement->GetNext())
						pStatement = pStatement->GetNext();
					ppStatement = pStatement->GetPNext();

					if (GetToken() == tk_semicolon)
						return pHeadStatement;

					break;
				}
			case tk_semicolon:
				{
					if (bLBrack) {
						pIdent->SetDimenList(dimenList);
						dimenList.clear();
					}
					GetNextToken();

					// variable was not initialized, initialize to unknown 'hx if declared within a method
					CHtfeIdent *pHier2 = pHier->FindHierMethodOrFunction();
					if (pHier2) {
						CHtfeStatement *pUnknownStatement = InitializeToUnknown(pHier, pIdent);
						pHier2->InsertStatementList(pUnknownStatement);
					}
					return pHeadStatement;
				}
			case tk_colon:
				{
					// member width
					CConstValue value;
					GetNextToken();
					if (!ParseConstExpr(value)) {
						ParseMsg(PARSE_ERROR, "expected a member width");
						SkipTo(tk_semicolon);
						GetNextToken();
						return 0;
					}

					pIdent->SetWidth(value.GetSint32());
					break;
				}
			default:
				ParseMsg(PARSE_ERROR, "Expected '}' or ','");
				SkipTo(tk_semicolon);
				GetNextToken();
				return 0;
			}

			if (GetToken() == tk_comma) {
				GetNextToken();
				break;
			}
		}
	}
}

bool CHtfeDesign::ParseFunctionDecl(CHtfeIdent *pHier, const CHtfeIdent *pHier2, CHtfeIdent *pIdent)
{
	// pHier - space where current declaration is located
	// pHier2 - space where class is located          CClass::Func;

	if (pHier->GetId() == CHtfeIdent::id_function) {

		ParseMsg(PARSE_ERROR, "Illegal function declaration nesting");
		SkipTo(tk_semicolon);
		GetNextToken();
		return false;
	}

	if (pIdent->GetId() == CHtfeIdent::id_variable) {
		pIdent->SetId(CHtfeIdent::id_function);
		pIdent->SetIsUserDefinedFunc();
	}

	ParseParamListDecl(const_cast<CHtfeIdent*>(pHier2), pIdent);

	if (GetToken() == tk_const)
		GetNextToken();

	if (GetToken() == tk_lbrace) {

		if (pIdent->IsBodyDefined())
			ParseMsg(PARSE_ERROR, "body already defined for function");

		if (const_cast<CHtfeIdent*>(pHier2)->IsModule())
			const_cast<CHtfeIdent*>(pHier2)->SetIsMethodDefined();

		if (IsInputFile()) {
			pIdent->SetIsInputFile();
			const_cast<CHtfeIdent*>(pHier2)->SetIsInputFile();
		}

		pIdent->SetIsBodyDefined();
		pIdent->SetPrevHier(const_cast<CHtfeIdent*>(pHier2));
		pIdent->SetIsLocal();
		//if (!pIdent->IsMethod())
		pIdent->NewFlatTable(); // functions and tasks have a local flat table (methods do not)

		// add parameters as local variables
		for (int paramId = 0; paramId < pIdent->GetParamCnt(); paramId += 1) {
			CHtfeIdent *pParam = pIdent->InsertIdent(pIdent->GetParamName(paramId));
			pParam->SetType(pIdent->GetParamType(paramId));
			pParam->SetId(CHtfeIdent::id_variable);
			pParam->SetLineInfo(GetLineInfo());
			pParam->SetIsParam();
			pParam->SetIsConst(pIdent->GetParamIsConst(paramId));
			pParam->SetIsNoLoad(pIdent->IsParamNoload(paramId));
			pIdent->SetParamIdent(paramId, pParam);

			if (pIdent->GetParamIsScState(paramId))
				pParam->SetIsScState();
		}

		// used for memory write format checking
		m_statementNestLevel = 0;
		m_parseReturnCount = 0;

		pIdent->AppendStatementList(ParseCompoundStatement(pIdent, false));

		// transform statement list such that no state is modified following a return statement
		TransformFunctionStatementListForReturns(pIdent->GetStatementList());

		return false;
	}

	return true;
}

void CHtfeDesign::TransformFunctionStatementListForReturns(CHtfeStatement *pStmtHead)
{
	// functions are implemented as tasks in verilog. Tasks do not have a return value.
	//   returned values are implemented as a pass by reference parameter.
	//   transform if statements with embedded returns to ensure no state is modified
	//   after a return statement.
	// The algorithm merges post if-else statements into the if-else.
	//   The algorithm starts at the end of the statement list and
	//   works to the beginning using recussion.

	if (GetErrorCnt() > 0)
		return;	// errors can cause unexpected statement sequences

	CHtfeStatement *pStmt = pStmtHead;
	while (pStmt && (pStmt->GetStType() != st_if || !(IsEmbeddedReturnStatement(pStmt->GetCompound1()) ||
		IsEmbeddedReturnStatement(pStmt->GetCompound2()))))
		pStmt = pStmt->GetNext();

	CHtfeStatement * pStIf = pStmt;
	if (pStIf) {
		// found an if statement with embedded return, see if there are more if's in the list
		CHtfeStatement *pStmt2 = pStIf->GetNext();
		while (pStmt2 && (pStmt2->GetStType() != st_if || !(IsEmbeddedReturnStatement(pStmt2->GetCompound1()) ||
			IsEmbeddedReturnStatement(pStmt2->GetCompound2()))))
			pStmt2 = pStmt2->GetNext();

		if (pStmt2)
			TransformFunctionStatementListForReturns(pStmt2);

		if (IsEmbeddedReturnStatement(pStIf)) {
			// found an embedded return statement

			CHtfeStatement *pPostList = pStIf->GetNext();
			pStIf->SetNext(0);

			TransformFunctionIfStatementList(pStIf->GetPCompound1(), pPostList);
			TransformFunctionIfStatementList(pStIf->GetPCompound2(), pPostList); 
		}
	}
}

void CHtfeDesign::TransformFunctionIfStatementList(CHtfeStatement **ppList, CHtfeStatement *pPostList)
{
	// Append postList on to pIfList provided pIfList doesn't end in a return
	CHtfeStatement **ppTail = ppList;
	while (*ppTail && (*ppTail)->GetNext())
		ppTail = (*ppTail)->GetPNext();

	if (*ppTail == 0)
		*ppTail = pPostList;
	else if ((*ppTail)->GetStType() != st_return)
		(*ppTail)->SetNext(pPostList);

	TransformFunctionStatementListForReturns(*ppList);
}

bool CHtfeDesign::IsEmbeddedReturnStatement(CHtfeStatement *pStatement)
{
	while (pStatement) {
		switch (pStatement->GetStType()) {
		case st_assign:
		case st_switch:
			// no possibility of an embedded return in these statements
			break;
		case st_return:
			return true;
		case st_if:
			if (IsEmbeddedReturnStatement(pStatement->GetCompound1()) ||
				IsEmbeddedReturnStatement(pStatement->GetCompound2()))
				return true;
			break;
		default:
			ParseMsg(PARSE_INFO, "  Unexpected statement type %d", pStatement->GetStType());
			Assert(0);
		}
		pStatement = pStatement->GetNext();
	}
	return false;
}

CHtfeStatement * CHtfeDesign::ParseVarDeclInitializer(CHtfeIdent * pHier, CHtfeIdent * pIdent)
{
	// int a[4][5] = { { 1, 2 }, { 5 }, 7, 0, 0, 0, 0, { 9, 10 } };

	CHtfeStatement * pStatementHead = 0;
	CHtfeStatement * pStatementTail = 0;
	CHtfeStatement * pStatement = 0;

	bool bAutoSizeLeadingDimen = false;
	if (pIdent->GetDimenCnt() > 0 && pIdent->GetDimenList()[0] == 0) {
		bAutoSizeLeadingDimen = true;
		pIdent->GetDimenList()[0] = 0x3ffff;
	}

	int dimenCnt = pIdent->GetDimenCnt();

	if (dimenCnt > 0 && pIdent->GetType()->IsStruct())
		ParseMsg(PARSE_FATAL, "  Initialization of dimensioned structures is not currently supported");

	vector<int> refList(dimenCnt, 0);
	vector<bool> constList(dimenCnt, false);
	size_t openBraceLvl = 0;
	for (;;) {
		while (GetToken() == tk_lbrace) {
			if (openBraceLvl < refList.size() && refList[openBraceLvl] != 0)
				ParseMsg(PARSE_ERROR, "Initializer error");

			openBraceLvl += 1;

			GetNextToken();
		}

		if (GetToken() == tk_string) {

			if (refList[dimenCnt-1] != 0) {
				ParseMsg(PARSE_ERROR, "Initializer error");
				SkipTo(tk_semicolon);
				if (bAutoSizeLeadingDimen)
					pIdent->SetDimen(0, refList[0]+1);
				return pStatement;
			}

			string initStr = GetString();

			if ((int)initStr.size() + 1 > pIdent->GetDimenList()[dimenCnt-1]) {
				ParseMsg(PARSE_ERROR, "Too many characters in string");
				SkipTo(tk_semicolon);
				if (bAutoSizeLeadingDimen)
					pIdent->SetDimen(0, refList[0]+1);
				return pStatement;
			}

			for (int i = 0; i < pIdent->GetDimenList()[dimenCnt-1]; i += 1) {

				char initBuf[8];
				if (openBraceLvl == 0 && bAutoSizeLeadingDimen && i > (int)initStr.size())
					break;
				else if (i >= (int)initStr.size())
					sprintf(initBuf, "'\\0'");
				else
					sprintf(initBuf, "'%c'", initStr[i]);

				pStatement = InsertVarDeclInitializer(pHier, pIdent, refList, 0, initBuf);

				if (pStatementHead == 0)
					pStatementHead = pStatement;
				else
					pStatementTail->SetNext(pStatement);
				pStatementTail = pStatement;

				pIdent->DimenIter(refList, constList);
			}

			GetNextToken();

		} else {

			CHtfeOperand * pExpr = ParseExpression(pHier, true);

			//if (pIdent->GetType()->IsStruct() && pExpr->GetSubField() == 0 && (!pExpr->IsConstValue() || pExpr->GetConstValue().GetSint64() != 0)) {
			//    ParseMsg(PARSE_ERROR, "struct initialization to non-zero value");
			//    SkipTo(tk_semicolon);
			//    if (bAutoSizeLeadingDimen)
			//       pIdent->SetDimen(0, refList[0]+1);
			//    return pStatement;
			//}

			pStatement = InsertVarDeclInitializer(pHier, pIdent, refList, pExpr);

			if (pStatementHead == 0)
				pStatementHead = pStatement;
			else
				pStatementTail->SetNext(pStatement);
			pStatementTail = pStatement;

			pIdent->DimenIter(refList, constList);
		}

		if (GetToken() == tk_rbrace) {
			while (openBraceLvl > 0 && GetToken() == tk_rbrace) {
				openBraceLvl -= 1;

				// fill with zero to end of dimension
				bool bAtEnd = false;
				if (openBraceLvl == 0 && bAutoSizeLeadingDimen) {
					bAtEnd = true;
					for (size_t i = 1; i < refList.size(); i += 1)
						bAtEnd &= refList[i] == 0;
				}

				while (!bAtEnd && refList[openBraceLvl] != 0) {

					pStatement = InsertVarDeclInitializer(pHier, pIdent, refList, 0, "0");

					if (pStatementHead == 0)
						pStatementHead = pStatement;
					else
						pStatementTail->SetNext(pStatement);
					pStatementTail = pStatement;

					pIdent->DimenIter(refList, constList);
				}

				GetNextToken();
			}

			if (openBraceLvl == 0)
				break;
			else if (GetToken() == tk_comma) {
				GetNextToken();
				continue;
			} else {
				ParseMsg(PARSE_ERROR, "Initializer error");
				SkipTo(tk_semicolon);
			}
		}

		if (openBraceLvl == 0) {
			break;

		} else if (GetToken() == tk_comma) {
			if (openBraceLvl > refList.size() || refList[openBraceLvl-1] == 0) {
				ParseMsg(PARSE_ERROR, "Initializer error");
				SkipTo(tk_semicolon);
				if (bAutoSizeLeadingDimen)
					pIdent->SetDimen(0, refList[0]+1);
				return pStatement;
			}

			GetNextToken();
		} else {
			ParseMsg(PARSE_ERROR, "Initializer syntax error");
			SkipTo(tk_semicolon);
			if (bAutoSizeLeadingDimen)
				pIdent->SetDimen(0, refList[0]+1);
			return pStatement;
		}
	}

	if (GetToken() != tk_semicolon && GetToken() != tk_comma)
		ParseMsg(PARSE_ERROR, "Initializer syntax error");

	if (bAutoSizeLeadingDimen)
		pIdent->SetDimen(0, refList[0]);

	CHtfeIdent *pHier2 = pHier->FindHierMethod();

	if (pHier2 && pHier != pHier2) {
		// variable was declared within a name space, initialize to unknown to avoid latches
		CHtfeStatement *pUnknownStatement = InitializeToUnknown(pHier, pIdent);
		pHier2->InsertStatementList(pUnknownStatement);
	}

	return pStatementHead;
}

struct CConvPair {
	CConvPair(CHtfeIdent * pMemberConv, CHtfeIdent * pOp1Conv, CHtfeIdent * pOp2Conv) : m_pMemberConv(pMemberConv), m_pOp1Conv(pOp1Conv), m_pOp2Conv(pOp2Conv) {}
	CConvPair(CHtfeIdent * pOp1Conv, CHtfeIdent * pOp2Conv) : m_pOp1Conv(pOp1Conv), m_pOp2Conv(pOp2Conv) {}
	CHtfeIdent * m_pMemberConv;
	CHtfeIdent * m_pOp1Conv;
	CHtfeIdent * m_pOp2Conv;
};

CHtfeStatement * CHtfeDesign::InsertVarDeclInitializer(CHtfeIdent * pHier, CHtfeIdent * pIdent, vector<int> &refList, CHtfeOperand * pExpr, char * pInitStr)
{
	string idxList;
	char idxBuf[32];
	for (size_t i = 0; i < refList.size(); i += 1) {
		sprintf(idxBuf, "[%d]", refList[i]);
		idxList += idxBuf;
	}

	bool bRecord = pIdent->GetType()->GetId() == CHtfeIdent::id_struct;

	char lineBuf[512];

	if (!bRecord)
		sprintf(lineBuf, "{ %s%s = %s; }", pIdent->GetName().c_str(), idxList.c_str(), pInitStr );
	else {
		sprintf(lineBuf, "{ %s%s = %s; %s%s = %s; }",
			pIdent->GetName().c_str(), idxList.c_str(), pInitStr,
			pIdent->GetName().c_str(), idxList.c_str(), pInitStr);
	}

	OpenLine(lineBuf);
	GetNextToken();

	CHtfeStatement * pStatement = ParseCompoundStatement(pHier);

	CHtfeStatement * pStatement2 = pStatement;
	if (bRecord)
		pStatement2 = pStatement2->GetNext();

	if (pStatement2 && pExpr) {
		CHtfeOperand * pOp1 = pStatement2->GetExpr()->GetOperand1();
		CHtfeOperand * pOp2 = pExpr;
		// check for user conversions
		// iterate through op2 options
		CHtfeIdent * pOp1Type = pOp1->GetType();
		CHtfeIdent * pOp2Type = pOp2->GetType();

		// iterate through op1 options
		int minConvCnt = 100;
		vector<CConvPair> convList;

		CHtfeOperatorIter op1Iter(pOp1Type);
		for (op1Iter.Begin(); !op1Iter.End(); op1Iter++) {

			if (op1Iter.IsUserConversion()) continue;
			if (op1Iter.IsOverloadedOperator()) continue;

			// iterate through op2 options
			CHtfeOperatorIter op2Iter(pOp2Type);
			for (op2Iter.Begin(); !op2Iter.End(); op2Iter++) {

				if (op2Iter.IsConstructor()) continue;
				if (op2Iter.IsOverloadedOperator()) continue;

				// check if op1Conv and op2Conv work for operation
				if (op1Iter.GetType()->GetId() == CHtfeIdent::id_intType && op2Iter.GetType()->GetId() == CHtfeIdent::id_intType ||
					op1Iter.GetType() == op2Iter.GetType() ||
					pOp2->IsConstValue() && pOp2->GetConstValue().IsZero())
				{
					int convCnt = op1Iter.GetConvCnt() + op2Iter.GetConvCnt();
					if (convCnt > minConvCnt) continue;
					if (convCnt < minConvCnt)
						convList.clear();
					convList.push_back(CConvPair(op1Iter.GetMethod(), op2Iter.GetMethod()));
					minConvCnt = convCnt;
				}
			}
		}

		if (minConvCnt == 100)
			ParseMsg(PARSE_ERROR, "Incompatible operands for expression");
		else if (minConvCnt > 0) {
			if (convList.size() == 1) {
				// apply conversions to expression
				if (convList[0].m_pOp2Conv) {
					if (convList[0].m_pOp2Conv->IsNullConvOperator()) {
						pOp2Type = convList[0].m_pOp2Conv->GetType();
					} else {
						CHtfeOperand *pConvMethodOp = HandleNewOperand();
						pConvMethodOp->InitAsIdentifier(GetLineInfo(), convList[0].m_pOp2Conv);
						pConvMethodOp->SetType(convList[0].m_pOp2Conv->GetType());

						CHtfeOperand *pConvRslt = HandleNewOperand();
						pConvRslt->InitAsOperator(GetLineInfo(), tk_period, pOp2, pConvMethodOp);
						pConvRslt->SetType(convList[0].m_pOp2Conv->GetType());
						pOp2 = pConvRslt;
						pOp2Type = pOp2->GetType();
					}
				}

				if (convList[0].m_pOp1Conv) {
					if (convList[0].m_pOp1Conv->IsNullConvOperator()) {
						pOp1Type = convList[0].m_pOp1Conv->GetType();
					} else {
						if (convList[0].m_pOp1Conv->IsConstructor()) {
							CHtfeOperand *pConvMethodOp = HandleNewOperand();
							pConvMethodOp->InitAsFunction(GetLineInfo(), convList[0].m_pOp1Conv);
							pConvMethodOp->SetType(convList[0].m_pOp1Conv->GetType());

							vector<CHtfeOperand *> paramList;
							paramList.push_back(pOp2);
							pConvMethodOp->SetParamList(paramList);

							CHtfeOperand *pConvRslt = HandleNewOperand();
							pConvRslt->InitAsOperator(GetLineInfo(), tk_period, pOp1, pConvMethodOp);
							pConvRslt->SetType(convList[0].m_pOp1Conv->GetType());

							pStatement2->SetExpr(pConvRslt);
							HandleIntegerConstantPropagation(pStatement2->GetExpr());

							return pStatement;
						} else
							Assert(0);
					}
				}

			} else {
				// ambiguous conversion solutions
				ParseMsg(PARSE_ERROR, "ambiguous expression for operator =, possible solutions are:");

				for (size_t i = 0; i < convList.size(); i += 1) {
					string op1Conv;
					if (convList[i].m_pOp1Conv == 0)
						op1Conv = pOp1Type->GetName();
					else if (convList[i].m_pOp1Conv->IsConstructor()) {
						op1Conv = convList[i].m_pOp1Conv->GetParamType(0)->GetName() + "()";
					} else
						Assert(0);

					string op2Conv;
					if (convList[i].m_pOp2Conv == 0)
						op2Conv = pOp2Type->GetName();
					else if (convList[i].m_pOp2Conv->IsOverloadedOperator()) {
						op2Conv = "operator " + CHtfeLex::GetTokenString(convList[i].m_pOp2Conv->GetOverloadedOperator()) + " (" +
							convList[i].m_pOp2Conv->GetParamType(0)->GetName() + ")";
					} else if (convList[i].m_pOp2Conv->IsUserConversion()) {
						op2Conv = "operator " + convList[i].m_pOp2Conv->GetType()->GetName() + " ()";
					} else
						Assert(0);

					ParseMsg(PARSE_INFO, "%s, %s", op1Conv.c_str(), op2Conv.c_str());
				}
			}
		}

		pStatement2->GetExpr()->SetOperand2(pOp2);
		HandleIntegerConstantPropagation(pStatement2->GetExpr());
	}

	return pStatement;
}

CHtfeStatement * CHtfeDesign::InitializeToUnknown(CHtfeIdent *pHier, CHtfeIdent * pIdent)
{
	CHtfeStatement *pStatementHead = 0;
	CHtfeStatement **ppStatementTail = &pStatementHead;

	vector<int> refList(pIdent->GetDimenCnt(), 0);

	do {
		vector<CHtfeOperand *> indexList;
		for (size_t i = 0; i < refList.size(); i += 1) {
			CHtfeOperand *pOp = HandleNewOperand();
			pOp->InitAsConstant(GetLineInfo(), CConstValue(refList[i]));

			indexList.push_back(pOp);
		}

		CHtfeOperand *pVar = HandleNewOperand();
		pVar->InitAsIdentifier(GetLineInfo(), pIdent->GetFlatIdent());
		pVar->SetIndexList(indexList);

		CHtfeOperand *pScX = HandleNewOperand();
		pScX->InitAsConstant(GetLineInfo(), CConstValue(0));
		if (g_pHtfeArgs->IsLvxEnabled())
			pScX->SetIsScX();

		CHtfeOperand *pAssign = HandleNewOperand();
		pAssign->InitAsOperator(GetLineInfo(), tk_equal, pVar, pScX);

		CHtfeStatement *pStatement = HandleNewStatement();
		pStatement->Init(st_assign, GetLineInfo());

		pStatement->SetExpr(pAssign);
		pStatement->SetIsInitStatement(true);

		*ppStatementTail = pStatement;
		ppStatementTail = pStatement->GetPNext();

	} while (!pIdent->DimenIter(refList));

	return pStatementHead;
}

bool CHtfeDesign::IsRegisterName(const string &name)
{
	const char *pName = name.c_str();
	if (*pName++ != 'r')
		return false;

	while (isdigit(*pName)) pName++;

	return *pName == '_';
}

/////////////////////////////////////////////////////////////////////
//  Expression routines

bool CHtfeDesign::ParseConstExpr(CConstValue &value)
{
	// Parse a const token expression and return the value
	vector<CConstValue> operandStack;
	vector<EToken> operatorStack;
	operatorStack.push_back(tk_exprBegin);

	EToken tk;
	for (;;) {
		// first parse an operand
		tk = GetToken();
		// pre-operand unary operators
		if (tk == tk_minus || tk == tk_plus || tk == tk_tilda || tk == tk_bang) {

			if (tk == tk_minus)
				tk = tk_unaryMinus;
			else if (tk == tk_plus)
				tk = tk_unaryPlus;

			ParseEvalConstExpr(tk, operandStack, operatorStack);

			tk = GetNextToken(); // unary operators
		}

		if (tk == tk_num_int || tk == tk_num_hex || tk == tk_num_string) {

			operandStack.push_back(GetConstValue());
			GetNextToken();

		} else if (GetToken() == tk_identifier && GetString() == "true") {

			CConstValue value = 1;
			operandStack.push_back(value);
			GetNextToken();

		} else if (GetToken() == tk_identifier && GetString() == "false") {

			CConstValue value = 0;
			operandStack.push_back(value);
			GetNextToken();

		} else if (GetToken() == tk_lparen) {

			GetNextToken();

			CConstValue value;
			if (!ParseConstExpr(value))
				return false;

			operandStack.push_back(value);

			if (GetToken() != tk_rparen) {
				ParseMsg(PARSE_ERROR, "Expected a )");
				return false;
			}

			GetNextToken();
		} else 
			return false;

		switch (GetToken()) {
		case tk_rparen:
		default:

			ParseEvalConstExpr(tk_exprEnd, operandStack, operatorStack);

			if (operandStack.size() != 1) {
				ParseMsg(PARSE_FATAL, "Evaluation error, operandStack");
				return false;
			} else if (operatorStack.size() != 0) {
				ParseMsg(PARSE_FATAL, "Evaluation error, operatorStack");
				return false;
			}

			value = operandStack.back();
			return true;

		case tk_plus:
		case tk_minus:
		case tk_slash:
		case tk_asterisk:
		case tk_percent:
		case tk_vbar:
		case tk_carot:
		case tk_ampersand:
		case tk_lessLess:
		case tk_greaterGreater:

			ParseEvalConstExpr(GetToken(), operandStack, operatorStack);

			GetNextToken();
			break;
		case tk_question:
			ParseEvalConstExpr(GetToken(), operandStack, operatorStack);
			GetNextToken();

			CConstValue value;
			if (!ParseConstExpr(value))
				return false;

			operandStack.push_back(value);

			if (GetToken() != tk_colon) {
				ParseMsg(PARSE_ERROR, "Expected a :");
				return false;
			}

			GetNextToken();
			break;
		}
	}
}

void CHtfeDesign::ParseEvalConstExpr(EToken tk, vector<CConstValue> &operandStack, vector<EToken> &operatorStack)
{
	CConstValue op1, op2, op3, rslt;

	for (;;) {
		int tkPrec = GetTokenPrec(tk);
		EToken stackTk = operatorStack.back();
		int stackPrec = GetTokenPrec(stackTk);

		if (tkPrec > stackPrec || tkPrec == stackPrec && IsTokenAssocLR(tk)) {
			operatorStack.pop_back();

			if (stackTk == tk_exprBegin) {
				Assert(operandStack.size() == 1 && operatorStack.size() == 0);
				return;

			} else if (stackTk == tk_unaryPlus || stackTk == tk_unaryMinus || stackTk == tk_tilda) {

				// get operand off stack
				op1 = operandStack.back();
				operandStack.pop_back();

				switch (stackTk) {
				case tk_unaryPlus:	rslt = op1; break;
				case tk_unaryMinus:	rslt = - op1; break;
				case tk_tilda:		rslt = ~ op1; break;
				default: Assert(0); // only valid operators should be seen here
				}

				operandStack.push_back(rslt);

			}
			else if (stackTk == tk_question) {

				// turnary operator ?:
				Assert(operandStack.size() >= 3);
				op3 = operandStack.back();
				operandStack.pop_back();
				op2 = operandStack.back();
				operandStack.pop_back();
				op1 = operandStack.back();
				operandStack.pop_back();

				rslt = op1 != 0 ? op2 : op3;

				operandStack.push_back(rslt);
			}
			else if (stackTk != tk_question) {

				// binary operators
				Assert(operandStack.size() >= 2);
				op2 = operandStack.back();
				operandStack.pop_back();
				op1 = operandStack.back();
				operandStack.pop_back();

				switch (stackTk) {
				case tk_plus:			rslt = op1 + op2; break;
				case tk_minus:			rslt = op1 - op2; break;
				case tk_asterisk:		rslt = op1 * op2; break;
				case tk_slash:			rslt = op1 / op2; break;
				case tk_percent:		rslt = op1 % op2; break;
				case tk_vbar:			rslt = op1 | op2; break;
				case tk_carot:			rslt = op1 ^ op2; break;
				case tk_ampersand:		rslt = op1 & op2; break;
				case tk_lessLess:		rslt = op1 << op2; break;
				case tk_greaterGreater:	rslt = op1 >> op2; break;
				default: Assert(0); // only valid operators should be seen here
				}

				operandStack.push_back(rslt);
			}

		} else {
			// push operator on stack
			operatorStack.push_back(tk);
			break;
		}
	}
}

struct CFuncConv {
	CHtfeIdent * m_pFunc;
	vector<CHtfeIdent *> m_pArgConvList;
};

CHtfeOperand *g_pErrOp = 0;

CHtfeOperand * CHtfeDesign::ParseExpression(CHtfeIdent *pHier,
	bool bCommaAsSeparator, bool bFoldConstants, bool bAllowOpEqual)
{
	if (g_pErrOp == 0) {
		// if a parse error occurs, return g_pErrOp to avoid seg faults
		g_pErrOp = HandleNewOperand();
		g_pErrOp->InitAsConstant(GetLineInfo(), CConstValue(0));
		g_pErrOp->SetIsErrOp();   // set flag to prohibit deletion
	}

	CHtfeIdent *pIdent = 0;
	CHtfeOperand *pOp1;
	CHtfeOperand *pOperand;
	CHtfeOperand * pOpObj = 0;
	vector<CHtfeOperand *> argsList;

	vector<CHtfeOperand *> operandStack;
	vector<EToken> operatorStack;
	operatorStack.push_back(tk_exprBegin);

#ifdef WIN32
	if (GetLineInfo().m_lineNum == 37)
		bool stop = true;
#endif

	EToken tk;
	for (;;) {
		// first parse an operand
		tk = GetToken();
		// pre-operand unary operators
		if (tk == tk_minus || tk == tk_plus || tk == tk_tilda || tk == tk_bang
			|| tk == tk_plusPlus || tk == tk_minusMinus) {

				switch (tk) {
				case tk_minus: tk = tk_unaryMinus; break;
				case tk_plus: tk = tk_unaryPlus; break;
				case tk_plusPlus: tk = tk_preInc; break;
				case tk_minusMinus: tk = tk_preDec; break;
				default: break;
				}

				ParseEvaluateExpression(pHier, tk, operandStack, operatorStack);

				tk = GetNextToken(); // unary operators

				continue;
		}

		if (tk == tk_ampersand)
			ParseMsg(PARSE_FATAL, "address of operator is not supported");

		if (tk == tk_asterisk)
			ParseMsg(PARSE_FATAL, "address de-referencing is not supported");

		// SC_MODULE allocator
		if (tk == tk_new) {
			if (g_pHtfeArgs->IsScHier()) {
				ParseEvaluateExpression(pHier, tk, operandStack, operatorStack);
				GetNextToken();
			} else
				ParseMsg(PARSE_FATAL, "new operator is not supported\n");
		}

		if (tk == tk_delete)
			ParseMsg(PARSE_FATAL, "delete operator is not supported\n");

		if (tk == tk_num_int || tk == tk_num_hex || tk == tk_num_string) {

			pOperand = HandleNewOperand();
			pOperand->InitAsConstant(GetLineInfo(), GetConstValue());

			operandStack.push_back(pOperand);
			GetNextToken();

		} else if (GetToken() == tk_string) {

			ParseMsg(PARSE_ERROR, "character string constants are not supported");

			pOperand = HandleNewOperand();
			pOperand->InitAsString(GetLineInfo(), GetString());

			operandStack.push_back(pOperand);
			GetNextToken();

		} else if (GetToken() == tk_identifier) {

			if (GetString() == "typeid")
				ParseMsg(PARSE_FATAL, "typeid expressions are not suported");
			else if (GetString() == "sizeof") {
				uint64_t bytes;
				if (!ParseSizeofFunction(pHier, bytes))
					return g_pErrOp;

				pOperand = HandleNewOperand();
				pOperand->InitAsConstant(GetLineInfo(), CConstValue(bytes));
			} else {

				const CHtfeIdent *pHier2;
				pIdent = ParseVariableRef(pHier, pHier2, true, true, bAllowOpEqual);

				if (!pIdent) {
					string identName = GetString();
					CLineInfo lineInfo = GetLineInfo();
					if (GetNextToken() == tk_colon)
						ParseMsg(PARSE_FATAL, "statement labels are not supported");
					else
						ParseMsg(PARSE_ERROR, lineInfo, "undeclared variable %s", identName.c_str());
					return g_pErrOp;
				}

				if (pIdent->IsType()) {
					ParseMsg(PARSE_ERROR, "constructor style type conversion is not supported");
					return g_pErrOp;
				}

				vector<CHtfeOperand *> indexList;
				ParseVariableIndexing(pHier, pIdent, indexList);

				if (pIdent && pIdent->GetType() == 0)
					pIdent = 0;

				if (!pIdent) {
					pOperand = HandleNewOperand();
					pOperand->InitAsIdentifier(GetLineInfo(), 0);
				} else if (pIdent->GetType()->IsHtQueue())
					pOperand = ParseHtQueueExpr(pHier, pIdent, indexList);
				else if (pIdent->GetType()->IsHtMemory())
					pOperand = ParseHtMemoryExpr(pHier, pIdent, indexList);
				else {
					pOperand = HandleNewOperand();
					pOperand->InitAsIdentifier(GetLineInfo(), 
						(pIdent->IsFunction() || pHier2->IsStruct()) ? pIdent : pIdent->GetFlatIdent());

					pOperand->SetIndexList(indexList);
				}
			}

			if (pOperand == 0)
				return g_pErrOp;

			operandStack.push_back(pOperand);

		} else if (GetToken() == tk_lparen) {

			GetNextToken();
			CHtfeTypeAttrib typeAttrib;
			CHtfeIdent *pType = ParseTypeDecl(pHier, typeAttrib, false);

			if (typeAttrib.m_bIsStatic)
				ParseMsg(PARSE_ERROR, "static type specifier not supported");

			if (pType && GetToken() == tk_rparen) {
				// check for a cast operation
				GetNextToken();
				ParseEvaluateExpression(pHier, tk_typeCast, operandStack, operatorStack);

				pOperand = HandleNewOperand();
				pOperand->InitAsIdentifier(GetLineInfo(), pType);

				if (pType->IsStruct())
					ParseMsg(PARSE_ERROR, "cast operator to a struct/union type not supported");

				operandStack.push_back(pOperand);
				continue;
			}

			if (!(pOperand = ParseExpression(pHier, false, false)))
				return g_pErrOp;

			pOperand->SetIsParenExpr();
			operandStack.push_back(pOperand);

			if (GetToken() != tk_rparen) {
				ParseMsg(PARSE_ERROR, "Expected a ')'");
				SkipTo(tk_semicolon);
				return g_pErrOp;
			}

			GetNextToken();
		} else {
			ParseMsg(PARSE_ERROR, "expected an operand");
			SkipTo(tk_semicolon);
			return g_pErrOp;
		}

		CScSubField ** ppSubField = operandStack.back()->GetSubFieldP();
		while (*ppSubField)
			ppSubField = &(*ppSubField)->m_pNext;

		int fieldIndexCnt = 0;

		for (;;) {
			// post operand unary operators

			CHtfeIdent *pMember = 0;

			switch (GetToken()) {
			case tk_plusPlus:
			case tk_minusMinus:

				switch (GetToken()) {
				case tk_plusPlus: tk = tk_postInc; break;
				case tk_minusMinus: tk = tk_postDec; break;
				default: break;
				}

				ParseEvaluateExpression(pHier, tk, operandStack, operatorStack);
				GetNextToken();
				continue;

			case tk_period:
				if (GetNextToken() != tk_identifier) {
					ParseMsg(PARSE_ERROR, "expected a member name");
					SkipTo(tk_semicolon);
					return g_pErrOp;
				}

				pOp1 = operandStack.back();

				if (!pOp1->GetType()) {
					ParseMsg(PARSE_ERROR, "expected a class or struct identifier");
					SkipTo(tk_semicolon);
					return g_pErrOp;
				}

				pIdent = pOp1->GetType()->FindIdent(GetString());

				if (pIdent && pIdent->GetId() == CHtfeIdent::id_variable) {
					// struct field reference, determine field starting bit position and width
					CHtfeIdent *pMemberType = pOp1->GetType();

					if (pMemberType == 0 || !pMemberType->IsStruct()) {
						ParseMsg(PARSE_ERROR, "expected struct identifier");
						SkipTo(tk_semicolon);
						GetNextToken();
						return g_pErrOp;
					}

					HtfeOperandList_t indexList;
					string dimRefStr;
					size_t dimIdx = 0;
					while (GetNextToken() == tk_lbrack) {

						if (pIdent->GetDimenCnt() == dimIdx)
							break;	// bit index

						GetNextToken();
						if (pIdent->GetDimenCnt() > dimIdx) {
							CHtfeOperand *pOp = ParseExpression(pHier);
							CConstValue idxValue;
							if (!pOp->IsConstValue()) {
								//ParseMsg(PARSE_ERROR, "parsing '[ integer index ]', expected an constant expression");
								//SkipTo(tk_rbrack);
								idxValue = 0;
							} else
								idxValue = pOp->GetConstValue();

							// range check constant index. Due to for loop unrolling, value can be out of range
							//   if constant is out of range, set index to zero
							if (idxValue.GetSint64() < 0 || idxValue.GetSint64() >= pIdent->GetDimenList()[dimIdx])
								idxValue = 0;

							char buf[32];
							sprintf(buf, "[%d]", (int)idxValue.GetSint64());
							dimRefStr += buf;

							if (GetToken() != tk_rbrack) {
								ParseMsg(PARSE_ERROR, "parsing '[ index ]', expected ]");
								SkipTo(tk_rbrack);
							}

							indexList.push_back(pOp);
						}
						dimIdx += 1;
					}

					if (pIdent->GetDimenCnt() > dimIdx) {
						ParseMsg(PARSE_ERROR, "insufficient indecies for identifier");
						continue;
					}

					CHtfeIdent *pOrigIdent = pIdent;
					
					if (pIdent->GetDimenCnt() > 0) {
						CHtfeIdent *pElem = pIdent->FindIdent(pIdent->GetName() + dimRefStr);
						Assert(pElem);
						pIdent = pElem;
					}

					pOp1->SetType(pIdent->GetType());

					(*ppSubField) = new CScSubField(pOrigIdent, indexList, pOrigIdent->GetWidth());
					ppSubField = &(*ppSubField)->m_pNext;

					continue;
				}

				if (pIdent == 0 || pIdent->GetId() != CHtfeIdent::id_function) {
					ParseMsg(PARSE_ERROR, "'%s', expected a function or member name", GetString().c_str());
					SkipTo(tk_semicolon);
					return g_pErrOp;
				}

				// we found a method, push the tk_period operator
				ParseEvaluateExpression(pHier, tk_period, operandStack, operatorStack);

				pOperand = HandleNewOperand();
				pOperand->InitAsIdentifier(GetLineInfo(), pIdent);

				operandStack.push_back(pOperand);

				pOpObj = pOp1;

				GetNextToken();
				continue;

			case tk_lbrack:
				pOp1 = operandStack.back();

				if (pOp1->GetMember() == 0) {
					ParseMsg(PARSE_ERROR, "unexpected [");
					SkipTo(tk_semicolon);
					return g_pErrOp;
				}

				if (pOp1->GetMember()->GetPrevHier() && pOp1->GetMember()->GetPrevHier()->IsStruct() && pOp1->GetDimenCnt() == 0) {
					// member indexing
					HtfeOperandList_t indexList;
					string dimRefStr;
					size_t dimIdx = 0;

					while (GetToken() == tk_lbrack) {

						if (pIdent->GetDimenCnt() == dimIdx)
							break;	// bit index

						GetNextToken();
						if (pIdent->GetDimenCnt() > dimIdx) {
							CHtfeOperand *pOp = ParseExpression(pHier);
							CConstValue idxValue;
							if (!pOp->IsConstValue()) {
								//ParseMsg(PARSE_ERROR, "parsing '[ integer index ]', expected an constant expression");
								//SkipTo(tk_rbrack);
								idxValue = 0;
							} else
								idxValue = pOp->GetConstValue();

							// range check constant index. Due to for loop unrolling, value can be out of range
							//   if constant is out of range, set index to zero
							if (idxValue.GetSint64() < 0 || idxValue.GetSint64() >= pIdent->GetDimenList()[dimIdx])
								idxValue = 0;

							char buf[32];
							sprintf(buf, "[%d]", (int)idxValue.GetSint64());
							dimRefStr += buf;

							if (GetToken() != tk_rbrack) {
								ParseMsg(PARSE_ERROR, "parsing '[ index ]', expected ]");
								SkipTo(tk_rbrack);
							}

							indexList.push_back(pOp);
						}
						dimIdx += 1;
						fieldIndexCnt += 1;

						GetNextToken();
					}

					if (pIdent->GetDimenCnt() > dimIdx) {
						ParseMsg(PARSE_ERROR, "insufficient indecies for identifier");
						continue;
					}

					CHtfeIdent *pOrigIdent = pIdent;

					if (pIdent->GetDimenCnt() > 0) {
						CHtfeIdent *pElem = pIdent->FindIdent(pIdent->GetName() + dimRefStr);
						Assert(pElem);
						pIdent = pElem;
					}

					pOp1->SetType(pIdent->GetType());

					(*ppSubField) = new CScSubField(pOrigIdent, indexList, pOrigIdent->GetWidth());
					ppSubField = &(*ppSubField)->m_pNext;

				} else {
					// bit indexed variable

					GetNextToken();

					CHtfeOperand *pOp = ParseExpression(pHier);

					if (GetToken() != tk_rbrack)
						ParseMsg(PARSE_ERROR, "expected a ]");

					GetNextToken();

					HtfeOperandList_t indexList(1, pOp);
					(*ppSubField) = new CScSubField(0, indexList, 1, true);
					ppSubField = &(*ppSubField)->m_pNext;

					pOp1->SetType(m_pUInt64Type);
				}
				continue;

			case tk_lparen:

				// function call
				ParseEvaluateExpression(pHier, GetToken(), operandStack, operatorStack);

				// multiple call types handled
				// 1. user defined function: rslt = func(,,,)
				// 2. build in read/write: rslt = var.read() or var.write() = expr
				// 3. range function: var.range(idx1, idx0)
				// 4. overloaded range: var(idx1, idx0)

				// parse args
				argsList.clear();
				if (GetNextToken() != tk_rparen) {
					for(;;) {

						argsList.push_back(ParseExpression(pHier, true));

						if (GetToken() != tk_comma)
							break;

						GetNextToken();
					}
				}

				if (GetToken() != tk_rparen) {
					ParseMsg(PARSE_ERROR, "syntax error, expected a ')'");
					SkipTo(tk_semicolon);
					return g_pErrOp;
				}

				GetNextToken();	

				// now find function that matches arguments
				pOp1 = operandStack.back();

				if (pOp1->IsFunction() && pOp1->GetMember()->GetName() == "range" &&
					operatorStack[operatorStack.size()-2] == tk_period)
				{
					// pop range off stack
					operandStack.pop_back();
					pOp1 = operandStack.back();

					// pop period off operator stack
					operatorStack.pop_back();
				}

				CHtfeIdent *pMemberType;
				if (pOp1->GetOperator() == tk_typeCast) {
					pIdent = pOp1->GetOperand1()->GetMember();
					pMemberType = 0;
				} else {
					pIdent = pOp1->GetMember();
					pMemberType = pOp1->GetType();
				}

				// adjust stack
				operatorStack.pop_back(); // delete tk_lparen

				if (pIdent && pIdent->GetId() != CHtfeIdent::id_function) {
					// 4. overloaded range: var(idx1, idx0)
					// pOp1 has var, argsList has idx0 and idx1
					if (pIdent->GetId() == CHtfeIdent::id_variable && pMemberType->IsParenOverloaded()) {
						pMember = pOp1->GetMember();
						pIdent = pMemberType->GetParenOperator();
					} else if (pIdent->IsType() && pIdent->IsParenOverloaded()) {
						pMember = pOp1->GetOperand2()->GetMember();
						pIdent = pIdent->GetParenOperator();
					} else if (pIdent->IsType() && pIdent->GetId() == CHtfeIdent::id_class) {
						if (argsList.size() != 1 || argsList[0]->GetString().size() == 0)
							ParseMsg(PARSE_ERROR, "expected instance name for new module");
						else
							pIdent->SetInstanceName(argsList[0]->GetString());
					} else {
						ParseMsg(PARSE_ERROR, "term does not evaluate to a function");
						SkipTo(tk_semicolon);
						return g_pErrOp;
					}
				} else {
					pMember = pOp1->GetMember();
				}

				if (!pIdent) {
					ParseMsg(PARSE_ERROR, "function not declared");
					SkipTo(tk_semicolon);
					return g_pErrOp;
				}

				// check if a function exists with matching parameters
				if (pIdent->GetId() != CHtfeIdent::id_class) {

					vector<CFuncConv> convList;

					int minConvCnt = 0x10000;
					for (CHtfeIdent * pFunc = pIdent; pFunc; pFunc = pFunc->GetNextOverloadedFunc()) {

						int minParamCnt = 0;
						for (int paramId = 0; paramId < pFunc->GetParamCnt() && !pFunc->ParamHasDefaultValue(paramId); paramId += 1)
							minParamCnt += 1;

						if ((int)argsList.size() < minParamCnt || (int)argsList.size() > pFunc->GetParamCnt())
							continue;

						int convCnt = 0;
						bool bNoMatch = false;

						CFuncConv funcConv;
						funcConv.m_pFunc = pFunc;

						// now check argument for user defined conversions
						for (size_t i = 0; i < argsList.size(); i += 1) {

							CHtfeOperatorIter paramIter(pFunc->GetParamType(i));
							CHtfeOperatorIter argIter(argsList[i]->GetType());

							int matchCnt = 0;
							for (argIter.Begin(); !argIter.End(); argIter++) {

								if (argIter.IsOverloadedOperator()) continue;

								// check if op1Conv and op2Conv work for operation
								if (paramIter.GetType()->GetId() == CHtfeIdent::id_intType && argIter.GetType()->GetId() == CHtfeIdent::id_intType ||
									paramIter.GetType() == argIter.GetType()) {

									convCnt += argIter.GetConvCnt();
									funcConv.m_pArgConvList.push_back(argIter.GetMethod());
									matchCnt += 1;
								}
							}

							if (matchCnt != 1) {
								bNoMatch = true;
								break;
							}
						}

						if (bNoMatch) continue;
						if (convCnt > minConvCnt) continue;

						if (convCnt < minConvCnt)
							convList.clear();

						convList.push_back(funcConv);
						minConvCnt = convCnt;
					}

					if (minConvCnt == 0x10000) {
						ParseMsg(PARSE_ERROR, "no function found that matches arguments");
						SkipTo(tk_semicolon);
						return g_pErrOp;
					} else if (convList.size() > 1) {
						ParseMsg(PARSE_ERROR, "multiple functions found that match arguments");
						SkipTo(tk_semicolon);
						return g_pErrOp;
					} else {
						// found a unique match, apply argument conversions
						CHtfeIdent * pFunc = convList[0].m_pFunc;

						for (size_t i = 0; i < argsList.size(); i += 1) {

							if (convList[0].m_pArgConvList[i] == 0) continue;

							CHtfeOperand *pConvMethodOp = HandleNewOperand();
							pConvMethodOp->InitAsIdentifier(GetLineInfo(), convList[0].m_pArgConvList[i]);
							pConvMethodOp->SetType(convList[0].m_pArgConvList[i]->GetType());

							CHtfeOperand *pConvRslt = HandleNewOperand();
							pConvRslt->InitAsOperator(GetLineInfo(), tk_period, argsList[i], pConvMethodOp);
							pConvRslt->SetType(convList[0].m_pArgConvList[i]->GetType());

							argsList[i] = pConvRslt;
						}

						for (int paramId = (int)argsList.size(); paramId < pFunc->GetParamCnt(); paramId += 1) {
							CHtfeOperand * pOp = HandleNewOperand();
							pOp->InitAsConstant(GetLineInfo(), pIdent->GetParamDefaultValue(paramId));

							argsList.push_back(pOp);
						}
					}
				}

				if (pIdent->GetName() == m_rangeString || pIdent->GetName() == m_operatorParenString) {
					// 3. range function: var.range(idx1, idx0)
					// pOp1 has var.range, argsList has idx0 and idx1

					pOp1->SetMember(pMember);

					int newLow = 0;
					int newHigh = 0;
					CConstValue lowValue, highValue;

					if (!EvalConstantExpr(argsList[0], highValue, true) || !EvalConstantExpr(argsList[1], lowValue, true)) {
						ParseMsg(PARSE_ERROR, "expected integer bit range selection values");
					} else {
						newLow = (int)lowValue.GetSint64();
						newHigh = (int)highValue.GetSint64();
					}

					HtfeOperandList_t indexList(1, argsList[1]);
					(*ppSubField) = new CScSubField(0, indexList, newHigh - newLow + 1);
					ppSubField = &(*ppSubField)->m_pNext;

				} else {

					if (pIdent->GetName() == "read" || pIdent->GetName() == "write") {

						// 2. built in read/write: rslt = var.read() or var.write() = expr
						operandStack.pop_back();    // delete read/write function
						operatorStack.pop_back();   // delete tk_period
						//continue;

						// pOp1 has var.read
						operandStack.back()->SetType(pOpObj->GetType()->GetType());
						operandStack.back()->SetMember(pOpObj->GetMember());

						continue;

					} else if (pIdent->GetName() == "pos" || pIdent->GetName() == "neg") {

						// built in pos/neg functions, used in sensitive list of CTOR
						if (!pHier->IsModule())
							ParseMsg(PARSE_ERROR, "pos/neg functions are only allowed in sensitivity list of CTOR");

						pOp1->SetType(pOpObj->GetType()->GetType());
						//pOp1->SetMember(pOp1->GetObject());
						//pOp1->SetObject(0);
						pOpObj->GetMember()->SetIsClock();

						if (pIdent->GetName() == "pos")
							pOpObj->GetMember()->SetSensitiveType(tk_sensitive_pos);
						else
							pOpObj->GetMember()->SetSensitiveType(tk_sensitive_neg);

						continue;

					} else {
						// 1. user defined function: rslt = func(,,,)
						// pOp1 has function, argsList has arguments

						pOp1->SetParamList(argsList);
						pIdent->AddCaller(pHier);

						ppSubField = operandStack.back()->GetSubFieldP();
					}
				}

				pHier->AppendFunctionCall(pIdent);

				continue;
			default:
				break;
			}

			pOp1 = operandStack.back();
			if (bCommaAsSeparator) {
				if (pOp1->GetDimenCnt() != 0 && pOp1->GetDimenCnt() != pOp1->GetMember()->GetDimenCnt())
					ParseMsg(PARSE_ERROR, "Parameter list variables must be fully indexed or no indexing");
			} else {
				//if (pOp1->GetMember() && (pOp1->GetDimenCnt() + fieldIndexCnt) != pOp1->GetMember()->GetDimenCnt())
				//    ParseMsg(PARSE_ERROR, "Array dimension / index count mismatch");
			}

			break;
		}

		pOpObj = 0;

		EToken my_tk = GetToken();
		string my_s = GetTokenString().c_str();

		switch (my_tk) {
			//switch (GetToken()) {
		case tk_comma:	// concatenation or separater?
			if (!bCommaAsSeparator) {
				ParseEvaluateExpression(pHier, GetToken(), operandStack, operatorStack);
				GetNextToken();
				break;
			}
			// fall into next section
		case tk_semicolon:
		case tk_rparen:
		case tk_rbrack:
		case tk_rbrace:
		case tk_colon:

			ParseEvaluateExpression(pHier, tk_exprEnd, operandStack, operatorStack);

			if (operandStack.size() != 1)
				ParseMsg(PARSE_ERROR, "Evaluation error, operandStack");
			else if (operatorStack.size() != 0)
				ParseMsg(PARSE_ERROR, "Evaluation error, operatorStack");

			if (bFoldConstants) {
				SetOpWidthAndSign(operandStack.back());
				FoldConstantExpr(operandStack.back());
			}

			CheckForUnsafeExpression(operandStack.back());

			return operandStack.back();

		case tk_ampersandEqual:
		case tk_vbarEqual:
		case tk_minusEqual:
		case tk_lessLessEqual:
		case tk_greaterGreaterEqual:
		case tk_carotEqual:
		case tk_asteriskEqual:
		case tk_slashEqual:
		case tk_percentEqual:
		case tk_plusEqual:
		case tk_equal:
			if (!bAllowOpEqual)
				ParseMsg(PARSE_ERROR, "unexpected %s", GetTokenString().c_str());
			bAllowOpEqual = false;
		case tk_plus:
		case tk_minus:
		case tk_slash:
		case tk_asterisk:
		case tk_percent:
		case tk_vbar:
		case tk_carot:
		case tk_ampersand:
		case tk_equalEqual:
		case tk_vbarVbar:
		case tk_ampersandAmpersand:
		case tk_bangEqual:
		case tk_less:
		case tk_greater:
		case tk_greaterEqual:
		case tk_lessEqual:
		case tk_lessLess:
		case tk_greaterGreater:

			ParseEvaluateExpression(pHier, GetToken(), operandStack, operatorStack);

			GetNextToken();
			break;
		case tk_period:

			ParseEvaluateExpression(pHier, GetToken(), operandStack, operatorStack);

			GetNextToken();
			break;

		case tk_question:

			ParseEvaluateExpression(pHier, GetToken(), operandStack, operatorStack);

			GetNextToken();

			operandStack.push_back(ParseExpression(pHier, false, false));

			if (GetToken() != tk_colon) {
				ParseMsg(PARSE_ERROR, "expected a ':'");
				SkipTo(tk_semicolon);
				return g_pErrOp;
			}

			GetNextToken();
			break;

		case tk_minusGreater:
			ParseMsg(PARSE_ERROR, "unexpected operator '->'");
			SkipTo(tk_semicolon);
			return g_pErrOp;

		default:
			ParseMsg(PARSE_ERROR, "Expected an operator, near %s", GetTokenString().c_str());
			SkipTo(tk_semicolon);
			return g_pErrOp;
		}
	}
}

void CHtfeDesign::ParseEvaluateExpression(CHtfeIdent *pHier, EToken tk, vector<CHtfeOperand *> &operandStack,
	vector<EToken> &operatorStack)
{
	CHtfeOperand *pOp1, *pOp2, *pOp3, *pRslt;
	bool bIsEqual;

	for (;;) {
		int tkPrec = GetTokenPrec(tk);
		EToken stackTk = operatorStack.back();
		int stackPrec = GetTokenPrec(stackTk);

		if (tkPrec > stackPrec || tkPrec == stackPrec && IsTokenAssocLR(tk)) {
			operatorStack.pop_back();

			bIsEqual = false;
			if (stackTk == tk_exprBegin) {
				Assert(operandStack.size() == 1 && operatorStack.size() == 0);
				pOp1 = operandStack.back();

				if (pOp1->GetMember()) {
					if (pOp1->GetMember()->IsVariable())
						pOp1->GetMember()->AddReader(pHier);
					else if (pOp1->GetMember()->IsFunction())
						pOp1->GetMember()->AddCaller(pHier);
				}

				return;
			} else switch (stackTk) {
			case tk_unaryPlus:
			case tk_unaryMinus:
			case tk_preInc:
			case tk_preDec:
			case tk_postInc:
			case tk_postDec:
			case tk_tilda:
			case tk_bang:
			case tk_indirection:
			case tk_addressOf:
			case tk_new:
			{
				// get operand off stack
				pOp1 = operandStack.back();
				operandStack.pop_back();

				// check for bit indexed identifier
				bool bIsBitIndexed = false;
				if (pOp1->IsVariable()) {
					CScSubField * pSubField = pOp1->GetSubField();
					while (pSubField && pSubField->m_pNext)
						pSubField = pSubField->m_pNext;
					if (pSubField && pSubField->m_bBitIndex)
						bIsBitIndexed = true;
				}

				pRslt = HandleNewOperand();
				pRslt->InitAsOperator(GetLineInfo(), (bIsBitIndexed && stackTk == tk_tilda) ? tk_bang : stackTk, pOp1);
				pRslt->SetType(pOp1->GetType());

				operandStack.push_back(pRslt);

				if (pOp1->GetMember() && pOp1->GetMember()->IsVariable())
					pOp1->GetMember()->AddReader(pHier);
				break;
			}
			case tk_equal:
				bIsEqual = true;
				// fall into default
			default:
				{
					// binary operators
					Assert(operandStack.size() >= 2);
					pOp2 = operandStack.back();
					operandStack.pop_back();
					pOp1 = operandStack.back();
					operandStack.pop_back();

					if (!pOp1->GetType()->IsStruct() && (stackTk == tk_ampersandEqual || stackTk == tk_vbarEqual ||
						stackTk == tk_minusEqual || stackTk == tk_lessLessEqual || stackTk == tk_greaterGreaterEqual ||
						stackTk == tk_carotEqual || stackTk == tk_asteriskEqual || stackTk == tk_slashEqual ||
						stackTk == tk_percentEqual || stackTk == tk_plusEqual))
					{
						CHtfeOperand *pTmpOp1 = HandleNewOperand();
						pTmpOp1->InitAsIdentifier(GetLineInfo(), pOp1->GetMember());
						pTmpOp1->SetType(pOp1->GetType());
						pTmpOp1->SetIndexList(pOp1->GetIndexList());
						*pTmpOp1->GetSubFieldP() = pOp1->GetSubField();

						// link replicated operands so common index equations can be handled
						pOp1->SetLinkedOp(pTmpOp1);
						pTmpOp1->SetLinkedOp(pOp1);

						CHtfeLex::EToken tkOp;
						switch (stackTk) {
						case tk_ampersandEqual:			tkOp = tk_ampersand; break;
						case tk_vbarEqual:				tkOp = tk_vbar; break;
						case tk_minusEqual:				tkOp = tk_minus; break;
						case tk_lessLessEqual:			tkOp = tk_lessLess; break;
						case tk_greaterGreaterEqual:	tkOp = tk_greaterGreater; break;
						case tk_carotEqual:				tkOp = tk_carot; break;
						case tk_asteriskEqual:			tkOp = tk_asterisk; break;
						case tk_slashEqual:				tkOp = tk_slash; break;
						case tk_percentEqual:			tkOp = tk_percent; break;
						case tk_plusEqual:				tkOp = tk_plus; break;
						default: Assert(0);				tkOp = tk_eof; break;
						}

						CHtfeOperand *pTmp = HandleNewOperand();
						pTmp->InitAsOperator(GetLineInfo(), tkOp, pTmpOp1, pOp2);

						pOp2->SetIsParenExpr();

						pRslt = HandleNewOperand();
						pRslt->InitAsOperator(GetLineInfo(), tk_equal, pOp1, pTmp);
						operandStack.push_back(pRslt);

						if (pOp1->GetMember() && pOp1->GetMember()->IsVariable())
							pOp1->GetMember()->AddWriter(pHier);
						if (pOp1->GetMember() && pOp1->GetMember()->IsVariable())
							pOp1->GetMember()->AddReader(pHier);
						if (pOp2->GetMember() && pOp2->GetMember()->IsVariable())
							pOp2->GetMember()->AddReader(pHier);
						break;
					}

					if (stackTk == tk_comma && (
						(!pOp1->IsLeaf() && pOp1->GetOperator() != tk_comma && pOp1->GetOperator() != tk_typeCast) ||
						(pOp1->IsLeaf() && !pOp1->IsVariable()) ||
						(!pOp2->IsLeaf() && pOp2->GetOperator() != tk_comma && pOp2->GetOperator() != tk_typeCast) ||
						(pOp2->IsLeaf() && !pOp2->IsVariable())
						))
					{
						// concatenation operator and operands are not simple variable expressions
						ParseMsg(PARSE_ERROR, "concatenation operator with operands other than a simple variable or type cast not supported");
						ParseMsg(PARSE_INFO, "(using other than simple variables / type casts in systemc produces unexpected results)");
					}

					CHtfeIdent * pRsltType = 0;

					if (stackTk == tk_period) {
						pRsltType = pOp2->GetType();
					} else if (stackTk != tk_typeCast) {
						CHtfeIdent * pOp1Type = pOp1->GetType();
						CHtfeIdent * pOp2Type = pOp2->GetType();

						Assert(pOp1Type);
						Assert(pOp2Type);

						// iterate through op1 options
						int minConvCnt = 100;
						vector<CConvPair> convList;

						CHtfeOperatorIter memberIter(pHier, true);
						for (memberIter.Begin(); !memberIter.End(); memberIter++) {

							if (memberIter.IsOverloadedOperator() && memberIter.GetOverloadedOperator() != stackTk) continue;
							if (memberIter.IsOverloadedOperator() && memberIter.GetMethod()->GetParamCnt() != 2) continue;

							CHtfeOperatorIter op1Iter(pOp1Type);
							for (op1Iter.Begin(); !op1Iter.End(); op1Iter++) {

								if (op1Iter.IsConstructor()) continue;
								if (op1Iter.IsUserConversion() && CHtfeLex::IsEqualOperator(stackTk)) continue;
								if (memberIter.IsOverloadedOperator() && op1Iter.IsOverloadedOperator()) continue;
								if (op1Iter.IsOverloadedOperator() && op1Iter.GetOverloadedOperator() != stackTk) continue;

								// iterate through op2 options
								CHtfeOperatorIter op2Iter(pOp2Type);
								for (op2Iter.Begin(); !op2Iter.End(); op2Iter++) {

									if (op2Iter.IsConstructor()) continue;
									if (op2Iter.IsOverloadedOperator()) continue;

									// check if op1Conv and op2Conv work for operation
									if (memberIter.GetMethod() == 0 && op1Iter.GetType()->GetId() == CHtfeIdent::id_intType && op2Iter.GetType()->GetId() == CHtfeIdent::id_intType ||
										memberIter.GetMethod() == 0 && stackTk == tk_equal && op1Iter.GetType() == op2Iter.GetType() ||
										memberIter.GetMethod() == 0 && op1Iter.IsOverloadedOperator() && op1Iter.GetType() == op2Iter.GetType() ||
										memberIter.GetMethod() == 0 && stackTk == tk_equal && pOp2->IsConstValue() && pOp2->GetConstValue().IsZero() ||
										memberIter.IsOverloadedOperator() &&
										(memberIter.GetMethod()->GetParamType(0)->GetConvType()->GetId() == CHtfeIdent::id_intType &&
										op1Iter.GetType()->GetId() == CHtfeIdent::id_intType || memberIter.GetMethod()->GetParamType(0)->GetConvType() == op1Iter.GetType()) &&
										(memberIter.GetMethod()->GetParamType(1)->GetConvType()->GetId() == CHtfeIdent::id_intType &&
										op2Iter.GetType()->GetId() == CHtfeIdent::id_intType || memberIter.GetMethod()->GetParamType(1)->GetConvType() == op2Iter.GetType()))
									{
										int convCnt = memberIter.GetConvCnt() + op1Iter.GetConvCnt() + op2Iter.GetConvCnt();
										if (convCnt > minConvCnt) continue;
										if (convCnt < minConvCnt)
											convList.clear();
										convList.push_back(CConvPair(memberIter.GetMethod(), op1Iter.GetMethod(), op2Iter.GetMethod()));
										minConvCnt = convCnt;
									}
								}
							}
						}

						if (minConvCnt == 100) {
							ParseMsg(PARSE_ERROR, "Incompatible operands for expression");
							operandStack.push_back(g_pErrOp);
							break;
						} else if (minConvCnt > 0) {
							if (convList.size() == 1) {
								// apply conversions to expression
								if (convList[0].m_pOp2Conv) {
									if (convList[0].m_pOp2Conv->IsNullConvOperator()) {
										pOp2Type = convList[0].m_pOp2Conv->GetType();
									} else {
										CHtfeOperand *pConvMethodOp = HandleNewOperand();
										pConvMethodOp->InitAsIdentifier(GetLineInfo(), convList[0].m_pOp2Conv);
										pConvMethodOp->SetType(convList[0].m_pOp2Conv->GetType());

										CHtfeOperand *pConvRslt = HandleNewOperand();
										pConvRslt->InitAsOperator(GetLineInfo(), tk_period, pOp2, pConvMethodOp);
										pConvRslt->SetType(convList[0].m_pOp2Conv->GetType());
										pOp2 = pConvRslt;
										pOp2Type = pOp2->GetType();
									}
								}

								if (convList[0].m_pOp1Conv) {
									if (convList[0].m_pOp1Conv->IsNullConvOperator()) {
										pOp1Type = convList[0].m_pOp1Conv->GetType();
									} else {
										if (convList[0].m_pOp1Conv->IsUserConversion()) {
											CHtfeOperand *pConvMethodOp = HandleNewOperand();
											pConvMethodOp->InitAsIdentifier(GetLineInfo(), convList[0].m_pOp1Conv);
											pConvMethodOp->SetType(convList[0].m_pOp1Conv->GetType());

											CHtfeOperand *pConvRslt = HandleNewOperand();
											pConvRslt->InitAsOperator(GetLineInfo(), tk_period, pOp1, pConvMethodOp);
											pConvRslt->SetType(convList[0].m_pOp1Conv->GetType());
											pOp1 = pConvRslt;
											pOp1Type = pOp1->GetType();
										} else if (convList[0].m_pOp1Conv->IsOverloadedOperator()) {
											CHtfeOperand *pConvMethodOp = HandleNewOperand();
											pConvMethodOp->InitAsFunction(GetLineInfo(), convList[0].m_pOp1Conv);
											pConvMethodOp->SetType(convList[0].m_pOp1Conv->GetType());

											vector<CHtfeOperand *> paramList;
											paramList.push_back(pOp2);
											pConvMethodOp->SetParamList(paramList);

											CHtfeOperand *pConvRslt = HandleNewOperand();
											pConvRslt->InitAsOperator(GetLineInfo(), tk_period, pOp1, pConvMethodOp);
											pConvRslt->SetType(convList[0].m_pOp1Conv->GetType());

											operandStack.push_back(pConvRslt);
											break;
										} else
											Assert(0);
									}
								}
							} else {
								// ambiguous conversion solutions
								ParseMsg(PARSE_ERROR, "ambiguous expression for operator %s, possible solutions are:", 
									CHtfeLex::GetTokenString(stackTk).c_str());

								for (size_t i = 0; i < convList.size(); i += 1) {
									string memberConv;
									if (convList[i].m_pMemberConv) {
										memberConv = VA("operator%s ( ", CHtfeLex::GetTokenString(convList[i].m_pMemberConv->GetOverloadedOperator()).c_str());
										memberConv += VA(" %s, %s ), ", convList[i].m_pMemberConv->GetParamType(0)->GetName().c_str(),
											convList[i].m_pMemberConv->GetParamType(1)->GetName().c_str());
									}
									string op1Conv;
									if (convList[i].m_pOp1Conv == 0)
										op1Conv = pOp1Type->GetName();
									else if (convList[i].m_pOp1Conv->IsOverloadedOperator()) {
										op1Conv = "operator" + CHtfeLex::GetTokenString(convList[i].m_pOp1Conv->GetOverloadedOperator()) + " (" +
											convList[i].m_pOp1Conv->GetParamType(0)->GetName() + ")";
									} else if (convList[i].m_pOp1Conv->IsUserConversion()) {
										op1Conv = "operator " + convList[i].m_pOp1Conv->GetType()->GetName() + " ()";
									} else
										Assert(0);

									string op2Conv;
									if (convList[i].m_pOp2Conv == 0)
										op2Conv = pOp2Type->GetName();
									else if (convList[i].m_pOp2Conv->IsOverloadedOperator()) {
										op2Conv = "operator " + CHtfeLex::GetTokenString(convList[i].m_pOp2Conv->GetOverloadedOperator()) + " (" +
											convList[i].m_pOp2Conv->GetParamType(0)->GetName() + ")";
									} else if (convList[i].m_pOp2Conv->IsUserConversion()) {
										op2Conv = "operator " + convList[i].m_pOp2Conv->GetType()->GetName() + " ()";
									} else
										Assert(0);

									ParseMsg(PARSE_INFO, "%s%s, %s", memberConv.c_str(), op1Conv.c_str(), op2Conv.c_str());
								}
							}
						}

						pRsltType = pOp1Type;
					}

					pRslt = HandleNewOperand();
					pRslt->InitAsOperator(GetLineInfo(), stackTk, pOp1, pOp2);
					pRslt->SetType(pRsltType);
					operandStack.push_back(pRslt);

					if (pOp1->GetMember() && pOp1->GetMember()->IsReference() &&
						!pOp2->IsLeaf() && pOp2->GetOperator() == tk_new && pOp2->GetOperand1()->GetMember() &&
						pOp2->GetOperand1()->GetMember()->GetId() == CHtfeIdent::id_class)
						pOp1->GetMember()->SetInstanceName(pOp2->GetOperand1()->GetMember()->GetInstanceName());

					if (pOp1->GetMember() && pOp1->GetMember()->IsVariable()) {
						if (bIsEqual)
							pOp1->GetMember()->AddWriter(pHier);
						else
							pOp1->GetMember()->AddReader(pHier);
					}
					if (pOp2->GetMember() && pOp2->GetMember()->IsVariable())
						pOp2->GetMember()->AddReader(pHier);
				}
				break;

			case tk_question:

				// turnary operator ?:
				Assert(operandStack.size() >= 3);
				pOp3 = operandStack.back();
				operandStack.pop_back();
				pOp2 = operandStack.back();
				operandStack.pop_back();
				pOp1 = operandStack.back();
				operandStack.pop_back();

				CHtfeOperatorIter op2Iter(pOp2->GetType());
				CHtfeOperatorIter op3Iter(pOp3->GetType());
				if (!(op2Iter.GetType()->GetId() == CHtfeIdent::id_intType && op3Iter.GetType()->GetId() == CHtfeIdent::id_intType ||
					op2Iter.GetType() == op3Iter.GetType())) {
					ParseMsg(PARSE_ERROR, "Incompatible operands for expression");
				}

				pRslt = HandleNewOperand();
				pRslt->InitAsOperator(GetLineInfo(), stackTk, pOp1, pOp2, pOp3);
				pRslt->SetType(pOp2->GetType());
				operandStack.push_back(pRslt);

				if (pOp1->GetMember() && pOp1->GetMember()->IsVariable())
					pOp1->GetMember()->AddReader(pHier);
				if (pOp2->GetMember() && pOp2->GetMember()->IsVariable())
					pOp2->GetMember()->AddReader(pHier);
				if (pOp3->GetMember() && pOp3->GetMember()->IsVariable())
					pOp3->GetMember()->AddReader(pHier);
			}

		} else {
			// push operator on stack
			operatorStack.push_back(tk);
			break;
		}
	}
}

void CHtfeDesign::CheckForUnsafeExpression(CHtfeOperand *pExpr, EToken prevTk, bool bIsLeftOfEqual)
{
	// recursively decend expression tree looking for unsafe expressions
	// 1. ~bool

	Assert(pExpr != 0);

	static CLineInfo prevErrorLineInfo;

	if (pExpr->IsLeaf()) {
		if (pExpr->IsConstValue())
			return;

		CHtfeIdent *pIdent = pExpr->GetMember();

		if (pIdent->IsFunction() && !bIsLeftOfEqual) {

			for (int i = 0; i < pExpr->GetParamListCnt(); i += 1) {
				CheckForUnsafeExpression(pExpr->GetParamList()[i]);
			}
		}

		if (pIdent->IsVariable() || pIdent->IsFunction() && !bIsLeftOfEqual && pExpr->GetType() != GetVoidType()) {

			if (prevTk == tk_tilda && pIdent->GetType() == m_pBoolType && prevErrorLineInfo != pExpr->GetLineInfo()) {
				ParseMsg(PARSE_WARNING, pExpr->GetLineInfo(), "use of ~ operator with boolean operand is not recommended");
				prevErrorLineInfo = pExpr->GetLineInfo();
			}

			// find sub expression in variable indexes
			for (size_t i = 0; i < pExpr->GetIndexList().size(); i += 1) {
				CheckForUnsafeExpression(pExpr->GetIndexList()[i]);
			}

			for (CScSubField *pSubField = pExpr->GetSubField() ; pSubField; pSubField = pSubField->m_pNext) {
				for (size_t i = 0; i < pSubField->m_indexList.size(); i += 1) {
					CheckForUnsafeExpression(pSubField->m_indexList[i]);
				}
			}
		}

	} else if (pExpr->GetOperator() == tk_period) {

		CHtfeOperand *pOp1 = pExpr->GetOperand1();
		CHtfeOperand *pOp2 = pExpr->GetOperand2();
		Assert(pOp2->IsLeaf() && pOp2->GetMember()->IsFunction());

		CheckForUnsafeExpression(pOp1);
		CheckForUnsafeExpression(pOp2);

	} else {

		CHtfeOperand *pOp1 = pExpr->GetOperand1();
		CHtfeOperand *pOp2 = pExpr->GetOperand2();
		CHtfeOperand *pOp3 = pExpr->GetOperand3();

		EToken tk = pExpr->GetOperator();

		if (pOp1) CheckForUnsafeExpression(pOp1, tk, tk == tk_equal);
		if (pOp2) CheckForUnsafeExpression(pOp2, tk);
		if (pOp3) CheckForUnsafeExpression(pOp3, tk);

		switch (tk) {
		case tk_preInc:
		case tk_preDec:
		case tk_postInc:
		case tk_postDec:
			break;
		case tk_unaryPlus:
		case tk_unaryMinus:
		case tk_tilda:
			break;
		case tk_equal:
			break;
		case tk_typeCast:
			break;
		case tk_less:
		case tk_greater:
		case tk_lessEqual:
		case tk_greaterEqual:
		case tk_bang:	
		case tk_vbarVbar:
		case tk_ampersandAmpersand:
		case tk_equalEqual:
		case tk_bangEqual:
			if (prevTk == tk_tilda && prevErrorLineInfo != pExpr->GetLineInfo()) {
				ParseMsg(PARSE_WARNING, pExpr->GetLineInfo(), "use of ~ operator with boolean operand is not recommended");
				prevErrorLineInfo = pExpr->GetLineInfo();
			}
			break;
		case tk_plus:
		case tk_minus:
		case tk_vbar:
		case tk_ampersand:
		case tk_carot:
		case tk_asterisk:
		case tk_percent:
		case tk_slash:
			break;
		case tk_comma:
			break;
		case tk_lessLess:
		case tk_greaterGreater:
			break;
		case tk_question:
			break;
		default:
			Assert(0);
		}
	}
}

bool CHtfeDesign::ParseSizeofFunction(CHtfeIdent *pHier, uint64_t &bytes)
{
	if (GetNextToken() != tk_lparen) {
		ParseMsg(PARSE_ERROR, "expected a (, 'sizeof( type )'");
		return false;
	}
	GetNextToken();

	CHtfeIdent *pIdent = ParseTypeDecl(pHier, true);

	if (GetToken() != tk_rparen) {
		ParseMsg(PARSE_ERROR, "expected a ), 'sizeof( type )'");
		return false;
	}
	GetNextToken();

	if (pIdent == 0)
		return false;

	if (!pIdent->GetSizeof(bytes)) {
		ParseMsg(PARSE_ERROR, "unsupported sizeof type, '%s' (ony native C types are currently supported)", pIdent->GetName().c_str());
		return false;
	}

	return true;
}

void CHtfeDesign::ParseParamListDecl(CHtfeIdent *pHier, CHtfeIdent *pFunction)
{
	if (GetNextToken() == tk_rparen) {
		if (pFunction->IsParamListDeclared() && pFunction->GetParamCnt() > 0)
			ParseMsg(PARSE_ERROR, "function parameter list does not match previous declaration, overloading not supported");

		pFunction->SetIsParamListDeclared();
		GetNextToken();
		return;
	}
	bool bDefaultValue = false;
	for (int paramId = 0; ; paramId += 1) {

		if (GetToken() != tk_identifier && GetToken() != tk_const) {
			ParseMsg(PARSE_ERROR, "Expected a type specifier");
			SkipTo(tk_rparen, tk_semicolon);
			return;
		}

		CHtfeTypeAttrib typeAttrib;
		CHtfeIdent *pType = ParseTypeDecl(pHier, typeAttrib);

		if (typeAttrib.m_bIsStatic)
			ParseMsg(PARSE_ERROR, "static type specifier not supported");

		if (!pType) {
			SkipTo(tk_rparen, tk_semicolon);
			return;
		}

		if (GetToken() == tk_asterisk) {
			static bool bErrorMsg = false;
			if (bErrorMsg == false) {
				ParseMsg(PARSE_ERROR, "parameter declared as an address is not supported");
				bErrorMsg = true;
			}
			GetNextToken();
		}

		bool bIsReference = false;
		if (GetToken() == tk_ampersand) {
			bIsReference = true;
			GetNextToken();
		}

		if (pFunction->IsParamListDeclared()) {
			if (paramId >= pFunction->GetParamCnt()) {
				ParseMsg(PARSE_ERROR, "function parameter list does not match previous declaration, overloading not supported");
				SkipTo(tk_rparen, tk_semicolon);
				return;
			}

			// verify same type and update name
			if (pFunction->GetParamType(paramId) != pType  && !(pFunction->GetParamType(paramId)->GetId() == CHtfeIdent::id_intType && pType->GetId() == CHtfeIdent::id_intType &&
				pFunction->GetParamType(paramId)->GetWidth() == pType->GetWidth() && pFunction->GetParamType(paramId)->IsSigned() == pType->IsSigned()) ||
				pFunction->GetParamIsConst(paramId) != (typeAttrib.m_bIsConst || !bIsReference) ||
				pFunction->GetParamIsRef(paramId) != bIsReference)
				ParseMsg(PARSE_ERROR, "previously declared parameter type does not match parameter type, overloading not supported");

			if (GetToken() == tk_identifier) {
				pFunction->SetParamName(paramId, GetString());
				GetNextToken();
			} else
				pFunction->SetParamName(paramId, "");

			if (typeAttrib.m_bIsHtNoLoad)
				pFunction->SetIsParamNoload(paramId);

		} else {

			if (GetToken() == tk_identifier) {
				CHtfeParam tmp = CHtfeParam(pType, typeAttrib.m_bIsConst || !bIsReference, bIsReference, typeAttrib.m_bIsHtState || pType->IsScState(), GetString());
				pFunction->AppendParam(tmp);
				GetNextToken(); // parameter identifier
			} else {
				CHtfeParam tmp = CHtfeParam(pType, typeAttrib.m_bIsConst);
				pFunction->AppendParam(tmp);
			}
		}

		if (GetToken() == tk_equal) {
			if (pFunction->IsParamListDeclared()) {
				ParseMsg(PARSE_ERROR, "default value must be specified at function declaration");
				SkipTo(tk_rparen, tk_semicolon);
				GetNextToken();
				return;
			}

			GetNextToken();

			CConstValue value;
			if (!ParseConstExpr(value)) {
				ParseMsg(PARSE_ERROR, "expected default value to be a integer constant value");
				SkipTo(tk_rparen, tk_semicolon);
				GetNextToken();
				return;
			}

			if (!pType->IsSigned()) {
				if (value.IsNeg())
					ParseMsg(PARSE_ERROR, "expected positive integer value for unsigned parameter type");
				else
					value.ConvertToUnsigned(GetLineInfo());
			}

			if (pType->GetWidth() < value.GetMinWidth())
				ParseMsg(PARSE_ERROR, "default value is too large for declared parameter type");

			pFunction->SetParamDefaultValue(paramId, value);
			bDefaultValue = true;

		} else if (bDefaultValue)
			ParseMsg(PARSE_ERROR, "expected default value for all subsequent parameters");

		if (GetToken() == tk_comma) {
			GetNextToken();
			continue;
		}

		if (GetToken() != tk_rparen) {
			ParseMsg(PARSE_ERROR, "Expected a ')'");
			SkipTo(tk_rparen, tk_semicolon);
			return;
		}

		GetNextToken();

		pFunction->SetIsParamListDeclared();

		return;
	}
}

void CHtfeDesign::ParseDestructor(CHtfeIdent *pHier)
{
	// just ignore destructor
	SkipTo(tk_lbrace);
	SkipTo(tk_rbrace);
	GetNextToken();
}

void CHtfeDesign::ParseModuleCtor(CHtfeIdent *pHier)
{
	if (GetNextToken() != tk_lparen) {
		ParseMsg(PARSE_ERROR, "Expected a '('");
		SkipTo(tk_lbrace);
	} else if (GetNextToken() != tk_identifier || GetString() != pHier->GetName()) {
		ParseMsg(PARSE_ERROR, "Expected the SC_MODULE name");
		SkipTo(tk_lbrace);
	} else if (GetNextToken() != tk_rparen) {
		ParseMsg(PARSE_ERROR, "Expected ')'");
		SkipTo(tk_lbrace);
	} else if (GetNextToken() != tk_lbrace) {
		ParseMsg(PARSE_ERROR, "Expected '{'");
		SkipTo(tk_lbrace);
	}

	if (GetToken() != tk_lbrace)
		return;

	GetNextToken();

	CHtfeIdent *pFunction = pHier->InsertIdent(pHier->GetName(), false);
	pFunction->SetId(CHtfeIdent::id_function);
	pFunction->SetPrevHier(pHier);
	pFunction->SetIsLocal();
	pFunction->SetIsCtor();

	EToken tk;
	CHtfeIdent *pScMethod = 0;
	int forCnt = 0;
	for (;;) {
		switch (tk = GetToken()) {
		case tk_for:
			// for loop statement
			//   record tokens and then playback substituting control variable for constant value
			ParseForStatement();

			if (GetToken() == tk_lbrace) {
				forCnt += 1;
				GetNextToken();
			}
			break;
		case tk_SC_METHOD:
			pScMethod = ParseScMethod(pHier);
			break;
		case tk_sensitive_pos:
		case tk_sensitive_neg:
		case tk_sensitive:
			ParseSensitiveStatement(pHier, pScMethod, tk);
			break;
		case tk_rbrace:
			GetNextToken();
			if (forCnt > 0) {
				if (GetToken() == tk_lbrace) {
					GetNextToken();
					break;
				}
				forCnt -= 1;
				break;
			}
			return;
		case tk_eof:
			ParseMsg(PARSE_ERROR, "Unexpected end-of-file");
			return;
		default:
			ParseCtorStatement(pFunction);
			break;
		}
	}
}

CHtfeIdent * CHtfeDesign::ParseScMethod(CHtfeIdent *pHier)
{
	if (GetNextToken() != tk_lparen) {
		ParseMsg(PARSE_ERROR, "Expected '('");
		SkipTo(tk_semicolon);
		return 0;
	}
	CHtfeIdent *pScMethod = 0;
	if (GetNextToken() != tk_identifier || (pScMethod = pHier->FindIdent(GetString())) == 0) {
		ParseMsg(PARSE_ERROR, "Expected a method name");
		SkipTo(tk_semicolon);
		return pScMethod;
	}
	if (GetNextToken() != tk_rparen) {
		ParseMsg(PARSE_ERROR, "Expected a ')'");
		SkipTo(tk_semicolon);
		return pScMethod;
	}
	if (GetNextToken() != tk_semicolon) {
		ParseMsg(PARSE_ERROR, "Expected a ';'");
		SkipTo(tk_semicolon);
	}
	GetNextToken();

	pScMethod->SetId(CHtfeIdent::id_function);
	pScMethod->SetIsMethod();

	return pScMethod;
}

void CHtfeDesign::ParseSensitiveStatement(CHtfeIdent *pHier, CHtfeIdent *pScMethod, EToken token)
{
	if (pScMethod == 0)
		ParseMsg(PARSE_FATAL, 0);

	// add port to sensitivity list
	if (GetNextToken() != tk_lessLess) {
		ParseMsg(PARSE_ERROR, "Expected '<<'");
		SkipTo(tk_semicolon);
		return;
	}
	for (;;) {
		CHtfeOperand *pOperand;
		if (GetNextToken() != tk_identifier || !(pOperand = ParseExpression(pHier))) {
			ParseMsg(PARSE_ERROR, "Expected a port or signal");
			SkipTo(tk_semicolon);
			return;
		}

		// signal/port with or without pos/neg
		if (pOperand->GetOperator() == tk_period)
			pOperand = pOperand->GetOperand1();

		if (!pOperand->GetMember() || !pOperand->GetMember()->IsPort() && !pOperand->GetMember()->IsSignal()) {
			ParseMsg(PARSE_ERROR, "Expected a port or signal");
			SkipTo(tk_semicolon);
			return;
		}

		CHtfeIdent *pSensitiveIdent = pOperand->GetMember();

		pOperand->SetReadRef();

		// determine element index for identifier
		int elemIdx = CalcElemIdx(pSensitiveIdent, pOperand->GetIndexList());
		if (elemIdx < 0) {
			ParseMsg(PARSE_ERROR, "sensitive list signals must use constant index expressions");
			SkipTo(tk_semicolon);
			return;
		}

		pScMethod->InsertSensitive(pSensitiveIdent, elemIdx);

		if (token == tk_sensitive_pos || token == tk_sensitive_neg) {
			pSensitiveIdent->SetSensitiveType(token);

			if (pScMethod->GetClockIdent())
				ParseMsg(PARSE_ERROR, "multiple edge sensitive declarations for module");
			else
				pScMethod->SetClockIdent(pSensitiveIdent);
			pSensitiveIdent->SetIsClock();

			GetToken();

		} else if (token == tk_sensitive) {
			if (pSensitiveIdent->GetSensitiveType() == tk_sensitive_pos
				|| pSensitiveIdent->GetSensitiveType() == tk_sensitive_neg) {

					if (pScMethod->GetClockIdent())
						ParseMsg(PARSE_ERROR, "multiple edge sensitive declarations for module");
					else
						pScMethod->SetClockIdent(pSensitiveIdent);
					pSensitiveIdent->SetIsClock();

					break;
			} else
				pSensitiveIdent->SetSensitiveType(token);
		}

		if (GetToken() != tk_lessLess)
			break;
	}

	if (GetToken() != tk_semicolon) {
		ParseMsg(PARSE_ERROR, "Expected a ';'");
		SkipTo(tk_semicolon);
	}
	GetNextToken();
}

void CHtfeDesign::ParseScMain()
{
	SkipTo(tk_lbrace);

	if (g_pHtfeArgs->IsScMain())
		HandleScMainInit();

	// parse sc_main
	CHtfeIdent *pModule;
	CHtfeInstance *pInstance;
	for(;;) {
		switch(GetNextToken()) {
		case tk_identifier:
			if (GetString() == "sc_signal") {
				ParseScSignal();
				break;
			}

			if (GetString() == "sc_clock" || GetString() == "sc_clk") {
				ParseScClock();
				break;
			}

			if (GetString() == "sc_start") {
				ParseScStart();
				break;
			}

			// Check if identifier is a module name
			pModule = m_pTopHier->FindIdent(GetString());
			if (pModule && pModule->GetId() == CHtfeIdent::id_class && pModule->IsModule()) {
				ParseModuleInstance(pModule);
				continue;
			}

			// Check if identifier is an instance name
			pInstance = m_instanceTbl.Find(GetString());
			if (pInstance) {
				ParseInstancePort(pInstance);
				continue;
			}

			ParseMsg(PARSE_ERROR, "unknown statement type");
			SkipTo(tk_semicolon);
			break;
		case tk_return:
			SkipTo(tk_semicolon);
			break;
		case tk_rbrace:
			HandleGenerateScMain();

			GetNextToken();

			HandleScMainStart();
			return;
		default:
			ParseMsg(PARSE_ERROR, "unknown statement type");
			SkipTo(tk_semicolon);
			break;
		}
	}
}

void CHtfeDesign::ParseScSignal()
{
	// Parse signal
	if (GetNextToken() != tk_less || GetNextToken() != tk_identifier) {
		ParseMsg(PARSE_ERROR, "Expected a signal type");
		SkipTo(tk_semicolon);
		return;
	}

	CHtfeIdent *pType = m_pTopHier->FindIdent(GetString());
	if (!pType) {
		ParseMsg(PARSE_ERROR, "Unknown signal type");
		SkipTo(tk_semicolon);
		return;
	}

	int signalWidth = -1;
	if (pType->IsVariableWidth()) {
		if (GetNextToken() != tk_less || (GetNextToken() != tk_num_int &&
			GetToken() != tk_num_hex) || GetNextToken() != tk_greater) {
				ParseMsg(PARSE_ERROR, "Expected signal width");
				SkipTo(tk_semicolon);
				return;
		}

		signalWidth = atoi(GetString().c_str());

		// create unique name for this width
		string typeName = pType->GetName() + "<" + GetString() + "> ";
		pType = m_pTopHier->InsertType(typeName);
		pType->SetWidth(signalWidth);
	}

	if (GetNextToken() != tk_greater) {
		ParseMsg(PARSE_ERROR, "Expected '>'");
		SkipTo(tk_semicolon);
		return;
	}

	if (GetNextToken() != tk_identifier) {
		ParseMsg(PARSE_ERROR, "Expected a signal name");
		SkipTo(tk_semicolon);
		return;
	}

	CHtfeSignal *pSignal = m_signalTbl.Insert(GetString());
	if (pSignal->GetType() != 0) {
		ParseMsg(PARSE_ERROR, "Signal multiply defined, see below:");
		ParseMsg(PARSE_ERROR, pSignal->GetLineInfo(), "Original signal declaration");
	} else {
		pSignal->SetType(pType);
		pSignal->SetLineInfo(GetLineInfo());
	}

	if (GetNextToken() != tk_semicolon) {
		ParseMsg(PARSE_ERROR, "Expected a semicolon");
		SkipTo(tk_semicolon);
	}
}

void CHtfeDesign::ParseScSignal2(CHtfeIdent *pHier)
{
	if (GetNextToken() != tk_less) {
		ParseMsg(PARSE_ERROR, "Expected '<'");
		SkipTo(tk_semicolon);
		return;
	}

	GetNextToken();

	CHtfeTypeAttrib typeAttrib;
	CHtfeIdent *pBaseType = ParseTypeDecl(pHier, typeAttrib);

	if (typeAttrib.m_bIsStatic)
		ParseMsg(PARSE_ERROR, "static type specifier not supported");

	if (!pBaseType) {
		ParseMsg(PARSE_ERROR, "Unknown signal type");
		SkipTo(tk_semicolon);
		return;
	}

	if (GetToken() != tk_greater) {
		ParseMsg(PARSE_ERROR, "Expected '>'");
		SkipTo(tk_semicolon);
		return;
	}

	// create unique name for this width
	string signalName = "sc_signal";

	string typeName = signalName + "<" + pBaseType->GetName() + "> ";
	CHtfeIdent *pType = m_pTopHier->InsertType(typeName);
	pType->SetType(pBaseType);

	if (pType->GetId() == CHtfeIdent::id_new) {
		pType->SetId(CHtfeIdent::id_class);
		pType->SetWidth(pBaseType->GetWidth());

		// initialize builtin functions
		CHtfeIdent *pFunc = pType->InsertIdent(m_readString);
		pFunc->SetId(CHtfeIdent::id_function);
		pFunc->SetType(pBaseType);

		pFunc = pType->InsertIdent(m_writeString);
		pFunc->SetId(CHtfeIdent::id_function);
		pFunc->SetType(pType);
		CHtfeParam tmp = CHtfeParam(pBaseType, typeAttrib.m_bIsConst);
		pFunc->AppendParam(tmp);
	} 

	if (GetNextToken() != tk_identifier) {
		ParseMsg(PARSE_ERROR, "Expected a signal name");
		SkipTo(tk_semicolon);
		return;
	}

	CHtfeIdent *pSignal = pHier->InsertIdent(GetString());
	if (pSignal->GetId() != CHtfeIdent::id_new) {
		ParseMsg(PARSE_ERROR, "Signal multiple defined");
		SkipTo(tk_semicolon);
		return;
	}

	pSignal->SetId(CHtfeIdent::id_variable);
	pSignal->SetType(pType);
	pSignal->SetLineInfo(GetLineInfo());

	if (GetNextToken() != tk_semicolon) {
		ParseMsg(PARSE_ERROR, "Expected a semicolon");
		SkipTo(tk_semicolon);
		return;
	}
	GetNextToken();
}

void CHtfeDesign::ParseScClock()
{
	if (GetNextToken() != tk_identifier) {
		ParseMsg(PARSE_ERROR, "Expected a clock signal name");
		SkipTo(tk_semicolon);
		return;
	}

	CHtfeSignal *pSignal = m_signalTbl.Insert(GetString());
	if (pSignal->GetType() != 0)
		ParseMsg(PARSE_ERROR, "Clock previously used as non-clock signal");

	pSignal->SetIsClock(true);

	CHtfeIdent *pType = m_pTopHier->FindIdent("bool");
	if (pSignal->GetRefCnt() != 0 && pSignal->GetType() != pType)
		ParseMsg(PARSE_ERROR, "Clock signal type error");
	else
		pSignal->SetType(pType);

	CHtfeInstance *pInstance = new CHtfeInstance(pSignal->GetName());
	CHtfeInstancePort *pInstancePort = pInstance->InsertPort(pSignal->GetName());
	pInstancePort->SetInstance(pInstance);
	CHtfeIdent *pModulePort = CHtfeDesign::NewIdent();
	pModulePort->SetName(pSignal->GetName());
	pModulePort->SetLineInfo(GetLineInfo());
	pModulePort->SetId(CHtfeIdent::id_variable)->SetPortDir(port_out);
	pModulePort->SetType(m_pTopHier->FindIdent("bool"));
	pInstancePort->SetModulePort(pModulePort);

	pSignal->AddReference(pInstancePort);

	while (GetNextToken() != tk_semicolon && GetToken() != tk_eof)
		pSignal->AppendToken(GetToken(), GetString());
}

void CHtfeDesign::ParseScStart()
{
	if (m_scStart.size() != 0) {
		ParseMsg(PARSE_ERROR, "Multiple sc_start statements");
		return;
	}

	while (GetNextToken() != tk_semicolon && GetToken() != tk_eof) {
		EToken tk = GetToken();
		CToken l(tk, GetTokenString(tk).length() > 0 ? 
			GetTokenString(tk) : GetString());
		m_scStart.push_back(l);
	}
}

void CHtfeDesign::ParseModuleInstance(CHtfeIdent *pHier)
{
	if (GetNextToken() != tk_identifier) {
		ParseMsg(PARSE_ERROR, "Expected module instance name");
		SkipTo(tk_semicolon);
		GetNextToken();
		return;
	}

	CHtfeInstance *pInstance = m_instanceTbl.Insert(GetString());
	if (pInstance->IsInUse()) {
		ParseMsg(PARSE_ERROR, "Instance name previously used");

		// skip instance port list
		do {
			SkipTo(tk_semicolon);
		} while (GetNextToken() == tk_identifier && GetString() == pInstance->GetName());
		return;
	}
	pInstance->SetModule(pHier);
	pInstance->SetInUse(true);
	pInstance->SetLineInfo(GetLineInfo());
	SkipTo(tk_semicolon);
}

void CHtfeDesign::ParseInstancePort(CHtfeInstance *pInstance)
{
	if (GetNextToken() != tk_period || GetNextToken() != tk_identifier) {
		ParseMsg(PARSE_ERROR, "Instance port name expected");
		SkipTo(tk_semicolon);
		return;
	}

	CHtfeIdent *pModulePort = pInstance->GetModule()->FindIdent(GetString());
	if (!pModulePort || !pModulePort->IsPort()) {
		ParseMsg(PARSE_ERROR, "Undefined instance port name");
		SkipTo(tk_semicolon);
		return;
	}

	if (GetNextToken() != tk_lparen || GetNextToken() != tk_identifier) {
		ParseMsg(PARSE_ERROR, "Expected a signal name");
		SkipTo(tk_semicolon);
		return;
	}

	CHtfeSignal *pSignal = m_signalTbl.Find(GetString());
	if (!pSignal) {
		ParseMsg(PARSE_ERROR, "Undefined signal name");
		SkipTo(tk_semicolon);
		return;
	}

	if (pSignal->GetType() != pModulePort->GetType())
		ParseMsg(PARSE_ERROR, "Module port type does not match signal type");

	CHtfeInstancePort *pInstancePort = pInstance->InsertPort(pModulePort->GetName());
	pInstancePort->SetSignal(pSignal);
	pInstancePort->SetModulePort(pModulePort);
	pInstancePort->SetInstance(pInstance);

	pSignal->AddReference(pInstancePort);

	if (pSignal->IsClock() && (GetNextToken() != tk_period || GetNextToken() != tk_identifier ||
		GetString() != "signal" || GetNextToken() != tk_lparen || GetNextToken() != tk_rparen)) {
			ParseMsg(PARSE_ERROR, "Expected a .signal() for clock");
			SkipTo(tk_semicolon);
			return;
	}

	if (GetNextToken() != tk_rparen) {
		ParseMsg(PARSE_ERROR, "Expected ')'");
		SkipTo(tk_semicolon);
		return;
	}
	if (GetNextToken() != tk_semicolon) {
		ParseMsg(PARSE_ERROR, "Expected ';'");
		SkipTo(tk_semicolon);
	}
}

void CHtfeDesign::CheckMissingInstancePorts()
{
	CHtfeInstanceTblIter instanceIter(m_instanceTbl);
	for (instanceIter.Begin() ; !instanceIter.End(); instanceIter++) {

		// check for missing port names in instanciation list
		CHtfeIdent *pModule = instanceIter->GetModule();

		CHtfeIdentTblIter modulePortIter(pModule->GetIdentTbl());
		//CHtfeIdentTbl *pVariableTbl = pModule->GetIdentTbl();
		for (modulePortIter.Begin(); !modulePortIter.End() && !modulePortIter->IsPort();
			modulePortIter++); // skip non-port identifiers

		CHtfeInstancePortTblIter instancePortIter(instanceIter->GetInstancePortTbl());
		for (instancePortIter.Begin(); !modulePortIter.End() || !instancePortIter.End(); ) {

			int diff;
			string n1, n2;
			if (modulePortIter.End())
				diff = 1;
			else if (instancePortIter.End())
				diff = -1;
			else {
				diff = modulePortIter->GetName().compare(instancePortIter->GetName());
				n1 = modulePortIter->GetName();
				n2 = instancePortIter->GetName();
			}

			if (diff < 0) {
				// skipped module port
				if (g_pHtfeArgs->IsScCode())
					ParseMsg(PARSE_ERROR, instanceIter->GetLineInfo(),
						"Unconnected module port - %s", modulePortIter->GetName().c_str());
				else {
					// make connection for port
					string signalName;
					string portName = modulePortIter->GetName();
					//#if defined(_MSC_VER)
					if (portName.compare(0, 2, "i_") == 0)
						signalName = portName.c_str()+2;
					else if (portName.compare(0, 2, "o_") == 0)
						signalName = portName.c_str()+2;
					else if (portName.compare(0, 3, "io_") == 0)
						signalName = portName.c_str()+3;
					else
						signalName = portName;
					/*#else

					if (portName.compare("i_", 0, 2) == 0)
					signalName = portName.c_str()+2;
					else if (portName.compare("o_", 0, 2) == 0)
					signalName = portName.c_str()+2;
					else if (portName.compare("io_", 0, 3) == 0)
					signalName = portName.c_str()+3;
					else
					signalName = portName;

					#endif*/
					CHtfeSignal *pSignal = m_signalTbl.Insert(signalName);
					if (pSignal->GetType() && pSignal->GetType() != modulePortIter->GetType() &&
						pSignal->GetType() != modulePortIter->GetBaseType()) {
							ParseMsg(PARSE_ERROR, modulePortIter->GetLineInfo(), "Module port type error, see signal below:");
							ParseMsg(PARSE_ERROR, pSignal->GetLineInfo(), "Declaration of signal '%s'", pSignal->GetName().c_str());
					}

					CHtfeInstancePort *pInstancePort = instanceIter->InsertPort(modulePortIter->GetName());
					pInstancePort->SetSignal(pSignal);
					pInstancePort->SetModulePort(modulePortIter());
					pInstancePort->SetInstance(instanceIter());

					pSignal->AddReference(pInstancePort);
				}

				modulePortIter++;
				while (!modulePortIter.End() && !modulePortIter->IsPort())
					modulePortIter++; // skip non-port identifiers

			} else if (diff > 0) {
				// this should not occur
				instancePortIter++;
			} else {
				// port match
				modulePortIter++;
				while (!modulePortIter.End() && !modulePortIter->IsPort())
					modulePortIter++; // skip non-port identifiers

				instancePortIter++;
			}
		}
	}

	return;
}

void CHtfeDesign::CheckConsistentPortTypes()
{
	// check for different port type and signal type
	CHtfeSignalTblIter signalIter(m_signalTbl);
	for (signalIter.Begin() ; !signalIter.End(); signalIter++) {

		if (signalIter->GetRefCnt() == 0)
			continue;

		if (signalIter->GetOutRefCnt() == 0) {
			ParseMsg(PARSE_ERROR, "Signal without an output port");
			DumpPortList(signalIter());
		}

		if (signalIter->GetOutRefCnt() > 1) {
			ParseMsg(PARSE_ERROR, "Signal with multiple output ports");
			DumpPortList(signalIter());
		}

		if (signalIter->GetInRefCnt() == 0) {
			ParseMsg(PARSE_ERROR, "Signal without an input");
			DumpPortList(signalIter());
		}

		CHtfeIdent *pType = signalIter->GetType();
		if (pType == 0)
			pType = signalIter->GetRefList()[0]->GetModulePort()->GetBaseType();

		bool mismatch = false;
		for (int i = 0; i < signalIter->GetRefCnt() && !mismatch; i += 1) {
			CHtfeInstancePort *pInstancePort = signalIter->GetRefList()[i];

			mismatch |= (pInstancePort->GetModulePort()->GetType() != pType &&
				pInstancePort->GetModulePort()->GetBaseType() != pType);
		}

		if (mismatch) {
			if (signalIter->GetType())
				ParseMsg(PARSE_ERROR, signalIter->GetLineInfo(), "Signal/Port type mismatch, signal '%s', see reference list:", signalIter->GetName().c_str());
			else
				ParseMsg(PARSE_ERROR, "Signal/Port type mismatch, signal '%s', see reference list:", signalIter->GetName().c_str());

			DumpPortList(signalIter());
		}
	}

	return;
}

void CHtfeDesign::DumpPortList(CHtfeSignal *pSignal)
{
	for (int i = 0; i < pSignal->GetRefCnt(); i += 1) {
		CHtfeInstancePort *pInstancePort = pSignal->GetRefList()[i];
		ParseMsg(PARSE_ERROR, pInstancePort->GetModulePort()->GetLineInfo(),
			"Module %s, port %s, type %s",
			pInstancePort->GetInstance()->GetName().c_str(),
			pInstancePort->GetModulePort()->GetName().c_str(),
			pInstancePort->GetModulePort()->GetType()->GetName().c_str() );
	}
}

void CHtfeDesign::SkipTo(EToken token)
{
	for (int lvl = 0;;) {
		EToken tk = GetNextToken();
		if (tk == tk_eof)
			return;
		if (token == tk) {
			if (lvl == 0)
				return;
			else
				lvl -= 1;
		}
		if (token == tk_rbrace && tk == tk_lbrace)
			lvl += 1;
		if (token == tk_rparen && tk == tk_lparen)
			lvl += 1;
	}
}

void CHtfeDesign::SkipTo(EToken token1, EToken token2)
{
	for (int lvl = 0;;) {
		EToken tk = GetNextToken();
		if (tk == tk_eof)
			return;
		if (token1 == tk || token2 == tk) {
			if (lvl == 0)
				return;
			else
				lvl -= 1;
		}
		if ((token1 == tk_rbrace || token2 == tk_rbrace) && tk == tk_lbrace)
			lvl += 1;
	}
}

void CHtfeDesign::CopyTo(EToken token, string & str)
{
	str += GetTokenString();
	for (int lvl = 0;;) {
		EToken tk = GetNextToken();
		str += GetTokenString();
		if (tk == tk_eof)
			return;
		if (token == tk) {
			if (lvl == 0)
				return;
			else
				lvl -= 1;
		}
		if (token == tk_rbrace && tk == tk_lbrace)
			lvl += 1;
		if (token == tk_rparen && tk == tk_lparen)
			lvl += 1;
	}
}


#ifdef DUMP_TO_FILE
CHtvFile sFp;

void CHtfeDesign::DumpFunction(CHtfeIdent *pFunction)
{
	if (!sFp.IsOpen()) sFp.Open("stDump.txt", "w");

	sFp.Print("Function: %s\n", pFunction->GetName().c_str());

	DumpStatementList(pFunction->GetStatementList());
}

void CHtfeDesign::DumpStatementList(CHtfeStatement *pStatement)
{
	for ( ; pStatement; pStatement = pStatement->GetNext())
		DumpStatement(pStatement);
}

void CHtfeDesign::DumpStatement(CHtfeStatement *pStatement)
{
	sFp.IncIndentLevel();
	switch (pStatement->GetStType()) {
	case st_assign: DumpExpression(pStatement->GetExpr()); sFp.Print(";\n"); break;
	case st_if:
		sFp.Print("if ("); DumpExpression(pStatement->GetExpr()); sFp.Print(") {\n");
		DumpStatementList(pStatement->GetCompound1());
		if (pStatement->GetCompound2()) {
			sFp.Print("} else {\n");
			DumpStatementList(pStatement->GetCompound2());
		}
		sFp.Print("}\n");
		break;
	case st_switch:
		sFp.Print("switch ("); DumpExpression(pStatement->GetExpr()); sFp.Print(") {\n");
		DumpStatementList(pStatement->GetCompound1());
		break;
	case st_case:
		sFp.Print("case "); DumpExpression(pStatement->GetExpr()); sFp.Print(":\n");
		DumpStatementList(pStatement->GetCompound1());
		break;
	case st_default:
		sFp.Print("default:\n");
		DumpStatementList(pStatement->GetCompound1());
		break;
	case st_break:
		sFp.Print("break;\n");
		break;
	case st_return:
		sFp.Print("return "); DumpExpression(pStatement->GetExpr()); sFp.Print(";\n");
		break;
	default: Assert(0);
	}
	sFp.DecIndentLevel();
}

void CHtfeDesign::DumpExpression(CHtfeOperand *pExpr)
{
	if (pExpr == 0) {
		sFp.Print("(null)");
		return;
	}

	if (!pExpr->IsLeaf()) {
		switch (pExpr->GetOperator()) {
		case tk_bang:
		case tk_tilda:
		case tk_unaryMinus:
			sFp.Print(" %s", GetTokenString(pExpr->GetOperator()).c_str());
			DumpExpression(pExpr->GetOperand1());
			break;
		case tk_slash:
		case tk_comma:
		case tk_carot:
		case tk_greaterGreater:
		case tk_lessLess:
		case tk_asterisk:
		case tk_percent:
		case tk_plus:
		case tk_minus:
		case tk_vbar:
		case tk_ampersand:
		case tk_vbarVbar:
		case tk_ampersandAmpersand:
		case tk_equal:
		case tk_greaterEqual:
		case tk_lessEqual:
		case tk_equalEqual:
		case tk_greater:
		case tk_less:
		case tk_bangEqual:
			DumpExpression(pExpr->GetOperand1());
			sFp.Print(" %s ", GetTokenString(pExpr->GetOperator()).c_str());
			DumpExpression(pExpr->GetOperand2());
			break;
		case tk_typeCast:
			sFp.Print("(");
			DumpExpression(pExpr->GetOperand1());
			sFp.Print(") ");
			DumpExpression(pExpr->GetOperand2());
			break;
		case tk_question:
			DumpExpression(pExpr->GetOperand1());
			sFp.Print(" %s ", GetTokenString(pExpr->GetOperator()).c_str());
			DumpExpression(pExpr->GetOperand2());
			sFp.Print(" : ");
			DumpExpression(pExpr->GetOperand3());
			break;
		default:
			Assert(0);
		}
	} else {
		if (pExpr->IsScX())
			sFp.Print("'hx");
		else if (pExpr->IsConstValue())
			sFp.Print("%lld", pExpr->GetConstValue().GetSint64());
		else {

			sFp.Print("%s", pExpr->GetMember()->GetName().c_str());
			if (pExpr->IsSubFieldValid()) {
				int lowBit, width;
				GetSubFieldRange(pExpr, lowBit, width, false);
				sFp.Print("(%d,%d)", lowBit+width-1, lowBit);
			}
		}
	}
}

#endif
