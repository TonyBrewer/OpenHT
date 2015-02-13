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

struct CMifModInst {
	CMifModInst(CModMemPort &modMemPort, CModInst &modInst) : m_pModMemPort(&modMemPort), m_pModInst(&modInst) {}

	CModMemPort *	m_pModMemPort;
	CModInst *		m_pModInst;
};

void CDsnInfo::GenerateMifFiles(int mifId)
{
	bool bClk2x = false;
	bool bMifRead = false;
	bool bMifWrite = false;
	int mifIntfCnt = 0;
	vector<CMifModInst>	mifInstList;

	bool bMultiQwSupport = g_appArgs.GetCoproc() == hc2 ||
			       g_appArgs.GetCoproc() == hc2ex ||
			       g_appArgs.GetCoproc() == wx690 ||
			       g_appArgs.GetCoproc() == wx2k;

	for (size_t dsnInstIdx = 0; dsnInstIdx < m_dsnInstList.size(); dsnInstIdx += 1) {
		CModInst &modInst = m_dsnInstList[dsnInstIdx];
		if (!modInst.m_pMod->m_bIsUsed) continue;

		for (size_t memPortIdx = 0; memPortIdx < modInst.m_pMod->m_memPortList.size(); memPortIdx += 1) {
			CModMemPort &modMemPort = *modInst.m_pMod->m_memPortList[memPortIdx];

			if (modInst.m_instParams.m_memPortList[memPortIdx] != mifId)
				continue;

			mifInstList.push_back(CMifModInst(modMemPort, modInst));

			mifIntfCnt += 1;

			bClk2x |= modInst.m_pMod->m_clkRate == eClk2x;
			bMifRead |= modMemPort.m_bRead;
			bMifWrite |= modMemPort.m_bWrite;
		}
	}

	// Max tid width check in GenModMif assumes mifTidBits is 4 or less.
	int mifTidBits = 0;
	switch (mifIntfCnt) {
	case 1: 
		mifTidBits = 0; break;
	case 2: 
		mifTidBits = 1; break;
	case 3: case 4:
		mifTidBits = 2; break;
	case 5: case 6: case 7: case 8:
		mifTidBits = 3; break;
	case 9: case 10: case 11: case 12: case 13: case 14: case 15:
		mifTidBits = 4; break;
	default:
		ParseMsg(Error, "MIF interface %d is connected to %d module instances.\n  (Maximum supported instances per MIF is %d)",
			mifId, mifIntfCnt, 8);
		break;
	}

	char mifName[16];
	sprintf(mifName, "%d", mifId);

	string name = (string)"Mif" + mifName;
	string fileName = g_appArgs.GetOutputFolder() + "/Pers" + m_unitName.Uc() + name + ".h";

	CHtFile incFile(fileName, "w");
	CHtCode incCode(incFile);

	GenPersBanner(incFile, m_unitName.Uc().c_str(), name.c_str(), true);

	string vcdModName = VA("PersMif%d", mifId);
	fprintf(incFile, "SC_MODULE(CPers%sMif%d) {\n", m_unitName.Uc().c_str(), mifId);
	fprintf(incFile, "\n");
	fprintf(incFile, "\tht_attrib(keep_hierarchy, CPers%sMif%d, \"true\");\n", m_unitName.Uc().c_str(), mifId);
	fprintf(incFile, "\n");
	GenModDecl(eVcdAll, incCode, vcdModName, "sc_in<bool>", VA("i_clock1x"));

	if (bClk2x) {
		GenModDecl(eVcdAll, incCode, vcdModName, "sc_in<bool>", VA("i_clock2x"));
	}
	GenModDecl(eVcdAll, incCode, vcdModName, "sc_in<bool>", VA("i_reset"));
	fprintf(incFile, "\n");

	for (size_t instIdx = 0; instIdx < mifInstList.size(); instIdx += 1) {
		CMifModInst & mifInst = mifInstList[instIdx];
		CModInst & modInst = *mifInst.m_pModInst;
		CModMemPort & modMemPort = *mifInst.m_pModMemPort;

		fprintf(incFile, "\t// Pers%s%s%d Interface\n",
			m_unitName.Uc().c_str(), modInst.m_instName.Uc().c_str(), mifId);
		GenModDecl(eVcdAll, incCode, vcdModName, "sc_in<bool>",
			VA("i_%sP%dToMif%d_reqRdy", modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx, mifId) );
		GenModDecl(eVcdAll, incCode, vcdModName, "sc_in<CMemRdWrReqIntf>",
			VA("i_%sP%dToMif%d_req", modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx, mifId) );
		GenModDecl(eVcdAll, incCode, vcdModName, "sc_out<bool>",
			VA("o_mif%dTo%sP%d_reqAvl", mifId, modInst.m_instName.Uc().c_str(), modMemPort.m_portIdx) );
		fprintf(incFile, "\n");

		if (modMemPort.m_bRead) {
			GenModDecl(eVcdAll, incCode, vcdModName, "sc_out<bool>", VA("o_mif%dTo%sP%d_rdRspRdy", mifId, modInst.m_instName.Uc().c_str(), modMemPort.m_portIdx) );
			GenModDecl(eVcdAll, incCode, vcdModName, "sc_out<CMemRdRspIntf>", VA("o_mif%dTo%sP%d_rdRsp", mifId, modInst.m_instName.Uc().c_str(), modMemPort.m_portIdx) );
			fprintf(incFile, "\n");
		}

		if (modMemPort.m_bWrite) {
			GenModDecl(eVcdAll, incCode, vcdModName, "sc_out<bool>", VA("o_mif%dTo%sP%d_wrRspRdy", mifId, modInst.m_instName.Uc().c_str(), modMemPort.m_portIdx) );
			GenModDecl(eVcdAll, incCode, vcdModName, "sc_out<sc_uint<MIF_TID_W> >", VA("o_mif%dTo%sP%d_wrRspTid", mifId, modInst.m_instName.Uc().c_str(), modMemPort.m_portIdx) );
			fprintf(incFile, "\n");
		}
	}

	fprintf(incFile, "\t// Interface to memory xbar\n");
	GenModDecl(eVcdAll, incCode, vcdModName, "sc_out<bool>", VA("o_mif%dToXbar%d_reqRdy", mifId, mifId) );
	GenModDecl(eVcdAll, incCode, vcdModName, "sc_out<CMemRdWrReqIntf>", VA("o_mif%dToXbar%d_req", mifId, mifId) );

	GenModDecl(eVcdAll, incCode, vcdModName, "sc_in<bool>", VA("i_xbar%dToMif%d_reqFull", mifId, mifId) );
	fprintf(incFile, "\n");

	GenModDecl(eVcdAll, incCode, vcdModName, "sc_out<bool>", VA("o_mif%dToXbar%d_rdRspFull", mifId, mifId) );
	GenModDecl(eVcdAll, incCode, vcdModName, "sc_out<bool>", VA("o_mif%dToXbar%d_wrRspFull", mifId, mifId) );
	fprintf(incFile, "\n");

	if (bMifRead) {
		GenModDecl(eVcdAll, incCode, vcdModName, "sc_in<bool>", VA("i_xbar%dToMif%d_rdRspRdy", mifId, mifId) );
	 	GenModDecl(eVcdAll, incCode, vcdModName, "sc_in<CMemRdRspIntf>", VA("i_xbar%dToMif%d_rdRsp", mifId, mifId) );
	} else {
		GenModDecl(eVcdAll, incCode, vcdModName, "sc_in<bool> ht_noload", VA("i_xbar%dToMif%d_rdRspRdy", mifId, mifId) );
		GenModDecl(eVcdAll, incCode, vcdModName, "sc_in<CMemRdRspIntf> ht_noload", VA("i_xbar%dToMif%d_rdRsp", mifId, mifId) );
	}
	fprintf(incFile, "\n");

	if (bMifWrite) {
		GenModDecl(eVcdAll, incCode, vcdModName, "sc_in<bool>", VA("i_xbar%dToMif%d_wrRspRdy", mifId, mifId) );
		GenModDecl(eVcdAll, incCode, vcdModName, "sc_in<sc_uint<MIF_TID_W> >", VA("i_xbar%dToMif%d_wrRspTid", mifId, mifId) );
	} else {
		GenModDecl(eVcdAll, incCode, vcdModName, "sc_in<bool> ht_noload", VA("i_xbar%dToMif%d_wrRspRdy", mifId, mifId) );
		GenModDecl(eVcdAll, incCode, vcdModName, "sc_in<sc_uint<MIF_TID_W> > ht_noload", VA("i_xbar%dToMif%d_wrRspTid", mifId, mifId) );
	}
	fprintf(incFile, "\n");

	fprintf(incFile, "\t//////////////////////////////\n");
	fprintf(incFile, "\t// State declarations \n");
	fprintf(incFile, "\n");

	for (size_t instIdx = 0; instIdx < mifInstList.size(); instIdx += 1) {
		CMifModInst & mifInst = mifInstList[instIdx];
		CModInst &modInst = *mifInst.m_pModInst;
		CModMemPort & modMemPort = *mifInst.m_pModMemPort;


		if (!modMemPort.m_bRead && !modMemPort.m_bWrite || modMemPort.m_queueW == 0) continue;

		fprintf(incFile, "\tht_dist_que<CMemRdWrReqIntf, %d> m_%sP%dReqQue;\n",
			modMemPort.m_queueW, modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx);
	}
	fprintf(incFile, "\n");

	GenModDecl(eVcdAll, incCode, vcdModName, "bool", "r_reset1x");
	if (bClk2x) {
		GenModDecl(eVcdAll, incCode, vcdModName, "sc_signal<bool>", "c_reset1x");
		GenModDecl(eVcdAll, incCode, vcdModName, "bool", "r_phase");
		fprintf(incFile, "\n");
	}

	for (size_t instIdx = 0; instIdx < mifInstList.size(); instIdx += 1) {
		CMifModInst & mifInst = mifInstList[instIdx];
		CModInst &modInst = *mifInst.m_pModInst;
		CModMemPort & modMemPort = *mifInst.m_pModMemPort;

		CModule * pMod = modInst.m_pMod;

		if (!modMemPort.m_bRead && !modMemPort.m_bWrite) continue;

		fprintf(incFile, "\n");

		if (modMemPort.m_queueW == 0) {
			if (pMod->m_clkRate == eClk1x)
				fprintf(incFile, "\tbool r_%sToMif_reqRdy;\n", modInst.m_instName.Lc().c_str());
			else {
				fprintf(incFile, "\tsc_signal<bool> r_%sP%dToMif_reqRdy;\n", modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx);
				fprintf(incFile, "\tsc_signal<bool> r_%sP%dToMif_reqRdy_2x;\n", modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx);
			}
			fprintf(incFile, "\tCMemRdWrReqIntf r_%sP%dToMif_req;\n", 
				modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx);
			if (pMod->m_clkRate == eClk2x)
				fprintf(incFile, "\tsc_signal<CMemRdWrReqIntf> r_%sP%dToMif_req_2x;\n", 
					modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx);
		} else {
			if (pMod->m_clkRate == eClk2x) {
				GenModDecl(eVcdAll, incCode, vcdModName, "bool", VA("r_%sP%dReqQueEmpty", modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx));
				GenModDecl(eVcdAll, incCode, vcdModName, "CMemRdWrReqIntf", VA("r_%sP%dReqQueFront", modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx));
			}
		}

		GenModDecl(eVcdAll, incCode, vcdModName, "bool", VA("r_mifTo%sP%d_reqAvl", modInst.m_instName.Uc().c_str(), modMemPort.m_portIdx));

		if (pMod->m_clkRate == eClk2x)
			fprintf(incFile, "\tsc_signal<bool> r_%sP%dReqSel;\n", modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx);
	}
	fprintf(incFile, "\n");

	GenModDecl(eVcdAll, incCode, vcdModName, VA("ht_uint%d", mifIntfCnt), "r_reqRr");

	for (size_t instIdx = 0; instIdx < mifInstList.size(); instIdx += 1) {
		CMifModInst & mifInst = mifInstList[instIdx];
		CModInst &modInst = *mifInst.m_pModInst;
		CModMemPort & modMemPort = *mifInst.m_pModMemPort;


		if (!modMemPort.m_bRead && !modMemPort.m_bWrite)
			continue;

		if (modMemPort.m_bMultiWr)
			GenModDecl(eVcdAll, incCode, vcdModName, "ht_uint3", VA("r_%sP%dRemQwCnt", modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx));
	}

	GenModDecl(eVcdNone, incCode, vcdModName, "bool", "r_mifToXbar_reqRdy");
	GenModDecl(eVcdNone, incCode, vcdModName, "CMemRdWrReqIntf", "r_mifToXbar_req");

	GenModDecl(eVcdAll, incCode, vcdModName, "bool", "r_xbarToMif_reqFull");
	fprintf(incFile, "\n");

	if (bMifRead) {
		GenModDecl(eVcdAll, incCode, vcdModName, "bool", "r_xbarToMif_rdRspRdy");
		GenModDecl(eVcdAll, incCode, vcdModName, "CMemRdRspIntf", "r_xbarToMif_rdRsp");
		fprintf(incFile, "\n");
	}

	if (bMifWrite) {
		GenModDecl(eVcdAll, incCode, vcdModName, "bool", "r_xbarToMif_wrRspRdy");
		GenModDecl(eVcdAll, incCode, vcdModName, "sc_uint<MIF_TID_W>", "r_xbarToMif_wrRspTid");
		fprintf(incFile, "\n");
	}

	for (size_t instIdx = 0; instIdx < mifInstList.size(); instIdx += 1) {
		CMifModInst & mifInst = mifInstList[instIdx];
		CModInst &modInst = *mifInst.m_pModInst;
		CModMemPort & modMemPort = *mifInst.m_pModMemPort;


		if (!modMemPort.m_bRead && !modMemPort.m_bWrite) continue;

		if (modMemPort.m_bRead) {
			if (modInst.m_pMod->m_clkRate == eClk1x)
				GenModDecl(eVcdNone, incCode, vcdModName, "bool", VA("r_mifTo%sP%d_rdRspRdy", modInst.m_instName.Uc().c_str(), modMemPort.m_portIdx) );
			else {
				GenModDecl(eVcdNone, incCode, vcdModName, "sc_signal<bool>", VA("r_mifTo%sP%d_rdRspRdy", modInst.m_instName.Uc().c_str(), modMemPort.m_portIdx) );
				GenModDecl(eVcdNone, incCode, vcdModName, "bool", VA("r_mifTo%sP%d_rdRspRdy_2x", modInst.m_instName.Uc().c_str(), modMemPort.m_portIdx) );
			}
			GenModDecl(eVcdNone, incCode, vcdModName, "CMemRdRspIntf", VA("r_mifTo%sP%d_rdRsp", modInst.m_instName.Uc().c_str(), modMemPort.m_portIdx) );
			fprintf(incFile, "\n");
		}

		if (modMemPort.m_bWrite) {
			if (modInst.m_pMod->m_clkRate == eClk1x)
				GenModDecl(eVcdNone, incCode, vcdModName, "bool", VA("r_mifTo%sP%d_wrRspRdy", modInst.m_instName.Uc().c_str(), modMemPort.m_portIdx) );
			else {
				GenModDecl(eVcdNone, incCode, vcdModName, "sc_signal<bool>", VA("r_mifTo%sP%d_wrRspRdy", modInst.m_instName.Uc().c_str(), modMemPort.m_portIdx) );
				GenModDecl(eVcdNone, incCode, vcdModName, "bool", VA("r_mifTo%sP%d_wrRspRdy_2x", modInst.m_instName.Uc().c_str(), modMemPort.m_portIdx) );
			}
			GenModDecl(eVcdNone, incCode, vcdModName, "sc_uint<MIF_TID_W>", VA("r_mifTo%sP%d_wrRspTid", modInst.m_instName.Uc().c_str(), modMemPort.m_portIdx) );
			fprintf(incFile, "\n");
		}
	}

	fprintf(incFile, "\t//////////////////////////////\n");
	fprintf(incFile, "\t// Method declarations \n");
	fprintf(incFile, "\n");

	fprintf(incFile, "\tvoid Pers%sMif1x();\n", m_unitName.Uc().c_str());

	if (bClk2x)
		fprintf(incFile, "\tvoid Pers%sMif2x();\n", m_unitName.Uc().c_str());
	fprintf(incFile, "\n");

	fprintf(incFile, "\t//////////////////////////////\n");
	fprintf(incFile, "\t// SystemC Contructor \n");
	fprintf(incFile, "\n");

	fprintf(incFile, "\tSC_CTOR(CPers%sMif%s) {\n", m_unitName.Uc().c_str(), mifName);

	bool bNewLine = false;
	for (size_t instIdx = 0; instIdx < mifInstList.size(); instIdx += 1) {
		CMifModInst & mifInst = mifInstList[instIdx];
		CModInst &modInst = *mifInst.m_pModInst;
		CModMemPort & modMemPort = *mifInst.m_pModMemPort;

		if (!modMemPort.m_bRead && !modMemPort.m_bWrite) continue;
		if (modMemPort.m_queueW == 0 || modInst.m_pMod->m_clkRate == eClk1x) continue;

		if (!bNewLine)
			fprintf(incFile, "#\t\tifndef _HTV\n");

		fprintf(incFile, "\t\tr_%sP%dReqQueEmpty = true;\n", modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx);
		bNewLine = true;
	}

	if (bNewLine) {
		fprintf(incFile, "#\t\tendif\n");
		fprintf(incFile, "\n");
	}

	fprintf(incFile, "\t\tSC_METHOD(Pers%sMif1x);\n", m_unitName.Uc().c_str());
	fprintf(incFile, "\t\tsensitive << i_clock1x.pos();\n");
	fprintf(incFile, "\n");

	if (bClk2x) {
		fprintf(incFile, "\t\tSC_METHOD(Pers%sMif2x);\n", m_unitName.Uc().c_str());
		fprintf(incFile, "\t\tsensitive << i_clock2x.pos();\n");
	}

	fprintf(incFile, "\t}\n");

	if (g_appArgs.IsVcdUserEnabled() || g_appArgs.IsVcdAllEnabled()) {
		fprintf(incFile, "\n");
		fprintf(incFile, "\t#ifndef _HTV\n");
		fprintf(incFile, "\tvoid start_of_simulation() {\n");

		m_vcdSos.Write(incFile);

		fprintf(incFile, "\t}\n");
		fprintf(incFile, "\t#endif\n");
	}

	fprintf(incFile, "};\n");

	incFile.FileClose();

	////////////////////////////////////////////////////////////////
	// Generate cpp file

	fileName = g_appArgs.GetOutputFolder() + "/Pers" + m_unitName.Uc() + "Mif" + mifName + ".cpp";

	CHtFile cppFile(fileName, "w");

	GenPersBanner(cppFile, m_unitName.Uc().c_str(), name.c_str(), false);

	fprintf(cppFile, "void\n");
	fprintf(cppFile, "CPers%sMif%s::Pers%sMif1x()\n", m_unitName.Uc().c_str(), mifName, m_unitName.Uc().c_str());
	fprintf(cppFile, "{\n");
	fprintf(cppFile, "\n");
	fprintf(cppFile, "\t/////////////////////////\n");
	fprintf(cppFile, "\t// Combinatorial Logic\n");
	fprintf(cppFile, "\n");
	fprintf(cppFile, "\t// Arbitrate Requests\n");

	for (size_t instIdx = 0; instIdx < mifInstList.size(); instIdx += 1) {
		CMifModInst & mifInst = mifInstList[instIdx];
		CModInst &modInst = *mifInst.m_pModInst;
		CModMemPort & modMemPort = *mifInst.m_pModMemPort;


		if (!modMemPort.m_bRead && !modMemPort.m_bWrite) continue;

		if (modMemPort.m_queueW == 0)
			fprintf(cppFile, "\tbool c_%sP%dReqRdy = r_%sP%dToMif_reqRdy;\n",
				modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx, modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx);
		else if (modInst.m_pMod->m_clkRate == eClk1x)
			fprintf(cppFile, "\tbool c_%sP%dReqRdy = !m_%sP%dReqQue.empty();\n",
				modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx, modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx);
		else
			fprintf(cppFile, "\tbool c_%sP%dReqRdy = !r_%sP%dReqQueEmpty;\n",
				modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx, modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx);
	}

	fprintf(cppFile, "\n");

	for (size_t instIdx = 0, i = 0; instIdx < mifInstList.size(); instIdx += 1) {
		CMifModInst & mifInst = mifInstList[instIdx];
		CModInst &modInst = *mifInst.m_pModInst;
		CModMemPort & modMemPort = *mifInst.m_pModMemPort;


		if (!modMemPort.m_bRead && !modMemPort.m_bWrite) continue;

		if (1 < mifIntfCnt) {
			fprintf(cppFile, "\tbool c_%sP%dReqSel = !r_xbarToMif_reqFull && c_%sP%dReqRdy && ((r_reqRr & 0x%lx) != 0 || !(\n",
				modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx, modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx, 1ul << i); 
		} else {
			fprintf(cppFile, "\tbool c_%sP%dReqSel = !r_xbarToMif_reqFull && c_%sP%dReqRdy && ((r_reqRr & 0x%lx) != 0);",
				modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx, modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx, 1ul << i);
		}

		for (int j = 1; j < mifIntfCnt; j += 1) {
			int k = (i + j) % mifIntfCnt;

			uint32_t mask1 = (1ul << mifIntfCnt)-1;
			uint32_t mask2 = ((1ul << j)-1) << (i+1);
			uint32_t mask3 = (mask2 & mask1) | (mask2 >> mifIntfCnt);

			CMifModInst & mifInstK = mifInstList[k];
			CModInst &modInstK = *mifInstK.m_pModInst;
			CModMemPort & modMemPortK = *mifInstK.m_pModMemPort;

			fprintf(cppFile, "\t\t(c_%sP%dReqRdy && (r_reqRr & 0x%x) != 0)%s\n",
				modInstK.m_instName.Lc().c_str(), modMemPortK.m_portIdx, mask3, j == mifIntfCnt-1 ? "));" : " ||");
		}

		fprintf(cppFile, "\tht_attrib(keep, r_mifTo%sP%d_reqAvl, \"true\");\n", modInst.m_instName.Uc().c_str(), modMemPort.m_portIdx);
		fprintf(cppFile, "\n");

		i += 1;
	}

	fprintf(cppFile, "\tbool c_mifToXbar_reqRdy = false;\n");
	fprintf(cppFile, "\tCMemRdWrReqIntf c_mifToXbar_req;\n");
	fprintf(cppFile, "\tc_mifToXbar_req.m_tid = 0;\n");
	fprintf(cppFile, "\tc_mifToXbar_req.m_type = 0;\n");
	fprintf(cppFile, "\tc_mifToXbar_req.m_host = 0;\n");
	fprintf(cppFile, "\tc_mifToXbar_req.m_addr = 0;\n");
	fprintf(cppFile, "\tc_mifToXbar_req.m_data = 0;\n");
	fprintf(cppFile, "\tc_mifToXbar_req.m_size = 0;\n");
	fprintf(cppFile, "#ifndef _HTV\n");
	fprintf(cppFile, "\tc_mifToXbar_req.m_line = 0;\n");
	fprintf(cppFile, "\tc_mifToXbar_req.m_file = NULL;\n");
	fprintf(cppFile, "\tc_mifToXbar_req.m_time = 0;\n");
	fprintf(cppFile, "\tc_mifToXbar_req.m_orderChk = false;\n");
	fprintf(cppFile, "#endif\n");
	fprintf(cppFile, "\tht_uint%d c_reqRr = r_reqRr;\n", mifIntfCnt);
	fprintf(cppFile, "\n");

	int i = 0;
	for (size_t instIdx = 0; instIdx < mifInstList.size(); instIdx += 1) {
		CMifModInst & mifInst = mifInstList[instIdx];
		CModInst &modInst = *mifInst.m_pModInst;
		CModMemPort & modMemPort = *mifInst.m_pModMemPort;


		if (!modMemPort.m_bRead && !modMemPort.m_bWrite)
			continue;

		bool bNeed_bRrHold = modMemPort.m_bMultiWr;
		bool bNeed_reqPop = bNeed_bRrHold || modMemPort.m_queueW == 0;

		char reqVarName[128];
		if (modMemPort.m_queueW == 0)
			sprintf(reqVarName, "r_%sP%dToMif_req", modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx);
		else if (modInst.m_pMod->m_clkRate == eClk1x)
			sprintf(reqVarName, "m_%sP%dReqQue.front()", modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx);
		else
			sprintf(reqVarName, "r_%sP%dReqQueFront", modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx);

		if (bNeed_reqPop)
			fprintf(cppFile, "\tbool c_%sP%dReqPop = false;\n", modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx);
		if (bNeed_bRrHold)
			fprintf(cppFile, "\tht_uint3 c_%sP%dRemQwCnt = r_%sP%dRemQwCnt;\n",
				modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx,
				modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx);
		fprintf(cppFile, "\tif (c_%sP%dReqSel) {\n", modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx);
		if (bNeed_reqPop)
			fprintf(cppFile, "\t\tc_%sP%dReqPop = true;\n", modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx);
		fprintf(cppFile, "\t\tc_mifToXbar_reqRdy = true;\n");

		char *pTab = "\t\t";
		if (bMultiQwSupport || bNeed_bRrHold) {
			if (bNeed_bRrHold)
				fprintf(cppFile, "\t\tbool bRrHold = false;\n");

			fprintf(cppFile, "\t\tsc_uint<3> qwCntM1 = (%s.m_tid >> MIF_TID_QWCNT_SHF) & MIF_TID_QWCNT_MSK;\n\n", reqVarName);

			if (bNeed_bRrHold) {
				fprintf(cppFile, "\t\tif (r_%sP%dRemQwCnt > 0)\n", modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx);
				fprintf(cppFile, "\t\t\tc_%sP%dRemQwCnt -= 1;\n", modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx);
				fprintf(cppFile, "\t\telse if (%s.m_type == MEM_REQ_WR)\n", reqVarName);
				fprintf(cppFile, "\t\t\tc_%sP%dRemQwCnt = qwCntM1;\n", modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx);
				fprintf(cppFile, "\t\telse\n");
				fprintf(cppFile, "\t\t\tc_%sP%dRemQwCnt = 0;\n", modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx);
				fprintf(cppFile, "\n");
				fprintf(cppFile, "\t\tif (c_%sP%dRemQwCnt > 0) {\n", modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx);
				fprintf(cppFile, "\t\t\tc_%sP%dReqPop = %s.m_type == MEM_REQ_WR;\n", modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx, reqVarName);
				fprintf(cppFile, "\t\t\tbRrHold = true;\n");
				fprintf(cppFile, "\t\t}\n");
				fprintf(cppFile, "\n");
			}
		}

		if (bMifWrite) {
			if (!modMemPort.m_bWrite)
				fprintf(cppFile, "%sc_mifToXbar_req.m_type = MEM_REQ_RD;\n", pTab);
			else if (!modMemPort.m_bRead)
				fprintf(cppFile, "%sc_mifToXbar_req.m_type = MEM_REQ_WR;\n", pTab);
			else
				fprintf(cppFile, "%sc_mifToXbar_req.m_type = %s.m_type;\n", pTab, reqVarName);
		}

		fprintf(cppFile, "%sc_mifToXbar_req.m_addr = %s.m_addr;\n", pTab, reqVarName);

		fprintf(cppFile, "\t\tc_mifToXbar_req.m_host = %s.m_host;\n", reqVarName);

		if (bMultiQwSupport) {
			fprintf(cppFile, "\t\tc_mifToXbar_req.m_tid = (sc_uint<MIF_TID_W>)(%s.m_tid << %d) | 0x%x | qwCntM1;\n",
				reqVarName, mifTidBits+3, i<<3);
		} else
			fprintf(cppFile, "\t\tc_mifToXbar_req.m_tid = (sc_uint<MIF_TID_W>)(%s.m_tid << %d) | 0x%x;\n",
				reqVarName, mifTidBits+3, i<<3);

		if (modMemPort.m_bWrite)
			fprintf(cppFile, "\t\tc_mifToXbar_req.m_data = %s.m_data;\n", reqVarName);

		fprintf(cppFile, "\t\tc_mifToXbar_req.m_size = %s.m_size;\n", reqVarName);

		fprintf(cppFile, "#ifndef _HTV\n");
		fprintf(cppFile, "\t\tc_mifToXbar_req.m_file = %s.m_file;\n", reqVarName);
		fprintf(cppFile, "\t\tc_mifToXbar_req.m_line = %s.m_line;\n", reqVarName);
		fprintf(cppFile, "\t\tc_mifToXbar_req.m_time = %s.m_time;\n", reqVarName);
		fprintf(cppFile, "\t\tc_mifToXbar_req.m_orderChk = %s.m_orderChk;\n", reqVarName);
		fprintf(cppFile, "#endif\n");

		if (modMemPort.m_queueW != 0 && modInst.m_pMod->m_clkRate == eClk1x) {
			if (modMemPort.m_bRead && modMemPort.m_bMultiRd && bNeed_reqPop)
				fprintf(cppFile, "\t\tif (c_%sP%dReqPop)\n\t", modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx);
			fprintf(cppFile, "\t\tm_%sP%dReqQue.pop();\n", modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx);
		}

		if (bNeed_bRrHold)
			fprintf(cppFile, "\t\tc_reqRr = bRrHold ? 0x%x : 0x%x;\n", 1u << i, (int)i == (mifIntfCnt-1) ? 1u : (1u << (i+1)));
		else
			fprintf(cppFile, "\t\tc_reqRr = 0x%x;\n", (int)i == (mifIntfCnt-1) ? 1 : (1u << (i+1)));
		fprintf(cppFile, "\t}\n");
		fprintf(cppFile, "\n");

		if (modMemPort.m_queueW != 0 && modInst.m_pMod->m_clkRate == eClk2x) {
			string reqSel;
			if (modMemPort.m_bRead && modMemPort.m_bMultiRd && bNeed_reqPop)
				reqSel = VA("c_%sP%dReqPop", modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx);
			else
				reqSel = VA("c_%sP%dReqSel", modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx);

			fprintf(cppFile, "\tbool c_%sP%dReqQueEmpty = (r_%sP%dReqQueEmpty || %s) && m_%sP%dReqQue.empty();\n",
				modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx, modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx, 
				reqSel.c_str(), modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx);
			fprintf(cppFile, "\tCMemRdWrReqIntf c_%sP%dReqQueFront = (r_%sP%dReqQueEmpty || %s) ? m_%sP%dReqQue.front() : r_%sP%dReqQueFront;\n",
				modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx, modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx, reqSel.c_str(),
				modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx, modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx);
			fprintf(cppFile, "\tif ((r_%sP%dReqQueEmpty || %s) && !m_%sP%dReqQue.empty())\n",
				modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx, reqSel.c_str(),
				modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx);
			fprintf(cppFile, "\t\tm_%sP%dReqQue.pop();\n", modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx);
			fprintf(cppFile, "\n");
		}

		i += 1;
	}

	// Forward Read Responses
	fprintf(cppFile, "\t// Forward Read Responses\n");

	i = 0;
	for (size_t instIdx = 0; instIdx < mifInstList.size(); instIdx += 1) {
		CMifModInst & mifInst = mifInstList[instIdx];
		CModInst &modInst = *mifInst.m_pModInst;
		CModMemPort & modMemPort = *mifInst.m_pModMemPort;

		if (!modMemPort.m_bRead && !modMemPort.m_bWrite) continue;

		if (modMemPort.m_bRead) {
			fprintf(cppFile, "\tbool c_mifTo%sP%d_rdRspRdy = r_xbarToMif_rdRspRdy && (r_xbarToMif_rdRsp.m_tid & 0x%lx) == (ht_uint%d) 0x%x;\n",
				modInst.m_instName.Uc().c_str(), modMemPort.m_portIdx, ((1ul << mifTidBits)-1) << 3, mifTidBits == 0 ? 4 : mifTidBits+3, i << 3);
			fprintf(cppFile, "\tCMemRdRspIntf c_mifTo%sP%d_rdRsp;\n", modInst.m_instName.Uc().c_str(), modMemPort.m_portIdx);
			fprintf(cppFile, "\tc_mifTo%sP%d_rdRsp.m_tid = (sc_uint<MIF_TID_W>)(((r_xbarToMif_rdRsp.m_tid & MIF_TID_QWCNT_MSK) << MIF_TID_QWCNT_SHF) | (r_xbarToMif_rdRsp.m_tid >> %d));\n",
				modInst.m_instName.Uc().c_str(), modMemPort.m_portIdx, mifTidBits+3);
			fprintf(cppFile, "\tc_mifTo%sP%d_rdRsp.m_data = r_xbarToMif_rdRsp.m_data;\n",
				modInst.m_instName.Uc().c_str(), modMemPort.m_portIdx);
			fprintf(cppFile, "\n");
		}

		if (modMemPort.m_bWrite) {
			fprintf(cppFile, "\tbool c_mifTo%sP%d_wrRspRdy = r_xbarToMif_wrRspRdy && (r_xbarToMif_wrRspTid & 0x%lx) == (ht_uint%d) 0x%x;\n",
				modInst.m_instName.Uc().c_str(), modMemPort.m_portIdx, ((1ul << mifTidBits)-1) << 3, mifTidBits == 0 ? 4 : mifTidBits+3, i << 3);
			fprintf(cppFile, "\tsc_uint<MIF_TID_W> c_mifTo%sP%d_wrRspTid = (sc_uint<MIF_TID_W>)(r_xbarToMif_wrRspTid >> %d);\n",
				modInst.m_instName.Uc().c_str(), modMemPort.m_portIdx, mifTidBits+3);
			fprintf(cppFile, "\n");
		}

		i += 1;
	}

	fprintf(cppFile, "\t// Handle 1x clock input requests\n");

	for (size_t instIdx = 0; instIdx < mifInstList.size(); instIdx += 1) {
		CMifModInst & mifInst = mifInstList[instIdx];
		CModInst &modInst = *mifInst.m_pModInst;
		CModMemPort & modMemPort = *mifInst.m_pModMemPort;

		CModule * pMod = modInst.m_pMod;

		if (!modMemPort.m_bRead && !modMemPort.m_bWrite)
			continue;

		bool bNeed_bRrHold = modMemPort.m_bMultiWr;
		bool bNeed_reqPop = bNeed_bRrHold || modMemPort.m_queueW == 0;

		if (modMemPort.m_queueW == 0) {
			char reqPop[128];
			if (bNeed_reqPop)
				sprintf(reqPop, " && !c_%sP%dReqPop", modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx);
			else
				reqPop[0] = '\0';

			if (pMod->m_clkRate == eClk1x)
				fprintf(cppFile, "\tbool c_%sP%dToMif_reqRdy = i_%sP%dToMif%d_reqRdy.read() || r_%sP%dToMif_reqRdy%s;\n",
					modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx, modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx,
					mifId, modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx, reqPop);
			else
				fprintf(cppFile, "\tbool c_%sP%dToMif_reqRdy = r_%sP%dToMif_reqRdy_2x.read() || r_%sP%dToMif_reqRdy%s;\n",
					modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx, modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx, 
					modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx, reqPop);

			fprintf(cppFile, "\tCMemRdWrReqIntf c_%sP%dToMif_req = r_%sP%dToMif_req;\n",
				modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx,
				modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx);
		}
		if (pMod->m_clkRate == eClk1x)
			fprintf(cppFile, "\tif (i_%sP%dToMif%d_reqRdy.read())\n", modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx, mifId);
		else
			if (modMemPort.m_queueW == 0)
				fprintf(cppFile, "\tif (r_%sP%dToMif_reqRdy_2x.read())\n", modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx);

		if (modMemPort.m_queueW == 0) {
			if (pMod->m_clkRate == eClk1x)
				fprintf(cppFile, "\t\tc_%sP%dToMif_req = i_%sP%dToMif%d_req.read();\n",
					modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx,
					modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx, mifId);
			else
				fprintf(cppFile, "\t\tc_%sP%dToMif_req = r_%sP%dToMif_req_2x.read();\n",
					modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx,
					modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx);
		} else
			if (pMod->m_clkRate == eClk1x)
				fprintf(cppFile, "\t\tm_%sP%dReqQue.push(i_%sP%dToMif%d_req.read());\n", 
				modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx,
				modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx, mifId);
		fprintf(cppFile, "\n");
	}

	fprintf(cppFile, "\t/////////////////////////\n");
	fprintf(cppFile, "\t// Registers\n");
	fprintf(cppFile, "\n");

	// Clock queues
	for (size_t instIdx = 0; instIdx < mifInstList.size(); instIdx += 1) {
		CMifModInst & mifInst = mifInstList[instIdx];
		CModInst &modInst = *mifInst.m_pModInst;
		CModMemPort & modMemPort = *mifInst.m_pModMemPort;

		CModule * pMod = modInst.m_pMod;

		if (!modMemPort.m_bRead && !modMemPort.m_bWrite || modMemPort.m_queueW == 0)
			continue;

		if (pMod->m_clkRate == eClk1x)
			fprintf(cppFile, "\tm_%sP%dReqQue.clock(r_reset1x);\n", modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx);
		else
			fprintf(cppFile, "\tm_%sP%dReqQue.pop_clock(r_reset1x);\n", modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx);
	}
	fprintf(cppFile, "\n");

	for (size_t instIdx = 0; instIdx < mifInstList.size(); instIdx += 1) {
		CMifModInst & mifInst = mifInstList[instIdx];
		CModInst &modInst = *mifInst.m_pModInst;
		CModMemPort & modMemPort = *mifInst.m_pModMemPort;

		CModule * pMod = modInst.m_pMod;

		if (!modMemPort.m_bRead && !modMemPort.m_bWrite || modMemPort.m_queueW != 0 || pMod->m_clkRate != eClk2x)
			continue;

		fprintf(cppFile, "\tr_%sP%dToMif_reqRdy = !r_reset1x && c_%sP%dToMif_reqRdy;\n",
			modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx,
			modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx);
		fprintf(cppFile, "\tr_%sP%dToMif_req = c_%sP%dToMif_req;\n",
			modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx,
			modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx);
		fprintf(cppFile, "\n");
	}

	// register output flow control signals
	bNewLine = false;
	for (size_t instIdx = 0; instIdx < mifInstList.size(); instIdx += 1) {
		CMifModInst & mifInst = mifInstList[instIdx];
		CModInst &modInst = *mifInst.m_pModInst;
		CModMemPort & modMemPort = *mifInst.m_pModMemPort;

		CModule * pMod = modInst.m_pMod;

		if (!modMemPort.m_bRead && !modMemPort.m_bWrite) continue;

		bool bNeed_bRrHold = modMemPort.m_bMultiWr;
		bool bNeed_reqPop = bNeed_bRrHold || modMemPort.m_queueW == 0;

		char reqSel[128];
		if (bNeed_reqPop)
			sprintf(reqSel, "c_%sP%dReqPop", modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx);
		else
			sprintf(reqSel, "c_%sP%dReqSel", modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx);

		if (pMod->m_clkRate == eClk1x)
			fprintf(cppFile, "\tr_mifTo%sP%d_reqAvl = %s;\n", modInst.m_instName.Uc().c_str(), modMemPort.m_portIdx, reqSel);
		else {
			fprintf(cppFile, "\tr_%sP%dReqSel = %s;\n", modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx, reqSel);
			fprintf(cppFile, "\tr_%sP%dReqQueEmpty = r_reset1x || c_%sP%dReqQueEmpty;\n", 
				modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx,
				modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx);
			fprintf(cppFile, "\tr_%sP%dReqQueFront = c_%sP%dReqQueFront;\n", 
				modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx,
				modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx);
		}
		bNewLine = true;
	}
	if (bNewLine)
		fprintf(cppFile, "\n");

	// register interface inputs without queuing
	bNewLine = false;
	for (size_t instIdx = 0; instIdx < mifInstList.size(); instIdx += 1) {
		CMifModInst & mifInst = mifInstList[instIdx];
		CModInst &modInst = *mifInst.m_pModInst;
		CModMemPort & modMemPort = *mifInst.m_pModMemPort;

		CModule * pMod = modInst.m_pMod;

		if (!modMemPort.m_bRead && !modMemPort.m_bWrite || pMod->m_clkRate != eClk1x || modMemPort.m_queueW != 0)	continue;

		fprintf(cppFile, "\tr_%sP%dToMif_reqRdy = !r_reset1x && c_%sP%dToMif_reqRdy;\n", 
			modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx,
			modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx);
		fprintf(cppFile, "\tr_%sP%dToMif_req = c_%sP%dToMif_req;\n",
			modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx,
			modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx);
		bNewLine = true;
	}
	if (bNewLine)
		fprintf(cppFile, "\n");

	// register input xbar signals
	fprintf(cppFile, "\tr_xbarToMif_reqFull = i_xbar%sToMif%d_reqFull.read();\n", mifName, mifId);
	if (bMifWrite) {
		fprintf(cppFile, "\tr_xbarToMif_wrRspRdy = r_reset1x ? (bool)0 : i_xbar%sToMif%d_wrRspRdy.read();\n", mifName, mifId);
		fprintf(cppFile, "\tr_xbarToMif_wrRspTid = i_xbar%sToMif%d_wrRspTid.read();\n", mifName, mifId);
	}

	if (bMifRead) {
		fprintf(cppFile, "\tr_xbarToMif_rdRspRdy = r_reset1x ? (bool)0 : i_xbar%sToMif%d_rdRspRdy.read();\n", mifName, mifId);
		fprintf(cppFile, "\tr_xbarToMif_rdRsp = i_xbar%sToMif%d_rdRsp.read();\n", mifName, mifId);
		fprintf(cppFile, "\n");
	}

	// register output xbar signals
	fprintf(cppFile, "\tr_reqRr = r_reset1x ? (ht_uint%d)0x01 : c_reqRr;\n", mifIntfCnt);

	for (size_t instIdx = 0; instIdx < mifInstList.size(); instIdx += 1) {
		CMifModInst & mifInst = mifInstList[instIdx];
		CModInst &modInst = *mifInst.m_pModInst;
		CModMemPort & modMemPort = *mifInst.m_pModMemPort;


		if (!modMemPort.m_bRead && !modMemPort.m_bWrite)
			continue;

		if (modMemPort.m_bMultiWr)
			fprintf(cppFile, "\tr_%sP%dRemQwCnt = r_reset1x ? (ht_uint3)0 : c_%sP%dRemQwCnt;\n", 
				modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx,
				modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx);
	}

	fprintf(cppFile, "\tr_mifToXbar_reqRdy = r_reset1x ? (bool)0 : c_mifToXbar_reqRdy;\n");
	fprintf(cppFile, "\tht_attrib(equivalent_register_removal, r_mifToXbar_req, \"no\");\n");
	fprintf(cppFile, "\tr_mifToXbar_req = c_mifToXbar_req;\n");
	fprintf(cppFile, "\n");

	// register output signals to unit modules
	for (size_t instIdx = 0; instIdx < mifInstList.size(); instIdx += 1) {
		CMifModInst & mifInst = mifInstList[instIdx];
		CModInst &modInst = *mifInst.m_pModInst;
		CModMemPort & modMemPort = *mifInst.m_pModMemPort;


		if (!modMemPort.m_bRead && !modMemPort.m_bWrite) continue;

		if (modMemPort.m_bRead) {
			fprintf(cppFile, "\tr_mifTo%sP%d_rdRspRdy = r_reset1x ? (bool)0 : c_mifTo%sP%d_rdRspRdy;\n",
				modInst.m_instName.Uc().c_str(), modMemPort.m_portIdx,
				modInst.m_instName.Uc().c_str(), modMemPort.m_portIdx);
			fprintf(cppFile, "\tr_mifTo%sP%d_rdRsp = c_mifTo%sP%d_rdRsp;\n",
				modInst.m_instName.Uc().c_str(), modMemPort.m_portIdx,
				modInst.m_instName.Uc().c_str(), modMemPort.m_portIdx);
			fprintf(cppFile, "\n");
		}

		if (modMemPort.m_bWrite) {
			fprintf(cppFile, "\tr_mifTo%sP%d_wrRspRdy = r_reset1x ? (bool)0 : c_mifTo%sP%d_wrRspRdy;\n",
				modInst.m_instName.Uc().c_str(), modMemPort.m_portIdx, 
				modInst.m_instName.Uc().c_str(), modMemPort.m_portIdx);
			fprintf(cppFile, "\tr_mifTo%sP%d_wrRspTid = c_mifTo%sP%d_wrRspTid;\n",
				modInst.m_instName.Uc().c_str(), modMemPort.m_portIdx, 
				modInst.m_instName.Uc().c_str(), modMemPort.m_portIdx);
			fprintf(cppFile, "\n");
		}
	}

	fprintf(cppFile, "\tHtResetFlop(r_reset1x, i_reset.read());\n");
	if (bClk2x)
		fprintf(cppFile, "\tc_reset1x = r_reset1x;\n");

	fprintf(cppFile, "\n");

	fprintf(cppFile, "\t/////////////////////////\n");
	fprintf(cppFile, "\t// Update Outputs\n");
	fprintf(cppFile, "\n");

	for (size_t instIdx = 0; instIdx < mifInstList.size(); instIdx += 1) {
		CMifModInst & mifInst = mifInstList[instIdx];
		CModInst &modInst = *mifInst.m_pModInst;
		CModMemPort & modMemPort = *mifInst.m_pModMemPort;

		CModule * pMod = modInst.m_pMod;

		if (!modMemPort.m_bRead && !modMemPort.m_bWrite || pMod->m_clkRate == eClk2x)
			continue;

		fprintf(cppFile, "\to_mif%dTo%sP%d_reqAvl = r_mifTo%sP%d_reqAvl;\n", 
			mifId, modInst.m_instName.Uc().c_str(), modMemPort.m_portIdx,
			modInst.m_instName.Uc().c_str(), modMemPort.m_portIdx); 
	}
	fprintf(cppFile, "\n");

	fprintf(cppFile, "\to_mif%dToXbar%s_reqRdy = r_mifToXbar_reqRdy;\n", mifId, mifName);
	fprintf(cppFile, "\to_mif%dToXbar%s_req = r_mifToXbar_req;\n", mifId, mifName);
	fprintf(cppFile, "\n");

	fprintf(cppFile, "\to_mif%dToXbar%s_rdRspFull = false;\n", mifId, mifName);
	fprintf(cppFile, "\to_mif%dToXbar%s_wrRspFull = false;\n", mifId, mifName);
	fprintf(cppFile, "\n");

	for (size_t instIdx = 0; instIdx < mifInstList.size(); instIdx += 1) {
		CMifModInst & mifInst = mifInstList[instIdx];
		CModInst &modInst = *mifInst.m_pModInst;
		CModMemPort & modMemPort = *mifInst.m_pModMemPort;


		if (!modMemPort.m_bRead && !modMemPort.m_bWrite) continue;

		if (modMemPort.m_bRead) {
			if (modInst.m_pMod->m_clkRate == eClk1x)
				fprintf(cppFile, "\to_mif%dTo%sP%d_rdRspRdy = r_mifTo%sP%d_rdRspRdy;\n",
					mifId, modInst.m_instName.Uc().c_str(), modMemPort.m_portIdx,
					modInst.m_instName.Uc().c_str(), modMemPort.m_portIdx);
			fprintf(cppFile, "\to_mif%dTo%sP%d_rdRsp = r_mifTo%sP%d_rdRsp;\n",
				mifId, modInst.m_instName.Uc().c_str(), modMemPort.m_portIdx,
				modInst.m_instName.Uc().c_str(), modMemPort.m_portIdx);
			fprintf(cppFile, "\n");
		}

		if (modMemPort.m_bWrite) {
			if (modInst.m_pMod->m_clkRate == eClk1x)
				fprintf(cppFile, "\to_mif%dTo%sP%d_wrRspRdy = r_mifTo%sP%d_wrRspRdy;\n",
					mifId, modInst.m_instName.Uc().c_str(), modMemPort.m_portIdx,
					modInst.m_instName.Uc().c_str(), modMemPort.m_portIdx);
			fprintf(cppFile, "\to_mif%dTo%sP%d_wrRspTid = r_mifTo%sP%d_wrRspTid;\n",
				mifId, modInst.m_instName.Uc().c_str(), modMemPort.m_portIdx,
				modInst.m_instName.Uc().c_str(), modMemPort.m_portIdx);
			fprintf(cppFile, "\n");
		}
	}

	fprintf(cppFile, "}\n");

	if (bClk2x) {
		fprintf(cppFile, "\n");
		fprintf(cppFile, "void\n");
		fprintf(cppFile, "CPers%sMif%s::Pers%sMif2x()\n", m_unitName.Uc().c_str(), mifName, m_unitName.Uc().c_str());
		fprintf(cppFile, "{\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t/////////////////////////\n");
		fprintf(cppFile, "\t// Combinatorial Logic\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t// Flow control signals\n");

		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t// Handle 2x clock input requests\n");

		for (size_t instIdx = 0; instIdx < mifInstList.size(); instIdx += 1) {
			CMifModInst & mifInst = mifInstList[instIdx];
			CModInst &modInst = *mifInst.m_pModInst;
			CModMemPort & modMemPort = *mifInst.m_pModMemPort;

			CModule * pMod = modInst.m_pMod;

			if (!modMemPort.m_bRead && !modMemPort.m_bWrite || pMod->m_clkRate != eClk2x) continue;

			if (modMemPort.m_queueW == 0)
				fprintf(cppFile, "\tCMemRdWrReqIntf c_%sP%dToMif_req_2x = r_%sP%dToMif_req_2x.read();\n",
					modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx,
					modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx);
			fprintf(cppFile, "\tif (i_%sP%dToMif%d_reqRdy.read())\n", modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx, mifId);
			if (modMemPort.m_queueW == 0) {
				fprintf(cppFile, "\t\tc_%sP%dToMif_req_2x = i_%sP%dToMif%d_req.read();\n", 
					modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx, 
					modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx, mifId);
				fprintf(cppFile, "\n");
				fprintf(cppFile, "\tbool c_%sP%dToMif_reqRdy_2x = r_%sP%dToMif_reqRdy_2x && (r_phase\n",
					modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx,
					modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx);
				fprintf(cppFile, "\t\t|| r_%sP%dToMif_reqRdy.read() && !r_%sP%dReqSel.read()) || i_%sP%dToMif%d_reqRdy.read();\n",
					modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx,
					modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx,
					modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx, mifId);
			} else
				fprintf(cppFile, "\t\tm_%sP%dReqQue.push(i_%sP%dToMif%d_req.read());\n", 
					modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx,
					modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx, mifId);
			fprintf(cppFile, "\n");
		}

		// register output flow control signals
		for (size_t instIdx = 0; instIdx < mifInstList.size(); instIdx += 1) {
			CMifModInst & mifInst = mifInstList[instIdx];
			CModInst &modInst = *mifInst.m_pModInst;
			CModMemPort & modMemPort = *mifInst.m_pModMemPort;

			CModule * pMod = modInst.m_pMod;

			if (!modMemPort.m_bRead && !modMemPort.m_bWrite) continue;

			if (pMod->m_clkRate == eClk2x)
				fprintf(cppFile, "\tbool c_%sP%dReqAvl = r_phase & r_%sP%dReqSel;\n", 
					modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx, 
					modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx);
		}
		fprintf(cppFile, "\n");

		fprintf(cppFile, "\t/////////////////////////\n");
		fprintf(cppFile, "\t// Registers\n");
		fprintf(cppFile, "\n");

		// Clock queues
		for (size_t instIdx = 0; instIdx < mifInstList.size(); instIdx += 1) {
			CMifModInst & mifInst = mifInstList[instIdx];
			CModInst &modInst = *mifInst.m_pModInst;
			CModMemPort & modMemPort = *mifInst.m_pModMemPort;

			CModule * pMod = modInst.m_pMod;

			if (!modMemPort.m_bRead && !modMemPort.m_bWrite || modMemPort.m_queueW == 0) continue;

			if (pMod->m_clkRate == eClk2x)
				fprintf(cppFile, "\tm_%sP%dReqQue.push_clock(c_reset1x);\n", modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx);
		}
		fprintf(cppFile, "\n");

		// register output flow control signals
		for (size_t instIdx = 0; instIdx < mifInstList.size(); instIdx += 1) {
			CMifModInst & mifInst = mifInstList[instIdx];
			CModInst &modInst = *mifInst.m_pModInst;
			CModMemPort & modMemPort = *mifInst.m_pModMemPort;

			CModule * pMod = modInst.m_pMod;

			if (!modMemPort.m_bRead && !modMemPort.m_bWrite || pMod->m_clkRate == eClk1x) continue;

			fprintf(cppFile, "\tr_mifTo%sP%d_reqAvl = c_%sP%dReqAvl;\n", 
				modInst.m_instName.Uc().c_str(), modMemPort.m_portIdx,
				modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx);

			if (modMemPort.m_bRead)
				fprintf(cppFile, "\tr_mifTo%sP%d_rdRspRdy_2x = r_mifTo%sP%d_rdRspRdy & r_phase;\n",
					modInst.m_instName.Uc().c_str(), modMemPort.m_portIdx, 
					modInst.m_instName.Uc().c_str(), modMemPort.m_portIdx);

			if (modMemPort.m_bWrite)
				fprintf(cppFile, "\tr_mifTo%sP%d_wrRspRdy_2x = r_mifTo%sP%d_wrRspRdy & r_phase;\n",
					modInst.m_instName.Uc().c_str(), modMemPort.m_portIdx, 
					modInst.m_instName.Uc().c_str(), modMemPort.m_portIdx);

			if (modMemPort.m_queueW == 0) {
				fprintf(cppFile, "\n");
				fprintf(cppFile, "\tr_%sP%dToMif_reqRdy_2x = !c_reset1x.read() && c_%sP%dToMif_reqRdy_2x;\n",
					modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx,
					modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx);
				fprintf(cppFile, "\tr_%sP%dToMif_req_2x = c_%sP%dToMif_req_2x;\n",
					modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx, 
					modInst.m_instName.Lc().c_str(), modMemPort.m_portIdx);
			}
			fprintf(cppFile, "\n");
		}

		fprintf(cppFile, "\tr_phase = c_reset1x.read() || !r_phase;\n");
		fprintf(cppFile, "\n");

		fprintf(cppFile, "\t/////////////////////////\n");
		fprintf(cppFile, "\t// Update Outputs\n");
		fprintf(cppFile, "\n");

		for (size_t instIdx = 0; instIdx < mifInstList.size(); instIdx += 1) {
			CMifModInst & mifInst = mifInstList[instIdx];
			CModInst &modInst = *mifInst.m_pModInst;
			CModMemPort & modMemPort = *mifInst.m_pModMemPort;

			CModule * pMod = modInst.m_pMod;

			if (!modMemPort.m_bRead && !modMemPort.m_bWrite || pMod->m_clkRate == eClk1x)
				continue;

			fprintf(cppFile, "\to_mif%dTo%sP%d_reqAvl = r_mifTo%sP%d_reqAvl;\n", 
				mifId, modInst.m_instName.Uc().c_str(), modMemPort.m_portIdx,
				modInst.m_instName.Uc().c_str(), modMemPort.m_portIdx); 

			if (modMemPort.m_bRead)
				fprintf(cppFile, "\to_mif%dTo%sP%d_rdRspRdy = r_mifTo%sP%d_rdRspRdy_2x;\n",
					mifId, modInst.m_instName.Uc().c_str(), modMemPort.m_portIdx, 
					modInst.m_instName.Uc().c_str(), modMemPort.m_portIdx);

			if (modMemPort.m_bWrite)
				fprintf(cppFile, "\to_mif%dTo%sP%d_wrRspRdy = r_mifTo%sP%d_wrRspRdy_2x;\n",
					mifId, modInst.m_instName.Uc().c_str(), modMemPort.m_portIdx,
					modInst.m_instName.Uc().c_str(), modMemPort.m_portIdx);
		}

		fprintf(cppFile, "}\n");
	}

	cppFile.FileClose();
}
