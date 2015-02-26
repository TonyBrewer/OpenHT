/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "CnyHt.h"
#include "DsnInfo.h"

void
CDefineTable::Insert(string const &name, vector<string> const & paramList, string const &value,
bool bPreDefined, bool bHasParamList, string scope, string modName)
{
	// check for redundant defines
	// currently modules can define the same name with different values
	size_t i;
	for (i = 0; i < m_defineList.size(); i += 1) {
		if (m_defineList[i].m_name == name && m_defineList[i].m_scope == scope) {
			if (m_defineList[i].m_value != value)
				CPreProcess::ParseMsg(Error, "Redundant AddDefine name with different values (%s)", name.c_str());
			return;
		}
	}
	m_defineList.push_back(CDefine(name, paramList, value, bPreDefined, bHasParamList, scope, modName));
}

bool CDefineTable::FindStringValue(CLineInfo const &lineInfo, const char * &pValueStr, int &rtnValue, bool & bIsSigned)
{
	GetNextToken(pValueStr);

	return ParseExpression(lineInfo, pValueStr, rtnValue, bIsSigned);
}

void
CDefineTable::SkipSpace(const char *&pPos)
{
	while (*pPos == ' ' || *pPos == '\t')
		pPos += 1;
}

bool
CDefineTable::ParseExpression(CLineInfo const &lineInfo, const char * &pPos, int &rtnValue, bool &bRtnIsSigned)
{
	bool bIsSigned;
	bRtnIsSigned = true;
	SkipSpace(pPos);

	vector<int> operandStack;
	vector<int> operatorStack;
	operatorStack.push_back(eTkExprBegin);

	EToken tk;
	for (;;) {
		// first parse an operand
		tk = GetToken();
		if (tk == eTkError) return false;

		if (tk == eTkBang) {
			EvaluateExpression(tk, operandStack, operatorStack);
			tk = GetNextToken(pPos);
			if (tk == eTkError) return false;
		}

		if (tk == eTkNumInt || tk == eTkNumHex) {

			operandStack.push_back(GetTokenValue());
			bRtnIsSigned &= GetIsSigned();

		} else if (GetToken() == eTkIdent) {

			string strValue;
			if (!FindStringInTable(GetTokenString(), strValue)) {
				CPreProcess::ParseMsg(Error, lineInfo, "%s is undefined", GetTokenString().c_str());
				return false;
			}  else {
				// convert macro to an integer value
				const char *pValueStr = strValue.c_str();

				GetNextToken(pValueStr);

				if (!ParseExpression(lineInfo, pValueStr, rtnValue, bIsSigned))
					return false;

				bRtnIsSigned &= bIsSigned;
				operandStack.push_back(rtnValue);
			}

		} else if (GetToken() == eTkLParen) {
			GetNextToken(pPos);

			if (!ParseExpression(lineInfo, pPos, rtnValue, bIsSigned))
				return false;

			bRtnIsSigned &= bIsSigned;
			operandStack.push_back(rtnValue);

			if (GetToken() != eTkRParen) {
				CPreProcess::ParseMsg(Error, lineInfo, "expected ')'");
				return false;
			}
		} else {
			CPreProcess::ParseMsg(Error, lineInfo, "expected an operand");
			return false;
		}

		switch (GetNextToken(pPos)) {
		case eTkExprEnd:
		case eTkRParen:
		case eTkRBrack:
		case eTkColon:
			EvaluateExpression(eTkExprEnd, operandStack, operatorStack);

			if (operandStack.size() != 1)
				CPreProcess::ParseMsg(Fatal, lineInfo, "Evaluation error, operandStack");
			else if (operatorStack.size() != 0)
				CPreProcess::ParseMsg(Fatal, lineInfo, "Evaluation error, operatorStack");

			rtnValue = operandStack.back();
			return true;
		case eTkPlus:
		case eTkMinus:
		case eTkSlash:
		case eTkAsterisk:
		case eTkPercent:
		case eTkEqualEqual:
		case eTkVbar:
		case eTkVbarVbar:
		case eTkCarot:
		case eTkAmpersand:
		case eTkAmpersandAmpersand:
		case eTkBangEqual:
		case eTkLess:
		case eTkGreater:
		case eTkGreaterEqual:
		case eTkLessEqual:
		case eTkLessLess:
		case eTkGreaterGreater:
			EvaluateExpression(GetToken(), operandStack, operatorStack);
			GetNextToken(pPos);
			break;
		case eTkQuestion:
			EvaluateExpression(GetToken(), operandStack, operatorStack);
			GetNextToken(pPos);

			if (!ParseExpression(lineInfo, pPos, rtnValue, bIsSigned))
				return false;

			bRtnIsSigned &= bIsSigned;
			operandStack.push_back(rtnValue);

			if (GetToken() != eTkColon) {
				CPreProcess::ParseMsg(Error, lineInfo, "expected a ':'");
				return false;
			}

			GetNextToken(pPos);
			break;
		default:
			CPreProcess::ParseMsg(Error, lineInfo, "Unknown operator type");
			return false;
		}
	}
}

