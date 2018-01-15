/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
// SequenceRules.cpp: Check variable assignment sequence rules 
//
//////////////////////////////////////////////////////////////////////

#include "Htfe.h"
#include "HtfeDesign.h"

/******************************************************************
**  Sequence Rules
**   r_, m_ - have the values IsPreClk, IsPostClk
**            initial value of IsPreClk for clocked modules
**            initial value of IsPostClk for non-clocked modules
**   c_     - have the values IsUnknown, IsPreClk, IsPostClk, IsContinuous
**            if (written in clocked module)
**              initial value of IsPreClk for clocked modules
**              initial value of IsPostClk for non-clocked modules
**            else
**              initial value of IsContinuous for all modules
**   i_     - has the value IsContinuous
**   local c_ starts as IsUnknown for all modules
*/

void CHtfeDesign::CheckSequenceRules()
{
    // First check
    CHtfeIdentTblIter moduleIter(m_pTopHier->GetIdentTbl());
    for (moduleIter.Begin(); !moduleIter.End(); moduleIter++) {

        if (!moduleIter->IsModule() || !moduleIter->IsMethodDefined())
            continue;

        CHtfeIdentTblIter funcIter(moduleIter->GetIdentTbl());
        for (funcIter.Begin(); !funcIter.End(); funcIter++) {

            if (!funcIter->IsFunction() || !funcIter->IsMethod())
                continue;

            // A method is a function with a sensitivity list
            CHtfeOperand * pTop = HandleNewOperand();
			pTop->InitAsIdentifier(funcIter()->GetLineInfo(), funcIter());
            m_callStack.push_back(pTop);

            CheckSequenceRules(moduleIter(), funcIter());

            m_callStack.pop_back();

			delete pTop;
        }
    }		
}

void CHtfeDesign::CheckSequenceRules(CHtfeIdent *pModule, CHtfeIdent *pMethod)
{
    // Initialize global variable sequence state
    CHtfeIdentTblIter identIter(pModule->GetFlatIdentTbl());
    for (identIter.Begin(); !identIter.End(); identIter++) {

        CHtfeIdent *pIdent = identIter->GetThis();

        switch (pIdent->GetId()) {
        case CHtfeIdent::id_variable:
            if (pIdent->IsLocal())
                // local to function or method
                pIdent->InitSeqState(ss_Unknown);
            else if (pIdent->IsRegister())
                pIdent->InitSeqState(ss_PreClk);
            else
                // non-register declared globally
                pIdent->InitSeqState(ss_Continuous);
            break;
		case CHtfeIdent::id_enum:
		case CHtfeIdent::id_const:
			pIdent->InitSeqState(ss_Continuous);
            break;
        case CHtfeIdent::id_function:
            //pIdent->InitSeqState(ss_Function);
            break;
        case CHtfeIdent::id_enumType:
            pIdent->InitSeqState(ss_Unknown);
            break;
        case CHtfeIdent::id_class:
            break;
        default:
            Assert(0);
        }
    }

    // initialize method's local variable sequence state
    CHtfeIdentTblIter localIter(pMethod->GetFlatIdentTbl());
    for (localIter.Begin(); !localIter.End(); localIter++) {

        if (!localIter->IsLocal())
            continue;

        CHtfeIdent *pLocalIdent = localIter();
        pLocalIdent->SetAssigningMethod(0);

        switch (pLocalIdent->GetId()) {
        case CHtfeIdent::id_variable:
            pLocalIdent->InitSeqState(ss_Unknown);
            break;
        default:
            Assert(0);
        }
    }

    // Now scan statements checking usage of variables
    m_seqStateStack.clear();
    m_seqStateStack.push_back(ss_Unknown);
    CheckSequenceRules(pMethod, 0, pMethod->GetStatementList());
    m_seqStateStack.pop_back();
}

