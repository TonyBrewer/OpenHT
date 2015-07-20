/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
// WidthRules.cpp: Check Width Rules
//   These routines fold constant expressions, set and check
//     expression widths.
//
//////////////////////////////////////////////////////////////////////

#include "Htfe.h"
#include "HtfeDesign.h"

void CHtfeDesign::CheckWidthRules()
{
    CHtfeIdentTblIter moduleIter(m_pTopHier->GetIdentTbl());
    for (moduleIter.Begin(); !moduleIter.End(); moduleIter++) {

        if (!moduleIter->IsModule() || !moduleIter->IsMethodDefined())
            continue;

        CHtfeIdentTblIter funcIter(moduleIter->GetIdentTbl());
        for (funcIter.Begin(); !funcIter.End(); funcIter++) {

            if (!funcIter->IsFunction())
                continue;

            CheckWidthRules(funcIter->GetThis(), funcIter->GetStatementList());
        }
    }

	if (GetErrorCnt() != 0) {
		ParseMsg(PARSE_ERROR, "unable to generate verilog due to previous errors");
		ErrorExit();
	}
}

void CHtfeDesign::CheckWidthRules(CHtfeIdent *pHier, CHtfeStatement *pStatement, int switchWidth)
{
    // Recursively walk statement lists
    vector<uint64> caseItemList;
    bool bDefaultSeen = false;
    CLineInfo lineInfo;

    for ( ; pStatement != 0; pStatement = pStatement->GetNext()) {

        switch (pStatement->GetStType()) {
            case st_compound:
                CheckWidthRules(pHier, pStatement->GetCompound1());
                break;
            case st_assign:
				if (IsTask(pStatement))
					CheckWidthRules(pStatement->GetExpr());
                else
					SetExpressionWidth(pStatement->GetExpr());
                break;
            case st_return:
				if (pStatement->GetExpr())
					SetExpressionWidth(pStatement->GetExpr(), pHier->GetWidth());
                break;
            case st_if:
                SetExpressionWidth(pStatement->GetExpr(), 0, true);

                CheckWidthRules(pHier, pStatement->GetCompound1());
                CheckWidthRules(pHier, pStatement->GetCompound2());
                break;
            case st_switch:
                {
                    CHtfeOperand * pExpr = pStatement->GetExpr();
					if (pExpr->IsLeaf() ? (pExpr->GetType()->IsSigned() && pExpr->GetType()->GetId() != CHtfeIdent::id_enumType) : pExpr->IsSigned())
						ParseMsg(PARSE_ERROR, pStatement->GetLineInfo(), "Signed switch expression not supported");

                    int switchWidth = SetExpressionWidth(pStatement->GetExpr());
                    CheckWidthRules(pHier, pStatement->GetCompound1(), switchWidth);
                }
                break;
            case st_case:
                {
                    lineInfo = pStatement->GetExpr()->GetLineInfo();

                    m_bCaseExpr = true;
                    SetExpressionWidth(pStatement->GetExpr(), switchWidth);
                    m_bCaseExpr = false;

                    if (pStatement->GetExpr()->IsConstValue()) {
                        // check if case constant is wider than switch expression width
						pStatement->GetExpr()->GetConstValueRef().ConvertToUnsigned(pStatement->GetExpr()->GetLineInfo());
                        int caseWidth = pStatement->GetExpr()->GetConstValue().GetMinWidth();
                        if (caseWidth > switchWidth)
                            ParseMsg(PARSE_ERROR, pStatement->GetExpr()->GetLineInfo(),
                            "case width (%d) greater than switch width (%d)", caseWidth, switchWidth);

                        // check unique case items and number of ones in value for hot states
                        uint64 caseItem = pStatement->GetExpr()->GetConstValue().GetSint64();
                        unsigned int i;
                        for (i = 0; i < caseItemList.size(); i += 1) {
                            if (caseItemList[i] == caseItem) {
                                ParseMsg(PARSE_ERROR, pStatement->GetExpr()->GetLineInfo(),
                                    "duplicate case item (%d)", caseItem);
                                break;
                            }
                        }

                        if (i == caseItemList.size())
                            caseItemList.push_back(caseItem);

                    } else
                        ParseMsg(PARSE_ERROR, pStatement->GetExpr()->GetLineInfo(),
                        "expected constant case expression");

                    CheckWidthRules(pHier, pStatement->GetCompound1());
                    break;
                }
            case st_default:
                bDefaultSeen = true;
                CheckWidthRules(pHier, pStatement->GetCompound1());
                break;
            case st_null:
            case st_break:
                break;
            default:
                Assert(0);
        }
    }

    if (switchWidth > 0 && !bDefaultSeen && caseItemList.size() != (1u << switchWidth))
        ParseMsg(PARSE_ERROR, lineInfo, "Switch statement not fully specified\n");
}

int CHtfeDesign::SetExpressionWidth(CHtfeOperand *pExpr, int reqWidth, bool bForceBoolean)
{
    if (pExpr == 0)
        return 0;

    // Set sign of expression for checking
    SetOpWidthAndSign(pExpr);

    CheckWidthRules(pExpr, bForceBoolean);

    if (reqWidth == 0) {
        int minWidth=0;
        int sigWidth=0;
        int setWidth=0;
		int opWidth=0;

        GetExpressionWidth(pExpr, minWidth, sigWidth, setWidth, opWidth);

        if (setWidth > 0)
            reqWidth = setWidth;
        else if (sigWidth > 0)
            reqWidth = sigWidth;
        else
            reqWidth = minWidth;
    }

    return reqWidth;
}

