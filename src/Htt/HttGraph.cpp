/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
// Synthesize.cpp
//
//////////////////////////////////////////////////////////////////////

#include "Htt.h"
#include "HttArgs.h"
#include "HttDesign.h"

void CHttDesign::ConstructGraph()
{
    if (GetErrorCnt() != 0) {
        ParseMsg(PARSE_ERROR, "unable to continue due to previous errors");
        exit(1);
    }

    CHttIdentTblIter topHierIter(GetTopHier()->GetIdentTbl());
    for (topHierIter.Begin(); !topHierIter.End(); topHierIter++) {
        if (!topHierIter->IsModule() || !topHierIter->IsMethodDefined())
            continue;

        CHttIdent *pModule = topHierIter->GetThis();

        CHttIdentTblIter moduleIter(pModule->GetIdentTbl());

        // insert function memory initialization statements into caller
        for (moduleIter.Begin(); !moduleIter.End(); moduleIter++) {
            if (!moduleIter->IsFunction() || moduleIter->IsMethod() || moduleIter->GetName() == pModule->GetName())
                continue;

            CHttIdent *pFunc = moduleIter->GetThis();

            if (pFunc->GetCallerListCnt() == 0 || pFunc->GetMemInitStatements() == 0)
                continue;

            // sc_memory functions must be called from a single method
            bool bMultiple;
            CHttIdent *pMethod = FindUniqueAccessMethod(&pFunc->GetCallerList(), bMultiple);

            if (bMultiple)
                ParseMsg(PARSE_FATAL, pFunc->GetLineInfo(), "function with sc_memory variables called from more than one method");

            pMethod->InsertStatementList(pFunc->GetMemInitStatements());
        }

        // Generate logic blocks
        for (moduleIter.Begin(); !moduleIter.End(); moduleIter++) {
            if (!moduleIter->IsMethod())
                continue;

			printf("Name: %s\n", moduleIter->GetName().c_str());
        }
    }
}
