/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
// PreSynthesis.cpp: pattern match and manipulate code
//
//////////////////////////////////////////////////////////////////////

#include "Htv.h"
#include "HtvDesign.h"

void CHtvDesign::PreSynthesis()
{
    // First check
    CHtvIdentTblIter moduleIter(GetTopHier()->GetIdentTbl());
    for (moduleIter.Begin(); !moduleIter.End(); moduleIter++) {

        if (!moduleIter->IsModule() || !moduleIter->IsMethodDefined())
            continue;

        CHtvIdentTblIter funcIter(moduleIter->GetIdentTbl());
        for (funcIter.Begin(); !funcIter.End(); funcIter++) {

            if (!funcIter->IsFunction() || !funcIter->IsMethod())
                continue;

            // A method is a function with a sensitivity list
            PreSynthesis(moduleIter->GetThis(), funcIter->GetThis());

			// remove statements where the assigned variable was not referenced
			RemoveUnusedVariableStatements((CHtvStatement **)funcIter->GetPStatementList());
        }
    }		
}

void CHtvDesign::PreSynthesis(CHtvIdent *pModule, CHtvIdent *pMethod)
{
    // Now scan statements
    PreSynthesis(pMethod, (CHtvStatement *)pMethod->GetStatementList());
}

void CHtvDesign::PreSynthesis(CHtvIdent *pHier, CHtvStatement *pStatement)
{
    for ( ; pStatement; pStatement = pStatement->GetNext()) {
        switch (pStatement->GetStType()) {
			case st_compound:
				break;
            case st_assign:
                break;
            case st_if:
                break;
            case st_switch:
                break;
            case st_case:
                break;
            case st_default:
                break;
            case st_break:
            case st_null:
                break;
            case st_return:
                break;
            default:
                Assert(0);
        }
    }
}

void CHtvDesign::RemoveUnusedVariableStatements(CHtvStatement **ppStatement)
{
    for ( ; *ppStatement; ) {
        switch ((*ppStatement)->GetStType()) {
			case st_compound:
				RemoveUnusedVariableStatements((*ppStatement)->GetPCompound1());
				break;
            case st_assign:
                {
                    CHtvOperand *pExpr = (*ppStatement)->GetExpr();

                    if (pExpr->GetOperator() != tk_equal)
                        break;	// function calls without a return value

                    CHtvOperand *pOp1 = pExpr->GetOperand1();
                    CHtvIdent *pMember = pOp1->GetMember();

                    if (!pMember->GetHierIdent()->IsRemoveIfWriteOnly() || pMember->GetHierIdent()->IsReadRef())
                        break;

					// Found a statement to be removed
					*ppStatement = (*ppStatement)->GetNext();
					continue;
                }
                break;
            case st_if:
				RemoveUnusedVariableStatements((*ppStatement)->GetPCompound1());
				RemoveUnusedVariableStatements((*ppStatement)->GetPCompound2());
                break;
            case st_switch:
 				RemoveUnusedVariableStatements((*ppStatement)->GetPCompound1());
                break;
            case st_case:
				RemoveUnusedVariableStatements((*ppStatement)->GetPCompound1());
                break;
            case st_default:
				RemoveUnusedVariableStatements((*ppStatement)->GetPCompound1());
                break;
            case st_break:
            case st_null:
                break;
            case st_return:
                break;
            default:
                Assert(0);
        }

		ppStatement = (*ppStatement)->GetPNext();
    }
}
