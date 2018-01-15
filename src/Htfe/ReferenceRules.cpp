/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
// ReferenceRules.cpp: Check variable reference counts and sequence 
//
//////////////////////////////////////////////////////////////////////

#include "Htfe.h"
#include "HtfeDesign.h"

void CHtfeDesign::CheckReferenceRules()
{
    CHtfeOperand *pObj = 0;

	// Scan parsed structures and set reference info
	CHtfeIdentTblIter moduleIter(m_pTopHier->GetIdentTbl());
	for (moduleIter.Begin(); !moduleIter.End(); moduleIter++) {

		if (!moduleIter->IsModule() || !moduleIter->IsMethodDefined())
			continue;

		// first create always at list for each method in module
		CHtfeIdentTblIter methodIter(moduleIter->GetIdentTbl());
		for (methodIter.Begin(); !methodIter.End(); methodIter++) {

			if (!methodIter->IsMethod())
				continue;

			ClearRefInfo(moduleIter());
			SetRefInfo(pObj, moduleIter(), false, true, methodIter->GetStatementList());

			// now check if appropriate signals are in sensitivity list for instantiated methods
			CheckSensitivityList(moduleIter(), methodIter());
		}

		// check variables local to each function
		CHtfeIdentTblIter funcIter(moduleIter->GetIdentTbl());
		for (funcIter.Begin(); !funcIter.End(); funcIter++) {

			if (!funcIter->IsFunction() || funcIter->IsCtor())
				continue;

			// check function's local variable reference counts
			ClearRefInfo(funcIter());
			SetRefInfo(pObj, funcIter(), true, false, funcIter->GetStatementList());
			CheckRefCnts(true, funcIter());
		}

		// now check module reference counts
		ClearRefInfo(moduleIter());

		// first scan instantiated primitives
		CHtfeIdentTblIter varIter(moduleIter->GetFlatIdentTbl());
		for (varIter.Begin(); !varIter.End(); varIter++) {

			if (!varIter->IsVariable() || !varIter->IsReference())
				continue;

			// first check that the declared instance has a complete set of ports
			CheckPortList(varIter());
		}

		// next scan methods
		methodIter = moduleIter->GetIdentTbl();
		for (methodIter.Begin(); !methodIter.End(); methodIter++) {

			if (!methodIter->IsMethod())
				continue;

			if (methodIter->GetClockIdent())
				methodIter->GetClockIdent()->SetReadRef(0);

			SetRefInfo(pObj, methodIter(), false, false, methodIter->GetStatementList());
		}

		CheckRefCnts(false, moduleIter());

		// finally, set reference counts for synthesis
		ClearRefInfo(moduleIter());

		methodIter = moduleIter->GetIdentTbl();
		for (methodIter.Begin(); !methodIter.End(); methodIter++) {

			if (!methodIter->IsMethod())
				continue;

			if (methodIter->GetClockIdent())
				methodIter->GetClockIdent()->SetReadRef(0);

			SetRefInfo(pObj, methodIter(), false, true, methodIter->GetStatementList());
		}
	}		
}

void CHtfeDesign::ClearRefInfo(CHtfeIdent *pFunc)
{
	CHtfeIdentTblIter identIter(pFunc->GetFlatIdentTbl());
	for (identIter.Begin(); !identIter.End(); identIter++)
		identIter->ClearRefInfo();

	// walk through statements and clear function parameter references
	ClearRefInfo(pFunc->GetStatementList());
}

void CHtfeDesign::ClearRefInfo(CHtfeStatement * pStatement)
{
	// Count references within statements
	for ( ; pStatement != 0; pStatement = pStatement->GetNext()) {

		switch (pStatement->GetStType()) {
			case st_compound:
				ClearRefInfo(pStatement->GetCompound1());
				break;
			case st_return:
				if (pStatement->GetExpr() == 0)
					break;
				// else fall into st_assign
			case st_assign:
				if (!pStatement->IsInitStatement())
					ClearRefInfo(pStatement->GetExpr());
				break;
			case st_if:
				ClearRefInfo(pStatement->GetExpr());
				ClearRefInfo(pStatement->GetCompound1());
				ClearRefInfo(pStatement->GetCompound2());
				break;
			case st_switch:
			case st_case:
				ClearRefInfo(pStatement->GetExpr());
				ClearRefInfo(pStatement->GetCompound1());
				break;
			case st_default:
				ClearRefInfo(pStatement->GetCompound1());
				break;
			case st_null:
			case st_break:
				break;
			default:
				Assert(0);
		}
	}
}