void CHtfeDesign::CheckSequenceRules(CHtfeIdent *pHier, CHtfeOperand * pObj, CHtfeStatement *pStatement)
{
    for ( ; pStatement; pStatement = pStatement->GetNext()) {

        switch (pStatement->GetStType()) {
        case st_compound:
            CheckSequenceRules(pHier, pObj, pStatement->GetCompound1());
            break;
        case st_assign:
            // check for method being called as a function
            {
                CHtfeOperand *pExpr = pStatement->GetExpr();
                if (pExpr->IsLeaf()) {
                    CHtfeIdent *pMember = pExpr->GetMember();
                    if (pMember && pMember->IsMethod())
                        break;
                }
            }
            m_seqStateStack.back() = ss_Unknown;
            CheckSequenceRules(pHier, pObj, pStatement->GetExpr());
            break;
        case st_if:
            m_seqStateStack.back() = ss_Unknown;
            CheckSequenceRules(pHier, pObj, pStatement->GetExpr());
            CheckSequenceRules(pHier, pObj, pStatement->GetCompound1());
            CheckSequenceRules(pHier, pObj, pStatement->GetCompound2());
            break;
        case st_switch:
            m_seqStateStack.back() = ss_Unknown;
            CheckSequenceRules(pHier, pObj, pStatement->GetExpr());
            CheckSequenceRules(pHier, pObj, pStatement->GetCompound1());
            break;
        case st_case:
            m_seqStateStack.back() = ss_Unknown;
            CheckSequenceRules(pHier, pObj, pStatement->GetExpr());
            CheckSequenceRules(pHier, pObj, pStatement->GetCompound1());
            break;
        case st_default:
            CheckSequenceRules(pHier, pObj, pStatement->GetCompound1());
            break;
        case st_break:
        case st_null:
            break;
        case st_return:
            if (pStatement->GetExpr() == 0)
                break;

            m_seqStateStack.back() = m_seqStateStack[m_seqStateStack.size()-2];
            CheckSequenceRules(pHier, pObj, pStatement->GetExpr());
            m_seqStateStack[m_seqStateStack.size()-2] = m_seqStateStack.back();
            break;
        default:
            Assert(0);
        }
    }
}