bool
CDefineTable::FindStringInTable(string &name, string &value)
{
	size_t i;
	for (i = 0; i < m_defineList.size(); i += 1)
		if (m_defineList[i].m_name == name)
			break;

	if (i < m_defineList.size()) {
		value = m_defineList[i].m_value;
		return true;
	}

	return false;
}

void
CDefineTable::EvaluateExpression(EToken tk, vector<int> &operandStack,
vector<int> &operatorStack)
{
	int op1, op2, op3, rslt;

	for (;;) {
		int tkPrec = GetTokenPrec(tk);
		EToken stackTk = (EToken)operatorStack.back();
		int stackPrec = GetTokenPrec(stackTk);

		if (tkPrec > stackPrec || tkPrec == stackPrec && CLex::IsTokenAssocLR(tk)) {
			operatorStack.pop_back();

			if (stackTk == eTkExprBegin) {
				HtlAssert(operandStack.size() == 1 && operatorStack.size() == 0);
				return;
			} else if (stackTk == eTkUnaryPlus || stackTk == eTkUnaryMinus ||
				stackTk == eTkTilda || stackTk == eTkBang) {

				// get operand off stack
				op1 = operandStack.back();
				operandStack.pop_back();

				switch (stackTk) {
				case eTkUnaryPlus:
					rslt = op1;
					break;
				case eTkUnaryMinus:
					rslt = -op1;
					break;
				case eTkTilda:
					rslt = ~op1;
					break;
				case eTkBang:
					rslt = !op1;
					break;
				default:
					HtlAssert(0);
				}
			} else if (stackTk == eTkQuestion) {

				// turnary operator ?:
				HtlAssert(operandStack.size() >= 3);
				op3 = operandStack.back();
				operandStack.pop_back();
				op2 = operandStack.back();
				operandStack.pop_back();
				op1 = operandStack.back();
				operandStack.pop_back();

				rslt = op1 ? op2 : op3;

			} else {

				// binary operators
				int depth = operandStack.size();
				HtlAssert(depth >= 2);
				op2 = operandStack.back();
				operandStack.pop_back();
				op1 = operandStack.back();
				operandStack.pop_back();

				switch (stackTk) {
				case eTkPlus:
					rslt = op1 + op2;
					break;
				case eTkMinus:
					rslt = op1 - op2;
					break;
				case eTkSlash:
					rslt = op1 / op2;
					break;
				case eTkAsterisk:
					rslt = op1 * op2;
					break;
				case eTkPercent:
					rslt = op1 % op2;
					break;
				case eTkVbar:
					rslt = op1 | op2;
					break;
				case eTkCarot:
					rslt = op1 ^ op2;
					break;
				case eTkAmpersand:
					rslt = op1 & op2;
					break;
				case eTkLessLess:
					rslt = op1 << op2;
					break;
				case eTkGreaterGreater:
					rslt = op1 >> op2;
					break;
				case eTkEqualEqual:
					rslt = op1 == op2;
					break;
				case eTkVbarVbar:
					rslt = op1 || op2;
					break;
				case eTkAmpersandAmpersand:
					rslt = op1 && op2;
					break;
				case eTkBangEqual:
					rslt = op1 != op2;
					break;
				case eTkLess:
					rslt = op1 < op2;
					break;
				case eTkGreater:
					rslt = op1 > op2;
					break;
				case eTkGreaterEqual:
					rslt = op1 >= op2;
					break;
				case eTkLessEqual:
					rslt = op1 <= op2;
					break;
				default:
					HtlAssert(0);
				}
			}

			operandStack.push_back(rslt);

		} else {
			// push operator on stack
			operatorStack.push_back(tk);
			break;
		}
	}
}

int
CDefineTable::GetTokenPrec(EToken tk)
{
	switch (tk) {
	case eTkUnaryPlus:
	case eTkUnaryMinus:
	case eTkTilda:
	case eTkBang:
		return 3;
	case eTkAsterisk:
	case eTkSlash:
	case eTkPercent:
		return 4;
	case eTkPlus:
	case eTkMinus:
		return 5;
	case eTkLessLess:
	case eTkGreaterGreater:
		return 6;
	case eTkLess:
	case eTkLessEqual:
	case eTkGreater:
	case eTkGreaterEqual:
		return 7;
	case eTkEqualEqual:
	case eTkBangEqual:
		return 8;
	case eTkAmpersand:
		return 9;
	case eTkCarot:
		return 10;
	case eTkVbar:
		return 11;
	case eTkAmpersandAmpersand:
		return 12;
	case eTkVbarVbar:
		return 13;
	case eTkQuestion:
		return 14;
	case eTkExprBegin:
		return 17;
	case eTkExprEnd:
		return 18;
	default:
		HtlAssert(0);
		return 0;
	}
}