void CHtfeDesign::SetOpWidthAndSign(CHtfeOperand *pExpr, EToken prevTk, bool bIsLeftOfEqual)
{
#ifdef WIN32
	if (pExpr->GetLineInfo().m_lineNum == 416)
		bool stop = true;
#endif
    if (pExpr->IsLeaf()) {

        if (pExpr->IsVariable()) {

			if (pExpr->GetType()->IsScState())
				return;

            bool bCompareOp = prevTk == tk_equalEqual || prevTk == tk_bangEqual || prevTk == tk_less
                || prevTk == tk_greater || prevTk == tk_greaterEqual || prevTk == tk_lessEqual;

            // determine sign of expression
            bool bIsSigned = pExpr->GetType()->IsSigned();

            if (!bIsLeftOfEqual && !bCompareOp && pExpr->GetType()->IsPromoteToSigned())
                bIsSigned = true;

            pExpr->SetIsSigned(bIsSigned);

            if (pExpr->GetSubField() == 0 || pExpr->GetSubField()->m_pIdent == 0) {
                pExpr->SetOpWidth( pExpr->GetType()->GetOpWidth() );
            } else {
                CScSubField * pSubField = pExpr->GetSubField();
                while (pSubField->m_pNext && pSubField->m_pNext->m_pIdent)
                    pSubField = pSubField->m_pNext;
                pExpr->SetOpWidth( pSubField->m_pIdent->GetType()->GetOpWidth() );
            }

            HtfeOperandList_t &indexList = pExpr->GetIndexList();
            for (size_t i = 0; i < indexList.size(); i += 1) {

                CHtfeOperand * pIndexOp = indexList[i];

                SetOpWidthAndSign(pIndexOp);
            }

            for (CScSubField *pSubField = pExpr->GetSubField() ; pSubField; pSubField = pSubField->m_pNext) {
                for (size_t i = 0; i < pSubField->m_indexList.size(); i += 1) {
                    CHtfeOperand * pIndexOp = pSubField->m_indexList[i];

                    SetOpWidthAndSign(pIndexOp);
                }
            }

        } else if (pExpr->IsFunction()) {

            pExpr->SetIsSigned(pExpr->GetType()->IsSigned());

            //CHtfeIdent * pFunc = pExpr->GetMember();
            vector<CHtfeOperand *> &paramList = pExpr->GetParamList();
            for (unsigned int i = 0; i < paramList.size(); i += 1) {
                CHtfeOperand *pParam = paramList[i];

                if (pParam->GetMember() && pParam->GetMember()->IsScState())
                    continue;

                SetOpWidthAndSign(pParam);
            }

            if (pExpr->GetType() != m_pVoidType) {
                if (pExpr->GetSubField() == 0 || pExpr->GetSubField()->m_pIdent == 0)
                    pExpr->SetOpWidth( pExpr->GetType()->GetOpWidth() );
                else {
                    CScSubField * pSubField = pExpr->GetSubField();
                    while (pSubField->m_pNext && pSubField->m_pNext->m_pIdent)
                        pSubField = pSubField->m_pNext;

                    pExpr->SetOpWidth( pSubField->m_pIdent->GetType()->GetOpWidth() );
                }
            }

        } else if (pExpr->IsConstValue()) {
            pExpr->SetIsSigned(pExpr->GetConstValue().IsSigned());
			pExpr->SetOpWidth(pExpr->GetConstValue().Is32Bit() ? 32 : 64);

        } else if (pExpr->GetMember()->IsType()) {
            pExpr->SetIsSigned(pExpr->GetType()->IsSigned());

        } else
            Assert(0);

    } else {

        CHtfeOperand *pOp1 = pExpr->GetOperand1();
        CHtfeOperand *pOp2 = pExpr->GetOperand2();
        CHtfeOperand *pOp3 = pExpr->GetOperand3();

        EToken tk = pExpr->GetOperator();

        if (tk == tk_equal && pOp1->IsVariable() && !pOp1->IsSigned() && pOp2->IsConstValue() && pOp2->IsSigned())
            pOp2->GetConstValueRef().ConvertToUnsigned(pOp2->GetLineInfo());

        if (pOp1)
            SetOpWidthAndSign(pOp1, tk, tk == tk_equal);
        if (pOp2)
            SetOpWidthAndSign(pOp2, tk);
        if (pOp3)
            SetOpWidthAndSign(pOp3);

        switch (tk) {
        case tk_equal:
            pExpr->SetIsSigned(pOp1->IsSigned());
            pExpr->SetOpWidth(pOp1->GetOpWidth());

            if (pOp2->IsConstValue() && (pOp1->IsSigned() != pOp2->IsSigned())) {
                if (!pOp2->GetConstValue().IsNeg()) {
                    if (pOp1->IsSigned()) {
                        pOp2->GetConstValueRef().ConvertToSigned(pOp2->GetLineInfo());
                        pOp2->SetIsSigned(true);
                    } else {
                        pOp2->GetConstValueRef().ConvertToUnsigned(pOp2->GetLineInfo());
                        pOp2->SetIsSigned(false);
                    }
                }
            }
            break;
        case tk_period:
            pExpr->SetIsSigned(pOp2->IsSigned());
            pExpr->SetOpWidth(pOp2->GetOpWidth());
            break;
        case tk_vbarVbar:
        case tk_ampersandAmpersand:
        case tk_bang:
            pExpr->SetIsSigned(false);
            pExpr->SetOpWidth(1);
            break;
        case tk_equalEqual:
        case tk_bangEqual:
        case tk_less:
        case tk_greater:
        case tk_greaterEqual:
        case tk_lessEqual:
            if (pOp1->IsSigned() != pOp2->IsSigned()) {
                // check if constant value needs converted to signed/unsigned
                if (!pOp1->IsConstValue() && pOp2->IsConstValue() && pOp2->GetConstValue().IsPos()) {
                    if (pOp1->IsSigned() && pOp2->GetConstValue().GetMinWidth() < 32) {
                        pOp2->GetConstValueRef().ConvertToSigned(pOp2->GetLineInfo());
                        pOp2->SetIsSigned(true);
                    } else if (!pOp1->IsSigned()) {
                        pOp2->GetConstValueRef().ConvertToUnsigned(pOp2->GetLineInfo());
                        pOp2->SetIsSigned(false);
                    }

                } else if (!pOp2->IsConstValue() && pOp1->IsConstValue() && pOp1->GetConstValue().IsPos()) {
                    if (pOp2->IsSigned() && pOp1->GetConstValue().GetMinWidth() < 32) {
                        pOp1->GetConstValueRef().ConvertToSigned(pOp1->GetLineInfo());
                        pOp1->SetIsSigned(true);
                    } else if (!pOp2->IsSigned()) {
                        pOp1->GetConstValueRef().ConvertToUnsigned(pOp1->GetLineInfo());
                        pOp1->SetIsSigned(false);
                    }

                } else if (pOp1->IsVariable() && pOp1->GetType()->IsPromoteToSigned())
                    pOp1->SetIsSigned(true);

                else if (pOp2->IsVariable() && pOp2->GetType()->IsPromoteToSigned())
                    pOp2->SetIsSigned(true);
            }

            pExpr->SetIsSigned(false);
            pExpr->SetOpWidth(1);
            break;
        case tk_question:
            pExpr->SetIsSigned(pOp2->IsSigned() && pOp3->IsSigned());
            pExpr->SetOpWidth(max(pOp2->GetOpWidth(), pOp3->GetOpWidth()));

            if (pOp2->IsConstWidth() && pOp3->IsConstWidth()) {
                pExpr->SetConstWidth(true);
                pExpr->SetIsPosSignedConst( pOp2->IsPosSignedConst() && pOp3->IsPosSignedConst() );

            } else if (pOp2->IsConstWidth()) {

                //if (pOp2->IsConstValue() && pOp2->GetConstValue().IsPos() && pOp2->IsSigned() && pOp1->IsSigned() != pOp2->IsSigned()) {
                //    pOp2->GetConstValueRef().ConvertToUnsigned(pOp2->GetLineInfo());
                //    pOp2->SetIsSigned(false);
                //}

                pExpr->SetIsPosSignedConst( pOp2->GetMinWidth() > pOp3->GetMinWidth() && pOp2->IsPosSignedConst() );

            } else if (pOp3->IsConstWidth()) {

                //if (pOp1->IsConstValue() && pOp1->GetConstValue().IsPos() && pOp1->IsSigned() && pOp1->IsSigned() != pOp2->IsSigned()) {
                //    pOp1->GetConstValueRef().ConvertToUnsigned(pOp1->GetLineInfo());
                //    pOp1->SetIsSigned(false);
                //}

                pExpr->SetIsPosSignedConst( pOp3->GetMinWidth() > pOp2->GetMinWidth() && pOp3->IsPosSignedConst() );
            }
            break;
        case tk_vbar:
        case tk_carot:
        case tk_ampersand:
            pExpr->SetIsSigned(pOp1->IsSigned() && pOp2->IsSigned());
            pExpr->SetOpWidth(max(pOp1->GetOpWidth(), pOp2->GetOpWidth()));
            break;
        case tk_slash:
            pExpr->SetIsSigned(pOp1->IsSigned() && pOp2->IsSigned());
            pExpr->SetOpWidth(max(pOp1->GetOpWidth(), pOp2->GetOpWidth()));
           
            if (pOp1->IsConstWidth() && pOp2->IsConstWidth())
                pExpr->SetConstWidth(true);

            if (pOp2->IsConstValue() && pOp2->GetConstValue().IsPos() &&
                pOp2->IsSigned() && pOp1->IsSigned() != pOp2->IsSigned())
            {
                pOp2->GetConstValueRef().ConvertToUnsigned(pOp2->GetLineInfo());
                pOp2->SetIsSigned(false);
            }
            break;
        case tk_plus:
        case tk_minus:
            pExpr->SetIsSigned(pOp1->IsSigned() && pOp2->IsSigned());
            pExpr->SetOpWidth(max(pOp1->GetOpWidth(), pOp2->GetOpWidth()));

            if (pOp1->IsConstWidth() && pOp2->IsConstWidth())
                pExpr->SetConstWidth(true);

            break;
        case tk_asterisk:	// multiply
            pExpr->SetIsSigned(pOp1->IsSigned() && pOp2->IsSigned());
            pExpr->SetOpWidth(max(pOp1->GetOpWidth(), pOp2->GetOpWidth()));

            if (pOp1->IsConstWidth() && pOp2->IsConstWidth())
                pExpr->SetConstWidth(true);

            if (pOp1->IsConstValue() && pOp1->GetConstValue().IsPos() &&
                pOp1->IsSigned() && pOp1->IsSigned() != pOp2->IsSigned()) 
            {
                pOp1->GetConstValueRef().ConvertToUnsigned(pOp1->GetLineInfo());
                pOp1->SetIsSigned(false);
            }

            if (pOp2->IsConstValue() && pOp2->GetConstValue().IsPos() &&
                pOp2->IsSigned() && pOp1->IsSigned() != pOp2->IsSigned())
            {
                pOp2->GetConstValueRef().ConvertToUnsigned(pOp2->GetLineInfo());
                pOp2->SetIsSigned(false);
            }
            break;
        case tk_percent:	// modulo
            pExpr->SetIsSigned(pOp1->IsSigned() && pOp2->IsSigned());
            pExpr->SetOpWidth(max(pOp1->GetOpWidth(), pOp2->GetOpWidth()));

            if (pOp1->IsConstWidth() && pOp2->IsConstWidth())
                pExpr->SetConstWidth(true);
                
            if (pOp2->IsConstValue() && pOp2->GetConstValue().IsPos() &&
                pOp2->IsSigned() && pOp1->IsSigned() != pOp2->IsSigned())
            {
                pOp2->GetConstValueRef().ConvertToUnsigned(pOp2->GetLineInfo());
                pOp2->SetIsSigned(false);
            }
            break;
        case tk_tilda:
        case tk_unaryMinus:
        case tk_unaryPlus:
        case tk_preInc:
        case tk_preDec:
        case tk_postInc:
        case tk_postDec:
            pExpr->SetIsSigned(pOp1->IsSigned());
            pExpr->SetOpWidth(pOp1->GetOpWidth());

            if (pOp1->IsConstWidth())
                pExpr->SetConstWidth(true);

            break;
        case tk_greaterGreater:	// shift right
        case tk_lessLess:	// shift left
			{
				pExpr->SetIsSigned(pOp1->IsSigned());
				pExpr->SetOpWidth(pOp1->GetOpWidth());

				int op1Width;
				if (pOp1->GetOpWidth() <= 32)
					op1Width = 5;
				else if (pOp1->GetOpWidth() <= 64)
					op1Width = 6;
				else
					op1Width = 32;

				int op2Width;
				if (pOp2->IsLeaf())
					op2Width = pOp2->GetWidth();
				else if (pOp2->GetOperator() == tk_typeCast)
					op2Width = pOp2->GetType()->GetWidth();
				else
					op2Width = 6;

				int width = min(op1Width, op2Width);

				if (pOp2->IsConstValue()) {

					if (pOp2->GetConstValue().IsNeg())
						ParseMsg(PARSE_ERROR, pExpr->GetLineInfo(), "shift by negative constant");

					if (pOp1->GetOpWidth() <= pOp2->GetConstValue().GetSint64())
					   ParseMsg(PARSE_ERROR, pExpr->GetLineInfo(), "shift by amount greater than operand width");

					pOp2->GetConstValueRef() = pOp2->GetConstValue() & ((1 << width)-1);

				} else if (width < op2Width && (
					pOp2->GetOperator() != tk_typeCast || pOp2->GetType()->GetWidth() != width ||
					pOp2->GetType()->IsSigned()))
				{
					// insert a typecast to limit with of shift amount operand
					CHtfeIdent * pCastType = CreateUniqueScIntType(m_pScUintType, width);

					CHtfeOperand * pTypeOp = HandleNewOperand();
					pTypeOp->InitAsIdentifier(pExpr->GetLineInfo(), pCastType);

					CHtfeOperand * pCastOp = HandleNewOperand();
					pCastOp->InitAsOperator(pExpr->GetLineInfo(), tk_typeCast, pTypeOp, pOp2);

					pExpr->SetOperand2(pCastOp);
				}

				//if (pOp2->IsConstValue() && pOp2->GetConstValue().IsPos() &&
				//    pOp2->IsSigned() && pOp1->IsSigned() != pOp2->IsSigned())
				//{
				//    pOp2->GetConstValueRef().ConvertToUnsigned(pOp2->GetLineInfo());
				//    pOp2->SetIsSigned(false);
				//}
			}
            break;
        case tk_typeCast:
            pExpr->SetIsSigned(pOp1->IsSigned());
            pOp1->SetOpWidth(pOp1->GetMember()->GetOpWidth());
            pExpr->SetOpWidth(pOp1->GetOpWidth());
            break;
        case tk_comma:
            pExpr->SetIsSigned(false);
            pExpr->SetOpWidth(max(pOp1->GetOpWidth(), pOp2->GetOpWidth()));

            if (pOp1->IsConstWidth() && pOp2->IsConstWidth())
                pExpr->SetConstWidth(true);

            break;
        default:
            Assert(0);
        }
    }
}