void CHtfeDesign::ClearRefInfo(CHtfeOperand * pExpr)
{
	// recursively desend expression tree searching for called functions
	Assert(pExpr != 0);
	if (pExpr->IsLeaf()) {
		CHtfeIdent *pIdent = pExpr->GetMember();

		if (!pIdent)
			return;

		pIdent->ClearRefInfo();

		if (pIdent->GetId() == CHtfeIdent::id_function) {

			// clear function parameter references
			vector<CHtfeOperand *> &argList = pExpr->GetParamList();
			for (unsigned int i = 0; i < argList.size(); i += 1)
				ClearRefInfo(argList[i]);
		}

	} else {

		CHtfeOperand *pOp1 = pExpr->GetOperand1();
		CHtfeOperand *pOp2 = pExpr->GetOperand2();
		CHtfeOperand *pOp3 = pExpr->GetOperand3();

		if (pOp1) ClearRefInfo(pOp1);
		if (pOp2) ClearRefInfo(pOp2);
		if (pOp3) ClearRefInfo(pOp3);
	}
}

void CHtfeDesign::SetRefInfo(CHtfeOperand * pObj, CHtfeIdent * pHier, bool bLocal, bool bAlwaysAt, CHtfeStatement *pStatement)
{
	// Count references within statements
	for ( ; pStatement != 0; pStatement = pStatement->GetNext()) {

		switch (pStatement->GetStType()) {
			case st_compound:
				SetRefInfo(pObj, pHier, bLocal, bAlwaysAt, pStatement->GetCompound1());
				break;
			case st_return:
				if (pStatement->GetExpr() == 0)
					break;
				// else fall into st_assign
			case st_assign:
				if (!pStatement->IsInitStatement())
					SetRefInfo(pObj, pHier, bLocal, bAlwaysAt, pStatement->GetExpr());
				break;
			case st_if:
				SetRefInfo(pObj, pHier, bLocal, bAlwaysAt, pStatement->GetExpr());
				SetRefInfo(pObj, pHier, bLocal, bAlwaysAt, pStatement->GetCompound1());
				SetRefInfo(pObj, pHier, bLocal, bAlwaysAt, pStatement->GetCompound2());
				break;
			case st_switch:
			case st_case:
				SetRefInfo(pObj, pHier, bLocal, bAlwaysAt, pStatement->GetExpr());
				SetRefInfo(pObj, pHier, bLocal, bAlwaysAt, pStatement->GetCompound1());
				break;
			case st_default:
				SetRefInfo(pObj, pHier, bLocal, bAlwaysAt, pStatement->GetCompound1());
				break;
			case st_null:
			case st_break:
				break;
			default:
				Assert(0);
		}
	}
}