EToken
CDefineTable::GetNextToken(const char *&pPos)
{
	const char *pInitPos;

	while (*pPos == ' ' || *pPos == '\t') pPos += 1;
	switch (*pPos++) {
	case '!':
		if (*pPos == '=') {
			pPos += 1;
			return m_tk = eTkBangEqual;
		}
		return m_tk = eTkBang;
	case '|':
		if (*pPos == '|') {
			pPos += 1;
			return m_tk = eTkVbarVbar;
		}
		return m_tk = eTkVbar;
	case '&':
		if (*pPos == '&') {
			pPos += 1;
			return m_tk = eTkAmpersandAmpersand;
		}
		return m_tk = eTkAmpersand;
	case '[':
		return m_tk = eTkLBrack;
	case ']':
		return m_tk = eTkRBrack;
	case '(':
		return m_tk = eTkLParen;
	case ')':
		return m_tk = eTkRParen;
	case '+':
		return m_tk = eTkPlus;
	case '-':
		return m_tk = eTkMinus;
	case '/':
		return m_tk = eTkSlash;
	case '*':
		return m_tk = eTkAsterisk;
	case '%':
		return m_tk = eTkPercent;
	case '^':
		return m_tk = eTkCarot;
	case '~':
		return m_tk = eTkTilda;
	case '=':
		if (*pPos++ == '=')
			return m_tk = eTkEqualEqual;
		return eTkError;
	case '<':
		if (*pPos == '<') {
			pPos += 1;
			return m_tk = eTkLessLess;
		}
		if (*pPos == '=') {
			pPos += 1;
			return m_tk = eTkLessEqual;
		}
		return m_tk = eTkLess;
	case '>':
		if (*pPos == '>') {
			pPos += 1;
			return m_tk = eTkGreaterGreater;
		}
		if (*pPos == '=') {
			pPos += 1;
			return m_tk = eTkGreaterEqual;
		}
		return m_tk = eTkGreater;
	case '?':
		return m_tk = eTkQuestion;
	case ':':
		return m_tk = eTkColon;
	case '\0':
		return m_tk = eTkExprEnd;
	default:
		pPos -= 1;
		if (isalpha(*pPos) || *pPos == '_') {
			pInitPos = pPos;
			while (isalnum(*pPos) || *pPos == '_') pPos += 1;
			m_tkString.assign(pInitPos, pPos - pInitPos);
			return m_tk = eTkIdent;
		}
		if (isdigit(*pPos)) {
			pInitPos = pPos;
			if (*pPos == '0' && pPos[1] == 'x') {
				pPos += 2;
				while (isxdigit(*pPos)) pPos += 1;
				m_tkString.assign(pInitPos, pPos - pInitPos);
				m_bIsSigned = false;
				SkipTypeSuffix(pPos);
				return m_tk = eTkNumHex;
			} else {
				while (isdigit(*pPos)) pPos += 1;
				m_tkString.assign(pInitPos, pPos - pInitPos);
				m_bIsSigned = true;
				SkipTypeSuffix(pPos);
				return m_tk = eTkNumInt;
			}
		}
		CPreProcess::ParseMsg(Error, "unknown preprocess token type");
		return eTkError;
	}
}

void
CDefineTable::SkipTypeSuffix(const char * &pPos)
{
	for (;;) {
		if (*pPos == 'u' || *pPos == 'U') {
			m_bIsSigned = false;
			pPos += 1;
		} else if (*pPos == 'l' || *pPos == 'L')
			pPos += 1;
		else
			break;
	}
}

int
CDefineTable::GetTokenValue()
{
	const char *pStr = GetTokenString().c_str();
	unsigned int value = 0;

	if (m_tk == eTkNumInt) {
		while (*pStr) {
			if (value >= 0xffffffff / 10) {
				CPreProcess::ParseMsg(Error, "integer too large");
				return 0;
			}
			value = value * 10 + *pStr - '0';
			pStr += 1;
		}
		return value;
	} else if (m_tk == eTkNumHex) {
		while (*pStr) {
			if (value >= 0xffffffff / 16) {
				CPreProcess::ParseMsg(Error, "integer too large");
				return 0;
			}
			if (*pStr >= '0' && *pStr <= '9')
				value = value * 16 + *pStr - '0';
			else if (*pStr >= 'A' && *pStr <= 'F')
				value = value * 16 + *pStr - 'A' + 10;
			else if (*pStr >= 'a' && *pStr <= 'f')
				value = value * 16 + *pStr - 'a' + 10;
			pStr += 1;
		}
		return value;
	} else {
		HtlAssert(0);
		return 0;
	}
}
