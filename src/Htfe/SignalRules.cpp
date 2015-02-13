/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
// SignalRules.cpp: Check Signal Rules
//   Verify that:
//   1. There is only one method driving each variable
//   2. If multiple methods reference a variable then it must be a signal
//
//////////////////////////////////////////////////////////////////////

#include "Htfe.h"
#include "HtfeDesign.h"

void CHtfeDesign::CheckSignalRules()
{
    CHtfeIdentTblIter moduleIter(m_pTopHier->GetIdentTbl());
    for (moduleIter.Begin(); !moduleIter.End(); moduleIter++) {

        if (!moduleIter->IsModule() || !moduleIter->IsMethodDefined())
            continue;

        CHtfeIdentTblIter identIter(moduleIter->GetIdentTbl());
        for (identIter.Begin(); !identIter.End(); identIter++) {

            if (identIter->IsFunction() || identIter->IsHtQueue() || identIter->IsHtMemory())
                continue;

			if (identIter->GetWriterListCnt() > 0) {
				const vector<CHtfeIdent *> *pWriterList = &identIter->GetWriterList();

				// determine number of unique methods writing variable
				bool bMultiple;
				CHtfeIdent *pUniqueWriter = FindUniqueAccessMethod(pWriterList, bMultiple);

				//while (pWriterList->size() == 1 && !(*pWriterList)[0]->IsMethod())
				//	pWriterList = &(*pWriterList)[0]->GetWriterList();

				if (bMultiple)
					ParseMsg(PARSE_ERROR, identIter->GetLineInfo(), "Multiple methods drive variable '%s'", identIter->GetName().c_str());

				else {
					if (pUniqueWriter != 0 && identIter->GetReaderListCnt() > 0) {
						// check if the lone writer is different than the receiver
						bool bMultiple = CheckMultipleRefMethods(pUniqueWriter, identIter->GetReaderList());

						if (bMultiple && !identIter->IsSignal() && !identIter->IsIgnoreSignalCheck())
							ParseMsg(PARSE_ERROR, identIter->GetLineInfo(), "variable with reference by multiple methods must be declared as a signal '%s'", identIter->GetName().c_str());

						if (!bMultiple && identIter->IsSignal())
							ParseMsg(PARSE_ERROR, identIter->GetLineInfo(), "variable with reference by a single method should not be declared as a signal '%s'", identIter->GetName().c_str());
					}
				}
			}
        }
    }		
}

CHtfeIdent * CHtfeDesign::FindUniqueAccessMethod(const vector<CHtfeIdent *> *pWriterList, bool &bMultiple)
{
	bMultiple = false;
	CHtfeIdent *pWriter = 0;
	for (size_t i = 0; i < pWriterList->size(); i += 1) {
		if ((*pWriterList)[i]->IsMethod()) {

			if (pWriter == 0)
				pWriter = (*pWriterList)[i];
			else if (pWriter != (*pWriterList)[i]) {
				bMultiple = true;
				return 0;	// multiple methods are writers
			}
		} else {
			Assert((*pWriterList)[i]->IsFunction());

			if ((*pWriterList)[i]->GetCallerListCnt() == 0)
				continue;

			CHtfeIdent *pCaller = FindUniqueAccessMethod(&(*pWriterList)[i]->GetCallerList(), bMultiple);

			if (bMultiple)
				return 0;
			else if (pWriter == 0)
				pWriter = pCaller;
			else if (pCaller && pWriter != pCaller) {
				bMultiple = true;
				return 0;
			}
		}
	}
	return pWriter;
}

bool CHtfeDesign::CheckMultipleRefMethods(CHtfeIdent *pWriter, const vector<CHtfeIdent *> & readerList)
{
	for (size_t i = 0; i < readerList.size(); i += 1) {
		if (readerList[i]->IsMethod()) {
			if (pWriter != readerList[i])
				return true;
		} else if (readerList[i]->GetCallerListCnt() > 0) {
			// function call, check each caller
			if (CheckMultipleRefMethods(pWriter, readerList[i]->GetCallerList()))
				return true;
		}
	}

	return false;
}