void CHtfeDesign::CheckWidthRules(CHtfeOperand *pExpr, bool bForceBoolean)
{
    char op1Buf[6];
    char op2Buf[6];
    char op3Buf[6];

#ifdef WIN32
	if (pExpr->GetLineInfo().m_lineNum == 130)
		bool stop = true;
#endif

    if (pExpr->GetOperator() == tk_period) {
        pExpr = pExpr->GetOperand2();
        Assert(pExpr->IsLeaf() && pExpr->GetMember()->IsFunction());
    }

    if (pExpr->IsLeaf()) {
        if (pExpr->IsVariable()) {

            pExpr->SetMinWidth( pExpr->GetSubFieldWidth() );

            HtfeOperandList_t &indexList = pExpr->GetIndexList();
            for (size_t i = 0; i < indexList.size(); i += 1) {

                CHtfeOperand * pIndexOp = indexList[i];

                CheckWidthRules(pIndexOp);

                if (pExpr->GetMember()->GetDimenCnt() != indexList.size())
                    ParseMsg(PARSE_FATAL, pExpr->GetLineInfo(), "invalid array dimension size");

                int idxDimen = (int)pExpr->GetMember()->GetDimenList()[i];
				int idxWidth;
                if (!GetFieldWidthFromDimen( idxDimen, idxWidth ))
                    ParseMsg(PARSE_FATAL, pExpr->GetLineInfo(), "invalid array dimension size (32K limit)");

                if (pIndexOp->IsConstValue() && (pIndexOp->GetConstValue().GetSint32() < 0 || pIndexOp->GetConstValue().GetSint32() >= idxDimen))
				    ParseMsg(PARSE_ERROR, pIndexOp->GetLineInfo(), "invalid value for array index: index value(%d) declared range (0-%d)",
					    (int)pIndexOp->GetConstValue().GetSint64(), idxDimen-1);

                if (!pIndexOp->IsConstValue() && (pIndexOp->GetMinWidth() > idxWidth || pIndexOp->GetSigWidth() < idxWidth))

                    ParseMsg(PARSE_ERROR, pIndexOp->GetLineInfo(), "invalid width for array index: declared width %d, expression width %s",
                        idxWidth, GetParseMsgWidthStr(pIndexOp, op1Buf));
            }

            CHtfeIdent *pIdent = pExpr->GetMember();
            for (CScSubField *pSubField = pExpr->GetSubField() ; pSubField; pSubField = pSubField->m_pNext) {
                for (size_t i = 0; i < pSubField->m_indexList.size(); i += 1) {
                    CHtfeOperand * pIndexOp = pSubField->m_indexList[i];

                    CheckWidthRules(pIndexOp);

                    if (pSubField->m_pIdent) {
                        // field indexing
                        if (pSubField->m_pIdent->GetDimenCnt() != pSubField->m_indexList.size())
                            ParseMsg(PARSE_FATAL, pExpr->GetLineInfo(), "invalid field dimension size");

                        int idxDimen = (int)pSubField->m_pIdent->GetDimenList()[i];
                        int idxWidth;
						if (!GetFieldWidthFromDimen( idxDimen, idxWidth ))
                            ParseMsg(PARSE_FATAL, pExpr->GetLineInfo(), "invalid field dimension size (32K limit)");

                        if (pIndexOp->IsConstValue() && (pIndexOp->GetConstValue().GetSint32() < 0 || pIndexOp->GetConstValue().GetSint32() >= idxDimen)
                            || !pIndexOp->IsConstValue() && (pIndexOp->GetMinWidth() > idxWidth || pIndexOp->GetSigWidth() < idxWidth))

                            ParseMsg(PARSE_ERROR, pIndexOp->GetLineInfo(), "invalid width for field index: declared width %d, expression width %s",
								idxWidth, GetParseMsgWidthStr(pIndexOp, op1Buf));

                        pIdent = pSubField->m_pIdent;
                    } else {
                        // bit indexing
                        int idxDimen = pIdent->GetWidth();
                        int idxWidth;
						if (!GetFieldWidthFromDimen( idxDimen, idxWidth ))
                            ParseMsg(PARSE_FATAL, pExpr->GetLineInfo(), "invalid bit indexing value (32K limit)");

                        CConstValue indexValue;
                        bool bIndexConst;
                        if (bIndexConst = pIndexOp->IsConstValue())
                            indexValue = pIndexOp->GetConstValue();
                        else if (bIndexConst = (pIndexOp->IsVariable() && pIndexOp->GetMember()->IsConstVar()))
                            indexValue = pIndexOp->GetMember()->GetConstValue();

                        if (bIndexConst && (indexValue.GetSint32() < 0 || indexValue.GetSint32() >= idxDimen)
                            || !bIndexConst && (pIndexOp->GetMinWidth() > idxWidth || pIndexOp->GetSigWidth() < idxWidth))

                            ParseMsg(PARSE_ERROR, pIndexOp->GetLineInfo(), "invalid width for bit index: variable width %d, expression width %s",
                                idxWidth, GetParseMsgWidthStr(pIndexOp, op1Buf));
                    }
                }
            }

        } else if (pExpr->IsFunction()) {

            //pExpr->SetIsSigned(pExpr->GetType()->IsSigned());

            CHtfeIdent * pFunc = pExpr->GetMember();
            vector<CHtfeOperand *> &paramList = pExpr->GetParamList();
            for (unsigned int i = 0; i < paramList.size(); i += 1) {
                CHtfeOperand *pParam = paramList[i];

                if (pParam->GetMember() && pParam->GetMember()->IsScState())
                    continue;

                SetExpressionWidth(pParam, pFunc->GetParamWidth(i));

                if (pParam->IsConstValue() && pParam->GetConstValue().IsPos())
                    pParam->GetConstValueRef().ConvertToUnsigned(pParam->GetLineInfo());

                CHtfeIdent *pType = pExpr->GetMember()->GetParamType(i);
                if (pParam->GetMinWidth() > pType->GetWidth() ||
					(pParam->GetMember() && !pParam->IsConstValue() && pParam->GetSigWidth() < pType->GetWidth()))

                    ParseMsg(PARSE_ERROR, pParam->GetLineInfo(), "invalid width for parameter %d: declared width %d, expression width %s",
                        i+1, pType->GetWidth(), GetParseMsgWidthStr(pParam, op1Buf));
            }

            if (pExpr->GetType() != m_pVoidType)
                pExpr->SetMinWidth( pExpr->GetSubFieldWidth() );

        } else if (!pExpr->IsConstValue())
            Assert(0);

    } else {

        CHtfeOperand *pOp1 = pExpr->GetOperand1();
        CHtfeOperand *pOp2 = pExpr->GetOperand2();
        CHtfeOperand *pOp3 = pExpr->GetOperand3();

        if (pOp1 && pExpr->GetOperator() != tk_typeCast)
            CheckWidthRules(pOp1);
        if (pOp2)
            CheckWidthRules(pOp2);
        if (pOp3)
            CheckWidthRules(pOp3);

        switch (pExpr->GetOperator()) {
        case tk_equal:

            if (pOp2->IsConstValue() && (pOp1->IsSigned() != pOp2->IsSigned())) {
                if (pOp2->GetConstValue().IsNeg())
                    ParseMsg(PARSE_ERROR, pOp2->GetLineInfo(), "Negative constant assigned to unsigned variable");
            }

            pExpr->SetMinWidth(pOp1->GetMinWidth());

            if (pOp2->IsUnknownWidth())
                break;

            //if (pOp1->GetMinWidth() >= pOp2->GetMinWidth() && pOp1->GetMinWidth() <= pOp2->GetSigWidth())
            //    break;

            if (/*pOp2->IsConstWidth() && */pOp2->GetMinWidth() <= pOp1->GetMinWidth())
                break;

            if (pOp2->IsPosSignedConst() && !pOp1->IsSigned() && pOp2->GetMinWidth()-1 <= pOp1->GetMinWidth())
                break;

            ParseMsg(PARSE_ERROR, pOp2->GetLineInfo(), "Assignment width mismatch: %d = %d+%d",
                pOp1->GetMinWidth(), pOp2->GetMinWidth(), pOp2->GetSigWidth() - pOp2->GetMinWidth());

            break;
        case tk_vbarVbar:
        case tk_ampersandAmpersand:
            InsertCompareOperator(pOp1);
            InsertCompareOperator(pOp2);

            pExpr->SetMinWidth(1);

            break;
        case tk_bang:
            InsertCompareOperator(pOp1);

            pExpr->SetMinWidth(1);

            break;
        case tk_equalEqual:
        case tk_bangEqual:
        case tk_less:
        case tk_greater:
        case tk_greaterEqual:
        case tk_lessEqual:
            pExpr->SetMinWidth(1);

            if (pOp1->IsSigned() != pOp2->IsSigned())
                ParseMsg(PARSE_ERROR, pOp1->GetLineInfo(), "comparison operation has inconsistent signed/unsigned operands");

            break;

        case tk_question:

            if (pOp1->IsUnknownWidth() || pOp2->IsUnknownWidth() || pOp3->IsUnknownWidth())
                break;		

            if (!pOp2->IsConstWidth() && !pOp3->IsConstWidth() && 
                !(pOp2->GetMinWidth() <= pOp3->GetSigWidth() && pOp2->GetSigWidth() >= pOp3->GetMinWidth()))
            {
                ParseMsg(PARSE_ERROR, pOp1->GetLineInfo(), "operand width mismatch, %s ? %s : %s\n",
                    GetParseMsgWidthStr(pOp1, op1Buf), GetParseMsgWidthStr(pOp2, op2Buf), GetParseMsgWidthStr(pOp3, op3Buf));
            }

            pExpr->SetMinWidth(max(pOp2->GetMinWidth(), pOp3->GetMinWidth()));
            pExpr->SetSigWidth(max(pOp2->GetSigWidth(), pOp3->GetSigWidth()));

            break;
        case tk_vbar:
        case tk_carot:

            if (pOp1->IsUnknownWidth() || pOp2->IsUnknownWidth())
                break;

            if (pOp1->IsConstWidth() && pOp2->IsConstWidth()) {
                pExpr->SetConstWidth(true);

                if (pOp1->GetMinWidth() > pOp2->GetMinWidth()) 
                    pExpr->SetMinWidth(pOp1->GetMinWidth());
                else
                    pExpr->SetMinWidth(pOp2->GetMinWidth());

                break;
            } else if (pOp1->IsConstWidth()) {
                //if (pOp1->IsConstValue() && pOp1->GetConstValue().IsPos() && pOp1->IsSigned() && pOp1->IsSigned() != pOp2->IsSigned())
                //    pOp1->GetConstValueRef().ConvertToUnsigned(pOp1->GetLineInfo());

                int op1MinWidth = pOp1->GetMinWidth();
                if (pOp1->IsConstValue() && pOp1->GetConstValue().IsPos() && pOp1->IsSigned())
                    op1MinWidth -= 1;

                pExpr->SetMinWidth(max(op1MinWidth, pOp2->GetMinWidth()));
                pExpr->SetSigWidth(max(op1MinWidth, pOp2->GetSigWidth()));

                break;
            } else if (pOp2->IsConstWidth()) {
                //if (pOp2->IsConstValue() && pOp2->GetConstValue().IsPos() && pOp2->IsSigned() && pOp1->IsSigned() != pOp2->IsSigned())
                //    pOp2->GetConstValueRef().ConvertToUnsigned(pOp2->GetLineInfo());

                int op2MinWidth = pOp2->GetMinWidth();
                if (pOp2->IsConstValue() && pOp2->GetConstValue().IsPos() && pOp2->IsSigned())
                    op2MinWidth -= 1;

                pExpr->SetMinWidth(max(pOp1->GetMinWidth(), op2MinWidth));
                pExpr->SetSigWidth(max(pOp1->GetSigWidth(), op2MinWidth));
                break;
            } else {
                pExpr->SetMinWidth(max(pOp1->GetMinWidth(), pOp2->GetMinWidth()));
                pExpr->SetSigWidth(max(pOp1->GetSigWidth(), pOp2->GetSigWidth()));
                break;
            }

            ParseMsg(PARSE_ERROR, pOp1->GetLineInfo(), "operand width mismatch, %s %s %s\n",
                GetParseMsgWidthStr(pOp1, op1Buf), GetTokenString(pExpr->GetOperator()).c_str(), GetParseMsgWidthStr(pOp2, op2Buf));
            break;
        case tk_ampersand:

            if (pOp1->IsUnknownWidth() || pOp2->IsUnknownWidth())
                break;

            if (pOp1->IsConstWidth() && pOp2->IsConstWidth()) {
                pExpr->SetConstWidth(true);
                pExpr->SetMinWidth(min(pOp1->GetSigWidth(), pOp2->GetSigWidth()));

            } else if (pOp1->IsConstWidth()) {

				if (pOp1->IsConstValue()) {
					if (pOp1->GetConstValue().GetSint64() < 0) {
						pExpr->SetMinWidth(pOp2->GetMinWidth());
						pExpr->SetSigWidth(pOp2->GetSigWidth());
					} else {
						int op1SigWidth = pOp1->GetConstValue().GetSigWidth();
						if (pOp1->IsSigned())
							op1SigWidth -= 1;

						pExpr->SetMinWidth(min(op1SigWidth, pOp2->GetMinWidth()));
						pExpr->SetSigWidth(min(op1SigWidth, pOp2->GetSigWidth()));
					}
				} else {
					pExpr->SetMinWidth(min(pOp1->GetSigWidth(), pOp2->GetMinWidth()));
					pExpr->SetSigWidth(min(pOp1->GetSigWidth(), pOp2->GetSigWidth()));
				}
                break;
            } else if (pOp2->IsConstWidth()) {

				if (pOp2->IsConstValue()) {
					if (pOp2->GetConstValue().GetSint64() < 0) {
						pExpr->SetMinWidth(pOp1->GetMinWidth());
						pExpr->SetSigWidth(pOp1->GetSigWidth());
					} else {
						int op2SigWidth = pOp2->GetConstValue().GetSigWidth();
						if (pOp2->IsSigned())
							op2SigWidth -= 1;

						pExpr->SetMinWidth(min(pOp1->GetMinWidth(), op2SigWidth));
						pExpr->SetSigWidth(min(pOp1->GetSigWidth(), op2SigWidth));
					}
				} else {
					pExpr->SetMinWidth(min(pOp1->GetMinWidth(), pOp2->GetSigWidth()));
					pExpr->SetSigWidth(min(pOp1->GetSigWidth(), pOp2->GetSigWidth()));
				}
                break;
            } else {
                pExpr->SetMinWidth(min(pOp1->GetMinWidth(), pOp2->GetMinWidth()));
                pExpr->SetSigWidth(min(pOp1->GetSigWidth(), pOp2->GetSigWidth()));
                break;
            }

            break;
        case tk_slash:
            {
                if (pOp1->IsUnknownWidth() || pOp2->IsUnknownWidth())
                    break;

                if (pOp2->IsConstValue()) {
                    // determine maximum number of bits for result

                    if (pOp2->GetConstValue().IsNeg())
                        ParseMsg(PARSE_ERROR, pExpr->GetLineInfo(), "division by a negative value");
                    else if (pOp2->GetConstValue().GetSint64() == 0)
                        ParseMsg(PARSE_ERROR, pExpr->GetLineInfo(), "division by zero");

                    else if (pOp1->IsSigned()) {
                        uint64_t maxValue = ((1ull << pOp1->GetMinWidth())-1u) / pOp2->GetConstValue().GetUint64();
                        CConstValue v(maxValue);
                        pExpr->SetMinWidth(v.GetMinWidth()+1);

                        maxValue = ((1ull << pOp1->GetSigWidth())-1u) / pOp2->GetConstValue().GetUint64();
                        CConstValue v2(maxValue);
                        pExpr->SetSigWidth(v2.GetMinWidth()+1);

                    } else {
                        uint64_t maxValue = ((1ull << pOp1->GetMinWidth())-1u) / pOp2->GetConstValue().GetUint64();
                        CConstValue v(maxValue);
                        pExpr->SetMinWidth(v.GetMinWidth());

                        maxValue = ((1ull << pOp1->GetSigWidth())-1u) / pOp2->GetConstValue().GetUint64();
                        CConstValue v2(maxValue);
                        pExpr->SetSigWidth(v2.GetMinWidth());
                    }
                    break;
                }

                pExpr->SetMinWidth(pOp1->GetMinWidth());
                pExpr->SetSigWidth(pOp1->GetSigWidth());
            }
            break;
        case tk_plus:
        case tk_minus:

            if (pOp1->IsUnknownWidth() || pOp2->IsUnknownWidth())
                break;

            pExpr->SetMinWidth(max(pOp1->GetMinWidth(), pOp2->GetMinWidth()));
            pExpr->SetSigWidth(max(pOp1->GetSigWidth(), pOp2->GetSigWidth())+1);

            break;
        case tk_asterisk:	// multiply
            {
                if (pOp1->IsUnknownWidth() || pOp2->IsUnknownWidth())
                    break;

                if (pOp1->IsConstValue()) {

                    int op1Width = pOp1->GetMinWidth();

                    if (pOp1->IsSigned())
                        op1Width -= 1;

                    int64_t op1Value = pOp1->GetConstValue().GetSint64();
                    if (op1Value < 0) op1Value = -op1Value;
                    if (((op1Value-1) & op1Value) == 0)
                        op1Width -= 1;	// Power of 2 multiply is really a shift

                    pExpr->SetMinWidth(pOp2->GetMinWidth() + op1Width);
                    pExpr->SetSigWidth(pOp2->GetSigWidth() + op1Width);
                    break;
                }

                if (pOp2->IsConstValue()) {

                    int op2Width = pOp2->GetMinWidth();

                    if (pOp2->IsSigned())
                        op2Width -= 1;

                    int64_t op2Value = pOp2->GetConstValue().GetSint64();
                    if (op2Value < 0) op2Value = -op2Value;
                    if (((op2Value-1) & op2Value) == 0)
                        op2Width -= 1;	// Power of 2 multiply is really a shift

                    pExpr->SetMinWidth(pOp1->GetMinWidth() + op2Width);
                    pExpr->SetSigWidth(pOp1->GetSigWidth() + op2Width);
                    break;
                }

                pExpr->SetMinWidth(pOp1->GetMinWidth() + pOp2->GetMinWidth());
                pExpr->SetSigWidth(pOp1->GetSigWidth() + pOp2->GetSigWidth());
            }
            break;
        case tk_percent:	// modulo
            {
                if (pOp1->IsUnknownWidth() || pOp2->IsUnknownWidth())
                    break;

                int op2Width = pOp2->GetMinWidth();
                if (pOp2->IsConstValue()) {

                    if (pOp2->IsSigned())
                        op2Width -= 1;

                    int64_t op2Value = pOp2->GetConstValue().GetSint64();
                    if (((op2Value-1) & op2Value) == 0)
                        op2Width -= 1;	// Power of 2 modulo is really a mask

                    pExpr->SetMinWidth(min(pOp1->GetMinWidth(), op2Width));
                    pExpr->SetSigWidth(min(pOp1->GetSigWidth(), op2Width));

                } else {
                    pExpr->SetMinWidth(min(pOp1->GetMinWidth(), op2Width));
                    pExpr->SetSigWidth(min(pOp1->GetSigWidth(), pOp2->GetSigWidth()));
                }
            }
            break;
        case tk_tilda:
        case tk_unaryMinus:
        case tk_unaryPlus:
        case tk_preInc:
        case tk_preDec:
        case tk_postInc:
        case tk_postDec:

            if (pOp1->IsUnknownWidth())
                break;

            pExpr->SetMinWidth(pOp1->GetMinWidth());
            pExpr->SetSigWidth(pOp1->GetSigWidth());
            break;
        case tk_greaterGreater:	// shift right

            if (pOp2->IsConstValue()) {

                if (pOp2->GetConstValue().IsNeg())
                    ParseMsg(PARSE_ERROR, pExpr->GetLineInfo(), "shift by negative constant");

                pExpr->SetMinWidth((int)(pOp1->GetMinWidth() - pOp2->GetConstValue().GetSint64()));
                pExpr->SetSigWidth((int)(pOp1->GetSigWidth() - pOp2->GetConstValue().GetSint64()));
            } else {
                pExpr->SetMinWidth(pOp1->GetMinWidth());
                pExpr->SetSigWidth(pOp1->GetSigWidth());
            }

            break;
        case tk_lessLess:	// shift left
            //pExpr->SetIsSigned(pOp1->IsSigned());

            if (pOp2->IsConstValue()) {

                if (pOp2->GetConstValue().IsNeg())
                    ParseMsg(PARSE_ERROR, pExpr->GetLineInfo(), "shift by negative constant");

                pExpr->SetMinWidth((int)(pOp1->GetMinWidth() + pOp2->GetConstValue().GetSint64()));
                pExpr->SetSigWidth((int)(pOp1->GetSigWidth() + pOp2->GetConstValue().GetSint64()));
            } else {
                pExpr->SetMinWidth(pOp1->GetMinWidth() + ((1 << pOp2->GetMinWidth())-1));
                pExpr->SetSigWidth(pOp1->GetSigWidth() + ((1 << pOp2->GetSigWidth())-1));
            }

            break;
        case tk_typeCast:
            pExpr->SetMinWidth(pOp1->GetMember()->GetWidth());

			if (pExpr->GetOperand1()->GetMember() == m_pBoolType)
	            InsertCompareOperator(pExpr->GetOperand2());

            break;
        case tk_comma:

            if (pOp1->IsUnknownWidth() || pOp2->IsUnknownWidth())
                break;

            pExpr->SetMinWidth(pOp1->GetMinWidth() + pOp2->GetMinWidth());
            pExpr->SetSigWidth(pOp1->GetSigWidth() + pOp2->GetSigWidth());

            break;
        default:
            Assert(0);
        }
    }

    if (bForceBoolean)
        InsertCompareOperator(pExpr);
}