void CHtfeDesign::SetRefInfo(CHtfeOperand * pObj, CHtfeIdent * pHier, bool bLocal, bool bAlwaysAt, CHtfeOperand *pExpr, bool bIsLeftSideOfEqual)
{
	// recursively desend expression tree incrementing reference counts
	Assert(pExpr != 0);
	if (pExpr->IsLeaf()) {
		CHtfeIdent *pIdent = pExpr->GetMember();

		vector<CHtfeOperand *> &argList = pExpr->GetParamList();

		if (bIsLeftSideOfEqual) {
			// either a variable or a range function
			if (pIdent == 0) {
				ParseMsg(PARSE_ERROR, pExpr->GetLineInfo(), "can not assign to a constant value");
				return;
			}

			Assert(pIdent && pIdent->GetId() == CHtfeIdent::id_variable);

            if (pObj && pIdent->GetPrevHier() && pIdent->GetPrevHier()->IsUserDefinedType()) {
                // handle reference to a member variable within a method
                pIdent = pObj->GetMember();
                pExpr = pObj;
            }


			bool bIdentIsLocalScope = pIdent->GetPrevHier() && pIdent->GetPrevHier()->GetPrevHier() != GetTopHier();

			int elemIdx = pIdent->GetDimenElemIdx(pExpr);
			if (!pIdent->IsHtPrimOutput(elemIdx) || bIdentIsLocalScope == bLocal) {
				if (pIdent->IsHtPrimOutput(elemIdx) && pIdent->IsWriteRef(elemIdx) && !pIdent->IsScState()) {
					if (bIdentIsLocalScope)
						ParseMsg(PARSE_ERROR, pExpr->GetLineInfo(), "primitive output variable can not have local scope, %s",
							pIdent->GetDimenElemName(elemIdx).c_str());
					else
						ParseMsg(PARSE_ERROR, pExpr->GetLineInfo(), "primitive output variable was written multiple times, %s",
							pIdent->GetDimenElemName(elemIdx).c_str());
				}
				pIdent->SetWriteRef(pExpr);
			}

		} else if (pIdent) {

            if (pIdent->IsVariable() || pIdent->IsIdConst()) {
                if (pIdent->GetPrevHier() && pIdent->GetPrevHier()->IsStruct()) {
                    // member of a class referenced within a method
                    Assert(pIdent->GetPrevHier() == pHier);
                    pObj->SetReadRef();

                } else
                    pIdent->SetReadRef(pExpr);
            }

			if (pIdent->GetId() == CHtfeIdent::id_function) {

				if (pIdent->IsUserDefinedFunc() && !pIdent->IsScPrim() && !pIdent->IsBodyDefined())
					ParseMsg(PARSE_FATAL, pIdent->GetLineInfo(), "user defined function '%s' was called but not defined",
						pIdent->GetName().c_str());

				if (!pIdent->IsMethod()) {
					if (!pIdent->IsFunctionCalled()) {
						// Count references in called function (but not function UpdateOutput)
						pIdent->SetIsFunctionCalled(true);
						SetRefInfo(pObj, pIdent->GetPrevHier(), false, bAlwaysAt, pIdent->GetStatementList());
						pIdent->SetIsFunctionCalled(false);
					}
				}

                // mark references made in function to parameters
                if (pIdent->IsScPrim()) {
				    // mark ScPrim parameter references
                    //  with scPrims, we don't have the body so we just mark references based on argument declaration type
				    for (unsigned int i = 0; i < argList.size(); i += 1) {
				    	bool bIsReadOnly = pIdent->GetParamIsConst(i) || !pIdent->GetParamIsRef(i);
				    	SetRefInfo(pObj, pHier, bLocal, bAlwaysAt, argList[i], !bIsReadOnly);
				    }
                } else {
                    // we have the body, and have scanned the statement list. Mark references based on actual usage
				    for (unsigned int i = 0; i < argList.size(); i += 1) {

						CHtfeIdent * pParamIdent = pIdent->GetParamIdent(i);

						if (pParamIdent == 0)
                            continue;

						int elemIdx = pParamIdent->GetDimenElemIdx(argList[i]);

						if (pParamIdent->IsReadRef(elemIdx))
					        SetRefInfo(pObj, pHier, bLocal, bAlwaysAt, argList[i], false);
						if (pParamIdent->IsWriteRef(elemIdx) && pIdent->GetParamIsRef(i))
					        SetRefInfo(pObj, pHier, bLocal, bAlwaysAt, argList[i], true);
				    }
                }
			}
		}

		// check if variable is bit indexed with a variable
		if (pIdent && pIdent->IsVariable() && argList.size() == 1)
			// bit index expression
			SetRefInfo(pObj, pHier, bLocal, bAlwaysAt, argList[0]);

		// check if variable is array indexed
		vector <CHtfeOperand *> &indexList = pExpr->GetIndexList();
		for (size_t i = 0; i < indexList.size(); i += 1)
			SetRefInfo(pObj, pHier, bLocal, bAlwaysAt, indexList[i]);

		// check if variable is field indexed
		CScSubField * pSubField = pExpr->GetSubField();
		for ( ; pSubField; pSubField = pSubField->m_pNext) {

			for (size_t i = 0; i < pSubField->m_indexList.size(); i += 1)
				SetRefInfo(pObj, pHier, bLocal, bAlwaysAt, pSubField->m_indexList[i]);
		}

	} else {

		CHtfeOperand *pOp1 = pExpr->GetOperand1();
		CHtfeOperand *pOp2 = pExpr->GetOperand2();
		CHtfeOperand *pOp3 = pExpr->GetOperand3();

        EToken tk = pExpr->GetOperator();

		if (tk == tk_equal) {
			SetRefInfo(pObj, pHier, bLocal, bAlwaysAt, pOp1, true);
			SetRefInfo(pObj, pHier, bLocal, bAlwaysAt, pOp2);

        } else if (tk == tk_preInc || tk == tk_preDec || tk == tk_postInc || tk == tk_postDec) {
            // variable is both read and written
			SetRefInfo(pObj, pHier, bLocal, bAlwaysAt, pOp1, true);
			SetRefInfo(pObj, pHier, bLocal, bAlwaysAt, pOp1);

		} else if (tk == tk_typeCast) {
			if (pOp2)
				SetRefInfo(pObj, pHier, bLocal, bAlwaysAt, pOp2);

		} else {
			if (pOp1)
				SetRefInfo(pObj, pHier, bLocal, bAlwaysAt, pOp1);

			bool bIsStructMember = pOp1->GetMember() && pOp1->GetMember()->GetPrevHier()
				&& pOp1->GetMember()->GetPrevHier()->IsUserDefinedType();

			CHtfeOperand * pObj2 = pObj;
			if (pExpr->GetOperator() == tk_period) {
				if (pExpr->GetOperand1()->GetOperator() == tk_period)
					pObj2 = pExpr->GetOperand1()->GetOperand2();
				else if (!bIsStructMember)
					pObj2 = pExpr->GetOperand1();
			}

			if (pOp2)
				SetRefInfo(pObj2, pHier, bLocal, bAlwaysAt, pOp2);
			if (pOp3)
				SetRefInfo(pObj, pHier, bLocal, bAlwaysAt, pOp3);
		}
	}
}

