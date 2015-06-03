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

#include "Htv.h"
#include "HtvArgs.h"
#include "HtvDesign.h"
#include "HtvFileBkup.h"

void CHtvDesign::GenVerilog()
{
    if (GetErrorCnt() != 0) {
        ParseMsg(PARSE_ERROR, "unable to generate verilog due to previous errors");
        ErrorExit();
    }

    CHtvIdentTblIter topHierIter(GetTopHier()->GetIdentTbl());
    for (topHierIter.Begin(); !topHierIter.End(); topHierIter++) {
        if (!topHierIter->IsModule() || !topHierIter->IsMethodDefined())
            continue;

        CHtvIdent *pModule = topHierIter->GetThis();

        CHtvIdentTblIter moduleIter(pModule->GetIdentTbl());

        // insert function memory initialization statements into caller
        for (moduleIter.Begin(); !moduleIter.End(); moduleIter++) {
            if (!moduleIter->IsFunction() || moduleIter->IsMethod() || moduleIter->GetName() == pModule->GetName())
                continue;

            CHtvIdent *pFunc = moduleIter->GetThis();

            if (pFunc->GetCallerListCnt() == 0 || pFunc->GetMemInitStatements() == 0)
                continue;

            // sc_memory functions must be called from a single method
            bool bMultiple;
            CHtvIdent *pMethod = FindUniqueAccessMethod(&pFunc->GetCallerList(), bMultiple);

            if (bMultiple)
                ParseMsg(PARSE_FATAL, pFunc->GetLineInfo(), "function with sc_memory variables called from more than one method");

            pMethod->InsertStatementList(pFunc->GetMemInitStatements());
        }

        // find global variables used within functions
        for (moduleIter.Begin(); !moduleIter.End(); moduleIter++) {
            if (!moduleIter->IsFunction() || moduleIter->IsMethod() 
                || moduleIter->GetName() == pModule->GetName() || moduleIter->IsGlobalReadRef())
                continue;

            FindGlobalRefSet(moduleIter->GetThis(), (CHtvStatement *)moduleIter->GetStatementList());

            moduleIter->SetIsGlobalReadRef(true);
        }

        // Generate logic blocks
        if (g_htvArgs.GetXilinxFdPrims()) {
            for (moduleIter.Begin(); !moduleIter.End(); moduleIter++) {
                if (!moduleIter->IsMethod())
                    continue;

                FindXilinxFdPrims(moduleIter->GetThis(), 0);
            }
        }

        for (moduleIter.Begin(); !moduleIter.End(); moduleIter++) {
            if (!moduleIter->IsMethod())
                continue;

            FindWriteRefSets(moduleIter->GetThis());
        }

        GenModuleHeader(topHierIter());

        // Generate global functions used in module
        CHtvIdentTblIter topHierIter2(GetTopHier()->GetIdentTbl());
        for (topHierIter2.Begin(); !topHierIter2.End(); topHierIter2++) {

            if (!topHierIter2->IsFunction())
                continue;

            CHtvIdent *pFunc = topHierIter2->GetThis();

            if (!pFunc->IsBodyDefined() || pFunc->IsScPrim())
                continue;

            // check if used in this module
			if (!isMethodCalledByModule(&(pFunc->GetCallerList()), pModule))
				continue;

            GenFunction(pFunc, 0, 0);
        }

        // Generate functions
        for (moduleIter.Begin(); !moduleIter.End(); moduleIter++) {
            if (!moduleIter->IsFunction() || moduleIter->IsMethod() || moduleIter->GetName() == pModule->GetName())
                continue;

            GenFunction(moduleIter->GetThis(), 0, 0);
        }

        // Generate logic blocks
        for (moduleIter.Begin(); !moduleIter.End(); moduleIter++) {
            if (!moduleIter->IsMethod())
                continue;

			m_pInlineHier = moduleIter->GetPrevHier();

			GenFuncVarDecl(moduleIter->GetThis());

            SynIndividualStatements(moduleIter->GetThis(), 0, 0);

			m_pInlineHier = 0;
        }

		// flush all statements
		m_vFile.FlushLineBuffer();

		// flush primitive instances
		m_vFile.SetLineBuffer(1);
		m_vFile.FlushLineBuffer();

		m_vFile.SetLineBuffering(false);

        // Generate memories
        SynHtDistQueRams(topHierIter());
        SynHtBlockQueRams(topHierIter());
        SynHtDistRams(topHierIter());
        SynHtBlockRams(topHierIter());
        SynHtAsymBlockRams(topHierIter());

        GenModuleTrailer();

        // generate template modules
        SynHtDistRamModule(true);
        SynHtDistRamModule(false);
        SynHtBlockRamModule();
        SynHtAsymBlockRamModule();

        if (g_htvArgs.IsLedaEnabled())
            m_vFile.Print("\n`include \"leda_trailer.vh\"\n");
    }
}

bool CHtvDesign::isMethodCalledByModule(const vector<CHtfeIdent *> *pCallerList, CHtfeIdent *pModule)
{
	if (pCallerList == 0)
		return false;

	for (size_t i = 0; i < pCallerList->size(); i += 1) {
		Assert((*pCallerList)[i]->IsFunction());

		if ((*pCallerList)[i]->GetPrevHier() == pModule)
			return true;

		if ((*pCallerList)[i]->GetCallerListCnt() == 0)
			continue;

		if (isMethodCalledByModule(&(*pCallerList)[i]->GetCallerList(), pModule))
			return true;
	}

	return false;
}

void CHtvDesign::GenerateScModule(CHtvIdent *pClass)
{
    // Generate transactions for an ScModule (v, cpp or both)
    // first create a signal/port list
    GenScModuleInsertMissingInstancePorts(pClass);

    // The complete signal list is now created, write out scModule
    m_incFile.Print(" {\n");

    GenScModuleWritePortAndSignalList(pClass);

    // Now write out instance declarations
    m_incFile.Print("\n");

    GenScModuleWriteInstanceDecl(pClass);

    // Write out destructor

    GenScModuleWriteInstanceDestructors(pClass);

    m_incFile.Print("\n  SC_CTOR(%s) {\n", pClass->GetName().c_str());
    m_incFile.Print("\n");
    m_vFile.Print("\n");

    GenScModuleWriteSignalsAssignedConstant(pClass);

    m_incFile.Flush();

    GenScModuleWriteInstanceAndPortList(pClass);

    m_incFile.Print("  }\n};\n");
    m_vFile.Print("\nendmodule\n");
    m_vFile.Print("\n");

    //////////////////////////////////////
    // Update Fixture.h and Fixture.cpp
    //

    if (g_htvArgs.IsGenFixture() && pClass->IsScFixtureTop())
        GenFixtureFiles(pClass);
}

void CHtvDesign::GenScModuleWriteInstanceAndPortList(CHtvIdent *pClass)
{
    char *pFirstStr;

    // write out instance with port list
    CHtvArrayTblIter instanceIter(pClass->GetIdentTbl());
    for (instanceIter.Begin(); !instanceIter.End(); instanceIter++)
    {
        if (instanceIter->GetId() != CHtvIdent::id_variable || !instanceIter->IsReference())
            continue;

        int elemIdx = instanceIter->GetElemIdx();
        CHtvIdent *pBaseIdent;
        if (instanceIter->GetPrevHier()->IsArrayIdent())
            pBaseIdent = instanceIter->GetPrevHier();
        else
            pBaseIdent = instanceIter();

        const string identName = instanceIter->GetName();
        const string typeName = instanceIter->GetType()->GetName();
        const string instName = instanceIter->GetInstanceName();

        Assert(identName == pBaseIdent->GetDimenElemName(elemIdx));
        Assert(typeName == pBaseIdent->GetType()->GetName());
        Assert(instName == pBaseIdent->GetInstanceName(elemIdx));

        m_incFile.Print("    %s = new %s(\"%s\");\n", identName.c_str(), typeName.c_str(), instName.c_str());

        CHtvIdent *pInstance = instanceIter->GetThis();
        CHtvIdent *pModule = pInstance->GetType();

        bool bIsScFixture = pModule->IsScFixture();

        if (!bIsScFixture) {
            m_vFile.Print("\n");

            string verilogInstName = instanceIter->GetFlatIdent()->GetName();
            Assert(verilogInstName == pBaseIdent->GetVerilogName(elemIdx));

            GenHtAttributes(instanceIter(), verilogInstName);

            m_vFile.Print("  %s %s(\n", typeName.c_str(), verilogInstName.c_str());

        } else {

            m_vFile.Print("\n  initial begin\n");
            m_vFile.Print("    clock = 1;\n");
            m_vFile.Print("  end\n");

            m_vFile.Print("\n  always begin\n");
            m_vFile.Print("    #5 clock = ~clock;\n");
            m_vFile.Print("  end\n");

            const char *pModuleStr = typeName.c_str();
            m_vFile.Print("\n  initial $%s (\n", pModuleStr);
        }

        pFirstStr = "";
        int clockCnt = 0;
        string clockName = "clock";

        if (pClass->IsScFixtureTop() && pModule->IsScFixture()) {

            CHtvIdentTblIter instPortIter(instanceIter->GetIdentTbl());
            for (instPortIter.Begin(); !instPortIter.End(); instPortIter++) {

                m_incFile.Print("    %s->%s(%s);\n", identName.c_str(), instPortIter->GetName().c_str(), instPortIter->GetSignalIdent()->GetName().c_str());
            }

            CHtvIdentTblIter moduleIdentIter(pClass->GetIdentTbl());
            for (moduleIdentIter.Begin(); !moduleIdentIter.End(); moduleIdentIter++) {

                if (moduleIdentIter->IsOutPort()) {
                    const string portName = moduleIdentIter->GetName();

                    m_vFile.Print("%s    %s", pFirstStr, portName.c_str());
                    pFirstStr = ",\n";
                }
            }
            for (moduleIdentIter.Begin(); !moduleIdentIter.End(); moduleIdentIter++) {

                if (moduleIdentIter->IsInPort()) {
                    const string portName = moduleIdentIter->GetName();

                    m_vFile.Print("%s    %s", pFirstStr, portName.c_str());
                    pFirstStr = ",\n";
                }
            }

        } else {
            CHtvArrayTblIter modulePortIter(pModule->GetIdentTbl());
            for (modulePortIter.Begin(); !modulePortIter.End(); modulePortIter++)
            {
                // find ports of module
                if (!modulePortIter->IsPort())
                    continue;

                string signalName;
                string verilogSignalName;
                CHtvIdent *pInstancePort = pInstance->FindIdent(modulePortIter->GetName());

                if (modulePortIter->GetDimenCnt() == 0) {

                    if (pInstancePort) {
                        signalName = pInstancePort->GetPortSignal()->GetName();
                        verilogSignalName = pInstancePort->GetPortSignal()->GetFlatIdent()->GetName();
                    } else {
                        signalName = modulePortIter->GetName();
                        verilogSignalName = modulePortIter->GetFlatIdent()->GetName();
                    }

                    m_incFile.Print("    %s->%s(%s);\n", instanceIter->GetName().c_str(),
                        modulePortIter->GetName().c_str(), signalName.c_str());

                } else {

                    CHtvIdentTblIter instPortIter(modulePortIter->GetIdentTbl());
                    for (instPortIter.Begin(); !instPortIter.End(); instPortIter++) {

                        if (pInstancePort) {
                            signalName = pInstancePort->GetPortSignal()->GetName();
                            verilogSignalName = pInstancePort->GetPortSignal()->GetFlatIdent()->GetName();
                        } else {
                            signalName = instPortIter->GetName();
                            verilogSignalName = instPortIter->GetFlatIdent()->GetName();
                        }

                        m_incFile.Print("    %s->%s(%s);\n", instanceIter->GetName().c_str(),
                            instPortIter->GetName().c_str(), signalName.c_str());
                    }						
                }

                if (!bIsScFixture) {
                    m_vFile.Print("%s    .%s(%s)", pFirstStr,
                        modulePortIter->GetFlatIdent()->GetName().c_str(), verilogSignalName.c_str());
                    pFirstStr = ",\n";
                } else {
                    // defer print of 2nd occurrence of clock to prevent Verilog sim crash
                    if (modulePortIter->GetSignalIdent()->IsClock() && clockCnt++ > 0)
                        continue;

                    int width = modulePortIter->GetWidth();
                    if (width <= 32) {
                        m_vFile.Print("%s    %s", pFirstStr, signalName.c_str());
                        pFirstStr = ",\n";
                    } else {
                        for (int i = 0; i < width; i += 32) {
                            m_vFile.Print("%s    %s[%d:%d]", pFirstStr, signalName.c_str(), (width < i+32) ? width-1 : i+31, i);
                            pFirstStr = ",\n";
                        }
                    }
                }
            }
        }

        m_incFile.Print("\n");
        m_vFile.Print("\n  );\n");
    }
}

struct CMissingInfo {
    CMissingInfo() {
        m_bHasPrefix = false;
        m_bIsUsedOnInputPort = false;
        m_bIsUsedOnOutputPort = false;
        m_bIsDeclAsInput = false;
        m_bIsDeclAsOutput = false;
        m_bIsDeclAsSignal = false;
        m_pSignal = 0;
    }

    bool	m_bHasPrefix;
    bool	m_bIsUsedOnInputPort;
    bool	m_bIsUsedOnOutputPort;
    bool	m_bIsDeclAsInput;
    bool	m_bIsDeclAsOutput;
    bool	m_bIsDeclAsSignal;

    CHtvIdent *	m_pSignal;
};

void CHtvDesign::GenScModuleInsertMissingInstancePorts(CHtvIdent *pClass)
{
    // First create a table of missing port names. The information is used
    //   to decide if the created signal name is local to the module, or
    //   is itself a port name.
    typedef map<string, CMissingInfo>			MissingInfoTblMap;
    typedef pair<string, CMissingInfo>			MissingInfoTblMap_ValuePair;
    typedef MissingInfoTblMap::iterator			MissingInfoTblMap_Iter;
    typedef pair<MissingInfoTblMap_Iter, bool>	MissingInfoTblMap_InsertPair;

    MissingInfoTblMap	missingInfoTblMap;

    CHtvArrayTblIter instanceIter(pClass->GetIdentTbl());
    for (instanceIter.Begin(); !instanceIter.End(); instanceIter++) {
        if (instanceIter->GetId() != CHtvIdent::id_variable || !instanceIter->IsReference())
            continue;

        CHtvIdent *pModule = instanceIter->GetType();

        // was Array
        CHtvArrayTblIter modulePortIter(pModule->GetIdentTbl());
        for (modulePortIter.Begin(); !modulePortIter.End(); modulePortIter++)
        {
            // find ports of module
            if (!modulePortIter->IsPort())
                continue;

            CHtvIdent *pInstancePort = instanceIter->FindIdent(modulePortIter->GetName());

            string signalName;
            if (pInstancePort) {
                signalName = pInstancePort->GetSignalIdent()->GetName();

                // strip off indexing
                signalName = signalName.substr(0, signalName.find('['));
            } else {
                signalName = modulePortIter->GetName();

                // strip off indexing
                signalName = signalName.substr(0, signalName.find('['));
            }
            string prefix = signalName.substr(0,2);
            bool bHasPrefix = false;
            if (prefix == "i_" || prefix == "o_") {
                signalName = signalName.substr(2);
                bHasPrefix = true;
            }

            CMissingInfo *pInfo;
            MissingInfoTblMap_Iter iter = missingInfoTblMap.find(signalName);
            if (iter == missingInfoTblMap.end()) {
                MissingInfoTblMap_InsertPair insertPair = missingInfoTblMap.insert(MissingInfoTblMap_ValuePair(signalName, CMissingInfo()));
                pInfo = &insertPair.first->second;
            } else
                pInfo = &iter->second;

            pInfo->m_bHasPrefix |= bHasPrefix;

            if (modulePortIter->IsInPort())
                pInfo->m_bIsUsedOnInputPort = true;
            else if (modulePortIter->IsOutPort())
                pInfo->m_bIsUsedOnOutputPort = true;
            else
                Assert(0);

            CHtvIdent *pIdent = pClass->FindIdent(signalName);
            if (pIdent && pIdent->GetId() != CHtvIdent::id_new) {
                pInfo->m_bIsDeclAsSignal = true;
                pInfo->m_pSignal = pIdent;
            }

            string inPortName = (bHasPrefix ? "i_" : "") + signalName;
            pIdent = pClass->FindIdent(inPortName);
            if (pIdent && pIdent->GetId() != CHtvIdent::id_new) {
                pInfo->m_bIsDeclAsInput = true;
                pInfo->m_pSignal = pIdent;
            }

            string outPortName = (bHasPrefix ? "o_" : "") + signalName;
            pIdent = pClass->FindIdent(outPortName);
            if (pIdent && pIdent->GetId() != CHtvIdent::id_new) {
                pInfo->m_bIsDeclAsOutput = true;
                pInfo->m_pSignal = pIdent;
            }
        }
    }

    // determine port/signal for missing info and insert into pClass table
    MissingInfoTblMap_Iter iter = missingInfoTblMap.begin();
    for ( ; iter != missingInfoTblMap.end(); iter++) {

        string signalName = iter->first;
        if (pClass->IsScFixtureTop()) {
            // all signals are local for fixture top
            if (iter->second.m_pSignal == 0) {
                iter->second.m_pSignal = pClass->InsertIdent(signalName);
                //iter->second.m_pSignal->SetElemIdx(0);
            }
            if (iter->second.m_pSignal->GetPortDir() == port_zero) {
                if (iter->second.m_bIsUsedOnInputPort)
                    iter->second.m_pSignal->SetPortDir(port_out);
                else
                    iter->second.m_pSignal->SetPortDir(port_in);
            }
        } else if (iter->second.m_bIsUsedOnInputPort && iter->second.m_bIsUsedOnOutputPort) {
            // local signal
            if (iter->second.m_pSignal == 0) {
                iter->second.m_pSignal = pClass->InsertIdent(signalName);
                //iter->second.m_pSignal->SetElemIdx(0);
            }
            if (iter->second.m_pSignal->GetPortDir() == port_zero)
                iter->second.m_pSignal->SetPortDir(port_signal);
        } else if (iter->second.m_bIsUsedOnInputPort) {
            // input port
            string inPortName = (iter->second.m_bHasPrefix ? "i_" : "") + signalName;
            if (iter->second.m_pSignal == 0) {
                iter->second.m_pSignal = pClass->InsertIdent(inPortName);
                //iter->second.m_pSignal->SetElemIdx(0);
            }
            if (iter->second.m_pSignal->GetPortDir() == port_zero)
                iter->second.m_pSignal->SetPortDir(port_in);
        } else {
            // output port
            string outPortName = (iter->second.m_bHasPrefix ? "o_" : "") + signalName;
            if (iter->second.m_pSignal == 0) {
                iter->second.m_pSignal = pClass->InsertIdent(outPortName);
                //iter->second.m_pSignal->SetElemIdx(0);
            }
            if (iter->second.m_pSignal->GetPortDir() == port_zero)
                iter->second.m_pSignal->SetPortDir(port_out);
        }
        iter->second.m_pSignal->SetId(CHtvIdent::id_variable);
    }

    // scan the instantiated modules again
    for (instanceIter.Begin(); !instanceIter.End(); instanceIter++) {
        if (instanceIter->GetId() != CHtvIdent::id_variable || !instanceIter->IsReference())
            continue;

        CHtvIdent *pModule = instanceIter->GetType();

        // was Array
        CHtvArrayTblIter modulePortIter(pModule->GetIdentTbl());
        for (modulePortIter.Begin(); !modulePortIter.End(); modulePortIter++)
        {
            // find ports of module
            if (!modulePortIter->IsPort())
                continue;

            // Instance ports are not kept as arrays
            CHtvIdent *pInstancePort = instanceIter->FindIdent(modulePortIter->GetName());

            CHtvIdent *pSignal = 0;
            if (pInstancePort) {
                pSignal = pInstancePort->GetPortSignal();
                Assert(pSignal);

            } else {

                string signalName = modulePortIter->GetName();
                string prefix = signalName.substr(0,2);
                if (prefix == "i_" || prefix == "o_")
                    signalName = signalName.substr(2);

                // get name from missing signal info table
                string baseSignalName = signalName.substr(0, signalName.find('['));
                MissingInfoTblMap_Iter iter = missingInfoTblMap.find(baseSignalName);
                CMissingInfo *pInfo = &iter->second;
                Assert(pInfo);

                pSignal = pInfo->m_pSignal;
                pSignal->SetId(CHtvIdent::id_variable);
                pSignal->SetType(modulePortIter->GetType());
                pSignal->SetLineInfo(modulePortIter->GetLineInfo());

                CHtvIdent *pPortArrayIdent = 0;
                if (modulePortIter->GetPrevHier()->IsArrayIdent())
                    pPortArrayIdent = modulePortIter->GetPrevHier();

                if (pPortArrayIdent) {

                    if (pSignal->GetDimenCnt() == 0) {
                        vector<int> emptyList = pPortArrayIdent->GetDimenList();
                        for (size_t i = 0; i < emptyList.size(); i += 1)
                            emptyList[i] = 0;

                        pSignal->SetDimenList(emptyList);

                        pSignal->SetIsArrayIdent();
                        pSignal->SetFlatIdentTbl(pSignal->GetFlatIdent()->GetFlatIdentTbl());

                        UpdateVarArrayDecl(pSignal, pPortArrayIdent->GetDimenList());
                    }

                    string dimIdxStr = signalName.substr(signalName.find('['));
                    CHtvIdent *pSignal2 = pSignal->FindIdent(pSignal->GetName() + dimIdxStr);
                    if (!pSignal2) {
                        ParseMsg(PARSE_ERROR, instanceIter->GetLineInfo(), "signal '%s' does not have index %s\n",
                            pSignal->GetName().c_str(), dimIdxStr.c_str());
                        ErrorExit();
                    }
                    pSignal = pSignal2;

                } else {
                    // non-arrayed port

                    bool bDimenMatch = true;
                    for (size_t i = 0; i < pSignal->GetDimenCnt() && i < modulePortIter->GetDimenCnt(); i += 1)
                        bDimenMatch &= pSignal->GetDimenList()[i] == modulePortIter->GetDimenList()[i];
                    if (!bDimenMatch || pSignal->GetDimenCnt() != modulePortIter->GetDimenCnt()) {
                        char dimStr[16];
                        string portName = modulePortIter->GetName();
                        for (size_t i = 0; i < modulePortIter->GetDimenCnt(); i += 1) {
                            //_itoa(modulePortIter->GetDimenList()[i], dimStr, 10);
                            sprintf(dimStr, "%d", modulePortIter->GetDimenList()[i]);
                            portName.append("[").append(dimStr).append("]");
                        }
                        string signalName = pSignal->GetName();
                        for (size_t i = 0; i < pSignal->GetDimenCnt(); i += 1) {
                            //_itoa(pSignal->GetDimenList()[i], dimStr, 10);
                            sprintf(dimStr, "%d", pSignal->GetDimenList()[i]);
                            signalName.append("[").append(dimStr).append("]");
                        }
                        ParseMsg(PARSE_FATAL, instanceIter->GetLineInfo(), "module port (%s) and signal (%s) dimensions do not match",
                            portName.c_str(), signalName.c_str());
                    }
                }

                if (modulePortIter->IsClock())
                    pSignal->SetIsClock();

                modulePortIter->SetSignalIdent(pSignal);

                pInstancePort = instanceIter->InsertIdent(modulePortIter->GetName(), false);
                pInstancePort->SetSignalIdent(pSignal);
                pInstancePort->SetPortSignal(pSignal);

                if (modulePortIter->GetDimenCnt()) {
                    CHtvIdentTblIter moduleArrayIter(modulePortIter->GetIdentTbl());
                    CHtvIdentTblIter instArrayIter(pSignal->GetIdentTbl());

                    for (moduleArrayIter.Begin(), instArrayIter.Begin(); !moduleArrayIter.End(); moduleArrayIter++, instArrayIter++) {

                        moduleArrayIter->SetSignalIdent(instArrayIter->GetThis());
                    }
                }
            }

            if (modulePortIter->GetPortDir() == port_in)
                pSignal->SetReadRef(pSignal->GetElemIdx());
            else if (modulePortIter->GetPortDir() == port_out)
                pSignal->SetWriteRef(pSignal->GetElemIdx());
            else if (modulePortIter->GetPortDir() == port_inout) {
                pSignal->SetReadRef(pSignal->GetElemIdx());
                pSignal->SetWriteRef(pSignal->GetElemIdx());
            }
        }
    }
}

void CHtvDesign::GenScModuleWritePortAndSignalList(CHtvIdent *pClass)
{
    char *pFirstStr;

    if (pClass->IsScFixtureTop()) {

        CHtvIdent *pClk4x = 0;
        CHtvIdent *pClk2x = 0;
        CHtvIdent *pClk1x = 0;

        // find highest frequency clock clk2x, clk, or clkhx
        CHtvIdentTblIter moduleIdentIter(pClass->GetIdentTbl());
        for (moduleIdentIter.Begin(); !moduleIdentIter.End(); moduleIdentIter++) {
            if (moduleIdentIter->GetName() == "clk4x" || moduleIdentIter->GetName() == "clock4x") {
                pClk4x = moduleIdentIter();
                pClk4x->SetIsClock();
            }
            if (moduleIdentIter->GetName() == "clk2x" || moduleIdentIter->GetName() == "clock2x") {
                pClk2x = moduleIdentIter();
                pClk2x->SetIsClock();
            }
            if (moduleIdentIter->GetName() == "clk1x" || moduleIdentIter->GetName() == "clock1x" || moduleIdentIter->GetName() == "clock") {
                pClk1x = moduleIdentIter();
                pClk1x->SetIsClock();
            }
        }

        if (pClk4x)
            pClass->AddClock(pClk4x);
        if (pClk2x)
            pClass->AddClock(pClk2x);
        if (pClk1x)
            pClass->AddClock(pClk1x);
    }

    if (g_htvArgs.IsKeepHierarchyEnabled())
        m_vFile.Print("(* keep_hierarchy = \"true\" *)\n");

    GenHtAttributes(pClass);

    m_vFile.Print("module %s (\n", pClass->GetName().c_str());

    if (pClass->IsScFixtureTop()) {
        // Top level topology file is handled special. All top level signals become I/O ports for fixture

        m_vFile.Print(");\n\n");

        CHtvIdentTblIter identIter(pClass->GetIdentTbl());
        for (identIter.Begin(); !identIter.End(); identIter++)
        {
            if (!identIter->IsPort())
                continue;

			string signalDecl = VA("sc_signal<%s>", identIter->GetType()->GetType()->GetName().c_str());

            m_incFile.Print("  %-24s %s;\n", signalDecl.c_str(), identIter->GetName().c_str());

            GenHtAttributes(identIter(), identIter->GetName());

 			string bitRange;
            if (identIter->GetWidth() > 1)
                bitRange = VA("[%d:0]", identIter->GetWidth()-1);

           m_vFile.Print("  %s %-8s    %s;\n", identIter->IsClock() ? "reg " : "wire", bitRange.c_str(),
				identIter->GetName().c_str());
        }

    } else {
        CHtvIdentTblIter hierIdentIter(pClass->GetIdentTbl());

        for (hierIdentIter.Begin(); !hierIdentIter.End(); hierIdentIter++)
        {
            if (!hierIdentIter->IsSignal())
                continue;

            for(int elemIdx = 0; elemIdx < hierIdentIter->GetDimenElemCnt(); elemIdx += 1) {

                bool bIsReadRef = hierIdentIter->IsReadRef(elemIdx);
                bool bIsWriteRef = hierIdentIter->IsWriteRef(elemIdx);

                string name = hierIdentIter->GetDimenElemName(elemIdx);

                if (!bIsReadRef && !bIsWriteRef)
                    ParseMsg(PARSE_ERROR, hierIdentIter->GetLineInfo(), "signal not referenced %s", name.c_str());
                else if (hierIdentIter->GetWriterListCnt() > 1)
                    ParseMsg(PARSE_ERROR, hierIdentIter->GetLineInfo(), "signal is written by multiple output ports", hierIdentIter->GetName().c_str());
                else if (!bIsReadRef)
                    ParseMsg(PARSE_ERROR, hierIdentIter->GetLineInfo(), "signal not read %s", hierIdentIter->GetName().c_str());
                else if (!bIsWriteRef)
                    ParseMsg(PARSE_ERROR, hierIdentIter->GetLineInfo(), "signal not written %s", hierIdentIter->GetName().c_str());
            }
        }

        // First write out input ports for SC file
        //CHtvIdentTblIter hierIdentIter(pClass->GetIdentTbl());
        pFirstStr = "";
        for (hierIdentIter.Begin(); !hierIdentIter.End(); hierIdentIter++)
        {
            if (!hierIdentIter->IsInPort())
                continue;

            if (pClass->IsScTopologyTop() && !hierIdentIter->IsWriteRef()) {
                ParseMsg(PARSE_INFO, "signal %s was not written in top topology file", hierIdentIter->GetName().c_str());
                continue;
            } else if (hierIdentIter->IsWriteRef()) {
                ParseMsg(PARSE_ERROR, "module input signal %s was written", hierIdentIter->GetName().c_str());
                continue;
            } 

            Assert(hierIdentIter->GetType() && hierIdentIter->GetType()->GetType());
            string inDecl = VA("sc_in<%s>", hierIdentIter->GetType()->GetType()->GetName().c_str());
            m_incFile.Print("  %-24s %s", inDecl.c_str(), hierIdentIter->GetName().c_str());

            for (size_t i = 0; i < hierIdentIter->GetDimenCnt(); i += 1)
                m_incFile.Print("[%d]", hierIdentIter->GetDimenList()[i]);

            m_incFile.Print(";\n");

            // now input ports for verilog
			string bitRange;
            if (hierIdentIter->GetWidth() > 1)
                bitRange = VA("[%d:0]", hierIdentIter->GetWidth()-1);

            CHtvIdent *pFlatIdent = hierIdentIter->GetFlatIdent();
            CScArrayIdentIter flatIdentIter(pFlatIdent, pFlatIdent->GetFlatIdentTbl());
            for (flatIdentIter.Begin(); !flatIdentIter.End(); flatIdentIter++) {

                m_vFile.Print("%s", pFirstStr);

                GenHtAttributes(pFlatIdent, flatIdentIter->GetName());

                m_vFile.Print("  input %-8s    %s", bitRange.c_str(), flatIdentIter->GetName().c_str());
                pFirstStr = ",\n";
            }
        }

        // Next write out output ports
        for (hierIdentIter.Begin(); !hierIdentIter.End(); hierIdentIter++)
        {
            if (!hierIdentIter->IsOutPort())
                continue;

            if (pClass->IsScTopologyTop() && !hierIdentIter->IsReadRef()) {
                ParseMsg(PARSE_INFO, "signal %s was not read in top topology file", hierIdentIter->GetName().c_str());
                continue;
            }

            Assert(hierIdentIter->GetType() && hierIdentIter->GetType()->GetType());
            string outDecl = VA("sc_out<%s>", hierIdentIter->GetType()->GetType()->GetName().c_str());
            m_incFile.Print("  %-24s %s", outDecl.c_str(), hierIdentIter->GetName().c_str());

            for (size_t i = 0; i < hierIdentIter->GetDimenCnt(); i += 1)
                m_incFile.Print("[%d]", hierIdentIter->GetDimenList()[i]);

            m_incFile.Print(";\n");

            // now output ports for verilog
			string bitRange;
            if (hierIdentIter->GetWidth() > 1)
                bitRange = VA("[%d:0]", hierIdentIter->GetWidth()-1);

            CHtvIdent *pFlatIdent = hierIdentIter->GetFlatIdent();
            CScArrayIdentIter flatIdentIter(pFlatIdent, pFlatIdent->GetFlatIdentTbl());
            for (flatIdentIter.Begin(); !flatIdentIter.End(); flatIdentIter++) {

                m_vFile.Print("%s", pFirstStr);

                GenHtAttributes(pFlatIdent, flatIdentIter->GetName());

                m_vFile.Print("  output %-8s   %s", bitRange.c_str(), flatIdentIter->GetName().c_str());
                pFirstStr = ",\n";
            }
        }

        m_vFile.Print("\n);\n\n");

        // Next write out signals
        for (hierIdentIter.Begin(); !hierIdentIter.End(); hierIdentIter++)
        {
            if (!hierIdentIter->IsSignal() || hierIdentIter->IsHtQueue())
                continue;

            Assert(hierIdentIter->GetType() && hierIdentIter->GetType()->GetType());
			string signalDecl = VA("sc_signal<%s>", hierIdentIter->GetType()->GetType()->GetName().c_str());
            m_incFile.Print("  %-24s %s", signalDecl.c_str(), hierIdentIter->GetName().c_str());

            for (size_t i = 0; i < hierIdentIter->GetDimenCnt(); i += 1)
                m_incFile.Print("[%d]", hierIdentIter->GetDimenList()[i]);

            m_incFile.Print(";\n");

            // now signals for verilog
			string bitRange;
            if (hierIdentIter->GetWidth() > 1)
                bitRange = VA("[%d:0]", hierIdentIter->GetWidth()-1);

            CHtvIdent *pFlatIdent = hierIdentIter->GetFlatIdent();
            CScArrayIdentIter flatIdentIter(pFlatIdent, pFlatIdent->GetFlatIdentTbl());
            for (flatIdentIter.Begin(); !flatIdentIter.End(); flatIdentIter++) {

                vector<int> refList(flatIdentIter->GetDimenCnt(), 0);

                do {
                    string identName = GenVerilogIdentName(flatIdentIter->GetName(), refList);

                    GenHtAttributes(pFlatIdent, identName);

                    m_vFile.Print("  %s %-8s     %s;\n", hierIdentIter->IsClock() ? "reg " : "wire", bitRange.c_str(), identName.c_str());

                } while (!flatIdentIter->DimenIter(refList));
            }
        }
    }
}

void CHtvDesign::GenScModuleWriteInstanceDecl(CHtvIdent *pClass)
{
    CHtvIdentTblIter instanceIter(pClass->GetIdentTbl());
    for (instanceIter.Begin(); !instanceIter.End(); instanceIter++) {
        if (instanceIter->GetId() != CHtvIdent::id_variable || !instanceIter->IsReference())
            continue;

        m_incFile.Print("  %s *%s", instanceIter->GetType()->GetName().c_str(),
            instanceIter->GetName().c_str());

        for (size_t i = 0; i < instanceIter->GetDimenCnt(); i += 1)
            m_incFile.Print("[%d]", instanceIter->GetDimenList()[i]);

        m_incFile.Print(";\n");
    }
}

void CHtvDesign::GenScModuleWriteInstanceDestructors(CHtvIdent *pClass)
{
    m_incFile.Print("\n");
    m_incFile.Print("  ~%s() {\n", pClass->GetName().c_str());

    CHtvArrayTblIter instanceIter(pClass->GetIdentTbl());
    for (instanceIter.Begin(); !instanceIter.End(); instanceIter++)
    {
        if (instanceIter->GetId() != CHtvIdent::id_variable || !instanceIter->IsReference())
            continue;

        m_incFile.Print("    delete %s;\n", instanceIter->GetName().c_str());
    }
    m_incFile.Print("  }\n");
}

void CHtvDesign::GenScModuleWriteSignalsAssignedConstant(CHtvIdent *pClass)
{
    // write out signals assigned a const value
    CHtvArrayTblIter signalIter(pClass->GetIdentTbl());
    for (signalIter.Begin(); !signalIter.End(); signalIter++)
    {
        if (!(signalIter->IsSignal() || signalIter->IsOutPort()) || !signalIter->IsConst())
            continue;

        m_incFile.Print("    %s = 0x%llx;\n", signalIter->GetName().c_str(), signalIter->GetConstValue().GetUint64());

        m_vFile.Print("  assign %s = %d;\n", signalIter->GetFlatIdent()->GetName().c_str(), signalIter->GetConstValue().GetUint64());
    }
    m_incFile.Print("\n");
}

void CHtvDesign::GenerateScMain()
{
    // write signals
    CHtvSignalTblIter signalIter(GetSignalTbl());
    for (signalIter.Begin() ; !signalIter.End(); signalIter++) {
        if (signalIter->IsClock())
            continue;

        if (signalIter->GetRefCnt() > 0)  {
            CHtvIdent *pType = signalIter->GetType();
            if (pType == 0)
                pType = signalIter->GetRefList(0)->GetModulePort()->GetBaseType();
            if (pType) {
                string signalDecl = VA("  sc_signal<%s>", pType->GetName().c_str());
                m_cppFile.Print("%-32s%s;\n", signalDecl.c_str(), signalIter->GetName().c_str());
            }
        }
    }
    m_cppFile.Print("\n");

    // write clocks
    CHtfeSignalTblIter clockIter(GetSignalTbl());
    for (clockIter.Begin() ; !clockIter.End(); clockIter++) {
        if (!clockIter->IsClock())
            continue;

        m_cppFile.Print("  sc_clock %s", clockIter->GetName().c_str());
        for (unsigned int i = 0; i < clockIter->GetClockTokenList().size(); i += 1) {
            CToken const &tl = clockIter->GetClockTokenList()[i];
            m_cppFile.Print("%s", tl.GetString().c_str());
            if (tl.GetToken() == tk_comma)
                m_cppFile.Print(" ");
        }
        m_cppFile.Print(";\n");
    }
    m_cppFile.Print("\n");

    // Generate module instances
    CHtfeInstanceTblIter instanceIter(GetInstanceTbl());
    for (instanceIter.Begin() ; !instanceIter.End(); instanceIter++)
    {
        m_cppFile.Print("  %s %s(\"%s\");\n",
            instanceIter->GetModule()->GetName().c_str(),
            instanceIter->GetName().c_str(),
            instanceIter->GetName().c_str());

        // Generate instance port list
        CHtfeInstancePortTblIter instancePortIter(instanceIter->GetInstancePortTbl());
        for (instancePortIter.Begin(); !instancePortIter.End(); instancePortIter++)
        {
            m_cppFile.Print("  %s.%s(%s%s);\n",
                instanceIter->GetName().c_str(),
                instancePortIter->GetName().c_str(),
                instancePortIter->GetSignal()->GetName().c_str(),
                instancePortIter->GetSignal()->IsClock() ? ".signal()" : "");
        }

        m_cppFile.Print("\n");
    }

    // generate sc_start statement
	CHtfeLex::CTokenList const & stStart = GetScStartList();
    if (stStart.size() == 0)
        ParseMsg(PARSE_ERROR, "Expected a sc_start statement");
    else {
        m_cppFile.Print("  sc_start");
        for (unsigned int i = 0; i < stStart.size(); i += 1) {
            CToken const &tl = stStart[i];
            m_cppFile.Print("%s", tl.GetString().c_str());
            if (tl.GetToken() == tk_comma)
                m_cppFile.Print(" ");
        }
        m_cppFile.Print(";\n");
    }

    m_cppFile.Print("\n  return 0;\n}\n");
}

void CHtvDesign::FindGlobalRefSet(CHtvIdent *pFunc, CHtvStatement *pStatement)
{
    // create identifier global list for each statement
    for ( ; pStatement; pStatement = pStatement->GetNext()) {
        switch (pStatement->GetStType()) {
        case st_compound:
            FindGlobalRefSet(pFunc, pStatement->GetCompound1());
            break;
        case st_assign:
        case st_return:
            FindGlobalRefSet(pFunc, pStatement->GetExpr());
            break;
        case st_if:
            // special case for memory write statements.
            FindGlobalRefSet(pFunc, pStatement->GetExpr());
            FindGlobalRefSet(pFunc, pStatement->GetCompound1());
            FindGlobalRefSet(pFunc, pStatement->GetCompound2());
            break;
        case st_switch:
        case st_case:
            FindGlobalRefSet(pFunc, pStatement->GetExpr());
            FindGlobalRefSet(pFunc, pStatement->GetCompound1());
            break;
        case st_default:
            FindGlobalRefSet(pFunc, pStatement->GetCompound1());
            break;
        case st_null:
        case st_break:
            break;
        default:
            Assert(0);
        }
    }
} 

void CHtvDesign::FindGlobalRefSet(CHtvIdent *pFunc, CHtvOperand *pExpr)
{
    // recursively desend expression tree

    Assert(pExpr != 0);
    if (pExpr->IsLeaf()) {
        CHtvIdent *pIdent = pExpr->GetMember();
        if (pIdent == 0 || pIdent->IsScState() || (pIdent->IsLocal() && !pIdent->IsFunction()))
            return;

        if (pIdent->GetId() == CHtvIdent::id_function) {

            for (unsigned int i = 0; i < pExpr->GetParamCnt(); i += 1) {
                //bool bIsReadOnly = pIdent->GetParamIsConst(i);

                //if (bIsReadOnly)
                FindGlobalRefSet(pFunc, pExpr->GetParam(i));
            }

            // find global variable references in called function
            if (!pIdent->IsGlobalReadRef()) {
                FindGlobalRefSet(pIdent, (CHtvStatement *)pIdent->GetStatementList());
                pIdent->SetIsGlobalReadRef(true);
            }

            // merge called references
            pFunc->MergeGlobalRefSet(pIdent->GetGlobalRefSet());

        } else if (pIdent->IsVariable()) {

            vector<bool> fieldConstList(pExpr->GetIndexCnt(), false);
            vector<int> refList(pExpr->GetIndexCnt(), 0);

            for (size_t j = 0; j < pExpr->GetIndexCnt(); j += 1) {
                fieldConstList[j] = pExpr->GetIndex(j)->IsConstValue();

                if (pExpr->GetIndex(j)->IsConstValue()) {
                    refList[j] = pExpr->GetIndex(j)->GetConstValue().GetSint32();
                    continue;
                }
            }

            do {
                int elemIdx = CalcElemIdx(pIdent, refList);
                CHtIdentElem identElem(pIdent, elemIdx);

                pFunc->AddGlobalRef(identElem);

            } while (!pIdent->DimenIter(refList, fieldConstList));

        }

    } else {

        CHtvOperand *pOp1 = pExpr->GetOperand1();
        CHtvOperand *pOp2 = pExpr->GetOperand2();
        CHtvOperand *pOp3 = pExpr->GetOperand3();

        if (pOp1) FindGlobalRefSet(pFunc, pOp1);
        if (pOp2) FindGlobalRefSet(pFunc, pOp2);
        if (pOp3) FindGlobalRefSet(pFunc, pOp3);
    }
}

void CHtvDesign::FindWriteRefSets(CHtvIdent *pHier)
{
    // create identifier write list for each statement, include pHier for local variables
    for (CHtvStatement *pStatement = (CHtvStatement *)pHier->GetStatementList(); pStatement; pStatement = pStatement->GetNext()) {

        switch (pStatement->GetStType()) {
        case st_compound:
            {
                CAlwaysType alwaysType = FindWriteRefVars(pHier, pStatement, pStatement->GetCompound1());

                if (alwaysType.IsError())
                    ParseMsg(PARSE_ERROR, pStatement->GetLineInfo(), "Mix of register and non-register assignments in compound statement");
            }
            break;
        case st_assign:
        case st_return:
            {
                // scPrims do not get a write ref list to avoid primitive ending up in always at blocks
                CAlwaysType alwaysType = FindWriteRefVars(pHier, pStatement, pStatement->GetExpr());

                if (alwaysType.IsError())
                    ParseMsg(PARSE_ERROR, pStatement->GetLineInfo(), "Mix of register and non-register assignments in assignment statement");
            }
            break;
        case st_if:
            {
                CAlwaysType alwaysType = FindWriteRefVars(pHier, pStatement, pStatement->GetCompound1());
                alwaysType += FindWriteRefVars(pHier, pStatement, pStatement->GetCompound2());

                if (alwaysType.IsError())
                    ParseMsg(PARSE_ERROR, pStatement->GetLineInfo(), "Mix of register and non-register assignments in if statement");

                if (!alwaysType.IsClocked())
                    FindWriteRefVars(pHier, pStatement, pStatement->GetExpr());
            }
            break;
        case st_switch:
            {
                CAlwaysType alwaysType = FindWriteRefVars(pHier, pStatement, pStatement->GetCompound1());

                if (alwaysType.IsError())
                    ParseMsg(PARSE_ERROR, pStatement->GetLineInfo(), "Mix of register and non-register assignments in switch statement");

                if (!alwaysType.IsClocked())
                    FindWriteRefVars(pHier, pStatement, pStatement->GetExpr());
            }
            break;
            //case st_case:
            //	FindWriteRefVars(pHier, pStatement, pStatement->GetExpr());
            //	FindWriteRefVars(pHier, pStatement, pStatement->GetCompound1());
            //	break;
            //case st_default:
            //	FindWriteRefVars(pHier, pStatement, pStatement->GetCompound1());
            //	break;
        case st_null:
        case st_break:
            break;
        default:
            Assert(0);
        }
    }

    // create synthesize statement lists
    int synSetId = 0;
    for (CHtvStatement *pStatement = (CHtvStatement *)pHier->GetStatementList() ; pStatement; pStatement = pStatement->GetNext()) {

        if (pStatement->IsSynthesized())
            continue;

        if (IsAssignToHtPrimOutput(pStatement))
            continue;

        synSetId += 1;

        // find set of variables that must be handled together
        int synSetSize = 1;
        pStatement->SetIsInSynSet();
        bool bIsAllAssignConst = true;

        // Scan statements to merge all statements with common write references
        MergeWriteRefSets(bIsAllAssignConst, synSetSize, pStatement);

        // Must scan twice due to indexed variables that may cause missed statements
        MergeWriteRefSets(bIsAllAssignConst, synSetSize, pStatement);

        pStatement->SetSynSetSize(synSetSize);
        if (bIsAllAssignConst) {
			CHtvOperand * pOp = pStatement->GetExpr()->GetOperand1();
            pOp->GetMember()->SetIsAssignOutput(pOp);
		}

        if (pStatement->IsScPrim()) {
            if (synSetSize > 1)
                ParseMsg(PARSE_ERROR, pStatement->GetExpr()->GetLineInfo(), "sc_prim output assigned outside of primitive");

            continue;
        }

        CHtIdentElem clockElem(pHier->GetClockIdent(), 0);
        bool bClockedAlwaysAt = pStatement->GetWriteRefSet().size() == 1 && pStatement->GetWriteRefSet()[0] == clockElem;

        if (bClockedAlwaysAt)
            pStatement->SetIsClockedAlwaysAt();

        FindWriteRefSets(pStatement, synSetId, synSetSize, bClockedAlwaysAt);
    }
}

bool CHtvDesign::IsAssignToHtPrimOutput(CHtvStatement * pStatement)
{
    Assert(pStatement);
    if (pStatement->GetStType() != st_assign)
        return false;

    CHtvOperand * pExpr = pStatement->GetExpr();
    if (pExpr == 0 || pExpr->GetOperator() != tk_equal)
        return false;

    CHtvOperand * pOp1 = pExpr->GetOperand1();
    CHtvOperand * pOp2 = pExpr->GetOperand2();

    if (!pOp2 || !pOp2->IsConstValue() || pOp2->GetConstValue().GetSint64() != 0)
        return false;

    if (!pOp1 || !pOp1->IsLeaf() || pOp1->GetMember() == 0 || pOp1->GetMember()->GetId() != CHtvIdent::id_variable)
        return false;

	int elemIdx = pOp1->GetMember()->GetDimenElemIdx(pOp1);
    return pOp1->GetMember()->IsHtPrimOutput(elemIdx);
}

void CHtvDesign::MergeWriteRefSets(bool &bIsAllAssignConst, int &synSetSize, CHtvStatement * pStatement)
{

    bIsAllAssignConst &= IsRightHandSideConst(pStatement);

    unsigned i;
    bool bAddedNew = true;
    while (bAddedNew) {
        bAddedNew = false;
        for (CHtvStatement * pStatement2 = pStatement->GetNext(); pStatement2; pStatement2 = pStatement2->GetNext()) {
            if (pStatement2->IsSynthesized() || pStatement2->IsInSynSet() || pStatement2->IsScPrim())
                continue;

            if (IsAssignToHtPrimOutput(pStatement2))
                continue;

            vector<CHtIdentElem> &writeRefSet2 = pStatement2->GetWriteRefSet();
            for (i = 0; i < writeRefSet2.size(); i += 1) {
                if (pStatement->IsInWriteRefSet(writeRefSet2[i]))
                    break;
            }

            if (i == writeRefSet2.size())
                continue;

            for (i = 0; i < writeRefSet2.size(); i += 1)
                pStatement->AddWriteRef(writeRefSet2[i]);

            pStatement2->SetIsInSynSet();
            synSetSize += 1;
            bAddedNew = true;

            bIsAllAssignConst &= IsRightHandSideConst(pStatement2);
        }
    }
}

bool CHtvDesign::IsRightHandSideConst(CHtvStatement * pStatement)
{
    bool bIsRightHandSideConst = pStatement->GetStType() == st_assign && !pStatement->IsScPrim() 
        && !pStatement->GetExpr()->IsLeaf() && pStatement->GetExpr()->GetOperand2()->IsConstValue()
        && pStatement->GetExpr()->GetOperand1()->GetSubFieldWidth() == pStatement->GetExpr()->GetOperand1()->GetMember()->GetWidth();

    if (!bIsRightHandSideConst)
        return false;

    // now check if left hand side has non-constant indexing
    for (size_t idx = 0; idx < pStatement->GetExpr()->GetOperand1()->GetIndexList().size(); idx += 1) {
        CHtvOperand *pIndexOp = pStatement->GetExpr()->GetOperand1()->GetIndex(idx);

        if (!pIndexOp->IsLeaf() || !pIndexOp->IsConstValue())
            return false;
    }

    return bIsRightHandSideConst;
}

void CHtvDesign::FindWriteRefSets(CHtvStatement *pStatement, int synSetId, int synSetSize, bool bClockedAlwaysAt)
{
    for (CHtvStatement *pStatement2 = (CHtvStatement *)pStatement; pStatement2; pStatement2 = pStatement2->GetNext()) {
        if (!pStatement2->IsInSynSet() || pStatement2->IsSynthesized())
            continue;

        pStatement2->SetSynSetId(synSetId);

        switch (pStatement2->GetStType()) {
        case st_assign:
            break;
        case st_compound:
            FindWriteRefSets(pStatement2->GetCompound1(), synSetId, synSetSize, bClockedAlwaysAt);
            break;
        case st_switch:
            break;
        case st_if:
        case st_null:
            break;
        default:
            Assert(0);
        }

        pStatement2->SetIsSynthesized();
    }
}

void CHtvDesign::FindXilinxFdPrims(CHtvIdent *pHier, CHtvObject * pObj)
{
    if (!g_htvArgs.GetXilinxFdPrims())
        return;

    // Find register assignment states that will be translated to FD prims
    FindXilinxFdPrims(pObj, (CHtvStatement *)pHier->GetStatementList());
}

void CHtvDesign::FindXilinxFdPrims(CHtvObject * pObj, CHtvStatement *pStatement)
{
    for ( ; pStatement; pStatement = pStatement->GetNext()) {

        if (pStatement->IsScPrim())
            continue;

        switch (pStatement->GetStType()) {
        case st_assign:
            IsXilinxClockedPrim(pObj, pStatement);
            break;
        case st_if:
            IsXilinxClockedPrim(pObj, pStatement);
            break;
        case st_switch:
            break;
        case st_compound:
            FindXilinxFdPrims(pObj, pStatement->GetCompound1());
            break;
        case st_null:
            break;
        default:
            Assert(0);
        }
    }
}

void CHtvDesign::SynIndividualStatements(CHtvIdent *pHier, CHtvObject * pObj, CHtvObject * pRtnObj)
{
    // synthesize statements
    for (CHtvStatement *pStatement = (CHtvStatement *)pHier->GetStatementList() ; pStatement; pStatement = pStatement->GetNext()) {

        int synSetSize = pStatement->GetSynSetSize();
        if (synSetSize == 0)
            continue;	// not first statement in writeRefSet

        // find set of variables that must be handled together
        if (pStatement->IsScPrim()) {
            Assert(synSetSize == 1);

            SynScPrim(pHier, pObj, pStatement);

            continue;
        }

        m_bClockedAlwaysAt = pStatement->IsClockedAlwaysAt();

        bool bAlwaysAtNeeded = true;
        if (m_bClockedAlwaysAt && g_htvArgs.GetXilinxFdPrims()) {
            // first translate assignments to Xilinx register
            bAlwaysAtNeeded = false;

            SynXilinxRegisters(pHier, pObj, pStatement, bAlwaysAtNeeded);
        }

        if (!bAlwaysAtNeeded) {
            // all register assignments were translated to primitives
            m_bClockedAlwaysAt = false;
            continue;
        }

        synSetSize = 2; // assign to 2 to force always @ block

        if (m_bClockedAlwaysAt)
            GenAlwaysAtHeader(pHier->GetClockIdent());
        else if (synSetSize > 1) {
            if (HandleAllConstantAssignments(pHier, pObj, pRtnObj, pStatement, synSetSize))
                continue;

            GenAlwaysAtHeader(true);

			RemoveWrDataMux(pStatement, synSetSize);
        }

        SynIndividualStatements(pHier, pObj, pRtnObj, pStatement, synSetSize);

        if (m_bClockedAlwaysAt || synSetSize > 1)
            GenAlwaysAtTrailer(true);

        m_bClockedAlwaysAt = false;
    }
}

void CHtvDesign::SynScPrim(CHtvIdent *pHier, CHtvObject * pObj, CHtvStatement *pStatement)
{
    CHtvOperand *pExpr = pStatement->GetExpr();
    CHtvIdent *pFunc = pExpr->GetMember();

    int firstOutput;
    for (firstOutput = 0; firstOutput < pExpr->GetParamListCnt(); firstOutput += 1)
        if (!pFunc->GetParamIsConst(firstOutput))
            break;

    Assert(firstOutput < pExpr->GetParamListCnt());

	CHtvOperand * pParamOp = pExpr->GetParam(firstOutput);
	int elemIdx = pParamOp->GetMember()->GetDimenElemIdx(pParamOp);
    string str = pParamOp->GetMember()->GetVerilogName(elemIdx) + "_";

    if (m_prmFp)
        fprintf(m_prmFp, "%s %s\n", str.c_str(), pFunc->GetName().c_str());

    m_vFile.Print("\n%s %s (\n", pFunc->GetName().c_str(), str.c_str());
    m_vFile.IncIndentLevel();

    // verify that function that calls primitive has a unique instance
    CHtvIdent * pCaller = pHier;
    while (pCaller->IsFunction() && !pCaller->IsMethod()) {
        if (pCaller->GetWriterListCnt() > 1)
            ParseMsg(PARSE_FATAL, pExpr->GetLineInfo(), 
            "clocked ht_prim %s was called from a non-unique function instance",
            pFunc->GetName().c_str());

        pCaller = pCaller->GetWriterList(0);
    }

    if (pFunc->GetHtPrimClkList().size() > 0) {
        if (!pCaller->GetClockIdent())
            ParseMsg(PARSE_FATAL, pExpr->GetLineInfo(),
			   "clocked ht_prim %s was called from non-clocked function",
				pFunc->GetName().c_str());

		string callerName = pCaller->GetClockIdent()->GetName();
		size_t len = callerName.size();

		int callerRate = -1;
		if (callerName[len-1] == 'x' && isdigit(callerName[len-2])) {
			callerRate = callerName[len-2]-'0';
			callerName = callerName.substr(0, len-2);
		}

		vector<string> & clkList = pFunc->GetHtPrimClkList();
		for (size_t i = 0; i < clkList.size(); i += 1) {

			string primClkName = clkList[i];
			len = primClkName.size();

			int primClkRate = -1;
			if (len >= 2 && clkList[i][len-1] == 'x' && isdigit(clkList[i][len-2]))
				primClkRate = clkList[i][len-2]-'0';

			bool bHtlClk = primClkName == "intfClk1x" || primClkName == "intfClk2x" || primClkName == "intfClk4x" ||
				primClkName == "clk1x" || primClkName == "clk2x" || primClkName == "clk4x";


			if (!bHtlClk) {

				m_vFile.Print(".%s(%s),\n", primClkName.c_str(), pCaller->GetClockIdent()->GetName().c_str());

			} else if (primClkName.substr(0, 7) == "intfClk" && primClkRate > 0 && callerRate > 0) {

				if (callerRate * primClkRate > 4)
					ParseMsg(PARSE_FATAL, pExpr->GetLineInfo(), 
						"unable to generate clock for ht_prim %s, prim clock %s with interface clock %s",
						pFunc->GetName().c_str(), primClkName.c_str(), pCaller->GetClockIdent()->GetName().c_str());
				else if (callerRate * primClkRate == 4)
					ParseMsg(PARSE_FATAL, pExpr->GetLineInfo(),
						"ht_prim %s requires unsupported 4x clock, prim clock %s with interface clock %s",
						pFunc->GetName().c_str(), primClkName.c_str(), pCaller->GetClockIdent()->GetName().c_str());

				m_vFile.Print(".%s(%s%dx),\n", primClkName.c_str(), callerName.c_str(), callerRate * primClkRate);

			} else if (primClkName.substr(0, 3) == "clk" && primClkRate > 0 && callerRate > 0) {

				m_vFile.Print(".%s(%s%dx),\n", primClkName.c_str(), callerName.c_str(), primClkRate);

			}
		}
    }

    char *pFirst = "";
    for (unsigned i = 0; i < pExpr->GetParamCnt(); i += 1) {

        if (!pFunc->GetParamIsScState(i)) {
            if (pExpr->GetParam(i)->IsConstValue()) {
                m_vFile.Print("%s.%s(",
                    pFirst, pFunc->GetParamName(i).c_str());

                if (pExpr->GetParam(i)->GetConstValue().IsSigned()) {
                    if (pExpr->GetParam(i)->GetConstValue().IsPos())
                        m_vFile.Print("%d'd%lld",
                        pFunc->GetParamType(i)->GetWidth(),
                        pExpr->GetParam(i)->GetConstValue().GetSint64());
                    else
                        m_vFile.Print("-%d'd%lld",
                        pFunc->GetParamType(i)->GetWidth(),
                        - pExpr->GetParam(i)->GetConstValue().GetSint64());
                } else
                    m_vFile.Print("%d'h%llx",
                    pFunc->GetParamType(i)->GetWidth(),
                    pExpr->GetParam(i)->GetConstValue().GetSint64());

                m_vFile.Print(")");
                pFirst = ",\n";
                continue;
            }

			CHtvOperand * pArgOp = pExpr->GetParam(i);
			int elemIdx = pArgOp->GetMember()->GetDimenElemIdx(pArgOp);
			string argStr = pArgOp->GetMember()->GetVerilogName(elemIdx);

            m_vFile.Print("%s.%s(%s", pFirst, pFunc->GetParamName(i).c_str(), argStr.c_str());

            if (pExpr->GetParam(i)->IsSubFieldValid()) {
                int lowBit, width;
                bool bIsConst = GetSubFieldRange(pExpr->GetParam(i), lowBit, width, false);
                Assert(pExpr->GetParam(i)->GetVerWidth() <= width);

                if (!bIsConst)
                    ParseMsg(PARSE_FATAL, pExpr->GetParam(i)->GetLineInfo(), "non-constant bit range specifier in primitive parameters");

                if (width == 1)
                    m_vFile.Print("[%d]", lowBit);
                else
                    m_vFile.Print("[%d:%d]", lowBit + width - 1, lowBit);
            }

            m_vFile.Print(")");
            pFirst = ",\n";
        }
    }

    m_vFile.DecIndentLevel();
    m_vFile.Print("\n);\n");
}

void CHtvDesign::SynXilinxRegisters(CHtvIdent *pHier, CHtvObject * pObj, CHtvStatement *pStatement, bool &bAlwaysAtNeeded)
{
    for (CHtvStatement *pStatement2 = pStatement; pStatement2; pStatement2 = pStatement2->GetNext()) {
        if (pStatement->GetSynSetId() != pStatement2->GetSynSetId())
            continue;

        switch (pStatement2->GetStType()) {
        case st_compound:
            SynXilinxRegisters(pHier, pObj, pStatement2->GetCompound1(), bAlwaysAtNeeded);
            break;
        case st_assign:
            if (!pStatement2->IsXilinxFdPrim() || !GenXilinxClockedPrim(pHier, pObj, pStatement2))
                bAlwaysAtNeeded = true;
            break;
        case st_if:
            if (!GenXilinxClockedPrim(pHier, pObj, pStatement2))
                bAlwaysAtNeeded = true;
            break;
        case st_switch:
            bAlwaysAtNeeded = true;
            break;
        default:
            Assert(0);
        }
    }
}

bool CHtvDesign::HandleAllConstantAssignments(CHtvIdent *pHier, CHtvObject * pObj, CHtvObject * pRtnObj, CHtvStatement *pStatement, int synSetSize)
{
    CHtvStatement *pLastStatement = 0;
    for (CHtvStatement *pStatement2 = pStatement; pStatement2; pStatement2 = pStatement2->GetNext()) {
        if (pStatement->GetSynSetId() != pStatement2->GetSynSetId())
            continue;

        if (pStatement2->GetStType() != st_assign)
            return false;

        CHtvOperand *pExpr = pStatement2->GetExpr();
        if (pExpr->IsLeaf() || !pExpr->GetOperand2()->IsConstValue())
            // right hand side was not a constant
            return false;

        for (size_t idx = 0; idx < pExpr->GetOperand1()->GetIndexList().size(); idx += 1) {
            CHtvOperand *pIndexOp = pExpr->GetOperand1()->GetIndex(idx);

            if (!pIndexOp->IsLeaf() || !pIndexOp->IsConstValue())
                return false;
        }

        pLastStatement = pStatement2;
    }

    CHtvOperand *pExpr = pLastStatement->GetExpr();
    // all right hand sides of assignment states were constants
    // all left hand sides are the same variable

    if (pExpr->GetOperand1()->GetSubFieldWidth() == pExpr->GetOperand1()->GetMember()->GetWidth()) {
        SynIndividualStatements(pHier, pObj, pRtnObj, pLastStatement, 1);
        return true;
    }

    // found one, create new variable
    pExpr = pStatement->GetExpr();
    string wireName = "w$" + pExpr->GetOperand1()->GetMember()->GetName();
    CHtvIdent *pWire = pHier->InsertIdent(wireName)->GetFlatIdent();
    pWire->SetId(CHtvIdent::id_variable);
    pWire->SetWidth(pExpr->GetOperand1()->GetWidth());
    pExpr->GetOperand2()->SetMinWidth(pExpr->GetOperand1()->GetWidth());

    m_vFile.Print("wire %s = ", pWire->GetName().c_str());
    GenSubExpr(pObj, pRtnObj, pExpr->GetOperand2());
    m_vFile.Print(";\n");

    pExpr->GetOperand2()->SetIsConstValue(false);
    pExpr->GetOperand2()->SetMember(pWire);

    return false;
}

struct CMemVarIndexList {
	void operator = (vector<CHtfeOperand *> & opList) {
		for (size_t i = 0; i < opList.size(); i += 1) {
			if (opList[i]->IsConstValue()) {
				int idx = opList[i]->GetConstValue().GetSint32();
				m_indexList.push_back(idx);
			}
			else
				Assert(0);
		}
	}
	bool operator == (vector<CHtfeOperand *> & opList) {
		Assert(opList.size() == m_indexList.size());
		for (size_t i = 0; i < opList.size(); i += 1) {
			if (opList[i]->IsConstValue()) {
				int idx = opList[i]->GetConstValue().GetSint32();
				if (m_indexList[i] != idx)
					return false;
			}
			else
				Assert(0);
		}
		return true;
	}

	vector<int> m_indexList;
};

struct CMemVarInfo {
	CMemVarInfo() { m_pMemVar = 0; m_bWrEnInit = false; m_bWrDataInit = false; m_bWrEnSet = false;  m_wrEnSize = 0; m_bConvert = true; }

	CHtvIdent * m_pMemVar;
	bool m_bWrEnInit;
	bool m_bWrDataInit;
	bool m_bWrEnSet;
	vector<int> m_wrEnLowBit;
	int m_wrEnSize;
	bool m_bConvert;
	CMemVarIndexList m_indexList;
};

void CHtvDesign::RemoveWrDataMux(CHtvStatement *pStatement, int synSetSize)
{
	//if (GetAlwaysBlockIdx() == 4)
	//	bool stop = true;

	vector<CMemVarInfo> memVarList;

	for (CHtvStatement ** ppStatement2 = &pStatement; *ppStatement2; ppStatement2 = (*ppStatement2)->GetPNext()) {
		if (pStatement->GetSynSetId() != (*ppStatement2)->GetSynSetId())
			continue;

		switch ((*ppStatement2)->GetStType()) {
		case st_assign:
		{
			if ((*ppStatement2)->GetExpr()->IsLeaf())
				break;

			CHtvOperand * pOp1 = (*ppStatement2)->GetExpr()->GetOperand1();
			CHtvIdent * pIdent = pOp1->GetMember();
			if (pIdent->IsMemVarWrData() || pIdent->IsMemVarWrEn()) {
				CHtvIdent * pMemVar = pIdent->GetMemVar();

				CMemVarInfo * pMemVarInfo = 0;
				for (size_t i = 0; i < memVarList.size(); i += 1) {
					if (memVarList[i].m_pMemVar == pMemVar && memVarList[i].m_indexList == pOp1->GetIndexList()) {
						pMemVarInfo = &memVarList[i];
						break;
					}
				}
				if (pMemVarInfo == 0) {
					memVarList.push_back(CMemVarInfo());
					pMemVarInfo = &memVarList.back();
					pMemVarInfo->m_pMemVar = pMemVar;
					pMemVarInfo->m_indexList = pOp1->GetIndexList();
				}

				CHtvOperand * pOp2 = (*ppStatement2)->GetExpr()->GetOperand2();
				if (pOp2->IsConstValue() && pOp2->GetConstValue().IsZero()) {
					if (pIdent->IsMemVarWrData()) {
						if (pMemVarInfo->m_bWrDataInit)
							pMemVarInfo->m_bConvert = false;
						else
							pMemVarInfo->m_bWrDataInit = true;
					} else {
						if (pMemVarInfo->m_bWrEnInit)
							pMemVarInfo->m_bConvert = false;
						else
							pMemVarInfo->m_bWrEnInit = true;
					}
				}
			}
			break;
		}
		case st_if:
		{
			bool bConvert = true;
			if ((*ppStatement2)->GetCompound2())
				return;

			for (CHtvStatement **ppStatement3 = (*ppStatement2)->GetPCompound1(); *ppStatement3; ) {
				if ((*ppStatement3)->GetStType() != st_assign)
					return;

				CHtvOperand * pOp1 = (*ppStatement3)->GetExpr()->GetOperand1();
				if (pOp1 == 0)
					return; // function call

				CHtvIdent * pIdent = pOp1->GetMember();
				if (pIdent->IsMemVarWrData() || pIdent->IsMemVarWrEn()) {
					CHtvIdent * pMemVar = pIdent->GetMemVar();

					size_t k;
					for (k = 0; k < pOp1->GetIndexList().size(); k += 1) {
						if (!pOp1->GetIndexList()[k]->IsConstValue())
							break;
					}

					if (k < pOp1->GetIndexList().size()) {
						for (size_t i = 0; i < memVarList.size(); i += 1) {
							if (memVarList[i].m_pMemVar == pMemVar)
								memVarList[i].m_bConvert = false;
						}
						ppStatement3 = (*ppStatement3)->GetPNext();
						continue;
					}

					CMemVarInfo * pMemVarInfo = 0;
					for (size_t i = 0; i < memVarList.size(); i += 1) {
						if (memVarList[i].m_pMemVar == pMemVar && memVarList[i].m_indexList == pOp1->GetIndexList()) {
							pMemVarInfo = &memVarList[i];
							break;
						}
					}

					if (pMemVarInfo == 0) {
						ppStatement3 = (*ppStatement3)->GetPNext();
						continue;
					}

					CHtvOperand * pOp2 = (*ppStatement3)->GetExpr()->GetOperand2();
					if (pIdent->IsMemVarWrEn()) {
						if (pOp2->IsConstValue() && pOp2->GetConstValue().GetUint64() == 1) {
							if (!pMemVarInfo->m_bWrEnInit || pMemVarInfo->m_bWrEnSet)
								pMemVarInfo->m_bConvert = false;
							else {
								int highBit, lowBit;
								pOp1->GetDistRamWeWidth(highBit, lowBit);
								if (highBit == -1 && lowBit == -1) {
									if (pMemVarInfo->m_wrEnSize == 0)
										pMemVarInfo->m_bWrEnSet = true;
									else
										pMemVarInfo->m_bConvert = false;
								} else {
									size_t i;
									for (i = 0; i < pMemVarInfo->m_wrEnLowBit.size(); i += 1) {
										if (lowBit == pMemVarInfo->m_wrEnLowBit[i]) {
											pMemVarInfo->m_bConvert = false;
											break;
										}
									}
									if (i == pMemVarInfo->m_wrEnLowBit.size()) {
										pMemVarInfo->m_wrEnLowBit.push_back(lowBit);
										pMemVarInfo->m_wrEnSize += highBit - lowBit + 1;

										if (pMemVarInfo->m_wrEnSize == pMemVar->GetWidth())
											pMemVarInfo->m_bWrEnSet = true;
									}
								}
							}
						}
					}

					if (pIdent->IsMemVarWrData()) {
						if (pMemVarInfo->m_bWrEnInit && pMemVarInfo->m_bWrDataInit && pMemVarInfo->m_bWrEnSet && pMemVarInfo->m_bConvert && bConvert) {
							// Do the conversion
							CHtvStatement * pStatement3 = *ppStatement3;
							*ppStatement3 = (*ppStatement3)->GetNext();

							pStatement3->SetNext(*ppStatement2);
							*ppStatement2 = pStatement3;
							ppStatement2 = (*ppStatement2)->GetPNext();

							pStatement3->SetSynSetId(pStatement->GetSynSetId());

							pMemVarInfo->m_bConvert = false;
							continue;

						} else
							pMemVarInfo->m_bConvert = false;
					}
				} else
					bConvert = false;

				ppStatement3 = (*ppStatement3)->GetPNext();
			}
			break;
		}
		default:
			return;
		}
	}
}

void CHtvDesign::SynIndividualStatements(CHtvIdent *pHier, CHtvObject * pObj, CHtvObject * pRtnObj, CHtvStatement *pStatement, int synSetSize)
{
    for (CHtvStatement *pStatement2 = pStatement; pStatement2; pStatement2 = pStatement2->GetNext()) {
        if (pStatement->GetSynSetId() != pStatement2->GetSynSetId())
            continue;

        switch (pStatement2->GetStType()) {
        case st_compound:
            SynIndividualStatements(pHier, pObj, pRtnObj, pStatement2->GetCompound1(), synSetSize);
            break;
        case st_assign:
            {
                if (pStatement2->IsXilinxFdPrim())
                    break;

                CHtvOperand *pExpr = pStatement2->GetExpr();

                if (pExpr->IsLeaf() && pExpr->GetMember() != 0 && pExpr->GetMember()->IsMethod())
                    // Remove UpdateOutputs()
                    break;

                bool bIsTask = IsTask(pStatement);

                if (bIsTask && synSetSize == 1)
                    GenAlwaysAtHeader(false);

                SynAssignmentStatement(pHier, pObj, pRtnObj, pStatement2, bIsTask || synSetSize > 1 || m_bClockedAlwaysAt);

                if (bIsTask && synSetSize == 1)
                    GenAlwaysAtTrailer(false);
            }
            break;
        case st_switch:
            if (synSetSize == 1)
                GenAlwaysAtHeader(false);

            SynSwitchStatement(pHier, pObj, pRtnObj, pStatement2);

            if (synSetSize == 1)
                GenAlwaysAtTrailer(false);
            break;
        case st_if:
            if (pStatement2->IsXilinxFdPrim())
                break;

            if (synSetSize == 1)
                GenAlwaysAtHeader(pStatement2->GetCompound2() != 0);

            SynIfStatement(pHier, pObj, pRtnObj, pStatement2);

            if (synSetSize == 1)
                GenAlwaysAtTrailer(pStatement2->GetCompound2() != 0);
            break;
        case st_null:
            break;
        default:
            Assert(0);
            break;
        }
    }
}

void CHtvDesign::SynScLocalAlways(CHtvIdent *pMethod, CHtvObject * pObj)
{
    // first check if there are any local variables
    CHtvIdentTblIter identIter(pMethod->GetIdentTbl());
    for (identIter.Begin(); !identIter.End(); identIter++)
        if (identIter->IsVariable() && !identIter->IsParam() && identIter->IsScLocal())
            break;

    if (identIter.End())
        return;	// no local variables

    GenAlwaysAtHeader(false);

    // blocks with local variables must be named
    string name = pMethod->GetName();
    name.insert(name.size(), "$");
    m_vFile.Print("begin : %s\n", name.c_str());

    m_vFile.IncIndentLevel();

    // now declare local variables
	m_vFile.SetVarDeclLines(true);
    for ( ; !identIter.End(); identIter++) {
        if (identIter->IsVariable() && !identIter->IsParam() && identIter->IsScLocal()) {
			string bitRange;
            if (identIter->GetWidth() > 1)
                bitRange = VA("[%d:0] ", identIter->GetWidth()-1);
            m_vFile.Print("reg %s%s;\n",
                bitRange.c_str(), identIter->GetName().c_str());
        }
    }
    m_vFile.Print("\n");
	m_vFile.SetVarDeclLines(false);

    SynCompoundStatement(pMethod, pObj, 0, false, true);

    m_vFile.DecIndentLevel(); // always
}

void CHtvDesign::SynCompoundStatement(CHtvIdent *pHier, CHtvObject * pObj, CHtvObject * pRtnObj, bool bForceSyn, bool bDeclareLocalVariables)
{
    if (bDeclareLocalVariables) {
        // blocks with local variables must be named
        m_vFile.Print("begin : %s$\n", pHier->GetName().c_str());
    } else
        m_vFile.Print("begin\n");

    m_vFile.IncIndentLevel();

    if (bDeclareLocalVariables) {

        // now declare temps
        CHtvIdentTblIter identIter(pHier->GetFlatIdentTbl());
        for (identIter.Begin(); !identIter.End(); identIter++) {

			for (int elemIdx = 0; elemIdx < identIter->GetDimenElemCnt(); elemIdx += 1) {

				if (identIter->IsVariable() && !identIter->IsParam() && !identIter->IsRegClkEn() && !identIter->IsHtPrimOutput(elemIdx)) {
					string bitRange;
					if (identIter->GetWidth() > 1)
						bitRange = VA("[%d:0] ", identIter->GetWidth()-1);

                //vector<int> refList(identIter->GetDimenCnt(), 0);

                //do {
					string identName = identIter->GetVerilogName(elemIdx);
                    //string identName = GenVerilogIdentName(identIter->GetName(), refList);

                    GenHtAttributes(identIter(), identName);

                    m_vFile.Print("reg %s%s;\n", bitRange.c_str(), identName.c_str());

				} //while (!identIter->DimenIter(refList));
			}
        }
    }

    SynStatementList(pHier, pObj, pRtnObj, (CHtvStatement *)pHier->GetStatementList());

    m_vFile.DecIndentLevel();
    m_vFile.Print("end\n");
}

void CHtvDesign::SynStatementList(CHtvIdent *pHier, CHtvObject * pObj, CHtvObject * pRtnObj, CHtvStatement *pStatement)
{
    if (pStatement == 0)
        return;

    for ( ; pStatement; pStatement = pStatement->GetNext()) {

        switch (pStatement->GetStType()) {
        case st_compound:
            SynStatementList(pHier, pObj, pRtnObj, pStatement->GetCompound1());
            break;
        case st_assign:
            SynAssignmentStatement(pHier, pObj, pRtnObj, pStatement);
            break;
        case st_switch:
            SynSwitchStatement(pHier, pObj, pRtnObj, pStatement);
            break;
        case st_if:
            SynIfStatement(pHier, pObj, pRtnObj, pStatement);
            break;
        case st_break:
            break;
        case st_return:
            SynReturnStatement(pHier, pObj, pRtnObj, pStatement);
            break;
        default:
            Assert(0);
            break;
        }
    }
}

void CHtvDesign::SynReturnStatement(CHtvIdent *pFunc, CHtvObject * pObj, CHtvObject * pRtnObj, CHtvStatement *pStatement)
{
	Assert(pFunc->IsFunction());

	CHtvOperand * pExpr = PrependObjExpr(pObj, pStatement->GetExpr());
    if (pExpr == 0)
        return;

	if (pFunc->IsReturnRef()) {
		Assert(pRtnObj && !pRtnObj->m_pOp);

		bool bFoundSubExpr = false;
		bool bWriteIndex = false;
		if (pExpr->GetOperator() == tk_period && pExpr->GetOperand2()->IsFunction()) {
			FindSubExpr(pObj, pRtnObj, pExpr, bFoundSubExpr, bWriteIndex, false);

			pRtnObj->m_pOp = pExpr->GetTempOp();
		} else {
			pExpr->SetTempOpSubGened(false);
			pRtnObj->m_pOp = pExpr;
		}

		if (bFoundSubExpr) {
			m_vFile.DecIndentLevel();
			m_vFile.Print("end\n");
		}

		return;
	}

    // used for translating return statements to 'function = expression'
    CHtvOperand *pRslt = new CHtvOperand;
	pRslt->SetLineInfo(pExpr->GetLineInfo());
    pRslt->SetMember(pFunc);
    pRslt->SetMinWidth(pFunc->GetWidth());
    pRslt->SetVerWidth(pFunc->GetWidth());
    pRslt->SetIsLeaf();

    CHtvOperand *pEqual = new CHtvOperand;
	pEqual->SetLineInfo(pExpr->GetLineInfo());
    pEqual->SetOperator(tk_equal);
    pEqual->SetOperand1(pRslt);
    pEqual->SetOperand2(pExpr);
    pEqual->SetVerWidth(pRslt->GetVerWidth());

    bool bFoundSubExpr;
    bool bWriteIndex;
	FindSubExpr(pObj, 0, pEqual, bFoundSubExpr, bWriteIndex, false);

	if (!pFunc->IsReturnRef()) {
		PrintSubExpr(pObj, 0, pEqual);
	    m_vFile.Print(";\n");
	}

    if (bFoundSubExpr) {
        m_vFile.DecIndentLevel();
        m_vFile.Print("end\n");
    }
}

void CHtvDesign::SynAssignmentStatement(CHtvIdent *pHier, CHtvObject * pObj, CHtvObject * pRtnObj, CHtvStatement *pStatement, bool bIsAlwaysAt)
{
    if (pStatement->IsXilinxFdPrim())
        return;

    if (pStatement->IsScPrim()) {

		m_vFile.SetHtPrimLines(true);

        SynScPrim(pHier, pObj, pStatement);

		m_vFile.SetHtPrimLines(false);

        return;
    }

	if (pStatement->IsInitStatement()) {
		CHtvOperand * pOp1 = pStatement->GetExpr()->GetOperand1();
		int elemIdx = pOp1->GetMember()->GetDimenElemIdx(pOp1);
		if (pOp1->GetMember()->IsHtPrimOutput(elemIdx))
			return;
	}

#ifdef _WIN32
	if (pStatement->GetExpr()->GetLineInfo().m_lineNum == 619)
		bool stop = true;
#endif

    CHtvOperand *pExpr = PrependObjExpr(pObj, pStatement->GetExpr());

    // handle distributed ram write enables
    if (!pExpr->IsLeaf() && pExpr->GetOperator() == tk_equal) {
        CHtvOperand * pLeftOfEqualOp = pExpr->GetOperand1();
        if (pLeftOfEqualOp->IsLeaf() && pLeftOfEqualOp->IsVariable()) {
            CHtvIdent * pLeftOfEqualIdent = pLeftOfEqualOp->GetMember();
            if (pLeftOfEqualIdent->GetHtDistRamWeWidth() != 0) {
                SynHtDistRamWe(pHier, pObj, pRtnObj, pExpr, bIsAlwaysAt);
                return;
            }
        }
    }

    if (!bIsAlwaysAt)
        m_vFile.Print("assign ");

    bool bFoundSubExpr = false;
    bool bWriteIndex = false;
	FindSubExpr(pObj, pRtnObj, pExpr, bFoundSubExpr, bWriteIndex);

    if (!pExpr->IsLeaf() && pExpr->GetOperator() != tk_period && !bWriteIndex || !bFoundSubExpr)
        GenSubExpr(pObj, pRtnObj, pExpr, false, false);

    if (!pExpr->IsLeaf() && pExpr->GetOperator() != tk_period && !bWriteIndex)
        m_vFile.Print(";\n");

    if (bFoundSubExpr) {
        m_vFile.DecIndentLevel();
        m_vFile.Print("end\n");
    }
}

void CHtvDesign::SynIfStatement(CHtvIdent *pHier, CHtvObject * pObj, CHtvObject * pRtnObj, CHtvStatement *pStatement)
{
    // check if statement is a memory write
    CHtvStatement **ppIfList = pStatement->GetPCompound1();
    RemoveMemoryWriteStatements(ppIfList);

    if (*ppIfList == 0 && pStatement->GetCompound2() == 0)
        return; // remove if statement

	CHtvOperand *pExpr = PrependObjExpr(pObj, pStatement->GetExpr());

    bool bFoundSubExpr;
    bool bWriteIndex;
    FindSubExpr(pObj, pRtnObj, pExpr, bFoundSubExpr, bWriteIndex);

    m_vFile.Print("if (");
    PrintSubExpr(pObj, pRtnObj, pExpr, false, false, false, "", tk_toe);
    m_vFile.Print(")\n");

    m_vFile.IncIndentLevel();
    m_vFile.Print("begin\n");
    m_vFile.IncIndentLevel();

    SynStatementList(pHier, pObj, pRtnObj, pStatement->GetCompound1());

    m_vFile.DecIndentLevel();
    m_vFile.Print("end\n");
    m_vFile.DecIndentLevel();

    if (pStatement->GetCompound2()) {
        m_vFile.Print("else\n");

        m_vFile.IncIndentLevel();
        m_vFile.Print("begin\n");
        m_vFile.IncIndentLevel();

        SynStatementList(pHier, pObj, pRtnObj, pStatement->GetCompound2());

        m_vFile.DecIndentLevel();
        m_vFile.Print("end\n");
        m_vFile.DecIndentLevel();
    }

    if (bFoundSubExpr) {
        m_vFile.DecIndentLevel();
        m_vFile.Print("end\n");
    }
}

void CHtvDesign::SynSwitchStatement(CHtvIdent *pHier, CHtvObject * pObj, CHtvObject * pRtnObj, CHtvStatement *pSwitch)
{
    if (SynXilinxRom(pHier, pSwitch))
        return;

	CHtvOperand *pSwitchOperand = PrependObjExpr(pObj, pSwitch->GetExpr());


    bool bForceTemp = !pSwitchOperand->IsLeaf();
    bool bFoundSubExpr;
    bool bWriteIndex;
    FindSubExpr(pObj, pRtnObj, pSwitchOperand, bFoundSubExpr, bWriteIndex, bForceTemp);

    m_vFile.Print("case (");

    int caseWidth = max(pSwitchOperand->GetMinWidth(), pSwitchOperand->GetSigWidth());
    if (pSwitchOperand->HasExprTempVar())
        pSwitchOperand->SetExprTempVarWidth( caseWidth );

    PrintSubExpr(pObj, pRtnObj, pSwitchOperand);

    m_vFile.Print(")\n");
    m_vFile.IncIndentLevel();

    CHtvStatement *pDefaultStatement = 0;
    CHtvOperand *pCaseOperand;
    CHtvStatement *pStatement = pSwitch->GetCompound1();
    for (; pStatement ; pStatement = pStatement->GetNext()) {

        // if a combination of case and default for same statement then drop case entry points and use default entry.
        bool bFoundDefault = false;
        for (CHtvStatement *pStatement2 = pStatement;; pStatement2 = pStatement2->GetNext()) {
            bFoundDefault |= pStatement2->GetStType() == st_default;

            if (pStatement2->GetCompound1() != 0 || pStatement2->GetNext() == 0) {
                if (bFoundDefault)
                    pStatement = pStatement2;
                break;
            }
        }

        if (bFoundDefault) {
            pDefaultStatement = pStatement;
            continue;
        } else {
            for(;; pStatement = pStatement->GetNext()) {
				pCaseOperand = PrependObjExpr(pObj, pStatement->GetExpr());

                bool bFoundSubExpr;
                bool bWriteIndex;
                FindSubExpr(pObj, pRtnObj, pCaseOperand, bFoundSubExpr, bWriteIndex);

                if (bFoundSubExpr)
                    ParseMsg(PARSE_FATAL, "functions in case values not supported");

                if (pCaseOperand->IsConstValue())
                    pCaseOperand->SetOpWidth( caseWidth );

                GenSubExpr(pObj, pRtnObj, pCaseOperand, false, false);

                if (pStatement->GetCompound1() != 0) {
                    m_vFile.Print(":\n");
                    break;
                } else
                    m_vFile.Print(", ");
            }
        }

        m_vFile.IncIndentLevel();
        m_vFile.Print("begin\n");
        m_vFile.IncIndentLevel();

        SynStatementList(pHier, pObj, pRtnObj, pStatement->GetCompound1());

        m_vFile.DecIndentLevel();
        m_vFile.Print("end\n");
        m_vFile.DecIndentLevel();
    }

    // default statement must be at the end of the case statement
    if (pDefaultStatement && pDefaultStatement->GetCompound1() && pDefaultStatement->GetCompound1()->GetStType() != st_break) {
        m_vFile.Print("default:\n");

        m_vFile.IncIndentLevel();
        m_vFile.Print("begin\n");
        m_vFile.IncIndentLevel();

        SynStatementList(pHier, pObj, pRtnObj, pDefaultStatement->GetCompound1());

        m_vFile.DecIndentLevel();
        m_vFile.Print("end\n");
        m_vFile.DecIndentLevel();
    }

    m_vFile.DecIndentLevel();
    m_vFile.Print("endcase\n");

    if (bFoundSubExpr) {
        m_vFile.DecIndentLevel();
        m_vFile.Print("end\n");
    }
}

CHtvOperand * CHtvDesign::CreateTempOp(CHtvOperand * pOp, string const &tempName)
{
	CHtvIdent * pIdent = new CHtvIdent;
	pIdent->SetName(tempName);
	pIdent->SetType(pOp->GetMember()->GetType());
	pIdent->SetId(CHtfeIdent::id_variable);
	pIdent->SetLineInfo(pOp->GetLineInfo());
	pIdent->SetPrevHier(GetTopHier());

	CHtvOperand * pTempOp = new CHtvOperand;
	pTempOp->InitAsIdentifier(pOp->GetLineInfo(), pIdent);

	return pTempOp;
}


// Create a new operand node with the appended subfield list from a second node
CHtvOperand * CHtvDesign::CreateObject(CHtvOperand * pObj, CHtvOperand * pOp)
{
	CHtvOperand * pNewObj = new CHtvOperand;
	pNewObj->InitAsIdentifier(pOp->GetLineInfo(), pObj->GetMember());
	pNewObj->SetIndexList(pObj->GetIndexList());

	if (pNewObj->IsLinkedOp())
		Assert(0);

	// copy subField list
	CScSubField * pSubField = pObj->GetSubField();
	CScSubField ** pNewSubField = pNewObj->GetSubFieldP();
	for (; pSubField; pSubField = pSubField->m_pNext) {
		CScSubField * pSf = new CScSubField;
		*pSf = *pSubField;
		*pNewSubField = pSf;
		pNewSubField = &pSf->m_pNext;
	}

	// append new subField to list
	if (pOp->GetSubField() == 0) {
		CScSubField * pSf = new CScSubField;

		pSf->m_pIdent = pOp->GetMember();
		pSf->m_indexList = pOp->GetIndexList();
		pSf->m_bBitIndex = false;
		pSf->m_subFieldWidth = pOp->GetMember()->GetWidth();
		pSf->m_pNext = 0;

		*pNewSubField = pSf;
	} else {
		pSubField = pOp->GetSubField();

		for (; pSubField; pSubField = pSubField->m_pNext) {
			CScSubField * pSf = new CScSubField;
			*pSf = *pSubField;
			*pNewSubField = pSf;
			pNewSubField = &pSf->m_pNext;
		}
	}

	return pNewObj;
}

void CHtvDesign::GenSubObject(CHtvObject * pSubObj, CHtvObject * pObj, CHtvOperand * pOp)
{
	if (!pOp->GetMember()->IsStruct()) {
		pSubObj->m_pOp = pOp;

	} else {
		Assert(pObj);

		// found a member, prepend the object
		CHtvOperand * pNewOp = new CHtvOperand;
		pNewOp->InitAsIdentifier(pOp->GetLineInfo(), pObj->m_pOp->GetMember());
		pNewOp->SetIndexList(pObj->m_pOp->GetIndexList());

		if (pNewOp->IsLinkedOp())
			Assert(0);

		// copy subField list
		CScSubField * pSubField = pObj->m_pOp->GetSubField();
		CScSubField ** pNewSubField = pNewOp->GetSubFieldP();
		for (; pSubField; pSubField = pSubField->m_pNext) {
			CScSubField * pSf = new CScSubField;
			*pSf = *pSubField;
			*pNewSubField = pSf;
			pNewSubField = &pSf->m_pNext;
		}

		// append new subField to list
		if (pOp->GetSubField() == 0) {
			CScSubField * pSf = new CScSubField;

			pSf->m_pIdent = pOp->GetMember();
			pSf->m_indexList = pOp->GetIndexList();
			pSf->m_bBitIndex = false;
			pSf->m_subFieldWidth = pOp->GetMember()->GetWidth();
			pSf->m_pNext = 0;

			*pNewSubField = pSf;
		} else {
			pSubField = pOp->GetSubField();

			for (; pSubField; pSubField = pSubField->m_pNext) {
				CScSubField * pSf = new CScSubField;
				*pSf = *pSubField;
				*pNewSubField = pSf;
				pNewSubField = &pSf->m_pNext;
			}
		}

	}
}

CHtvOperand * CHtvDesign::PrependObjExpr(CHtvObject * pObj, CHtvOperand * pExpr)
{
	// Recursively decend expression tree and duplicate tree. References to members
	// are replaced with obj.member

	if (pObj == 0 || pExpr == 0)
		return pExpr;

	if (pExpr->IsLeaf()) {
		if (pExpr->IsConstValue())
			return pExpr;

		CHtvIdent *pIdent = pExpr->GetMember();

		if (pIdent->IsFunction()) {
			// methods do not get a prepended object but it's parameter do
			if (pExpr->GetParamCnt() > 0) {

				CHtvOperand * pNewExpr = new CHtvOperand;
				pNewExpr->InitAsFunction(pExpr->GetLineInfo(), pExpr->GetMember());
				pNewExpr->SetType(pExpr->GetType());

				for (size_t paramIdx = 0; paramIdx < pExpr->GetParamCnt(); paramIdx += 1) {
					CHtvOperand * pParamOp = pExpr->GetParam(paramIdx);
					pNewExpr->GetParamList().push_back(PrependObjExpr(pObj, pParamOp));
				}

				return pNewExpr;
			}

			return pExpr;
		}

		if (pIdent->GetPrevHier() && !pIdent->GetPrevHier()->IsStruct())
			return pExpr;

		// found a member, prepend the object
		return PrependObjExpr(pObj, pObj->m_pOp, pExpr);

	} else {

		EToken tk = pExpr->GetOperator();

		CHtvOperand *pOp1 = PrependObjExpr(pObj, pExpr->GetOperand1());
		CHtvOperand *pOp2 = PrependObjExpr(pObj, pExpr->GetOperand2());
		CHtvOperand *pOp3 = PrependObjExpr(pObj, pExpr->GetOperand3());

		CHtvOperand * pNewExpr = new CHtvOperand;
		pNewExpr->InitAsOperator(pExpr->GetLineInfo(), tk, pOp1, pOp2, pOp3);
		pNewExpr->SetType(pExpr->GetType());
		pNewExpr->SetIsParenExpr(pExpr->IsParenExpr());

		return pNewExpr;
	}
}

CHtvOperand * CHtvDesign::PrependObjExpr(CHtvObject * pObj, CHtvOperand * pObjOp, CHtvOperand * pExpr)
{
	// replicate pObjOp and append pExpr's subField list to each identifier
	if (pObjOp->IsLeaf() && pObjOp->IsConstValue())
		return pObjOp;

	if (pObjOp->IsLeaf() || pObjOp->GetOperator() == tk_period) {
		CHtvOperand * pNewExpr = new CHtvOperand;
		pNewExpr->InitAsIdentifier(pExpr->GetLineInfo(), pObjOp->GetMember());
		pNewExpr->SetType(pObjOp->GetType());
		pNewExpr->SetIndexList(pObjOp->GetIndexList());

		if (pNewExpr->IsLinkedOp())
			Assert(0);

		// copy subField list
		CScSubField * pSubField = pObjOp->GetSubField();
		CScSubField ** pNewSubField = pNewExpr->GetSubFieldP();
		for (; pSubField; pSubField = pSubField->m_pNext) {
			CScSubField * pSf = new CScSubField;
			*pSf = *pSubField;
			*pNewSubField = pSf;
			pNewSubField = &pSf->m_pNext;
		}

		// add expr's member as a subfield
		if (pExpr->GetSubField() == 0 || pExpr->GetSubField()->m_pIdent != pExpr->GetMember()) {
			CScSubField * pSf = new CScSubField;
			pSf->m_pIdent = pExpr->GetMember();
			pSf->m_indexList = pExpr->GetIndexList();
			pSf->m_bBitIndex = false;
			pSf->m_subFieldWidth = pExpr->GetMember()->GetWidth();
			pSf->m_pNext = 0;

			*pNewSubField = pSf;
			pNewSubField = &pSf->m_pNext;
		}

		// append new subField to list
		if (pExpr->GetSubField() != 0) {
			pSubField = pExpr->GetSubField();

			for (; pSubField; pSubField = pSubField->m_pNext) {
				CScSubField * pSf = new CScSubField;
				pSf->m_pIdent = pSubField->m_pIdent;
				pSf->m_subFieldWidth = pSubField->m_subFieldWidth;
				pSf->m_bBitIndex = pSubField->m_bBitIndex;
				pSf->m_pNext = 0;

				for (size_t idx = 0; idx < pSubField->m_indexList.size(); idx += 1) {
					CHtvOperand * pIndexOp = (CHtvOperand *)pSubField->m_indexList[idx];
					pSf->m_indexList.push_back(PrependObjExpr(pObj, pIndexOp));
				}

				*pNewSubField = pSf;
				pNewSubField = &pSf->m_pNext;
			}
		}

		return pNewExpr;
	} else {

		EToken tk = pObjOp->GetOperator();
		Assert(tk == tk_question);

		CHtvOperand *pObjOp1 = pObjOp->GetOperand1();
		CHtvOperand *pObjOp2 = pObjOp->GetOperand2();
		CHtvOperand *pObjOp3 = pObjOp->GetOperand3();

		CHtvOperand *pNewOp1 = pObjOp1;// ? PrependObjExpr(pObj, pObjOp1, pExpr) : 0;
		CHtvOperand *pNewOp2 = pObjOp2 ? PrependObjExpr(pObj, pObjOp2, pExpr) : 0;
		CHtvOperand *pNewOp3 = pObjOp3 ? PrependObjExpr(pObj, pObjOp3, pExpr) : 0;

		CHtvOperand * pNewExpr = new CHtvOperand;
		pNewExpr->InitAsOperator(pExpr->GetLineInfo(), tk, pNewOp1, pNewOp2, pNewOp3);
		pNewExpr->SetType(pObjOp->GetType());
		pNewExpr->SetIsParenExpr(pExpr->IsParenExpr());

		return pNewExpr;
	}
}

void CHtvDesign::FindSubExpr(CHtvObject * pObj, CHtvObject * pRtnObj, CHtvOperand *pExpr, bool &bFoundSubExpr, bool &bWriteIndex, bool bForceTemp, bool bOutputParam)
{
	vector<CHtvDesign::CTempVar> tempVarList;

#ifdef WIN32
	if (pExpr->GetLineInfo().m_lineNum == 16)// || pExpr->GetLineInfo().m_lineNum == 18)
		bool stop = true;
#endif

    //string exprBlockId = GetNextExprBlockId(pExpr);
    //if (exprBlockId == "17$109")
    //	bool stop = true;

    SetOpWidthAndSign(pExpr);

    bFoundSubExpr = FindIfSubExprRequired(pExpr, bOutputParam, false, false, false) || bForceTemp;

    if (bFoundSubExpr) {
        string exprBlockId = GetNextExprBlockId(pExpr);
#ifdef WIN32
        if (exprBlockId == "23$360b")
            bool stop = true;
#endif

        m_vFile.Print("begin : Block$%s  // %s:%d\n", exprBlockId.c_str(),
            pExpr->GetLineInfo().m_fileName.c_str(), pExpr->GetLineInfo().m_lineNum);
        m_vFile.IncIndentLevel();
    }

    if (bForceTemp && !pExpr->HasExprTempVar()) {
		string tempVarName = GetTempVarName(GetExprBlockId(), tempVarList.size());
        pExpr->SetIsSubExprRequired(true);
        pExpr->SetExprTempVar( tempVarName, pExpr->IsSigned());
        tempVarList.push_back(CTempVar(eTempVar, tempVarName, pExpr->GetOpWidth()));
    }

    if (bFoundSubExpr)
        MarkSubExprTempVars(pObj, pExpr, tempVarList, tk_eof, true, false);
	
    bWriteIndex = false;
    if (!pExpr->IsLeaf() && pExpr->GetOperator() == tk_equal) {
        CHtvOperand * pLeftOfEqualOp = pExpr->GetOperand1();
        if (pLeftOfEqualOp->IsLeaf() && pLeftOfEqualOp->IsVariable())
            bWriteIndex = pLeftOfEqualOp->HasArrayTempVar();
    }

    if (bFoundSubExpr) {
		m_vFile.SetVarDeclLines(true);
 
        for (size_t i = 0; i < tempVarList.size(); i += 1) {

            switch (tempVarList[i].m_tempType) {
            case eTempVar:
                {
                    int width = tempVarList[i].m_width;

					string bitRange;
                    if (width > 1)
                        bitRange = VA("[%d:0] ", width-1);

 					tempVarList[i].m_declWidth = width;

					m_vFile.Print("reg    %-10s %s;\n", bitRange.c_str(), tempVarList[i].m_name.c_str());

					m_vFile.PrintVarInit("%s = 32'h0;\n", tempVarList[i].m_name.c_str());
                }
                break;
            case eTempArrIdx:
                {
                    CHtvOperand *pOp = tempVarList[i].m_pOperand;
                    CHtvIdent *pIdent = pOp->GetMember();

                    HtfeOperandList_t &indexList = pOp->GetIndexList();
                    int idxBits = 0;
                    for (size_t j = 0; j < indexList.size(); j += 1) {
                        if (indexList[j]->IsConstValue())
                            continue;

						int idxWidth;
						if (!GetFieldWidthFromDimen( pIdent->GetDimenList()[j], idxWidth ))
                            ParseMsg(PARSE_FATAL, pExpr->GetLineInfo(), "invalid array index (32K limit)");

                        idxBits += idxWidth;
                    }

					tempVarList[i].m_declWidth = idxBits;

					string bitRange;
                    if (idxBits > 1)
                        bitRange = VA("[%d:0] ", idxBits-1);

                    m_vFile.Print("reg    %-10s %s;\n", bitRange.c_str(), tempVarList[i].m_name.c_str());

					m_vFile.PrintVarInit("%s = 32'h0;\n", tempVarList[i].m_name.c_str());
				}
                break;
            case eTempFldIdx:
                {
                    int width;
					if (!GetFieldWidthFromDimen( tempVarList[i].m_width, width ))
                        ParseMsg(PARSE_FATAL, pExpr->GetLineInfo(), "invalid field index (32K limit)");

					string bitRange;
					if (width > 1)
                        bitRange = VA("[%d:0] ", width-1);

 					tempVarList[i].m_declWidth = width;

					m_vFile.Print("reg    %-10s %s;\n", bitRange.c_str(), tempVarList[i].m_name.c_str());

					m_vFile.PrintVarInit("%s = 32'h0;\n", tempVarList[i].m_name.c_str());
				}
                break;
            }

			m_tempVarMap.insert(TempVarMap_ValuePair(tempVarList[i].m_name, tempVarList[i].m_declWidth) );
        }

		m_vFile.SetVarDeclLines(false);
    }

    // generate sub expressions
    GenSubExpr(pObj, pRtnObj, pExpr, false, false, true);

    return;
}

bool CHtvDesign::FindIfSubExprRequired(CHtvOperand *pExpr, bool bIsLeftOfEqual, bool bPrevTkIsEqual, bool bIsFuncParam, bool bHasObj)
{
    // recursively decend expression tree looking for function calls and ++/--
    //  mark nodes from the function/++/-- to the top of the tree with IsSubExprRequired

    Assert(pExpr != 0);
    bool bSubExprFound = false;

    pExpr->SetIsSubExprRequired(false);
    pExpr->SetIsSubExprGenerated(false);
    pExpr->SetArrayTempGenerated(false);
    pExpr->SetFieldTempGenerated(false);
    pExpr->SetIsFuncGenerated(false);
    pExpr->SetExprTempVar("", false);
    pExpr->SetArrayTempVar("");
    pExpr->SetFieldTempVar("");
    pExpr->SetFuncTempVar("", false);
	pExpr->SetTempOp(0);

    if (pExpr->IsLeaf()) {
        if (pExpr->IsConstValue()) {
            pExpr->SetVerWidth(pExpr->GetOpWidth());
            return false;
        }

        CHtvIdent *pIdent = pExpr->GetMember();

        bSubExprFound = !bIsLeftOfEqual && pIdent->IsFunction() && pIdent->GetType() != GetVoidType();
        pExpr->SetIsSubExprRequired(bSubExprFound);

        if (pIdent->IsFunction() && !bIsLeftOfEqual) {

            bool bMemberFunc = pIdent->GetPrevHier() && pIdent->GetPrevHier()->IsStruct();
			if (pIdent->GetGlobalRefSet().size() == 0 || bMemberFunc) {

				// find subExpr in function parameters
				for (size_t i = 0; i < pExpr->GetParamCnt(); i += 1) {
					if (FindIfSubExprRequired(pExpr->GetParam(i), false, false, true, false)) {
						pExpr->SetIsSubExprRequired(true);
						bSubExprFound = true;
					}
				}
			}
        }

        if (pIdent->IsVariable() || pIdent->IsFunction() && !bIsLeftOfEqual && pExpr->GetType() != GetVoidType()) {

            // find sub expression in variable indexes
            for (size_t i = 0; i < pExpr->GetIndexCnt(); i += 1) {
                int idxDimen = (int)pExpr->GetMember()->GetDimenList()[i];
                int idxWidth;
				if (!GetFieldWidthFromDimen( idxDimen, idxWidth ))
					ParseMsg(PARSE_FATAL, pExpr->GetLineInfo(), "invalid variable index (32K limit)");

                pExpr->GetIndex(i)->SetVerWidth( idxWidth );

                if (pExpr->GetIndex(i)->IsConstValue())
                    continue;

                pExpr->SetIsSubExprRequired(true);

                pExpr->GetIndex(i)->SetIsSubExprRequired(true);
                bSubExprFound = true;

                FindIfSubExprRequired(pExpr->GetIndex(i), false, false, false, false);
            }

            // check if a non-const sub-field index exists
			int lowBit;
			if (g_htvArgs.IsVivadoEnabled() && /*!pExpr->IsLinkedOp() &&*/ !bIsFuncParam && pIdent->IsVariable() && !bHasObj &&
				(!IsConstSubFieldRange(pExpr, lowBit) || lowBit > 0) && !bPrevTkIsEqual && !bIsLeftOfEqual)
			{
				pExpr->SetIsSubExprRequired(true);
                bSubExprFound = true;
			}

            for (CScSubField *pSubField = pExpr->GetSubField() ; pSubField; pSubField = pSubField->m_pNext) {
                for (size_t i = 0; i < pSubField->m_indexList.size(); i += 1) {
                    CConstValue value;
                    if (!EvalConstantExpr(pSubField->m_indexList[i], value)) {
                        bSubExprFound = true;
                        pExpr->SetIsSubExprRequired(true);

                        FindIfSubExprRequired((CHtvOperand *)pSubField->m_indexList[i], false, false, false, false);
                        break;
                    }
                }
            }

            pExpr->SetVerWidth( pExpr->GetSubFieldWidth() );
        }

        if (pIdent->IsType())
            pExpr->SetVerWidth( pIdent->GetWidth() );

    } else if (pExpr->GetOperator() == tk_period) {

        CHtvOperand *pOp1 = pExpr->GetOperand1();
        CHtvOperand *pOp2 = pExpr->GetOperand2();
        Assert(pOp2->IsLeaf() && pOp2->GetMember()->IsFunction());

		bool bSubExprFound1 = false;
		if (pOp1->GetOperator() == tk_period)
			bSubExprFound1 = FindIfSubExprRequired(pOp1, false, false, false, true);
        bool bSubExprFound2 = FindIfSubExprRequired(pOp2, false, false, false, true);

        bSubExprFound = bSubExprFound1 || bSubExprFound2;

        pExpr->SetIsSubExprRequired(bSubExprFound);

    } else {

        CHtvOperand *pOp1 = pExpr->GetOperand1();
        CHtvOperand *pOp2 = pExpr->GetOperand2();
        CHtvOperand *pOp3 = pExpr->GetOperand3();

        EToken tk = pExpr->GetOperator();

        if (tk == tk_preInc || tk == tk_preDec || tk == tk_postInc || tk == tk_postDec)
            bSubExprFound = true;

        bool bSubExprFound1 = false;
        bool bSubExprFound2 = false;
        bool bSubExprFound3 = false;

        if (pOp1) bSubExprFound1 = FindIfSubExprRequired(pOp1, tk == tk_equal, false, false, bHasObj);
        if (pOp2) bSubExprFound2 = FindIfSubExprRequired(pOp2, false, tk == tk_equal, false, bHasObj);
        if (pOp3) bSubExprFound3 = FindIfSubExprRequired(pOp3, false, false, false, bHasObj);

        bSubExprFound = bSubExprFound1 || bSubExprFound2 || bSubExprFound3 || bSubExprFound;

        int verWidth1 = 0;
        int verWidth2 = 0;

        if (pOp1) verWidth1 = min( pExpr->GetOperand1()->GetVerWidth(), pExpr->GetOperand1()->GetOpWidth() );
        if (pOp2) verWidth2 = min( pExpr->GetOperand2()->GetVerWidth(), pExpr->GetOperand2()->GetOpWidth() );

        if (tk != tk_equal && tk != tk_typeCast && pExpr->GetOpWidth() != 1 && pOp2 != 0) {
            if (pOp1 && pOp1->IsSigned() && pOp1->GetVerWidth() < pExpr->GetOpWidth() 
                && (!pOp1->IsConstValue() || pOp1->GetOpWidth() < pExpr->GetOpWidth())
                && !pOp1->IsLinkedOp() && !bIsLeftOfEqual)
                bSubExprFound = true;

            if (pOp2 && pOp2->IsSigned() && pOp2->GetVerWidth() < pExpr->GetOpWidth()
                && (!pOp2->IsConstValue() || pOp2->GetOpWidth() < pExpr->GetOpWidth())
                && !pOp2->IsLinkedOp())
                bSubExprFound = true;

            if (pOp3 && pOp3->IsSigned() && pOp3->GetVerWidth() < pExpr->GetOpWidth()
                && (!pOp3->IsConstValue() || pOp3->GetOpWidth() < pExpr->GetOpWidth()))
                bSubExprFound = true;
        }

        switch (tk) {
        case tk_preInc:
        case tk_preDec:
        case tk_postInc:
        case tk_postDec:
            pExpr->SetVerWidth( pExpr->GetOpWidth() );
            bSubExprFound = true;
            break;
        case tk_unaryPlus:
        case tk_unaryMinus:
        case tk_tilda:
            pExpr->SetVerWidth( pExpr->GetOpWidth() );

            if (pOp1->GetVerWidth() < pExpr->GetOpWidth())
                bSubExprFound = true;
            break;
        case tk_equal:
            // set to operand 1's verWidth
            pExpr->SetVerWidth(verWidth1);

            if (verWidth1 > pOp2->GetOpWidth() && !pOp2->IsConstValue())
                bSubExprFound = true;
            break;
        case tk_typeCast:
            // set to operand 1's verWidth
            pExpr->SetVerWidth(verWidth1);

            if (verWidth1 != pOp2->GetOpWidth())
                bSubExprFound = true;
            break;
        case tk_less:
        case tk_greater:
        case tk_lessEqual:
        case tk_greaterEqual:
            // set to one
            pExpr->SetVerWidth(1);
            break;
        case tk_bang:	
        case tk_vbarVbar:
        case tk_ampersandAmpersand:
        case tk_equalEqual:
        case tk_bangEqual:
            // set to one
            pExpr->SetVerWidth(1);
            break;
        case tk_plus:
        case tk_minus:
        case tk_vbar:
        case tk_ampersand:
        case tk_carot:
        case tk_asterisk:
        case tk_percent:
        case tk_slash:
            pExpr->SetVerWidth( pExpr->GetOpWidth() );

            if (pOp1->GetVerWidth() < pExpr->GetOpWidth() && (!pOp1->IsConstValue() || pOp1->GetOpWidth() < pExpr->GetOpWidth())
                && pOp2->GetVerWidth() < pExpr->GetOpWidth() && (!pOp2->IsConstValue() || pOp2->GetOpWidth() < pExpr->GetOpWidth()))
                bSubExprFound = true;
            break;
        case tk_comma:
            // sum of operands 1 & 2
            pExpr->SetVerWidth(min(pExpr->GetOpWidth(), verWidth1 + verWidth2));
            break;
        case tk_lessLess:
        case tk_greaterGreater:
            pExpr->SetVerWidth( pExpr->GetOpWidth() );

            if (pOp1->GetVerWidth() < pExpr->GetOpWidth())
                bSubExprFound = true;

            if (!pOp2->IsConstValue() && (pOp2->IsSigned() || pOp2->GetVerWidth() > 6))
                bSubExprFound = true;
            break;
        case tk_question:
            // maximum of operands 2 & 3
            pExpr->SetVerWidth( pExpr->GetOpWidth() );

            if (pOp2->GetVerWidth() < pExpr->GetOpWidth() && pOp3->GetVerWidth() < pExpr->GetOpWidth())
                bSubExprFound = true;

            if (pOp2->IsSubExprRequired() || pOp3->IsSubExprRequired())
                bSubExprFound = true;

            break;
        default:
            Assert(0);
        }

        pExpr->SetIsSubExprRequired(bSubExprFound);
    }

    return bSubExprFound;
}

void CHtvDesign::MarkSubExprTempVars(CHtvObject * &pObj, CHtvOperand *pExpr, vector<CTempVar> &tempVarList, EToken prevTk, bool bParentNotLogical, bool bIsLeftOfEqual, bool bIsFuncParam)
{
	// recursively desend expression tree looking for function calls and conditional expressions (?:)
	//  Mark expression node and all assencing nodes where found

	// generate list of temporary variables needed for expression
	//  Temp needed for:
	//  1. function with non-boolean returned value
	//  2. && or || node where parent node is not && or ||
	//  3. ?: node with subexpr function call

	Assert(pExpr != 0);

	if (pExpr->IsLeaf()) {
		if (pExpr->IsConstValue())
			return;

		CHtvIdent *pIdent = pExpr->GetMember();

		if (pIdent->IsFunction() && !bIsLeftOfEqual) {

			if (pExpr->GetType() != GetVoidType() && !pExpr->GetMember()->IsReturnRef()) {
				pExpr->SetIsSubExprRequired(true);
				pExpr->SetFuncTempVar(GetTempVarName(GetExprBlockId(), tempVarList.size()), pIdent->IsSigned());
				string tempVarName = GetTempVarName(GetExprBlockId(), tempVarList.size());
				tempVarList.push_back(CTempVar(eTempVar, tempVarName, pIdent->GetWidth()));
			}

			bool bMemberFunc = pIdent->GetPrevHier() && pIdent->GetPrevHier()->IsStruct();
			if (pIdent->GetGlobalRefSet().size() == 0 || bMemberFunc) {

				// find temps in function parameters for non-inlined function
				for (size_t i = 0; i < pExpr->GetParamCnt(); i += 1) {

					MarkSubExprTempVars(pObj, pExpr->GetParam(i), tempVarList, tk_eof, true, false, true);

					CHtvOperand * pOp = pExpr->GetParam(i);
					if (!pOp->HasExprTempVar() && (bMemberFunc ? !pOp->IsConstValue() : (!pOp->IsLeaf() || pOp->HasArrayTempVar() || pOp->HasFieldTempVar()))) {
						pOp->SetIsSubExprRequired(true);
						pOp->SetExprTempVar( GetTempVarName(GetExprBlockId(), tempVarList.size()), pExpr->IsSigned());
						string tempVarName = GetTempVarName(GetExprBlockId(), tempVarList.size());
						tempVarList.push_back(CTempVar(eTempVar, tempVarName, pOp->GetVerWidth()));
					}
				}
			}
		}

		if (pIdent->IsVariable() || pIdent->IsFunction() && !bIsLeftOfEqual) {
			// mark sub expression in variable indexes
			for (size_t i = 0; i < pExpr->GetIndexCnt(); i += 1) {
				if (pExpr->GetIndex(i)->IsConstValue())
					continue;

				// mark array variable as needing a temp
				if (!pExpr->HasExprTempVar()) {
					pExpr->SetIsSubExprRequired(true);
					pExpr->SetExprTempVar( GetTempVarName(GetExprBlockId(), tempVarList.size()), pExpr->IsSigned());
					string tempVarName = GetTempVarName(GetExprBlockId(), tempVarList.size());
					tempVarList.push_back(CTempVar(eTempVar, tempVarName, pExpr->GetVerWidth()));
				}

				// mark index variable as needing a temp
				if (!pExpr->HasArrayTempVar()) {
					pExpr->SetIsSubExprRequired(true);
					pExpr->SetArrayTempVar( GetTempVarName(GetExprBlockId(), tempVarList.size()) );
					string tempVarName = GetTempVarName(GetExprBlockId(), tempVarList.size());
					tempVarList.push_back(CTempVar(eTempArrIdx, tempVarName, -1, pExpr));
				}

				// check if index has sub expressions
				MarkSubExprTempVars(pObj, pExpr->GetIndex(i), tempVarList, pExpr->GetOperator(), true);
			}

			// mark sub expression in field indexing
			bool bIsSigned = false;
			bool bHasNonConstIndex = false;
			int tempVarWidth = 0;
			for (CScSubField *pSubField = pExpr->GetSubField() ; pSubField; pSubField = pSubField->m_pNext) {
				for (size_t i = 0; i < pSubField->m_indexList.size(); i += 1) {
					CConstValue value;
					if (!EvalConstantExpr(pSubField->m_indexList[i], value)) {
						//if (!pSubField->m_indexList[i]->IsConstValue()) {
						bHasNonConstIndex = true;

						// mark field index expression as needing a temp
						if (!pExpr->HasFieldTempVar()) {
							pExpr->SetIsSubExprRequired(true);
							pExpr->SetFieldTempVar(GetTempVarName(GetExprBlockId(), tempVarList.size()));
							string tempVarName = GetTempVarName(GetExprBlockId(), tempVarList.size());

							int width;
							if (pObj) {
								CHtvOperand * pOp = pObj->m_pOp;
								while (!pOp->IsLeaf()) {
									Assert(pOp->GetOperator() == tk_question);
									pOp = pOp->GetOperand2();
								}
								width = pOp->GetWidth();
							} else {
								CHtvIdent * pIdent = pExpr->GetMember();

								while (pIdent->GetPrevHier()->GetId() == CHtfeIdent::id_struct)
									pIdent = pIdent->GetPrevHier();

								width = pIdent->GetWidth();
							}

							tempVarList.push_back(CTempVar(eTempFldIdx, tempVarName, width));

						}

						// check if index has sub expressions
						MarkSubExprTempVars(pObj, (CHtvOperand *)pSubField->m_indexList[i], tempVarList, pExpr->GetOperator(), true);
					}
				}

				bIsSigned = pSubField->m_pIdent && pSubField->m_pIdent->GetType()->IsSigned();
				if (bIsSigned)
					tempVarWidth = pSubField->m_pIdent->GetWidth();
			}

			if (bIsSigned && bHasNonConstIndex && !bIsLeftOfEqual && !pExpr->HasExprTempVar()) {
				// Signed subfield, use temp to avoid simv bug
				pExpr->SetIsSubExprRequired(true);
				pExpr->SetExprTempVar( GetTempVarName(GetExprBlockId(), tempVarList.size()), true);
				string tempVarName = GetTempVarName(GetExprBlockId(), tempVarList.size());
				tempVarList.push_back(CTempVar(eTempVar, tempVarName, tempVarWidth, pExpr));
			}

			int lowBit;
			if (g_htvArgs.IsVivadoEnabled() && /*!pExpr->IsLinkedOp() &&*/ !bIsFuncParam && pIdent->IsVariable() && pObj == 0 &&
				(!IsConstSubFieldRange(pExpr, lowBit) || lowBit > 0) && !bIsLeftOfEqual && !pExpr->HasExprTempVar() && prevTk != tk_equal)
			{
				// use temp to avoid vivado very wide math operations
				pExpr->SetIsSubExprRequired(true);
				pExpr->SetExprTempVar( GetTempVarName(GetExprBlockId(), tempVarList.size()), bIsSigned);
				string tempVarName = GetTempVarName(GetExprBlockId(), tempVarList.size());
				tempVarList.push_back(CTempVar(eTempVar, tempVarName, pExpr->GetVerWidth(), pExpr));
			}
		}

		return;

	} else if (pExpr->GetOperator() == tk_period) {
		CHtvOperand *pOp1 = pExpr->GetOperand1();
		CHtvOperand *pOp2 = pExpr->GetOperand2();
		Assert(pOp2->IsLeaf() && pOp2->GetMember()->IsFunction());
		CHtvObject * pOrigObj = pObj;

		CHtvObject obj;

		if (prevTk != tk_period && pObj == 0) {
			obj.m_pOp = 0;
			pObj = &obj;
		}

		if (pOp1->IsLeaf() && pObj->m_pOp == 0)
			pObj->m_pOp = pOp1;

		// Functions that return a reference must have a field index temp
		if (pOp1->IsFunction() && pOp1->GetMember()->IsReturnRef()) {
			pOp1->SetFieldTempVar( GetTempVarName(GetExprBlockId(), tempVarList.size()) );
			string tempVarName = GetTempVarName(GetExprBlockId(), tempVarList.size());

			int width;
			if (pObj)
				width = pObj->m_pOp->GetWidth();
			else {
				CHtvIdent * pIdent = pOp1->GetMember();

				while (pIdent->GetPrevHier()->GetId() == CHtfeIdent::id_struct)
					pIdent = pIdent->GetPrevHier();

				width = pIdent->GetWidth();
			}

			tempVarList.push_back(CTempVar(eTempFldIdx, tempVarName, width));
		}

		MarkSubExprTempVars(pObj, pOp1, tempVarList, tk_period, true, false);

		if (pOp2->IsFunction() && pOp2->GetMember()->IsReturnRef()) {
			pOp2->SetFieldTempVar(GetTempVarName(GetExprBlockId(), tempVarList.size()));
			string tempVarName = GetTempVarName(GetExprBlockId(), tempVarList.size());

			int width;
			if (pObj)
				width = pObj->m_pOp->GetWidth();
			else {
				CHtvIdent * pIdent = pOp2->GetMember();

				while (pIdent->GetPrevHier()->GetId() == CHtfeIdent::id_struct)
					pIdent = pIdent->GetPrevHier();

				width = pIdent->GetWidth();
			}

			tempVarList.push_back(CTempVar(eTempFldIdx, tempVarName, width));
		}

		MarkSubExprTempVars(pObj, pOp2, tempVarList, tk_period, true, false);

		if (prevTk != tk_period)
			pObj = pOrigObj;

	} else {
		CHtvOperand *pOp1 = pExpr->GetOperand1();
		CHtvOperand *pOp2 = pExpr->GetOperand2();
		CHtvOperand *pOp3 = pExpr->GetOperand3();

		EToken tk = pExpr->GetOperator();

		bool bLogical = tk == tk_ampersandAmpersand || tk == tk_vbarVbar;

		if (bLogical && pExpr->IsSubExprRequired()) {
			if (bParentNotLogical) {
				if (!pExpr->HasExprTempVar()) {
					pExpr->SetExprTempVar( GetTempVarName(GetExprBlockId(), tempVarList.size()), pExpr->IsSigned());
					string tempVarName = GetTempVarName(GetExprBlockId(), tempVarList.size());
					tempVarList.push_back(CTempVar(eTempVar, tempVarName, pExpr->GetOpWidth()));
				}
			}

			pOp1->SetIsSubExprRequired(true);
			pOp1->SetExprTempVar(pExpr->GetExprTempVar(), pOp1->IsSigned());
			pOp2->SetIsSubExprRequired(true);
			pOp2->SetExprTempVar(pExpr->GetExprTempVar(), pOp2->IsSigned());
		}

		if (tk != tk_typeCast && pOp1) {
			MarkSubExprTempVars(pObj, pOp1, tempVarList, tk, !bLogical, tk == tk_equal);

			if (tk == tk_equal && pOp1->HasExprTempVar()) {
				// array indexing on left of equal, use same temp for right of equal
				Assert(!pOp2->HasExprTempVar());
				pOp2->SetIsSubExprRequired(true);
				pOp2->SetExprTempVar(pOp1->GetExprTempVar(), pOp2->IsSigned());
			}

			if (tk == tk_equal && pOp1->HasFieldTempVar() && !pOp2->HasExprTempVar()) {
				pOp2->SetIsSubExprRequired(true);
				pOp2->SetExprTempVar( GetTempVarName(GetExprBlockId(), tempVarList.size()), pOp2->IsSigned());
				string tempVarName = GetTempVarName(GetExprBlockId(), tempVarList.size());
				tempVarList.push_back(CTempVar(eTempVar, tempVarName, pOp2->GetOpWidth()));
			}
		}

		if (pOp2) MarkSubExprTempVars(pObj, pOp2, tempVarList, tk, !bLogical, false);
		if (pOp3) MarkSubExprTempVars(pObj, pOp3, tempVarList, tk, !bLogical);

		if (tk != tk_equal && tk != tk_typeCast && pExpr->GetOpWidth() != 1 && pOp2 != 0) {
			if (pOp1 && pOp1->IsSigned() && pOp1->GetVerWidth() < pExpr->GetOpWidth() 
				&& (!pOp1->IsConstValue() || pOp1->GetOpWidth() < pExpr->GetOpWidth())
				&& !pOp1->IsLinkedOp() && !bIsLeftOfEqual)
			{
				if (!pOp1->HasExprTempVar()) {
					// insert temp if signed operand is narrower than opWidth
					pOp1->SetIsSubExprRequired(true);
					pOp1->SetExprTempVar( GetTempVarName(GetExprBlockId(), tempVarList.size()), pOp1->IsSigned());
					string tempVarName = GetTempVarName(GetExprBlockId(), tempVarList.size());
					tempVarList.push_back(CTempVar(eTempVar, tempVarName, pExpr->GetOpWidth()));
				} else {
					pOp1->SetIsExprTempVarSigned(pOp1->IsSigned());

					// find existing temp in list
					for (size_t i = 0; i < tempVarList.size(); i += 1)
						if (tempVarList[i].m_name == pOp1->GetExprTempVar()) {
							tempVarList[i].m_width = pExpr->GetOpWidth();
							break;
						}
				}
			}

			if (pOp2 && pOp2->IsSigned() && pOp2->GetVerWidth() < pExpr->GetOpWidth()
				&& (!pOp2->IsConstValue() || pOp2->GetOpWidth() < pExpr->GetOpWidth())
				&& !pOp2->IsLinkedOp())
			{
				if (!pOp2->HasExprTempVar()) {
					// insert temp if signed operand is narrower than opWidth
					pOp2->SetIsSubExprRequired(true);
					pOp2->SetExprTempVar( GetTempVarName(GetExprBlockId(), tempVarList.size()), pOp2->IsSigned());
					string tempVarName = GetTempVarName(GetExprBlockId(), tempVarList.size());
					tempVarList.push_back(CTempVar(eTempVar, tempVarName, pExpr->GetOpWidth()));
				} else {
					pOp2->SetIsExprTempVarSigned(pOp2->IsSigned());

					// find existing temp in list
					for (size_t i = 0; i < tempVarList.size(); i += 1)
						if (tempVarList[i].m_name == pOp2->GetExprTempVar()) {
							tempVarList[i].m_width = pExpr->GetOpWidth();
							break;
						}
				}
			}

			if (pOp3 && pOp3->IsSigned() && pOp3->GetVerWidth() < pExpr->GetOpWidth()
				&& (!pOp3->IsConstValue() || pOp3->GetOpWidth() < pExpr->GetOpWidth()))
			{
				if (!pOp3->HasExprTempVar()) {
					// insert temp if signed operand is narrower than opWidth
					pOp3->SetIsSubExprRequired(true);
					pOp3->SetExprTempVar( GetTempVarName(GetExprBlockId(), tempVarList.size()), pOp3->IsSigned());
					string tempVarName = GetTempVarName(GetExprBlockId(), tempVarList.size());
					tempVarList.push_back(CTempVar(eTempVar, tempVarName, pExpr->GetOpWidth()));
				} else {
					pOp3->SetIsExprTempVarSigned(pOp3->IsSigned());

					// find existing temp in list
					for (size_t i = 0; i < tempVarList.size(); i += 1)
						if (tempVarList[i].m_name == pOp3->GetExprTempVar()) {
							tempVarList[i].m_width = pExpr->GetOpWidth();
							break;
						}
				}
			}
		}

		switch (tk) {
		case tk_preInc:
		case tk_preDec:
		case tk_postInc:
		case tk_postDec:
			if (!pExpr->HasExprTempVar()) {
				pExpr->SetIsSubExprRequired(true);
				pExpr->SetExprTempVar( GetTempVarName(GetExprBlockId(), tempVarList.size()), pExpr->IsSigned());
				string tempVarName = GetTempVarName(GetExprBlockId(), tempVarList.size());
				tempVarList.push_back(CTempVar(eTempVar, tempVarName, pExpr->GetOpWidth()));
			}

			pOp1->SetIsSubExprRequired(true);
			pOp1->SetExprTempVar(pExpr->GetExprTempVar(), pOp1->IsSigned());
			break;
		case tk_unaryPlus:
		case tk_unaryMinus:
		case tk_tilda:
			if (pOp1->GetVerWidth() < pExpr->GetOpWidth() && !pOp1->HasExprTempVar())
			{
				pOp1->SetIsSubExprRequired(true);
				pOp1->SetExprTempVar( GetTempVarName(GetExprBlockId(), tempVarList.size()), pOp1->IsSigned());
				string tempVarName = GetTempVarName(GetExprBlockId(), tempVarList.size());
				tempVarList.push_back(CTempVar(eTempVar, tempVarName, pExpr->GetOpWidth()));
			}
			break;
		case tk_equal:
			if (pOp1->GetVerWidth() > pOp2->GetOpWidth() && !pOp2->IsConstValue()) {
				if (!pOp2->HasExprTempVar()) {
					pOp2->SetIsSubExprRequired(true);
					pOp2->SetExprTempVar( GetTempVarName(GetExprBlockId(), tempVarList.size()), pOp2->IsSigned());
					string tempVarName = GetTempVarName(GetExprBlockId(), tempVarList.size());
					tempVarList.push_back(CTempVar(eTempVar, tempVarName, pOp2->GetOpWidth()));
				}
			}
			break;
		case tk_typeCast:
			if (pExpr->GetVerWidth() > pOp2->GetOpWidth() && pExpr->GetVerWidth() < pExpr->GetOpWidth() &&
				!pExpr->IsSigned() && pOp2->IsSigned())
			{
				// must insert cast operator to get another temp
				CHtvOperand * pNewType = new CHtvOperand;
				pNewType->SetLineInfo(pOp2->GetLineInfo());
				pNewType->SetIsLeaf();
				pNewType->SetMember(pOp1->GetType());

				CHtvOperand * pNewCast = new CHtvOperand;
				pNewCast->SetLineInfo(pOp2->GetLineInfo());
				pNewCast->SetOperator(tk_typeCast);
				pNewCast->SetOperand1(pNewType);
				pNewCast->SetOperand2(pOp2);

				pExpr->SetOperand2(pNewCast);

				// reduced range temp
				string tempVarName = GetTempVarName(GetExprBlockId(), tempVarList.size());
				if (!pOp2->HasExprTempVar()) {
					pOp2->SetIsSubExprRequired(true);
					pOp2->SetExprTempVar( tempVarName, pOp2->IsSigned());
					tempVarList.push_back(CTempVar(eTempVar, tempVarName, pOp2->GetOpWidth()));
				} else {
					pOp2->SetIsExprTempVarSigned(pOp2->IsSigned());

					// find existing temp in list
					for (size_t i = 0; i < tempVarList.size(); i += 1)
						if (tempVarList[i].m_name == pOp2->GetExprTempVar()) {
							tempVarList[i].m_width = pOp2->GetOpWidth();
							break;
						}
				}

				// expand range temp
				pNewCast->SetIsSubExprRequired(true);
				pNewCast->SetExprTempVar( GetTempVarName(GetExprBlockId(), tempVarList.size()), pExpr->IsSigned());
				pNewCast->SetExprTempVarWidth(pExpr->GetVerWidth());

				tempVarName = GetTempVarName(GetExprBlockId(), tempVarList.size());
				tempVarList.push_back(CTempVar(eTempVar, tempVarName, pExpr->GetVerWidth()));

			} else if (pExpr->GetVerWidth() > pOp2->GetOpWidth() && pExpr->IsSigned() && !pOp2->IsSigned()) {

				// casting an unsigned value to a wider signed value
				//  add unsigned temp to width of cast

				string tempVarName = GetTempVarName(GetExprBlockId(), tempVarList.size());
				int castWidth = pExpr->GetVerWidth();

				if (!pOp2->HasExprTempVar()) {
					pOp2->SetIsSubExprRequired(true);
					pOp2->SetExprTempVar( tempVarName, false);
					tempVarList.push_back(CTempVar(eTempVar, tempVarName, castWidth));
				} else {
					pOp2->SetIsExprTempVarSigned(false);

					// find existing temp in list
					for (size_t i = 0; i < tempVarList.size(); i += 1)
						if (tempVarList[i].m_name == pOp2->GetExprTempVar()) {
							tempVarList[i].m_width = castWidth;
							break;
						}
				}

			} else if (pExpr->GetVerWidth() > pOp2->GetOpWidth() && pExpr->IsSigned() == pOp2->IsSigned() &&
				pOp2->IsLeaf()) {

					// casting to a wider width

					string tempVarName = GetTempVarName(GetExprBlockId(), tempVarList.size());
					int castWidth = pExpr->GetVerWidth();

					if (!pOp2->HasExprTempVar()) {
						pOp2->SetIsSubExprRequired(true);
						pOp2->SetExprTempVar( tempVarName, pExpr->IsSigned());
						tempVarList.push_back(CTempVar(eTempVar, tempVarName, castWidth));
					} else {
						pOp2->SetIsExprTempVarSigned(pExpr->IsSigned());

						// find existing temp in list
						for (size_t i = 0; i < tempVarList.size(); i += 1)
							if (tempVarList[i].m_name == pOp2->GetExprTempVar()) {
								tempVarList[i].m_width = castWidth;
								break;
							}
					}

			} else if (pExpr->GetVerWidth() != pOp2->GetOpWidth() || pExpr->IsSigned() != pOp2->IsSigned()) {

				string tempVarName = GetTempVarName(GetExprBlockId(), tempVarList.size());
				int castWidth = min(pExpr->GetVerWidth(), pOp2->GetOpWidth());
				if (!pOp2->HasExprTempVar()) {
					pOp2->SetIsSubExprRequired(true);
					pOp2->SetExprTempVar( tempVarName, pExpr->IsSigned());
					tempVarList.push_back(CTempVar(eTempVar, tempVarName, castWidth));
				} else {
					pOp2->SetIsExprTempVarSigned(pExpr->IsSigned());

					// find existing temp in list
					for (size_t i = 0; i < tempVarList.size(); i += 1)
						if (tempVarList[i].m_name == pOp2->GetExprTempVar()) {
							tempVarList[i].m_width = castWidth;
							break;
						}
				}
			}
			break;
		case tk_less:
		case tk_greater:
		case tk_lessEqual:
		case tk_greaterEqual:
			break;
		case tk_bang:	
		case tk_vbarVbar:
		case tk_ampersandAmpersand:
		case tk_equalEqual:
		case tk_bangEqual:
			break;
		case tk_plus:
		case tk_minus:
		case tk_vbar:
		case tk_ampersand:
		case tk_carot:
		case tk_asterisk:
		case tk_percent:
		case tk_slash:
			if (pOp1->GetVerWidth() < pExpr->GetOpWidth() && (!pOp1->IsConstValue() || pOp1->GetOpWidth() < pExpr->GetOpWidth())
				&& pOp2->GetVerWidth() < pExpr->GetOpWidth() && (!pOp2->IsConstValue() || pOp2->GetOpWidth() < pExpr->GetOpWidth())
				&& !pOp1->HasExprTempVar() && !pOp2->HasExprTempVar())
			{
				pOp2->SetIsSubExprRequired(true);
				pOp2->SetExprTempVar( GetTempVarName(GetExprBlockId(), tempVarList.size()), pOp2->IsSigned());
				string tempVarName = GetTempVarName(GetExprBlockId(), tempVarList.size());
				tempVarList.push_back(CTempVar(eTempVar, tempVarName, pExpr->GetOpWidth()));
			}
			break;
		case tk_comma:
			break;
		case tk_lessLess:
		case tk_greaterGreater:
			if (pOp1->GetVerWidth() < pExpr->GetOpWidth() && !pOp1->HasExprTempVar())
			{
				pOp1->SetIsSubExprRequired(true);
				pOp1->SetExprTempVar( GetTempVarName(GetExprBlockId(), tempVarList.size()), pOp1->IsSigned());
				string tempVarName = GetTempVarName(GetExprBlockId(), tempVarList.size());
				tempVarList.push_back(CTempVar(eTempVar, tempVarName, pExpr->GetOpWidth()));
			}
			break;
		case tk_question:
			if (pOp2->GetVerWidth() < pExpr->GetOpWidth() && pOp3->GetVerWidth() < pExpr->GetOpWidth()
				&& !pOp2->HasExprTempVar() && !pOp3->HasExprTempVar())
			{
				pOp2->SetIsSubExprRequired(true);
				pOp2->SetExprTempVar( GetTempVarName(GetExprBlockId(), tempVarList.size()), pOp2->IsSigned());
				string tempVarName = GetTempVarName(GetExprBlockId(), tempVarList.size());
				tempVarList.push_back(CTempVar(eTempVar, tempVarName, pExpr->GetOpWidth()));
			}

			if (pOp2->IsSubExprRequired() || pOp3->IsSubExprRequired()) {
				// first check if op2 or op3 has temp with correct width
				string tempVar;
				if (pOp2->HasExprTempVar() && pExpr->GetOpWidth() == tempVarList[ GetTempVarIdx(pOp2->GetExprTempVar()) ].m_width)
					tempVar = pOp2->GetExprTempVar();
				else if (pOp3->HasExprTempVar() && pExpr->GetOpWidth() == tempVarList[ GetTempVarIdx(pOp3->GetExprTempVar()) ].m_width)
					tempVar = pOp3->GetExprTempVar();
				else if (pExpr->HasExprTempVar())
					tempVar = pExpr->GetExprTempVar();
				else {
					tempVar = GetTempVarName(GetExprBlockId(), tempVarList.size());
					string tempVarName = GetTempVarName(GetExprBlockId(), tempVarList.size());
					tempVarList.push_back(CTempVar(eTempVar, tempVarName, pExpr->GetOpWidth()));
				}

				// pre nodes with common temp
				if (!pExpr->HasExprTempVar()) {
					pExpr->SetIsSubExprRequired(true);
					pExpr->SetExprTempVar( tempVar, pExpr->IsSigned());
				}

				if (!pOp2->HasExprTempVar()) {
					pOp2->SetIsSubExprRequired(true);
					pOp2->SetExprTempVar(tempVar, pOp2->IsSigned());
				}                       
				if (!pOp3->HasExprTempVar()) {
					pOp3->SetIsSubExprRequired(true);
					pOp3->SetExprTempVar(tempVar, pOp3->IsSigned());
				}
			}
			break;
		default:
			Assert(0);
		}

		if (pOp1 && pOp1->IsSubExprRequired() || pOp2 && pOp2->IsSubExprRequired() || pOp3 && pOp3->IsSubExprRequired())
			pExpr->SetIsSubExprRequired(true);
	}
}

bool CHtvDesign::SynXilinxRom(CHtvIdent *pHier, CHtvStatement *pSwitch)
{
    // First check if switch width is eight and all case statements are constants
    CHtvOperand *pSwitchOperand = pSwitch->GetExpr();

    if (!pHier->IsMethod())
        return false;   // functions and tasks can not instanciate primitives

    if (pSwitchOperand->GetVerWidth() != 8)
        return false; // Only 256 entry Roms

    CHtvIdent *pInputIdent = pSwitchOperand->GetMember();

    // verify right side of statements are constant
    CHtvIdent *pOutputIdent = 0;
    int romWidth = 0;
    uint64 romTable[256];
    bool    romTableAssigned[256];
    for (int i = 0; i < 256; i += 1) romTableAssigned[i] = false;
    bool bDefaultFound = false;
    uint64 defaultValue = 0;
    CHtvStatement *pStatement = pSwitch->GetCompound1();
    for (; pStatement ; pStatement = pStatement->GetNext()) {
        int64_t romTableIdx;
        if (pStatement->GetStType() == st_default) {
            romTableIdx = 0;
            bDefaultFound = true;
        } else {
            CHtvOperand *pCaseOperand = pStatement->GetExpr();
            if (!pCaseOperand->IsConstValue())
                return false;
            romTableIdx = pCaseOperand->GetConstValue().GetUint64();
            if (romTableIdx >= 256)
                return false;
        }

        CHtvStatement *pCaseStatement = pStatement->GetCompound1();

        if (pCaseStatement == 0)
            return false;

        if (pCaseStatement->GetStType() != st_assign)
            return false;

        CHtvOperand *pAssignExpr = pCaseStatement->GetExpr();
        if (pAssignExpr->IsLeaf() || pAssignExpr->GetOperator() != tk_equal)
            return false;

        CHtvOperand *pAssignLeftExpr = pAssignExpr->GetOperand1();

        if (pOutputIdent == 0) {
            pOutputIdent = pAssignLeftExpr->GetMember();
            romWidth = pOutputIdent->GetWidth();
            if (romWidth > 64)
                return false;
        } else {
            if (pOutputIdent != pAssignLeftExpr->GetMember())
                return false;
        }

        CHtvOperand *pAssignRightExpr = pAssignExpr->GetOperand2();

        if (!pAssignRightExpr->IsConstValue())
            return false;

        if (pStatement->GetStType() == st_default) {
            defaultValue = pAssignRightExpr->GetConstValue().GetUint64();
        } else {
            if (romTableAssigned[romTableIdx])
                return false;

            romTableAssigned[romTableIdx] = true;
            romTable[romTableIdx] = pAssignRightExpr->GetConstValue().GetUint64();
        }
    }

    if (bDefaultFound) {
        for (int idx = 0; idx < 256; idx += 1)
            if (!romTableAssigned[idx])
                romTable[idx] = defaultValue;
    } else {
        for (int idx = 0; idx < 256; idx += 1)
            if (!romTableAssigned[idx])
                return false;
    }

    // Valid 256 entry rom table found, generate Xilinx primitives

    for (int lutIdx = 0; lutIdx < 4; lutIdx += 1)
        m_vFile.Print("wire [%d:0] %s_L%d;\n", romWidth-1, pOutputIdent->GetName().c_str(), lutIdx);
    for (int muxIdx = 0; muxIdx < 2; muxIdx += 1)
        m_vFile.Print("reg  [%d:0] %s_M%d;\n", romWidth-1, pOutputIdent->GetName().c_str(), muxIdx);

    for (int bit = 0; bit < romWidth; bit += 1) {
        int romData = 0;
        for (int romIdx = 255; romIdx >= 0; romIdx -= 1) {
            romData <<= 1;
            romData |= (romTable[romIdx] >> bit) & 1;

            if ((romIdx & 0x3f) == 0x3f) {
                if (romIdx != 255)
                    m_vFile.Print(";\n");
                m_vFile.Print("defparam %s_L%d$%d.INIT = 64'h", pOutputIdent->GetName().c_str(), romIdx >> 6, bit);
            }

            if ((romIdx & 3) == 0) {
                m_vFile.Print("%x", romData);
                romData = 0;
            }
        }
        m_vFile.Print(";\n");

        for (int lutIdx = 3; lutIdx >= 0; lutIdx -= 1) {
            m_vFile.Print("LUT6 %s_L%d$%d ( .O(%s_L%d[%d])", pOutputIdent->GetName().c_str(), lutIdx, bit,
                pOutputIdent->GetName().c_str(), lutIdx, bit);
            for (int addrBit = 5; addrBit >= 0; addrBit -= 1)
                m_vFile.Print(", .I%d(%s[%d])", addrBit, pInputIdent->GetName().c_str(), addrBit);
            m_vFile.Print(" );\n");
        }

        m_vFile.Print("MUXF7 %s_M0$%d ( .O(%s_M0[%d]), .I0(%s_L0[%d]), .I1(%s_L1[%d]), .S(%s[6]) );\n",
            pOutputIdent->GetName().c_str(), bit, pOutputIdent->GetName().c_str(), bit,
            pOutputIdent->GetName().c_str(), bit, pOutputIdent->GetName().c_str(), bit,
            pInputIdent->GetName().c_str() );

        m_vFile.Print("MUXF7 %s_M1$%d ( .O(%s_M1[%d]), .I0(%s_L2[%d]), .I1(%s_L3[%d]), .S(%s[6]) );\n",
            pOutputIdent->GetName().c_str(), bit, pOutputIdent->GetName().c_str(), bit,
            pOutputIdent->GetName().c_str(), bit, pOutputIdent->GetName().c_str(), bit,
            pInputIdent->GetName().c_str() );

        m_vFile.Print("MUXF8 %s_M2$%d ( .O(%s[%d]), .I0(%s_M0[%d]), .I1(%s_M1[%d]), .S(%s[7]) );\n",
            pOutputIdent->GetName().c_str(), bit, pOutputIdent->GetName().c_str(), bit,
            pOutputIdent->GetName().c_str(), bit, pOutputIdent->GetName().c_str(), bit,
            pInputIdent->GetName().c_str() );

    }

    return true;
}

void CHtvDesign::RemoveMemoryWriteStatements(CHtvStatement **ppIfList)
{
    while (*ppIfList) {
        EStType stType = (*ppIfList)->GetStType();
        if (stType == st_null) {
            // delete null statements
            *ppIfList = (*ppIfList)->GetNext();
            continue;
        }

        CHtvOperand *pExpr = (*ppIfList)->GetExpr();
        if (stType != st_assign || pExpr == 0 || pExpr->GetOperator() != tk_equal) {
            ppIfList = (*ppIfList)->GetPNext();
            continue;
        }

        pExpr = pExpr->GetOperand1();
        ppIfList = (*ppIfList)->GetPNext();
    }
}

/////////////////////////////////////////////////////////////////////
//  Generation routines

#if !defined(VERSION)
#define VERSION "unknown"
#endif
#if !defined(BLDDTE)
#define BLDDTE "unknown"
#endif
#if !defined(VCSREV)
#define VCSREV "unknown"
#endif

void CHtvDesign::GenVFileHeader()
{
    m_vFile.Print("/*****************************************************************************/\n");
    m_vFile.Print("// Generated with htv %s-%s (%s)\n", VERSION, VCSREV, BLDDTE);
    m_vFile.Print("//\n");
    m_vFile.Print("/*****************************************************************************/\n");
    m_vFile.Print("`timescale 1ns / 1ps\n\n");
}

void CHtvDesign::GenHtAttributes(CHtvIdent * pIdent, string instName)
{
    if (pIdent->GetFlatIdent()) {
        GenHtAttributes(pIdent->GetFlatIdent(), instName);
        return;
    }
    for (int i = 0; i < pIdent->GetScAttribCnt(); i += 1) {
        CHtAttrib htAttrib = pIdent->GetScAttrib(i);

		bool skip = false;
		string name = htAttrib.m_name;
		string value = htAttrib.m_value;

		// Quartus hacks FIXME
		if (g_htvArgs.IsQuartusEnabled()) {
			if (name == "keep_hierarchy") 
				skip = true;
			if (name == "equivalent_register_removal") {
				name = "syn_preserve";
				value = "true";
			}
			if (name == "keep")
				name = "noprune";
		}

        if ((instName.size() == 0 || instName == htAttrib.m_inst) && !skip)
			m_vFile.Print("(* %s = \"%s\" *)\n", name.c_str(), value.c_str());
    }
}

void CHtvDesign::GenModuleHeader(CHtvIdent *pModule)
{
    CHtvIdentTblIter moduleIter(pModule->GetFlatIdentTbl());

    m_vFile.SetIndentLevel(0);

    if (g_htvArgs.IsLedaEnabled())
        m_vFile.Print("`include \"leda_header.vh\"\n\n");

    m_vFile.Print("`define HTDLY\n\n");

    if (g_htvArgs.IsKeepHierarchyEnabled())
        m_vFile.Print("(* keep_hierarchy = \"true\" *)\n");

    GenHtAttributes(pModule);

    m_vFile.Print("module %s (", pModule->GetName().c_str());
    m_vFile.IncIndentLevel();

	m_vFile.SetVarDeclLines(true);

    // declare outputs
    bool bFirst = true;
    char *pFirstStr = "\n// Outputs\n";
    for (moduleIter.Begin(); !moduleIter.End(); moduleIter++) {
        if (moduleIter->GetPortDir() == port_out) {
			string bitRange;
            if (moduleIter->GetWidth() > 1)
                bitRange = VA("[%d:0]", moduleIter->GetWidth()-1);

            // iterate through array elements
			for (int elemIdx = 0; elemIdx < moduleIter->GetDimenElemCnt(); elemIdx += 1) {

				bool bWire = moduleIter->IsAssignOutput(elemIdx);

				string identName = moduleIter->GetVerilogName(elemIdx);

                m_vFile.Print("%s", pFirstStr);

                GenHtAttributes(moduleIter(), identName);

                m_vFile.Print("output %s %-10s %s",
                    bWire ? "   " : "reg", bitRange.c_str(), identName.c_str());

                bFirst = false;
                pFirstStr = ",\n";
            }
        }
    }

    // declare inputs
    if (bFirst)
        pFirstStr = "\n\n// Inputs\n";
    else
        pFirstStr = ",\n\n// Inputs\n";
    bFirst = true;
    for (moduleIter.Begin(); !moduleIter.End(); moduleIter++) {
        if (moduleIter->GetPortDir() == port_in) {
			string bitRange;
            if (moduleIter->GetWidth() > 1)
                bitRange = VA("[%d:0]", moduleIter->GetWidth()-1);

            // iterate through array elements
            vector<int> refList(moduleIter->GetDimenCnt(), 0);

            do {
                string identName = GenVerilogIdentName(moduleIter->GetName(), refList);

                m_vFile.Print("%s", pFirstStr);

                GenHtAttributes(moduleIter(), identName);

                m_vFile.Print("input      %-10s %s", bitRange.c_str(), identName.c_str());

                bFirst = false;
                pFirstStr = ",\n";

            } while (!moduleIter->DimenIter(refList));
        }
    }
    m_vFile.Print("\n);\n");

    // builtin constants
    bool bBuiltinComment = true;
    CHtvIdentTblIter topIter(GetTopHier()->GetFlatIdentTbl());
    for (topIter.Begin(); !topIter.End(); topIter++) {
        if (topIter->GetId() != CHtvIdent::id_const)
            continue;

        if (bBuiltinComment) {
            m_vFile.Print("\n// builtin constants\n");
            bBuiltinComment = false;
        }

        if (topIter->IsReadRef(0))
            m_vFile.Print("wire %s = %d'h%x;\n", topIter->GetName().c_str(),
            topIter->GetWidth(), topIter->GetConstValue().GetSint64());
    }

    // global enum constants
    CHtvIdentTblIter globalIdentIter(pModule->GetPrevHier()->GetIdentTbl());
    for (globalIdentIter.Begin(); !globalIdentIter.End(); globalIdentIter++) {
        if (!globalIdentIter->IsIdEnumType())
            continue;

        m_vFile.Print("\n// enum %s\n", globalIdentIter->GetName().c_str());

		string bitRange;
        if (globalIdentIter->GetWidth() > 1)
            bitRange = VA("[%d:0]", globalIdentIter->GetWidth()-1);

        CHtvIdentTblIter enumIter(globalIdentIter->GetFlatIdentTbl());
        for (enumIter.Begin(); !enumIter.End(); enumIter++) {
            if (enumIter->GetId() != CHtvIdent::id_enum || globalIdentIter->GetThis() != enumIter->GetType())
                continue;

            //if (enumIter->IsReadRef(0))
            m_vFile.Print("wire   %-10s %s = %d'h%x;\n", bitRange.c_str(), enumIter->GetName().c_str(),
                globalIdentIter->GetWidth(), enumIter->GetConstValue().GetSint64());
        }
    }

    // enum constants
    CHtvIdentTblIter moduleIdentIter(pModule->GetIdentTbl());
    for (moduleIdentIter.Begin(); !moduleIdentIter.End(); moduleIdentIter++) {
        if (!moduleIdentIter->IsIdEnumType())
            continue;

        m_vFile.Print("\n// enum %s\n", moduleIdentIter->GetName().c_str());

        string bitRange;
        if (moduleIdentIter->GetWidth() > 1)
            bitRange = VA("[%d:0]", moduleIdentIter->GetWidth()-1);

        CHtvIdentTblIter enumIter(moduleIdentIter->GetFlatIdentTbl());
        for (enumIter.Begin(); !enumIter.End(); enumIter++) {
            if (enumIter->GetId() != CHtvIdent::id_enum || moduleIdentIter->GetThis() != enumIter->GetType())
                continue;

            //if (enumIter->IsReadRef(0))
            m_vFile.Print("wire   %-10s %s = %d'h%x;\n", bitRange.c_str(), enumIter->GetName().c_str(),
                moduleIdentIter->GetWidth(), enumIter->GetConstValue().GetSint64());
        }
    }

    if (g_htvArgs.IsSynXilinxPrims())
        return;

    // declare variables
    pFirstStr = "\n// variables\n";
    for (moduleIter.Begin(); !moduleIter.End(); moduleIter++) {

        // skip references
        if (moduleIter->IsVariable() && !moduleIter->IsPort() && !moduleIter->IsScState() &&
            !moduleIter->IsReference() && !moduleIter->IsHtQueue() && !moduleIter->IsHtMemory()) {

            if (moduleIter->GetHierIdent()->IsRemoveIfWriteOnly() && !moduleIter->GetHierIdent()->IsReadRef())
                continue;

			string bitRange;
            vector<CHtDistRamWeWidth> *pWeWidth = moduleIter->GetHtDistRamWeWidth();
            if (pWeWidth == 0) {
                if (moduleIter->GetWidth() > 1)
                    bitRange = VA("[%d:0]", moduleIter->GetWidth()-1);

                // iterate through array elements
				for (int elemIdx = 0; elemIdx < moduleIter->GetDimenElemCnt(); elemIdx += 1) {

					bool bWire = moduleIter->IsAssignOutput(elemIdx) || moduleIter->IsHtPrimOutput(elemIdx) || moduleIter->IsPrimOutput();

					string identName = moduleIter->GetVerilogName(elemIdx);

                    GenHtAttributes(moduleIter(), identName);

                    m_vFile.Print("%s%s   %-10s %s", 
                        pFirstStr, bWire ? "wire" : "reg ", bitRange.c_str(), identName.c_str());

                    m_vFile.Print(";\n");
                    pFirstStr = "";
                }

            } else if (pWeWidth->size() == 1) {

                // iterate through array elements
				for (int elemIdx = 0; elemIdx < moduleIter->GetDimenElemCnt(); elemIdx += 1) {

					bool bWire = moduleIter->IsAssignOutput(elemIdx) || moduleIter->IsHtPrimOutput(elemIdx) || moduleIter->IsPrimOutput();

					string identName = moduleIter->GetVerilogName(elemIdx);
 
                    GenHtAttributes(moduleIter(), identName);

                    m_vFile.Print("%s%s   %-10s %s", 
                        pFirstStr, bWire ? "wire" : "reg ", "", identName.c_str());

                    m_vFile.Print(";\n");
                    pFirstStr = "";
                }

            } else for (size_t j = 0; j < pWeWidth->size(); j += 1) {
                int highBit = (*pWeWidth)[j].m_highBit;
                int lowBit = (*pWeWidth)[j].m_lowBit;

				// iterate through array elements
				for (int elemIdx = 0; elemIdx < moduleIter->GetDimenElemCnt(); elemIdx += 1) {

					bool bWire = moduleIter->IsAssignOutput(elemIdx) || moduleIter->IsHtPrimOutput(elemIdx) || moduleIter->IsPrimOutput();

					string dimStr = moduleIter->GetVerilogName(elemIdx, false);

					string name = VA("%s_%d_%d%s", moduleIter->GetName().c_str(), highBit, lowBit, dimStr.c_str());

					GenHtAttributes(moduleIter(), name.c_str());

					m_vFile.Print("%s%s   %-10s %s;\n", pFirstStr, bWire ? "wire" : "reg ", "", name.c_str());
					pFirstStr = "";
				}
            }
        }
    }
	m_vFile.SetVarDeclLines(false);

    // write instantiated primitives
    for (moduleIter.Begin(); !moduleIter.End(); moduleIter++)
    {
        if (moduleIter->GetId() != CHtvIdent::id_variable || !moduleIter->IsReference())
            continue;

        int elemIdx = moduleIter->GetElemIdx();
        CHtvIdent *pBaseIdent;
        if (moduleIter->GetPrevHier()->IsArrayIdent())
            pBaseIdent = moduleIter->GetPrevHier();
        else
            pBaseIdent = moduleIter();

        Assert(moduleIter->GetInstanceName() == pBaseIdent->GetInstanceName(elemIdx));

        m_vFile.Print("\n  %s ", moduleIter->GetType()->GetName().c_str());

        m_vFile.Print("%s(\n", moduleIter->GetInstanceName().c_str());
        pFirstStr = "";

        CHtvIdent *pInstance = moduleIter();

        CHtvIdentTblIter portIter(pInstance->GetIdentTbl());
        for (portIter.Begin(); !portIter.End(); portIter++)
        {
            m_vFile.Print("%s    .%s(%s)", pFirstStr,
                portIter->GetName().c_str(), portIter->GetPortSignal()->GetName().c_str());
            pFirstStr = ",\n";
        }

        m_vFile.Print("\n  );\n");
    }
}

void CHtvDesign::GenModuleTrailer()
{
    m_vFile.DecIndentLevel();
    m_vFile.Print("endmodule\n");
}

void CHtvDesign::GenFunction(CHtvIdent *pFunction, CHtvObject * pObj, CHtvObject * pRtnObj)
{
    if (!pFunction->IsBodyDefined())
        return;

    if (pFunction->IsInlinedCall())
        return;

    m_vFile.Print("\ntask %s;\n", pFunction->GetName().c_str());
    m_vFile.IncIndentLevel();

    if (pFunction->GetType() != GetVoidType()) {
        // function return
		string bitRange;
        if (pFunction->GetWidth() > 1)
            bitRange = VA("[%d:0] ", pFunction->GetWidth()-1);

        m_vFile.Print("output %s%s$Rtn;\n", bitRange.c_str(), pFunction->GetName().c_str());
    }

    // list parameters
    for (int paramId = 0; paramId < pFunction->GetParamCnt(); paramId += 1) {
        CHtvIdent *pParamType = pFunction->GetParamType(paramId);

		string bitRange;
        if (pParamType->GetWidth() > 1)
            bitRange = VA("[%d:0] ", pParamType->GetWidth()-1);

        string identName = pFunction->GetParamName(paramId);
        CHtvIdent *pParamIdent = pFunction->FindIdent(identName);

        if (pParamIdent->IsReadOnly()) {
            GenHtAttributes(pParamIdent, identName);
            m_vFile.Print("input %s%s;\n", bitRange.c_str(), identName.c_str());
        } else {
            // list as output
            GenHtAttributes(pParamIdent, identName);
            m_vFile.Print("input %s%s$In;\n", bitRange.c_str(), identName.c_str());

            GenHtAttributes(pParamIdent, identName);
            m_vFile.Print("output %s%s$Out;\n", bitRange.c_str(), identName.c_str());
        }
    }

    // list global variables
    for (size_t refIdx = 0; refIdx < pFunction->GetGlobalRefSet().size(); refIdx += 1) {
        CHtIdentElem &identElem = pFunction->GetGlobalRefSet()[refIdx];

		string bitRange;
        if (identElem.m_pIdent->GetWidth() > 1)
            bitRange = VA("[%d:0] ", identElem.m_pIdent->GetWidth()-1);

        vector<CHtDistRamWeWidth> *pWeWidth = identElem.m_pIdent->GetHtDistRamWeWidth();

        int startIdx = pWeWidth == 0 ? -1 : 0;
        int lastIdx = pWeWidth == 0 ? 0 : (int)(*pWeWidth).size();

        for (int rangeIdx = startIdx; rangeIdx < lastIdx; rangeIdx += 1) {

            string identName = GenVerilogIdentName(identElem, rangeIdx);

            GenHtAttributes((CHtvIdent *)identElem.m_pIdent, identName);

            if (identElem.m_pIdent->IsHtPrimOutput(identElem.m_elemIdx))
                m_vFile.Print("input %s%s;\n", bitRange.c_str(), identName.c_str());
            else
                m_vFile.Print("input %s%s_global;\n", bitRange.c_str(), identName.c_str());
        }
    }

    // list input/output parameters as local register variable
    for (int paramId = 0; paramId < pFunction->GetParamCnt(); paramId += 1) {
        CHtvIdent *pParamType = pFunction->GetParamType(paramId);

		string bitRange;
        if (pParamType->GetWidth() > 1)
            bitRange = VA("[%d:0] ", pParamType->GetWidth()-1);

        string identName = pFunction->GetParamName(paramId);
        CHtvIdent *pParamIdent = pFunction->FindIdent(identName);

        GenHtAttributes(pParamIdent, identName);

        if (!pParamIdent->IsReadOnly())
            m_vFile.Print("reg %s%s;\n", bitRange.c_str(), identName.c_str());
    }

    m_vFile.Print("begin : %s$\n", pFunction->GetName().c_str());
    m_vFile.IncIndentLevel();

	m_vFile.VarInitOpen();
	m_bInAlwaysAtBlock = true;

    // gen input parameter assignments
    for (int paramId = 0; paramId < pFunction->GetParamCnt(); paramId += 1) {

        CHtvIdent *pParamIdent = pFunction->FindIdent(pFunction->GetParamName(paramId));

        if (!pParamIdent->IsReadOnly()) {
            // list as input
            m_vFile.Print("%s = %s$In;\n", pFunction->GetParamName(paramId).c_str(), pFunction->GetParamName(paramId).c_str());
        }
    }

    SynCompoundStatement(pFunction, pObj, pRtnObj, true, true);

    // gen output parameter assignments
    for (int paramId = 0; paramId < pFunction->GetParamCnt(); paramId += 1) {

        CHtvIdent *pParamIdent = pFunction->FindIdent(pFunction->GetParamName(paramId));

        if (!pParamIdent->IsReadOnly()) {
            // list as output
            m_vFile.Print("%s$Out = %s;\n", pFunction->GetParamName(paramId).c_str(), pFunction->GetParamName(paramId).c_str());
        }
    }

	m_vFile.VarInitClose();
	m_bInAlwaysAtBlock = false;

	m_vFile.DecIndentLevel();
    m_vFile.Print("end\n");

    m_vFile.DecIndentLevel();
    m_vFile.Print("endtask\n");
}

void CHtvDesign::GenMethod(CHtvIdent *pMethod, CHtvObject * pObj)
{
    SynIndividualStatements(pMethod, pObj, 0);
}

void CHtvDesign::GenAlwaysAtHeader(bool bBeginEnd)
{
    // Always @ block
    m_vFile.Print("\nalways @(*)\n");
    m_vFile.IncIndentLevel();

    if (bBeginEnd) {
		m_vFile.Print("begin : Always$%d\n", GetNextAlwaysBlockIdx());
        m_vFile.IncIndentLevel(); // begin
		m_vFile.VarInitOpen();
		m_bInAlwaysAtBlock = true;
    }
}

void CHtvDesign::GenAlwaysAtHeader(CHtvIdent *pIdent)
{
    // Always @ block
    m_vFile.Print("\nalways @(posedge %s)\n", pIdent->GetName().c_str());
    m_vFile.IncIndentLevel();

	m_vFile.Print("begin : Always$%d\n", GetNextAlwaysBlockIdx());
    m_vFile.IncIndentLevel(); // begin
	m_vFile.VarInitOpen();
	m_bInAlwaysAtBlock = true;
}

void CHtvDesign::GenAlwaysAtTrailer(bool bBeginEnd)
{
    if (bBeginEnd) {
		m_vFile.DecIndentLevel(); // begin
        m_vFile.Print("end\n");
		m_vFile.VarInitClose();
		m_bInAlwaysAtBlock = false;
    }

    m_vFile.DecIndentLevel(); // always
}

void CHtvDesign::GenArrayIndexingSubExpr(CHtvObject * pObj, CHtvObject * pRtnObj, CHtvOperand * pOperand, bool bLeftOfEqual)
{
    CHtvIdent * pIdent = pOperand->GetMember();

    pOperand->SetIsSubExprGenerated(true);

    // determine number of bits for single case index
    Assert(pOperand->GetIndexCnt() > 0);
    Assert(pOperand->HasArrayTempVar());

    vector<int> fieldWidthList(pOperand->GetIndexCnt(), 0);
    vector<bool> fieldConstList(pOperand->GetIndexCnt(), false);
    vector<int> refList(pOperand->GetIndexCnt(), 0);

    int idxBits = 0;
    for (size_t j = 0; j < pOperand->GetIndexCnt(); j += 1) {
        fieldConstList[j] = pOperand->GetIndex(j)->IsConstValue();

        if (pOperand->GetIndex(j)->IsConstValue()) {
            refList[j] = pOperand->GetIndex(j)->GetConstValue().GetSint32();
            continue;
        }

		int idxWidth;
		if (!GetFieldWidthFromDimen(pIdent->GetDimenList()[j], idxWidth))
			ParseMsg(PARSE_FATAL, pOperand->GetLineInfo(), "invalid array index (32K limit)");

        idxBits += fieldWidthList[j] = idxWidth;
    }

    int bitPos = idxBits-1;
    for (size_t j = 0; j < pOperand->GetIndexCnt(); j += 1) {
        if (pOperand->GetIndex(j)->IsConstValue())
            continue;

        // gen sub expressions
        GenSubExpr(pObj, pRtnObj, pOperand->GetIndex(j), false, false, true);

        m_vFile.Print("%s", pOperand->GetArrayTempVar().c_str());

        if (idxBits > 1) {
            if (bitPos == bitPos - fieldWidthList[j]+1)
                m_vFile.Print("[%d]", bitPos);
            else
                m_vFile.Print("[%d:%d]", bitPos, bitPos - fieldWidthList[j]+1);
        }
        m_vFile.Print(" = ");

        GenSubExpr(pObj, pRtnObj, pOperand->GetIndex(j));

        m_vFile.Print(";\n");

        bitPos -= fieldWidthList[j];
    }

    m_vFile.Print("case (%s)\n", pOperand->GetArrayTempVar().c_str());
    m_vFile.IncIndentLevel();

    int caseCnt = 0;
    do {
        int j = 0;
        for (size_t i = 0; i < pOperand->GetIndexCnt(); i += 1) {
            j <<= fieldWidthList[i];
            if (fieldConstList[i])
                continue;
            j |= refList[i];
        }

        m_vFile.Print("%d'h%x: ", idxBits, j);

        if (!bLeftOfEqual)
            m_vFile.Print("%s = ", pOperand->GetExprTempVar().c_str());

        m_vFile.Print("%s", GenVerilogIdentName(pIdent->GetName(), refList).c_str() );
        GenSubExpr(pObj, pRtnObj, pOperand, false, true);

        if (bLeftOfEqual)
            m_vFile.Print(" = %s", pOperand->GetExprTempVar().c_str());

        m_vFile.Print(";\n");
        caseCnt += 1;
    } while (!pIdent->DimenIter(refList, fieldConstList));

    if (caseCnt < (1 << idxBits) && !bLeftOfEqual)
        m_vFile.Print("default: %s = %d'h0;\n", pOperand->GetExprTempVar().c_str(), pIdent->GetWidth());

    m_vFile.DecIndentLevel();
    m_vFile.Print("endcase\n");
}

void CHtvDesign::GenSubExpr(CHtvObject * pObj, CHtvObject * pRtnObj, CHtvOperand *pExpr, bool bIsLeftOfEqual, bool bArrayIndexing, bool bGenSubExpr, EToken prevTk)
{
    Assert(pExpr);

#ifdef WIN32
	if (pExpr->GetLineInfo().m_lineNum == 337)
		bool stop = true;
#endif

    if (bGenSubExpr) {
        // walk expression tree down to leaves
        //   on the way back up, generate expressions for each temp variable

        if (!pExpr->IsSubExprRequired())
            return;

        if (!pExpr->IsLeaf() && !pExpr->IsSubExprGenerated()) {
            // decend
            EToken tk = pExpr->GetOperator();

            CHtvOperand *pOp1 = pExpr->GetOperand1();
            CHtvOperand *pOp2 = pExpr->GetOperand2();
            CHtvOperand *pOp3 = pExpr->GetOperand3();

            if ((tk == tk_ampersandAmpersand || tk == tk_vbarVbar) 
                && (pOp1->IsSubExprRequired() || pOp2->IsSubExprRequired()))
            {
                PrintLogicalSubExpr(pObj, pRtnObj, pExpr);
                return;
            }

            if (tk == tk_question && (pOp2->IsSubExprRequired() || pOp3->IsSubExprRequired())) {
                PrintQuestionSubExpr(pObj, pRtnObj, pExpr);
                return;
            }

            if (tk == tk_preInc || tk == tk_postInc || tk == tk_preDec || tk == tk_postDec) {
                // increment/decrement
                PrintIncDecSubExpr(pObj, pRtnObj, pExpr);
                return;
            }

            if (tk == tk_period) {

				CHtvObject subObj;

				if (pOp1->GetOperator() == tk_period) {
					GenSubExpr(pObj, &subObj, pOp1, false, false, true, tk);
					subObj.m_pOp = pOp1->GetTempOp();
				} else {
					GenSubObject(&subObj, pObj, pOp1);
				}

				CHtvObject rtnObj2;
				CHtvObject * pRtnObj2 = pOp2->IsFunction() && pOp2->GetType() == GetVoidType() ? 0 : &rtnObj2;

				GenSubExpr(&subObj, pRtnObj2, pOp2, false, false, true, tk);

				if (pExpr->HasExprTempVar() &&
					(pExpr->GetExprTempVar() != pExpr->GetOperand2()->GetExprTempVar() ||
					pExpr->GetOperand2()->HasFuncTempVar() ))
				{
					m_vFile.Print("%s = ", pExpr->GetExprTempVar().c_str());
					PrintSubExpr(pObj, pRtnObj, pExpr->GetOperand2());
					m_vFile.Print(";\n");
				}

				if (pRtnObj2 && pRtnObj2->m_pOp) {
					pExpr->SetTempOp(pRtnObj2->m_pOp);

					if (prevTk != tk_period && !pRtnObj2->m_pOp->IsTempOpSubGened()) {
						bool bFoundSubExpr = false;
						bool bWriteIndex = false;
						FindSubExpr(0, 0, pRtnObj2->m_pOp, bFoundSubExpr, bWriteIndex);

						pRtnObj2->m_pOp->SetTempOpSubGened(true);

						if (bFoundSubExpr) {
							m_vFile.DecIndentLevel();
							m_vFile.Print("end\n");
						}
					}
				}

			    pExpr->SetIsSubExprGenerated(true);
                return;
            }

            if (pOp1 && tk != tk_equal) GenSubExpr(pObj, pRtnObj, pOp1, false, false, true, tk);
			if (pOp2) GenSubExpr(pObj, pRtnObj, pOp2, false, false, true, tk);
            if (pOp3) GenSubExpr(pObj, pRtnObj, pOp3, false, false, true, tk);

            if (pOp1 && tk == tk_equal) {
                if (!pOp1->HasArrayTempVar())
                    GenSubExpr(pObj, pRtnObj, pOp1, true, false, true, tk);
                else
                    PrintArrayIndex(pObj, 0, pExpr, true);
            }

        } else {
            // at a leaf node, check if field indexing is needed
            if (pExpr->IsFunction()) {

                for (size_t i = 0; i < pExpr->GetParamCnt(); i += 1)
                    // gen sub expressions
                    GenSubExpr(0, 0, pExpr->GetParam(i), false, false, true);

                // generate function call
				if (pExpr->HasFuncTempVar() || pExpr->HasFieldTempVar()) {
					PrintSubExpr(pObj, pRtnObj, pExpr, bIsLeftOfEqual, true);

					if (pExpr->GetSubField()) {

						// Append subfield to result
						if (pRtnObj && pRtnObj->m_pOp)
							pRtnObj->m_pOp = CreateObject(pRtnObj->m_pOp, pExpr);
						else if (pExpr->GetTempOp())
							pExpr->SetTempOp(CreateObject(pExpr->GetTempOp(), pExpr));
					}
				}
            }
            
            if (pExpr->IsVariable() || pExpr->IsFunction() && !bIsLeftOfEqual && !pExpr->GetMember()->IsReturnRef()) {
                if (pExpr->HasFieldTempVar()) {
                    if (!pExpr->IsFieldTempGenerated()) {
                        // non-constant field indexing, check for subexpressions
                        CScSubField * pSubField = pExpr->GetSubField();
                        for ( ; pSubField; pSubField = pSubField->m_pNext) {
                            for (size_t i = 0; i < pSubField->m_indexList.size(); i += 1) {
                                if (!pSubField->m_indexList[i]->IsConstValue())
                                    GenSubExpr(pObj, 0, (CHtvOperand *)pSubField->m_indexList[i], false, false, true);
                            }
                        }

                        PrintFieldIndex(pObj, pRtnObj, pExpr); // generate shift amount expression
                        pExpr->SetFieldTempGenerated(true);
                    }
                }

                if (pExpr->HasArrayTempVar()) {
                    if (!pExpr->IsArrayTempGenerated() && bIsLeftOfEqual) {
                        // non-constant array indexing, check for subexpressions
                        for (size_t j = 0; j < pExpr->GetIndexCnt(); j += 1) {
                            if (!pExpr->GetIndex(j)->IsConstValue())
                                GenSubExpr(pObj, 0, pExpr->GetIndex(j), false, false, true);
                        }
                    }

                    if (!bIsLeftOfEqual) {
                        PrintArrayIndex(pObj, pRtnObj, pExpr, bIsLeftOfEqual); // generate read case statement
                        pExpr->SetArrayTempGenerated(true);
                    }

                    return;
                }

				if (!pExpr->HasExprTempVar() && !pExpr->IsFunction())
					return;
            }
        }
    }

    // generate an expression, start at current node and decend until a temp variable or leaf node is found
    if (!bGenSubExpr || pExpr->HasExprTempVar()
        || pExpr->IsLeaf() && pExpr->IsFunction() && pExpr->GetType() == GetVoidType())
    {
        PrintSubExpr(pObj, pRtnObj, pExpr, bIsLeftOfEqual, true);
    }
}

void CHtvDesign::PrintArrayIndex(CHtvObject * pObj, CHtvObject * pRtnObj, CHtvOperand * pOperand, bool bLeftOfEqual)
{
    bool bUseExpr = bLeftOfEqual && pOperand->GetOperator() == tk_equal;
    CHtvOperand *pExpr = 0;
    if (bUseExpr) {
        pExpr = pOperand;
        pOperand = pExpr->GetOperand1();
    }

    CHtvIdent * pIdent = pOperand->GetMember();

    pOperand->SetIsSubExprGenerated(true);

	// handle field indexing
    if (pOperand->HasFieldTempVar() && !pOperand->IsFieldTempGenerated()) {
        // non-constant field indexing, check for subexpressions
        CScSubField * pSubField = pOperand->GetSubField();
        for ( ; pSubField; pSubField = pSubField->m_pNext) {
            for (size_t i = 0; i < pSubField->m_indexList.size(); i += 1) {
                if (!pSubField->m_indexList[i]->IsConstValue())
                    GenSubExpr(pObj, 0, (CHtvOperand *)pSubField->m_indexList[i], false, false, true);
            }
        }

        PrintFieldIndex(pObj, 0, pOperand); // generate shift amount expression
        pOperand->SetFieldTempGenerated(true);
    }

    // determine number of bits for single case index
    Assert(pOperand->GetIndexCnt() > 0);
    Assert(pOperand->HasArrayTempVar());

    vector<int> fieldWidthList(pOperand->GetIndexCnt(), 0);
    vector<bool> fieldConstList(pOperand->GetIndexCnt(), false);
    vector<int> refList(pOperand->GetIndexCnt(), 0);

    int idxBits = 0;
    for (size_t j = 0; j < pOperand->GetIndexCnt(); j += 1) {
        fieldConstList[j] = pOperand->GetIndex(j)->IsConstValue();

        if (pOperand->GetIndex(j)->IsConstValue()) {
            refList[j] = pOperand->GetIndex(j)->GetConstValue().GetSint32();
            continue;
        }

		int idxWidth;
		if (!GetFieldWidthFromDimen(pIdent->GetDimenList()[j], idxWidth))
			ParseMsg(PARSE_FATAL, pOperand->GetLineInfo(), "invalid array index (32K limit)");

        idxBits += fieldWidthList[j] = idxWidth;
    }

    if (!pOperand->IsArrayTempGenerated()) {
        int bitPos = idxBits-1;
        for (size_t j = 0; j < pOperand->GetIndexCnt(); j += 1) {
            if (pOperand->GetIndex(j)->IsConstValue())
                continue;

            GenSubExpr(pObj, 0, pOperand->GetIndex(j), false, false, true);

            m_vFile.Print("%s", pOperand->GetArrayTempVar().c_str());

            if (idxBits > 1) {
                if (bitPos == bitPos - fieldWidthList[j]+1)
                    m_vFile.Print("[%d]", bitPos);
                else
                    m_vFile.Print("[%d:%d]", bitPos, bitPos - fieldWidthList[j]+1);
            }
            m_vFile.Print(" = ");

            PrintSubExpr(pObj, 0, pOperand->GetIndex(j));

            m_vFile.Print(";\n");

            bitPos -= fieldWidthList[j];
        }
    }


    m_vFile.Print("case (%s)\n", pOperand->GetArrayTempVar().c_str());
    m_vFile.IncIndentLevel();

    int caseCnt = 0;
    do {
        int j = 0;
        for (size_t i = 0; i < pOperand->GetIndexCnt(); i += 1) {
            j <<= fieldWidthList[i];
            if (fieldConstList[i])
                continue;
            j |= refList[i];
        }

        m_vFile.Print("%d'h%x: ", idxBits, j);

        if (!bLeftOfEqual)
            m_vFile.Print("%s = ", pOperand->GetExprTempVar().c_str());

        string arrayName = GenVerilogIdentName(pIdent->GetName(), refList);
        //m_vFile.Print("%s", arrayName.c_str() );
        PrintSubExpr(pObj, 0, bUseExpr ? pExpr : pOperand, bLeftOfEqual, false, true, arrayName);

        if (bLeftOfEqual && !pOperand->HasFieldTempVar()) {
            m_vFile.Print(" = ");
            PrintSubExpr(pObj, 0, bUseExpr ? pExpr->GetOperand2() : pOperand, false);
            //m_vFile.Print(" = temp$%s$%d", pOperand->GetExprBlockId().c_str(), pOperand->GetSubExprTempVarIdx());
        }

        m_vFile.Print(";\n");
        caseCnt += 1;
    } while (!pIdent->DimenIter(refList, fieldConstList));

    if (caseCnt < (1 << idxBits) && !bLeftOfEqual)
        m_vFile.Print("default: %s = %d'h0;\n", pOperand->GetExprTempVar().c_str(), pIdent->GetWidth());

	if (pRtnObj)
		pRtnObj->m_pOp = 0;

    m_vFile.DecIndentLevel();
    m_vFile.Print("endcase\n");
}

void CHtvDesign::PrintLogicalSubExpr(CHtvObject * pObj, CHtvObject * pRtnObj, CHtvOperand *pExpr)
{
    // a function call exists in the expression, and there are && or || operators. Must us if statement to
    //   conditionally execute expression to mimic C/C++ behavior

    if (pExpr->GetOperand1()->IsSubExprRequired()) {

        EToken tk1 = pExpr->GetOperand1()->GetOperator();
        bool bLogical1 = tk1 == tk_ampersandAmpersand || tk1 == tk_vbarVbar;

        if (bLogical1)
            PrintLogicalSubExpr(pObj, pRtnObj, pExpr->GetOperand1());
        else {
            GenSubExpr(pObj, pRtnObj, pExpr->GetOperand1(), false, false, true);

            if (pExpr->GetOperand1()->HasFieldTempVar())
                PrintSubExpr(pObj, pRtnObj, pExpr->GetOperand1(), false, true);
        }

    } else
        PrintSubExpr(pObj, pRtnObj, pExpr->GetOperand1(), false, true);

    if (pExpr->GetOperator() == tk_ampersandAmpersand)
        m_vFile.Print("if (%s)", pExpr->GetExprTempVar().c_str());
    else
        m_vFile.Print("if (!%s)", pExpr->GetExprTempVar().c_str());

    m_vFile.IncIndentLevel();

    if (pExpr->GetOperand2()->IsSubExprRequired()) {

        EToken tk2 = pExpr->GetOperand2()->GetOperator();
        bool bLogical2 = tk2 == tk_ampersandAmpersand || tk2 == tk_vbarVbar;

        CHtvOperand * pOp2 = pExpr->GetOperand2();
        bool bBeginEnd = !pOp2->IsLeaf()
            || pOp2->HasFuncTempVar() && pOp2->HasExprTempVar()
            || pOp2->HasArrayTempVar()
            || pOp2->HasFieldTempVar();

        if (bBeginEnd)
            m_vFile.Print(" begin\n");
        else
            m_vFile.Print("\n");

        if (bLogical2)
            PrintLogicalSubExpr(pObj, pRtnObj, pExpr->GetOperand2());
        else {
            GenSubExpr(pObj, pRtnObj, pExpr->GetOperand2(), false, false, true);

            if (pExpr->GetOperand2()->HasFieldTempVar())
                PrintSubExpr(pObj, pRtnObj, pExpr->GetOperand2(), false, true);
        }

		m_vFile.DecIndentLevel();

        if (bBeginEnd)
            m_vFile.Print("end\n");

    } else {
        m_vFile.Print("\n");
        PrintSubExpr(pObj, pRtnObj, pExpr->GetOperand2(), false, true);

		m_vFile.DecIndentLevel();
    }


    pExpr->SetIsSubExprGenerated(true);
}

void CHtvDesign::PrintQuestionSubExpr(CHtvObject * pObj, CHtvObject * pRtnObj, CHtvOperand *pExpr)
{
    // conditional expression with function call
    //  must use if/else statement

    if (pExpr->GetOperand1()->IsSubExprRequired())
        GenSubExpr(pObj, pRtnObj, pExpr->GetOperand1(), false, false, true);

    if (pExpr->GetOperand1()->HasExprTempVar()) {

        GenSubExpr(pObj, pRtnObj, pExpr->GetOperand1(), false, false, true);

        m_vFile.Print("if (%s)", pExpr->GetOperand1()->GetExprTempVar().c_str());
    } else {
        m_vFile.Print("if (");

        PrintSubExpr(pObj, pRtnObj, pExpr->GetOperand1());

        m_vFile.Print(")");
    }
    m_vFile.IncIndentLevel();

    if (pExpr->GetOperand2()->IsSubExprRequired()) {

        bool bBeginEnd = !pExpr->GetOperand2()->IsLeaf()
            || pExpr->GetOperand2()->IsFunction()
            || pExpr->GetExprTempVar() != pExpr->GetOperand2()->GetExprTempVar()
            || pExpr->GetOperand2()->HasArrayTempVar()
            || pExpr->GetOperand2()->HasFieldTempVar();

        if (bBeginEnd)
            m_vFile.Print(" begin\n");
        else
            m_vFile.Print("\n");

        GenSubExpr(pObj, pRtnObj, pExpr->GetOperand2(), false, false, true);

        if (pExpr->GetExprTempVar() != pExpr->GetOperand2()->GetExprTempVar()
            /*|| pExpr->GetOperand2()->HasFieldTempVar()*/)
        {
            m_vFile.Print("%s = ", pExpr->GetExprTempVar().c_str());
            PrintSubExpr(pObj, pRtnObj, pExpr->GetOperand2());
            m_vFile.Print(";\n");
        }

		m_vFile.DecIndentLevel();

        if (bBeginEnd)
            m_vFile.Print("end\n");

    } else {

        m_vFile.Print("\n");
        PrintSubExpr(pObj, pRtnObj, pExpr->GetOperand2(), false, true);
		m_vFile.DecIndentLevel();
    }


    m_vFile.Print("else");
    m_vFile.IncIndentLevel();

    if (pExpr->GetOperand3()->IsSubExprRequired()) {

        bool bBeginEnd = !pExpr->GetOperand3()->IsLeaf()
            || pExpr->GetOperand3()->IsFunction()
            || pExpr->GetExprTempVar() != pExpr->GetOperand3()->GetExprTempVar()
            || pExpr->GetOperand3()->HasArrayTempVar()
            || pExpr->GetOperand3()->HasFieldTempVar();

        if (bBeginEnd)
            m_vFile.Print(" begin\n");
        else
            m_vFile.Print("\n");

        GenSubExpr(pObj, pRtnObj, pExpr->GetOperand3(), false, false, true);

        if (pExpr->GetExprTempVar() != pExpr->GetOperand3()->GetExprTempVar()
            /*|| pExpr->GetOperand3()->HasFieldTempVar()*/)
        {
            m_vFile.Print("%s = ", pExpr->GetExprTempVar().c_str());
            PrintSubExpr(pObj, pRtnObj, pExpr->GetOperand3());
            m_vFile.Print(";\n");
        }

		m_vFile.DecIndentLevel();

        if (bBeginEnd)
            m_vFile.Print("end\n");

    } else {

        m_vFile.Print("\n");
        PrintSubExpr(pObj, pRtnObj, pExpr->GetOperand3(), false, true);
		m_vFile.DecIndentLevel();
    }

    pExpr->SetIsSubExprGenerated(true);
}

void CHtvDesign::PrintIncDecSubExpr(CHtvObject * pObj, CHtvObject * pRtnObj, CHtvOperand *pExpr)
{
    EToken tk = pExpr->GetOperator();

    if (tk == tk_postInc || tk == tk_postDec) {
        // copy variable to temp
        pExpr->GetOperand1()->SetIsSubExprRequired(true);
        GenSubExpr(pObj, pRtnObj, pExpr->GetOperand1(), false, false, true);
    }

    // perform inc/dec
    bool bSavedFlag = pExpr->GetOperand1()->IsSubExprGenerated();
    pExpr->GetOperand1()->SetIsSubExprGenerated(false);
    PrintSubExpr(pObj, pRtnObj, pExpr->GetOperand1(), true, false, false, "", tk, true);
    m_vFile.Print(" = ");
    pExpr->GetOperand1()->SetIsSubExprGenerated(false);
    PrintSubExpr(pObj, pRtnObj, pExpr->GetOperand1(), false, false, false, "", tk, true);
    m_vFile.Print(tk == tk_preInc || tk == tk_postInc ? " + " : " - ");
    m_vFile.Print(pExpr->GetOperand1()->IsSigned() ? "2'sd1;\n" : "1'h1;\n");
    pExpr->GetOperand1()->SetIsSubExprGenerated(bSavedFlag);

    if (tk == tk_preInc || tk == tk_preDec)
        // copy variable to temp
        PrintSubExpr(pObj, pRtnObj, pExpr->GetOperand1(), false, true);

    pExpr->SetIsSubExprGenerated(true);
}

string CHtvDesign::GetHexMask(int width)
{
	string mask;
	switch (width % 4) {
	case 0: break;
	case 1: mask = "1"; break;
	case 2: mask = "3"; break;
	case 3: mask = "7"; break;
	}

	width -= width % 4;
	for (int i = 0; i < width; i += 4)
		mask += "f";

	return mask;
}

void CHtvDesign::PrintSubExpr(CHtvObject * pObj, CHtvObject * pRtnObj, CHtvOperand *pExpr, bool bIsLeftOfEqual, bool bEntry, 
	bool bArrayIndexing, string arrayName, EToken prevTk, bool bIgnoreExprWidth)
{
 #ifdef WIN32
	if (pExpr->GetLineInfo().m_lineNum == 26)
		bool stop = true;
#endif

	bool bTempAssignment = bEntry && pExpr->HasExprTempVar() && (!pExpr->IsFunction() || pExpr->IsFuncGenerated());
    if (bTempAssignment) {
        m_vFile.Print("%s = ", pExpr->GetExprTempVar().c_str());
		prevTk = tk_equal;
	}

	if (pExpr->GetTempOp())
		pExpr = pExpr->GetTempOp();

    if (!pExpr->IsLeaf() && (!pExpr->IsSubExprGenerated() || !pExpr->HasExprTempVar())) {
        EToken tk = pExpr->GetOperator();

		CHtvOperand * pOp1 = pExpr->GetOperand1();
		CHtvOperand * pOp2 = pExpr->GetOperand2();
		CHtvOperand * pOp3 = pExpr->GetOperand3();

		bool bParenExpr = pExpr->IsParenExpr() && !(prevTk == tk_toe || prevTk == tk_equal) ||
			tk == tk_bang && prevTk == tk_bang;

        if (pOp2 == 0) {
            // unary operator
            if (bParenExpr)
                m_vFile.Print("(");

            m_vFile.Print("%s", GetTokenString(pExpr->GetOperator()).c_str());
            PrintSubExpr(pObj, pRtnObj, pOp1, false, false, false, "", tk);

            if (bParenExpr)
                m_vFile.Print(")");

        } else if (pOp3 == 0) {
            // binary operator
            bool bSigWidth = tk == tk_comma;

            if (!bSigWidth && bParenExpr)
                m_vFile.Print("(");

            if (bSigWidth)
                m_vFile.Print("{");

            if (tk != tk_typeCast) {

                if (tk == tk_equal && pOp1->HasFieldTempVar()) {
                    // generate a left of equal field indexing assignment
                    Assert(pOp2->HasExprTempVar());

                    PrintSubExpr(pObj, pRtnObj, pOp1, true, false, bArrayIndexing, arrayName, tk, bIgnoreExprWidth);

                    m_vFile.Print(" = (");

                    pOp1->SetIsSubExprGenerated(false);
                    PrintSubExpr(pObj, pRtnObj, pOp1, true, false, bArrayIndexing, arrayName, tk, bIgnoreExprWidth);

					int maskWidth = pOp1->GetVerWidth();
					uint64_t mask = maskWidth == 64 ? ~0ull : ((1ull << maskWidth)-1);

                    m_vFile.Print(" & ~(%d'h%llx << %s)) | ((%s & %d'h%llx) << %s)",
                        pOp1->GetVerWidth(), mask,
                        pOp1->GetFieldTempVar().c_str(), 
                        pOp2->GetExprTempVar().c_str(),
                        pOp1->GetVerWidth(), mask,
                        pOp1->GetFieldTempVar().c_str());

                } else if (tk == tk_period) {

					CHtvObject rtnObj;
					CHtvOperand * pNewOp;

					if (pOp1->GetOperator() == tk_period)
						PrintSubExpr(pObj, &rtnObj, pOp1, false, false, false, "", tk);
					else {
						if (pOp1->GetMember()->GetPrevHier()->IsStruct()) {
							pNewOp = new CHtvOperand;
							*pNewOp = *pObj->m_pOp;

							// copy subField list
							CScSubField * pSubField = pObj->m_pOp->GetSubField();
							CScSubField ** pNewSubField = pNewOp->GetSubFieldP();
							for (; pSubField; pSubField = pSubField->m_pNext) {
								CScSubField * pSf = new CScSubField;
								*pSf = *pSubField;
								*pNewSubField = pSf;
								pNewSubField = &pSf->m_pNext;
							}

							// append new subField to list
							if (pOp1->GetSubField() == 0) {
								CScSubField * pSf = new CScSubField;

								pSf->m_pIdent = pOp1->GetMember();
								pSf->m_indexList = pOp1->GetIndexList();
								pSf->m_bBitIndex = false;
								pSf->m_subFieldWidth = pOp1->GetMember()->GetWidth();
								pSf->m_pNext = 0;

								*pNewSubField = pSf;
							} else {
								pSubField = pOp1->GetSubField();

								for (; pSubField; pSubField = pSubField->m_pNext) {
									CScSubField * pSf = new CScSubField;
									*pSf = *pSubField;
									*pNewSubField = pSf;
									pNewSubField = &pSf->m_pNext;
								}
							}

							// create rtnObj
							rtnObj.m_pOp = pNewOp;

						} else {
							rtnObj.m_pOp = pOp1;
						}
					}

					if (prevTk != tk_period)
			           PrintSubExpr(&rtnObj, 0, pOp2, false, false, false, "", tk);

					if (!pOp2->GetMember()->IsReturnRef()) {
						rtnObj.m_pOp = 0;
						if (pOp2->HasExprTempVar()) {
							Assert(0);
						}
					}

					if (prevTk == tk_period && pRtnObj)
						*pRtnObj = rtnObj;

                } else {

                    PrintSubExpr(pObj, pRtnObj, pOp1, tk == tk_equal, false, bArrayIndexing, arrayName, tk);

                    // don't generate equal for left side indexed variable
                    if (tk != tk_equal || !pOp1->HasExprTempVar()) {

                        if (m_bClockedAlwaysAt && tk == tk_equal)

                            m_vFile.Print(" <= `HTDLY ");

                        else if (pExpr->GetOperator() == tk_greaterGreater && (!pOp1->IsLeaf() && pOp1->IsSigned() ||
							pOp1->IsLeaf() && pOp1->GetMember() == 0 && pOp1->IsSigned() ||
							pOp1->IsLeaf() && pOp1->GetMember() && pOp1->GetType()->IsSigned()))
						{
							// must use arithmetic shift right if op1 is signed
							m_vFile.Print(" >>> ");
						} else
                            m_vFile.Print(" %s ", GetTokenString(pExpr->GetOperator()).c_str());

                        PrintSubExpr(pObj, pRtnObj, pOp2, false, false, false, "", tk);
                    }
                }

            } else {

                PrintSubExpr(pObj, pRtnObj, pOp2, false, false, false, "", tk);
            }

            if (!bSigWidth && bParenExpr)
                m_vFile.Print(")");

            if (bSigWidth)
                m_vFile.Print("}");
        } else {
            // ? :
            m_vFile.Print("(");
            PrintSubExpr(pObj, pRtnObj, pOp1);
            m_vFile.Print(" ? ");
            PrintSubExpr(pObj, pRtnObj, pOp2);
            m_vFile.Print(" : ");
            PrintSubExpr(pObj, pRtnObj, pOp3);
            m_vFile.Print(")");
        }
         
        pExpr->SetIsSubExprGenerated(true);
    } else {

        // print leaf node
        CHtvIdent *pIdent = pExpr->GetMember();
        if (pExpr->IsConstValue() && (!pExpr->HasExprTempVar() || !pExpr->IsSubExprGenerated())) {
            if (pIdent && (pIdent->GetId() == CHtvIdent::id_const || pIdent->GetId() == CHtvIdent::id_enum)) {
                m_vFile.Print("%s", pIdent->GetName().c_str());
            } else if (pExpr->GetVerWidth() > 0) {
                const char *format = pExpr->IsScX() ? "%s%d'hx" : (pExpr->IsSigned() ? "%s%d'sd%lld" : "%s%d'h%llx");
                int constWidth = pExpr->GetOpWidth();
                //int constWidth = max(pExpr->GetVerWidth(), pExpr->GetConstValue().GetMinWidth());
                uint64_t mask = ~0ull >> (64 - constWidth);
                if (pExpr->GetConstValue().IsNeg())
                    m_vFile.Print(format, "-", constWidth, -pExpr->GetConstValue().GetSint64() & mask);
                else
                    m_vFile.Print(format, "", constWidth, pExpr->GetConstValue().GetUint64() & mask);
            } else {
                const char *format = pExpr->IsScX() ? "'hx" : (pExpr->IsSigned() ? "'sd%lld" : "'h%llx");
                m_vFile.Print(format, pExpr->GetConstValue().GetUint64());
            }

            pExpr->SetIsSubExprGenerated(true);

        } else if (pExpr->IsVariable() || pExpr->IsFunction() && bIsLeftOfEqual ||
            pExpr->IsFunction() && pExpr->IsFuncGenerated() ||
            pExpr->IsSubExprGenerated() && pExpr->HasExprTempVar())
        {
			string name;
			bool bTempVar;
			bool bSubField;
			int subFieldLowBit;
			int subFieldWidth;
			string fieldPosName;

			EIdentFmt identFmt = FindIdentFmt(pObj, pRtnObj, pExpr, prevTk, bIsLeftOfEqual, bArrayIndexing, arrayName,
				bIgnoreExprWidth, name, bTempVar, bSubField, subFieldLowBit, subFieldWidth, fieldPosName);

            CHtvIdent *pType = pExpr->GetCastType() ? pExpr->GetCastType() : pExpr->GetType();
            bool bSigned = !bIsLeftOfEqual && ((bTempVar && !bArrayIndexing) ? pExpr->IsExprTempVarSigned()
                : pType && pType->IsSigned());
            if (bSigned)
                m_vFile.Print("$signed(");

			switch (identFmt) {
			case eShfFldTmpAndVerMask:
               m_vFile.Print("((%s >> %s) & {%d{1'b1}})",
                    name.c_str(), pExpr->GetFieldTempVar().c_str(), pExpr->GetVerWidth());
				break;
			case eName:
		        m_vFile.Print("%s", name.c_str());
				break;
			case eShfLowBitAndVerMask:
				m_vFile.Print("((%s >> %d) & {%d{1'b1}}", 
					name.c_str(), subFieldLowBit, pExpr->GetVerWidth());
				break;
			case eNameAndVerRng:
				m_vFile.Print("%s[%d:%d]", 
					name.c_str(), subFieldLowBit + pExpr->GetVerWidth()-1, subFieldLowBit );
				break;
			case eShfLowBitAndMask1:
				m_vFile.Print("((%s >> %d) & 1'b1)", name.c_str(), subFieldLowBit);
				break;
			case eNameAndConstBit:
				m_vFile.Print("%s[%d]", name.c_str(), subFieldLowBit);
				break;
			case eNameAndVarBit:
                m_vFile.Print("%s[%s]", name.c_str(), fieldPosName.c_str() );
				break;
			case eShfLowBitAndSubMask:
				m_vFile.Print("((%s >> %d) & {%d{1'b1}})", 
					name.c_str(), subFieldLowBit, subFieldWidth);
				break;
			case eNameAndSubRng:
				m_vFile.Print("%s[%d:%d]", 
					name.c_str(), subFieldLowBit + subFieldWidth-1, subFieldLowBit);
				break;
			case eShfFldPosAndSubMask:
                m_vFile.Print("((%s >> %s) & {%d{1'b1}})", 
                    name.c_str(), fieldPosName.c_str(), subFieldWidth);
				break;
			case eNameAndCondTmpRng:
				m_vFile.Print("%s", name.c_str());

				if (g_htvArgs.IsVivadoEnabled() && !bIsLeftOfEqual) {
					// this is a hack for vivado
					int width = pExpr->GetVerWidth();
					if (name[0] == 't') {
						TempVarMap_Iter iter = m_tempVarMap.find(name);
						if (iter != m_tempVarMap.end())
							width = iter->second;
					}
					if (width > 1)
						m_vFile.Print("[%d:0]", width-1 );
				}
				break;
			default:
				Assert(0);
			}

            if (bSigned)
                m_vFile.Print(")");

            pExpr->SetIsSubExprGenerated(true);

        } else if (pExpr->IsFunction()) {

            GenVerilogFunctionCall(pObj, pRtnObj, pExpr);

            if (pExpr->GetType() != GetVoidType())
                pExpr->SetIsFuncGenerated(true);
        }
    }

    if (bTempAssignment)
        m_vFile.Print(";\n");
}

CHtvDesign::EIdentFmt CHtvDesign::FindIdentFmt(CHtvObject * pObj, CHtvObject * pRtnObj, CHtvOperand *pExpr, EToken prevTk,
	bool bIsLeftOfEqual, bool bArrayIndexing, string arrayName, bool bIgnoreExprWidth, string &name, bool &bTempVar,
	bool &bSubField, int &subFieldLowBit, int &subFieldWidth, string &fieldPosName)
{
	EIdentFmt identFmt = eInvalid;
	bSubField = false;
	subFieldLowBit = 0;
	subFieldWidth = pExpr->GetMember() ? pExpr->GetMember()->GetWidth() : pExpr->GetVerWidth();
	bool bConstRange = true;
	CHtvOperand * pOp = pExpr;
	bool bUseObjStk = false;
	bTempVar = pOp->IsSubExprGenerated() && pOp->HasExprTempVar() && !bArrayIndexing;

    CHtvIdent *pIdent = pExpr->GetMember();

	if (pExpr->HasFieldTempVar())
		fieldPosName = pExpr->GetFieldTempVar();

	if (bArrayIndexing) {
		// generate variable without variable name (just array indexing)
		bConstRange = GetSubFieldRange(pExpr, subFieldLowBit, subFieldWidth, false);
		bSubField = subFieldWidth < pExpr->GetMember()->GetWidth();
		name = arrayName;

	} else if (bTempVar) {

		if (!bArrayIndexing)
			name = pOp->GetExprTempVar();

		if (false && pOp->GetMember()) {
			bConstRange = GetSubFieldRange(pExpr, subFieldLowBit, subFieldWidth, false);
			bSubField = subFieldWidth < pExpr->GetMember()->GetWidth();
		}

	} else if (pExpr->HasArrayTempVar()) {

		GenArrayIndexingSubExpr(pObj, pRtnObj, pExpr, bIsLeftOfEqual);

	} else {
		if (pIdent->IsFunction() && pExpr->IsFuncGenerated())
			name = pOp->GetFuncTempVar();
		else
			name = GenVerilogIdentName(pIdent->GetName(pExpr), pExpr->GetIndexList());

		bConstRange = GetSubFieldRange(pExpr, subFieldLowBit, subFieldWidth, false);
		bSubField = subFieldWidth < pExpr->GetMember()->GetWidth();
	}

	if (!pExpr->IsSubExprGenerated() && pIdent->IsFunction() && bIsLeftOfEqual)
		name += "$Rtn";

    if (bTempVar && pExpr->IsExprTempVarWidth() && pExpr->GetExprTempVarWidth() < subFieldWidth) {
        bSubField = true;
        subFieldWidth = pExpr->GetExprTempVarWidth();
    }

	identFmt = eInvalid;

	if (!bTempVar || pExpr->IsExprTempVarWidth() || bUseObjStk && bTempVar && bSubField) {

		if (!bIgnoreExprWidth && pExpr->GetVerWidth() > 0 && pExpr->GetVerWidth() < subFieldWidth) {
			if (!bConstRange) {
				if (!bIsLeftOfEqual) {
					identFmt = eShfFldTmpAndVerMask;
				} else
					identFmt = eName;
			} else {
				if (g_htvArgs.IsVivadoEnabled() && !bIsLeftOfEqual && subFieldLowBit > 0 && prevTk != tk_comma) {
					identFmt = eShfLowBitAndVerMask;
				} else
					identFmt = eNameAndVerRng;
			}
		} else if (bSubField) {
			if (subFieldWidth == 1 && (bConstRange || !bIsLeftOfEqual)) {
				if (bConstRange) {
					if (g_htvArgs.IsVivadoEnabled() && !bIsLeftOfEqual && subFieldLowBit > 0 && prevTk != tk_comma)
						identFmt = eShfLowBitAndMask1;
					else if (bTempVar && pExpr->IsExprTempVarWidth() && pExpr->GetExprTempVarWidth() == 1)
						identFmt = eName;
					else
						identFmt = eNameAndConstBit;
				} else
					identFmt = eNameAndVarBit;
			} else {
				if (bConstRange) {
					if (g_htvArgs.IsVivadoEnabled() && !bIsLeftOfEqual && subFieldLowBit > 0 && prevTk != tk_comma) {
						identFmt = eShfLowBitAndSubMask;
					} else
						identFmt = eNameAndSubRng;
				} else if (!bIsLeftOfEqual) {
					identFmt = eShfFldPosAndSubMask;
				} else
					identFmt = eName;
			}
		} else
			identFmt = eNameAndCondTmpRng;
	} else
		identFmt = eNameAndCondTmpRng;

	return identFmt;
}

void CHtvDesign::PrintFieldIndex(CHtvObject * pObj, CHtvObject * pRtnObj, CHtvOperand *pExpr)
{
    // found non-constant sub-field index, generate index equation
    m_vFile.Print("%s =", pExpr->GetFieldTempVar().c_str());

    char *pPrefix = " ";

	for (CScSubField * pSubField = pExpr->GetSubField(); pSubField; pSubField = pSubField->m_pNext) {

        if (pSubField->m_pIdent && pSubField->m_pIdent->GetStructPos() > 0) {
            m_vFile.Print("%s%d", pPrefix, pSubField->m_pIdent->GetStructPos());
            pPrefix = " + ";
        }

        // check if all indecies are constants
		int idxBase = 1;
        for (size_t i = 0; i < pSubField->m_indexList.size(); i += 1)
			idxBase *= pSubField->m_pIdent ? pSubField->m_pIdent->GetDimenList()[i] : 1;

        int idx = 0;
        for (size_t i = 0; i < pSubField->m_indexList.size(); i += 1) {
            idxBase /= pSubField->m_pIdent ? pSubField->m_pIdent->GetDimenList()[i] : 1;
            if (pSubField->m_indexList[i]->IsConstValue()) {
                if (pSubField->m_indexList[i]->GetConstValue().GetSint32() * idxBase > 0) {
                    m_vFile.Print("%s%d * %s%d", pPrefix, pSubField->m_subFieldWidth, 
						g_htvArgs.IsQuartusEnabled() ? "(* multstyle = \"logic\" *) " : "",
						pSubField->m_indexList[i]->GetConstValue().GetSint32() * idxBase);
                    idx += pSubField->m_indexList[i]->GetConstValue().GetSint32() * idxBase;
                    pPrefix = " + ";
                }
            } else {
				m_vFile.Print("%s%d * %s(", pPrefix, pSubField->m_subFieldWidth * idxBase,
					g_htvArgs.IsQuartusEnabled() ? "(* multstyle = \"logic\" *) " : "");

                PrintSubExpr(pObj, pRtnObj, (CHtvOperand *)pSubField->m_indexList[i]);
                m_vFile.Print(")");
                pPrefix = " + ";
            }
        }

        if (!pSubField->m_pIdent)
            m_vFile.Print("%s%d", pPrefix, idx);

        pPrefix = " + ";
    }
    m_vFile.Print(";\n");
}

void CHtvDesign::GenVerilogFunctionCall(CHtvObject * pObj, CHtvObject * pRtnObj, CHtvOperand *pExpr)
{
    CHtvIdent *pIdent = pExpr->GetMember();

    if (!pIdent->IsBodyDefined() && !pIdent->GetType()->IsUserDefinedType()) {

        SynBuiltinFunctionCall(pObj, pRtnObj, pExpr);

	} else if (!pIdent->IsInlinedCall()) {
        m_vFile.Print("%s", pIdent->GetName().c_str());

        if (pExpr->GetParamCnt() > 0 || pIdent->GetGlobalRefSet().size() > 0 || pExpr->HasFuncTempVar()) {

            char *pSeparater = "(";

            if (pExpr->HasFuncTempVar()) {

                m_vFile.Print("(%s", pExpr->GetFuncTempVar().c_str());

                pSeparater = ", ";
			}

            for (int paramId = 0; paramId < pIdent->GetParamCnt(); paramId += 1) {

                CHtvIdent *pParamIdent = pIdent->FindIdent(pIdent->GetParamName(paramId));

                m_vFile.Print(pSeparater);

                PrintSubExpr(pObj, pRtnObj, pExpr->GetParam(paramId), false, false, false, "", tk_equal);

                pSeparater = ", ";

                if (!pParamIdent->IsReadOnly()) {
                    // list as output
                    m_vFile.Print(pSeparater);

                    PrintSubExpr(pObj, pRtnObj, pExpr->GetParam(paramId), true, false, false, "", tk_equal);

                    pSeparater = ", ";
                }
            }

			for (size_t i = 0; i < pIdent->GetGlobalRefSet().size(); i += 1) {

				CHtIdentElem &identElem = pIdent->GetGlobalRefSet()[i];

				vector<CHtDistRamWeWidth> *pWeWidth = identElem.m_pIdent->GetHtDistRamWeWidth();

				int startIdx = pWeWidth == 0 || (*pWeWidth).size() == 1 ? -1 : 0;
				int lastIdx = pWeWidth == 0 || (*pWeWidth).size() == 1 ? 0 : (int)(*pWeWidth).size();

				for (int rangeIdx = startIdx; rangeIdx < lastIdx; rangeIdx += 1) {

					m_vFile.Print("%s%s", pSeparater, GenVerilogIdentName(identElem, rangeIdx).c_str());

					pSeparater = ", ";
				}
			}

			m_vFile.Print(");\n");
		} else 
			m_vFile.Print(";\n");

		// generate array indexed output assignments
		for (int paramId = 0; paramId < pIdent->GetParamCnt(); paramId += 1) {

            CHtvIdent *pParamIdent = pIdent->FindIdent(pIdent->GetParamName(paramId));

            if (!pParamIdent->IsReadOnly()) {
                static CHtvOperand * pEqualOp = 0;
                if (pEqualOp == 0) {
                    pEqualOp = new CHtvOperand;
					pEqualOp->InitAsOperator(GetLineInfo(), tk_equal);
                }
                pEqualOp->SetOperand1(pExpr->GetParam(paramId));
                pEqualOp->SetOperand2(pExpr->GetParam(paramId));
                pEqualOp->SetExprTempVar("", false);
                pEqualOp->SetArrayTempVar("");
                pEqualOp->SetFieldTempVar("");
                pEqualOp->SetFuncTempVar("", false);

                if (pExpr->GetParam(paramId)->HasArrayTempVar()) {
                    PrintArrayIndex(pObj, pRtnObj, pEqualOp, true);
                } else if (pExpr->GetParam(paramId)->HasFieldTempVar()) {
                    pExpr->GetParam(paramId)->SetIsSubExprGenerated(false);
                    PrintSubExpr(pObj, pRtnObj, pEqualOp, true);
                    m_vFile.Print(";\n");
                }
            }
        }

    } else {
        // Inline function call
        SynInlineFunctionCall(pObj, pRtnObj, pExpr);
    }
}

void CHtvDesign::SynInlineFunctionCall(CHtvObject * pObj, CHtvObject * pRtnObj, CHtvOperand *pExpr)
{
    CHtvIdent *pFunc = pExpr->GetMember();

	if (pRtnObj && !pFunc->IsReturnRef())
		pRtnObj->m_pOp = CreateTempOp(pExpr, pExpr->GetFuncTempVar());

    // blocks with local variables must be named
#ifdef WIN32
	if (pExpr->GetLineInfo().m_lineNum == 48)
		bool stop = true;
#endif

	if (pFunc->GetPrevHier()->IsStruct()) {
		m_vFile.Print("begin : \\%s.%s$%d  // %s:%d\n", 
			pFunc->GetPrevHier()->GetName().c_str(), pFunc->GetName().c_str(), pFunc->GetNextBeginBlockId(),
			pExpr->GetLineInfo().m_fileName.c_str(), pExpr->GetLineInfo().m_lineNum);
	} else {
		m_vFile.Print("begin : \\%s$%d  // %s:%d\n", pFunc->GetName().c_str(), pFunc->GetNextBeginBlockId(),
			pExpr->GetLineInfo().m_fileName.c_str(), pExpr->GetLineInfo().m_lineNum);
	}

    m_vFile.IncIndentLevel();

	GenFuncVarDecl(pFunc);

    if (pFunc->GetType() != GetVoidType() && !pFunc->IsReturnRef()) {

        string bitRange;
        if (pFunc->GetWidth() > 1)
            bitRange = VA("[%d:0] ", pFunc->GetWidth()-1);

        m_vFile.Print("reg %s%s$Rtn;\n", bitRange.c_str(), pFunc->GetName().c_str());
		m_vFile.Print("\n");
    }

	// enum constants
    CHtvIdentTblIter identIter(pFunc->GetFlatIdentTbl());
	bool bEnumFound = false;
	for (identIter.Begin(); !identIter.End(); identIter++) {
		if (!identIter->IsIdEnum())
			continue;

		if (!bEnumFound) {
			m_vFile.Print("// initialization of enum constants\n");
			bEnumFound = true;
		}

		//if (enumIter->IsReadRef(0))
		m_vFile.Print("%s = %d'h%x;\n", identIter->GetName().c_str(),
			identIter->GetType()->GetWidth(), identIter->GetConstValue().GetSint64());
	}
	if (bEnumFound)
	    m_vFile.Print("\n");

    // initialize input parameters
    for (int paramIdx = 0; paramIdx < pFunc->GetParamCnt(); paramIdx += 1) {
        CHtvIdent * pParamIdent = pFunc->GetParamIdent(paramIdx);

        CHtvOperand *pParam = pExpr->GetParam(paramIdx);

        if (pFunc->GetGlobalRefSet().size() == 0 || pFunc->IsStructMember()) {
            m_vFile.Print("%s = ", pParamIdent->GetFlatIdent()->GetName().c_str());

            PrintSubExpr(0, 0, pParam);
            m_vFile.Print(";\n");

        } else {
            bool bFoundSubExpr;
            bool bWriteIndex;
            FindSubExpr(0, 0, pParam, bFoundSubExpr, bWriteIndex);

            m_vFile.Print("%s = ", pParamIdent->GetFlatIdent()->GetName().c_str());

            PrintSubExpr(0, 0, pParam);
            m_vFile.Print(";\n");

            if (bFoundSubExpr) {
                m_vFile.DecIndentLevel();
                m_vFile.Print("end\n");
            }
        }
    }

    SynStatementList(pFunc, pObj, pRtnObj, (CHtvStatement *)pFunc->GetStatementList());

    // assign outputs parameters
    for (int paramIdx = 0; paramIdx < pFunc->GetParamCnt(); paramIdx += 1) {
        if (pFunc->GetParamIsConst(paramIdx))
            continue;

		CHtvOperand * pOp2 = new CHtvOperand;
		pOp2->InitAsIdentifier(GetLineInfo(), pFunc->GetParamIdent(paramIdx)->GetFlatIdent());

		CHtvOperand * pEqualOp = new CHtvOperand;
		pEqualOp->InitAsOperator(GetLineInfo(), tk_equal, pExpr->GetParam(paramIdx), pOp2);

        bool bFoundSubExpr;
        bool bWriteIndex;
		FindSubExpr(pObj, pRtnObj, pEqualOp, bFoundSubExpr, bWriteIndex, false, true);

		if (!bWriteIndex || !bFoundSubExpr)
			GenSubExpr(pObj, pRtnObj, pEqualOp, false, false);

		if (!bWriteIndex)
			m_vFile.Print(";\n");
 
        if (bFoundSubExpr) {
            m_vFile.DecIndentLevel();
            m_vFile.Print("end\n");
        }
    }

	if (pFunc->GetType() != GetVoidType() && !pFunc->IsReturnRef()) {
		m_vFile.Print("%s = %s$Rtn;\n", pExpr->GetFuncTempVar().c_str(), pFunc->GetName().c_str());
		pExpr->SetTempOp(CreateTempOp(pExpr, pExpr->GetFuncTempVar()));
	}

    m_vFile.DecIndentLevel();
    m_vFile.Print("end\n");
}

void CHtvDesign::GenFuncVarDecl(CHtvIdent * pFunc)
{
	// enum constants
    CHtvIdentTblIter identIter(pFunc->GetFlatIdentTbl());
	for (identIter.Begin(); !identIter.End(); identIter++) {
		if (!identIter->IsIdEnum())
			continue;

		string bitRange;
		if (identIter->GetType()->GetWidth() > 1)
			bitRange = VA("[%d:0] ", identIter->GetType()->GetWidth()-1);

		//if (enumIter->IsReadRef(0))
		m_vFile.Print("reg %s%s;\n", bitRange.c_str(), identIter->GetName().c_str(),
			identIter->GetType()->GetWidth(), identIter->GetConstValue().GetSint64());
	}

    // declare temps
    for (identIter.Begin(); !identIter.End(); identIter++) {
        if (identIter->IsVariable() && !identIter->IsRegClkEn() /*&& !identIter->IsScPrimOutput()*/) {

			string bitRange;
            if (identIter->GetWidth() > 1)
                bitRange = VA("[%d:0] ", identIter->GetWidth()-1);

            vector<int> refList(identIter->GetDimenCnt(), 0);

            CHtvIdent * pFlatIdent = m_pInlineHier->InsertFlatIdent( identIter->GetHierIdent(), identIter->GetHierIdent()->GetName() );
            pFlatIdent->SetId(CHtvIdent::id_variable);

            identIter->SetInlineName(pFlatIdent->GetName());

			for (int elemIdx = 0; elemIdx < identIter->GetDimenElemCnt(); elemIdx += 1) {

				bool bWire = identIter->IsAssignOutput(elemIdx) || identIter->IsHtPrimOutput(elemIdx) || identIter->IsPrimOutput();

				string identName = identIter->GetVerilogName(elemIdx);

				m_vFile.SetVarDeclLines(true);

				GenHtAttributes(identIter(), identName);

                m_vFile.Print("%s   %-10s %s;\n", bWire ? "wire" : "reg ", bitRange.c_str(), identName.c_str());

				if (!bWire && m_bInAlwaysAtBlock)
					m_vFile.PrintVarInit("%s = 32'h0;\n", identName.c_str());

				m_vFile.SetVarDeclLines(false);
			}
        }
    }
}

void CHtvDesign::SynBuiltinFunctionCall(CHtvObject * pObj, CHtvObject * pRtnObj, CHtvOperand * pExpr)
{
    CHtvIdent * pFunc = pExpr->GetMember();

    switch (pFunc->GetFuncId()) {
    case CHtvIdent::fid_and_reduce:

        GenSubExpr(pObj, pRtnObj, (CHtvOperand *)pObj->m_pOp, false, false, true);

        m_vFile.Print("%s = & ", pExpr->GetFuncTempVar().c_str());
        
        PrintSubExpr(pObj, pRtnObj, (CHtvOperand *)pObj->m_pOp);

        m_vFile.Print(";\n");

        break;
    case CHtvIdent::fid_nand_reduce:

        GenSubExpr(pObj, pRtnObj, (CHtvOperand *)pObj->m_pOp, false, false, true);

        m_vFile.Print("%s = ~& ", pExpr->GetFuncTempVar().c_str());
        
        PrintSubExpr(pObj, pRtnObj, (CHtvOperand *)pObj->m_pOp);

        m_vFile.Print(";\n");

        break;
    case CHtvIdent::fid_or_reduce:

        GenSubExpr(pObj, pRtnObj, (CHtvOperand *)pObj->m_pOp, false, false, true);

        m_vFile.Print("%s = | ", pExpr->GetFuncTempVar().c_str());
        
        PrintSubExpr(pObj, pRtnObj, (CHtvOperand *)pObj->m_pOp);

        m_vFile.Print(";\n");

        break;
    case CHtvIdent::fid_nor_reduce:

        GenSubExpr(pObj, pRtnObj, (CHtvOperand *)pObj->m_pOp, false, false, true);

        m_vFile.Print("%s = ~| ", pExpr->GetFuncTempVar().c_str());
        
        PrintSubExpr(pObj, pRtnObj, (CHtvOperand *)pObj->m_pOp);

        m_vFile.Print(";\n");

        break;
    case CHtvIdent::fid_xor_reduce:

        GenSubExpr(pObj, pRtnObj, (CHtvOperand *)pObj->m_pOp, false, false, true);

        m_vFile.Print("%s = ^ ", pExpr->GetFuncTempVar().c_str());
        
        PrintSubExpr(pObj, pRtnObj, (CHtvOperand *)pObj->m_pOp);

        m_vFile.Print(";\n");

        break;
    case CHtvIdent::fid_xnor_reduce:

        GenSubExpr(pObj, pRtnObj, (CHtvOperand *)pObj->m_pOp, false, false, true);

        m_vFile.Print("%s = ~^ ", pExpr->GetFuncTempVar().c_str());
        
        PrintSubExpr(pObj, pRtnObj, (CHtvOperand *)pObj->m_pOp);

        m_vFile.Print(";\n");

        break;
    default:
        ParseMsg(PARSE_FATAL, pExpr->GetLineInfo(), "internal error - unknown builtin function");
    }
}

const char * CHtvDesign::GenVerilogName(const char *pOrigName, const char *pPrefix)
{
    static char buf[1024];

    strcpy(buf, g_htvArgs.GetDesignPrefixString().c_str());
    char *pBuf = buf + strlen(buf);

    if (pPrefix) {
        *pBuf++ = '_';
        strcpy(pBuf, pPrefix);
        pBuf += strlen(pPrefix);
    }

    const char *pOrig = pOrigName;
    if (*pOrig == 'C')
        pOrig += 1;

    bool bFirstChar = true;
    while (*pOrig) {
        if (isupper(*pOrig) && !bFirstChar)
            *pBuf++ = '_';
        bFirstChar = false;
        *pBuf++ = tolower(*pOrig++);
    }
    *pBuf = '\0';

    return buf;
}

void CHtvDesign::PrintHotStatesCaseItem(CHtvOperand *pExpr, CHtvIdent *pCaseIdent, int hotStatesWidth)
{
    if (hotStatesWidth > 1)
        m_vFile.Print("{");

    int exprWidth = pExpr->GetMinWidth();
    int64_t caseItemValue = pExpr->GetConstValue().GetSint64();

    bool first = true;
    for (int i = exprWidth-1; i >= 0; i -= 1) {
        if (caseItemValue & (((uint64)1)<<i)) {
            m_vFile.Print("%s%s[%d]", first ? "" : ", ", pCaseIdent->GetName().c_str(), i);
            first = false;
        }
    }
    if (hotStatesWidth > 1)
        m_vFile.Print("}");

    if (pExpr->GetMember())
        m_vFile.Print(" /* %s */ ", pExpr->GetMember()->GetName().c_str());
}

CHtvDesign::EAlwaysType CHtvDesign::FindWriteRefVars(CHtvIdent *pHier, CHtvStatement *pStatement, CHtvOperand *pExpr, bool bIsLeftSideOfEqual, bool bFunction)
{
    // recursively desend expression tree

    CAlwaysType alwaysType;
    Assert(pExpr != 0);
    if (pExpr->IsLeaf()) {
        CHtvIdent *pIdent = pExpr->GetMember();
        if (pIdent == 0 || pIdent->IsScState())
            return atUnknown;

        int elemIdx = CalcElemIdx(pIdent, pExpr->GetIndexList());
        CHtIdentElem identElem(pIdent, elemIdx);

        if (bIsLeftSideOfEqual) {

            Assert(pIdent && pIdent->GetId() == CHtvIdent::id_variable);

            if (pIdent->IsRegister() && !pStatement->IsScPrim()) {
                CHtIdentElem clockElem(pHier->GetClockIdent(), 0);
                pStatement->AddWriteRef(clockElem);
                alwaysType = atClocked;	// register assignment

            } else {
                if (pIdent->IsVariable() && !pIdent->IsRegister() && !pIdent->IsPort()
                    && !pIdent->IsParam() && (!pIdent->IsLocal() || !bFunction)) {
                        if (!pStatement->IsScPrim())
                            pStatement->AddWriteRef(identElem);
                }
                alwaysType = atNotClocked;
            }

        } else {

            if (pIdent->IsScLocal()) {
                if (!pStatement->IsScPrim())
                    pStatement->AddWriteRef(identElem);
            }

            if (pIdent->GetId() == CHtvIdent::id_function) {
                // count function parameter references

                for (unsigned int i = 0; i < pExpr->GetParamCnt(); i += 1) {
                    bool bIsReadOnly = pIdent->GetParamIsConst(i);

                    if (bIsReadOnly && pIdent->IsScPrim()) continue;

                    CAlwaysType paramType = FindWriteRefVars(pHier, pStatement, pExpr->GetParam(i), !bIsReadOnly, bFunction);

                    if (!bIsReadOnly)
                        alwaysType += paramType;
                }

                if (alwaysType.IsError())
                    ParseMsg(PARSE_FATAL, pExpr->GetLineInfo(), "Mix of register and non-register outputs");

                if (!pIdent->IsMethod())
                    // Count references in called function (but not function UpdateOutput)
                    alwaysType += FindWriteRefVars(pHier, pIdent->IsScPrim() ? 0 : pStatement, (CHtvStatement *)pIdent->GetStatementList(), true);

            } else if (pIdent->IsVariable() && !pIdent->IsRegister() && !pIdent->IsPort() && !pIdent->IsParam() 
                &&  (!pIdent->IsLocal() || !bFunction)) {

                    if (!pStatement->IsScPrim())
                        pStatement->AddWriteRef(identElem);

                    alwaysType = atNotClocked;
            }
        }

    } else {

        CHtvOperand *pOp1 = pExpr->GetOperand1();
        CHtvOperand *pOp2 = pExpr->GetOperand2();
        CHtvOperand *pOp3 = pExpr->GetOperand3();

        if (pExpr->GetOperator() == tk_equal) {

            alwaysType += FindWriteRefVars(pHier, pStatement, pOp1, true, bFunction);
            if (alwaysType.IsClocked())
                // Found register on left of equal, ignore right side
                // Fixme - must check right side of equal in case a function is called and assigns a non-register
                return atClocked;

            alwaysType += FindWriteRefVars(pHier, pStatement, pOp2, false, bFunction);

        } else {
            if (pOp1)
                alwaysType += FindWriteRefVars(pHier, pStatement, pOp1, false, bFunction);
            if (pOp2)
                alwaysType += FindWriteRefVars(pHier, pStatement, pOp2, false, bFunction);
            if (pOp3)
                alwaysType += FindWriteRefVars(pHier, pStatement, pOp3, false, bFunction);
        }
    }

    return alwaysType;
}

CHtvDesign::EAlwaysType CHtvDesign::FindWriteRefVars(CHtvIdent *pHier, CHtvStatement *pEntryBaseStatement, CHtvStatement *pStatement, bool bFunction)
{
    CAlwaysType alwaysType;

    for ( ; pStatement; pStatement = pStatement->GetNext()) {
        CHtvStatement *pBaseStatement = pEntryBaseStatement ? pEntryBaseStatement : pStatement;
        switch (pStatement->GetStType()) {
        case st_compound:
            alwaysType += FindWriteRefVars(pHier, pBaseStatement, pStatement->GetCompound1(), bFunction);
            break;
        case st_return:
            if (pStatement->GetExpr() == 0)
                break;
            // else fall into st_assign
        case st_assign:
            alwaysType += FindWriteRefVars(pHier, pBaseStatement, pStatement->GetExpr(), false, bFunction);
            break;
        case st_if:
            alwaysType += FindWriteRefVars(pHier, pBaseStatement, pStatement->GetCompound1(), bFunction);
            alwaysType += FindWriteRefVars(pHier, pBaseStatement, pStatement->GetCompound2(), bFunction);

            if (!alwaysType.IsClocked())
                FindWriteRefVars(pHier, pBaseStatement, pStatement->GetExpr(), false, bFunction);
            break;
        case st_switch:
        case st_case:
            alwaysType += FindWriteRefVars(pHier, pBaseStatement, pStatement->GetCompound1(), bFunction);

            if (!alwaysType.IsClocked())
                FindWriteRefVars(pHier, pBaseStatement, pStatement->GetExpr(), false, bFunction);
            break;
        case st_default:
            alwaysType += FindWriteRefVars(pHier, pBaseStatement, pStatement->GetCompound1(), bFunction);
            break;
        case st_null:
        case st_break:
            break;
        default:
            Assert(0);
        }
    }

    return alwaysType;
}

bool CHtvDesign::IsExpressionEquivalent(CHtvOperand * pExpr1, CHtvOperand * pExpr2)
{
    // determine if the two expressions are equivalent
    if (pExpr1->IsLeaf() != pExpr2->IsLeaf())
        return false;

    // recursively walk expression
    if (pExpr1->IsLeaf()) {
        CHtvIdent *pIdent1 = pExpr1->GetMember();
        CHtvIdent *pIdent2 = pExpr2->GetMember();

        if (pIdent1 != pIdent2 || pExpr1->IsConstValue() != pExpr2->IsConstValue())
            return false;

        if (pExpr1->IsConstValue()) {
            if (!CConstValue::IsEquivalent(pExpr1->GetConstValue(), pExpr2->GetConstValue()))
                return false;
        } else {
            Assert(pIdent1);

            // check index lists
            for (size_t i = 0; i < pExpr1->GetIndexCnt(); i += 1)
                if (!IsExpressionEquivalent(pExpr1->GetIndex(i), pExpr2->GetIndex(i)))
                    return false;

            return true;
        }
    } else {

        if (pExpr1->GetOperator() != pExpr2->GetOperator())
            return false;

        CHtvOperand *pOp11 = pExpr1->GetOperand1();
        CHtvOperand *pOp12 = pExpr1->GetOperand2();
        CHtvOperand *pOp13 = pExpr1->GetOperand3();

        CHtvOperand *pOp21 = pExpr2->GetOperand1();
        CHtvOperand *pOp22 = pExpr2->GetOperand2();
        CHtvOperand *pOp23 = pExpr2->GetOperand3();

        if (pOp11 && !!IsExpressionEquivalent(pOp11, pOp21))
            return false;
        if (pOp12 && !!IsExpressionEquivalent(pOp12, pOp22))
            return false;
        if (pOp13 && !!IsExpressionEquivalent(pOp13, pOp23))
            return false;
    }
    return false;
}

string CHtvDesign::GenVerilogIdentName(CHtIdentElem & identElem, int distRamWrEnRangeIdx)
{
    string dimenName = identElem.m_pIdent->GetName();

    if (distRamWrEnRangeIdx >= 0) {
        CHtDistRamWeWidth &weWidth = (*identElem.m_pIdent->GetHtDistRamWeWidth())[distRamWrEnRangeIdx];
        dimenName += VA("_%d_%d", weWidth.m_highBit, weWidth.m_lowBit);
    }

    int elemIdx = identElem.m_elemIdx;
    vector<int> & dimenList = identElem.m_pIdent->GetDimenList();
    for (size_t i = 0; i < dimenList.size(); i += 1) {
        int dimIdx = elemIdx % dimenList[i];
        elemIdx /= dimenList[i];

        dimenName += VA("$%d", dimIdx);
    }

    return dimenName;
}

string CHtvDesign::GenVerilogIdentName(const string &name, const vector<int> &refList)
{
    string identName = name;

    for (size_t i = 0; i < refList.size(); i += 1)
        identName += VA("$%d", refList[i]);

    return identName;
}

string CHtvDesign::GenVerilogIdentName(const string &name, const HtfeOperandList_t &indexList)
{
    string identName = name;
 
    for (size_t i = 0; i < indexList.size(); i += 1) {
        Assert(indexList[i]->IsConstValue());
        identName += VA("$%d", indexList[i]->GetConstValue().GetSint32());
    }

    return identName;
}

string CHtvDesign::GenVerilogArrayIdxName(const string &name, int idx)
{
    string indexName;
    if (name.substr(0, 2) == "r_")
        indexName = "c_" + name.substr(2) + "$r$AI" + FormatString("%d", idx);
    else
        indexName = name + "$AI" + FormatString("%d", idx);

    return indexName;
}

string CHtvDesign::GenVerilogFieldIdxName(const string &name, int idx)
{
    string indexName;
    if (name.substr(0, 2) == "r_")
        indexName = "c_" + name.substr(2) + "$r$FI" + FormatString("%d", idx);
    else
        indexName = name + "$FI" + FormatString("%d", idx);

    return indexName;
}

bool CHtvDesign::GenXilinxClockedPrim(CHtvIdent *pHier, CHtvObject * pObj, CHtvStatement *pStatement, CHtvOperand *pEnable, bool bCheckOnly) 
{
    if (pStatement->GetStType() == st_if) {
        if (pStatement->GetCompound1() == 0 || pStatement->GetCompound2() != 0)
            return false;

        CHtvOperand *pIfExpr = pStatement->GetExpr();
        if (!pIfExpr->IsLeaf() || pIfExpr->GetMember() == 0 || pIfExpr->GetVerWidth() != 1)
            return false;

        bool bAllXilinxFdPrims = true;
        for (CHtvStatement *pStatement2 = pStatement->GetCompound1(); pStatement2; pStatement2 = pStatement2->GetNext())
            bAllXilinxFdPrims &= GenXilinxClockedPrim(pHier, pObj, pStatement2, pStatement->GetExpr(), bCheckOnly);

        if (bCheckOnly && bAllXilinxFdPrims)
            pStatement->SetIsXilinxFdPrim();

        return bAllXilinxFdPrims;
    }

    if (pStatement->GetStType() == st_switch)
        return false;

    Assert (pStatement->GetStType() == st_assign);

    if (!bCheckOnly && !pStatement->IsXilinxFdPrim())
        return false;

    CHtvOperand *pExpr = pStatement->GetExpr();

    CHtvOperand *pOp1 = pExpr->GetOperand1();

    if (pOp1 == 0 || pOp1->IsLeaf() && pOp1->GetMember()->GetId() == CHtvIdent::id_function)
        return false;

    Assert(pExpr->IsLeaf() == false && pExpr->GetOperator() == tk_equal);

    if (!pOp1->GetMember()->IsRegister())
        return false;

    //if (pOp1->GetMember()->IsHtQueueRam())
    //	return true;

    CHtvOperand *pOp2 = pExpr->GetOperand2();

    if (IsConcatLeaves(pOp2)) {	// FD / FDE
        if (bCheckOnly) {
            pStatement->SetIsXilinxFdPrim();
            pOp1->GetMember()->SetIsHtPrimOutput(pOp1);
            return true;
        }

        int op1BaseIdx = pOp1->IsSubFieldValid() ? GetSubFieldLowBit(pOp1) : 0;

        for (int i = 0; i < pExpr->GetVerWidth(); i += 1) {
            if (pExpr->GetVerWidth() == 1 && !pOp1->IsSubFieldValid())
                m_vFile.Print("%s %s_ ( ", pEnable ? "FDE" : "FD", pOp1->GetMember()->GetVerilogName().c_str());
            else
                m_vFile.Print("%s %s_%d_ ( ", pEnable ? "FDE" : "FD", pOp1->GetMember()->GetVerilogName().c_str(), op1BaseIdx+i);

            if (pOp1->GetMember()->GetType()->GetWidth() == 1)
                m_vFile.Print(".Q(%s), ", pOp1->GetMember()->GetName().c_str() );
            else
                m_vFile.Print(".Q(%s[%d]), ", pOp1->GetMember()->GetName().c_str(), op1BaseIdx+i );

            int bitIdx;
            bool bDimen;
            string name = GetConcatLeafName(i, pOp2, bDimen, bitIdx);
            if (!bDimen)
                m_vFile.Print(".D(%s), ", name.c_str() );
            else
                m_vFile.Print(".D(%s[%d]), ", name.c_str(), bitIdx );

            if (pEnable) {
                if (pEnable->IsSubFieldValid())
                    m_vFile.Print(".CE(%s[%d]), ", pEnable->GetMember()->GetName(pEnable).c_str(), GetSubFieldLowBit(pEnable));
                else
                    m_vFile.Print(".CE(%s), ", pEnable->GetMember()->GetName().c_str() );
            }
            m_vFile.Print(".C(%s));\n", pHier->GetClockIdent()->GetName().c_str());
        }
        return true;
    } else if (pOp2->GetOperator() == tk_question) {
        CHtvOperand *pExpr2 = pOp2;
        CHtvOperand *pOp2_1 = pExpr2->GetOperand1();
        CHtvOperand *pOp2_2 = pExpr2->GetOperand2();
        CHtvOperand *pOp2_3 = pExpr2->GetOperand3();

        if (pOp2_3->IsLeaf()) {

            // Operand 1 must be an identifier with width 1
            if (!pOp2_1->IsLeaf() || pOp2_1->GetMember() == 0) {
                ParseMsg(PARSE_WARNING, pExpr->GetLineInfo(), "Unable to translate statement to Xilinx FD Primitive(s)");
                Assert(bCheckOnly);
                return false;
            }

            // Operand 2 must be a constant of zero or all ones
            if (!pOp2_2->IsLeaf() || !pOp2_2->IsConstValue()) {
                ParseMsg(PARSE_WARNING, pExpr->GetLineInfo(), "Unable to translate statement to Xilinx FD Primitive(s)");
                Assert(bCheckOnly);
                return false;
            }

            if (pOp2_3->IsLeaf() && pOp2_3->GetMember()) {
                if (bCheckOnly) {
                    pStatement->SetIsXilinxFdPrim();
                    pOp1->GetMember()->SetIsHtPrimOutput(pOp1);
                    return true;
                }

                // FDR / FDRE / FDSE
                int op1BaseIdx = pOp1->IsSubFieldValid() ? GetSubFieldLowBit(pOp1) : 0;
                int op2_1BaseIdx = pOp2_1->IsSubFieldValid() ? GetSubFieldLowBit(pOp2_1) : 0;
                int op2_3BaseIdx = pOp2_3->IsSubFieldValid() ? GetSubFieldLowBit(pOp2_3) : 0;

                uint64 constValue = pOp2_2->GetConstValue().GetUint64();
                for (int i = 0; i < pExpr->GetVerWidth(); i += 1) {
                    bool bIsFDRE = ((constValue >> i) & 1) == 0;

                    if (bIsFDRE) {
                        if (pExpr->GetVerWidth() == 1 && !pOp1->IsSubFieldValid())
                            m_vFile.Print("%s %s_ ( ", pEnable ? "FDRE" : "FDR", pOp1->GetMember()->GetVerilogName().c_str(), op1BaseIdx+i);
                        else
                            m_vFile.Print("%s %s_%d_ ( ", pEnable ? "FDRE" : "FDR", pOp1->GetMember()->GetVerilogName().c_str(), op1BaseIdx+i);
                    } else {
                        if (pExpr->GetVerWidth() == 1 && !pOp1->IsSubFieldValid())
                            m_vFile.Print("%s %s_ ( ", pEnable ? "FDSE" : "FDS", pOp1->GetMember()->GetVerilogName().c_str(), op1BaseIdx+i);
                        else
                            m_vFile.Print("%s %s_%d_ ( ", pEnable ? "FDSE" : "FDS", pOp1->GetMember()->GetVerilogName().c_str(), op1BaseIdx+i);
                    }

                    if (pOp1->GetMember()->GetType()->GetWidth() == 1)
                        m_vFile.Print(".Q( %s ), ", pOp1->GetMember()->GetName(pOp1).c_str() );
                    else
                        m_vFile.Print(".Q( %s[%d] ), ", pOp1->GetMember()->GetName(pOp1).c_str(), op1BaseIdx+i );

                    if (pOp2_3->GetMember()->GetType()->GetWidth() == 1)
                        m_vFile.Print(".D( %s ), ", pOp2_3->GetMember()->GetName(pOp2_3).c_str() );
                    else
                        m_vFile.Print(".D( %s[%d] ), ", pOp2_3->GetMember()->GetName(pOp2_3).c_str(), op2_3BaseIdx+i );

                    if (pOp2_1->GetMember()->GetType()->GetWidth() == 1)
                        m_vFile.Print(".%c( %s ), ", bIsFDRE ? 'R' : 'S', pOp2_1->GetMember()->GetName(pOp2_1).c_str() );
                    else
                        m_vFile.Print(".%c( %s[%d] ), ", bIsFDRE ? 'R' : 'S', pOp2_1->GetMember()->GetName(pOp2_1).c_str(), op2_1BaseIdx );

                    if (pEnable) {
                        if (pEnable->IsSubFieldValid())
                            m_vFile.Print(".CE( %s[%d] ), ", pEnable->GetMember()->GetName(pEnable).c_str(), GetSubFieldLowBit(pEnable));
                        else
                            m_vFile.Print(".CE( %s ), ", pEnable->GetMember()->GetName(pEnable).c_str() );
                    }
                    m_vFile.Print(".C( %s ) );\n", pHier->GetClockIdent()->GetName().c_str());
                }
                return true;
            }

        } else {	// Check for FDRS / FDRSE
            // Operand 1 must be an identifier with width 1
            if (!pOp2_1->IsLeaf() || pOp2_1->GetMember() == 0) {
                ParseMsg(PARSE_WARNING, pExpr->GetLineInfo(), "Unable to translate statement to Xilinx FD Primitive(s)");
                Assert(bCheckOnly);
                return false;
            }

            // Operand 2 must be a constant of zero
            if (!pOp2_2->IsLeaf() || !pOp2_2->IsConstValue()) {
                ParseMsg(PARSE_WARNING, pExpr->GetLineInfo(), "Unable to translate statement to Xilinx FD Primitive(s)");
                Assert(bCheckOnly);
                return false;
            }

            uint64 constValue = pOp2_2->GetConstValue().GetUint64();

            if (constValue != 0) {
                ParseMsg(PARSE_WARNING, pExpr->GetLineInfo(), "Unable to translate statement to Xilinx FD Primitive(s)");
                Assert(bCheckOnly);
                return false;
            }

            if (pOp2_3->GetOperator() != tk_question) {
                ParseMsg(PARSE_WARNING, pExpr->GetLineInfo(), "Unable to translate statement to Xilinx FD Primitive(s)");
                Assert(bCheckOnly);
                return false;
            }

            CHtvOperand *pExpr2_3 = pOp2_3;
            CHtvOperand *pOp2_3_1 = pExpr2_3->GetOperand1();
            CHtvOperand *pOp2_3_2 = pExpr2_3->GetOperand2();
            CHtvOperand *pOp2_3_3 = pExpr2_3->GetOperand3();

            // Operand 1 must be an identifier with width 1
            if (!pOp2_3_1->IsLeaf() || pOp2_3_1->GetMember() == 0) {
                ParseMsg(PARSE_WARNING, pExpr->GetLineInfo(), "Unable to translate statement to Xilinx FD Primitive(s)");
                Assert(bCheckOnly);
                return false;
            }

            // Operand 2 must be a constant of zero of all ones
            if (!pOp2_3_2->IsLeaf() || !pOp2_3_2->IsConstValue()) {
                ParseMsg(PARSE_WARNING, pExpr->GetLineInfo(), "Unable to translate statement to Xilinx FD Primitive(s)");
                Assert(bCheckOnly);
                return false;
            }

            constValue = pOp2_3_2->GetConstValue().GetUint64();
            uint64 constOnes = (uint64)0xffffffffffffffffLL >> (64 - pOp2_3_2->GetVerWidth());

            if (constValue != constOnes) {
                ParseMsg(PARSE_WARNING, pExpr->GetLineInfo(), "Unable to translate statement to Xilinx FD Primitive(s)");
                Assert(bCheckOnly);
                return false;
            }

            if (pOp2_3_3->IsLeaf() && pOp2_3_3->GetMember()) {
                if (bCheckOnly) {
                    pStatement->SetIsXilinxFdPrim();
                    pOp1->GetMember()->SetIsHtPrimOutput(pOp1);
                    return true;
                }

                // FDRS / FDRSE
                int op1BaseIdx = pOp1->IsSubFieldValid() ? GetSubFieldLowBit(pOp1) : 0;
                int op2_1BaseIdx = pOp2_1->IsSubFieldValid() ? GetSubFieldLowBit(pOp2_1) : 0;
                int op2_3_1BaseIdx = pOp2_3_1->IsSubFieldValid() ? GetSubFieldLowBit(pOp2_3_1) : 0;
                int op2_3_3BaseIdx = pOp2_3_3->IsSubFieldValid() ? GetSubFieldLowBit(pOp2_3_3) : 0;

                for (int i = 0; i < pExpr->GetVerWidth(); i += 1) {
                    if (pExpr->GetVerWidth() == 1 && !pOp1->IsSubFieldValid())
                        m_vFile.Print("%s %s_ ( ", pEnable ? "FDRSE" : "FDRS", pOp1->GetMember()->GetVerilogName().c_str(), op1BaseIdx+i);
                    else
                        m_vFile.Print("%s %s_%d_ ( ", pEnable ? "FDRSE" : "FDRS", pOp1->GetMember()->GetVerilogName().c_str(), op1BaseIdx+i);

                    if (pOp1->GetMember()->GetType()->GetWidth() == 1)
                        m_vFile.Print(".Q( %s ), ", pOp1->GetMember()->GetName(pOp1).c_str() );
                    else
                        m_vFile.Print(".Q( %s[%d] ), ", pOp1->GetMember()->GetName(pOp1).c_str(), op1BaseIdx+i );

                    if (pOp2_3_3->GetMember()->GetType()->GetWidth() == 1)
                        m_vFile.Print(".D( %s ), ", pOp2_3_3->GetMember()->GetName(pOp2_3_3).c_str() );
                    else
                        m_vFile.Print(".D( %s[%d] ), ", pOp2_3_3->GetMember()->GetName(pOp2_3_3).c_str(), op2_3_3BaseIdx+i );

                    if (pOp2_1->GetMember()->GetType()->GetWidth() == 1)
                        m_vFile.Print(".R( %s ), ", pOp2_1->GetMember()->GetName(pOp2_1).c_str() );
                    else
                        m_vFile.Print(".R( %s[%d] ), ", pOp2_1->GetMember()->GetName(pOp2_1).c_str(), op2_1BaseIdx );

                    if (pOp2_3_1->GetMember()->GetType()->GetWidth() == 1)
                        m_vFile.Print(".S( %s ), ", pOp2_3_1->GetMember()->GetName(pOp2_3_1).c_str() );
                    else
                        m_vFile.Print(".S( %s[%d] ), ", pOp2_3_1->GetMember()->GetName(pOp2_3_1).c_str(), op2_3_1BaseIdx );

                    if (pEnable) {
                        if (pEnable->IsSubFieldValid())
                            m_vFile.Print(".CE( %s[%d] ), ", pEnable->GetMember()->GetName(pEnable).c_str(), GetSubFieldLowBit(pEnable));
                        else
                            m_vFile.Print(".CE( %s ), ", pEnable->GetMember()->GetName(pEnable).c_str() );
                    }
                    m_vFile.Print(".C( %s ) );\n", pHier->GetClockIdent()->GetName().c_str());
                }
                return true;
            }
        }
    }

    ParseMsg(PARSE_WARNING, pExpr->GetLineInfo(), "Unable to translate statement to Xilinx FD Primitive(s)");
    Assert(bCheckOnly);
    return false;
}

bool CHtvDesign::IsConcatLeaves(CHtvOperand *pOperand)
{
    // a single leaf is considered a concatenated leaf
    if (pOperand->IsLeaf())
        return true;

    if (pOperand->GetOperator() == tk_comma)
        return IsConcatLeaves(pOperand->GetOperand1()) && IsConcatLeaves(pOperand->GetOperand2());

    return false;
}

string CHtvDesign::GetConcatLeafName(int exprIdx, CHtvOperand *pExpr, bool &bDimen, int &bitIdx)
{
    if (pExpr->IsLeaf()) {
        bDimen = pExpr->GetMember()->GetWidth() > 1;//GetDimenCnt() != 0;

        int baseIdx = pExpr->IsSubFieldValid() ? GetSubFieldLowBit(pExpr) : 0;

        bitIdx = baseIdx + exprIdx;
        return pExpr->GetMember()->GetName(pExpr);
    }

    Assert(pExpr->GetOperator() == tk_comma);
    if (exprIdx < pExpr->GetOperand2()->GetMinWidth())
        return GetConcatLeafName(exprIdx, pExpr->GetOperand2(), bDimen, bitIdx);
    else
        return GetConcatLeafName(exprIdx - pExpr->GetOperand2()->GetMinWidth(), pExpr->GetOperand1(), bDimen, bitIdx);
}

void CHtvDesign::GenFixtureFiles(CHtvIdent *pModule)
{
    GenRandomH();
    GenRandomCpp();
    GenScMainCpp();
    GenFixtureH(pModule);
    GenFixtureCpp(pModule);
    GenFixtureModelH(pModule);
    GenFixtureModelCpp(pModule);
    GenSandboxVpp(pModule);
}

FILE * CHtvDesign::BackupAndOpen(const string &fileName)
{
    // first backup old file
    string originalName = m_pathName + fileName;
    string backupName = m_backupPath + fileName;
    rename(originalName.c_str(), backupName.c_str());

    FILE *fp;
    if (!(fp = fopen(originalName.c_str(), "w"))) {
        printf("Could not open '%s' for writing\n", originalName.c_str());
        ErrorExit();
    }
    return fp;
}

void CHtvDesign::PrintHeader(FILE *fp, char *pComment)
{
    fprintf(fp, "// %s\n", pComment);
    fprintf(fp, "//\n");
    fprintf(fp, "//////////////////////////////////////////////////////////////////////\n");
    fprintf(fp, "\n");
}

void CHtvDesign::GenRandomH()
{
    string fileName = m_pathName + "Random.h";
    FILE *fp;
    if (fp = fopen(fileName.c_str(), "r")) {
        fclose(fp);
        return;
    }

    if (!(fp = fopen(fileName.c_str(), "w"))) {
        printf("Could not open '%s' for writing\n", fileName.c_str());
        ErrorExit();
    }

    PrintHeader(fp, "Random.h: Header for random number generator");

    fprintf(fp, "#pragma once\n");
    fprintf(fp, "\n");
    fprintf(fp, "class CRandom;\n");
    fprintf(fp, "\n");
    fprintf(fp, "class CRndGen\n");
    fprintf(fp, "{\n");
    fprintf(fp, "public:\n");
    fprintf(fp, "\tCRndGen(unsigned long init=1);\n");
    fprintf(fp, "\tvirtual ~CRndGen();\n");
    fprintf(fp, "\n");
    fprintf(fp, "\tuint32_t AsUint32();\n");
    fprintf(fp, "\tuint64_t AsUint64() { return ((uint64_t)AsUint32() << 32) | AsUint32(); }\n");
    fprintf(fp, "\tint64_t AsSint64() { return (int64_t)(((uint64_t)AsUint32() << 32) | AsUint32()); }\n");
    fprintf(fp, "\tdouble AsDouble();\n");
    fprintf(fp, "\tfloat AsFloat();\n");
    fprintf(fp, "\tunsigned int GetSeed() { return initialSeed; }\n");
    fprintf(fp, "\tvoid Seed(unsigned int init);\n");
    fprintf(fp, "\n");
    fprintf(fp, "private:\n");
    fprintf(fp, "\tvoid ResetRndGen();\n");
    fprintf(fp, "\n");
    fprintf(fp, "private:\n");
    fprintf(fp, "\tunion PrivateDoubleType {	   	// used to access doubles as unsigneds\n");
    fprintf(fp, "\t\tdouble d;\n");
    fprintf(fp, "\t\tuint32_t u[2];\n");
    fprintf(fp, "\t};\n");
    fprintf(fp, "\tPrivateDoubleType doubleMantissa;\n");
    fprintf(fp, "\n");
    fprintf(fp, "\tunion PrivateFloatType {		// used to access floats as unsigneds\n");
    fprintf(fp, "\t\tfloat f;\n");
    fprintf(fp, "\t\tunsigned long u;\n");
    fprintf(fp, "\t};\n");
    fprintf(fp, "\tPrivateFloatType floatMantissa;\n");
    fprintf(fp, "\n");
    fprintf(fp, "\t// struct defines a knot of the circular queue\n");
    fprintf(fp, "\t// used by the lagged Fibonacci algorithm\n");
    fprintf(fp, "\tstruct Vknot {\n");
    fprintf(fp, "\t\tVknot() { v = 0; vnext = 0; }\n");
    fprintf(fp, "\t\tunsigned long v;\n");
    fprintf(fp, "\t\tVknot *vnext;\n");
    fprintf(fp, "\t} m_Vknot[97];\n");
    fprintf(fp, "\tunsigned long initialSeed;  // start value of initialization routine\n");
    fprintf(fp, "\tunsigned long Xn;		// generated value\n");
    fprintf(fp, "\tunsigned long cn, Vn;	// temporary values\n");
    fprintf(fp, "\tVknot *V33, *V97, *Vanker;	// some pointers to the queue\n");
    fprintf(fp, "};\n");
    fprintf(fp, "\n");
    fprintf(fp, "class CRandom\n");
    fprintf(fp, "{\n");
    fprintf(fp, "\tfriend class CRndGen;\n");
    fprintf(fp, "public:\n");
    fprintf(fp, "\tCRandom() {}\n");
    fprintf(fp, "\t//	~CRandom() {};\n");
    fprintf(fp, "\tvoid Seed(unsigned int init) { m_rndGen.Seed(init); }\n");
    fprintf(fp, "\n");
    fprintf(fp, "\tbool RndBool() { return (m_rndGen.AsUint32() & 1) == 1; }\n");
    fprintf(fp, "\n");
    fprintf(fp, "\tint32_t RndSint32() { return m_rndGen.AsUint32(); }\n");
    fprintf(fp, "\n");
    fprintf(fp, "\tuint32_t RndUint32() { return m_rndGen.AsUint32(); }\n");
    fprintf(fp, "\n");
    fprintf(fp, "\tuint32_t RndUint32(uint32_t low, uint32_t high)\n");
    fprintf(fp, "\t{ return (m_rndGen.AsUint32() %% (high-low+1)) + low; }\n");
    fprintf(fp, "\n");
    fprintf(fp, "\tuint64_t RndSint64(int64_t low, int64_t high)\n");
    fprintf(fp, "\t{ return (m_rndGen.AsUint64() %% (high-low+1)) + low; }\n");
    fprintf(fp, "\n");
    fprintf(fp, "\tuint32_t RndSint32(int low, int high)\n");
    fprintf(fp, "\t{ return (m_rndGen.AsUint32() %% (high-low+1)) + low; }\n");
    fprintf(fp, "\n");
    fprintf(fp, "\tuint64_t RndUint64() { return m_rndGen.AsUint64(); }\n");
    fprintf(fp, "\n");
    fprintf(fp, "\tuint64_t RndUint64(uint64_t low, uint64_t high)\n");
    fprintf(fp, "\t{ return (m_rndGen.AsUint64() %% (high-low+1)) + low; }\n");
    fprintf(fp, "\n");
    fprintf(fp, "\tdouble RndUniform(double pLow=0.0, double pHigh=1.0)\n");
    fprintf(fp, "\t{ return pLow + (pHigh - pLow) * m_rndGen.AsDouble(); }\n");
    fprintf(fp, "\n");
    fprintf(fp, "\tfloat RndUniform(float pLow, float pHigh=1.0)\n");
    fprintf(fp, "\t{ return pLow + (pHigh - pLow) * m_rndGen.AsFloat(); }\n");
    fprintf(fp, "\n");
    fprintf(fp, "\tdouble RndPercent() { return RndUniform(0.0, 99.999999999); }\n");
    fprintf(fp, "\n");
    fprintf(fp, "\t//	double RndNegExp(double mean)\n");
    fprintf(fp, "\t//	{ return(-mean * log(m_rndGen.AsDouble())); }\n");
    fprintf(fp, "\n");
    fprintf(fp, "private:\n");
    fprintf(fp, "\tCRndGen m_rndGen;\n");
    fprintf(fp, "};\n");
}

void CHtvDesign::GenRandomCpp()
{
    string fileName = m_pathName + "Random.cpp";
    FILE *fp;
    if (fp = fopen(fileName.c_str(), "r")) {
        fclose(fp);
        return;
    }

    if (!(fp = fopen(fileName.c_str(), "w"))) {
        printf("Could not open '%s' for writing\n", fileName.c_str());
        ErrorExit();
    }

    PrintHeader(fp, "Random.cpp: Random number generator for fixture");

    fprintf(fp, "#include <stdint.h>\n");
    fprintf(fp, "#include \"Random.h\"\n");
    fprintf(fp, "\n");
    fprintf(fp, "//////////////////////////////////////////////////////////////////////\n");
    fprintf(fp, "// Construction/Destruction\n");
    fprintf(fp, "//////////////////////////////////////////////////////////////////////\n");
    fprintf(fp, "\n");
    fprintf(fp, "CRndGen::CRndGen(unsigned long init)\n");
    fprintf(fp, "{\n");
    fprintf(fp, "\t// create a 97 element circular queue\n");
    fprintf(fp, "\tVknot *Vhilf;\n");
    fprintf(fp, "\tVanker = V97 = Vhilf = &m_Vknot[0];\n");
    fprintf(fp, "\n");
    fprintf(fp, "\tfor( short i=96; i>0; i-- ) {\n");
    fprintf(fp, "\t\tVhilf->vnext = &m_Vknot[i];\n");
    fprintf(fp, "\t\tVhilf = Vhilf->vnext;\n");
    fprintf(fp, "\t\tif (i == 33)\n");
    fprintf(fp, "\t\t\tV33 = Vhilf;\n");
    fprintf(fp, "\t}\n");
    fprintf(fp, "\n");
    fprintf(fp, "\tVhilf->vnext = Vanker;\n");
    fprintf(fp, "\n");
    fprintf(fp, "\tinitialSeed = init;\n");
    fprintf(fp, "\n");
    fprintf(fp, "\tResetRndGen();\n");
    fprintf(fp, "\n");
    fprintf(fp, "\t// initialize mantissa to all ones\n");
    fprintf(fp, "\tdoubleMantissa.d = 1.0;\n");
    fprintf(fp, "\tdoubleMantissa.u[0] ^= 0xffffffff;\n");
    fprintf(fp, "\tdoubleMantissa.u[1] ^= 0x3fffffff;\n");
    fprintf(fp, "\n");
    fprintf(fp, "\t// initialize mantissa to all ones\n");
    fprintf(fp, "\tfloatMantissa.f = 1.0;\n");
    fprintf(fp, "\tfloatMantissa.u ^= 0x3fffffff;\n");
    fprintf(fp, "}\n");
    fprintf(fp, "\n");
    fprintf(fp, "CRndGen::~CRndGen()\n");
    fprintf(fp, "{\n");
    fprintf(fp, "}\n");
    fprintf(fp, "\n");
    fprintf(fp, "void\n");
    fprintf(fp, "CRndGen::Seed(unsigned int init)\n");
    fprintf(fp, "{\n");
    fprintf(fp, "\tinitialSeed = init;\n");
    fprintf(fp, "\n");
    fprintf(fp, "\tResetRndGen();\n");
    fprintf(fp, "}\n");
    fprintf(fp, "\n");
    fprintf(fp, "void\n");
    fprintf(fp, "CRndGen::ResetRndGen()\n");
    fprintf(fp, "{\n");
    fprintf(fp, "\t// initialise FIBOG like RANMAR (or IMAV)\n");
    fprintf(fp, "\tVknot *Vhilf = V97;\n");
    fprintf(fp, "\n");
    fprintf(fp, "\tlong ijkl = 54217137;\n");
    fprintf(fp, "\tif (initialSeed >0 && initialSeed <= 0x7fffffff) ijkl = initialSeed;\n");
    fprintf(fp, "\tlong ij = ijkl / 30082;\n");
    fprintf(fp, "\tlong kl = ijkl - 30082 * ij;\n");
    fprintf(fp, "\tlong i = (ij/177) %% 177 + 2;\n");
    fprintf(fp, "\tlong j = ij %% 177 + 2;\n");
    fprintf(fp, "\tlong k = (kl/169) %% 178 + 1;\n");
    fprintf(fp, "\tlong l = kl %% 169;\n");
    fprintf(fp, "\tlong s, t;\n");
    fprintf(fp, "\tfor( int ii=0; ii<97; ii++) {\n");
    fprintf(fp, "\t\ts = 0; t = 0x80000000;\n");
    fprintf(fp, "\t\tfor( int jj=0; jj<32; jj++) {\n");
    fprintf(fp, "\t\t\tlong m = (((i * j) %% 179) * k) %% 179;\n");
    fprintf(fp, "\t\t\ti = j; j = k; k = m;\n");
    fprintf(fp, "\t\t\tl = (53 * l + 1) %% 169;\n");
    fprintf(fp, "\t\t\tif ((l * m) %% 64 >= 32) s = s + t;\n");
    fprintf(fp, "\t\t\tt /= 2;\n");
    fprintf(fp, "\t\t}\n");
    fprintf(fp, "\n");
    fprintf(fp, "\t\tVhilf->v = s;\n");
    fprintf(fp, "\t\tVhilf = Vhilf->vnext;\n");
    fprintf(fp, "\t}\n");
    fprintf(fp, "\n");
    fprintf(fp, "\tcn = 15362436;\n");
    fprintf(fp, "}\n");
    fprintf(fp, "\n");
    fprintf(fp, "uint32_t\n");
    fprintf(fp, "CRndGen::AsUint32()\n");
    fprintf(fp, "{\n");
    fprintf(fp, "\tVn = (V97->v - V33->v) & 0xffffffff;	// (x & 2^32) might be faster  \n");
    fprintf(fp, "\tcn = (cn + 362436069)  & 0xffffffff;	// than (x %% 2^32)\n");
    fprintf(fp, "\tXn = (Vn - cn)         & 0xffffffff;\n");
    fprintf(fp, "\n");
    fprintf(fp, "\tV97->v = Vn;\n");
    fprintf(fp, "\tV97 = V97->vnext;\n");
    fprintf(fp, "\tV33 = V33->vnext;\n");
    fprintf(fp, "\n");
    fprintf(fp, "\treturn Xn;\n");
    fprintf(fp, "}\n");
    fprintf(fp, "\n");
    fprintf(fp, "double\n");
    fprintf(fp, "CRndGen::AsDouble()\n");
    fprintf(fp, "{\n");
    fprintf(fp, "\tPrivateDoubleType result;\n");
    fprintf(fp, "\tresult.d = 1.0;\n");
    fprintf(fp, "\tresult.u[1] |= (AsUint32() & doubleMantissa.u[1]);\n");
    fprintf(fp, "\tresult.u[0] |= (AsUint32() & doubleMantissa.u[0]);\n");
    fprintf(fp, "\tresult.d -= 1.0;\n");
    fprintf(fp, "\treturn( result.d );\n");
    fprintf(fp, "}\n");
    fprintf(fp, "\n");
    fprintf(fp, "float\n");
    fprintf(fp, "CRndGen::AsFloat()\n");
    fprintf(fp, "{\n");
    fprintf(fp, "\tPrivateFloatType result;\n");
    fprintf(fp, "\tresult.f = 1.0;\n");
    fprintf(fp, "\tresult.u |= (AsUint32() & floatMantissa.u);\n");
    fprintf(fp, "\tresult.f -= 1.0;\n");
    fprintf(fp, "\treturn( result.f );\n");
    fprintf(fp, "}\n");

    fclose(fp);
}

void CHtvDesign::GenScMainCpp()
{
    string fileName = m_pathName + "ScMain.cpp";
    FILE *fp;
    if (fp = fopen(fileName.c_str(), "r")) {
        fclose(fp);
        return;
    }

    if (!(fp = fopen(fileName.c_str(), "w"))) {
        printf("Could not open '%s' for writing\n", fileName.c_str());
        ErrorExit();
    }

    PrintHeader(fp, "ScMain.cpp: Entry point for program");

    fprintf(fp, "#include <stdint.h>\n");
    fprintf(fp, "#include \"FixtureTop.h\"\n");
    fprintf(fp, "\n");
    fprintf(fp, "int\n");
    fprintf(fp, "sc_main(int argc, char *argv[])\n");
    fprintf(fp, "{\n");
    fprintf(fp, "\tCFixtureTop FixtureTop(\"FixtureTop\");\n");
    fprintf(fp, "\n");
    fprintf(fp, "\tsc_start();\n");
    fprintf(fp, "\n");
    fprintf(fp, "\treturn 0;\n");
    fprintf(fp, "}\n");

    fclose(fp);
}

void CHtvDesign::GenFixtureH(CHtvIdent *pModule)
{
    FILE *fp;
    string readName = m_pathName + "Fixture.h";

    vector<string>	userLines;
    int userLinesCnt[2] = { 0, 0 };

    int regionCnt = 0;

    if (fp = fopen(readName.c_str(), "r")) {
        char lineBuf[512];
        while (regionCnt < 2 && fgets(lineBuf, 512, fp)) {
            if (strncmp(lineBuf, "//>>", 4) == 0) {
                while (fgets(lineBuf, 512, fp)) {
                    if (strncmp(lineBuf, "//<<", 4) == 0) {
                        userLinesCnt[regionCnt++] = userLines.size();
                        break;
                    }
                    userLines.push_back(lineBuf);
                }
            }
        }
        fclose(fp);
    }

    CFileBkup fb(m_pathName, "Fixture", "h");

    fb.PrintHeader("Fixture.h: Top level fixture include file");

    fb.Print("#pragma once\n");
    fb.Print("\n");
    fb.Print("#ifdef __SYSTEM_C__\n");
    fb.Print("#include <SystemC.h>\n");
    fb.Print("#else\n");
    fb.Print("#include \"acc_user.h\"\n");
    fb.Print("extern \"C\" int InitFixture();\n");
    fb.Print("extern \"C\" int Fixture();\n");
    fb.Print("#endif\n");
    fb.Print("\n");
    fb.Print("#ifndef _SC_LINT\n");
    fb.Print("#include <time.h>\n");
    fb.Print("#include <stdint.h>\n");
    fb.Print("#include \"Random.h\"\n");
    fb.Print("#include \"FixtureModel.h\"\n");
    fb.Print("\n");
    fb.Print("extern CRandom cnyRnd;\n");
    fb.Print("\n");

    fb.Print("struct CFixtureTest {\n");
    fb.Print("\tCFixtureTest() { }\n");
    fb.Print("\tCFixtureTest(uint64_t clkCnt, uint64_t testCnt) : m_clkCnt(clkCnt), m_testCnt(testCnt) { }\n");
    fb.Print("\n");

    fb.Print("\tCFixtureTest &Test(");

    CHtvIdentTblIter identIter(pModule->GetIdentTbl());

    int cnt = 0;
    if (pModule->GetClockListCnt() > 1) {
        cnt += 1;
        string clkName = pModule->GetClockList()[1]->GetName();
        fb.Print("uint32_t phase%sCnt", clkName.c_str()+3);
    }

    for (identIter.Begin(); !identIter.End(); identIter++) {
        if (identIter->IsOutPort() && !identIter->IsClock()) {

            if (cnt > 0)
                fb.Print((cnt % 5 != 0) ? ", " : ",\n\t\t\t\t");
            cnt += 1;

            const string portName = identIter->GetName();
            if (identIter->GetType()->GetType()->IsStruct())
                fb.Print("%s %s", identIter->GetType()->GetType()->GetName().c_str(), portName.c_str());
            else
                fb.Print("uint%d_t %s", identIter->GetWidth() <= 32 ? 32 : 64, portName.c_str());
        }
    }

    fb.Print(") {\n");
    fb.Print("\n");

    fb.Print("\t\tm_pModel->Model(");

    cnt = 0;
    if (pModule->GetClockListCnt() > 1) {
        cnt += 1;
        string clkName = pModule->GetClockList()[1]->GetName();
        fb.Print("phase%sCnt", clkName.c_str()+3);
    }

    for (identIter.Begin(); !identIter.End(); identIter++) {
        if (identIter->IsOutPort() && !identIter->IsClock()) {

            if (cnt > 0)
                fb.Print((cnt % 8 != 0) ? ", " : ",\n\t\t\t\t");
            cnt += 1;

            const string portName = identIter->GetName();
            fb.Print("%s", portName.c_str());
        }
    }

    for (identIter.Begin(); !identIter.End(); identIter++) {
        if (identIter->IsInPort() && !identIter->IsClock()) {

            if (cnt > 0)
                fb.Print((cnt % 8 != 0) ? ", " : ",\n\t\t\t\t");
            cnt += 1;

            const string portName = identIter->GetName();
            fb.Print("m_%s", portName.c_str());
        }
    }
    fb.Print(");\n");
    fb.Print("\n");
    for (identIter.Begin(); !identIter.End(); identIter++) {
        if (identIter->IsOutPort() && !identIter->IsClock()) {

            const string portName = identIter->GetName();
            fb.Print("\t\tm_%s = %s;\n", portName.c_str(), portName.c_str());
        }
    }

    fb.Print("\n");
    fb.Print("\t\treturn *this;\n");
    fb.Print("\t}\n");

    fb.Print("\tCFixtureTest & End() {\n");
    fb.Print("\t\tm_testCnt = 0xffffffffffffffffLL;\n");
    fb.Print("\t\treturn *this;\n");
    fb.Print("\t}\n");
    fb.Print("\n");
    fb.Print("\tbool IsEnd() { return m_testCnt == 0xffffffffffffffffLL; }\n");
    fb.Print("\n");
    fb.Print("static void SetModel(CFixtureModel *pModel) { m_pModel = pModel; }\n");
    fb.Print("\n");
    fb.Print("\tuint64_t    \tm_clkCnt;\n");
    fb.Print("\tuint64_t    \tm_testCnt;\n");

    for (identIter.Begin(); !identIter.End(); identIter++) {
        if ((identIter->IsOutPort() || identIter->IsInPort()) && !identIter->IsClock()) {

            char typeName[128];
            if (identIter->GetType()->GetType()->IsStruct())
                sprintf(typeName, "%s", identIter->GetType()->GetType()->GetName().c_str());
            else
                sprintf(typeName, "uint%d_t", identIter->GetWidth() <= 32 ? 32 : 64);

            const string portName = identIter->GetName();
            fb.Print("\t%-12s\tm_%s;\n", typeName, portName.c_str());
        }
    }
    fb.Print("\n");
    fb.Print("\tstatic CFixtureModel *\tm_pModel;\n");
    fb.Print("};\n");
    fb.Print("\n");
    fb.Print("#include <queue>\n");
    fb.Print("using namespace std;\n");
    fb.Print("\n");
    fb.Print("typedef queue<CFixtureTest>  CTestQue;\n");
    fb.Print("\n");
    fb.Print("#define TEST_CNT	1000000000000LL\n");
    fb.Print("#define sc_fixture\n");
    fb.Print("#define sc_fixture_top\n");
    fb.Print("\n");
    fb.Print("#endif\n");
    fb.Print("\n");
    fb.Print("#ifdef __SYSTEM_C__\n");
    fb.Print("sc_fixture\n");
    fb.Print("SC_MODULE(CFixture) {\n");

    if (pModule->GetClockList().size() == 0) {
        fprintf(stderr, "Clock signal not found for generating 'Fixture.h'\n");
        ErrorExit();
    }

    fb.Print("\tsc_in<bool>         \ti_%s;\n", pModule->GetClockList()[0]->GetName().c_str());
    fb.Print("\n");
    for (identIter.Begin(); !identIter.End(); identIter++) {
        if (identIter->IsOutPort() || identIter->IsInPort()) {
            const string portName = identIter->GetName();

            char typeName[128];
            if (identIter->IsInPort())
                sprintf(typeName, "sc_in<%s>", identIter->GetType()->GetType()->GetName().c_str());
            else
                sprintf(typeName, "sc_out<%s>", identIter->GetType()->GetType()->GetName().c_str());

            if (identIter->IsInPort())
                fb.Print("\t%-20s \ti_%s;\n", typeName, portName.c_str());
            else
                fb.Print("\t%-20s \to_%s;\n", typeName, portName.c_str());
        }
    }
    fb.Print("\n");
    fb.Print("#else // __SYSTEM_C__\n");
    fb.Print("class CFixture {\n");
    fb.Print("public:\n");
    fb.Print("#endif // __SYSTEM_C__\n");
    fb.Print("\n");
    fb.Print("#ifndef _SC_LINT\n");
    fb.Print("\n");
    fb.Print("\tCFixtureModel\t\t\tm_model;\n");
    fb.Print("\tvector<CFixtureTest>\tm_directedTests;\n");
    fb.Print("\n");
    fb.Print("\tuint64_t	r_clkCnt;\n");
    fb.Print("\tint		r_inCnt;\n");
    fb.Print("\tint		r_outCnt;\n");
    fb.Print("\tuint64_t	r_testCnt;\n");
    fb.Print("\n");
    fb.Print("\ttime_t	startTime;\n");
    fb.Print("\n");
    fb.Print("\tCTestQue	m_testQue;\n");
    fb.Print("\n");
    fb.Print("\tCRndGen\tm_rndGen;\n");
    fb.Print("\tint\t\tm_randomSeed;\n");
    fb.Print("\n");
    if (pModule->GetClockListCnt() > 1)
        fb.Print("\tuint32_t\tr_phase%sCnt;\n", pModule->GetClockList()[1]->GetName().c_str()+3);
    if (pModule->GetClockListCnt() > 2)
        fb.Print("\tbool\t\tr_phase%s;\n", pModule->GetClockList()[2]->GetName().c_str()+3);

    for (identIter.Begin(); !identIter.End(); identIter++) {
        if (identIter->IsInPort() && !identIter->IsClock()) {
            const string portName = identIter->GetName();

            fb.Print("\tuint32_t\tm_stage_%s;\n", portName.c_str());
        }
    }

    fb.Print("\n");
    fb.Print("//////////////////////////////////////\n");
    fb.Print("// Fixture specific declarations\n");
    fb.Print("//>>\n");
    if (regionCnt > 0) {
        for (int i = 0; i < userLinesCnt[0]; i += 1)
            fb.Write(userLines[i].c_str());
    }
    fb.Print("//<<\n");
    fb.Print("// Fixture specific declarations\n");
    fb.Print("//////////////////////////////////////\n");
    fb.Print("\n");
    fb.Print("#endif\n");
    fb.Print("\n");
    fb.Print("\tvoid Fixture();\n");
    fb.Print("\tvoid DelayEarlyOutputs();\n");
    fb.Print("\tvoid GetOutputs(");

    cnt = 0;
    for (identIter.Begin(); !identIter.End(); identIter++) {
        if (identIter->IsInPort() && !identIter->IsClock()) {
            const string portName = identIter->GetName();

            if (cnt > 0)
                fb.Print((cnt % 5 != 0) ? ", " : ",\n\t\t\t\t");
            cnt += 1;

            fb.Print("uint%d_t &%s", identIter->GetWidth() <= 32 ? 32 : 64, portName.c_str());
        }
    }

    fb.Print(");\n");
    fb.Print("\tvoid GenRandomInputs(");

    cnt = 0;
    for (identIter.Begin(); !identIter.End(); identIter++) {
        if (identIter->IsOutPort() && !identIter->IsClock()) {
            const string portName = identIter->GetName();

            if (cnt > 0)
                fb.Print((cnt % 5 != 0) ? ", " : ",\n\t\t\t\t");
            cnt += 1;

            if (identIter->GetType()->GetType()->IsStruct())
                fb.Print("%s &%s", identIter->GetType()->GetType()->GetName().c_str(), portName.c_str());
            else
                fb.Print("uint%d_t &%s", identIter->GetWidth() <= 32 ? 32 : 64, portName.c_str());
        }
    }

    fb.Print(");\n");
    fb.Print("\n");
    fb.Print("#ifndef _SC_LINT\n");
    fb.Print("\tvoid InitTest();\n");
    fb.Print("#endif\n");
    fb.Print("\n");
    fb.Print("\t// Constructor\n");
    fb.Print("#ifdef __SYSTEM_C__\n");
    fb.Print("\t#ifndef _SC_LINT\n");
    fb.Print("\t\tsc_clock	*pClock;\n");
    fb.Print("\t\tvoid Clock() {\n");
    fb.Print("\t\t\tbool %s = *pClock;\n", pModule->GetClockList()[0]->GetName().c_str());
    string clkName;
    string clkhxName;
    if (pModule->GetClockListCnt() > 1) {
        clkName = pModule->GetClockList()[1]->GetName();
        fb.Print("\t\t\tif (%s) {\n", pModule->GetClockList()[0]->GetName().c_str());
        fb.Print("\t\t\t\tr_phase%sCnt = (r_phase%sCnt + 1) & PHASE_CNT_MASK;\n", clkName.c_str()+3, clkName.c_str()+3);
        if (pModule->GetClockListCnt() > 2) {
            clkhxName = pModule->GetClockList()[2]->GetName();
            fb.Print("\t\t\t\tif (r_phase%sCnt == 0)\n", clkName.c_str()+3);
            fb.Print("\t\t\t\t\tr_phase%s = !r_phase%s;\n", clkhxName.c_str()+3, clkhxName.c_str()+3);
        }
        fb.Print("\t\t\t}\n");
    }
    fb.Print("\t\t\to_%s = %s;\n", pModule->GetClockList()[0]->GetName().c_str(),
        pModule->GetClockList()[0]->GetName().c_str());
    if (pModule->GetClockListCnt() > 1)
        fb.Print("\t\t\to_%s = r_phase%sCnt == 0;\n", pModule->GetClockList()[1]->GetName().c_str(),
        clkName.c_str()+3);
    if (pModule->GetClockListCnt() > 2)
        fb.Print("\t\t\to_%s = r_phase%s;\n", pModule->GetClockList()[2]->GetName().c_str(),
        clkhxName.c_str()+3);
    fb.Print("\t\t}\n");

    fb.Print("\t#endif\n");
    fb.Print("\n");
    fb.Print("\tSC_CTOR(CFixture) {\n");
    fb.Print("\t\tSC_METHOD(Fixture);\n");
    fb.Print("\t\tsensitive << i_%s.neg();\n", pModule->GetClockList()[0]->GetName().c_str());
    fb.Print("\n");
    fb.Print("#ifndef _SC_LINT\n");
    fb.Print("\n");
    fb.Print("\tpClock = new sc_clock(\"%s\", sc_time(10.0, SC_NS), 0.5, SC_ZERO_TIME, true);\n",
        pModule->GetClockList()[0]->GetName().c_str());

    fb.Print("\n");
    fb.Print("\tSC_METHOD(Clock);\n");
    fb.Print("\tsensitive << *pClock;\n");
    fb.Print("\n");
    fb.Print("\t#endif // _SC_LINT\n");
    fb.Print("#else // __SYSTEM_C__\n");
    fb.Print("\tCFixture() {\n");
    fb.Print("#endif//  __SYSTEM_C__\n");
    fb.Print("\n");
    fb.Print("\t#ifndef _SC_LINT\n");
    fb.Print("\n");
    fb.Print("\tCFixtureTest::SetModel(&m_model);\n");
    fb.Print("\n");
    if (pModule->GetClockListCnt() > 1) {
        string clkName = pModule->GetClockList()[1]->GetName();
        fb.Print("\tr_phase%sCnt = 0;\n", clkName.c_str()+3);
    }
    fb.Print("\tr_clkCnt = 0;\n");
    fb.Print("\tr_inCnt = 0;\n");
    fb.Print("\tr_outCnt = 0;\n");
    fb.Print("\tr_testCnt = 0;\n");
    fb.Print("\n");
    fb.Print("\ttime(&startTime);\n");
    fb.Print("\n");
    fb.Print("\tInitTest();\n");
    fb.Print("\n");
    fb.Print("///////////////////////////////////////////\n");
    fb.Print("// Fixture specific state initialization\n");
    fb.Print("//>>\n");
    if (regionCnt > 1) {
        for (int i = userLinesCnt[0]; i < userLinesCnt[1]; i += 1)
            fb.Write(userLines[i].c_str());
    }
    fb.Print("//<<\n");
    fb.Print("// Fixture specific state initialization\n");
    fb.Print("///////////////////////////////////////////\n");
    fb.Print("\n");
    fb.Print("#endif\n");
    fb.Print("}\n");
    fb.Print("\n");
    fb.Print("#ifndef _SC_LINT\n");
    fb.Print("\t// I/O access routines\n");
    fb.Print("\tbool ClockFallingEdge() {\n");
    fb.Print("\t\t#ifdef __SYSTEM_C__\n");
    fb.Print("\t\t\treturn true;\n");
    fb.Print("\t\t#else\n");
    fb.Print("\t\t\tp_acc_value pAccValue;\n");
    fb.Print("\t\t\tchar *pCk = acc_fetch_value(sig.m_ck, \"%%x\", pAccValue);\n");
    fb.Print("\t\t\treturn *pCk == '0';\n");
    fb.Print("\t\t#endif\n");
    fb.Print("\t}\n");
    fb.Print("\t\n");

    for (identIter.Begin(); !identIter.End(); identIter++) {
        const string portName = identIter->GetName();

        if (identIter->IsOutPort() && !identIter->IsClock()) {
            int portWidth = identIter->GetWidth();


            if (identIter->GetType()->GetType()->IsStruct())
                fb.Print("\tvoid SetSig_%s(%s %s) {\n", portName.c_str(),
                identIter->GetType()->GetType()->GetName().c_str(), portName.c_str());
            else
                fb.Print("\tvoid SetSig_%s(uint64_t %s) {\n", portName.c_str(), portName.c_str());
            fb.Print("\t\t#ifdef __SYSTEM_C__\n");
            fb.Print("\t\t\to_%s = %s;\n", portName.c_str(), portName.c_str());
            fb.Print("\t\t#else\n");
            if (portWidth > 32) {
                fb.Print("\t\t\tSetSigValue(sig.m_%sU, (uint32_t)(%s >> 32));\n", portName.c_str(), portName.c_str());
                fb.Print("\t\t\tSetSigValue(sig.m_%sL, (uint32_t)%s);\n", portName.c_str(), portName.c_str());
            } else {
                fb.Print("\t\t\tSetSigValue(sig.m_%s, %s);\n", portName.c_str(), portName.c_str());
            }
            fb.Print("\t\t#endif\n");
            fb.Print("\t}\n");

        } else if (identIter->IsInPort() && !identIter->IsClock()) {
            int portWidth = identIter->GetWidth();

            fb.Print("\tuint%d_t GetSig_%s() {\n", portWidth <= 32 ? 32 : 64, portName.c_str());
            fb.Print("\t\t#ifdef __SYSTEM_C__\n");
            if (identIter->GetType()->GetType()->IsStruct())
                fb.Print("\t\t\treturn i_%s.read();\n", portName.c_str());
            else
                fb.Print("\t\t\treturn (uint%d_t)i_%s.read();\n", portWidth <= 32 ? 32 : 64, portName.c_str());
            fb.Print("\t\t#else\n");
            if (portWidth > 32)
                fb.Print("\t\t\treturn ((uint64_t)GetSigValue(sig.m_%sU, \"%sU\") << 32) | GetSigValue(sig.m_%sL, \"%sL\");\n",
                portName.c_str(), portName.c_str(), portName.c_str(), portName.c_str());
            else
                fb.Print("\t\t\treturn GetSigValue(sig.m_%s, \"%s\");\n", portName.c_str(), portName.c_str());
            fb.Print("\t\t#endif\n");
            fb.Print("\t}\n");
        }
    }
    fb.Print("\n");
    fb.Print("\t#ifndef __SYSTEM_C__\n");
    fb.Print("\n");
    fb.Print("\t\tstruct CHandles {\n");

    fb.Print("\t\t\thandle m_ck;\n");
    for (identIter.Begin(); !identIter.End(); identIter++) {
        if (identIter->IsOutPort() || identIter->IsInPort()) {
            const string portName = identIter->GetName();
            int portWidth = identIter->GetWidth();

            if (portWidth <= 32)
                fb.Print("\t\t\thandle m_%s;\n", portName.c_str());
            else {
                fb.Print("\t\t\thandle m_%sU;\n", portName.c_str());
                fb.Print("\t\t\thandle m_%sL;\n", portName.c_str());
            }
        }
    }
    fb.Print("\t\t} sig;\n");
    fb.Print("\n");
    fb.Print("\t\tvoid SetSigValue(handle sig, uint32_t value) {\n");
    fb.Print("\t\t\ts_setval_value value_s;\n");
    fb.Print("\t\t\tvalue_s.format = accIntVal;\n");
    fb.Print("\t\t\tvalue_s.value.integer = value;\n");
    fb.Print("\n");
    fb.Print("\t\t\ts_setval_delay delay_s;\n");
    fb.Print("\t\t\tdelay_s.model = accNoDelay;\n");
    fb.Print("\t\t\tdelay_s.time.type = accTime;\n");
    fb.Print("\t\t\tdelay_s.time.low = 0;\n");
    fb.Print("\t\t\tdelay_s.time.high = 0;\n");
    fb.Print("\n");
    fb.Print("\t\t\tacc_set_value(sig, &value_s, &delay_s);\n");
    fb.Print("\t\t}\n");
    fb.Print("\n");
    fb.Print("\t\tuint32_t GetSigValue(handle sig, char *pName) {\n");
    fb.Print("\t\t\tp_acc_value pAccValue;\n");
    fb.Print("\t\t\tconst char *pNum = acc_fetch_value(sig, \"%%x\", pAccValue);\n");
    fb.Print("\t\t\tconst char *pNum2 = pNum;\n");
    fb.Print("\n");
    fb.Print("\t\t\tuint32_t value = 0;\n");
    fb.Print("\t\t\twhile (isdigit(*pNum) || tolower(*pNum) >= 'a' && tolower(*pNum) <= 'f')\n");
    fb.Print("\t\t\t\tvalue = value * 16 + (isdigit(*pNum) ? (*pNum++ - '0') : (tolower(*pNum++) - 'a' + 10));\n");
    fb.Print("\n");
    fb.Print("\t\t\tif (*pNum != '\\0')\n");
    fb.Print("\t\t\t\tprintf(\"  Signal '%%s' had value '%%s'\\n\", pName, pNum2);\n");
    fb.Print("\n");
    fb.Print("\t\t\treturn value;\n");
    fb.Print("\t\t}\n");
    fb.Print("\n");
    fb.Print("\t\tvoid InitFixture() {\n");
    fb.Print("\t\t\tacc_initialize();\n");
    fb.Print("\t\t\tint i = 1;\n");

    bool bFoundCk = false;
    for (identIter.Begin(); !identIter.End(); identIter++) {
        if (identIter->IsInPort()) {
            const string portName = identIter->GetName();

            int portWidth = identIter->GetWidth();

            if (strcmp(portName.c_str(), "clock") == 0) {
                if (!bFoundCk)
                    fb.Print("\t\t\tsig.m_ck = acc_handle_tfarg(i++);\n");
                bFoundCk = true;
            } else if (portWidth <= 32)
                fb.Print("\t\t\tsig.m_%s = acc_handle_tfarg(i++);\n", portName.c_str());
            else {
                fb.Print("\t\t\tsig.m_%sL = acc_handle_tfarg(i++);\n", portName.c_str());
                fb.Print("\t\t\tsig.m_%sU = acc_handle_tfarg(i++);\n", portName.c_str());
            }
        }
    }

    for (identIter.Begin(); !identIter.End(); identIter++) {
        if (identIter->IsOutPort()) {
            const string portName = identIter->GetName();

            int portWidth = identIter->GetWidth();

            if (identIter->IsClock()) {
                if (!bFoundCk)
                    fb.Print("\t\t\tsig.m_ck = acc_handle_tfarg(i++);\n");
                bFoundCk = true;
            } else if (portWidth <= 32)
                fb.Print("\t\t\tsig.m_%s = acc_handle_tfarg(i++);\n", portName.c_str());
            else {
                fb.Print("\t\t\tsig.m_%sL = acc_handle_tfarg(i++);\n", portName.c_str());
                fb.Print("\t\t\tsig.m_%sU = acc_handle_tfarg(i++);\n", portName.c_str());
            }
        }
    }

    fb.Print("\t\t\tacc_vcl_add(sig.m_ck, ::Fixture, null, vcl_verilog_logic);\n");
    fb.Print("\t\t\tacc_close();\n");
    fb.Print("\n");
    fb.Print("\t\t\ttime(&startTime);\n");
    fb.Print("\n");
    fb.Print("\t\t\tInitTest();\n");
    fb.Print("\t\t}\n");
    fb.Print("\t#endif // __SYSTEM_C__\n");
    fb.Print("#endif // _SC_LINT\n");
    fb.Print("\n");
    fb.Print("};\n");

    fb.Close();
}

void CHtvDesign::GenFixtureCpp(CHtvIdent *pModule)
{
    string fileName = m_pathName + "Fixture.cpp";
    FILE *fp = fopen(fileName.c_str(), "r");

    vector<string>	userLines;
    int userLinesBegin[8] = { 0, 0, 0, 0, 0, 0 };
    int userLinesEnd[8] = { 0, 0, 0, 0, 0, 0 };

    if (fp) {
        char lineBuf[512];
        while (fgets(lineBuf, 512, fp)) {
            char *pStr = lineBuf;
            while (pStr[0] == ' ' || pStr[0] == '\t') pStr++;
            if (strncmp(pStr, "//>>", 4) == 0) {
                int regionId = pStr[4] - '1';
                if (regionId > 7)
                    continue;

                userLinesBegin[regionId] = userLines.size();
                while (fgets(lineBuf, 512, fp)) {
                    char *pStr = lineBuf;
                    while (pStr[0] == ' ' || pStr[0] == '\t') pStr++;
                    if (strncmp(pStr, "//<<", 4) == 0) {
                        userLinesEnd[regionId] = userLines.size();
                        break;
                    }
                    userLines.push_back(lineBuf);
                }
            }
        }
        fclose(fp);
    }

    CFileBkup fb(m_pathName, "Fixture", "cpp");

    fb.PrintHeader("Fixture.cpp: Fixture method file");

    fb.Print("#include <Assert.h>\n");
    fb.Print("#include <stdint.h>\n");
    fb.Print("#include \"Random.h\"\n");
    //fb.Print("#include \"%sOpcodes.h\"\n", m_pHierTop->GetName().c_str());
    fb.Print("#include \"Fixture.h\"\n");
    fb.Print("\n");
    fb.Print("CRandom cnyRnd;\n");
    fb.Print("CFixtureModel * CFixtureTest::m_pModel;\n");
    fb.Print("\n");
    fb.Print("#ifndef __SYSTEM_C__\n");
    fb.Print("CFixture fixture;\n");
    fb.Print("\n");
    fb.Print("extern \"C\" int InitFixture() {\n");
    fb.Print("\tfixture.InitFixture();\n");
    fb.Print("}\n");
    fb.Print("\n");
    fb.Print("extern \"C\" int Fixture() {\n");
    fb.Print("\tfixture.Fixture();\n");
    fb.Print("}\n");
    fb.Print("#endif\n");
    fb.Print("\n");
    fb.Print("void\n");
    fb.Print("CFixture::Fixture()\n");
    fb.Print("{\n");
    fb.Print("\tif (!ClockFallingEdge())\n");
    fb.Print("\t\treturn;\n");
    fb.Print("\n");
    fb.Print("\tr_clkCnt += 1;\n");
    fb.Print("\n");
    fb.Print("\tDelayEarlyOutputs();\n");
    fb.Print("\n");
    fb.Print("\tif (r_clkCnt <= 32) {\n");
    fb.Print("\n");
    fb.Print("\t\t// SetSig_reset(r_clkCnt <= 2 ? 1 : 0);\n");
    fb.Print("\t\t// SetSig_active(1);\n");

    CHtvIdentTblIter identIter(pModule->GetIdentTbl());
    for (identIter.Begin(); !identIter.End(); identIter++) {
        if (identIter->IsOutPort() && !identIter->IsClock()) {
            const string portName = identIter->GetName();

            fb.Print("\t\tSetSig_%s(m_directedTests[0].m_%s);\n", portName.c_str(), portName.c_str());
        }
    }
    fb.Print("\n");
    fb.Print("\t} else {\n");
    fb.Print("\t\tif (!m_directedTests[r_inCnt].IsEnd()) {\n");
    for (identIter.Begin(); !identIter.End(); identIter++) {
        if (identIter->IsOutPort() && !identIter->IsClock()) {
            const string portName = identIter->GetName();

            fb.Print("\t\t\tSetSig_%s(m_directedTests[r_inCnt].m_%s);\n", portName.c_str(), portName.c_str());
        }
    }
    fb.Print("\n");
    fb.Print("\t\t\tm_directedTests[r_inCnt].m_clkCnt = r_clkCnt;\n");
    fb.Print("\t\t\tm_directedTests[r_inCnt].m_testCnt = r_testCnt++;\n");
    fb.Print("\n");
    fb.Print("\t\t\tm_testQue.push(m_directedTests[r_inCnt]);\n");
    fb.Print("\t\t\tr_inCnt += 1;\n");
    fb.Print("\n");
    fb.Print("\t\t} else if (r_testCnt != TEST_CNT) {\n");
    fb.Print("\n");
    fb.Print("\t\t\tCFixtureTest test(r_clkCnt, r_testCnt);\n");
    fb.Print("\n");
    fb.Print("\t\t\t//>>1\tSet rsltDly to appropriate value for design\n");
    if (userLinesBegin[0] == userLinesEnd[0]) {
        fb.Print("\t\t\t//if (r_testCnt == 847)\n");
        fb.Print("\t\t\t//\tbool stop = true;\n");
    } else {
        for (int i = userLinesBegin[0]; i < userLinesEnd[0]; i += 1)
            fb.Write(userLines[i].c_str());
    }
    fb.Print("\t\t\t//<<\n");
    fb.Print("\n");
    fb.Print("\t\t\t// Generate random inputs\n");
    for (identIter.Begin(); !identIter.End(); identIter++) {
        if (identIter->IsOutPort() && !identIter->IsClock()) {
            const string portName = identIter->GetName();
            int portWidth = identIter->GetWidth();

            if (identIter->GetType()->GetType()->IsStruct())
                fb.Print("\t\t\t%s %s;\n", identIter->GetType()->GetType()->GetName().c_str(), portName.c_str());
            else
                fb.Print("\t\t\tuint%d_t %s;\n", portWidth <= 32 ? 32 : 64, portName.c_str());
        }
    }
    fb.Print("\n");
    fb.Print("\t\t\tGenRandomInputs(");

    int cnt = 0;
    for (identIter.Begin(); !identIter.End(); identIter++) {
        if (identIter->IsOutPort() && !identIter->IsClock()) {
            const string portName = identIter->GetName();

            if (cnt > 0)
                fb.Print((cnt % 8 != 0) ? ", " : ",\n\t\t\t\t\t");
            cnt += 1;

            fb.Print("%s", portName.c_str());
        }
    }

    fb.Print(");\n");
    fb.Print("\n");
    fb.Print("\t\t\ttest.Test(");

    cnt = 0;
    if (pModule->GetClockListCnt() > 1) {
        cnt += 1;
        string clkName = pModule->GetClockList()[1]->GetName();
        fb.Print("r_phase%sCnt", clkName.c_str()+3);
    }
    for (identIter.Begin(); !identIter.End(); identIter++) {
        if (identIter->IsOutPort() && !identIter->IsClock()) {
            const string portName = identIter->GetName();

            if (cnt > 0)
                fb.Print((cnt % 8 != 0) ? ", " : ",\n\t\t\t\t\t");
            cnt += 1;

            fb.Print("%s", portName.c_str());
        }
    }
    fb.Print(");\n");
    fb.Print("\n");
    fb.Print("\t\t\tm_testQue.push(test);\n");
    fb.Print("\t\t\tr_testCnt += 1;\n");
    fb.Print("\n");
    fb.Print("\t\t\tif (r_testCnt %% 1000000 == 0) {\n");
    fb.Print("\t\t\t\ttime_t endTime;\n");
    fb.Print("\t\t\t\ttime(&endTime);\n");
    fb.Print("\t\t\t\ttime_t deltaTime = endTime - startTime;\n");
    fb.Print("\t\t\t\tprintf(\"%%d (%%d sec)\\n\", (int)r_testCnt, (unsigned)(deltaTime));\n");
    fb.Print("\t\t\t\tstartTime = endTime;\n");
    fb.Print("\t\t\t}\n");

    for (identIter.Begin(); !identIter.End(); identIter++) {
        if (identIter->IsOutPort() && !identIter->IsClock()) {
            const string portName = identIter->GetName();

            fb.Print("\t\t\tSetSig_%s(test.m_%s);\n", portName.c_str(), portName.c_str());
        }
    }
    fb.Print("\t\t}\n");
    fb.Print("\n");
    fb.Print("\t\t//>>2\tSet rsltDly to appropriate value for design\n");
    if (userLinesBegin[1] == userLinesEnd[1])
        fb.Print("\t\tint rsltDly = 17;\n");
    else {
        for (int i = userLinesBegin[1]; i < userLinesEnd[1]; i += 1)
            fb.Write(userLines[i].c_str());
    }
    fb.Print("\t\t//<<\n");
    fb.Print("\n");
    fb.Print("\t\tif (!m_testQue.empty() && m_testQue.front().m_clkCnt == r_clkCnt-rsltDly) {\n");
    fb.Print("\n");
    for (identIter.Begin(); !identIter.End(); identIter++) {
        if (identIter->IsInPort() && !identIter->IsClock()) {
            const string portName = identIter->GetName();
            int portWidth = identIter->GetWidth();

            fb.Print("\t\t\tuint%d_t %s;\n", portWidth <= 32 ? 32 : 64, portName.c_str());
        }
    }
    fb.Print("\n");

    fb.Print("\t\t\tGetOutputs(");

    cnt = 0;
    for (identIter.Begin(); !identIter.End(); identIter++) {
        if (identIter->IsInPort() && !identIter->IsClock()) {
            const string portName = identIter->GetName();

            if (cnt > 0)
                fb.Print((cnt % 8 != 0) ? ", " : ",\n\t\t\t\t\t");
            cnt += 1;

            fb.Print("%s", portName.c_str());
        }
    }
    fb.Print(");\n");

    fb.Print("\n");
    fb.Print("\t\t\t// check random test\n");
    fb.Print("\t\t\tCFixtureTest test = m_testQue.front();\n");
    fb.Print("\t\t\tm_testQue.pop();\n");
    fb.Print("\n");
    fb.Print("\t\t\t//>>3\tDesign specific difference reporting qualifications\n");
    if (userLinesBegin[2] == userLinesEnd[2])
        fb.Print("\t\t\tif (test.m_testCnt > 2 && (\n");
    else {
        for (int i = userLinesBegin[2]; i < userLinesEnd[2]; i += 1)
            fb.Write(userLines[i].c_str());
    }
    fb.Print("\t\t\t//<<\n\t\t\t\t");

    cnt = 0;
    for (identIter.Begin(); !identIter.End(); identIter++) {
        if (identIter->IsInPort() && !identIter->IsClock()) {
            const string portName = identIter->GetName();

            if (cnt > 0)
                fb.Print(" ||\n\t\t\t\t");
            cnt += 1;

            fb.Print("%s != test.m_%s", portName.c_str(), portName.c_str());
        }
    }
    fb.Print("\n\t\t\t\t)) {\n");
    fb.Print("\n");
    fb.Print("\t\t\t\tprintf(\"%%lld:\\n\", test.m_testCnt);\n");
    fb.Print("\n");

    for (identIter.Begin(); !identIter.End(); identIter++) {
        if (identIter->IsOutPort() && !identIter->IsClock()) {
            const string portName = identIter->GetName();
            int portWidth = identIter->GetWidth();

            fb.Print("\t\t\t\tprintf(\"    %-28s = 0x%%0%dllx\\n\", (uint64_t)test.m_%s%s);\n",
                portName.c_str(), (portWidth+3)/4, portName.c_str(),
                identIter->GetType()->GetType()->IsStruct() ? ".m_data" : "");
        }
    }
    fb.Print("\n");
    for (identIter.Begin(); !identIter.End(); identIter++) {
        if (identIter->IsInPort() && !identIter->IsClock()) {
            const string portName = identIter->GetName();
            int portWidth = identIter->GetWidth();

            fb.Print("\t\t\t\tprintf(\"    %-19s Expected = 0x%%0%dllx\\n\", (uint64_t)test.m_%s%s);\n",
                portName.c_str(), (portWidth+3)/4, portName.c_str(),
                identIter->GetType()->GetType()->IsStruct() ? ".m_data" : "");

            fb.Print("\t\t\t\tif (%s != test.m_%s)\n", portName.c_str(), portName.c_str());
            fb.Print("\t\t\t\t\tprintf(\"                          Actual = 0x%%0%dllx\\n\", (uint64_t)%s%s);\n",
                (portWidth+3)/4, portName.c_str(),
                identIter->GetType()->GetType()->IsStruct() ? ".m_data" : "");
            fb.Print("\n");
        }
    }
    fb.Print("\t\t\t\tprintf(\"\\tm_directedTests.push_back(test.Test(");
    const char *pSeparator = "";
    int sepCnt = 4;
    for (identIter.Begin(); !identIter.End(); identIter++) {
        if (identIter->IsOutPort() && !identIter->IsClock()) {
            const string portName = identIter->GetName();
            int portWidth = identIter->GetWidth();

            fb.Print("%s0x%%0%dllx%s", pSeparator, (portWidth+3)/4, portWidth > 32 ? "LL" : "");
            if (sepCnt == 8) {
                pSeparator = ", \"\n\t\t\t\t\t\"";
                sepCnt = 0;
            } else {
                pSeparator = ", ";
                sepCnt += 1;
            }
        }
    }
    fb.Print("));\\n\",");
    pSeparator = "\n\t\t\t\t\t";
    sepCnt = 0;
    for (identIter.Begin(); !identIter.End(); identIter++) {
        if (identIter->IsOutPort() && !identIter->IsClock()) {
            const string portName = identIter->GetName();

            fb.Print("%s(uint64_t)test.m_%s", pSeparator, portName.c_str());
            if (identIter->GetType()->GetType()->IsStruct())
                fb.Print(".m_data");
            if (sepCnt == 4) {
                pSeparator = ",\n\t\t\t\t\t";
                sepCnt = 0;
            } else {
                pSeparator = ", ";
                sepCnt += 1;
            }
        }
    }
    fb.Print(");\n");
    fb.Print("\n");
    fb.Print("\t\t\t\tbool stop = true;\n");
    fb.Print("\t\t\t\t//>>4\tSet what to do on error\n");
    if (userLinesBegin[3] == userLinesEnd[3]) {
        fb.Print("\t\t\t\texit(0);\n");
    } else {
        for (int i = userLinesBegin[3]; i < userLinesEnd[3]; i += 1)
            fb.Write(userLines[i].c_str());
    }
    fb.Print("\t\t\t\t//<<\n");
    fb.Print("\t\t\t}\n");
    fb.Print("\n");
    fb.Print("\t\t} else if (!m_testQue.empty() && m_testQue.front().m_clkCnt <= r_clkCnt-32)\n");
    fb.Print("\t\t\tassert(0);\n");
    fb.Print("\n");
    fb.Print("\t\tif (m_testQue.empty() && r_testCnt == TEST_CNT) {\n");
    fb.Print("\t\t\tprintf(\"Tests passed\\n\");\n");
    fb.Print("\t\t\texit(0);\n");
    fb.Print("\t\t}\n");
    fb.Print("\n");
    fb.Print("\t}\n");
    fb.Print("}\n");
    fb.Print("\n");
    fb.Print("void\n");
    fb.Print("CFixture::GenRandomInputs(");

    cnt = 0;
    for (identIter.Begin(); !identIter.End(); identIter++) {
        if (identIter->IsOutPort() && !identIter->IsClock()) {
            const string portName = identIter->GetName();

            if (cnt > 0)
                fb.Print((cnt % 5 != 0) ? ", " : ",\n\t\t\t\t");
            cnt += 1;

            if (identIter->GetType()->GetType()->IsStruct())
                fb.Print("%s &%s", identIter->GetType()->GetType()->GetName().c_str(), portName.c_str());
            else
                fb.Print("uint%d_t &%s", identIter->GetWidth() <= 32 ? 32 : 64, portName.c_str());
        }
    }

    fb.Print(")\n");
    fb.Print("{\n");

    for (identIter.Begin(); !identIter.End(); identIter++) {
        if (identIter->IsOutPort() && !identIter->IsClock()) {
            const string portName = identIter->GetName();
            int portWidth = identIter->GetWidth();

            fb.Print("\t%s%s = cnyRnd.RndUint%d() & 0x%llx%s;\n", portName.c_str(),
                identIter->GetType()->GetType()->IsStruct() ? ".m_data" : "", portWidth <= 32 ? 32 : 64,
                ~(~(uint64)0 << portWidth), portWidth > 32 ? "LL" : "");
        }
    }

    fb.Print("\n");
    fb.Print("\t//>>5\tOverride random input generation\n");
    for (int i = userLinesBegin[4]; i < userLinesEnd[4]; i += 1)
        fb.Write(userLines[i].c_str());
    fb.Print("\t//<<\n");

    fb.Print("}\n");
    fb.Print("\n");
    fb.Print("void\n");
    fb.Print("CFixture::DelayEarlyOutputs()\n");
    fb.Print("{\n");

    for (identIter.Begin(); !identIter.End(); identIter++) {
        if (identIter->IsInPort() && !identIter->IsClock()) {
            const string portName = identIter->GetName();

            fb.Print("\tm_stage_%s = (m_stage_%s << 1) | (GetSig_%s() & 1);\n", portName.c_str(), portName.c_str(), portName.c_str());
        }
    }

    fb.Print("}\n");
    fb.Print("\n");
    fb.Print("void\n");
    fb.Print("CFixture::GetOutputs(");

    cnt = 0;
    for (identIter.Begin(); !identIter.End(); identIter++) {
        if (identIter->IsInPort() && !identIter->IsClock()) {
            const string portName = identIter->GetName();

            if (cnt > 0)
                fb.Print((cnt % 5 != 0) ? ", " : ",\n\t\t\t\t");
            cnt += 1;

            fb.Print("uint%d_t &%s", identIter->GetWidth() <= 32 ? 32 : 64, portName.c_str());
        }
    }

    fb.Print(")\n");
    fb.Print("{\n");

    fb.Print("\t//>>6\tGet output values\n");
    if (userLinesBegin[5] == userLinesEnd[5]) {
        for (identIter.Begin(); !identIter.End(); identIter++) {
            if (identIter->IsInPort() && !identIter->IsClock()) {
                const string portName = identIter->GetName();

                fb.Print("\t%s = GetSig_%s();\n", portName.c_str(), portName.c_str());
            }
        }
        fb.Print("\n");
        for (identIter.Begin(); !identIter.End(); identIter++) {
            if (identIter->IsInPort() && !identIter->IsClock()) {
                const string portName = identIter->GetName();

                fb.Print("\t//%s = (m_stage_%s >> 0) & 1;\n", portName.c_str(), portName.c_str());
            }
        }
    } else {
        for (int i = userLinesBegin[5]; i < userLinesEnd[5]; i += 1)
            fb.Write(userLines[i].c_str());
    }
    fb.Print("\t//<<\n");

    fb.Print("}\n");
    fb.Print("\n");
    fb.Print("void\n");
    fb.Print("CFixture::InitTest()\n");
    fb.Print("{\n");
    fb.Print("\tCFixtureTest test;\n");
    fb.Print("\n");

    fb.Print("\t//>>7\tDirected tests\n");
    if (userLinesBegin[6] == userLinesEnd[6]) {
        fb.Print("\t//m_directedTests.push_back(test.Test(...));\n");
    } else {
        for (int i = userLinesBegin[6]; i < userLinesEnd[6]; i += 1)
            fb.Write(userLines[i].c_str());
    }
    fb.Print("\t//<<\n");
    fb.Print("\n");
    fb.Print("\tm_directedTests.push_back(test.End());\n");
    fb.Print("}\n");

    fb.Print("\n");
    fb.Print("//>>8	Fixture specific methods\n");
    for (int i = userLinesBegin[7]; i < userLinesEnd[7]; i += 1)
        fb.Write(userLines[i].c_str());
    fb.Print("//<<\n");

    fb.Close();
}

void CHtvDesign::GenScCodeFiles(const string &cppFileName)
{
    // ScLint was requested to generate a .v file. The .cpp source file does not exist.
    //   This routine generates a framework .cpp file as a starting point. This routine
    //   will also generate a .h file if one does not exist.

    // check if .h file exists
    string hFileName = cppFileName;
    int periodPos = hFileName.find_last_of(".");
    if (periodPos > 0)
        hFileName.erase(periodPos+1);
    hFileName += "h";

    // use file name for module name
    string baseName = cppFileName;
    if (periodPos > 0)
        baseName.erase(periodPos);
    int slashPos = baseName.find_last_of("\\/");
    baseName.erase(0, slashPos+1);

    FILE *pCppFile;
    if (!(pCppFile = fopen(cppFileName.c_str(), "w"))) {
        printf("  Could not open input file '%s' for writing, exiting\n", cppFileName.c_str());
        ErrorExit();
    }

    fprintf(pCppFile, "#include <stdint.h>\n");
    fprintf(pCppFile, "#include \"%s.h\"\n", baseName.c_str());
    fprintf(pCppFile, "//#include \"%sPrims.h\"\n", baseName.c_str());
    fprintf(pCppFile, "\n");

    if (OpenInputFile(hFileName)) {
        // .h file exists, parse file and extract info needed to produce framework cpp file

        do {
            if (GetNextToken() == tk_SC_CTOR) {
                GetNextToken();	// '('
                if (GetNextToken() == tk_identifier) {
                    const string moduleName = GetString();

                    do {
                        if (GetNextToken() == tk_SC_METHOD) {
                            GetNextToken();	// '('
                            if (GetNextToken() == tk_identifier) {
                                const string methodName = GetString();

                                fprintf(pCppFile, "void\n");
                                fprintf(pCppFile, "%s::%s()\n", moduleName.c_str(), methodName.c_str());
                                fprintf(pCppFile, "{\n");
                                fprintf(pCppFile, "\n");
                                fprintf(pCppFile, "#ifndef _SC_LINT\n");
                                fprintf(pCppFile, "\tsc_time time = sc_time_stamp();\n");
                                fprintf(pCppFile, "\tif (time == sc_time(3790, SC_NS))\n");
                                fprintf(pCppFile, "\t\tbool stop = true;\n");
                                fprintf(pCppFile, "#endif\n");
                                fprintf(pCppFile, "\n");
                                fprintf(pCppFile, "\t//zero = 0;\n");
                                fprintf(pCppFile, "\t//one = ~zero;\n");
                                fprintf(pCppFile, "\n");
                                fprintf(pCppFile, "\t///////////////////////\n");
                                fprintf(pCppFile, "\t// Combinatorial logic\n");
                                fprintf(pCppFile, "\t//\n");
                                fprintf(pCppFile, "\n");
                                fprintf(pCppFile, "\t///////////////////////\n");
                                fprintf(pCppFile, "\t// Register assignments\n");
                                fprintf(pCppFile, "\t//\n");
                                fprintf(pCppFile, "\n");
                                fprintf(pCppFile, "\t///////////////////////\n");
                                fprintf(pCppFile, "\t// Update outputs\n");
                                fprintf(pCppFile, "\t//\n");
                                fprintf(pCppFile, "\n");
                                fprintf(pCppFile, "#ifndef _SC_LINT\n");
                                fprintf(pCppFile, "\tif (time == sc_time(190, SC_NS))\n");
                                fprintf(pCppFile, "\t\tbool stop = true;\n");
                                fprintf(pCppFile, "#endif\n");
                                fprintf(pCppFile, "}\n");
                            }
                        }
                    } while (GetToken() != tk_eof);
                }
            }
        } while (GetToken() != tk_eof);

    } else {

        // neither .h or .cpp file exists, create files with default names
        fprintf(pCppFile, "void\n");
        fprintf(pCppFile, "C%s::%s()\n", baseName.c_str(), baseName.c_str());
        fprintf(pCppFile, "{\n");
        fprintf(pCppFile, "\n");
        fprintf(pCppFile, "#ifndef _SC_LINT\n");
        fprintf(pCppFile, "\tint time = (int)sc_simulation_time();\n");
        fprintf(pCppFile, "\tif (time == 3790)\n");
        fprintf(pCppFile, "\t\tbool stop = true;\n");
        fprintf(pCppFile, "#endif\n");
        fprintf(pCppFile, "\n");
        fprintf(pCppFile, "\t//zero = 0;\n");
        fprintf(pCppFile, "\t//one = ~zero;\n");
        fprintf(pCppFile, "\n");
        fprintf(pCppFile, "\t///////////////////////\n");
        fprintf(pCppFile, "\t// Combinatorial logic\n");
        fprintf(pCppFile, "\t//\n");
        fprintf(pCppFile, "\n");
        fprintf(pCppFile, "\t///////////////////////\n");
        fprintf(pCppFile, "\t// Register assignments\n");
        fprintf(pCppFile, "\t//\n");
        fprintf(pCppFile, "\n");
        fprintf(pCppFile, "\t///////////////////////\n");
        fprintf(pCppFile, "\t// Update outputs\n");
        fprintf(pCppFile, "\t//\n");
        fprintf(pCppFile, "\n");
        fprintf(pCppFile, "#ifndef _SC_LINT\n");
        fprintf(pCppFile, "\tif (time == 190)\n");
        fprintf(pCppFile, "\t\tbool stop = true;\n");
        fprintf(pCppFile, "#endif\n");
        fprintf(pCppFile, "}\n");

        FILE *pIncFile;
        if (!(pIncFile = fopen(hFileName.c_str(), "w"))) {
            printf("  Could not open input file '%s' for writing, exiting\n", cppFileName.c_str());
            ErrorExit();
        }

        fprintf(pIncFile, "#pragma once\n");
        fprintf(pIncFile, "\n");
        fprintf(pIncFile, "#include <SystemC.h>\n");
        fprintf(pIncFile, "\n");
        fprintf(pIncFile, "#ifndef _SC_LINT\n");
        fprintf(pIncFile, "#include \"ScPrims.h\"\n");
        fprintf(pIncFile, "#endif\n");
        fprintf(pIncFile, "\n");
        fprintf(pIncFile, "//#include \"%sOpcodes.h\"\n", baseName.c_str());
        fprintf(pIncFile, "//#include \"%sPrims.h\"\n", baseName.c_str());
        fprintf(pIncFile, "\n");
        fprintf(pIncFile, "SC_MODULE(C%s) {\n", baseName.c_str());
        fprintf(pIncFile, "\n");
        fprintf(pIncFile, "\t//sc_uint<64> zero;\n");
        fprintf(pIncFile, "\t//sc_uint<64> one;\n");
        fprintf(pIncFile, "\n");
        fprintf(pIncFile, "\t///////////////////////\n");
        fprintf(pIncFile, "\t// Port declarations\n");
        fprintf(pIncFile, "\t//\n");
        fprintf(pIncFile, "\n");
        fprintf(pIncFile, "\t//sc_in<bool>\t\t\ti_clock;\n");
        fprintf(pIncFile, "\t//sc_in<sc_uint<32> >\ti_data;\n");
        fprintf(pIncFile, "\t//sc_out<sc_uint<32> >\to_rslt;\n");
        fprintf(pIncFile, "\n");
        fprintf(pIncFile, "\t///////////////////////\n");
        fprintf(pIncFile, "\t// State declarations\n");
        fprintf(pIncFile, "\t//\n");
        fprintf(pIncFile, "\n");
        fprintf(pIncFile, "\t// state for first clock\n");
        fprintf(pIncFile, "\t// sc_uint<1>	r1_vm;\n");
        fprintf(pIncFile, "\t// sc_uint<32>	r1_data;\n");
        fprintf(pIncFile, "\n");
        fprintf(pIncFile, "\t// state for second clock\n");
        fprintf(pIncFile, "\t// sc_uint<32>	r2_rslt;\n");
        fprintf(pIncFile, "\n");
        fprintf(pIncFile, "\t///////////////////////\n");
        fprintf(pIncFile, "\t// Method declarations\n");
        fprintf(pIncFile, "\t//\n");
        fprintf(pIncFile, "\n");
        fprintf(pIncFile, "\tvoid %s();\n", baseName.c_str());
        fprintf(pIncFile, "\n");
        fprintf(pIncFile, "\tSC_CTOR(C%s) {\n", baseName.c_str());
        fprintf(pIncFile, "\t\tSC_METHOD(%s);\n", baseName.c_str());
        fprintf(pIncFile, "\t\t//sensitive << i_clock.pos();\n");
        fprintf(pIncFile, "\t}\n");
        fprintf(pIncFile, "};\n");
        fclose(pIncFile);
    }

    fclose(pCppFile);

    OpenInputFile(cppFileName);
}

void CHtvDesign::GenFixtureModelH(CHtvIdent *pModule)
{
    string fileName = m_pathName + "FixtureModel.h";
    FILE *fp;
    if (fp = fopen(fileName.c_str(), "r")) {
        fclose(fp);
        return;
    }

    if (!(fp = fopen(fileName.c_str(), "w"))) {
        printf("Could not open '%s' for writing\n", fileName.c_str());
        ErrorExit();
    }

    PrintHeader(fp, "FixtureModel.h: Fixture model include file");

    fprintf(fp, "#pragma once\n");
    fprintf(fp, "\n");
    fprintf(fp, "#include <stdint.h>\n");
    fprintf(fp, "\n");
    fprintf(fp, "class CFixtureModel\n");
    fprintf(fp, "{\n");
    fprintf(fp, "public:\n");
    fprintf(fp, "\n");
    fprintf(fp, "\tCFixtureModel(void)\n");
    fprintf(fp, "\t{\n");
    fprintf(fp, "\t}\n");
    fprintf(fp, "\n");
    fprintf(fp, "\t~CFixtureModel(void)\n");
    fprintf(fp, "\t{\n");
    fprintf(fp, "\t}\n");
    fprintf(fp, "\n");
    fprintf(fp, "\tvoid Model(");

    CHtvIdentTblIter identIter(pModule->GetIdentTbl());

    int cnt = 0;
    for (identIter.Begin(); !identIter.End(); identIter++) {
        if (identIter->IsOutPort() && !identIter->IsClock()) {
            const string portName = identIter->GetName();

            if (cnt > 0)
                fprintf(fp, (cnt % 5 != 0) ? ", " : ",\n\t\t\t");
            cnt += 1;

            fprintf(fp, "uint%d_t %s", identIter->GetWidth() <= 32 ? 32 : 64, portName.c_str());
        }
    }

    for (identIter.Begin(); !identIter.End(); identIter++) {
        if (identIter->IsInPort() && !identIter->IsClock()) {
            const string portName = identIter->GetName();

            if (cnt > 0)
                fprintf(fp, (cnt % 5 != 0) ? ", " : ",\n\t\t\t");
            cnt += 1;

            fprintf(fp, "uint%d_t &%s", identIter->GetWidth() <= 32 ? 32 : 64, portName.c_str());
        }
    }

    fprintf(fp, ");\n");
    fprintf(fp, "\n");
    fprintf(fp, "private:\n");
    fprintf(fp, "\n");
    fprintf(fp, "};\n");

    fclose(fp);
}

void CHtvDesign::GenFixtureModelCpp(CHtvIdent *pModule)
{
    string fileName = m_pathName + "FixtureModel.cpp";
    FILE *fp;
    if (fp = fopen(fileName.c_str(), "r")) {
        fclose(fp);
        return;
    }

    if (!(fp = fopen(fileName.c_str(), "w"))) {
        printf("Could not open '%s' for writing\n", fileName.c_str());
        ErrorExit();
    }

    PrintHeader(fp, "FixtureModel.cpp: Fixture model method file");
    fprintf(fp, "\n");
    fprintf(fp, "#include \"FixtureModel.h\"\n");
    fprintf(fp, "\n");
    fprintf(fp, "void\n");
    fprintf(fp, "CFixtureModel::Model(\n\t\t\t\t");

    CHtvIdentTblIter identIter(pModule->GetIdentTbl());

    int cnt = 0;
    for (identIter.Begin(); !identIter.End(); identIter++) {
        if (identIter->IsOutPort() && !identIter->IsClock()) {
            const string portName = identIter->GetName();

            if (cnt > 0)
                fprintf(fp, (cnt % 5 != 0) ? ", " : ",\n\t\t\t\t");
            cnt += 1;

            if (identIter->GetType()->GetType()->IsStruct())
                fprintf(fp, "%s %s", identIter->GetType()->GetType()->GetName().c_str(), portName.c_str());
            else
                fprintf(fp, "uint%d_t %s", identIter->GetWidth() <= 32 ? 32 : 64, portName.c_str());
        }
    }

    for (identIter.Begin(); !identIter.End(); identIter++) {
        if (identIter->IsInPort() && !identIter->IsClock()) {
            const string portName = identIter->GetName();

            if (cnt > 0)
                fprintf(fp, (cnt % 5 != 0) ? ", " : ",\n\t\t\t\t");
            cnt += 1;

            fprintf(fp, "uint%d_t &%s", identIter->GetWidth() <= 32 ? 32 : 64, portName.c_str());
        }
    }

    fprintf(fp, ")\n");
    fprintf(fp, "{\n");

    for (identIter.Begin(); !identIter.End(); identIter++) {
        if (identIter->IsInPort() && !identIter->IsClock()) {
            const string portName = identIter->GetName();

            fprintf(fp, "\t %s = 0;\n", portName.c_str());
        }
    }

    fprintf(fp, "}\n");

    fclose(fp);
}

void CHtvDesign::GenSandboxVpp(CHtvIdent *pModule)
{
    string fileName = m_pathName + "sandbox.vpp";
    FILE *fp;
    if (!(fp = fopen(fileName.c_str(), "w"))) {
        printf("Could not open '%s' for writing\n", fileName.c_str());
        ErrorExit();
    }

    PrintHeader(fp, "sandbox.vpp: Xilinx sandbox file");

    fprintf(fp, "(* keep_hierarchy = \"true\" *)\n");
    fprintf(fp, "module sandbox (\n");
    fprintf(fp, "input\t\tclk, clkhx, clkqx,\n");
    fprintf(fp, "input\t\treset,\n");
    fprintf(fp, "input\t[511:0]\tins,\n");
    fprintf(fp, "output\t[511:0]\touts\n");
    fprintf(fp, ");\n");
    fprintf(fp, "\t(* keep = \"true\" *) wire r_reset, phase;\n");
    fprintf(fp, "\tDFFS_KEEP (1, rst, clkhx, reset, 1'd0, r_reset);\n");
    fprintf(fp, "\tPHASEGEN  (1, phs, clkhx, clk, r_reset, phase, noconn);\n");
    fprintf(fp, "\n");
    fprintf(fp, "\t%s dut (\n", "spfd_mul");

    int insCnt = 0;
    int outsCnt = 0;
    const char *pSeparator = "\n\t\t";

    CHtvIdentTblIter identIter(pModule->GetFlatIdentTbl());
    for (identIter.Begin(); !identIter.End(); identIter++) {
        if (!identIter->IsOutPort() && !identIter->IsInPort())
            continue;

        bool bIsClock = identIter->IsClock();
        bool bIsInput = identIter->IsInPort();
        int portWidth = identIter->GetWidth();
        const string portName = identIter->GetName();

        if (bIsClock) {
            fprintf(fp, "%s.i_%s(clk)", pSeparator, portName.c_str());
        } else if (bIsInput) {
            fprintf(fp, "%s.i_%s(ins[%d:%d])", pSeparator, portName.c_str(), insCnt + portWidth - 1, insCnt);
            insCnt += portWidth;
        } else {
            fprintf(fp, "%s.o_%s(outs[%d:%d])", pSeparator, portName.c_str(), outsCnt + portWidth - 1, outsCnt);
            outsCnt += portWidth;
        }
        pSeparator = ",\n\t\t";
    }
    fprintf(fp, "\n\t);\n\nendmodule\n");
    fprintf(fp, "`include ./%s.v\n", "spfd_mul");
    fprintf(fp, "`include ./ScPrims.v\n");

    fclose(fp);
}


