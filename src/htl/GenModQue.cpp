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

void CDsnInfo::InitAndValidateModIhd()
{
        for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
                CModule * pMod = m_modList[modIdx];

                if (!pMod->m_bIsUsed || pMod->m_queIntfList.size() == 0) continue;

                for (size_t queIdx = 0; queIdx < pMod->m_queIntfList.size(); queIdx += 1) {
                        CQueIntf & queIntf = pMod->m_queIntfList[queIdx];

                        if (queIntf.m_queType == Push) {
                                if (pMod->m_instSet.GetTotalCnt() > 1)
                                        ParseMsg(Error, queIntf.m_lineInfo, "Inbound host data to replicated/multi-instance module not supported");
                        } else {
                                if (pMod->m_instSet.GetTotalCnt() > 1)
                                        ParseMsg(Error, queIntf.m_lineInfo, "Outbound host data from replicated/multi-instance module not supported");
                        }
                }
        }
}

void
CDsnInfo::GenModIhdStatements(CInstance * pModInst)
{
        CModule * pMod = pModInst->m_pMod;

        if (pMod->m_queIntfList.size() == 0)
                return;

        CHtCode & ihdPreInstr = pMod->m_clkRate == eClk1x ? m_ihdPreInstr1x : m_ihdPreInstr2x;
        CHtCode & ihdPostInstr = pMod->m_clkRate == eClk1x ? m_ihdPostInstr1x : m_ihdPostInstr2x;
        CHtCode & ihdReg = pMod->m_clkRate == eClk1x ? m_ihdReg1x : m_ihdReg2x;
        CHtCode & ihdPostReg = pMod->m_clkRate == eClk1x ? m_ihdPostReg1x : m_ihdPostReg2x;
        CHtCode & ihdOut = pMod->m_clkRate == eClk1x ? m_ihdOut1x : m_ihdOut2x;

        string reset = pMod->m_clkRate == eClk1x ? "r_reset1x" : "c_reset2x";

        // I/O declarations
        m_ihdIoDecl.Append("\t// Inbound host data\n");

        bool bIhd = false;
        for (size_t queIdx = 0; queIdx < pMod->m_queIntfList.size(); queIdx += 1) {
                CQueIntf & queIntf = pMod->m_queIntfList[queIdx];

                if (queIntf.m_queType != Pop) continue;
                bIhd = true;

                // in bound interface
                m_ihdIoDecl.Append("\tsc_in<bool> i_hifTo%s_%sRdy;\n",
                        pMod->m_modName.Uc().c_str(), queIntf.m_queName.Lc().c_str());
                string structName = queIntf.m_typeName + (queIntf.m_bInclude ? "_inc" : "");
                m_ihdIoDecl.Append("\tsc_in<%s> i_hifTo%s_%s;\n",
                        structName.c_str(),
                        pMod->m_modName.Uc().c_str(), queIntf.m_queName.Lc().c_str());
                m_ihdIoDecl.Append("\tsc_out<bool> o_%sToHif_%sAvl;\n",
                        pMod->m_modName.Lc().c_str(), queIntf.m_queName.Lc().c_str());
                m_ihdIoDecl.NewLine();
        }

        if (!bIhd)
                return;

        g_appArgs.GetDsnRpt().AddLevel("Inbound Host Data\n");

        // Ram declarations
        for (size_t queIdx = 0; queIdx < pMod->m_queIntfList.size(); queIdx += 1) {
                CQueIntf & queIntf = pMod->m_queIntfList[queIdx];

                if (queIntf.m_queType != Pop) continue;

                string structName = queIntf.m_typeName + (queIntf.m_bInclude ? "_inc" : "");
                m_ihdRamDecl.Append("\tht_dist_que<%s, 5> m_%sQue;\n",
                        structName.c_str(), queIntf.m_queName.Lc().c_str());

                m_ihdRegDecl.Append("\t%s r_%sQue_front;\n",
                        structName.c_str(), queIntf.m_queName.Lc().c_str());
                m_ihdRegDecl.Append("\tbool r_%sQue_empty;\n",
                        queIntf.m_queName.Lc().c_str());
                m_ihdCtorInit.Append("\t\tr_%sQue_empty = true;\n",
                        queIntf.m_queName.Lc().c_str());
        }

        // Reg declarations
        for (size_t queIdx = 0; queIdx < pMod->m_queIntfList.size(); queIdx += 1) {
                CQueIntf & queIntf = pMod->m_queIntfList[queIdx];

                if (queIntf.m_queType != Pop) continue;

                m_ihdRegDecl.Append("\tbool r_%sTo%s_%sAvl;\n",
                        pMod->m_modName.Lc().c_str(), queIntf.m_modName.Uc().c_str(), queIntf.m_queName.Lc().c_str());
        }
        m_ihdRegDecl.NewLine();

        // queue interface macros
        for (size_t queIdx = 0; queIdx < pMod->m_queIntfList.size(); queIdx += 1) {
                CQueIntf & queIntf = pMod->m_queIntfList[queIdx];

                if (queIntf.m_queType != Pop) continue;

                string unitNameUc = !g_appArgs.IsModuleUnitNamesEnabled() ? m_unitName.Uc() : "";

                m_ihdRegDecl.Append("\tbool ht_noload c_%sAvlCntBusy;\n", queIntf.m_queName.Lc().c_str());
                m_ihdRegDecl.Append("\tbool ht_noload c_%sAvlCntAvail;\n", queIntf.m_queName.Lc().c_str());
                string vcdModName = VA("Pers%s", pMod->m_modName.Uc().c_str());
                GenModTrace(eVcdUser, vcdModName, "RecvHostDataBusy()", VA("c_%sAvlCntBusy", queIntf.m_queName.Lc().c_str()));
                ihdPreInstr.Append("\tc_%sAvlCntAvail = false;\n", queIntf.m_queName.Lc().c_str());

                ihdPostReg.Append("#ifdef _HTV\n");
                ihdPostReg.Append("\tc_%sAvlCntBusy = r_%sQue_empty;\n", queIntf.m_queName.Lc().c_str(), queIntf.m_queName.Lc().c_str());
                ihdPostReg.Append("#else\n");
                ihdPostReg.Append("\tc_%sAvlCntBusy = r_%sQue_empty || (%s_RND_RETRY && !!(g_rndRetry() & 1));\n",
                        queIntf.m_queName.Lc().c_str(),
                        queIntf.m_queName.Lc().c_str(),
                        pModInst->m_instName.Upper().c_str());
                ihdPostReg.Append("#endif\n");

                g_appArgs.GetDsnRpt().AddItem("bool RecvHostDataBusy()\n");
                m_ihdFuncDecl.Append("\tbool RecvHostDataBusy();\n");
                m_ihdMacros.Append("bool CPers%s%s::RecvHostDataBusy()\n", unitNameUc.c_str(), pMod->m_modName.Uc().c_str());
                m_ihdMacros.Append("{\n");

                m_ihdMacros.Append("\tc_%sAvlCntAvail = !c_%sAvlCntBusy;\n", queIntf.m_queName.Lc().c_str(), queIntf.m_queName.Lc().c_str());
                m_ihdMacros.Append("\treturn c_%sAvlCntBusy;\n", queIntf.m_queName.Lc().c_str());
                m_ihdMacros.Append("}\n");
                m_ihdMacros.NewLine();

                g_appArgs.GetDsnRpt().AddItem("uint64_t PeekHostData()\n");
                m_ihdFuncDecl.Append("\tuint64_t PeekHostData();\n");
                m_ihdMacros.Append("uint64_t CPers%s%s::PeekHostData()\n", unitNameUc.c_str(), pMod->m_modName.Uc().c_str());
                m_ihdMacros.Append("{\n");

                m_ihdMacros.Append("\treturn r_%sQue_front.m_data;\n", queIntf.m_queName.Lc().c_str());
                m_ihdMacros.Append("}\n");
                m_ihdMacros.NewLine();

                g_appArgs.GetDsnRpt().AddItem("uint64_t RecvHostDataPeek()\n");
                m_ihdFuncDecl.Append("\tuint64_t RecvHostDataPeek();\n");
                m_ihdMacros.Append("uint64_t CPers%s%s::RecvHostDataPeek()\n", unitNameUc.c_str(), pMod->m_modName.Uc().c_str());
                m_ihdMacros.Append("{\n");

                m_ihdMacros.Append("\treturn r_%sQue_front.m_data;\n", queIntf.m_queName.Lc().c_str());
                m_ihdMacros.Append("}\n");
                m_ihdMacros.NewLine();

                g_appArgs.GetDsnRpt().AddItem("uint64_t RecvHostData()\n");
                m_ihdFuncDecl.Append("\tuint64_t RecvHostData();\n");

                m_ihdMacros.Append("uint64_t CPers%s%s::RecvHostData()\n", unitNameUc.c_str(), pMod->m_modName.Uc().c_str());
                m_ihdMacros.Append("{\n");

                m_ihdMacros.Append("\tassert_msg(c_%sAvlCntAvail, \"Runtime check failed in CPers%s::RecvHostData()"
                        " - expected RecvHostDataBusy() to have been called and not busy\");\n",
                        queIntf.m_queName.Lc().c_str(), pMod->m_modName.Uc().c_str());

                m_ihdMacros.Append("\tassert_msg(r_%sQue_front.m_ctl == HIF_DQ_DATA, \"Runtime check failed in CPers%s::PeekHostData()"
                        " - expected data to be ready (and not a data marker)\");\n",
                        queIntf.m_queName.Lc().c_str(), pMod->m_modName.Uc().c_str());

                m_ihdMacros.Append("\tc_%sTo%s_%sAvl = true;\n",
                        pMod->m_modName.Lc().c_str(), queIntf.m_modName.Uc().c_str(), queIntf.m_queName.Lc().c_str());

                m_ihdMacros.Append("\treturn r_%sQue_front.m_data;\n", queIntf.m_queName.Lc().c_str());
                m_ihdMacros.Append("}\n");
                m_ihdMacros.NewLine();

                g_appArgs.GetDsnRpt().AddItem("bool RecvHostDataMarker()\n");
                m_ihdFuncDecl.Append("\tbool RecvHostDataMarker();\n");

                m_ihdMacros.Append("bool CPers%s%s::RecvHostDataMarker()\n", unitNameUc.c_str(), pMod->m_modName.Uc().c_str());

                m_ihdMacros.Append("{\n");

                m_ihdMacros.Append("\tassert_msg(c_%sAvlCntAvail, \"Runtime check failed in CPers%s::RecvHostDataMarker()"
                        " - expected RecvHostDataBusy() to have been called and not busy\");\n",
                        queIntf.m_queName.Lc().c_str(), pMod->m_modName.Uc().c_str());

                m_ihdMacros.Append("\tif (r_%sQue_front.m_ctl == HIF_DQ_NULL) {\n", queIntf.m_queName.Lc().c_str());
                m_ihdMacros.Append("\t\tc_%sTo%s_%sAvl = true;\n",
                        pMod->m_modName.Lc().c_str(), queIntf.m_modName.Uc().c_str(), queIntf.m_queName.Lc().c_str());
                m_ihdMacros.Append("\t\treturn true;\n");
                m_ihdMacros.Append("\t} else\n");
                m_ihdMacros.Append("\t\treturn false;\n");
                m_ihdMacros.Append("}\n");
                m_ihdMacros.NewLine();
        }

        for (size_t queIdx = 0; queIdx < pMod->m_queIntfList.size(); queIdx += 1) {
                CQueIntf & queIntf = pMod->m_queIntfList[queIdx];

                if (queIntf.m_queType != Pop) continue;

                m_ihdRegDecl.Append("\tbool c_%sTo%s_%sAvl;\n",
                        pMod->m_modName.Lc().c_str(), queIntf.m_modName.Uc().c_str(), queIntf.m_queName.Lc().c_str());
                ihdPreInstr.Append("\tc_%sTo%s_%sAvl = false;\n",
                        pMod->m_modName.Lc().c_str(), queIntf.m_modName.Uc().c_str(), queIntf.m_queName.Lc().c_str());
        }
        ihdPreInstr.NewLine();

        // pre reg 1x
        for (size_t queIdx = 0; queIdx < pMod->m_queIntfList.size(); queIdx += 1) {
                CQueIntf & queIntf = pMod->m_queIntfList[queIdx];

                if (queIntf.m_queType != Pop) continue;

                m_ihdPostInstr1x.Append("\tif (i_%sTo%s_%sRdy.read())\n\t\tm_%sQue.push(i_%sTo%s_%s);\n",
                        queIntf.m_modName.Lc().c_str(), pMod->m_modName.Uc().c_str(), queIntf.m_queName.Lc().c_str(),
                        queIntf.m_queName.Lc().c_str(),
                        queIntf.m_modName.Lc().c_str(), pMod->m_modName.Uc().c_str(), queIntf.m_queName.Lc().c_str());


                string structName = queIntf.m_typeName + (queIntf.m_bInclude ? "_inc" : "");
                ihdPostInstr.Append("\t%s c_ihdQue_front = (c_%sTo%s_%sAvl || r_ihdQue_empty) ? m_ihdQue.front() : r_ihdQue_front;\n",
                        structName.c_str(),
                        pMod->m_modName.Lc().c_str(), queIntf.m_modName.Uc().c_str(), queIntf.m_queName.Lc().c_str());

                ihdPostInstr.Append("\tbool c_ihdQue_empty = m_ihdQue.empty() && (c_%sTo%s_%sAvl || r_ihdQue_empty);\n",
                        pMod->m_modName.Lc().c_str(), queIntf.m_modName.Uc().c_str(), queIntf.m_queName.Lc().c_str());

                ihdPostInstr.Append("\n");
                ihdPostInstr.Append("\tif (!m_ihdQue.empty() && (c_%sTo%s_%sAvl || r_ihdQue_empty))\n",
                        pMod->m_modName.Lc().c_str(), queIntf.m_modName.Uc().c_str(), queIntf.m_queName.Lc().c_str());
                ihdPostInstr.Append("\t\tm_%sQue.pop();\n",
                        queIntf.m_queName.Lc().c_str());
        }
        ihdPostInstr.NewLine();
        m_ihdPostInstr1x.NewLine();

        // ram clocks
        for (size_t queIdx = 0; queIdx < pMod->m_queIntfList.size(); queIdx += 1) {
                CQueIntf & queIntf = pMod->m_queIntfList[queIdx];

                if (queIntf.m_queType != Pop) continue;

                if (pMod->m_clkRate == eClk1x)
                        m_ihdRamClock1x.Append("\tm_%sQue.clock(r_reset1x);\n",
                        queIntf.m_queName.Lc().c_str());
                else {
                        m_ihdRamClock1x.Append("\tm_%sQue.push_clock(r_reset1x);\n",
                                queIntf.m_queName.Lc().c_str());
                        m_ihdRamClock2x.Append("\tm_%sQue.pop_clock(c_reset2x);\n",
                                queIntf.m_queName.Lc().c_str());
                }

                ihdReg.Append("\tr_ihdQue_front = c_ihdQue_front;\n");
                ihdReg.Append("\tr_ihdQue_empty = %s || c_ihdQue_empty;\n", reset.c_str());
        }

        // assign registers
        for (size_t queIdx = 0; queIdx < pMod->m_queIntfList.size(); queIdx += 1) {
                CQueIntf & queIntf = pMod->m_queIntfList[queIdx];

                if (queIntf.m_queType != Pop) continue;

                ihdReg.Append("\tr_%sTo%s_%sAvl = c_%sTo%s_%sAvl;\n",
                        pMod->m_modName.Lc().c_str(), queIntf.m_modName.Uc().c_str(), queIntf.m_queName.Lc().c_str(),
                        pMod->m_modName.Lc().c_str(), queIntf.m_modName.Uc().c_str(), queIntf.m_queName.Lc().c_str());
        }

        // assign output ports
        for (size_t queIdx = 0; queIdx < pMod->m_queIntfList.size(); queIdx += 1) {
                CQueIntf & queIntf = pMod->m_queIntfList[queIdx];

                if (queIntf.m_queType != Pop) continue;

                ihdOut.Append("\to_%sTo%s_%sAvl = r_%sTo%s_%sAvl;\n",
                        pMod->m_modName.Lc().c_str(), queIntf.m_modName.Uc().c_str(), queIntf.m_queName.Lc().c_str(),
                        pMod->m_modName.Lc().c_str(), queIntf.m_modName.Uc().c_str(), queIntf.m_queName.Lc().c_str());
        }
        ihdOut.NewLine();

        g_appArgs.GetDsnRpt().EndLevel();
}