char * CHtfeDesign::GetParseMsgWidthStr(CHtfeOperand *pOp, char *pBuf)
{
    if (pOp->GetMinWidth() == pOp->GetSigWidth())
        sprintf(pBuf, "%d", pOp->GetMinWidth());
    else
        sprintf(pBuf, "%d+%d", pOp->GetMinWidth(), pOp->GetSigWidth() - pOp->GetMinWidth());

    return pBuf;
}

bool CHtfeDesign::FoldConstantExpr(CHtfeOperand *pExpr)
{
    // recursively walk expression tree reducing constant expressions
    if (pExpr->IsLeaf()) {

        // fold constants for parameters and indecies
        vector<CHtfeOperand *> &paramList = pExpr->GetParamList();
        for (unsigned int i = 0; i < paramList.size(); i += 1)
            FoldConstantExpr(paramList[i]);

        return pExpr->IsConstValue();
    }

	if (pExpr->GetOperator() == tk_comma)
		return false;

    CHtfeOperand *pOp1 = pExpr->GetOperand1();
    CHtfeOperand *pOp2 = pExpr->GetOperand2();
    CHtfeOperand *pOp3 = pExpr->GetOperand3();

    bool op1Const = false;
    bool op2Const = false;

    if (pOp1)
        op1Const = FoldConstantExpr(pOp1);
    if (pOp2)
        op2Const = FoldConstantExpr(pOp2);
    if (pOp3)
        FoldConstantExpr(pOp3);

    if (IsConstCondExpression(pOp1) && pOp2 && IsConstCondExpression(pOp2) &&
        IsExpressionEqual(pOp1->GetOperand1(), pOp2->GetOperand1()) && !pOp3) {
            // two constant conditional expressions with same condition
            MergeConstCondExpression(pExpr);

            return false;
    }

    if (op1Const && pOp2 && IsConstCondExpression(pOp2) && !pOp3) {
        PushConstCondExpression(pExpr, pOp1, pOp2, false);
        return false;
    }

    if (op2Const && pOp1 && IsConstCondExpression(pOp1) && !pOp3) {
        PushConstCondExpression(pExpr, pOp1, pOp2, true);
        return false;
    }

    EToken tk = pExpr->GetOperator();
    CConstValue rslt;

    if (tk == tk_unaryPlus || tk == tk_unaryMinus || tk == tk_tilda ||
        tk == tk_bang || tk == tk_indirection || tk == tk_addressOf || tk == tk_new
        || tk == tk_preInc || tk == tk_postInc
		|| tk == tk_preDec || tk == tk_postDec) {

            Assert(pOp1 != 0 && pOp2 == 0 && pOp3 == 0);

            if (!pOp1->IsConstValue())
                return false;

            switch (tk) {
                case tk_unaryPlus:	rslt = pOp1->GetConstValue();	break;
                case tk_unaryMinus: rslt = - pOp1->GetConstValue(); break;
                case tk_tilda:		rslt = ~ pOp1->GetConstValue(); break;
                case tk_bang:		rslt = ! pOp1->GetConstValue(); break;
				case tk_preInc:
				case tk_postInc:
				case tk_preDec:
				case tk_postDec:
					ParseMsg(PARSE_ERROR, "++/-- operators can not be applied to constant values");
					return false;
                default:
                    return false;
            }

            //delete pOp1;

            pExpr->SetIsLeaf();
            pExpr->SetOperand1(0);
            pExpr->SetConstValue(rslt);

    } else if (tk != tk_question) {

        Assert(pOp1 != 0 && pOp2 != 0 && pOp3 == 0);

		bool bError = false;
		if (tk == tk_slash && pOp2->IsConstValue() && pOp2->GetConstValue().IsZero()) {
			ParseMsg(PARSE_ERROR, "division by zero");
			rslt = 0;
			bError = true;
		}

		if ((tk == tk_lessLess || tk == tk_greaterGreater) && pOp2->IsConstValue()) {
			int op1Width = pOp1->GetOpWidth();
			if (op1Width <= 32)
				op1Width = 32;
			else if (op1Width <= 64)
				op1Width = 64;
			int64_t op2Value = pOp2->GetConstValue().GetSint64();
			if (op1Width < op2Value || op2Value < 0) {
				ParseMsg(PARSE_WARNING, "shift amount is too large or negative, shift forced to zero");
				rslt = 0;
				bError = true;
			}
		}

		if (!bError) {
			if (!pOp1->IsConstValue() && tk != tk_typeCast || !pOp2->IsConstValue())
				return false;

			if (tk == tk_typeCast && pOp1->GetWidth() > 64)
				return false;
				
			switch (tk) {
				case tk_plus:				rslt = pOp1->GetConstValue() + pOp2->GetConstValue(); break;
				case tk_minus:				rslt = pOp1->GetConstValue() - pOp2->GetConstValue(); break;
				case tk_slash:				rslt = pOp1->GetConstValue() / pOp2->GetConstValue(); break;
				case tk_asterisk:			rslt = pOp1->GetConstValue() * pOp2->GetConstValue(); break;
				case tk_percent:			rslt = pOp1->GetConstValue() % pOp2->GetConstValue(); break;
				case tk_vbar:				rslt = pOp1->GetConstValue() | pOp2->GetConstValue(); break;
				case tk_carot:				rslt = pOp1->GetConstValue() ^ pOp2->GetConstValue(); break;
				case tk_ampersand:			rslt = pOp1->GetConstValue() & pOp2->GetConstValue(); break;
				case tk_lessLess:			rslt = pOp1->GetConstValue() << pOp2->GetConstValue(); break;
				case tk_greaterGreater:		rslt = pOp1->GetConstValue() >> pOp2->GetConstValue(); break;
				case tk_equalEqual:			rslt = pOp1->GetConstValue() == pOp2->GetConstValue(); break;
				case tk_vbarVbar:			rslt = pOp1->GetConstValue() || pOp2->GetConstValue(); break;
				case tk_ampersandAmpersand:	rslt = pOp1->GetConstValue() && pOp2->GetConstValue(); break;
				case tk_bangEqual:			rslt = pOp1->GetConstValue() != pOp2->GetConstValue(); break;
				case tk_less:				rslt = pOp1->GetConstValue() < pOp2->GetConstValue(); break;
				case tk_greater:			rslt = pOp1->GetConstValue() > pOp2->GetConstValue(); break;
				case tk_greaterEqual:		rslt = pOp1->GetConstValue() >= pOp2->GetConstValue(); break;
				case tk_lessEqual:			rslt = pOp1->GetConstValue() <= pOp2->GetConstValue(); break;
				case tk_equal:
				case tk_ampersandEqual:
				case tk_vbarEqual:
				case tk_plusEqual:
				case tk_minusEqual:
				case tk_lessLessEqual:
				case tk_greaterGreaterEqual:
				case tk_carotEqual:
				case tk_asteriskEqual:
				case tk_slashEqual:
				case tk_percentEqual:
					Assert(0);  // left side of equal was not an identifier
					return false;
				case tk_typeCast:
					Assert(pOp1->GetType() != 0);
					rslt = pOp2->GetConstValue().Cast(pOp1->GetType()->IsSigned(), pOp1->GetType()->GetWidth());
					break;
				case tk_comma: // concatenation
					ParseMsg(PARSE_ERROR, "concatenation of constants not supported");
					rslt = CConstValue(0);
					break;
				default:
					Assert(0);
			}
		}

        //delete pOp1;
        //delete pOp2;

        pExpr->SetIsLeaf();
        pExpr->SetOperand1(0);
        pExpr->SetOperand2(0);
        pExpr->SetConstValue(rslt);

    } else {
        // tk_question

        Assert(pOp1 != 0 && pOp2 != 0 && pOp3 != 0);

        if (!pOp1->IsConstValue() || !pOp2->IsConstValue() || !pOp3->IsConstValue())
            return false;

        rslt = pOp1->GetConstValue() != 0 ? pOp2->GetConstValue() : pOp3->GetConstValue();

        //delete pOp1;
        //delete pOp2;
        //delete pOp3;

        pExpr->SetOperand1(0);
        pExpr->SetOperand2(0);
        pExpr->SetOperand3(0);

        pExpr->SetIsLeaf();
        pExpr->SetConstValue(rslt);
    }

    return true;
}

