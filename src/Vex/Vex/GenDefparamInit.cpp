/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "stdafx.h"
#include "Design.h"

bool defParamInitDebug = false;

uint64
CDesign::GenDefparamInit(CDefparam *pDefparam, char lutSizeCh)
{
	int initBits = 1 << (lutSizeCh - '0');
	assert(initBits >= 0 && initBits <= 64);

    if (!pDefparam->m_bIsString) {
        COperand op(pDefparam->m_paramValue);
        return op.GetValue();
    }

	uint64 initValue = 0;
	uint32 identMask = 0;
	for (int i = 0; i < initBits; i += 1) {
		if (defParamInitDebug)
			printf(" inputs = 0x%x\n", i);

		SetLine(pDefparam->m_paramValue, pDefparam->m_lineInfo);
		GetNextToken();

		COperand op;

		if (!ParseInitExpr(pDefparam, i, identMask, op))
			return 0;

		if (GetNextToken() != tk_eof) {
			ParseMsg(PARSE_ERROR, "extra characters at end of expression");
			return 0;
		}

		if (op.GetWidth() != 1) {
			ParseMsg(PARSE_ERROR, "expression width not equal to one");
			return 0;
		}

		initValue |= ((op.GetValue() != 0) ? 1LL : 0) << i;

		if (defParamInitDebug)
			printf("\n");
	}

	if ((int)identMask != (initBits-1))
		ParseMsg(PARSE_ERROR, ".INIT identifiers do not match LUT size");

	return initValue;
}

bool
CDesign::ParseInitExpr(CDefparam *pDefparam, int identValue, uint32 &identMask, COperand &op)
{
	vector<COperand> operandStack;
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
			
			if (!EvalInitExpr(pDefparam, tk, operandStack, operatorStack))
				return false;
			
			tk = GetNextToken(); // unary operators
		}
				
		if (tk == tk_num) {
			COperand op(GetString());

			if (op.GetWidth() <= 0) {
				ParseMsg(PARSE_ERROR, pDefparam->m_lineInfo, "number missing width specifier");
				COperand op(GetString());
				return false;
			}

			operandStack.push_back(op);

			GetNextToken();
		} else if (tk == tk_ident) {
			if (GetString().size() != 2 || GetString()[0] != 'I' || !isdigit(GetString()[1]) ||
					GetString()[1] > '5') {
				ParseMsg(PARSE_ERROR, pDefparam->m_lineInfo, "expected identifier (I0-I5)");
				return false;
			}
			COperand op((identValue >> (GetString()[1] - '0')) & 1, 1);

			identMask |= 1 << (GetString()[1] - '0');

			operandStack.push_back(op);

			GetNextToken();
		} else if (tk == tk_lparen) {
			GetNextToken();

			COperand op;
			if (!ParseInitExpr(pDefparam, identValue, identMask, op))
				return false;

			operandStack.push_back(op);

			if (GetToken() != tk_rparen) {
				ParseMsg(PARSE_ERROR, "Expected a ')'");
				SkipTo(tk_semicolon);
				return 0;
			}

			GetNextToken();
		} else if (tk == tk_lbrace) {
			GetNextToken();

			COperand op(0,0);

			for (;;) {
				COperand op2;
				if (!ParseInitExpr(pDefparam, identValue, identMask, op2))
					return false;

				if (op2.GetWidth() < 0 && op.GetWidth() != 0) {
					ParseMsg(PARSE_ERROR, pDefparam->m_lineInfo, "cancatenation with undefined operand width");
					return false;
				}
			
				op = (op , op2);

				if (GetToken() != tk_comma)
					break;

				GetNextToken();
			}

			if (GetToken() != tk_rbrace) {
				ParseMsg(PARSE_ERROR, pDefparam->m_lineInfo, "expected '}'");
				return false;
			}

			operandStack.push_back(op);

			GetNextToken();
		} else {
			ParseMsg(PARSE_ERROR, "expected an operand");
			return false;
		}

		// post unary operator []
		if (GetToken() == tk_lbrack) {
			if (GetNextToken() != tk_num) {
				ParseMsg(PARSE_ERROR, "expected a number within []");
				return false;
			}

			int width;
			COperand op1(GetInteger(width), -1);

			if (GetNextToken() != tk_rbrack) {
				ParseMsg(PARSE_ERROR, "expected a ']'");
				return false;
			}

			COperand op2 = operandStack.back();
			operandStack.pop_back();

			if (op2.GetWidth() > 0 && op2.GetWidth() <= op1.GetValue()) {
				ParseMsg(PARSE_ERROR, "index greater than operand width");
				return false;
			}

			COperand rslt((op2.GetValue() >> op1.GetValue()) & 1, 1);

			if (defParamInitDebug)
				printf(" %s <= %s [ %s ]\n", rslt.GetDebugStr().c_str(), op2.GetDebugStr().c_str(),
					op1.GetDebugStr().c_str() );

			operandStack.push_back(rslt);

			GetNextToken();
		}
		
		switch (tk = GetToken()) {
		case tk_eof:
		case tk_comma:
		case tk_rbrace:
		case tk_rparen:
		case tk_colon:
		case tk_rbrack:

			tk = tk_exprEnd;

			if (!EvalInitExpr(pDefparam, tk, operandStack, operatorStack))
				return false;

			if (operandStack.size() != 1)
				ParseMsg(PARSE_ERROR, "Evaluation error, operandStack");
			else if (operatorStack.size() != 0)
				ParseMsg(PARSE_ERROR, "Evaluation error, operatorStack");

			op = operandStack.back();

			return true;

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

			if (!EvalInitExpr(pDefparam, tk, operandStack, operatorStack))
				return false;

			GetNextToken();
			break;

		case tk_question:

			if (!EvalInitExpr(pDefparam, tk, operandStack, operatorStack))
				return false;

			GetNextToken();

			if (!ParseInitExpr(pDefparam, identValue, identMask, op))
				return false;

			operandStack.push_back(op);

			if (GetToken() != tk_colon) {
				ParseMsg(PARSE_ERROR, "expected a ':'");
				SkipTo(tk_semicolon);
				return 0;
			}

			GetNextToken();
			break;

		default:
			ParseMsg(PARSE_ERROR, "Unexpected input while parsing expression");
			SkipTo(tk_semicolon);
			return 0;
		}
	}

	return 0;
}

