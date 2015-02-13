/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#pragma once

#include "HttArgs.h"
#include "HtfeDesign.h"

#include "HttIdent.h"
#include "HttOperand.h"
#include "HttObject.h"
#include "HttStatement.h"
#include "HttInstance.h"

class CHttDesign : public CHtfeDesign {
public:
	CHttDesign();

    void OpenFiles();
    void CloseFiles();

	void HandleInputFileInit();
	void HandleScMainInit();
	void HandleScMainStart();

	CHtfeStatement * HandleNewStatement() { return new CHttStatement(); }
	CHtfeOperand * HandleNewOperand() { return new CHttOperand(); }
	CHttIdent * HandleNewIdent() { return new CHttIdent(); }

	void ConstructGraph();

	CHttIdent * FindUniqueAccessMethod(const vector<CHtfeIdent *> *pWriterList, bool &bMultiple) { return (CHttIdent *)CHtfeDesign::FindUniqueAccessMethod(pWriterList, bMultiple); }

private:
    string m_pathName;
    CHtFile m_incFile;
    CHtFile m_cppFile;

    uint32_t m_genCppStartOffset;
};