bool CHtfeDesign::EvalConstantExpr(CHtfeOperand *pExpr, CConstValue & rslt, bool bMarkAsRead)
{
    // recursively walk expression tree evaluating constant expression
    //   variables that have previously been assigned a constant value are treated as constants
    //   expression tree is not modified
    if (pExpr->IsLeaf()) {

        if (pExpr->IsVariable() && pExpr->GetMember()->IsConstVar()) {
            pExpr->GetMember()->SetConstVarRead(true);
            rslt = pExpr->GetMember()->GetConstValue();
            return true;
        }

        if (pExpr->IsConstValue())
            rslt = pExpr->GetConstValue();

        return pExpr->IsConstValue();
    }

    CHtfeOperand *pOp1 = pExpr->GetOperand1();
    CHtfeOperand *pOp2 = pExpr->GetOperand2();
    CHtfeOperand *pOp3 = pExpr->GetOperand3();

    bool op1Const = false;
    bool op2Const = false;
    bool op3Const = false;

    CConstValue op1Value, op2Value, op3Value;

    if (pOp1)
        op1Const = EvalConstantExpr(pOp1, op1Value, bMarkAsRead);
    if (pOp2)
        op2Const = EvalConstantExpr(pOp2, op2Value, bMarkAsRead);
    if (pOp3)
        op3Const = EvalConstantExpr(pOp3, op3Value, bMarkAsRead);

    EToken tk = pExpr->GetOperator();

    if (tk == tk_unaryPlus || tk == tk_unaryMinus || tk == tk_tilda ||
        tk == tk_bang || tk == tk_indirection || tk == tk_addressOf || tk == tk_new
        || tk == tk_preInc || tk == tk_postInc
		|| tk == tk_preDec || tk == tk_postDec) {

            Assert(pOp1 != 0 && pOp2 == 0 && pOp3 == 0);

            if (!op1Const)
                return false;

            switch (tk) {
                case tk_unaryPlus:	rslt = op1Value;	break;
                case tk_unaryMinus: rslt = - op1Value; break;
                case tk_tilda:		rslt = ~ op1Value; break;
                case tk_bang:		rslt = ! op1Value; break;
				case tk_preInc:
				case tk_postInc:
				case tk_preDec:
				case tk_postDec:
                default:
                    return false;
            }

            return true;

    } else if (tk != tk_question) {

        Assert(pOp1 != 0 && pOp2 != 0 && pOp3 == 0);

        if (!op1Const && tk != tk_typeCast || !op2Const)
            return false;

        switch (tk) {
            case tk_plus:				rslt = op1Value + op2Value; break;
            case tk_minus:				rslt = op1Value - op2Value; break;
            case tk_slash:				rslt = op1Value / op2Value; break;
            case tk_asterisk:			rslt = op1Value * op2Value; break;
            case tk_percent:			rslt = op1Value % op2Value; break;
            case tk_vbar:				rslt = op1Value | op2Value; break;
            case tk_carot:				rslt = op1Value ^ op2Value; break;
            case tk_ampersand:			rslt = op1Value & op2Value; break;
            case tk_lessLess:			rslt = op1Value << op2Value; break;
            case tk_greaterGreater:		rslt = op1Value >> op2Value; break;
            case tk_equalEqual:			rslt = op1Value == op2Value; break;
            case tk_vbarVbar:			rslt = op1Value || op2Value; break;
            case tk_ampersandAmpersand:	rslt = op1Value && op2Value; break;
            case tk_bangEqual:			rslt = op1Value != op2Value; break;
            case tk_less:				rslt = op1Value < op2Value; break;
            case tk_greater:			rslt = op1Value > op2Value; break;
            case tk_greaterEqual:		rslt = op1Value >= op2Value; break;
            case tk_lessEqual:			rslt = op1Value <= op2Value; break;
            case tk_equal:
            case tk_ampersandEqual:
            case tk_vbarEqual:
            case tk_plusEqual:
            case tk_minusEqual:
            case tk_lessLessEqual:
            case tk_greaterGreaterEqual:
            case tk_carotEqual:
            case tk_asteriskEqual:
            case tk_slashEqual:
            case tk_percentEqual:
                return false;
            case tk_typeCast:
                Assert(pOp1->GetType() != 0);

				if (!pOp1->GetType()->IsSigned())
					rslt = op2Value.CastAsUint(pOp1->GetType()->GetWidth());
				else
					rslt = op2Value;
                break;
            case tk_comma: // concatenation
                return false;
            default:
                Assert(0);
        }

        return true;

    } else {
        // tk_question

        Assert(pOp1 != 0 && pOp2 != 0 && pOp3 != 0);

        if (!op1Const || !op2Const || !op3Const)
            return false;

        rslt = op1Value != 0 ? op2Value : op3Value;

        return true;
    }

    return true;
}