void CHtfeDesign::CheckSequenceRules(CHtfeIdent *pHier, CHtfeOperand * pObj, CHtfeOperand *pExpr, bool bIsLeftSideOfEqual, bool bIsParamExpr)
{
    // recursively desend expression tree
    Assert(pExpr != 0);
    if (pExpr->GetOperator() == tk_period) {

        if (pObj == 0) {
            CHtfeOperand * pOp1 = pExpr;
            while (pOp1->GetOperator() == tk_period)
                pOp1 = pOp1->GetOperand1();

            pObj = pOp1;
            Assert(pObj->IsVariable());
        }

        pExpr = pExpr->GetOperand2();
    }

    if (pExpr->IsLeaf()) {
        CHtfeIdent *pIdent = pExpr->GetMember();
        
        if (bIsLeftSideOfEqual) {
            // either a variable or a range function
            Assert(pIdent);
            //if (pIdent->GetId() == CHtfeIdent::id_function)
            //    pIdent = pExpr->GetObject();

            if (pObj && pIdent->GetPrevHier() && pIdent->GetPrevHier()->IsUserDefinedType()) {
                // handle reference to a member variable within a method
                pIdent = pObj->GetMember();
                pExpr = pObj;
            }

            Assert(pIdent && pIdent->GetId() == CHtfeIdent::id_variable);

            if (pIdent->IsRegister()) {
                if (m_seqStateStack.back() == ss_PostClk)
                    ParseMsg(PARSE_ERROR, pExpr->GetLineInfo(), "register assigned from a post-clock expression");

                pIdent->SetSeqState(ss_PostClk, pExpr);
            } else
                pIdent->SetSeqState(m_seqStateStack.back(), pExpr);

            if (pIdent->IsRegister())
                pIdent->SetClockIdent(pHier->GetClockIdent());

            if (pIdent->GetAssigningMethod() != 0) {
                if (pIdent->GetAssigningMethod() != pHier)
                    ParseMsg(PARSE_ERROR, pExpr->GetLineInfo(), "variable assigned in multiple methods, originally assigned in '%s'\n",
						pIdent->GetAssigningMethod()->GetName().c_str());
            } else
                pIdent->SetAssigningMethod(pHier);

            HtfeOperandList_t &indexList = pExpr->GetIndexList();
            for (size_t i = 0; i < indexList.size(); i += 1) {

                CHtfeOperand * pIndexOp = indexList[i];

                CheckSequenceRules(pHier, pObj, pIndexOp, false, false);
            }

            for (CScSubField *pSubField = pExpr->GetSubField() ; pSubField; pSubField = pSubField->m_pNext) {
                for (size_t i = 0; i < pSubField->m_indexList.size(); i += 1) {
                    CHtfeOperand * pIndexOp = pSubField->m_indexList[i];

                    CheckSequenceRules(pHier, pObj, pIndexOp, false, false);
                }
            }

        } else if (pIdent) {

            switch (pIdent->GetId()) {
            case CHtfeIdent::id_function:
                {
                    if (pIdent->IsScPrim()) {

                        bool bIsScClockedPrim = pIdent->IsScClockedPrim();

                        // check function parameters
                        ESeqState seqState = ss_Unknown;
                        vector<CHtfeOperand *> &paramList = pExpr->GetParamList();
                        for (unsigned int i = 0; i < paramList.size(); i += 1) {
                            bool bIsReadOnly = pIdent->GetParamIsConst(i);

                            if (bIsReadOnly) {
                                m_seqStateStack.back() = ss_Unknown;
                                CheckSequenceRules(pHier, pObj, paramList[i]);

                                CHtfeOperand *pOp = paramList[i];
                                if (pOp->IsConstValue())
                                    continue;

                                Assert(pOp && pOp->GetMember() && pOp->GetMember()->IsVariable());

                                CHtfeIdent *pReadIdent = pOp->GetMember();

                                if (m_seqStateStack.back() == ss_Unknown) {
                                    ParseMsg(PARSE_ERROR, pExpr->GetLineInfo(), "%s used before set", pReadIdent->GetName().c_str());
                                    for (size_t i = m_callStack.size()-1; pReadIdent->IsParam() && i > 0; i -= 1)
                                        ParseMsg(PARSE_INFO, m_callStack[i]->GetLineInfo(), " called from %s", m_callStack[i-1]->GetMember()->GetName().c_str());
                                
                                } else if (bIsScClockedPrim) {
                                    if (m_seqStateStack.back() == ss_PostClk)
				        if (pReadIdent->GetName() != "r_i_reset_hx")
					    ParseMsg(PARSE_ERROR, pExpr->GetLineInfo(), "%s, post clock input to clocked primative", pReadIdent->GetName().c_str());
                                } else {
                                    if (seqState == ss_Unknown || seqState == ss_Continuous)
                                        seqState = m_seqStateStack.back();
                                    else if (seqState != m_seqStateStack.back() && m_seqStateStack.back() != ss_Continuous)
                                        ParseMsg(PARSE_ERROR, pExpr->GetLineInfo(), "%s, inconsistent pre-clock / post-clock reference error", pReadIdent->GetName().c_str());
                                }
                            }
                        }

                        for (unsigned int i = 0; i < paramList.size(); i += 1) {
                            bool bIsWriteOnly = !pIdent->GetParamIsConst(i);

                            if (bIsWriteOnly && !pIdent->GetParamIsScState(i)) {
                                CHtfeOperand *pOp = paramList[i];
                                Assert(pOp && pOp->GetMember() && pOp->GetMember()->IsVariable());

                                CHtfeIdent *pWriteIdent = pOp->GetMember();

                                if (pIdent->IsScClockedPrim()) {

                                    if (!pWriteIdent->IsRegister()) {
                                        //ParseMsg(PARSE_ERROR, pExpr->GetLineInfo(), "clocked sc_prim outputs must be registers");

                                        pWriteIdent->SetSeqState(ss_PreClk, pOp);
                                    } else {
                                        if (pWriteIdent->GetSeqState(pOp) != ss_PreClk)
                                            ParseMsg(PARSE_ERROR, pExpr->GetLineInfo(), "%s assigned prior to sc_prim function", pWriteIdent->GetName().c_str());

                                        pWriteIdent->SetSeqState(ss_PostClk, pOp);
                                    }
                                } else {
                                    pWriteIdent->SetSeqState(seqState, pOp);

                                    if (pWriteIdent->IsRegister())
                                        ParseMsg(PARSE_ERROR, pExpr->GetLineInfo(), "non-clocked sc_prim outputs can not be registers");
                                }
                            }
                        }

                    } else if (!pIdent->IsBodyDefined() && !pIdent->GetType()->IsUserDefinedType()) {
                        // builtin function
                        // check function parameters
                        vector<CHtfeOperand *> &paramList = pExpr->GetParamList();
                        for (unsigned int i = 0; i < paramList.size(); i += 1) {

                            CheckSequenceRules(pHier, pObj, paramList[i], false, true);
                        }

                        pIdent = pObj->GetMember();
                        pExpr = pObj;

                        // check object sequence state
                        CheckSequenceRules(pIdent, pExpr);

                    } else {
                        if (!pIdent->IsBodyDefined() && pIdent->GetType()->IsUserDefinedType())
                            ParseMsg(PARSE_FATAL, pExpr->GetLineInfo(), "definition of referenced function was not found");

                        m_seqStateStack.push_back(ss_Unknown);

                        // initialize function's local variable sequence state
                        CHtfeIdentTblIter localIter(pIdent->GetFlatIdentTbl());
                        for (localIter.Begin(); !localIter.End(); localIter++) {

                            CHtfeIdent *pLocalIdent = localIter->GetThis();
                            pLocalIdent->SetAssigningMethod(0);

                            if (pLocalIdent->GetId() == CHtfeIdent::id_variable)
                                pLocalIdent->InitSeqState(ss_Unknown);
                        }

                        // check function parameters
                        vector<CHtfeOperand *> &paramList = pExpr->GetParamList();
                        for (unsigned int i = 0; i < paramList.size(); i += 1) {

                            m_seqStateStack.back() = ss_Unknown;
                            CheckSequenceRules(pHier, pObj, paramList[i], false, true);

                            // transfer sequence state to local variable
                            pIdent->GetParamIdent(i)->SetSeqState(m_seqStateStack.back(), 0);
                        }

                        CHtfeIdent *pFunc = pExpr->GetMember();

                        if (pFunc->IsSeqCheck()) {
                            ParseMsg(PARSE_ERROR, pExpr->GetLineInfo(), "recursive function call found '%s'", pFunc->GetName().c_str());
                            for (size_t i = m_callStack.size()-1; i > 0; i -= 1)
                                ParseMsg(PARSE_INFO, m_callStack[i]->GetLineInfo(), " called from %s", m_callStack[i-1]->GetMember()->GetName().c_str());
                        } else {
                            m_callStack.push_back(pExpr);
                            pFunc->SetIsSeqCheck(true);
                            CheckSequenceRules(pHier, pObj, pFunc->GetStatementList());
                            pFunc->SetIsSeqCheck(false);
                            m_callStack.pop_back();

                            // update variables from output parameters
                            for (unsigned int i = 0; i < paramList.size(); i += 1) {
                                bool bIsWriteOnly = !pIdent->GetParamIsConst(i);

                                if (bIsWriteOnly) {
                                    CHtfeOperand *pOp = paramList[i];
                                    Assert(pOp && pOp->GetMember() && pOp->GetMember()->IsVariable());

                                    ESeqState seqState = pIdent->GetParamIdent(i)->GetSeqState(pOp);

                                    CHtfeIdent *pWriteIdent = pOp->GetMember();

                                    if (pWriteIdent->IsRegister()) {
                                        if (seqState == ss_PostClk)
                                            ParseMsg(PARSE_ERROR, pExpr->GetLineInfo(), "register assigned from a post-clock parameter");

                                        pWriteIdent->SetSeqState(ss_PostClk, pOp);
                                    } else
                                        pWriteIdent->SetSeqState(seqState, pOp);
                                }
                            }
                        }

                        m_seqStateStack.pop_back();
                    }
                }
                break;
            case CHtfeIdent::id_variable:
                {
                    if (pObj && pIdent->GetPrevHier() && pIdent->GetPrevHier()->IsUserDefinedType()) {
                        // handle reference to a member variable within a method
                        pIdent = pObj->GetMember();
                        pExpr = pObj;
                    }

                    CheckSequenceRules(pIdent, pExpr, bIsParamExpr);

                    HtfeOperandList_t &indexList = pExpr->GetIndexList();
                    for (size_t i = 0; i < indexList.size(); i += 1) {

                        CHtfeOperand * pIndexOp = indexList[i];

                        CheckSequenceRules(pHier, pObj, pIndexOp, false, false);
                    }

                    for (CScSubField *pSubField = pExpr->GetSubField() ; pSubField; pSubField = pSubField->m_pNext) {
                        for (size_t i = 0; i < pSubField->m_indexList.size(); i += 1) {
                            CHtfeOperand * pIndexOp = pSubField->m_indexList[i];

                            CheckSequenceRules(pHier, pObj, pIndexOp, false, false);
                        }
                    }
                }
                break;
            case CHtfeIdent::id_enum:
            case CHtfeIdent::id_const:
                if (m_seqStateStack.back() == ss_Unknown)
                    m_seqStateStack.back() = ss_Continuous;
                break;
            default:
                Assert(0);
            }
        } else {
            Assert(pExpr->IsConstValue());

            if (m_seqStateStack.back() == ss_Unknown)
                m_seqStateStack.back() = ss_Continuous;
        }

    } else {

        CHtfeOperand *pOp1 = pExpr->GetOperand1();
        CHtfeOperand *pOp2 = pExpr->GetOperand2();
        CHtfeOperand *pOp3 = pExpr->GetOperand3();

        EToken tk = pExpr->GetOperator();

        if (tk == tk_equal) {

            CheckSequenceRules(pHier, pObj, pOp2);
            CheckSequenceRules(pHier, pObj, pOp1, true);

        } else if (tk == tk_preInc || tk == tk_preDec || tk == tk_postInc || tk == tk_postDec) {

            CheckSequenceRules(pHier, pObj, pOp1);
            CheckSequenceRules(pHier, pObj, pOp1, true);

        } else {
            if (pOp1 && tk != tk_typeCast)
                CheckSequenceRules(pHier, pObj, pOp1);
            if (pOp2)
                CheckSequenceRules(pHier, pObj, pOp2);
            if (pOp3)
                CheckSequenceRules(pHier, pObj, pOp3);
        }
    }
}