void CHtfeDesign::CheckRefCnts(bool bLocal, CHtfeIdent *pFunc)
{
	if (pFunc->GetIdentTbl().GetCount() == 0)
		return;

	CHtfeIdentTblIter identIter(pFunc->GetIdentTbl());
	for (identIter.Begin(); !identIter.End(); identIter++)
	{
		if (identIter->GetId() == CHtfeIdent::id_nameSpace) {
			CheckRefCnts(bLocal, identIter());
			continue;
		}

		if (identIter->IsLocal() != bLocal)
			continue;

		for (int elemIdx = 0; elemIdx < identIter->GetDimenElemCnt(); elemIdx += 1) {

			string name = identIter->GetDimenElemName(elemIdx);

			switch (identIter->GetPortDir()) {
			case port_in:
				if (!identIter->IsReadRef(elemIdx) && !identIter->IsNoLoad() && name != "i_clockhx")
					ParseMsg(PARSE_WARNING, identIter->GetLineInfo(), "port '%s' was not read", name.c_str());
				break;
			case port_out:
				if (!identIter->IsWriteRef(elemIdx))
					ParseMsg(PARSE_WARNING, identIter->GetLineInfo(), "port '%s' was not written", name.c_str());
				break;
			case port_inout:
				if (!identIter->IsReadRef(elemIdx) && !identIter->IsWriteRef(elemIdx))
					ParseMsg(PARSE_WARNING, identIter->GetLineInfo(), "port '%s' was not referenced", name.c_str());
				break;
			default:
				if (!identIter->IsVariable() || identIter->IsScState())
					break;

				if (identIter->IsHtQueue()) {
					if (!identIter->IsHtQueuePushSeen())
						ParseMsg(PARSE_WARNING, identIter->GetLineInfo(), "ht_%s_que '%s' does not call method push",
							identIter->IsHtBlockQue() ? "block" : "dist", name.c_str());
					if (!identIter->IsHtQueuePopSeen())
						ParseMsg(PARSE_WARNING, identIter->GetLineInfo(), "ht_%s_que '%s' does not call method pop",
							identIter->IsHtBlockQue() ? "block" : "dist", name.c_str());
					if (!identIter->IsHtQueueFrontSeen())
						ParseMsg(PARSE_WARNING, identIter->GetLineInfo(), "ht_%s_que '%s' does not call method front",
							identIter->IsHtBlockQue() ? "block" : "dist", name.c_str());
					if (!identIter->IsHtQueuePopClockSeen())
						ParseMsg(PARSE_ERROR, identIter->GetLineInfo(), "ht_%s_que '%s' does not call methods clock or pop_clock",
							identIter->IsHtBlockQue() ? "block" : "dist", name.c_str());
					if (!identIter->IsHtQueuePushClockSeen())
						ParseMsg(PARSE_ERROR, identIter->GetLineInfo(), "ht_%s_que '%s' does not call methods clock or push_clock",
							identIter->IsHtBlockQue() ? "block" : "dist", name.c_str());

				} else if (identIter->IsHtMemory()) {
					char const * pRamType = "";
					if (identIter->IsHtMrdBlockRam())
						pRamType = "_mrd";
					else if (identIter->IsHtMwrBlockRam())
						pRamType = "_mwr";

					if (!identIter->IsHtMemoryReadAddrSeen())
						ParseMsg(PARSE_WARNING, identIter->GetLineInfo(), "ht%s_%s_ram '%s' does not call method read_addr",
							pRamType, identIter->IsHtBlockRam() ? "block" : "dist", name.c_str());
					if (!identIter->IsHtMemoryReadMemSeen())
						ParseMsg(PARSE_WARNING, identIter->GetLineInfo(), "ht%s_%s_ram '%s' does not call method read_mem",
							pRamType,
							identIter->IsHtBlockRam() ? "block" : "dist", name.c_str());
					if (!identIter->IsHtMemoryReadClockSeen())
						ParseMsg(PARSE_ERROR, identIter->GetLineInfo(), "ht%s_%s_ram '%s' does not call methods clock or read_clock",
							pRamType,
							identIter->IsHtBlockRam() ? "block" : "dist", name.c_str());
					if (!identIter->IsHtMemoryWriteAddrSeen())
						ParseMsg(PARSE_WARNING, identIter->GetLineInfo(), "ht%s_%s_ram '%s' does not call method write_addr",
							pRamType,
							identIter->IsHtBlockRam() ? "block" : "dist", name.c_str());
					if (!identIter->IsHtMemoryWriteMemSeen())
						ParseMsg(PARSE_WARNING, identIter->GetLineInfo(), "ht%s_%s_ram '%s' does not call method write_mem",
							pRamType,
							identIter->IsHtBlockRam() ? "block" : "dist", name.c_str());
					if (!identIter->IsHtMemoryWriteClockSeen())
						ParseMsg(PARSE_ERROR, identIter->GetLineInfo(), "ht%s_%s_ram '%s' does not call methods clock or write_clock",
							pRamType,
							identIter->IsHtBlockRam() ? "block" : "dist", name.c_str());

				} else {

					if ((!identIter->IsReadRef(elemIdx) && !identIter->IsWriteOnly())
						&& (!identIter->IsWriteRef(elemIdx) && !identIter->IsReadOnly()))

						ParseMsg(PARSE_WARNING, identIter->GetLineInfo(), "'%s' was not referenced", name.c_str());

					else if (!identIter->IsReadRef(elemIdx) && !identIter->IsWriteOnly() && !identIter->IsNoLoad()
                        && !identIter->IsScState() && !identIter->IsConstVarRead())

						ParseMsg(PARSE_WARNING, identIter->GetLineInfo(), "'%s' was not read", name.c_str());

					else if (!identIter->IsWriteRef(elemIdx) && !identIter->IsReadOnly())
						ParseMsg(PARSE_ERROR, identIter->GetLineInfo(), "'%s' was not written", name.c_str());
				}

				break;
			}
		}
	}
}