void
CDsnInfo::GenModOhdStatements(CInstance * pModInst)
{
        CModule * pMod = pModInst->m_pMod;

        if (pMod->m_queIntfList.size() == 0)
                return;

        CHtCode & ohdPreInstr = pMod->m_clkRate == eClk1x ? m_ohdPreInstr1x : m_ohdPreInstr2x;
        CHtCode & ohdPostInstr = pMod->m_clkRate == eClk1x ? m_ohdPostInstr1x : m_ohdPostInstr2x;
        CHtCode & ohdReg = pMod->m_clkRate == eClk1x ? m_ohdReg1x : m_ohdReg2x;
        CHtCode & ohdPostReg = pMod->m_clkRate == eClk1x ? m_ohdPostReg1x : m_ohdPostReg2x;
        CHtCode & ohdOut = pMod->m_clkRate == eClk1x ? m_ohdOut1x : m_ohdOut2x;

        // I/O declarations
        m_ohdIoDecl.Append("\t// Outbound host data\n");

        bool bOhm = false;
        for (size_t queIdx = 0; queIdx < pMod->m_queIntfList.size(); queIdx += 1) {
                CQueIntf & queIntf = pMod->m_queIntfList[queIdx];

                if (queIntf.m_queType == Pop) continue;
                bOhm = true;

                // out bound interface
                m_ohdIoDecl.Append("\tsc_out<bool> o_%sToHif_%sRdy;\n",
                        pMod->m_modName.Lc().c_str(), queIntf.m_queName.Lc().c_str());
                string structName = queIntf.m_typeName + (queIntf.m_bInclude ? "_inc" : "");
                m_ohdIoDecl.Append("\tsc_out<%s> o_%sToHif_%s;\n",
                        structName.c_str(),
                        pMod->m_modName.Lc().c_str(), queIntf.m_queName.Lc().c_str());
                m_ohdIoDecl.Append("\tsc_in<bool> i_hifTo%s_%sAvl;\n",
                        pMod->m_modName.Uc().c_str(), queIntf.m_queName.Lc().c_str());
                m_ohdIoDecl.NewLine();
        }

        if (!bOhm)
                return;

        g_appArgs.GetDsnRpt().AddLevel("Outbound Host Data\n");

        // Reg declarations
        for (size_t queIdx = 0; queIdx < pMod->m_queIntfList.size(); queIdx += 1) {
                CQueIntf & queIntf = pMod->m_queIntfList[queIdx];

                if (queIntf.m_queType == Pop) continue;

                m_ohdRegDecl.Append("\tbool r_%sTo%s_%sRdy;\n",
                        pMod->m_modName.Lc().c_str(), queIntf.m_modName.Uc().c_str(), queIntf.m_queName.Lc().c_str());
                string structName = queIntf.m_typeName + (queIntf.m_bInclude ? "_inc" : "");
                m_ohdRegDecl.Append("\t%s r_%sTo%s_%s;\n",
                        structName.c_str(), pMod->m_modName.Lc().c_str(), queIntf.m_modName.Uc().c_str(), queIntf.m_queName.Lc().c_str());
                m_ohdRegDecl.Append("\tht_uint6 r_%sAvlCnt;\n", queIntf.m_queName.Lc().c_str());
                m_ohdAvlCntChk.Append("\t\tassert(r_%sAvlCnt == 32);\n", queIntf.m_queName.Lc().c_str());
                m_ohdRegDecl.Append("\tbool r_%sAvlCntZero;\n", queIntf.m_queName.Lc().c_str());
        }
        m_ohdRegDecl.NewLine();

        // queue interface macros
        m_ohdMacros.NewLine();
        for (size_t queIdx = 0; queIdx < pMod->m_queIntfList.size(); queIdx += 1) {
                CQueIntf & queIntf = pMod->m_queIntfList[queIdx];

                if (queIntf.m_queType == Pop) continue;

                string unitNameUc = !g_appArgs.IsModuleUnitNamesEnabled() ? m_unitName.Uc() : "";

                string vcdModName = VA("Pers%s", pMod->m_modName.Uc().c_str());
                m_ohdRegDecl.Append("\tbool ht_noload c_%sAvlCntBusy;\n", queIntf.m_queName.Lc().c_str());
                m_ohdRegDecl.Append("\tbool ht_noload c_%sAvlCntAvail;\n", queIntf.m_queName.Lc().c_str());
                GenModTrace(eVcdUser, vcdModName, "SendHostDataBusy()", VA("c_%sAvlCntBusy", queIntf.m_queName.Lc().c_str()));
                ohdPreInstr.Append("\tc_%sAvlCntAvail = false;\n", queIntf.m_queName.Lc().c_str());

                ohdPostReg.Append("#ifdef _HTV\n");
                ohdPostReg.Append("\tc_%sAvlCntBusy = r_%sAvlCntZero;\n", queIntf.m_queName.Lc().c_str(), queIntf.m_queName.Lc().c_str());
                ohdPostReg.Append("#else\n");
                ohdPostReg.Append("\tc_%sAvlCntBusy = r_%sAvlCntZero || (%s_RND_RETRY && !!(g_rndRetry() & 1));\n",
                        queIntf.m_queName.Lc().c_str(),
                        queIntf.m_queName.Lc().c_str(),
                        pModInst->m_instName.Upper().c_str());
                ohdPostReg.Append("#endif\n");

                g_appArgs.GetDsnRpt().AddItem("bool SendHostDataBusy()\n");
                m_ohdFuncDecl.Append("\tbool SendHostDataBusy();\n");
                m_ohdMacros.Append("bool CPers%s%s::SendHostDataBusy()\n", unitNameUc.c_str(), pMod->m_modName.Uc().c_str());
                m_ohdMacros.Append("{\n");

                m_ohdMacros.Append("\tc_%sAvlCntAvail = !c_%sAvlCntBusy;\n", queIntf.m_queName.Lc().c_str(), queIntf.m_queName.Lc().c_str());
                m_ohdMacros.Append("\treturn c_%sAvlCntBusy;\n", queIntf.m_queName.Lc().c_str());

                m_ohdMacros.Append("}\n");
                m_ohdMacros.NewLine();

                g_appArgs.GetDsnRpt().AddItem("bool SendHostDataFullIn(ht_uint6 cnt)\n");
                m_ohdFuncDecl.Append("\tbool SendHostDataFullIn(ht_uint6 cnt);\n");
                m_ohdMacros.Append("bool CPers%s%s::SendHostDataFullIn(ht_uint6 cnt)\n", unitNameUc.c_str(), pMod->m_modName.Uc().c_str());
                m_ohdMacros.Append("{\n");

                m_ohdMacros.Append("#ifdef _HTV\n");
                m_ohdMacros.Append("\treturn r_%sAvlCnt <= cnt;\n", queIntf.m_queName.Lc().c_str());
                m_ohdMacros.Append("#else\n");
                m_ohdMacros.Append("\treturn (c_%sAvlCntBusy = (r_%sAvlCnt <= cnt))\n",
                        queIntf.m_queName.Lc().c_str(),
                        queIntf.m_queName.Lc().c_str());
                m_ohdMacros.Append("\t\t|| (%s_RND_RETRY && !!(g_rndRetry() & 1));\n",
                        pModInst->m_instName.Upper().c_str());
                m_ohdMacros.Append("#endif\n");
                m_ohdMacros.Append("}\n");
                m_ohdMacros.NewLine();

                g_appArgs.GetDsnRpt().AddItem("void SendHostData(uint64_t data)\n");
                m_ohdFuncDecl.Append("\tvoid SendHostData(uint64_t data);\n");
                m_ohdMacros.Append("void CPers%s%s::SendHostData(uint64_t data)\n",
                        unitNameUc.c_str(), pMod->m_modName.Uc().c_str());

                m_ohdMacros.Append("{\n");
                m_ohdMacros.Append("\tassert_msg(c_%sTo%s_%sRdy == false, \"Runtime check failed in CPers%s::SendHostData()"
                        " - an HT outbound host data routine was already called\");\n",
                        pMod->m_modName.Lc().c_str(), queIntf.m_modName.Uc().c_str(), queIntf.m_queName.Lc().c_str(),
                        pMod->m_modName.Uc().c_str());
                m_ohdMacros.Append("\tassert_msg(c_%sAvlCntAvail, \"Runtime check failed in CPers%s::SendHostData()"
                        " - expected SendHostDataBusy() to have been called and not busy\");\n",
                        queIntf.m_queName.Lc().c_str(), pMod->m_modName.Uc().c_str());
                m_ohdMacros.Append("\tc_%sTo%s_%sRdy = true;\n",
                        pMod->m_modName.Lc().c_str(), queIntf.m_modName.Uc().c_str(), queIntf.m_queName.Lc().c_str());
                m_ohdMacros.Append("\tc_%sTo%s_%s.m_ctl = HIF_DQ_DATA;\n",
                        pMod->m_modName.Lc().c_str(), queIntf.m_modName.Uc().c_str(), queIntf.m_queName.Lc().c_str());
                m_ohdMacros.Append("\tc_%sTo%s_%s.m_data = data;\n",
                        pMod->m_modName.Lc().c_str(), queIntf.m_modName.Uc().c_str(), queIntf.m_queName.Lc().c_str());
                m_ohdMacros.Append("}\n");
                m_ohdMacros.NewLine();

                g_appArgs.GetDsnRpt().AddItem("void SendHostDataMarker()\n");
                m_ohdFuncDecl.Append("\tvoid SendHostDataMarker();\n");
                m_ohdMacros.Append("void CPers%s%s::SendHostDataMarker()\n",
                        unitNameUc.c_str(), pMod->m_modName.Uc().c_str());
                m_ohdMacros.Append("{\n");
                m_ohdMacros.Append("\tassert_msg(c_%sTo%s_%sRdy == false, \"Runtime check failed in CPers%s::SendHostDataMarker()"
                        " - an HT outbound host data routine was already called\");\n",
                        pMod->m_modName.Lc().c_str(), queIntf.m_modName.Uc().c_str(), queIntf.m_queName.Lc().c_str(),
                        pMod->m_modName.Uc().c_str());
                m_ohdMacros.Append("\tassert_msg(c_%sAvlCntAvail, \"Runtime check failed in CPers%s::SendHostDataMarker()"
                        " - expected SendHostDataBusy() to have been called and not busy\");\n",
                        queIntf.m_queName.Lc().c_str(), pMod->m_modName.Uc().c_str());
                m_ohdMacros.Append("\tc_%sTo%s_%sRdy = true;\n",
                        pMod->m_modName.Lc().c_str(), queIntf.m_modName.Uc().c_str(), queIntf.m_queName.Lc().c_str());
                m_ohdMacros.Append("\tc_%sTo%s_%s.m_ctl = HIF_DQ_NULL;\n",
                        pMod->m_modName.Lc().c_str(), queIntf.m_modName.Uc().c_str(), queIntf.m_queName.Lc().c_str());
                m_ohdMacros.Append("\tc_%sTo%s_%s.m_data = 0;\n",
                        pMod->m_modName.Lc().c_str(), queIntf.m_modName.Uc().c_str(), queIntf.m_queName.Lc().c_str());
                m_ohdMacros.Append("}\n");

                g_appArgs.GetDsnRpt().AddItem("void FlushHostData()\n");
                m_ohdFuncDecl.Append("\tvoid FlushHostData();\n");
                m_ohdMacros.NewLine();
                m_ohdMacros.Append("void CPers%s%s::FlushHostData()\n",
                        unitNameUc.c_str(), pMod->m_modName.Uc().c_str());
                m_ohdMacros.Append("{\n");
                m_ohdMacros.Append("\tassert_msg(c_%sTo%s_%sRdy == false, \"Runtime check failed in CPers%s::FlushHostData()"
                        " - an HT outbound host data routine was already called\");\n",
                        pMod->m_modName.Lc().c_str(), queIntf.m_modName.Uc().c_str(), queIntf.m_queName.Lc().c_str(),
                        pMod->m_modName.Uc().c_str());
                m_ohdMacros.Append("\tassert_msg(c_%sAvlCntAvail, \"Runtime check failed in CPers%s::FlushHostData()"
                        " - expected SendHostDataBusy() to have been called and not busy\");\n",
                        queIntf.m_queName.Lc().c_str(), pMod->m_modName.Uc().c_str());
                m_ohdMacros.Append("\tc_%sTo%s_%sRdy = true;\n",
                        pMod->m_modName.Lc().c_str(), queIntf.m_modName.Uc().c_str(), queIntf.m_queName.Lc().c_str());
                m_ohdMacros.Append("\tc_%sTo%s_%s.m_ctl = HIF_DQ_FLSH;\n",
                        pMod->m_modName.Lc().c_str(), queIntf.m_modName.Uc().c_str(), queIntf.m_queName.Lc().c_str());
                m_ohdMacros.Append("\tc_%sTo%s_%s.m_data = 0;\n",
                        pMod->m_modName.Lc().c_str(), queIntf.m_modName.Uc().c_str(), queIntf.m_queName.Lc().c_str());
                m_ohdMacros.Append("}\n");
                m_ohdMacros.NewLine();
        }

        for (size_t queIdx = 0; queIdx < pMod->m_queIntfList.size(); queIdx += 1) {
                CQueIntf & queIntf = pMod->m_queIntfList[queIdx];

                if (queIntf.m_queType == Pop) continue;

                m_ohdRegDecl.Append("\tbool c_%sTo%s_%sRdy;\n",
                        pMod->m_modName.Lc().c_str(), queIntf.m_modName.Uc().c_str(), queIntf.m_queName.Lc().c_str());
                ohdPreInstr.Append("\tc_%sTo%s_%sRdy = false;\n",
                        pMod->m_modName.Lc().c_str(), queIntf.m_modName.Uc().c_str(), queIntf.m_queName.Lc().c_str());

                string structName = queIntf.m_typeName + (queIntf.m_bInclude ? "_inc" : "");
                m_ohdRegDecl.Append("\t%s c_%sTo%s_%s;\n",
                        structName.c_str(), pMod->m_modName.Lc().c_str(), queIntf.m_modName.Uc().c_str(), queIntf.m_queName.Lc().c_str());
                ohdPreInstr.Append("\tc_%sTo%s_%s = 0;\n", pMod->m_modName.Lc().c_str(), queIntf.m_modName.Uc().c_str(), queIntf.m_queName.Lc().c_str());
        }
        ohdPreInstr.NewLine();

        for (size_t queIdx = 0; queIdx < pMod->m_queIntfList.size(); queIdx += 1) {
                CQueIntf & queIntf = pMod->m_queIntfList[queIdx];

                if (queIntf.m_queType == Pop) continue;

                m_ohdRegDecl.Append("\tht_uint6 c_%sAvlCnt;\n",
                        queIntf.m_queName.Lc().c_str());
                m_ohdRegDecl.Append("\tbool c_%sAvlCntZero;\n",
                        queIntf.m_queName.Lc().c_str());

                if (pMod->m_clkRate == eClk1x)
                        ohdPostInstr.Append("\tc_%sAvlCnt = r_%sAvlCnt + i_%sTo%s_%sAvl.read() - c_%sTo%s_%sRdy;\n",
                        queIntf.m_queName.Lc().c_str(), queIntf.m_queName.Lc().c_str(), queIntf.m_modName.Lc().c_str(),
                        pMod->m_modName.Uc().c_str(), queIntf.m_queName.Lc().c_str(),
                        pMod->m_modName.Lc().c_str(), queIntf.m_modName.Uc().c_str(), queIntf.m_queName.Lc().c_str());
                else
                        ohdPostInstr.Append("\tc_%sAvlCnt = r_%sAvlCnt + (i_%sTo%s_%sAvl.read() && r_phase) - c_%sTo%s_%sRdy;\n",
                        queIntf.m_queName.Lc().c_str(), queIntf.m_queName.Lc().c_str(), queIntf.m_modName.Lc().c_str(),
                        pMod->m_modName.Uc().c_str(), queIntf.m_queName.Lc().c_str(),
                        pMod->m_modName.Lc().c_str(), queIntf.m_modName.Uc().c_str(), queIntf.m_queName.Lc().c_str());

                ohdPostInstr.Append("\tc_%sAvlCntZero = c_%sAvlCnt == 0;\n",
                        queIntf.m_queName.Lc().c_str(), queIntf.m_queName.Lc().c_str());
        }
        ohdPostInstr.NewLine();

        // assign registers
        for (size_t queIdx = 0; queIdx < pMod->m_queIntfList.size(); queIdx += 1) {
                CQueIntf & queIntf = pMod->m_queIntfList[queIdx];

                if (queIntf.m_queType == Pop) continue;

                ohdReg.Append("\tr_%sTo%s_%sRdy = c_%sTo%s_%sRdy;\n",
                        pMod->m_modName.Lc().c_str(), queIntf.m_modName.Uc().c_str(), queIntf.m_queName.Lc().c_str(),
                        pMod->m_modName.Lc().c_str(), queIntf.m_modName.Uc().c_str(), queIntf.m_queName.Lc().c_str());
                ohdReg.Append("\tr_%sTo%s_%s = c_%sTo%s_%s;\n",
                        pMod->m_modName.Lc().c_str(), queIntf.m_modName.Uc().c_str(), queIntf.m_queName.Lc().c_str(),
                        pMod->m_modName.Lc().c_str(), queIntf.m_modName.Uc().c_str(), queIntf.m_queName.Lc().c_str());
                ohdReg.Append("\tr_%sAvlCnt = r_reset1x ? (ht_uint6)32 : c_%sAvlCnt;\n",
                        queIntf.m_queName.Lc().c_str(),
                        queIntf.m_queName.Lc().c_str());
                ohdReg.Append("\tr_%sAvlCntZero = !r_reset1x && c_%sAvlCntZero;\n",
                        queIntf.m_queName.Lc().c_str(),
                        queIntf.m_queName.Lc().c_str());
        }

        // assign output ports
        for (size_t queIdx = 0; queIdx < pMod->m_queIntfList.size(); queIdx += 1) {
                CQueIntf & queIntf = pMod->m_queIntfList[queIdx];

                if (queIntf.m_queType == Pop) continue;

                ohdOut.Append("\to_%sTo%s_%sRdy = r_%sTo%s_%sRdy;\n",
                        pMod->m_modName.Lc().c_str(), queIntf.m_modName.Uc().c_str(), queIntf.m_queName.Lc().c_str(),
                        pMod->m_modName.Lc().c_str(), queIntf.m_modName.Uc().c_str(), queIntf.m_queName.Lc().c_str());
                ohdOut.Append("\to_%sTo%s_%s = r_%sTo%s_%s;\n",
                        pMod->m_modName.Lc().c_str(), queIntf.m_modName.Uc().c_str(), queIntf.m_queName.Lc().c_str(),
                        pMod->m_modName.Lc().c_str(), queIntf.m_modName.Uc().c_str(), queIntf.m_queName.Lc().c_str());
        }
        ohdOut.NewLine();

        g_appArgs.GetDsnRpt().EndLevel();
}