void CHtfeDesign::CheckSequenceRules(CHtfeIdent * pIdent, CHtfeOperand * pExpr, bool bIsParamExpr)
{
    ESeqState seqState = m_seqStateStack.back();

    switch (pIdent->GetSeqState(pExpr)) {
    case ss_Unknown:
        if (!pIdent->IsScState() && !bIsParamExpr) {
            ParseMsg(PARSE_ERROR, pExpr->GetLineInfo(), "%s used before set", pIdent->GetName().c_str());
            for (size_t i = m_callStack.size()-1; pIdent->IsParam() && i > 0; i -= 1)
                ParseMsg(PARSE_INFO, m_callStack[i]->GetLineInfo(), " called from %s", m_callStack[i-1]->GetMember()->GetName().c_str());
        }
        m_seqStateStack.back() = ss_Unknown;
        break;
    case ss_Continuous:
        if (seqState == ss_Unknown)
            m_seqStateStack.back() = ss_Continuous;
        break;
    case ss_Error:
        break;
    case ss_PreClk:
    case ss_PostClk:
        if (seqState != ss_Unknown && seqState != ss_Continuous && seqState != ss_Error
            && seqState != pIdent->GetSeqState(pExpr)) {
                ParseMsg(PARSE_ERROR, pExpr->GetLineInfo(), "inconsistent pre-clock / post-clock reference error, '%s'",
                    pIdent->GetName().c_str());
                m_seqStateStack.back() = ss_Error;
        } else
            m_seqStateStack.back() = pIdent->GetSeqState(pExpr);
        break;
    default:
        Assert(0);
    }
}