bool CHtfeDesign::IsConstCondExpression(CHtfeOperand *pExpr)
{
    if (pExpr->IsLeaf() || pExpr->GetOperator() != tk_question)
        return false;

    CHtfeOperand *pOp2 = pExpr->GetOperand2();
    CHtfeOperand *pOp3 = pExpr->GetOperand3();

    return (pOp2->IsConstValue() || IsConstCondExpression(pOp2)) &&
        (pOp3->IsConstValue() || IsConstCondExpression(pOp3));
}

bool CHtfeDesign::IsExpressionEqual(CHtfeOperand *pOp1, CHtfeOperand *pOp2)
{
    if (pOp1->IsConstValue() && pOp2->IsConstValue() && (pOp1->GetConstValue() == pOp2->GetConstValue()) != 0)
        return true;

    if (pOp1->IsVariable() && pOp2->IsVariable() && pOp1->GetMember() == pOp2->GetMember() &&
        pOp1->GetMinWidth() == pOp2->GetMinWidth()) {
            vector<CHtfeOperand *> paramList1 = pOp1->GetParamList();
            vector<CHtfeOperand *> paramList2 = pOp2->GetParamList();
            if (paramList1.size() == paramList2.size() && (paramList1.size() == 0 ||
                (paramList1[0]->IsConstValue() && paramList2[0]->IsConstValue() &&
                (paramList1[0]->GetConstValue() == paramList2[0]->GetConstValue()) != 0))) {
                    // conditions are equal
                    return true;
            }
    }

    // either not equal, or more complex than we currently deal with
    return false;
}