bool
CDesign::EvalInitExpr(CDefparam *pDefparam, EToken tk, vector<COperand> &operandStack, vector<EToken> &operatorStack)
{
	COperand op1, op2, op3, rslt;
	
	for (;;) {
		int tkPrec = GetTokenPrec(tk);
		EToken stackTk = operatorStack.back();
		int stackPrec = GetTokenPrec(stackTk);
			
		if (tkPrec > stackPrec || tkPrec == stackPrec && IsTokenAssocLR(tk)) {
			operatorStack.pop_back();
			
			if (stackTk == tk_exprBegin) {
				assert(operandStack.size() == 1 && operatorStack.size() == 0);
				return true;
			} else if (stackTk == tk_unaryMinus || stackTk == tk_unaryPlus || stackTk == tk_tilda || stackTk == tk_bang) {
				
				// get operand off stack
				op1 = operandStack.back();
				operandStack.pop_back();

				if (stackTk == tk_unaryMinus)
					rslt = -op1;
				else if (stackTk == tk_tilda)
					rslt = ~op1;
				else if (stackTk == tk_bang)
					rslt = !op1;

				if (rslt.GetWidth() <= 0) {
					ParseMsg(PARSE_ERROR, pDefparam->m_lineInfo, "unary operator '%s' applied to operand with width %d",
						CLex::m_tokenTbl[stackTk].GetString().c_str(), rslt.GetWidth());
					return false;
				}
			
				if (defParamInitDebug)
					printf(" %s = %s %s\n", rslt.GetDebugStr().c_str(),
						CLex::m_tokenTbl[stackTk].GetString().c_str(), op1.GetDebugStr().c_str() );

				operandStack.push_back(rslt);
				
			} else if (stackTk != tk_question) {
				
				// binary operators
				assert(operandStack.size() >= 2);
				op2 = operandStack.back();
				operandStack.pop_back();
				op1 = operandStack.back();
				operandStack.pop_back();

				switch (stackTk) {
					case tk_lessLess: rslt = op1 << op2; break;
					case tk_vbar: rslt = op1 | op2; break;
					case tk_ampersand: rslt = op1 & op2; break;
					case tk_plus: rslt = op1 + op2; break;
					case tk_minus: rslt = op1 - op2; break;
					case tk_asterisk: rslt = op1 * op2; break;
					case tk_percent: rslt = op1 % op2; break;
					case tk_carot: rslt = op1 ^ op2; break;
					case tk_slash:
						if (op2.GetValue() == 0) {
							ParseMsg(PARSE_ERROR, pDefparam->m_lineInfo, "division by zero");
							return false;
						}
						rslt = op1 / op2;
						break;
					case tk_less:  rslt = op1 < op2; break;
					case tk_greater: rslt = op1 > op2; break;
					case tk_lessEqual: rslt = op1 <= op2; break;
					case tk_greaterEqual: rslt = op1 >= op2; break;
					case tk_equalEqual: rslt = op1 == op2; break;
					case tk_bangEqual:  rslt = op1 != op2; break;
					case tk_ampersandAmpersand: rslt = op1 && op2; break;
					case tk_vbarVbar: rslt = op1 || op2; break;
					default:
						ParseMsg(PARSE_ERROR, pDefparam->m_lineInfo, "unknown operator");
						return false;
				}

				if (rslt.GetWidth() <= 0) {
					ParseMsg(PARSE_ERROR, pDefparam->m_lineInfo, "binary operator '%s' applied to operands with widths %d and %d",
						CLex::m_tokenTbl[stackTk].GetString().c_str(), op1.GetWidth(), op2.GetWidth());
					return false;
				}

				if (defParamInitDebug)
					printf(" %s = %s %s %s\n", rslt.GetDebugStr().c_str(), op1.GetDebugStr().c_str(),
						CLex::m_tokenTbl[stackTk].GetString().c_str(), op2.GetDebugStr().c_str() );

				operandStack.push_back(rslt);

			} else {
				// turnary operator ?:

				assert(operandStack.size() >= 3);
				op3 = operandStack.back();
				operandStack.pop_back();
				op2 = operandStack.back();
				operandStack.pop_back();
				op1 = operandStack.back();
				operandStack.pop_back();

				rslt = COperand::SelectOperator(op1, op2, op3);

				if (rslt.GetWidth() <= 0) {
					ParseMsg(PARSE_ERROR, pDefparam->m_lineInfo, "select operator '?:' applied to operands with widths %d, %d and %d",
						op1.GetWidth(), op2.GetWidth(), op3.GetWidth());
					return false;
				}

				if (defParamInitDebug)
					printf(" %s = %s ? %s : %s\n", rslt.GetDebugStr().c_str(), op1.GetDebugStr().c_str(),
						op2.GetDebugStr().c_str(), op3.GetDebugStr().c_str() );

				operandStack.push_back(rslt);
			}
		} else {
			// push operator on stack
			operatorStack.push_back(tk);
			break;
		}
	}

	return true;
}