void CHtfeDesign::CheckPortList(CHtfeIdent *pInst)
{
	CHtfeIdentTblIter instPortIter(pInst->GetIdentTbl());
	CHtfeIdentTblIter primPortIter(pInst->GetType()->GetIdentTbl());
	instPortIter.Begin();
	primPortIter.Begin();

	while (!primPortIter.End() && !primPortIter->IsPort())
		primPortIter++;

	while (!primPortIter.End()) {
		if (instPortIter.End() || primPortIter->GetName() != instPortIter->GetName()) {
			ParseMsg(PARSE_ERROR, pInst->GetLineInfo(), "signal not specified for port '%s'",
				primPortIter->GetName().c_str());
		} else {
			switch (primPortIter->GetPortDir()) {
			case port_in:
				instPortIter->GetPortSignal()->SetReadRef(0);
				break;
			case port_out:
				instPortIter->GetPortSignal()->SetWriteRef(0);
				break;
			case port_inout:
				instPortIter->GetPortSignal()->SetReadRef(0);
				instPortIter->GetPortSignal()->SetWriteRef(0);
				break;
			default:
				ParseMsg(PARSE_FATAL, pInst->GetLineInfo(), "internal, unknown port type");
				break;
			}

			pInst->SetReadRef(0);
			pInst->SetWriteRef(0);

			instPortIter++;
		}

		do {
			primPortIter++;
		} while (!primPortIter.End() && !primPortIter->IsPort());
	}
}