void CHtfeDesign::PushConstCondExpression(CHtfeOperand *pExpr, CHtfeOperand *pOp1, CHtfeOperand *pOp2, bool bOp1Cond)
{
    EToken token = pExpr->GetOperator();

    if (bOp1Cond) {

        pExpr->SetOperator(pOp1->GetOperator());
        pExpr->SetOperand1(pOp1->GetOperand1());

        // operand2
        CHtfeOperand *pOperand = HandleNewOperand();
		pOperand->InitAsConstant(pOp2->GetLineInfo(), pOp2->GetConstValue());

        CHtfeOperand *pOperator = HandleNewOperand();
		pOperator->InitAsOperator(pOp1->GetLineInfo(), token, pOp1->GetOperand2(), pOperand);

        pExpr->SetOperand2(pOperator);

        // operand3
        pOp1->SetOperator(token);
        pOp1->SetOperand1(pOp1->GetOperand3());
        pOp1->SetOperand2(pOp2);
        pOp1->SetOperand3(0);

        pExpr->SetOperand3(pOp1);

    } else {

        pExpr->SetOperator(pOp2->GetOperator());
        pExpr->SetOperand1(pOp2->GetOperand1());

        // operand2
        CHtfeOperand *pOperand = HandleNewOperand();
		pOperand->InitAsConstant(pOp1->GetLineInfo(), pOp1->GetConstValue());

        CHtfeOperand *pOperator = HandleNewOperand();
		pOperator->InitAsOperator(pOp2->GetLineInfo(), token, pOperand, pOp2->GetOperand2());

        pExpr->SetOperand2(pOperator);

        // operand3
        pOp2->SetOperator(token);
        pOp2->SetOperand1(pOp1);
        pOp2->SetOperand2(pOp2->GetOperand3());
        pOp2->SetOperand3(0);

        pExpr->SetOperand3(pOp2);
    }

    FoldConstantExpr(pExpr);
}

void CHtfeDesign::MergeConstCondExpression(CHtfeOperand *pExpr)
{
    EToken token = pExpr->GetOperator();
    CHtfeOperand *pOp1 = pExpr->GetOperand1();
    CHtfeOperand *pOp2 = pExpr->GetOperand2();

    *pExpr = *pOp1;

    CHtfeOperand *pNewOp2 = HandleNewOperand();
	pNewOp2->InitAsOperator(pOp1->GetLineInfo(), token, pOp1->GetOperand2(), pOp2->GetOperand2());
    pExpr->SetOperand2(pNewOp2);

    CHtfeOperand *pNewOp3 = HandleNewOperand();
	pNewOp3->InitAsOperator(pOp2->GetLineInfo(), token, pOp1->GetOperand3(), pOp2->GetOperand3());
    pExpr->SetOperand3(pNewOp3);

    // delete extra CHtfeOperand's
    delete pOp2->GetOperand1();
    pOp1->SetOperand1(0);
    pOp1->SetOperand2(0);
    pOp1->SetOperand3(0);
    pOp2->SetOperand1(0);
    pOp2->SetOperand2(0);
    pOp2->SetOperand3(0);
    delete pOp1;
    delete pOp2;
}

void CHtfeDesign::GetExpressionWidth(CHtfeOperand *pExpr, int &minWidth, int &sigWidth, int &setWidth, int &opWidth)
{
    // Algorithm
    //  1. Find minimum, significant and maximum width for expression
    //	minimum: Set by minimum width of an operand
    //	significant: Set by minimum width plus extra width due to operations
    //	maximum: Set by left hand side of assignment
	//  operation width: Set by looking at operator and operand widths
    //  2. Decend tree and assign each node a width

    if (pExpr->GetVerWidth() > 0) {
        minWidth = pExpr->GetVerWidth();
        opWidth = pExpr->GetOpWidth();

    } else if (pExpr->IsLeaf()) {

		if (pExpr->IsConstValue()) {
            minWidth = pExpr->GetMinWidth();
			sigWidth = pExpr->GetSigWidth();
			opWidth = pExpr->GetConstValue().Is32Bit() ? 32 : 64;
		} else {
            CHtfeIdent *pVar = pExpr->GetMember();
            Assert(pVar);
			if (pVar->IsType() || pVar->IsFunction()) {

                minWidth = pVar->GetWidth();
			} else {
				minWidth = pExpr->GetSubFieldWidth();
				sigWidth = minWidth;
				setWidth = minWidth;
            }

            if (pExpr->GetSubField() == 0 || pExpr->GetSubField()->m_pIdent == 0)
                opWidth = pExpr->GetType()->GetOpWidth();
            else {
                CScSubField * pSubField = pExpr->GetSubField();
                while (pSubField->m_pNext && pSubField->m_pNext->m_pIdent)
                    pSubField = pSubField->m_pNext;
                opWidth = pSubField->m_pIdent->GetType()->GetOpWidth();
            }
        }
    } else {
        // operator
        CHtfeOperand *pOp1 = pExpr->GetOperand1();
        CHtfeOperand *pOp2 = pExpr->GetOperand2();
        CHtfeOperand *pOp3 = pExpr->GetOperand3();

        int minTmp=0;
        int sigTmp=0;
        int setTmp=0;
		int opTmp=0;

        switch (pExpr->GetOperator()) {
            case tk_unaryPlus:
            case tk_unaryMinus:
			case tk_preInc:
			case tk_preDec:
			case tk_postInc:
			case tk_postDec:
            case tk_tilda:
                GetExpressionWidth(pOp1, minWidth, sigWidth, setWidth, opWidth);
                break;
            case tk_equal:
            case tk_typeCast:
                GetExpressionWidth(pOp1, minWidth, sigWidth, setWidth, opWidth);
                setWidth = minWidth;
                break;
            case tk_plus:
            case tk_minus:
                GetExpressionWidth(pOp1, minWidth, sigWidth, setWidth, opWidth);
                GetExpressionWidth(pOp2, minTmp, sigTmp, setTmp, opTmp);

                minWidth = max(minWidth, minTmp);
                sigWidth = max(sigWidth, sigTmp);
                sigWidth = max(sigWidth, minWidth)+1;
                setWidth = max(setWidth, setTmp);
				opWidth = max(opWidth, opTmp);
                break;
			case tk_slash:
                GetExpressionWidth(pOp1, minWidth, sigWidth, setWidth, opWidth);
                GetExpressionWidth(pOp2, minTmp, sigTmp, setTmp, opTmp);

                minWidth = minWidth - minTmp + 1;
                setWidth = sigWidth;
 				opWidth = max(opWidth, opTmp);
               break;
            case tk_percent:
                GetExpressionWidth(pOp1, minWidth, sigWidth, setWidth, opWidth);
                GetExpressionWidth(pOp2, minTmp, sigTmp, setTmp, opTmp);

                minWidth = min(minWidth, minTmp);
                sigWidth = min(sigWidth, sigTmp);
                setWidth = minWidth;
				opWidth = max(opWidth, opTmp);
                break;
            case tk_ampersand:
                GetExpressionWidth(pOp1, minWidth, sigWidth, setWidth, opWidth);
                GetExpressionWidth(pOp2, minTmp, sigTmp, setTmp, opTmp);

				if (pOp1->IsConstWidth() && pOp2->IsConstWidth()) {
					pExpr->SetConstWidth(true);
					minWidth = min(sigWidth, sigTmp);
					sigWidth = minWidth;
					setWidth = minWidth;					
				} else if (pOp1->IsConstWidth()) {
					minWidth = min(sigWidth, minTmp);
					sigWidth = min(sigWidth, sigTmp);
					setWidth = sigWidth;
				} else if (pOp2->IsConstWidth()) {
					minWidth = min(minWidth, sigTmp);
					sigWidth = min(sigWidth, sigTmp);
					setWidth = sigWidth;
				} else {
					minWidth = min(minWidth, minTmp);
					sigWidth = min(sigWidth, sigTmp);
					setWidth = min(setWidth, setTmp);
				}
				opWidth = max(opWidth, opTmp);
				break;
            case tk_vbar:
            case tk_carot:
                GetExpressionWidth(pOp1, minWidth, sigWidth, setWidth, opWidth);
                GetExpressionWidth(pOp2, minTmp, sigTmp, setTmp, opTmp);

                minWidth = max(minWidth, minTmp);
                sigWidth = max(sigWidth, sigTmp);
                setWidth = max(setWidth, setTmp);
 				opWidth = max(opWidth, opTmp);
               break;
            case tk_question:
                GetExpressionWidth(pOp2, minWidth, sigWidth, setWidth, opWidth);
                GetExpressionWidth(pOp3, minTmp, sigTmp, setTmp, opTmp);

                minWidth = max(minWidth, minTmp);
                sigWidth = max(sigWidth, sigTmp);
                setWidth = max(setWidth, setTmp);
 				opWidth = max(opWidth, opTmp);
               break;
            case tk_equalEqual:
            case tk_bangEqual:
            case tk_lessEqual:
            case tk_less:
            case tk_greater:
            case tk_greaterEqual:
            case tk_bang:
            case tk_ampersandAmpersand:
            case tk_vbarVbar:
                minWidth = sigWidth = 1;
				opWidth = 1;
                break;
            case tk_comma:
                GetExpressionWidth(pOp1, minWidth, sigWidth, setWidth, opWidth);
                GetExpressionWidth(pOp2, minTmp, sigTmp, setTmp, opTmp);

                minWidth = minWidth + minTmp;
                sigWidth = sigWidth + sigTmp;
                setWidth = setWidth + setTmp;
  				opWidth = max(opWidth, opTmp);
              break;
            case tk_asterisk:
                GetExpressionWidth(pOp1, minWidth, sigWidth, setWidth, opWidth);
                GetExpressionWidth(pOp2, minTmp, sigTmp, setTmp, opTmp);

                minWidth = max(minWidth, minTmp);
                sigWidth = sigWidth + sigTmp;
                setWidth = setWidth + setTmp;
 				opWidth = max(opWidth, opTmp);
               break;
            case tk_greaterGreater:
                GetExpressionWidth(pOp1, minWidth, sigWidth, setWidth, opWidth);
                break;
            case tk_lessLess:
                GetExpressionWidth(pOp1, minWidth, sigWidth, setWidth, opWidth);

				if (pOp2->IsConstValue()) {
					minTmp = sigTmp = setTmp = (int)pOp2->GetConstValue().GetSint64();
					minWidth = minWidth + minTmp;
				} else {
					GetExpressionWidth(pOp2, minTmp, sigTmp, setTmp, opTmp);
					minTmp = sigTmp = setTmp = (1 << minTmp) - 1;
					minWidth = max(minWidth, minTmp);
				}

                sigWidth = sigWidth + sigTmp;
                setWidth = setWidth + setTmp;
                break;
            case tk_period:
                GetExpressionWidth(pOp2, minWidth, sigWidth, setWidth, opWidth);
                break;
            default:
                Assert(0);
        }
    }
}

void CHtfeDesign::InsertCompareOperator(CHtfeOperand *pExpr)
{
	if (pExpr->IsLeaf() && !pExpr->IsConstValue() && pExpr->IsFunction() && pExpr->GetType() == m_pVoidType) {
		ParseMsg(PARSE_ERROR, "function %s() with void return type used in expression", pExpr->GetMember()->GetName().c_str());
		return;
	}

    if (pExpr->IsConstValue() && !pExpr->GetConstValue().IsBinary()
        || pExpr->GetMinWidth() > 1 || pExpr->GetSigWidth() > 1)
    {
	    CHtfeOperand *pOrigExpr = HandleNewOperand();
	    *pOrigExpr = *pExpr;
	    if (!pOrigExpr->IsLeaf())
		    pOrigExpr->SetIsParenExpr();

	    CHtfeOperand *pZero = HandleNewOperand();
		pZero->InitAsConstant(pExpr->GetLineInfo(), CConstValue((uint64_t)0));

	    pExpr->Init();
	    pExpr->SetLineInfo(pOrigExpr->GetLineInfo());
	    pExpr->SetOperator(tk_bangEqual);
	    pExpr->SetOperand1(pOrigExpr);
	    pExpr->SetOperand2(pZero);
		pExpr->SetIsParenExpr();
    }
}