void CHtfeDesign::CheckSensitivityList(CHtfeIdent *pModule, CHtfeIdent *pMethod)
{
	if (pMethod->GetClockIdent() != 0) {
		// clocked method
		CScSensitiveTblIter sensitiveIter(pMethod->GetSensitiveTbl());
		sensitiveIter.Begin();

		if (pMethod->GetSensitiveTbl().GetCount() != 1 ||
			sensitiveIter->m_pIdent->GetSensitiveType() != tk_sensitive_pos &&
			sensitiveIter->m_pIdent->GetSensitiveType() != tk_sensitive_neg)

			ParseMsg(PARSE_ERROR, pMethod->GetLineInfo(), "clocked method with extra signals in sensitive list");

		else if (pModule->IsScFixture() == false &&
			sensitiveIter->m_pIdent->GetSensitiveType() == tk_sensitive_neg)

			ParseMsg(PARSE_ERROR, pMethod->GetLineInfo(), "negedge clocked method is allowed only in fixture");

	} else {

		// non-clocked methods
		CScSensitiveTblIter sensitiveIter(pMethod->GetSensitiveTbl());
		sensitiveIter.Begin();

		CScElemTblIter elemTblIter(pModule->GetIdentTbl());
		elemTblIter.Begin();

		while ( !elemTblIter.End() || !sensitiveIter.End() ) {
			CHtfeIdent *pIdent = 0;
			int elemIdx = 0;

			if (!elemTblIter.End()) {
				pIdent = elemTblIter->m_pIdent;
				elemIdx = elemTblIter->m_elemIdx;
			}

			if (elemTblIter.End() || pIdent->IsVariable() &&
				pIdent->IsReadRef(elemIdx) && !pIdent->IsWriteRef() && !pIdent->IsReference()) {
				
				// should be in the sensitivity list
				if (sensitiveIter.End() || elemTblIter() < sensitiveIter()) {
					// missing signal
					ParseMsg(PARSE_ERROR, pMethod->GetLineInfo(), "missing signal in sensitivity list '%s'",
						pIdent->GetDimenElemName(elemIdx).c_str());

					elemTblIter++;

				} else if (elemTblIter.End() || elemTblIter() > sensitiveIter()) {
					// extra signal
					ParseMsg(PARSE_ERROR, pMethod->GetLineInfo(), "extra signal in sensitivity list '%s'",
						sensitiveIter->m_pIdent->GetDimenElemName(sensitiveIter->m_elemIdx).c_str());

					sensitiveIter++;
				} else {

					elemTblIter++;
					sensitiveIter++;

				}
			} else
				elemTblIter++;
		}
	}
}

/********** OLD **************************************/


void CHtfeDesign::PrintReferenceCounts()
{
	CHtfeIdentTblIter moduleIter(m_pTopHier->GetIdentTbl());
	for (moduleIter.Begin(); !moduleIter.End(); moduleIter++) {

		PrintReferenceCounts(moduleIter->GetThis(), 0);

		if (!moduleIter->IsModule() || !moduleIter->IsMethodDefined())
			continue;

		CHtfeIdentTblIter varIter(moduleIter->GetIdentTbl());
		for (varIter.Begin(); !varIter.End(); varIter++) {

			PrintReferenceCounts(varIter->GetThis(), 1);

			if (!varIter->IsFunction())
				continue;

			CHtfeIdentTblIter localIter(varIter->GetIdentTbl());
			for (localIter.Begin(); !localIter.End(); localIter++) {
				PrintReferenceCounts(localIter->GetThis(), 2);
			}
		}
	}		
}

void CHtfeDesign::PrintReferenceCounts(CHtfeIdent *pIdent, int level)
{
	printf("%-*s %-32s Rd=%d  Wr=%d\n",
		level*2+8, pIdent->GetIdStr(), pIdent->GetName().c_str(),
		pIdent->IsReadRef(), pIdent->IsWriteRef());
}
