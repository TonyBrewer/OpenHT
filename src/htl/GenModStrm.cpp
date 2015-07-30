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

// Generate a module's stream code

void CDsnInfo::InitAndValidateModStrm()
{
	// first initialize HtStrings
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];

		if (!mod.m_bIsUsed) continue;

		for (size_t streamIdx = 0; streamIdx < mod.m_streamList.size(); streamIdx += 1) {
			CStream * pStrm = mod.m_streamList[streamIdx];

			pStrm->m_strmBw.InitValue(pStrm->m_lineInfo, false, 10);
			pStrm->m_elemCntW.InitValue(pStrm->m_lineInfo, false, 12);
			pStrm->m_strmCnt.InitValue(pStrm->m_lineInfo, false, 1);
			pStrm->m_reserve.InitValue(pStrm->m_lineInfo, false, 0);
			pStrm->m_rspGrpW.InitValue(pStrm->m_lineInfo, false, mod.m_threads.m_htIdW.AsInt());

			if (pStrm->m_strmCnt.AsInt() < 1 || pStrm->m_strmCnt.AsInt() > 64) {
				ParseMsg(Error, pStrm->m_lineInfo, "unsupported value for strmCnt (%d) must be 1-64");
				continue;
			}

			pStrm->m_strmCntW = FindLg2(pStrm->m_strmCnt.AsInt() - 1);

			if (pStrm->m_strmBw.AsInt() == 0 || pStrm->m_strmBw.AsInt() > 10) {
				ParseMsg(Error, pStrm->m_lineInfo, "unsupported value for strmBw (%d) must be 1-10", pStrm->m_strmBw.AsInt());
				continue;
			}

			string varName;
			CHtString bitWidth;
			pStrm->m_elemBitW = FindTypeWidth(varName, pStrm->m_pType->m_typeName, bitWidth, pStrm->m_lineInfo);
			if (pStrm->m_elemBitW != 8 && pStrm->m_elemBitW != 16 && pStrm->m_elemBitW != 32 && pStrm->m_elemBitW != 64)
				ParseMsg(Error, pStrm->m_lineInfo, "unsupported type width for stream (%d) must be 8, 16, 32 or 64", pStrm->m_elemBitW);

			if (pStrm->m_memPort.size() > 1 && (int)pStrm->m_memPort.size() != pStrm->m_strmCnt.AsInt())
				ParseMsg(Error, pStrm->m_lineInfo, "number of specifed memory ports (%d) does not match strmCnt (%d)",
				(int)pStrm->m_memPort.size(), pStrm->m_strmCnt.AsInt());

			if ((int)pStrm->m_memPort.size() != pStrm->m_strmCnt.AsInt())
				pStrm->m_memPort.resize(pStrm->m_strmCnt.AsInt(), pStrm->m_memPort[0]);

			pStrm->m_arbRr.resize(pStrm->m_memPort.size(), 0);

			for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
				int memPortIdx = pStrm->m_memPort[i];
				CModMemPort * pModMemPort = mod.m_memPortList[memPortIdx];

				if (pStrm->m_bRead)
					pModMemPort->m_rdStrmCnt += 1;
				else
					pModMemPort->m_wrStrmCnt += 1;

				pStrm->m_arbRr[i] = pModMemPort->m_strmList.size();
				pModMemPort->m_strmList.push_back(CMemPortStrm(pStrm, i, pStrm->m_bRead));

				static bool bError = false;
				if (!bError && pModMemPort->m_strmList.size() > 16) {
					bError = true;
					ParseMsg(Error, pStrm->m_lineInfo, "too many streams mapped to memory port %d, limit of 16", memPortIdx);
				}

				if (pModMemPort->m_bIsMem && pModMemPort->m_bIsStrm) {
					ParseMsg(Error, pStrm->m_lineInfo, "stream and memory can not use the same module memory port (port %d).", memPortIdx);
					ParseMsg(Info, pStrm->m_lineInfo, "AddReadMem and AddWriteMem HTD commands use a module's memory port 0.");
					ParseMsg(Info, pStrm->m_lineInfo, "AddReadStream and AddWriteStream HTD commands must be assigned memory ports starting");
					ParseMsg(Info, pStrm->m_lineInfo, "with port 1 if AddReadMem or AddWriteMem is specified for the module or port 1 if");
					ParseMsg(Info, pStrm->m_lineInfo, "AddReadMem and AddWriteMem are not specified for the module.");
				}
			}

			// calculate buffer pointer width and full threashold for read stream
			switch (pStrm->m_elemBitW) {
			case 8: pStrm->m_elemByteW = 0; pStrm->m_qwElemCnt = 8; break;
			case 16: pStrm->m_elemByteW = 1; pStrm->m_qwElemCnt = 4; break;
			case 32: pStrm->m_elemByteW = 2; pStrm->m_qwElemCnt = 2; break;
			case 64: pStrm->m_elemByteW = 3; pStrm->m_qwElemCnt = 1; break;
			}

			if (pStrm->m_bRead) {
				switch (pStrm->m_elemBitW) {
				case 8:
					pStrm->m_bufFull = pStrm->m_strmBw.AsInt() == 10 ? (1 << 9) - 79 : ((1 << 8) - 79 - (9 - pStrm->m_strmBw.AsInt()) * 19);
					pStrm->m_bufPtrW = FindLg2(pStrm->m_bufFull + 64 + 7) + 1;
					break;
				case 16:
					pStrm->m_bufFull = pStrm->m_strmBw.AsInt() == 10 ? (1 << 9) - 39 : ((1 << 8) - 39 - (9 - pStrm->m_strmBw.AsInt()) * 24);
					pStrm->m_bufPtrW = FindLg2(pStrm->m_bufFull + 32 + 3) + 1;
					break;
				case 32:
					pStrm->m_bufFull = pStrm->m_strmBw.AsInt() == 10 ? (1 << 9) - 19 : ((1 << 8) - 19 - (9 - pStrm->m_strmBw.AsInt()) * 26);
					pStrm->m_bufPtrW = FindLg2(pStrm->m_bufFull + 16 + 1) + 1;
					break;
				case 64:
					pStrm->m_bufFull = pStrm->m_strmBw.AsInt() == 10 ? (1 << 9) - 10 : ((1 << 8) - 10 - (9 - pStrm->m_strmBw.AsInt()) * 27);
					pStrm->m_bufPtrW = FindLg2(pStrm->m_bufFull + 8) + 1;
					break;
				default:
					assert(0);
				}
			} else
				pStrm->m_bufPtrW = FindLg2((1 << (4 + 3 - pStrm->m_elemByteW)) - 1) + 1;

			if (!pStrm->m_bRead)
				mod.m_rsmSrcCnt += pStrm->m_strmCnt.AsInt();
		}

		// now calculate width of read/write stream counts
		for (size_t memPortIdx = 0; memPortIdx < mod.m_memPortList.size(); memPortIdx += 1) {
			CModMemPort * pModMemPort = mod.m_memPortList[memPortIdx];

			if (!pModMemPort->m_bIsMem && pModMemPort->m_rdStrmCnt == 0 && pModMemPort->m_wrStrmCnt == 0) {
				string msg = "specified memory ports are non-contiguous, ";

				for (size_t port = 0; port < mod.m_memPortList.size(); port += 1) {
					CModMemPort * pPort = mod.m_memPortList[port];
					if (pPort->m_bIsMem || pPort->m_rdStrmCnt > 0 || pPort->m_wrStrmCnt > 0)
						msg += VA("%d ", port);
				}

				ParseMsg(Error, mod.m_threads.m_lineInfo, msg.c_str());
				ParseMsg(Info, mod.m_threads.m_lineInfo, "AddReadMem and AddWriteMem HTD commands use a module's memory port 0.");
				ParseMsg(Info, mod.m_threads.m_lineInfo, "AddReadStream and AddWriteStream HTD commands must be assigned memory ports starting");
				ParseMsg(Info, mod.m_threads.m_lineInfo, "with port 1 if AddReadMem or AddWriteMem is specified for the module or port 1 if");
				ParseMsg(Info, mod.m_threads.m_lineInfo, "AddReadMem and AddWriteMem are not specified for the module.");
				break;
			}

			pModMemPort->m_rdStrmCntW = FindLg2(pModMemPort->m_rdStrmCnt);
			pModMemPort->m_wrStrmCntW = FindLg2(pModMemPort->m_wrStrmCnt);

		}
	}
}

void CDsnInfo::GenModStrmStatements(CModule &mod)
{
	if (mod.m_streamList.size() == 0)
		return;

	bool bIsHc2 = g_appArgs.GetCoprocInfo().GetCoproc() == hc2 || g_appArgs.GetCoprocInfo().GetCoproc() == hc2ex;
	bool bIsWx = g_appArgs.GetCoprocInfo().GetCoproc() == wx690 || g_appArgs.GetCoprocInfo().GetCoproc() == wx2k;

	CHtCode & strmPreInstr = mod.m_clkRate == eClk2x ? m_strmPreInstr2x : m_strmPreInstr1x;
	CHtCode & strmPostInstr = mod.m_clkRate == eClk2x ? m_strmPostInstr2x : m_strmPostInstr1x;
	CHtCode & strmReg = mod.m_clkRate == eClk2x ? m_strmReg2x : m_strmReg1x;
	CHtCode & strmOut = mod.m_clkRate == eClk2x ? m_strmOut2x : m_strmOut1x;

	string reset = mod.m_clkRate == eClk1x ? "r_reset1x" : "c_reset2x";

	string vcdModName = VA("Pers%s", mod.m_modName.Uc().c_str());

	g_appArgs.GetDsnRpt().AddLevel("Stream\n");

	for (size_t memPortIdx = 0; memPortIdx < mod.m_memPortList.size(); memPortIdx += 1) {
		CModMemPort & modMemPort = *mod.m_memPortList[memPortIdx];

		if (!modMemPort.m_bIsStrm) continue;

		m_strmIoDecl.Append("\t// Stream Interface\n");
		GenModDecl(eVcdUser, m_strmIoDecl, vcdModName, "sc_out<bool>", VA("o_%sP%dToMif_reqRdy", mod.m_modName.Lc().c_str(), memPortIdx));
		m_strmIoDecl.Append("\tsc_out<CMemRdWrReqIntf> o_%sP%dToMif_req;\n",
			mod.m_modName.Lc().c_str(), memPortIdx);

		m_strmIoDecl.Append("\tsc_in<bool> i_mifTo%sP%d_reqAvl;\n", mod.m_modName.Uc().c_str(), memPortIdx);
		m_strmIoDecl.Append("\n");

		if (modMemPort.m_bRead) {
			GenModDecl(eVcdUser, m_strmIoDecl, vcdModName, "sc_in<bool>", VA("i_mifTo%sP%d_rdRspRdy", mod.m_modName.Uc().c_str(), memPortIdx));
			GenModDecl(eVcdUser, m_strmIoDecl, vcdModName, "sc_in<CMemRdRspIntf>", VA("i_mifTo%sP%d_rdRsp", mod.m_modName.Uc().c_str(), memPortIdx));
			GenModDecl(eVcdUser, m_strmIoDecl, vcdModName, "sc_out<bool>", VA("o_%sP%dToMif_rdRspFull", mod.m_modName.Lc().c_str(), memPortIdx));
			m_strmIoDecl.Append("\n");

			strmOut.Append("\to_%sP%dToMif_rdRspFull = false;\n",
				mod.m_modName.Lc().c_str(), memPortIdx);
		}

		if (modMemPort.m_bWrite) {
			m_strmIoDecl.Append("\tsc_in<bool> i_mifTo%sP%d_wrRspRdy;\n", mod.m_modName.Uc().c_str(), memPortIdx);
			if (mod.m_threads.m_htIdW.AsInt() == 0)
				m_strmIoDecl.Append("\tsc_in<sc_uint<MIF_TID_W> > i_mifTo%sP%d_wrRspTid;\n", mod.m_modName.Uc().c_str(), memPortIdx);
			else
				m_strmIoDecl.Append("\tsc_in<sc_uint<MIF_TID_W> > i_mifTo%sP%d_wrRspTid;\n", mod.m_modName.Uc().c_str(), memPortIdx);
			m_strmIoDecl.Append("\n");
		}

		if (modMemPort.m_strmList.size() > 1) {
			GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, VA("ht_uint%d", (int)modMemPort.m_strmList.size()), VA("r_%sP%dToMif_reqRr", mod.m_modName.Lc().c_str(), memPortIdx));
			m_strmRegDecl.Append("\tht_uint%d c_%sP%dToMif_reqRr;\n", (int)modMemPort.m_strmList.size(), mod.m_modName.Lc().c_str(), memPortIdx);
			strmPreInstr.Append("\tc_%sP%dToMif_reqRr = r_%sP%dToMif_reqRr;\n",
				mod.m_modName.Lc().c_str(), memPortIdx, mod.m_modName.Lc().c_str(), memPortIdx);
			strmReg.Append("\tr_%sP%dToMif_reqRr = r_reset1x ? (ht_uint%d)0x1 : c_%sP%dToMif_reqRr;\n",
				mod.m_modName.Lc().c_str(), memPortIdx, (int)modMemPort.m_strmList.size(), mod.m_modName.Lc().c_str(), memPortIdx);
			strmReg.Append("\n");
		}

		if (mod.m_clkRate == eClk2x) {
			GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, "bool", VA("r_%sP%dToMif_bReqSel", mod.m_modName.Lc().c_str(), memPortIdx));
			m_strmRegDecl.Append("\tbool c_%sP%dToMif_bReqSel;\n", mod.m_modName.Lc().c_str(), memPortIdx);
			strmReg.Append("\tr_%sP%dToMif_bReqSel = c_%sP%dToMif_bReqSel;\n",
				mod.m_modName.Lc().c_str(), memPortIdx, mod.m_modName.Lc().c_str(), memPortIdx);
		}

		GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, "CMemRdWrReqIntf", VA("r_t3_%sP%dToMif_req", mod.m_modName.Lc().c_str(), memPortIdx));
		m_strmRegDecl.Append("\tCMemRdWrReqIntf c_t2_%sP%dToMif_req;\n", mod.m_modName.Lc().c_str(), memPortIdx);
		strmPreInstr.Append("\tc_t2_%sP%dToMif_req = r_t3_%sP%dToMif_req;\n",
			mod.m_modName.Lc().c_str(), memPortIdx, mod.m_modName.Lc().c_str(), memPortIdx);
		strmReg.Append("\tr_t3_%sP%dToMif_req = c_t2_%sP%dToMif_req;\n",
			mod.m_modName.Lc().c_str(), memPortIdx, mod.m_modName.Lc().c_str(), memPortIdx);

		if (mod.m_clkRate == eClk2x && modMemPort.m_bWrite) {
			GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, "CMemRdWrReqIntf", VA("r_t4_%sP%dToMif_req", mod.m_modName.Lc().c_str(), memPortIdx));
			m_strmRegDecl.Append("\tCMemRdWrReqIntf c_t3_%sP%dToMif_req;\n", mod.m_modName.Lc().c_str(), memPortIdx);
			strmPreInstr.Append("\tc_t3_%sP%dToMif_req = r_t3_%sP%dToMif_req;\n",
				mod.m_modName.Lc().c_str(), memPortIdx, mod.m_modName.Lc().c_str(), memPortIdx);
			strmReg.Append("\tr_t4_%sP%dToMif_req = c_t3_%sP%dToMif_req;\n",
				mod.m_modName.Lc().c_str(), memPortIdx, mod.m_modName.Lc().c_str(), memPortIdx);
		}

		if (mod.m_clkRate == eClk2x && modMemPort.m_bWrite) {
			GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, "CMemRdWrReqIntf", VA("r_t5_%sP%dToMif_req", mod.m_modName.Lc().c_str(), memPortIdx));
			m_strmRegDecl.Append("\tCMemRdWrReqIntf c_t4_%sP%dToMif_req;\n", mod.m_modName.Lc().c_str(), memPortIdx);
			strmPreInstr.Append("\tc_t4_%sP%dToMif_req = r_t4_%sP%dToMif_req;\n",
				mod.m_modName.Lc().c_str(), memPortIdx, mod.m_modName.Lc().c_str(), memPortIdx);
			strmReg.Append("\tr_t5_%sP%dToMif_req = c_t4_%sP%dToMif_req;\n",
				mod.m_modName.Lc().c_str(), memPortIdx, mod.m_modName.Lc().c_str(), memPortIdx);
		}

		strmOut.Append("\to_%sP%dToMif_req = r_t%d_%sP%dToMif_req;\n",
			mod.m_modName.Lc().c_str(), memPortIdx,
			(mod.m_clkRate == eClk1x || !modMemPort.m_bWrite) ? 3 : 5, mod.m_modName.Lc().c_str(), memPortIdx);

		GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, "bool", VA("r_t3_%sP%dToMif_reqRdy", mod.m_modName.Lc().c_str(), memPortIdx));
		m_strmRegDecl.Append("\tbool c_t2_%sP%dToMif_reqRdy;\n", mod.m_modName.Lc().c_str(), memPortIdx);
		strmPreInstr.Append("\tc_t2_%sP%dToMif_reqRdy = false;\n",
			mod.m_modName.Lc().c_str(), memPortIdx);
		strmReg.Append("\tr_t3_%sP%dToMif_reqRdy = c_t2_%sP%dToMif_reqRdy;\n",
			mod.m_modName.Lc().c_str(), memPortIdx, mod.m_modName.Lc().c_str(), memPortIdx);

		if (mod.m_clkRate == eClk2x && modMemPort.m_bWrite) {
			GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, "bool", VA("r_t4_%sP%dToMif_reqRdy", mod.m_modName.Lc().c_str(), memPortIdx));
			m_strmRegDecl.Append("\tbool c_t3_%sP%dToMif_reqRdy;\n", mod.m_modName.Lc().c_str(), memPortIdx);
			strmPreInstr.Append("\tc_t3_%sP%dToMif_reqRdy = r_t3_%sP%dToMif_reqRdy;\n",
				mod.m_modName.Lc().c_str(), memPortIdx, mod.m_modName.Lc().c_str(), memPortIdx);
			strmReg.Append("\tr_t4_%sP%dToMif_reqRdy = c_t3_%sP%dToMif_reqRdy;\n",
				mod.m_modName.Lc().c_str(), memPortIdx, mod.m_modName.Lc().c_str(), memPortIdx);
		}

		if (mod.m_clkRate == eClk2x && modMemPort.m_bWrite) {
			GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, "bool", VA("r_t5_%sP%dToMif_reqRdy", mod.m_modName.Lc().c_str(), memPortIdx));
			m_strmRegDecl.Append("\tbool c_t4_%sP%dToMif_reqRdy;\n", mod.m_modName.Lc().c_str(), memPortIdx);
			strmPreInstr.Append("\tc_t4_%sP%dToMif_reqRdy = r_t4_%sP%dToMif_reqRdy;\n",
				mod.m_modName.Lc().c_str(), memPortIdx, mod.m_modName.Lc().c_str(), memPortIdx);
			strmReg.Append("\tr_t5_%sP%dToMif_reqRdy = c_t4_%sP%dToMif_reqRdy;\n",
				mod.m_modName.Lc().c_str(), memPortIdx, mod.m_modName.Lc().c_str(), memPortIdx);
		}

		strmOut.Append("\to_%sP%dToMif_reqRdy = r_t%d_%sP%dToMif_reqRdy;\n",
			mod.m_modName.Lc().c_str(), memPortIdx,
			(mod.m_clkRate == eClk1x || !modMemPort.m_bWrite) ? 3 : 5, mod.m_modName.Lc().c_str(), memPortIdx);

		GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, "ht_uint6", VA("r_%sP%dToMif_reqAvlCnt", mod.m_modName.Lc().c_str(), memPortIdx));
		m_strmRegDecl.Append("\tht_uint6 c_%sP%dToMif_reqAvlCnt;\n", mod.m_modName.Lc().c_str(), memPortIdx);
		strmPreInstr.Append("\tc_%sP%dToMif_reqAvlCnt = r_%sP%dToMif_reqAvlCnt;\n",
			mod.m_modName.Lc().c_str(), memPortIdx, mod.m_modName.Lc().c_str(), memPortIdx);
		strmReg.Append("\tr_%sP%dToMif_reqAvlCnt = r_reset1x ? (ht_uint6)32 : c_%sP%dToMif_reqAvlCnt;\n",
			mod.m_modName.Lc().c_str(), memPortIdx, mod.m_modName.Lc().c_str(), memPortIdx);
		m_strmAvlCntChk.Append("\t\tassert(r_%sP%dToMif_reqAvlCnt == 32);\n", mod.m_modName.Lc().c_str(), memPortIdx);

		GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, "bool", VA("r_%sP%dToMif_reqAvlCntBusy",
			mod.m_modName.Lc().c_str(), memPortIdx));
		m_strmRegDecl.Append("\tbool c_%sP%dToMif_reqAvlCntBusy;\n",
			mod.m_modName.Lc().c_str(), memPortIdx);
		strmPreInstr.Append("\tc_%sP%dToMif_reqAvlCntBusy = r_%sP%dToMif_reqAvlCntBusy;\n",
			mod.m_modName.Lc().c_str(), memPortIdx, mod.m_modName.Lc().c_str(), memPortIdx);
		strmReg.Append("\tr_%sP%dToMif_reqAvlCntBusy = !r_reset1x && c_%sP%dToMif_reqAvlCntBusy;\n",
			mod.m_modName.Lc().c_str(), memPortIdx, mod.m_modName.Lc().c_str(), memPortIdx);

		if (modMemPort.m_bRead) {
			GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, "bool", VA("r_mifTo%sP%d_rdRspRdy", mod.m_modName.Uc().c_str(), memPortIdx));
			strmReg.Append("\tr_mifTo%sP%d_rdRspRdy = i_mifTo%sP%d_rdRspRdy;\n",
				mod.m_modName.Uc().c_str(), memPortIdx, mod.m_modName.Uc().c_str(), memPortIdx);
			m_strmCtorInit.Append("\t\tr_mifTo%sP%d_rdRspRdy = false;\n", mod.m_modName.Uc().c_str(), memPortIdx);

			GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, "CMemRdRspIntf", VA("r_mifTo%sP%d_rdRsp", mod.m_modName.Uc().c_str(), memPortIdx));
			strmReg.Append("\tr_mifTo%sP%d_rdRsp = i_mifTo%sP%d_rdRsp;\n",
				mod.m_modName.Uc().c_str(), memPortIdx, mod.m_modName.Uc().c_str(), memPortIdx);
		}
		strmReg.Append("\n");
		m_strmRegDecl.Append("\n");
	}

	strmPostInstr.Append("\t// Stream Request Arbitration\n");
	for (size_t streamIdx = 0; streamIdx < mod.m_streamList.size(); streamIdx += 1) {
		CStream * pStrm = mod.m_streamList[streamIdx];

		string strmName = pStrm->m_name.size() == 0 ? "" : "_" + pStrm->m_name.AsStr();

		int bufPtrW = pStrm->m_bufPtrW;

		if (pStrm->m_bRead) {
			for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
				string strmIdxStr = pStrm->m_strmCnt.size() == 0 ? "" : VA("[%d]", i);

				strmPostInstr.Append("\t{ // rdStrm%s%s\n", strmName.c_str(), strmIdxStr.c_str());

				string selName;
				if (mod.m_clkRate == eClk2x)
					selName = VA(" && !r_%sP%dToMif_bReqSel", mod.m_modName.Lc().c_str(), pStrm->m_memPort[i]);

				strmPostInstr.Append("\t\tc_rdStrm%s_bufElemCnt%s = (r_rdStrm%s_reqWrPtr%s - r_rdStrm%s_rspRdPtr%s) & 0x%x;\n",
					strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), (1 << (bufPtrW - 1)) - 1);

				strmPostInstr.Append("\t\tc_rdStrm%s_bRspBufFull%s = c_rdStrm%s_bufElemCnt%s > %d;\n",
					strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), pStrm->m_bufFull);

				if (pStrm->m_bClose)
					strmPostInstr.Append("\t\tc_rdStrm%s_bReqRdy%s = r_rdStrm%s_bOpenReq%s && !c_rdStrm%s_bRspBufFull%s && r_rdStrm%s_reqCnt%s > 0%s;\n",
					strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), selName.c_str());
				else
					strmPostInstr.Append("\t\tc_rdStrm%s_bReqRdy%s = r_rdStrm%s_bOpenReq%s && !c_rdStrm%s_bRspBufFull%s%s;\n",
					strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), selName.c_str());

				strmPostInstr.Append("\t}\n");
				strmPostInstr.Append("\n");
			}
		} else {
			for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
				string strmIdxStr = pStrm->m_strmCnt.size() == 0 ? "" : VA("[%d]", i);

				strmPostInstr.Append("\t{ // wrStrm%s%s\n", strmName.c_str(), strmIdxStr.c_str());

				string selName;
				if (mod.m_clkRate == eClk2x)
					selName = VA(" && !r_%sP%dToMif_bReqSel", mod.m_modName.Lc().c_str(), pStrm->m_memPort[i]);

				string strmRspGrpIdStr = pStrm->m_rspGrpW.AsInt() == 0 ? "" : VA("[r_wrStrm%s_rspGrpId%s]", strmName.c_str(), strmIdxStr.c_str());

				if (mod.m_clkRate == eClk1x)
					strmPostInstr.Append("\t\tc_wrStrm%s_bReqRdy%s = r_wrStrm%s_bBufRd%s && !r_wrStrm%s_bCollision%s && !r_wrStrm%s_bRspGrpCntFull%s%s &&\n",
					strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmRspGrpIdStr.c_str(), selName.c_str());
				else
					strmPostInstr.Append("\t\tc_wrStrm%s_bReqRdy%s = r_wrStrm%s_bBufRd%s && !r_%sP%dToMif_bReqSel && !r_wrStrm%s_bRspGrpCntFull%s &&\n",
					strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), mod.m_modName.Lc().c_str(), pStrm->m_memPort[i], strmName.c_str(), strmRspGrpIdStr.c_str());

				if (pStrm->m_bClose)
					strmPostInstr.Append("\t\t\t(r_t3_wrStrm%s_reqQwRem%s > 0 || r_wrStrm%s_bufRdElemCnt%s >= %d || r_wrStrm%s_bClosingBufRd%s);\n",
					strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), pStrm->m_qwElemCnt * 8, strmName.c_str(), strmIdxStr.c_str());
				else
					strmPostInstr.Append("\t\t\t(r_t3_wrStrm%s_reqQwRem%s > 0 || r_wrStrm%s_bufRdElemCnt%s >= %d || r_wrStrm%s_bufRdRem%s <= r_wrStrm%s_bufRdElemCnt%s);\n",
					strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), pStrm->m_qwElemCnt * 8, strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());

				strmPostInstr.Append("\t}\n");
				strmPostInstr.Append("\n");
			}
		}
	}

	for (size_t memPortIdx = 0; memPortIdx < mod.m_memPortList.size(); memPortIdx += 1) {
		CModMemPort * pModMemPort = mod.m_memPortList[memPortIdx];

		if (pModMemPort->m_strmList.size() == 0) continue;

		if (pModMemPort->m_strmList.size() == 1) {
			CMemPortStrm & memPortStrm = pModMemPort->m_strmList[0];

			string strmName = memPortStrm.m_pStrm->m_name.size() == 0 ? "" : "_" + memPortStrm.m_pStrm->m_name.AsStr();
			string strmIdxStr = memPortStrm.m_pStrm->m_strmCnt.size() == 0 ? "" : VA("[%d]", memPortStrm.m_strmIdx);

			strmPostInstr.Append("\tc_t1_%sStrm%s_bReqSel%s = c_%sStrm%s_bReqRdy%s;\n",
				memPortStrm.m_bRead ? "rd" : "wr", strmName.c_str(), strmIdxStr.c_str(), memPortStrm.m_bRead ? "rd" : "wr", strmName.c_str(), strmIdxStr.c_str());

			if (mod.m_clkRate == eClk2x)
				strmPostInstr.Append("\tc_%sP%dToMif_bReqSel = c_t1_%sStrm%s_bReqSel%s;\n",
				mod.m_modName.Lc().c_str(), memPortIdx, memPortStrm.m_bRead ? "rd" : "wr", strmName.c_str(), strmIdxStr.c_str());

		} else {
			for (size_t arbRrId = 0; arbRrId < pModMemPort->m_strmList.size(); arbRrId += 1) {

				CMemPortStrm & memPortStrm = pModMemPort->m_strmList[arbRrId];

				string strmName = memPortStrm.m_pStrm->m_name.size() == 0 ? "" : "_" + memPortStrm.m_pStrm->m_name.AsStr();
				string strmIdxStr = memPortStrm.m_pStrm->m_strmCnt.size() == 0 ? "" : VA("[%d]", memPortStrm.m_strmIdx);

				strmPostInstr.Append("\tc_t1_%sStrm%s_bReqSel%s = c_%sStrm%s_bReqRdy%s && ((r_%sP%dToMif_reqRr & 0x%llx) != 0 || !(\n",
					memPortStrm.m_bRead ? "rd" : "wr", strmName.c_str(), strmIdxStr.c_str(),
					memPortStrm.m_bRead ? "rd" : "wr", strmName.c_str(), strmIdxStr.c_str(),
					mod.m_modName.Lc().c_str(), memPortIdx, 1ull << arbRrId);

				for (size_t j = 1; j < pModMemPort->m_strmList.size(); j += 1) {
					int k = (arbRrId + j) % pModMemPort->m_strmList.size();

					CMemPortStrm & memPortStrm2 = pModMemPort->m_strmList[k];

					string strmName = memPortStrm2.m_pStrm->m_name.size() == 0 ? "" : "_" + memPortStrm2.m_pStrm->m_name.AsStr();
					string strmIdxStr = memPortStrm2.m_pStrm->m_strmCnt.size() == 0 ? "" : VA("[%d]", memPortStrm2.m_strmIdx);

					uint32_t mask1 = (1ul << pModMemPort->m_strmList.size()) - 1;
					uint32_t mask2 = ((1ul << j) - 1) << (arbRrId + 1);
					uint32_t mask3 = (mask2 & mask1) | (mask2 >> pModMemPort->m_strmList.size());

					strmPostInstr.Append("\t\t(c_%sStrm%s_bReqRdy%s && (r_%sP%dToMif_reqRr & 0x%x) != 0)%s\n",
						memPortStrm2.m_bRead ? "rd" : "wr", strmName.c_str(), strmIdxStr.c_str(),
						mod.m_modName.Lc().c_str(), memPortIdx,
						mask3, j == pModMemPort->m_strmList.size() - 1 ? "));" : " ||");
				}
			}
			if (mod.m_clkRate == eClk2x) {
				strmPostInstr.Append("\tc_%sP%dToMif_bReqSel = ", mod.m_modName.Lc().c_str(), memPortIdx);

				string separator;
				for (size_t arbRrId = 0; arbRrId < pModMemPort->m_strmList.size(); arbRrId += 1) {
					CMemPortStrm & memPortStrm = pModMemPort->m_strmList[arbRrId];

					string strmName = memPortStrm.m_pStrm->m_name.size() == 0 ? "" : "_" + memPortStrm.m_pStrm->m_name.AsStr();
					string strmIdxStr = memPortStrm.m_pStrm->m_strmCnt.size() == 0 ? "" : VA("[%d]", memPortStrm.m_strmIdx);

					strmPostInstr.Append("%sc_t1_%sStrm%s_bReqSel%s",
						separator.c_str(), memPortStrm.m_bRead ? "rd" : "wr", strmName.c_str(), strmIdxStr.c_str());

					separator = " || ";
				}
				strmPostInstr.Append(";\n");
			}
		}

		strmPostInstr.Append("\n");
	}

	for (size_t streamIdx = 0; streamIdx < mod.m_streamList.size(); streamIdx += 1) {
		CStream * pStrm = mod.m_streamList[streamIdx];

		bool bMultiQwReq = bIsWx || bIsHc2 && pStrm->m_memSrc == "host";

		string strmName = pStrm->m_name.size() == 0 ? "" : "_" + pStrm->m_name.AsStr();

		int qwElemCnt = pStrm->m_qwElemCnt;
		int bufPtrW = pStrm->m_bufPtrW;	// sufficient to cover 3us of read latency
		int elemCntW = pStrm->m_elemCntW.AsInt();

		string strmIdDecl = pStrm->m_strmCnt.size() == 0 ? "" : VA("[%d]", pStrm->m_strmCnt.AsInt());
		int strmCnt = pStrm->m_strmCnt.AsInt();
		string strmIdStr = pStrm->m_strmCnt.size() == 0 ? "" : "[strmId]";

		vector<CHtString> strmIdVec;
		if (pStrm->m_strmCnt.size() > 0) {
			CHtString strmIdDimen;
			strmIdDimen.SetValue(pStrm->m_strmCnt.AsInt());
			strmIdVec.push_back(strmIdDimen);
		}

		int strmRspGrpCnt = 1 << pStrm->m_rspGrpW.AsInt();
		string strmRspGrpDecl = pStrm->m_rspGrpW.AsInt() == 0 ? "" : VA("[%d]", strmRspGrpCnt);
		string strmRspGrpStr = pStrm->m_rspGrpW.AsInt() == 0 ? "" : "[rspGrpId]";

		vector<CHtString> strmRspGrpVec;
		if (pStrm->m_rspGrpW.AsInt() > 0) {
			CHtString strmRspGrpDimen;
			strmRspGrpDimen.SetValue(strmRspGrpCnt);
			strmRspGrpVec.push_back(strmRspGrpDimen);
		}

		if (pStrm->m_bRead) {
			g_appArgs.GetDsnRpt().AddItem("void ReadStreamOpen%s(", strmName.c_str());
			m_strmFuncDecl.Append("\tvoid ReadStreamOpen%s(", strmName.c_str());
			m_strmFuncDef.Append("void CPers%s::ReadStreamOpen%s(",
				mod.m_modName.Uc().c_str(), strmName.c_str());

			if (pStrm->m_strmCnt.size() > 0) {
				g_appArgs.GetDsnRpt().AddText("ht_uint%d strmId, ", pStrm->m_strmCntW);
				m_strmFuncDecl.Append("ht_uint%d strmId, ", pStrm->m_strmCntW);
				m_strmFuncDef.Append("ht_uint%d strmId, ", pStrm->m_strmCntW);
			}

			g_appArgs.GetDsnRpt().AddText("ht_uint48 addr");
			m_strmFuncDecl.Append("ht_uint48 addr");
			m_strmFuncDef.Append("ht_uint48 addr");

			g_appArgs.GetDsnRpt().AddText(", ht_uint%d elemCnt", pStrm->m_elemCntW.AsInt());
			m_strmFuncDecl.Append(", ht_uint%d elemCnt", pStrm->m_elemCntW.AsInt());
			m_strmFuncDef.Append(", ht_uint%d elemCnt", pStrm->m_elemCntW.AsInt());

			if (pStrm->m_pTag != 0) {
				g_appArgs.GetDsnRpt().AddText(", %s tag", pStrm->m_pTag->m_typeName.c_str());
				m_strmFuncDecl.Append(", %s tag", pStrm->m_pTag->m_typeName.c_str());
				m_strmFuncDef.Append(", %s tag", pStrm->m_pTag->m_typeName.c_str());
			}

			g_appArgs.GetDsnRpt().AddText("t)\n");
			m_strmFuncDecl.Append(");\n");
			m_strmFuncDef.Append(")\n");

			m_strmFuncDef.Append("{\n");

			if (pStrm->m_strmCnt.size() > 0)
				m_strmFuncDef.Append("\tassert_msg(strmId < %d, \"Runtime check failed in CPers%s::ReadStreamOpen%s()"
				" - strmId out of range\");\n",
				strmCnt,
				mod.m_modName.Uc().c_str(), strmName.c_str());

			m_strmFuncDef.Append("\tassert_msg((addr & 0x%x) == 0, \"Runtime check failed in CPers%s::ReadStreamOpen%s()"
				" - address not aligned to element boundary (0x%x)\");\n",
				(1 << pStrm->m_elemByteW) - 1,
				mod.m_modName.Uc().c_str(), strmName.c_str(),
				1 << pStrm->m_elemByteW);

			m_strmFuncDef.Append("\tassert_msg(c_rdStrm%s_bOpenAvail%s, \"Runtime check failed in CPers%s::ReadStreamOpen%s()"
				" - expected ReadStreamBusy%s() to have been called and not busy\");\n",
				strmName.c_str(), strmIdStr.c_str(), mod.m_modName.Uc().c_str(), strmName.c_str(), strmName.c_str());

			m_strmFuncDef.Append("\tc_rdStrm%s_bOpenReq%s = elemCnt != 0;\n", strmName.c_str(), strmIdStr.c_str());

			m_strmFuncDef.Append("\tc_rdStrm%s_addr%s = addr;\n", strmName.c_str(), strmIdStr.c_str());
			m_strmFuncDef.Append("\tc_rdStrm%s_reqCnt%s = elemCnt;\n", strmName.c_str(), strmIdStr.c_str());
			m_strmFuncDef.Append("\n");

			if (pStrm->m_elemByteW < 3) {
				m_strmFuncDef.Append("\tc_rdStrm%s_reqWrPtr%s(%d,0) = addr(2,%d);\n",
					strmName.c_str(), strmIdStr.c_str(), 2 - pStrm->m_elemByteW,
					pStrm->m_elemByteW);
				m_strmFuncDef.Append("\n");
			}

			m_strmFuncDef.Append("\tCRdStrm%s_openRsp openRsp;\n", strmName.c_str());
			m_strmFuncDef.Append("\topenRsp.m_rdPtr = c_rdStrm%s_reqWrPtr%s;\n", strmName.c_str(), strmIdStr.c_str());
			if (!pStrm->m_bClose)
				m_strmFuncDef.Append("\topenRsp.m_cnt = elemCnt;\n");
			if (pStrm->m_pTag != 0)
				m_strmFuncDef.Append("\topenRsp.m_tag = tag;\n");

			m_strmFuncDef.Append("\n");

			m_strmFuncDef.Append("\tassert(!m_rdStrm%s_nextRspQue%s.full());\n", strmName.c_str(), strmIdStr.c_str());
			m_strmFuncDef.Append("\tif (elemCnt != 0)\n");
			m_strmFuncDef.Append("\t\tm_rdStrm%s_nextRspQue%s.push(openRsp);\n", strmName.c_str(), strmIdStr.c_str());
			m_strmFuncDef.Append("}\n");
			m_strmFuncDef.Append("\n");
		} else {
			g_appArgs.GetDsnRpt().AddItem("void WriteStreamOpen%s(", strmName.c_str());
			m_strmFuncDecl.Append("\tvoid WriteStreamOpen%s(", strmName.c_str());
			m_strmFuncDef.Append("void CPers%s::WriteStreamOpen%s(",
				mod.m_modName.Uc().c_str(), strmName.c_str());

			if (pStrm->m_strmCnt.size() > 0) {
				g_appArgs.GetDsnRpt().AddText("ht_uint%d strmId, ", pStrm->m_strmCntW);
				m_strmFuncDecl.Append("ht_uint%d strmId, ", pStrm->m_strmCntW);
				m_strmFuncDef.Append("ht_uint%d strmId, ", pStrm->m_strmCntW);
			}

			if (pStrm->m_rspGrpW.size() > 0 && pStrm->m_rspGrpW.AsInt() > 0) {
				g_appArgs.GetDsnRpt().AddText("ht_uint%d rspGrpId, ", pStrm->m_rspGrpW.AsInt());
				m_strmFuncDecl.Append("ht_uint%d rspGrpId, ", pStrm->m_rspGrpW.AsInt());
				m_strmFuncDef.Append("ht_uint%d rspGrpId, ", pStrm->m_rspGrpW.AsInt());
			}

			g_appArgs.GetDsnRpt().AddText("ht_uint48 addr");
			m_strmFuncDecl.Append("ht_uint48 addr");
			m_strmFuncDef.Append("ht_uint48 addr");

			if (pStrm->m_bClose) {
				g_appArgs.GetDsnRpt().AddText("t)\n");
				m_strmFuncDecl.Append(");\n");
				m_strmFuncDef.Append(")\n");
			} else {
				g_appArgs.GetDsnRpt().AddText(", ht_uint%d elemCnt)\n", pStrm->m_elemCntW.AsInt());
				m_strmFuncDecl.Append(", ht_uint%d elemCnt);\n", pStrm->m_elemCntW.AsInt());
				m_strmFuncDef.Append(", ht_uint%d elemCnt)\n", pStrm->m_elemCntW.AsInt());
			}

			m_strmFuncDef.Append("{\n");

			if (pStrm->m_strmCnt.size() > 0)
				m_strmFuncDef.Append("\tassert_msg(strmId < %d, \"Runtime check failed in CPers%s::WriteStreamOpen%s()"
				" - strmId out of range\");\n",
				strmCnt,
				mod.m_modName.Uc().c_str(), strmName.c_str());

			m_strmFuncDef.Append("\tassert_msg((addr & 0x%x) == 0, \"Runtime check failed in CPers%s::WriteStreamOpen%s()"
				" - address not aligned to element boundary (0x%x)\");\n",
				(1 << pStrm->m_elemByteW) - 1,
				mod.m_modName.Uc().c_str(), strmName.c_str(),
				1 << pStrm->m_elemByteW);

			m_strmFuncDef.Append("\tassert_msg(c_wrStrm%s_bOpenAvail%s, \"Runtime check failed in CPers%s::WriteStreamOpen%s()"
				" - expected WriteStreamBusy%s() to have been called and not busy\");\n",
				strmName.c_str(), strmIdStr.c_str(), mod.m_modName.Uc().c_str(), strmName.c_str(), strmName.c_str());

			char * pTab = "";
			if (!pStrm->m_bClose) {
				m_strmFuncDef.Append("\tif (elemCnt != 0) {\n");
				pTab = "\t";
			}
			m_strmFuncDef.Append("%s\tc_wrStrm%s_bOpenBusy%s = true;\n", pTab, strmName.c_str(), strmIdStr.c_str());
			m_strmFuncDef.Append("%s\tc_wrStrm%s_bOpenBufWr%s = true;\n", pTab, strmName.c_str(), strmIdStr.c_str());
			if (pStrm->m_elemByteW < 3) {
				m_strmFuncDef.Append("%s\tif (r_wrStrm%s_bufWrPtr%s(%d,0) != 0)\n", pTab,
					strmName.c_str(), strmIdStr.c_str(), 3 - pStrm->m_elemByteW - 1);
				m_strmFuncDef.Append("%s\t\tc_wrStrm%s_bufWrPtr%s(%d,%d) = r_wrStrm%s_bufWrPtr%s(%d,%d) + 1;\n", pTab,
					strmName.c_str(), strmIdStr.c_str(), bufPtrW - 1, 3 - pStrm->m_elemByteW,
					strmName.c_str(), strmIdStr.c_str(), bufPtrW - 1, 3 - pStrm->m_elemByteW);
				m_strmFuncDef.Append("%s\tc_wrStrm%s_bufWrPtr%s(%d,0) = addr(2,%d);\n", pTab,
					strmName.c_str(), strmIdStr.c_str(), 2 - pStrm->m_elemByteW, pStrm->m_elemByteW);
			}
			if (!pStrm->m_bClose) {
				m_strmFuncDef.Append("\t}\n");
				m_strmFuncDef.Append("\tc_wrStrm%s_bufWrRem%s = elemCnt;\n", strmName.c_str(), strmIdStr.c_str());
			}
			m_strmFuncDef.Append("\n");

			if (pStrm->m_bClose)
				m_strmFuncDef.Append("\tc_wrStrm%s_bOpenBufRd%s = true;\n", strmName.c_str(), strmIdStr.c_str());
			else
				m_strmFuncDef.Append("\tc_wrStrm%s_bOpenBufRd%s = elemCnt != 0;\n", strmName.c_str(), strmIdStr.c_str());
			m_strmFuncDef.Append("\tc_wrStrm%s_openAddr%s = addr;\n", strmName.c_str(), strmIdStr.c_str());
			if (!pStrm->m_bClose)
				m_strmFuncDef.Append("\tc_wrStrm%s_openCnt%s = elemCnt;\n", strmName.c_str(), strmIdStr.c_str());
			if (pStrm->m_rspGrpW.AsInt() > 0) {
				string wrGrpIdStr = pStrm->m_rspGrpW.size() == 0 ? VA("r_t%d_htId", mod.m_execStg) : "rspGrpId";
				m_strmFuncDef.Append("\tc_wrStrm%s_openRspGrpId%s = %s;\n", strmName.c_str(), strmIdStr.c_str(), wrGrpIdStr.c_str());
				if (pStrm->m_bClose)
					m_strmFuncDef.Append("\t\tc_wrStrm%s_rspGrpCnt[%s] += 1;\n", strmName.c_str(), wrGrpIdStr.c_str());
				else {
					m_strmFuncDef.Append("\tif (elemCnt != 0)\n");
					m_strmFuncDef.Append("\t\tc_wrStrm%s_rspGrpCnt[%s] += 1;\n", strmName.c_str(), wrGrpIdStr.c_str());
				}
			} else {
				if (pStrm->m_bClose)
					m_strmFuncDef.Append("\tc_wrStrm%s_rspGrpCnt += 1u;\n", strmName.c_str());
				else {
					m_strmFuncDef.Append("\tif (elemCnt != 0)\n");
					m_strmFuncDef.Append("\t\tc_wrStrm%s_rspGrpCnt += 1u;\n", strmName.c_str());
				}
			}

			m_strmFuncDef.Append("}\n");
			m_strmFuncDef.Append("\n");
		}

		if (pStrm->m_bRead) {
			if (pStrm->m_bClose) {
				g_appArgs.GetDsnRpt().AddItem("void ReadStreamClose%s(", strmName.c_str());
				m_strmFuncDecl.Append("\tvoid ReadStreamClose%s(", strmName.c_str());
				m_strmFuncDef.Append("void CPers%s::ReadStreamClose%s(",
					mod.m_modName.Uc().c_str(), strmName.c_str());

				if (pStrm->m_strmCnt.size() > 0) {
					g_appArgs.GetDsnRpt().AddText("ht_uint%d strmId", pStrm->m_strmCntW);
					m_strmFuncDecl.Append("ht_uint%d strmId", pStrm->m_strmCntW);
					m_strmFuncDef.Append("ht_uint%d strmId", pStrm->m_strmCntW);
				}

				g_appArgs.GetDsnRpt().AddText("t)\n");
				m_strmFuncDecl.Append(");\n");
				m_strmFuncDef.Append(")\n");

				m_strmFuncDef.Append("{\n");

				if (pStrm->m_strmCnt.size() > 0)
					m_strmFuncDef.Append("\tassert_msg(strmId < %d, \"Runtime check failed in CPers%s::ReadStreamClose%s()"
					" - strmId out of range\");\n",
					strmCnt,
					mod.m_modName.Uc().c_str(), strmName.c_str());

				m_strmFuncDef.Append("\tc_rdStrm%s_bClosingRsp%s = true;\n", strmName.c_str(), strmIdStr.c_str());
				m_strmFuncDef.Append("\tc_rdStrm%s_bClose%s = true;\n", strmName.c_str(), strmIdStr.c_str());
				m_strmFuncDef.Append("\tc_rdStrm%s_bOpenReq%s = false;\n", strmName.c_str(), strmIdStr.c_str());
				m_strmFuncDef.Append("\tc_rdStrm%s_bOpenRsp%s = true;\n", strmName.c_str(), strmIdStr.c_str());
				m_strmFuncDef.Append("}\n");
				m_strmFuncDef.Append("\n");
			}
		} else {
			if (pStrm->m_bClose) {
				g_appArgs.GetDsnRpt().AddItem("void WriteStreamClose%s(", strmName.c_str());
				m_strmFuncDecl.Append("\tvoid WriteStreamClose%s(", strmName.c_str());
				m_strmFuncDef.Append("void CPers%s::WriteStreamClose%s(",
					mod.m_modName.Uc().c_str(), strmName.c_str());

				if (pStrm->m_strmCnt.size() > 0) {
					g_appArgs.GetDsnRpt().AddText("ht_uint%d strmId", pStrm->m_strmCntW);
					m_strmFuncDecl.Append("ht_uint%d strmId", pStrm->m_strmCntW);
					m_strmFuncDef.Append("ht_uint%d strmId", pStrm->m_strmCntW);
				}

				g_appArgs.GetDsnRpt().AddText("t)\n");
				m_strmFuncDecl.Append(");\n");
				m_strmFuncDef.Append(")\n");

				m_strmFuncDef.Append("{\n");

				if (pStrm->m_strmCnt.size() > 0)
					m_strmFuncDef.Append("\tassert_msg(strmId < %d, \"Runtime check failed in CPers%s::WriteStreamClose%s()"
					" - strmId out of range\");\n",
					strmCnt,
					mod.m_modName.Uc().c_str(), strmName.c_str());

				if (pStrm->m_reserve.AsInt() > 0) {
					m_strmFuncDef.Append("\tc_wrStrm%s_strmWrEn%s = true;\n", strmName.c_str(), strmIdStr.c_str());
					m_strmFuncDef.Append("\tc_wrStrm%s_strmWrData%s.m_bClose = true;\n", strmName.c_str(), strmIdStr.c_str());
				} else {
					m_strmFuncDef.Append("\tc_wrStrm%s_bOpenBufWr%s = false;\n", strmName.c_str(), strmIdStr.c_str());
					m_strmFuncDef.Append("\tc_wrStrm%s_bClose%s = true;\n", strmName.c_str(), strmIdStr.c_str());
				}

				m_strmFuncDef.Append("}\n");
				m_strmFuncDef.Append("\n");
			}
		}

		if (pStrm->m_bRead) {

		} else {
			g_appArgs.GetDsnRpt().AddItem("void WriteStreamPause%s(", strmName.c_str());
			m_strmFuncDecl.Append("\tvoid WriteStreamPause%s(", strmName.c_str());
			m_strmFuncDef.Append("void CPers%s::WriteStreamPause%s(",
				mod.m_modName.Uc().c_str(), strmName.c_str());

			if (pStrm->m_rspGrpW.size() > 0 && pStrm->m_rspGrpW.AsInt() > 0) {
				g_appArgs.GetDsnRpt().AddText("ht_uint%d rspGrpId, ", pStrm->m_rspGrpW.AsInt());
				m_strmFuncDecl.Append("ht_uint%d rspGrpId, ", pStrm->m_rspGrpW.AsInt());
				m_strmFuncDef.Append("ht_uint%d rspGrpId, ", pStrm->m_rspGrpW.AsInt());
			}

			g_appArgs.GetDsnRpt().AddText("sc_uint<%s_INSTR_W> nextInstr)\n", mod.m_modName.Upper().c_str());
			m_strmFuncDecl.Append("sc_uint<%s_INSTR_W> nextInstr);\n", mod.m_modName.Upper().c_str());
			m_strmFuncDef.Append("sc_uint<%s_INSTR_W> nextInstr)\n", mod.m_modName.Upper().c_str());

			m_strmFuncDef.Append("{\n");

			string rspGrpName;
			string rspGrpIdx;
			if (pStrm->m_rspGrpW.AsInt() > 0) {
				rspGrpName = pStrm->m_rspGrpW.size() == 0 ? VA("r_t%d_htId", mod.m_execStg) : "rspGrpId";
				rspGrpIdx = "[" + rspGrpName + "]";
			}

			m_strmFuncDef.Append("\tassert_msg(c_t%d_htCtrl == HT_INVALID, \"Runtime check failed in CPers%s::WriteStreamPause%s()"
				" - an Ht control routine was already called\");\n", mod.m_execStg, mod.m_modName.Uc().c_str(), strmName.c_str());
			m_strmFuncDef.Append("\tassert_msg(!r_wrStrm%s_bPaused%s, \"Runtime check failed in CPers%s::WriteStreamPause%s()"
				" - pause already active\");\n",
				strmName.c_str(), rspGrpIdx.c_str(),
				mod.m_modName.Uc().c_str(), strmName.c_str());
			m_strmFuncDef.Append("\tc_t%d_htCtrl = HT_PAUSE;\n", mod.m_execStg);

			if (mod.m_threads.m_htIdW.AsInt() > 0 && pStrm->m_rspGrpW.size() > 0) {
				if (pStrm->m_rspGrpW.AsInt() == 0)
					m_strmFuncDef.Append("\tc_wrStrm%s_rsmHtId = r_t%d_htId;\n", strmName.c_str(), mod.m_execStg);
				else {
					m_strmFuncDef.Append("\tm_wrStrm%s_rsmHtId.write_addr(%s);\n", strmName.c_str(), rspGrpName.c_str());
					m_strmFuncDef.Append("\tm_wrStrm%s_rsmHtId.write_mem(r_t%d_htId);\n", strmName.c_str(), mod.m_execStg);
				}
			}

			if (pStrm->m_rspGrpW.AsInt() == 0)
				m_strmFuncDef.Append("\tc_wrStrm%s_rsmHtInstr = nextInstr;\n", strmName.c_str());
			else {
				m_strmFuncDef.Append("\tm_wrStrm%s_rsmHtInstr.write_addr(%s);\n", strmName.c_str(), rspGrpName.c_str());
				m_strmFuncDef.Append("\tm_wrStrm%s_rsmHtInstr.write_mem(nextInstr);\n", strmName.c_str());
			}

			if (mod.m_threads.m_htIdW.AsInt() > 0) {
				m_strmFuncDef.Append("#\tifndef _HTV\n");
				m_strmFuncDef.Append("\tassert(m_htRsmWait.read_mem_debug(r_t%d_htId) == false);\n", mod.m_execStg);
				m_strmFuncDef.Append("\tm_htRsmWait.write_mem_debug(r_t%d_htId) = true;\n", mod.m_execStg);
				m_strmFuncDef.Append("#\tendif\n");
			} else {
				m_strmFuncDef.Append("#\tifndef _HTV\n");
				m_strmFuncDef.Append("\tassert(r_htRsmWait == false);\n");
				m_strmFuncDef.Append("\tc_htRsmWait = true;\n");
				m_strmFuncDef.Append("#\tendif\n");
			}

			m_strmFuncDef.Append("\tc_wrStrm%s_bPaused%s = true;\n", strmName.c_str(), rspGrpIdx.c_str());

			m_strmFuncDef.Append("}\n");
			m_strmFuncDef.Append("\n");
		}

		if (pStrm->m_bRead) {
			g_appArgs.GetDsnRpt().AddItem("bool ReadStreamReady%s(", strmName.c_str());
			m_strmFuncDecl.Append("\tbool ReadStreamReady%s(", strmName.c_str());
			m_strmFuncDef.Append("bool CPers%s::ReadStreamReady%s(",
				mod.m_modName.Uc().c_str(), strmName.c_str());

			if (pStrm->m_strmCnt.size() > 0) {
				g_appArgs.GetDsnRpt().AddText("ht_uint%d strmId", pStrm->m_strmCntW);
				m_strmFuncDecl.Append("ht_uint%d strmId", pStrm->m_strmCntW);
				m_strmFuncDef.Append("ht_uint%d strmId", pStrm->m_strmCntW);
			}

			g_appArgs.GetDsnRpt().AddText(")\n");
			m_strmFuncDecl.Append(");\n");
			m_strmFuncDef.Append(")\n");

			m_strmFuncDef.Append("{\n");

			if (pStrm->m_strmCnt.size() > 0)
				m_strmFuncDef.Append("\tassert_msg(strmId < %d, \"Runtime check failed in CPers%s::ReadStreamReady%s()"
				" - strmId out of range\");\n",
				strmCnt,
				mod.m_modName.Uc().c_str(), strmName.c_str());

			if (pStrm->m_strmCnt.size() == 0)
				m_strmFuncDef.Append("\treturn r_rdStrm%s_bValid;\n", strmName.c_str());
			else
				m_strmFuncDef.Append("\treturn r_rdStrm%s_bValid[strmId];\n", strmName.c_str());

			m_strmFuncDef.Append("}\n");
			m_strmFuncDef.Append("\n");

		} else {
			g_appArgs.GetDsnRpt().AddItem("bool WriteStreamReady%s(", strmName.c_str());
			m_strmFuncDecl.Append("\tbool WriteStreamReady%s(", strmName.c_str());
			m_strmFuncDef.Append("bool CPers%s::WriteStreamReady%s(",
				mod.m_modName.Uc().c_str(), strmName.c_str());

			if (pStrm->m_strmCnt.size() > 0) {
				g_appArgs.GetDsnRpt().AddText("ht_uint%d strmId", pStrm->m_strmCntW);
				m_strmFuncDecl.Append("ht_uint%d strmId", pStrm->m_strmCntW);
				m_strmFuncDef.Append("ht_uint%d strmId", pStrm->m_strmCntW);
			}

			g_appArgs.GetDsnRpt().AddText(")\n");
			m_strmFuncDecl.Append(");\n");
			m_strmFuncDef.Append(")\n");

			m_strmFuncDef.Append("{\n");

			if (pStrm->m_strmCnt.size() > 0)
				m_strmFuncDef.Append("\tassert_msg(strmId < %d, \"Runtime check failed in CPers%s::WriteStreamReady%s()"
				" - strmId out of range\");\n",
				strmCnt,
				mod.m_modName.Uc().c_str(), strmName.c_str());

			if (pStrm->m_reserve.AsInt() == 0) {
				if (pStrm->m_strmCnt.size() == 0)
					m_strmFuncDef.Append("\treturn r_wrStrm%s_bOpenBufWr && !r_wrStrm%s_bBufFull;\n", strmName.c_str(), strmName.c_str());
				else
					m_strmFuncDef.Append("\treturn r_wrStrm%s_bOpenBufWr[strmId] && !r_wrStrm%s_bBufFull[strmId];\n", strmName.c_str(), strmName.c_str());
			} else {
				if (pStrm->m_strmCnt.size() == 0)
					m_strmFuncDef.Append("\treturn !r_wrStrm%s_bPipeQueFull;\n", strmName.c_str());
				else
					m_strmFuncDef.Append("\treturn !r_wrStrm%s_bPipeQueFull[strmId];\n", strmName.c_str());
			}

			m_strmFuncDef.Append("}\n");
			m_strmFuncDef.Append("\n");
		}

		if (pStrm->m_strmCnt.size() > 0) {
			if (pStrm->m_bRead) {
				g_appArgs.GetDsnRpt().AddItem("bool ReadStreamReadyMask%s(", strmName.c_str());
				m_strmFuncDecl.Append("\tbool ReadStreamReadyMask%s(", strmName.c_str());
				m_strmFuncDef.Append("bool CPers%s::ReadStreamReadyMask%s(",
					mod.m_modName.Uc().c_str(), strmName.c_str());

				g_appArgs.GetDsnRpt().AddText("ht_uint%d strmMask", pStrm->m_strmCnt.AsInt());
				m_strmFuncDecl.Append("ht_uint%d strmMask", pStrm->m_strmCnt.AsInt());
				m_strmFuncDef.Append("ht_uint%d strmMask", pStrm->m_strmCnt.AsInt());

				g_appArgs.GetDsnRpt().AddText(")\n");
				m_strmFuncDecl.Append(");\n");
				m_strmFuncDef.Append(")\n");

				m_strmFuncDef.Append("{\n");

				string preLine = "\treturn ";
				for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
					m_strmFuncDef.Append("%s(!strmMask[%d] || r_rdStrm%s_bValid[%d])",
						preLine.c_str(), i, strmName.c_str(), i);
					preLine = " &&\n\t\t";
				}
				m_strmFuncDef.Append(";\n");

				m_strmFuncDef.Append("}\n");
				m_strmFuncDef.Append("\n");

			} else {
				g_appArgs.GetDsnRpt().AddItem("bool WriteStreamReadyMask%s(", strmName.c_str());
				m_strmFuncDecl.Append("\tbool WriteStreamReadyMask%s(", strmName.c_str());
				m_strmFuncDef.Append("bool CPers%s::WriteStreamReadyMask%s(",
					mod.m_modName.Uc().c_str(), strmName.c_str());

				g_appArgs.GetDsnRpt().AddText("ht_uint%d strmMask", pStrm->m_strmCnt.AsInt());
				m_strmFuncDecl.Append("ht_uint%d strmMask", pStrm->m_strmCnt.AsInt());
				m_strmFuncDef.Append("ht_uint%d strmMask", pStrm->m_strmCnt.AsInt());

				g_appArgs.GetDsnRpt().AddText(")\n");
				m_strmFuncDecl.Append(");\n");
				m_strmFuncDef.Append(")\n");

				m_strmFuncDef.Append("{\n");

				if (pStrm->m_reserve.AsInt() == 0) {
					string preLine = "\treturn ";
					for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
						m_strmFuncDef.Append("%s(!strmMask[%d] || r_wrStrm%s_bOpenBufWr[%d] && !r_wrStrm%s_bBufFull[%d])",
							preLine.c_str(), i, strmName.c_str(), i, strmName.c_str(), i);
						preLine = " &&\n\t\t";
					}
					m_strmFuncDef.Append(";\n");
				} else {
					string preLine = "\treturn ";
					for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
						m_strmFuncDef.Append("%s(!strmMask[%d] || !r_wrStrm%s_bPipeQueFull[%d])",
							preLine.c_str(), i, strmName.c_str(), i);
						preLine = " &&\n\t\t";
					}
					m_strmFuncDef.Append(";\n");
				}

				m_strmFuncDef.Append("}\n");
				m_strmFuncDef.Append("\n");
			}
		}

		if (pStrm->m_bRead && !pStrm->m_bClose) {
			g_appArgs.GetDsnRpt().AddItem("bool ReadStreamLast%s(", strmName.c_str());
			m_strmFuncDecl.Append("\tbool ReadStreamLast%s(", strmName.c_str());
			m_strmFuncDef.Append("bool CPers%s::ReadStreamLast%s(",
				mod.m_modName.Uc().c_str(), strmName.c_str());

			if (pStrm->m_strmCnt.size() > 0) {
				g_appArgs.GetDsnRpt().AddText("ht_uint%d strmId", pStrm->m_strmCntW);
				m_strmFuncDecl.Append("ht_uint%d strmId", pStrm->m_strmCntW);
				m_strmFuncDef.Append("ht_uint%d strmId", pStrm->m_strmCntW);
			}

			g_appArgs.GetDsnRpt().AddText(")\n");
			m_strmFuncDecl.Append(");\n");
			m_strmFuncDef.Append(")\n");

			m_strmFuncDef.Append("{\n");

			if (pStrm->m_strmCnt.size() > 0)
				m_strmFuncDef.Append("\tassert_msg(strmId < %d, \"Runtime check failed in CPers%s::ReadStreamLast%s()"
				" - strmId out of range\");\n",
				strmCnt,
				mod.m_modName.Uc().c_str(), strmName.c_str());

			if (pStrm->m_strmCnt.size() == 0)
				m_strmFuncDef.Append("\treturn r_rdStrm%s_bLast;\n", strmName.c_str());
			else
				m_strmFuncDef.Append("\treturn r_rdStrm%s_bLast[strmId];\n", strmName.c_str());

			m_strmFuncDef.Append("}\n");
			m_strmFuncDef.Append("\n");

		}

		if (pStrm->m_bRead) {
			g_appArgs.GetDsnRpt().AddItem("bool ReadStreamBusy%s(", strmName.c_str());
			m_strmFuncDecl.Append("\tbool ReadStreamBusy%s(", strmName.c_str());
			m_strmFuncDef.Append("bool CPers%s::ReadStreamBusy%s(",
				mod.m_modName.Uc().c_str(), strmName.c_str());

			if (pStrm->m_strmCnt.size() > 0) {
				g_appArgs.GetDsnRpt().AddText("ht_uint%d strmId", pStrm->m_strmCntW);
				m_strmFuncDecl.Append("ht_uint%d strmId", pStrm->m_strmCntW);
				m_strmFuncDef.Append("ht_uint%d strmId", pStrm->m_strmCntW);
			}

			g_appArgs.GetDsnRpt().AddText(")\n");
			m_strmFuncDecl.Append(");\n");
			m_strmFuncDef.Append(")\n");

			m_strmFuncDef.Append("{\n");

			if (pStrm->m_strmCnt.size() > 0)
				m_strmFuncDef.Append("\tassert_msg(strmId < %d, \"Runtime check failed in CPers%s::ReadStreamBusy%s()"
				" - strmId out of range\");\n",
				strmCnt,
				mod.m_modName.Uc().c_str(), strmName.c_str());

			if (pStrm->m_strmCnt.size() == 0) {
				m_strmFuncDef.Append("\tc_rdStrm%s_bOpenAvail = !r_rdStrm%s_bOpenBusy;\n", strmName.c_str(), strmName.c_str());
				m_strmFuncDef.Append("\treturn r_rdStrm%s_bOpenBusy;\n", strmName.c_str());
			} else {
				m_strmFuncDef.Append("\tc_rdStrm%s_bOpenAvail[strmId] = !r_rdStrm%s_bOpenBusy[strmId];\n", strmName.c_str(), strmName.c_str());
				m_strmFuncDef.Append("\treturn r_rdStrm%s_bOpenBusy[strmId];\n", strmName.c_str());
			}

			m_strmFuncDef.Append("}\n");
			m_strmFuncDef.Append("\n");

		} else {
			g_appArgs.GetDsnRpt().AddItem("bool WriteStreamBusy%s(", strmName.c_str());
			m_strmFuncDecl.Append("\tbool WriteStreamBusy%s(", strmName.c_str());
			m_strmFuncDef.Append("bool CPers%s::WriteStreamBusy%s(",
				mod.m_modName.Uc().c_str(), strmName.c_str());

			if (pStrm->m_strmCnt.size() > 0) {
				g_appArgs.GetDsnRpt().AddText("ht_uint%d strmId", pStrm->m_strmCntW);
				m_strmFuncDecl.Append("ht_uint%d strmId", pStrm->m_strmCntW);
				m_strmFuncDef.Append("ht_uint%d strmId", pStrm->m_strmCntW);
			}

			g_appArgs.GetDsnRpt().AddText(")\n");
			m_strmFuncDecl.Append(");\n");
			m_strmFuncDef.Append(")\n");

			m_strmFuncDef.Append("{\n");

			if (pStrm->m_strmCnt.size() > 0)
				m_strmFuncDef.Append("\tassert_msg(strmId < %d, \"Runtime check failed in CPers%s::WriteStreamBusy%s()"
				" - strmId out of range\");\n",
				strmCnt,
				mod.m_modName.Uc().c_str(), strmName.c_str());

			if (pStrm->m_strmCnt.size() == 0) {
				m_strmFuncDef.Append("\tc_wrStrm%s_bOpenAvail = !r_wrStrm%s_bOpenBusy;\n", strmName.c_str(), strmName.c_str());
				m_strmFuncDef.Append("\treturn r_wrStrm%s_bOpenBusy;\n", strmName.c_str());
			} else {
				m_strmFuncDef.Append("\tc_wrStrm%s_bOpenAvail[strmId] = !r_wrStrm%s_bOpenBusy[strmId];\n", strmName.c_str(), strmName.c_str());
				m_strmFuncDef.Append("\treturn r_wrStrm%s_bOpenBusy[strmId];\n", strmName.c_str());
			}

			m_strmFuncDef.Append("}\n");
			m_strmFuncDef.Append("\n");
		}

		if (pStrm->m_strmCnt.size() > 0) {
			if (pStrm->m_bRead) {
				g_appArgs.GetDsnRpt().AddItem("bool ReadStreamBusyMask%s(", strmName.c_str());
				m_strmFuncDecl.Append("\tbool ReadStreamBusyMask%s(", strmName.c_str());
				m_strmFuncDef.Append("bool CPers%s::ReadStreamBusyMask%s(",
					mod.m_modName.Uc().c_str(), strmName.c_str());

				g_appArgs.GetDsnRpt().AddText("ht_uint%d strmMask", pStrm->m_strmCnt.AsInt());
				m_strmFuncDecl.Append("ht_uint%d strmMask", pStrm->m_strmCnt.AsInt());
				m_strmFuncDef.Append("ht_uint%d strmMask", pStrm->m_strmCnt.AsInt());

				g_appArgs.GetDsnRpt().AddText(")\n");
				m_strmFuncDecl.Append(");\n");
				m_strmFuncDef.Append(")\n");

				m_strmFuncDef.Append("{\n");

				for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1)
					m_strmFuncDef.Append("\tc_rdStrm%s_bOpenAvail[%d] |= strmMask[%d] && !r_rdStrm%s_bOpenBusy[%d];\n",
					strmName.c_str(), i, i, strmName.c_str(), i);

				string preLine = "\treturn ";
				for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
					m_strmFuncDef.Append("%sstrmMask[%d] && r_rdStrm%s_bOpenBusy[%d]",
						preLine.c_str(), i, strmName.c_str(), i);
					preLine = " ||\n\t\t";
				}
				m_strmFuncDef.Append(";\n");

				m_strmFuncDef.Append("}\n");
				m_strmFuncDef.Append("\n");

			} else {
				g_appArgs.GetDsnRpt().AddItem("bool WriteStreamBusyMask%s(", strmName.c_str());
				m_strmFuncDecl.Append("\tbool WriteStreamBusyMask%s(", strmName.c_str());
				m_strmFuncDef.Append("bool CPers%s::WriteStreamBusyMask%s(",
					mod.m_modName.Uc().c_str(), strmName.c_str());

				g_appArgs.GetDsnRpt().AddText("ht_uint%d strmMask", pStrm->m_strmCnt.AsInt());
				m_strmFuncDecl.Append("ht_uint%d strmMask", pStrm->m_strmCnt.AsInt());
				m_strmFuncDef.Append("ht_uint%d strmMask", pStrm->m_strmCnt.AsInt());

				g_appArgs.GetDsnRpt().AddText(")\n");
				m_strmFuncDecl.Append(");\n");
				m_strmFuncDef.Append(")\n");

				m_strmFuncDef.Append("{\n");

				for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1)
					m_strmFuncDef.Append("\tc_wrStrm%s_bOpenAvail[%d] |= strmMask[%d] && !r_wrStrm%s_bOpenBusy[%d];\n",
					strmName.c_str(), i, i, strmName.c_str(), i);

				string preLine = "\treturn ";
				for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
					m_strmFuncDef.Append("%sstrmMask[%d] && r_wrStrm%s_bOpenBusy[%d]",
						preLine.c_str(), i, strmName.c_str(), i);
					preLine = " ||\n\t\t";
				}
				m_strmFuncDef.Append(";\n");

				m_strmFuncDef.Append("}\n");
				m_strmFuncDef.Append("\n");
			}
		}

		if (pStrm->m_bRead) {
			g_appArgs.GetDsnRpt().AddItem("%s ReadStreamPeek%s(", pStrm->m_pType->m_typeName.c_str(), strmName.c_str());
			m_strmFuncDecl.Append("\t%s ReadStreamPeek%s(", pStrm->m_pType->m_typeName.c_str(), strmName.c_str());
			m_strmFuncDef.Append("%s CPers%s::ReadStreamPeek%s(",
				pStrm->m_pType->m_typeName.c_str(), mod.m_modName.Uc().c_str(), strmName.c_str());

			if (pStrm->m_strmCnt.size() > 0) {
				g_appArgs.GetDsnRpt().AddText("ht_uint%d strmId", pStrm->m_strmCntW);
				m_strmFuncDecl.Append("ht_uint%d strmId", pStrm->m_strmCntW);
				m_strmFuncDef.Append("ht_uint%d strmId", pStrm->m_strmCntW);
			}

			g_appArgs.GetDsnRpt().AddText(")\n");
			m_strmFuncDecl.Append(");\n");
			m_strmFuncDef.Append(")\n");

			m_strmFuncDef.Append("{\n");

			if (pStrm->m_strmCnt.size() > 0)
				m_strmFuncDef.Append("\tassert_msg(strmId < %d, \"Runtime check failed in CPers%s::ReadStreamPeek%s()"
				" - strmId out of range\");\n",
				strmCnt,
				mod.m_modName.Uc().c_str(), strmName.c_str());

			m_strmFuncDef.Append("\treturn r_rdStrm%s_data%s;\n", strmName.c_str(), strmIdStr.c_str());
			m_strmFuncDef.Append("}\n");
			m_strmFuncDef.Append("\n");
		}

		if (pStrm->m_bRead) {
			g_appArgs.GetDsnRpt().AddItem("%s ReadStream%s(", pStrm->m_pType->m_typeName.c_str(), strmName.c_str());
			m_strmFuncDecl.Append("\t%s ReadStream%s(", pStrm->m_pType->m_typeName.c_str(), strmName.c_str());
			m_strmFuncDef.Append("%s CPers%s::ReadStream%s(",
				pStrm->m_pType->m_typeName.c_str(), mod.m_modName.Uc().c_str(), strmName.c_str());

			if (pStrm->m_strmCnt.size() > 0) {
				g_appArgs.GetDsnRpt().AddText("ht_uint%d strmId", pStrm->m_strmCntW);
				m_strmFuncDecl.Append("ht_uint%d strmId", pStrm->m_strmCntW);
				m_strmFuncDef.Append("ht_uint%d strmId", pStrm->m_strmCntW);
			}

			g_appArgs.GetDsnRpt().AddText(")\n");
			m_strmFuncDecl.Append(");\n");
			m_strmFuncDef.Append(")\n");

			m_strmFuncDef.Append("{\n");

			if (pStrm->m_strmCnt.size() > 0)
				m_strmFuncDef.Append("\tassert_msg(strmId < %d, \"Runtime check failed in CPers%s::ReadStream%s()"
				" - strmId out of range\");\n",
				strmCnt,
				mod.m_modName.Uc().c_str(), strmName.c_str());

			m_strmFuncDef.Append("\tc_rdStrm%s_bValid%s = false;\n", strmName.c_str(), strmIdStr.c_str());
			m_strmFuncDef.Append("\treturn r_rdStrm%s_data%s;\n", strmName.c_str(), strmIdStr.c_str());
			m_strmFuncDef.Append("}\n");
			m_strmFuncDef.Append("\n");
		}

		if (pStrm->m_bRead && pStrm->m_pTag != 0) {
			g_appArgs.GetDsnRpt().AddItem("%s ReadStreamTag%s(", pStrm->m_pTag->m_typeName.c_str(), strmName.c_str());
			m_strmFuncDecl.Append("\t%s ReadStreamTag%s(", pStrm->m_pTag->m_typeName.c_str(), strmName.c_str());
			m_strmFuncDef.Append("%s CPers%s::ReadStreamTag%s(",
				pStrm->m_pTag->m_typeName.c_str(), mod.m_modName.Uc().c_str(), strmName.c_str());

			if (pStrm->m_strmCnt.size() > 0) {
				g_appArgs.GetDsnRpt().AddText("ht_uint%d strmId", pStrm->m_strmCntW);
				m_strmFuncDecl.Append("ht_uint%d strmId", pStrm->m_strmCntW);
				m_strmFuncDef.Append("ht_uint%d strmId", pStrm->m_strmCntW);
			}

			g_appArgs.GetDsnRpt().AddText(")\n");
			m_strmFuncDecl.Append(");\n");
			m_strmFuncDef.Append(")\n");

			m_strmFuncDef.Append("{\n");

			if (pStrm->m_strmCnt.size() > 0)
				m_strmFuncDef.Append("\tassert_msg(strmId < %d, \"Runtime check failed in CPers%s::ReadStreamTag%s()"
				" - strmId out of range\");\n",
				strmCnt,
				mod.m_modName.Uc().c_str(), strmName.c_str());

			m_strmFuncDef.Append("\treturn r_rdStrm%s_tag%s;\n", strmName.c_str(), strmIdStr.c_str());
			m_strmFuncDef.Append("}\n");
			m_strmFuncDef.Append("\n");
		}

		if (!pStrm->m_bRead) {
			g_appArgs.GetDsnRpt().AddItem("void WriteStream%s(", strmName.c_str());
			m_strmFuncDecl.Append("\tvoid WriteStream%s(", strmName.c_str());
			m_strmFuncDef.Append("void CPers%s::WriteStream%s(",
				mod.m_modName.Uc().c_str(), strmName.c_str());

			if (pStrm->m_strmCnt.size() > 0) {
				g_appArgs.GetDsnRpt().AddText("ht_uint%d strmId, ", pStrm->m_strmCntW);
				m_strmFuncDecl.Append("ht_uint%d strmId, ", pStrm->m_strmCntW);
				m_strmFuncDef.Append("ht_uint%d strmId, ", pStrm->m_strmCntW);
			}

			g_appArgs.GetDsnRpt().AddText("%s data)\n", pStrm->m_pType->m_typeName.c_str());
			m_strmFuncDecl.Append("%s data);\n", pStrm->m_pType->m_typeName.c_str());
			m_strmFuncDef.Append("%s data)\n", pStrm->m_pType->m_typeName.c_str());

			m_strmFuncDef.Append("{\n");

			if (pStrm->m_strmCnt.size() > 0)
				m_strmFuncDef.Append("\tassert_msg(strmId < %d, \"Runtime check failed in CPers%s::WriteStream%s()"
				" - strmId out of range\");\n",
				strmCnt,
				mod.m_modName.Uc().c_str(), strmName.c_str());

			if (pStrm->m_reserve.AsInt() == 0)
				m_strmFuncDef.Append("\tassert(r_wrStrm%s_bOpenBufWr%s);\n", strmName.c_str(), strmIdStr.c_str());

			if (pStrm->m_bClose && pStrm->m_reserve.AsInt() > 0) {

				m_strmFuncDef.Append("\tc_wrStrm%s_strmWrEn%s = true;\n", strmName.c_str(), strmIdStr.c_str());
				m_strmFuncDef.Append("\tc_wrStrm%s_strmWrData%s.m_bData = true;\n", strmName.c_str(), strmIdStr.c_str());
				m_strmFuncDef.Append("\tc_wrStrm%s_strmWrData%s.m_data = data;\n", strmName.c_str(), strmIdStr.c_str());

			} else {

				if (pStrm->m_reserve.AsInt() > 0) {
					m_strmFuncDef.Append("\tc_wrStrm%s_strmWrEn%s = true;\n", strmName.c_str(), strmIdStr.c_str());
					m_strmFuncDef.Append("\tc_wrStrm%s_strmWrData%s = data;\n", strmName.c_str(), strmIdStr.c_str());
				} else {
					m_strmFuncDef.Append("\tc_wrStrm%s_bufWrEn%s = true;\n", strmName.c_str(), strmIdStr.c_str());
					m_strmFuncDef.Append("\tc_wrStrm%s_bufWrData%s = data;\n", strmName.c_str(), strmIdStr.c_str());

					if (!pStrm->m_bClose) {
						m_strmFuncDef.Append("\tc_wrStrm%s_bufWrRem%s = r_wrStrm%s_bufWrRem%s - 1;\n",
							strmName.c_str(), strmIdStr.c_str(), strmName.c_str(), strmIdStr.c_str());

						m_strmFuncDef.Append("\n");
						m_strmFuncDef.Append("\tif (r_wrStrm%s_bufWrRem%s == 1)\n", strmName.c_str(), strmIdStr.c_str());
						m_strmFuncDef.Append("\t\tc_wrStrm%s_bOpenBufWr%s = false;\n", strmName.c_str(), strmIdStr.c_str());
					}
				}
			}

			m_strmFuncDef.Append("}\n");
			m_strmFuncDef.Append("\n");
		}

		if (pStrm->m_bRead) {
			m_strmRegDecl.Append("\tstruct CRdStrm%s_buf {\n", strmName.c_str());
			m_strmRegDecl.Append("#\t\tifndef _HTV\n");
			m_strmRegDecl.Append("\t\tfriend void sc_trace(sc_trace_file *tf, const CRdStrm%s_buf & v, const std::string & NAME ) {\n", strmName.c_str());
			m_strmRegDecl.Append("\t\t\tsc_trace(tf, v.m_data, NAME + \".m_data\");\n");
			m_strmRegDecl.Append("\t\t\tsc_trace(tf, v.m_toggle, NAME + \".m_toggle\");\n");
			m_strmRegDecl.Append("\t\t}\n");
			m_strmRegDecl.Append("\t\tvoid operator = (int zero) {\n");
			m_strmRegDecl.Append("\t\t\tassert(zero == 0);\n");
			m_strmRegDecl.Append("\t\t\tm_data = 0;\n");
			m_strmRegDecl.Append("\t\t\tm_toggle = 0;\n");
			m_strmRegDecl.Append("\t\t}\n");
			m_strmRegDecl.Append("#\t\tendif\n");
			m_strmRegDecl.Append("\t\t%s m_data;\n", pStrm->m_pType->m_typeName.c_str());
			m_strmRegDecl.Append("\t\tht_uint1 m_toggle;\n");
			m_strmRegDecl.Append("\t};\n");
			m_strmRegDecl.Append("\n");

			m_strmBadDecl.Append("#ifndef _HTV\n");
			m_strmBadDecl.Append("inline CPers%s::CRdStrm%s_buf ht_bad_data(CPers%s::CRdStrm%s_buf const &)\n",
				mod.m_modName.Uc().c_str(), strmName.c_str(), mod.m_modName.Uc().c_str(), strmName.c_str());
			m_strmBadDecl.Append("{\n");
			m_strmBadDecl.Append("\tCPers%s::CRdStrm%s_buf x;\n", mod.m_modName.Uc().c_str(), strmName.c_str());
			m_strmBadDecl.Append("\tx.m_data = ht_bad_data(x.m_data);\n");
			m_strmBadDecl.Append("\tx.m_toggle = ht_bad_data(x.m_toggle);\n");
			m_strmBadDecl.Append("\treturn x;\n");
			m_strmBadDecl.Append("}\n");
			m_strmBadDecl.Append("#endif\n");
			m_strmBadDecl.Append("\n");

			m_strmRegDecl.Append("\tstruct CRdStrm%s_openRsp {\n", strmName.c_str());
			m_strmRegDecl.Append("\t\tht_uint%d m_rdPtr;\n", bufPtrW);
			if (!pStrm->m_bClose)
				m_strmRegDecl.Append("\t\tht_uint%d m_cnt;\n", elemCntW);
			if (pStrm->m_pTag != 0)
				m_strmRegDecl.Append("\t\t%s m_tag;\n", pStrm->m_pTag->m_typeName.c_str());
			m_strmRegDecl.Append("\t};\n");
			m_strmRegDecl.Append("\n");

			GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, "bool", VA("r_rdStrm%s_bOpenReq", strmName.c_str()), strmIdVec);
			m_strmRegDecl.Append("\tbool c_rdStrm%s_bOpenReq%s;\n", strmName.c_str(), strmIdDecl.c_str());
			if (pStrm->m_strmCnt.size() == 0) {
				strmPreInstr.Append("\tc_rdStrm%s_bOpenReq = r_rdStrm%s_bOpenReq;\n", strmName.c_str(), strmName.c_str());
				strmReg.Append("\tr_rdStrm%s_bOpenReq = !r_reset1x && c_rdStrm%s_bOpenReq;\n", strmName.c_str(), strmName.c_str());
			} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
				strmPreInstr.Append("\tc_rdStrm%s_bOpenReq[%d] = r_rdStrm%s_bOpenReq[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				strmReg.Append("\tr_rdStrm%s_bOpenReq[%d] = !r_reset1x && c_rdStrm%s_bOpenReq[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
			}

			GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, "bool", VA("r_rdStrm%s_bOpenBusy", strmName.c_str()), strmIdVec);
			m_strmRegDecl.Append("\tbool c_rdStrm%s_bOpenBusy%s;\n", strmName.c_str(), strmIdDecl.c_str());
			if (pStrm->m_strmCnt.size() == 0) {
				strmPreInstr.Append("\tc_rdStrm%s_bOpenBusy = false;\n", strmName.c_str());
				strmReg.Append("\tr_rdStrm%s_bOpenBusy = c_rdStrm%s_bOpenBusy;\n", strmName.c_str(), strmName.c_str());
			} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
				strmPreInstr.Append("\tc_rdStrm%s_bOpenBusy[%d] = false;\n", strmName.c_str(), i);
				strmReg.Append("\tr_rdStrm%s_bOpenBusy[%d] = c_rdStrm%s_bOpenBusy[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
			}

			m_strmRegDecl.Append("\tbool ht_noload c_rdStrm%s_bOpenAvail%s;\n", strmName.c_str(), strmIdDecl.c_str());
			if (pStrm->m_strmCnt.size() == 0) {
				strmPreInstr.Append("\tc_rdStrm%s_bOpenAvail = false;\n", strmName.c_str());
			} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
				strmPreInstr.Append("\tc_rdStrm%s_bOpenAvail[%d] = false;\n", strmName.c_str(), i);
			}

			GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, "ht_uint48", VA("r_rdStrm%s_addr", strmName.c_str()), strmIdVec);
			m_strmRegDecl.Append("\tht_uint48 c_rdStrm%s_addr%s;\n", strmName.c_str(), strmIdDecl.c_str());
			if (pStrm->m_strmCnt.size() == 0) {

				if (mod.m_clkRate == eClk1x)
					strmPreInstr.Append("\tc_rdStrm%s_addr = r_rdStrm%s_addr;\n", strmName.c_str(), strmName.c_str());
				else
					strmPreInstr.Append("\tc_rdStrm%s_addr = r_rdStrm%s_addr + (r_rdStrm%s_sumCy << 24);\n",
					strmName.c_str(), strmName.c_str(), strmName.c_str());

				strmReg.Append("\tr_rdStrm%s_addr = c_rdStrm%s_addr;\n", strmName.c_str(), strmName.c_str());
			} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {

				if (mod.m_clkRate == eClk1x)
					strmPreInstr.Append("\tc_rdStrm%s_addr[%d] = r_rdStrm%s_addr[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				else
					strmPreInstr.Append("\tc_rdStrm%s_addr[%d] = r_rdStrm%s_addr[%d] + (r_rdStrm%s_sumCy[%d] << 24);\n",
					strmName.c_str(), i, strmName.c_str(), i, strmName.c_str(), i);

				strmReg.Append("\tr_rdStrm%s_addr[%d] = c_rdStrm%s_addr[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
			}

			if (mod.m_clkRate == eClk2x) {
				GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, "ht_uint1", VA("r_rdStrm%s_sumCy", strmName.c_str()), strmIdVec);
				m_strmRegDecl.Append("\tht_uint1 c_rdStrm%s_sumCy%s;\n", strmName.c_str(), strmIdDecl.c_str());
				if (pStrm->m_strmCnt.size() == 0) {
					strmPreInstr.Append("\tc_rdStrm%s_sumCy = 0;\n", strmName.c_str());
					strmReg.Append("\tr_rdStrm%s_sumCy = c_rdStrm%s_sumCy;\n", strmName.c_str(), strmName.c_str());
				} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
					strmPreInstr.Append("\tc_rdStrm%s_sumCy[%d] = 0;\n", strmName.c_str(), i);
					strmReg.Append("\tr_rdStrm%s_sumCy[%d] = c_rdStrm%s_sumCy[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				}
			}

			GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, VA("ht_uint%d", elemCntW), VA("r_rdStrm%s_reqCnt", strmName.c_str()), strmIdVec);
			m_strmRegDecl.Append("\tht_uint%d c_rdStrm%s_reqCnt%s;\n", elemCntW, strmName.c_str(), strmIdDecl.c_str());
			if (pStrm->m_strmCnt.size() == 0) {
				strmPreInstr.Append("\tc_rdStrm%s_reqCnt = r_rdStrm%s_reqCnt;\n", strmName.c_str(), strmName.c_str());
				strmReg.Append("\tr_rdStrm%s_reqCnt = c_rdStrm%s_reqCnt;\n", strmName.c_str(), strmName.c_str());
			} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
				strmPreInstr.Append("\tc_rdStrm%s_reqCnt[%d] = r_rdStrm%s_reqCnt[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				strmReg.Append("\tr_rdStrm%s_reqCnt[%d] = c_rdStrm%s_reqCnt[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
			}

			GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, VA("ht_uint%d", bufPtrW), VA("r_rdStrm%s_bufInitPtr", strmName.c_str()));
			m_strmRegDecl.Append("\tht_uint%d c_rdStrm%s_bufInitPtr;\n", bufPtrW, strmName.c_str());
			strmPreInstr.Append("\tc_rdStrm%s_bufInitPtr = r_rdStrm%s_bufInitPtr;\n", strmName.c_str(), strmName.c_str());
			strmReg.Append("\tr_rdStrm%s_bufInitPtr = r_reset1x ? (ht_uint%d)0 : c_rdStrm%s_bufInitPtr;\n", strmName.c_str(), bufPtrW, strmName.c_str());

			GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, VA("ht_uint%d", bufPtrW), VA("r_rdStrm%s_reqWrPtr", strmName.c_str()), strmIdVec);
			m_strmRegDecl.Append("\tht_uint%d c_rdStrm%s_reqWrPtr%s;\n", bufPtrW, strmName.c_str(), strmIdDecl.c_str());
			if (pStrm->m_strmCnt.size() == 0) {
				strmPreInstr.Append("\tc_rdStrm%s_reqWrPtr = r_rdStrm%s_reqWrPtr;\n", strmName.c_str(), strmName.c_str());
				strmReg.Append("\tr_rdStrm%s_reqWrPtr = r_reset1x ? (ht_uint%d)0 : c_rdStrm%s_reqWrPtr;\n", strmName.c_str(), bufPtrW, strmName.c_str());
			} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
				strmPreInstr.Append("\tc_rdStrm%s_reqWrPtr[%d] = r_rdStrm%s_reqWrPtr[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				strmReg.Append("\tr_rdStrm%s_reqWrPtr[%d] = r_reset1x ? (ht_uint%d)0 : c_rdStrm%s_reqWrPtr[%d];\n", strmName.c_str(), i, bufPtrW, strmName.c_str(), i);
			}

			m_strmRegDecl.Append("\tht_uint2 c_rdStrm%s_rspRdPtrSel%s;\n", strmName.c_str(), strmIdDecl.c_str());
			if (pStrm->m_strmCnt.size() == 0) {
				strmPreInstr.Append("\tc_rdStrm%s_rspRdPtrSel = 0;\n", strmName.c_str());
			} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
				strmPreInstr.Append("\tc_rdStrm%s_rspRdPtrSel[%d] = 0;\n", strmName.c_str(), i);
			}

			GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, VA("ht_uint%d", bufPtrW), VA("r_rdStrm%s_rspRdPtr", strmName.c_str()), strmIdVec);
			m_strmRegDecl.Append("\tht_uint%d c_rdStrm%s_rspRdPtr%s;\n", bufPtrW, strmName.c_str(), strmIdDecl.c_str());
			if (pStrm->m_strmCnt.size() == 0) {
				strmPreInstr.Append("\tc_rdStrm%s_rspRdPtr = r_rdStrm%s_rspRdPtr;\n", strmName.c_str(), strmName.c_str());
				strmReg.Append("\tr_rdStrm%s_rspRdPtr = r_reset1x ? (ht_uint%d)0 : c_rdStrm%s_rspRdPtr;\n", strmName.c_str(), bufPtrW, strmName.c_str());
			} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
				strmPreInstr.Append("\tc_rdStrm%s_rspRdPtr[%d] = r_rdStrm%s_rspRdPtr[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				strmReg.Append("\tr_rdStrm%s_rspRdPtr[%d] = r_reset1x ? (ht_uint%d)0 : c_rdStrm%s_rspRdPtr[%d];\n", strmName.c_str(), i, bufPtrW, strmName.c_str(), i);
			}

			GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, VA("ht_uint%d", bufPtrW), VA("r_rdStrm%s_rspRdPtr2", strmName.c_str()), strmIdVec);
			m_strmRegDecl.Append("\tht_uint%d c_rdStrm%s_rspRdPtr2%s;\n", bufPtrW, strmName.c_str(), strmIdDecl.c_str());
			if (pStrm->m_strmCnt.size() == 0) {
				strmPreInstr.Append("\tc_rdStrm%s_rspRdPtr2 = r_rdStrm%s_rspRdPtr2;\n", strmName.c_str(), strmName.c_str());
				strmReg.Append("\tr_rdStrm%s_rspRdPtr2 = c_rdStrm%s_rspRdPtr2;\n", strmName.c_str(), strmName.c_str());
			} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
				strmPreInstr.Append("\tc_rdStrm%s_rspRdPtr2[%d] = r_rdStrm%s_rspRdPtr2[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				strmReg.Append("\tr_rdStrm%s_rspRdPtr2[%d] = c_rdStrm%s_rspRdPtr2[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
			}

			GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, "bool", VA("r_rdStrm%s_rspWrRdy", strmName.c_str()), strmIdVec);
			m_strmRegDecl.Append("\tbool c_rdStrm%s_rspWrRdy%s;\n", strmName.c_str(), strmIdDecl.c_str());
			if (pStrm->m_strmCnt.size() == 0) {
				strmPreInstr.Append("\tc_rdStrm%s_rspWrRdy = false;\n", strmName.c_str());
				strmReg.Append("\tr_rdStrm%s_rspWrRdy = c_rdStrm%s_rspWrRdy;\n", strmName.c_str(), strmName.c_str());
			} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
				strmPreInstr.Append("\tc_rdStrm%s_rspWrRdy[%d] = false;\n", strmName.c_str(), i);
				strmReg.Append("\tr_rdStrm%s_rspWrRdy[%d] = c_rdStrm%s_rspWrRdy[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
			}

			bool bMultiQwReq = bIsWx || bIsHc2 && pStrm->m_memSrc == "host";
			if (mod.m_clkRate == eClk2x && bMultiQwReq) {
				GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, VA("ht_uint%d", bufPtrW), VA("r_rdStrm%s_rspWrPtr1", strmName.c_str()), strmIdVec);
				m_strmRegDecl.Append("\tht_uint%d c_rdStrm%s_rspWrPtr1%s;\n", bufPtrW, strmName.c_str(), strmIdDecl.c_str());
				if (pStrm->m_strmCnt.size() == 0) {
					strmReg.Append("\tr_rdStrm%s_rspWrPtr1 = c_rdStrm%s_rspWrPtr1;\n", strmName.c_str(), strmName.c_str());
				} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
					strmReg.Append("\tr_rdStrm%s_rspWrPtr1[%d] = c_rdStrm%s_rspWrPtr1[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				}
			}

			GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, VA("ht_uint%d", bufPtrW), VA("r_rdStrm%s_rspWrPtr", strmName.c_str()), strmIdVec);
			m_strmRegDecl.Append("\tht_uint%d c_rdStrm%s_rspWrPtr%s;\n", bufPtrW, strmName.c_str(), strmIdDecl.c_str());
			if (pStrm->m_strmCnt.size() == 0) {
				strmPreInstr.Append("\tc_rdStrm%s_rspWrPtr = r_rdStrm%s_rspWrPtr;\n", strmName.c_str(), strmName.c_str());
				strmReg.Append("\tr_rdStrm%s_rspWrPtr = c_rdStrm%s_rspWrPtr;\n", strmName.c_str(), strmName.c_str());
			} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
				strmPreInstr.Append("\tc_rdStrm%s_rspWrPtr[%d] = r_rdStrm%s_rspWrPtr[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				strmReg.Append("\tr_rdStrm%s_rspWrPtr[%d] = c_rdStrm%s_rspWrPtr[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
			}

			if (pStrm->m_bClose) {
				GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, VA("ht_uint%d", bufPtrW), VA("r_rdStrm%s_closingWrPtr", strmName.c_str()), strmIdVec);
				m_strmRegDecl.Append("\tht_uint%d c_rdStrm%s_closingWrPtr%s;\n", bufPtrW, strmName.c_str(), strmIdDecl.c_str());
				if (pStrm->m_strmCnt.size() == 0) {
					strmPreInstr.Append("\tc_rdStrm%s_closingWrPtr = r_rdStrm%s_closingWrPtr;\n", strmName.c_str(), strmName.c_str());
					strmReg.Append("\tr_rdStrm%s_closingWrPtr = c_rdStrm%s_closingWrPtr;\n", strmName.c_str(), strmName.c_str());
				} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
					strmPreInstr.Append("\tc_rdStrm%s_closingWrPtr[%d] = r_rdStrm%s_closingWrPtr[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
					strmReg.Append("\tr_rdStrm%s_closingWrPtr[%d] = c_rdStrm%s_closingWrPtr[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				}

				GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, "bool", VA("r_rdStrm%s_bOpenRsp", strmName.c_str()), strmIdVec);
				m_strmRegDecl.Append("\tbool c_rdStrm%s_bOpenRsp%s;\n", strmName.c_str(), strmIdDecl.c_str());
				if (pStrm->m_strmCnt.size() == 0) {
					strmPreInstr.Append("\tc_rdStrm%s_bOpenRsp = r_rdStrm%s_bOpenRsp;\n", strmName.c_str(), strmName.c_str());
					strmReg.Append("\tr_rdStrm%s_bOpenRsp = !r_reset1x && c_rdStrm%s_bOpenRsp;\n", strmName.c_str(), strmName.c_str());
				} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
					strmPreInstr.Append("\tc_rdStrm%s_bOpenRsp[%d] = r_rdStrm%s_bOpenRsp[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
					strmReg.Append("\tr_rdStrm%s_bOpenRsp[%d] = !r_reset1x && c_rdStrm%s_bOpenRsp[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				}

				GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, "bool", VA("r_rdStrm%s_bClosingRsp", strmName.c_str()), strmIdVec);
				m_strmRegDecl.Append("\tbool c_rdStrm%s_bClosingRsp%s;\n", strmName.c_str(), strmIdDecl.c_str());
				if (pStrm->m_strmCnt.size() == 0) {
					strmPreInstr.Append("\tc_rdStrm%s_bClosingRsp = r_rdStrm%s_bClosingRsp;\n", strmName.c_str(), strmName.c_str());
					strmReg.Append("\tr_rdStrm%s_bClosingRsp = !r_reset1x && c_rdStrm%s_bClosingRsp;\n", strmName.c_str(), strmName.c_str());
				} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
					strmPreInstr.Append("\tc_rdStrm%s_bClosingRsp[%d] = r_rdStrm%s_bClosingRsp[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
					strmReg.Append("\tr_rdStrm%s_bClosingRsp[%d] = !r_reset1x && c_rdStrm%s_bClosingRsp[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				}

				GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, "bool", VA("r_rdStrm%s_bClose", strmName.c_str()), strmIdVec);
				m_strmRegDecl.Append("\tbool c_rdStrm%s_bClose%s;\n", strmName.c_str(), strmIdDecl.c_str());
				if (pStrm->m_strmCnt.size() == 0) {
					strmPreInstr.Append("\tc_rdStrm%s_bClose = false;\n", strmName.c_str());
					strmReg.Append("\tr_rdStrm%s_bClose = c_rdStrm%s_bClose;\n", strmName.c_str(), strmName.c_str());
				} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
					strmPreInstr.Append("\tc_rdStrm%s_bClose[%d] = false;\n", strmName.c_str(), i);
					strmReg.Append("\tr_rdStrm%s_bClose[%d] = c_rdStrm%s_bClose[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				}

				if (mod.m_clkRate == eClk2x) {
					GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, "bool", VA("r_rdStrm%s_bClose2", strmName.c_str()), strmIdVec);
					m_strmRegDecl.Append("\tbool c_rdStrm%s_bClose2%s;\n", strmName.c_str(), strmIdDecl.c_str());
					if (pStrm->m_strmCnt.size() == 0) {
						strmPreInstr.Append("\tc_rdStrm%s_bClose2 = r_rdStrm%s_bClose;\n", strmName.c_str(), strmName.c_str());
						strmReg.Append("\tr_rdStrm%s_bClose2 = c_rdStrm%s_bClose2;\n", strmName.c_str(), strmName.c_str());
					} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
						strmPreInstr.Append("\tc_rdStrm%s_bClose2[%d] = r_rdStrm%s_bClose[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
						strmReg.Append("\tr_rdStrm%s_bClose2[%d] = c_rdStrm%s_bClose2[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
					}
				}
			}

			if (pStrm->m_pTag != 0) {
				GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, pStrm->m_pTag->m_typeName, VA("r_rdStrm%s_tag", strmName.c_str()), strmIdVec);
				m_strmRegDecl.Append("\t%s c_rdStrm%s_tag%s;\n", pStrm->m_pTag->m_typeName.c_str(), strmName.c_str(), strmIdDecl.c_str());
				if (pStrm->m_strmCnt.size() == 0) {
					strmPreInstr.Append("\tc_rdStrm%s_tag = r_rdStrm%s_tag;\n", strmName.c_str(), strmName.c_str());
					strmReg.Append("\tr_rdStrm%s_tag = c_rdStrm%s_tag;\n", strmName.c_str(), strmName.c_str());
				} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
					strmPreInstr.Append("\tc_rdStrm%s_tag[%d] = r_rdStrm%s_tag[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
					strmReg.Append("\tr_rdStrm%s_tag[%d] = c_rdStrm%s_tag[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				}

				GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, pStrm->m_pTag->m_typeName, VA("r_rdStrm%s_rspTag", strmName.c_str()), strmIdVec);
				m_strmRegDecl.Append("\t%s c_rdStrm%s_rspTag%s;\n", pStrm->m_pTag->m_typeName.c_str(), strmName.c_str(), strmIdDecl.c_str());
				if (pStrm->m_strmCnt.size() == 0) {
					strmPreInstr.Append("\tc_rdStrm%s_rspTag = r_rdStrm%s_rspTag;\n", strmName.c_str(), strmName.c_str());
					strmReg.Append("\tr_rdStrm%s_rspTag = c_rdStrm%s_rspTag;\n", strmName.c_str(), strmName.c_str());
				} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
					strmPreInstr.Append("\tc_rdStrm%s_rspTag[%d] = r_rdStrm%s_rspTag[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
					strmReg.Append("\tr_rdStrm%s_rspTag[%d] = c_rdStrm%s_rspTag[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				}
			}

			if (!pStrm->m_bClose) {
				GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, VA("ht_uint%d", elemCntW), VA("r_rdStrm%s_rspCnt", strmName.c_str()), strmIdVec);
				m_strmRegDecl.Append("\tht_uint%d c_rdStrm%s_rspCnt%s;\n", elemCntW, strmName.c_str(), strmIdDecl.c_str());
				if (pStrm->m_strmCnt.size() == 0) {
					strmPreInstr.Append("\tc_rdStrm%s_rspCnt = r_rdStrm%s_rspCnt;\n", strmName.c_str(), strmName.c_str());
					strmReg.Append("\tr_rdStrm%s_rspCnt = r_reset1x ? (ht_uint%d)0 : c_rdStrm%s_rspCnt;\n", strmName.c_str(), elemCntW, strmName.c_str());
				} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
					strmPreInstr.Append("\tc_rdStrm%s_rspCnt[%d] = r_rdStrm%s_rspCnt[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
					strmReg.Append("\tr_rdStrm%s_rspCnt[%d] = r_reset1x ? (ht_uint%d)0 : c_rdStrm%s_rspCnt[%d];\n", strmName.c_str(), i, elemCntW, strmName.c_str(), i);
				}

				m_strmRegDecl.Append("\tbool c_rdStrm%s_bRspLast%s;\n", strmName.c_str(), strmIdDecl.c_str());

				GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, "bool", VA("r_rdStrm%s_bOpenRsp", strmName.c_str()), strmIdVec);
				m_strmRegDecl.Append("\tbool c_rdStrm%s_bOpenRsp%s;\n", strmName.c_str(), strmIdDecl.c_str());
				if (pStrm->m_strmCnt.size() == 0) {
					strmPreInstr.Append("\tc_rdStrm%s_bOpenRsp = r_rdStrm%s_bOpenRsp;\n", strmName.c_str(), strmName.c_str());
					strmReg.Append("\tr_rdStrm%s_bOpenRsp = !r_reset1x && c_rdStrm%s_bOpenRsp;\n", strmName.c_str(), strmName.c_str());
				} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
					strmPreInstr.Append("\tc_rdStrm%s_bOpenRsp[%d] = r_rdStrm%s_bOpenRsp[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
					strmReg.Append("\tr_rdStrm%s_bOpenRsp[%d] = !r_reset1x && c_rdStrm%s_bOpenRsp[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				}
			}

			GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, "ht_uint2", VA("r_rdStrm%s_rspState", strmName.c_str()), strmIdVec);
			m_strmRegDecl.Append("\tht_uint2 c_rdStrm%s_rspState%s;\n", strmName.c_str(), strmIdDecl.c_str());
			if (pStrm->m_strmCnt.size() == 0) {
				strmPreInstr.Append("\tc_rdStrm%s_rspState = r_rdStrm%s_rspState;\n", strmName.c_str(), strmName.c_str());
				strmReg.Append("\tr_rdStrm%s_rspState = r_reset1x ? (ht_uint2)0 : c_rdStrm%s_rspState;\n", strmName.c_str(), strmName.c_str());
			} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
				strmPreInstr.Append("\tc_rdStrm%s_rspState[%d] = r_rdStrm%s_rspState[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				strmReg.Append("\tr_rdStrm%s_rspState[%d] = r_reset1x ? (ht_uint2)0 : c_rdStrm%s_rspState[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
			}

			GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, "bool", VA("r_rdStrm%s_bPreValid", strmName.c_str()), strmIdVec);
			m_strmRegDecl.Append("\tbool c_rdStrm%s_bPreValid%s;\n", strmName.c_str(), strmIdDecl.c_str());
			if (pStrm->m_strmCnt.size() == 0) {
				strmPreInstr.Append("\tc_rdStrm%s_bPreValid = r_rdStrm%s_bPreValid;\n", strmName.c_str(), strmName.c_str());
				strmReg.Append("\tr_rdStrm%s_bPreValid = !r_reset1x && c_rdStrm%s_bPreValid;\n", strmName.c_str(), strmName.c_str());
			} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
				strmPreInstr.Append("\tc_rdStrm%s_bPreValid[%d] = r_rdStrm%s_bPreValid[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				strmReg.Append("\tr_rdStrm%s_bPreValid[%d] = !r_reset1x && c_rdStrm%s_bPreValid[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
			}

			GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, pStrm->m_pType->m_typeName, VA("r_rdStrm%s_preData", strmName.c_str()), strmIdVec);
			m_strmRegDecl.Append("\t%s c_rdStrm%s_preData%s;\n", pStrm->m_pType->m_typeName.c_str(), strmName.c_str(), strmIdDecl.c_str());
			if (pStrm->m_strmCnt.size() == 0) {
				strmPreInstr.Append("\tc_rdStrm%s_preData = r_rdStrm%s_preData;\n", strmName.c_str(), strmName.c_str());
				strmReg.Append("\tr_rdStrm%s_preData = c_rdStrm%s_preData;\n", strmName.c_str(), strmName.c_str());
			} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
				strmPreInstr.Append("\tc_rdStrm%s_preData[%d] = r_rdStrm%s_preData[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				strmReg.Append("\tr_rdStrm%s_preData[%d] = c_rdStrm%s_preData[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
			}

			if (!pStrm->m_bClose) {
				GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, "bool", VA("r_rdStrm%s_bPreLast", strmName.c_str()), strmIdVec);
				m_strmRegDecl.Append("\tbool c_rdStrm%s_bPreLast%s;\n", strmName.c_str(), strmIdDecl.c_str());
				if (pStrm->m_strmCnt.size() == 0) {
					strmPreInstr.Append("\tc_rdStrm%s_bPreLast = r_rdStrm%s_bPreLast;\n", strmName.c_str(), strmName.c_str());
					strmReg.Append("\tr_rdStrm%s_bPreLast = c_rdStrm%s_bPreLast;\n", strmName.c_str(), strmName.c_str());
				} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
					strmPreInstr.Append("\tc_rdStrm%s_bPreLast[%d] = r_rdStrm%s_bPreLast[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
					strmReg.Append("\tr_rdStrm%s_bPreLast[%d] = c_rdStrm%s_bPreLast[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				}
			}

			m_strmRegDecl.Append("\tbool c_rdStrm%s_bRspValid%s;\n", strmName.c_str(), strmIdDecl.c_str());

			GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, "bool", VA("r_rdStrm%s_bValid", strmName.c_str()), strmIdVec);
			m_strmRegDecl.Append("\tbool c_rdStrm%s_bValid%s;\n", strmName.c_str(), strmIdDecl.c_str());
			if (pStrm->m_strmCnt.size() == 0) {
				strmPreInstr.Append("\tc_rdStrm%s_bValid = r_rdStrm%s_bValid;\n", strmName.c_str(), strmName.c_str());
				strmReg.Append("\tr_rdStrm%s_bValid = !r_reset1x && c_rdStrm%s_bValid;\n", strmName.c_str(), strmName.c_str());
			} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
				strmPreInstr.Append("\tc_rdStrm%s_bValid[%d] = r_rdStrm%s_bValid[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				strmReg.Append("\tr_rdStrm%s_bValid[%d] = !r_reset1x && c_rdStrm%s_bValid[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
			}

			GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, pStrm->m_pType->m_typeName, VA("r_rdStrm%s_data", strmName.c_str()), strmIdVec);
			m_strmRegDecl.Append("\t%s c_rdStrm%s_data%s;\n", pStrm->m_pType->m_typeName.c_str(), strmName.c_str(), strmIdDecl.c_str());
			if (pStrm->m_strmCnt.size() == 0) {
				strmPreInstr.Append("\tc_rdStrm%s_data = r_rdStrm%s_data;\n", strmName.c_str(), strmName.c_str());
				strmReg.Append("\tr_rdStrm%s_data = c_rdStrm%s_data;\n", strmName.c_str(), strmName.c_str());
			} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
				strmPreInstr.Append("\tc_rdStrm%s_data[%d] = r_rdStrm%s_data[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				strmReg.Append("\tr_rdStrm%s_data[%d] = c_rdStrm%s_data[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
			}

			if (!pStrm->m_bClose) {
				GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, "bool", VA("r_rdStrm%s_bLast", strmName.c_str()), strmIdVec);
				m_strmRegDecl.Append("\tbool c_rdStrm%s_bLast%s;\n", strmName.c_str(), strmIdDecl.c_str());
				if (pStrm->m_strmCnt.size() == 0) {
					strmPreInstr.Append("\tc_rdStrm%s_bLast = r_rdStrm%s_bLast;\n", strmName.c_str(), strmName.c_str());
					strmReg.Append("\tr_rdStrm%s_bLast = c_rdStrm%s_bLast;\n", strmName.c_str(), strmName.c_str());
				} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
					strmPreInstr.Append("\tc_rdStrm%s_bLast[%d] = r_rdStrm%s_bLast[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
					strmReg.Append("\tr_rdStrm%s_bLast[%d] = c_rdStrm%s_bLast[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				}
			}

			GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, "bool", VA("r_rdStrm%s_bNextRsp", strmName.c_str()), strmIdVec);
			m_strmRegDecl.Append("\tbool c_rdStrm%s_bNextRsp%s;\n", strmName.c_str(), strmIdDecl.c_str());
			if (pStrm->m_strmCnt.size() == 0) {
				strmPreInstr.Append("\tc_rdStrm%s_bNextRsp = r_rdStrm%s_bNextRsp;\n", strmName.c_str(), strmName.c_str());
				strmReg.Append("\tr_rdStrm%s_bNextRsp = !r_reset1x && c_rdStrm%s_bNextRsp;\n", strmName.c_str(), strmName.c_str());
			} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
				strmPreInstr.Append("\tc_rdStrm%s_bNextRsp[%d] = r_rdStrm%s_bNextRsp[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				strmReg.Append("\tr_rdStrm%s_bNextRsp[%d] = !r_reset1x && c_rdStrm%s_bNextRsp[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
			}

			GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, VA("ht_uint%d", bufPtrW), VA("r_rdStrm%s_nextRspRdPtr", strmName.c_str()), strmIdVec);
			m_strmRegDecl.Append("\tht_uint%d c_rdStrm%s_nextRspRdPtr%s;\n", bufPtrW, strmName.c_str(), strmIdDecl.c_str());
			if (pStrm->m_strmCnt.size() == 0) {
				strmPreInstr.Append("\tc_rdStrm%s_nextRspRdPtr = r_rdStrm%s_nextRspRdPtr;\n", strmName.c_str(), strmName.c_str());
				strmReg.Append("\tr_rdStrm%s_nextRspRdPtr = c_rdStrm%s_nextRspRdPtr;\n", strmName.c_str(), strmName.c_str());
			} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
				strmPreInstr.Append("\tc_rdStrm%s_nextRspRdPtr[%d] = r_rdStrm%s_nextRspRdPtr[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				strmReg.Append("\tr_rdStrm%s_nextRspRdPtr[%d] = c_rdStrm%s_nextRspRdPtr[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
			}

			if (pStrm->m_pTag != 0) {
				GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, pStrm->m_pTag->m_typeName, VA("r_rdStrm%s_openTag", strmName.c_str()), strmIdVec);
				m_strmRegDecl.Append("\t%s c_rdStrm%s_openTag%s;\n", pStrm->m_pTag->m_typeName.c_str(), strmName.c_str(), strmIdDecl.c_str());
				if (pStrm->m_strmCnt.size() == 0) {
					strmPreInstr.Append("\tc_rdStrm%s_openTag = r_rdStrm%s_openTag;\n", strmName.c_str(), strmName.c_str());
					strmReg.Append("\tr_rdStrm%s_openTag = c_rdStrm%s_openTag;\n", strmName.c_str(), strmName.c_str());
				} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
					strmPreInstr.Append("\tc_rdStrm%s_openTag[%d] = r_rdStrm%s_openTag[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
					strmReg.Append("\tr_rdStrm%s_openTag[%d] = c_rdStrm%s_openTag[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				}
			}

			if (!pStrm->m_bClose) {
				GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, VA("ht_uint%d", elemCntW), VA("r_rdStrm%s_openRspCnt", strmName.c_str()), strmIdVec);
				m_strmRegDecl.Append("\tht_uint%d c_rdStrm%s_openRspCnt%s;\n", elemCntW, strmName.c_str(), strmIdDecl.c_str());
				if (pStrm->m_strmCnt.size() == 0) {
					strmPreInstr.Append("\tc_rdStrm%s_openRspCnt = r_rdStrm%s_openRspCnt;\n", strmName.c_str(), strmName.c_str());
					strmReg.Append("\tr_rdStrm%s_openRspCnt = c_rdStrm%s_openRspCnt;\n", strmName.c_str(), strmName.c_str());
				} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
					strmPreInstr.Append("\tc_rdStrm%s_openRspCnt[%d] = r_rdStrm%s_openRspCnt[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
					strmReg.Append("\tr_rdStrm%s_openRspCnt[%d] = c_rdStrm%s_openRspCnt[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				}
			}

			GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, "bool", VA("r_rdStrm%s_bCollision", strmName.c_str()), strmIdVec);
			m_strmRegDecl.Append("\tbool c_rdStrm%s_bCollision%s;\n", strmName.c_str(), strmIdDecl.c_str());
			if (pStrm->m_strmCnt.size() == 0) {
				strmPreInstr.Append("\tc_rdStrm%s_bCollision = r_rdStrm%s_bCollision;\n", strmName.c_str(), strmName.c_str());
				strmReg.Append("\tr_rdStrm%s_bCollision = !r_reset1x && c_rdStrm%s_bCollision;\n", strmName.c_str(), strmName.c_str());
			} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
				strmPreInstr.Append("\tc_rdStrm%s_bCollision[%d] = r_rdStrm%s_bCollision[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				strmReg.Append("\tr_rdStrm%s_bCollision[%d] = !r_reset1x && c_rdStrm%s_bCollision[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
			}

			m_strmRegDecl.Append("\tht_uint%d c_rdStrm%s_bufElemCnt%s;\n", bufPtrW, strmName.c_str(), strmIdDecl.c_str());
			m_strmRegDecl.Append("\tbool c_rdStrm%s_bRspBufFull%s;\n", strmName.c_str(), strmIdDecl.c_str());
			m_strmRegDecl.Append("\tbool c_rdStrm%s_bReqRdy%s;\n", strmName.c_str(), strmIdDecl.c_str());

			if (mod.m_clkRate == eClk2x) {
				GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, "bool", VA("r_t2_rdStrm%s_bReqSel", strmName.c_str()), strmIdVec);
				m_strmRegDecl.Append("\tbool c_t1_rdStrm%s_bReqSel%s;\n", strmName.c_str(), strmIdDecl.c_str());
				if (pStrm->m_strmCnt.size() == 0) {
					strmReg.Append("\tr_t2_rdStrm%s_bReqSel = c_t1_rdStrm%s_bReqSel;\n", strmName.c_str(), strmName.c_str());
				} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
					strmReg.Append("\tr_t2_rdStrm%s_bReqSel[%d] = c_t1_rdStrm%s_bReqSel[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				}
			} else
				m_strmRegDecl.Append("\tbool c_t1_rdStrm%s_bReqSel%s;\n", strmName.c_str(), strmIdDecl.c_str());

			if (!pStrm->m_bClose) {
				if (mod.m_clkRate == eClk2x) {
					GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, "bool", VA("r_rdStrm%s_bReqCntZ", strmName.c_str()), strmIdVec);
					m_strmRegDecl.Append("\tbool c_rdStrm%s_bReqCntZ%s;\n", strmName.c_str(), strmIdDecl.c_str());
					if (pStrm->m_strmCnt.size() == 0) {
						strmReg.Append("\tr_rdStrm%s_bReqCntZ = c_rdStrm%s_bReqCntZ;\n", strmName.c_str(), strmName.c_str());
					} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
						strmReg.Append("\tr_rdStrm%s_bReqCntZ[%d] = c_rdStrm%s_bReqCntZ[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
					}
				} else
					m_strmRegDecl.Append("\tbool c_rdStrm%s_bReqCntZ%s;\n", strmName.c_str(), strmIdDecl.c_str());
			}

			if (mod.m_clkRate == eClk2x) {
				GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, "ht_uint4", VA("r_rdStrm%s_reqQwCnt", strmName.c_str()), strmIdVec);
				m_strmRegDecl.Append("\tht_uint4 c_rdStrm%s_reqQwCnt%s;\n", strmName.c_str(), strmIdDecl.c_str());
				if (pStrm->m_strmCnt.size() == 0) {
					strmReg.Append("\tr_rdStrm%s_reqQwCnt = c_rdStrm%s_reqQwCnt;\n", strmName.c_str(), strmName.c_str());
				} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
					strmReg.Append("\tr_rdStrm%s_reqQwCnt[%d] = c_rdStrm%s_reqQwCnt[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				}
			} else
				m_strmRegDecl.Append("\tht_uint4 c_rdStrm%s_reqQwCnt%s;\n", strmName.c_str(), strmIdDecl.c_str());

			if (mod.m_clkRate == eClk2x) {
				GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, VA("ht_uint%d", 4 + (3 - pStrm->m_elemByteW)), VA("r_rdStrm%s_reqElemCnt", strmName.c_str()), strmIdVec);
				m_strmRegDecl.Append("\tht_uint%d c_rdStrm%s_reqElemCnt%s;\n", 4 + (3 - pStrm->m_elemByteW), strmName.c_str(), strmIdDecl.c_str());
				if (pStrm->m_strmCnt.size() == 0) {
					strmReg.Append("\tr_rdStrm%s_reqElemCnt = c_rdStrm%s_reqElemCnt;\n", strmName.c_str(), strmName.c_str());
				} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
					strmReg.Append("\tr_rdStrm%s_reqElemCnt[%d] = c_rdStrm%s_reqElemCnt[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				}
			} else
				m_strmRegDecl.Append("\tht_uint%d c_rdStrm%s_reqElemCnt%s;\n", 4 + (3 - pStrm->m_elemByteW), strmName.c_str(), strmIdDecl.c_str());

			GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, VA("CRdStrm%s_buf", strmName.c_str()), VA("r_rdStrm%s_buf", strmName.c_str()), strmIdVec);
			m_strmRegDecl.Append("\tCRdStrm%s_buf c_rdStrm%s_buf%s;\n", strmName.c_str(), strmName.c_str(), strmIdDecl.c_str());
			if (pStrm->m_strmCnt.size() == 0) {
				strmPreInstr.Append("\tc_rdStrm%s_buf = r_rdStrm%s_buf;\n", strmName.c_str(), strmName.c_str());
				strmReg.Append("\tr_rdStrm%s_buf = c_rdStrm%s_buf;\n", strmName.c_str(), strmName.c_str());
			} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
				strmPreInstr.Append("\tc_rdStrm%s_buf[%d] = r_rdStrm%s_buf[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				strmReg.Append("\tr_rdStrm%s_buf[%d] = c_rdStrm%s_buf[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
			}

			if (pStrm->m_elemByteW == 3)
				m_strmRegDecl.Append("\tht_block_ram<CRdStrm%s_buf, 9> m_rdStrm%s_buf%s;\n", strmName.c_str(), strmName.c_str(), strmIdDecl.c_str());
			else
				m_strmRegDecl.Append("\tht_mwr_block_ram<CRdStrm%s_buf, %d, %d> m_rdStrm%s_buf%s;\n",
				strmName.c_str(), 3 - pStrm->m_elemByteW, pStrm->m_bufPtrW - (3 - pStrm->m_elemByteW) - 1, strmName.c_str(), strmIdDecl.c_str());

			for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
				string strmIdx = pStrm->m_strmCnt.size() == 0 ? "" : VA("[%d]", i);

				strmReg.Append("\tm_rdStrm%s_buf%s.clock();\n", strmName.c_str(), strmIdx.c_str());
			}

			m_strmRegDecl.Append("\tht_dist_que<CRdStrm%s_openRsp, 5> m_rdStrm%s_nextRspQue%s;\n", strmName.c_str(), strmName.c_str(), strmIdDecl.c_str());

			for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
				string strmIdx = pStrm->m_strmCnt.size() == 0 ? "" : VA("[%d]", i);

				strmReg.Append("\tm_rdStrm%s_nextRspQue%s.clock(%s);\n", strmName.c_str(), strmIdx.c_str(), reset.c_str());
			}

			//int strmWIdx = 0;
			strmPostInstr.Append("\t// rdStrm%s\n", strmName.c_str());
			string strmIdxStr;
			if (pStrm->m_strmCnt.size() == 0)
				strmPostInstr.Append("\t{\n");
			else {
				strmPostInstr.Append("\tfor (int idx = 0; idx < %d; idx += 1) {\n", pStrm->m_strmCnt.AsInt());
				strmIdxStr = "[idx]";
			}

			strmPostInstr.Append("\t\tif (!r_rdStrm%s_bNextRsp%s && !m_rdStrm%s_nextRspQue%s.empty()) {\n",
				strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
			strmPostInstr.Append("\t\t\tc_rdStrm%s_bNextRsp%s = true;\n", strmName.c_str(), strmIdxStr.c_str());
			strmPostInstr.Append("\t\t\tc_rdStrm%s_nextRspRdPtr%s = m_rdStrm%s_nextRspQue%s.front().m_rdPtr;\n",
				strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
			if (pStrm->m_pTag != 0)
				strmPostInstr.Append("\t\t\tc_rdStrm%s_openTag%s = m_rdStrm%s_nextRspQue%s.front().m_tag;\n",
				strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
			if (!pStrm->m_bClose)
				strmPostInstr.Append("\t\t\tc_rdStrm%s_openRspCnt%s = m_rdStrm%s_nextRspQue%s.front().m_cnt;\n",
				strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
			strmPostInstr.Append("\t\t\tm_rdStrm%s_nextRspQue%s.pop();\n", strmName.c_str(), strmIdxStr.c_str());
			strmPostInstr.Append("\t\t}\n");
			strmPostInstr.Append("\n");

			if (pStrm->m_bClose) {
				strmPostInstr.Append("\t\tif (r_rdStrm%s_bClose%s%s)\n", strmName.c_str(), mod.m_clkRate == eClk1x ? "" : "2", strmIdxStr.c_str());
				strmPostInstr.Append("\t\t\tc_rdStrm%s_closingWrPtr%s = r_rdStrm%s_reqWrPtr%s;\n",
					strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
				strmPostInstr.Append("\n");
			}

			strmPostInstr.Append("\t\tif (r_rdStrm%s_rspState%s(0,0) == 1 || !r_rdStrm%s_bPreValid%s || !r_rdStrm%s_bValid%s) {\n",
				strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
			strmPostInstr.Append("\t\t\tc_rdStrm%s_buf%s = m_rdStrm%s_buf%s.read_mem();\n",
				strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
			strmPostInstr.Append("\t\t\tc_rdStrm%s_bCollision%s = r_rdStrm%s_rspWrRdy%s && r_rdStrm%s_rspRdPtr%s(%d,%d) == r_rdStrm%s_rspWrPtr%s(%d,%d);\n",
				strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(),
				strmName.c_str(), strmIdxStr.c_str(), bufPtrW - 2, 3 - pStrm->m_elemByteW, strmName.c_str(), strmIdxStr.c_str(), bufPtrW - 2, 3 - pStrm->m_elemByteW);
			strmPostInstr.Append("\t\t\tc_rdStrm%s_rspRdPtr2%s = r_rdStrm%s_rspRdPtr%s;\n",
				strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
			strmPostInstr.Append("\t\t}\n");
			strmPostInstr.Append("\n");

			strmPostInstr.Append("\t\tc_rdStrm%s_bRspValid%s = false;\n", strmName.c_str(), strmIdxStr.c_str());
			if (!pStrm->m_bClose)
				strmPostInstr.Append("\t\tc_rdStrm%s_bRspLast%s = false;\n", strmName.c_str(), strmIdxStr.c_str());
			strmPostInstr.Append("\t\tswitch (r_rdStrm%s_rspState%s) {\n", strmName.c_str(), strmIdxStr.c_str());

			strmPostInstr.Append("\t\tcase 0:\n");
			strmPostInstr.Append("\t\t\tc_rdStrm%s_rspRdPtrSel%s = 0;\n", strmName.c_str(), strmIdxStr.c_str());
			strmPostInstr.Append("\t\t\tc_rdStrm%s_rspState%s = r_rdStrm%s_bNextRsp%s ? 1 : 0;\n",
				strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());

			strmPostInstr.Append("\t\t\tif (r_rdStrm%s_bNextRsp%s) {\n",
				strmName.c_str(), strmIdxStr.c_str());
			strmPostInstr.Append("\t\t\t\tc_rdStrm%s_bNextRsp%s = false;\n",
				strmName.c_str(), strmIdxStr.c_str());

			if (pStrm->m_pTag != 0)
				strmPostInstr.Append("\t\t\t\tc_rdStrm%s_rspTag%s = r_rdStrm%s_openTag%s;\n",
				strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());

			if (pStrm->m_bClose)
				strmPostInstr.Append("\t\t\t\tc_rdStrm%s_bOpenRsp%s = true;\n", strmName.c_str(), strmIdxStr.c_str());
			else {
				strmPostInstr.Append("\t\t\t\tc_rdStrm%s_rspCnt%s = r_rdStrm%s_openRspCnt%s;\n",
					strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
				strmPostInstr.Append("\t\t\t\tc_rdStrm%s_bOpenRsp%s = r_rdStrm%s_openRspCnt%s != 0;\n",
					strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
			}

			strmPostInstr.Append("\t\t\t}\n");
			strmPostInstr.Append("\t\t\tbreak;\n");

			strmPostInstr.Append("\t\tcase 1:\n");
			strmPostInstr.Append("\t\t\tc_rdStrm%s_rspRdPtrSel%s = 3;\n", strmName.c_str(), strmIdxStr.c_str());
			strmPostInstr.Append("\t\t\tc_rdStrm%s_rspState%s = 2;\n", strmName.c_str(), strmIdxStr.c_str());
			strmPostInstr.Append("\t\t\tbreak;\n");

			strmPostInstr.Append("\t\tcase 2:\n");
			strmPostInstr.Append("\t\t\tif (!r_rdStrm%s_bOpenRsp%s) {\n", strmName.c_str(), strmIdxStr.c_str());
			strmPostInstr.Append("\t\t\t\tc_rdStrm%s_rspRdPtrSel%s = 0;\n", strmName.c_str(), strmIdxStr.c_str());
			strmPostInstr.Append("\t\t\t\tc_rdStrm%s_rspState%s = r_rdStrm%s_bNextRsp%s ? 1 : 0;\n",
				strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());

			strmPostInstr.Append("\t\t\t\tif (r_rdStrm%s_bNextRsp%s && !c_rdStrm%s_bValid%s) {\n",
				strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
			strmPostInstr.Append("\t\t\t\t\tc_rdStrm%s_bNextRsp%s = false;\n",
				strmName.c_str(), strmIdxStr.c_str());

			if (pStrm->m_pTag != 0)
				strmPostInstr.Append("\t\t\t\t\tc_rdStrm%s_rspTag%s = r_rdStrm%s_openTag%s;\n",
				strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());

			if (pStrm->m_bClose)
				strmPostInstr.Append("\t\t\t\t\tc_rdStrm%s_bOpenRsp%s = true;\n", strmName.c_str(), strmIdxStr.c_str());
			else {
				strmPostInstr.Append("\t\t\t\t\tc_rdStrm%s_rspCnt%s = r_rdStrm%s_openRspCnt%s;\n",
					strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
				strmPostInstr.Append("\t\t\t\t\tc_rdStrm%s_bOpenRsp%s = r_rdStrm%s_openRspCnt%s != 0;\n",
					strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
			}

			strmPostInstr.Append("\t\t\t\t}\n");
			strmPostInstr.Append("\t\t\t} else if (r_rdStrm%s_buf%s.m_toggle != r_rdStrm%s_rspRdPtr2%s(%d,%d) && !r_rdStrm%s_bCollision%s) {\n",
				strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), bufPtrW - 1, bufPtrW - 1, strmName.c_str(), strmIdxStr.c_str());

			if (pStrm->m_bClose) {
				strmPostInstr.Append("\t\t\t\tif (!r_rdStrm%s_bPreValid%s || !r_rdStrm%s_bValid%s || r_rdStrm%s_bClosingRsp%s) {\n",
					strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
				strmPostInstr.Append("\t\t\t\t\tc_rdStrm%s_bRspValid%s = !r_rdStrm%s_bClosingRsp%s;\n",
					strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
			} else {
				strmPostInstr.Append("\t\t\t\tif (!r_rdStrm%s_bPreValid%s || !r_rdStrm%s_bValid%s) {\n",
					strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
				strmPostInstr.Append("\t\t\t\t\tc_rdStrm%s_bRspValid%s = true;\n", strmName.c_str(), strmIdxStr.c_str());
			}

			if (!pStrm->m_bClose) {
				strmPostInstr.Append("\t\t\t\t\tc_rdStrm%s_bRspLast%s = r_rdStrm%s_rspCnt%s == 1;\n",
					strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
				strmPostInstr.Append("\t\t\t\t\tc_rdStrm%s_bOpenRsp%s = r_rdStrm%s_rspCnt%s != 1;\n",
					strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
				strmPostInstr.Append("\t\t\t\t\tc_rdStrm%s_rspCnt%s = r_rdStrm%s_rspCnt%s - 1;\n",
					strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
			}
			strmPostInstr.Append("\t\t\t\t\tc_rdStrm%s_rspRdPtrSel%s = 3;\n", strmName.c_str(), strmIdxStr.c_str());
			strmPostInstr.Append("\t\t\t\t\tc_rdStrm%s_rspState%s = 2;\n", strmName.c_str(), strmIdxStr.c_str());
			strmPostInstr.Append("\t\t\t\t} else {\n");
			strmPostInstr.Append("\t\t\t\t\tc_rdStrm%s_rspRdPtrSel%s = 2;\n", strmName.c_str(), strmIdxStr.c_str());
			strmPostInstr.Append("\t\t\t\t\tc_rdStrm%s_rspState%s = 2;\n", strmName.c_str(), strmIdxStr.c_str());
			strmPostInstr.Append("\t\t\t\t}\n");

			strmPostInstr.Append("\t\t\t} else {\n");
			strmPostInstr.Append("\t\t\t\tc_rdStrm%s_rspRdPtrSel%s = 1;\n", strmName.c_str(), strmIdxStr.c_str());
			strmPostInstr.Append("\t\t\t\tc_rdStrm%s_rspState%s = 1;\n", strmName.c_str(), strmIdxStr.c_str());
			strmPostInstr.Append("\t\t\t}\n");

			if (pStrm->m_bClose) {
				if (mod.m_clkRate == eClk1x)
					strmPostInstr.Append("\t\t\tif (!r_rdStrm%s_bClose%s && r_rdStrm%s_bClosingRsp%s &&\n",
					strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
				else
					strmPostInstr.Append("\t\t\tif (!r_rdStrm%s_bClose%s && !r_rdStrm%s_bClose2%s && r_rdStrm%s_bClosingRsp%s &&\n",
					strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());

				strmPostInstr.Append("\t\t\t\tr_rdStrm%s_rspRdPtr2%s(%d,%d) == r_rdStrm%s_closingWrPtr%s(%d,%d))\n",
					strmName.c_str(), strmIdxStr.c_str(), bufPtrW - 2, 3 - pStrm->m_elemByteW,
					strmName.c_str(), strmIdxStr.c_str(), bufPtrW - 2, 3 - pStrm->m_elemByteW);
				strmPostInstr.Append("\t\t\t{\n");
				strmPostInstr.Append("\t\t\t\tc_rdStrm%s_bOpenRsp%s = false;\n", strmName.c_str(), strmIdxStr.c_str());
				strmPostInstr.Append("\t\t\t\tc_rdStrm%s_bClosingRsp%s = false;\n", strmName.c_str(), strmIdxStr.c_str());
				strmPostInstr.Append("\t\t\t}\n");
			}

			strmPostInstr.Append("\t\t\tbreak;\n");

			strmPostInstr.Append("\t\tdefault:\n");
			strmPostInstr.Append("\t\t\tbreak;\n");
			strmPostInstr.Append("\t\t}\n");
			strmPostInstr.Append("\n");

			strmPostInstr.Append("\t\tif (!r_rdStrm%s_bPreValid%s || !r_rdStrm%s_bValid%s) {\n",
				strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
			strmPostInstr.Append("\t\t\tc_rdStrm%s_preData%s = r_rdStrm%s_buf%s.m_data;\n",
				strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
			if (!pStrm->m_bClose)
				strmPostInstr.Append("\t\t\tc_rdStrm%s_bPreLast%s = c_rdStrm%s_bRspLast%s;\n",
				strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());

			strmPostInstr.Append("\t\t}\n");
			strmPostInstr.Append("\n");

			strmPostInstr.Append("\t\tif (!c_rdStrm%s_bValid%s) {\n", strmName.c_str(), strmIdxStr.c_str());
			strmPostInstr.Append("\t\t\tc_rdStrm%s_data%s = r_rdStrm%s_bPreValid%s ? r_rdStrm%s_preData%s : r_rdStrm%s_buf%s.m_data;\n",
				strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(),
				strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
			if (!pStrm->m_bClose)
				strmPostInstr.Append("\t\t\tc_rdStrm%s_bLast%s = r_rdStrm%s_bPreValid%s ? r_rdStrm%s_bPreLast%s : c_rdStrm%s_bRspLast%s;\n",
				strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(),
				strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());

			if (pStrm->m_pTag != 0) {
				strmPostInstr.Append("\n");
				if (!pStrm->m_bClose)
					strmPostInstr.Append("\t\t\tif (!r_rdStrm%s_bOpenRsp%s && (!r_rdStrm%s_bValid%s || r_rdStrm%s_bLast%s))\n",
					strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
				else
					strmPostInstr.Append("\t\t\tif (!r_rdStrm%s_bOpenRsp%s)\n",
					strmName.c_str(), strmIdxStr.c_str());
				strmPostInstr.Append("\t\t\t\tc_rdStrm%s_tag%s = r_rdStrm%s_openTag%s;\n",
					strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
				if (!pStrm->m_bClose)
					strmPostInstr.Append("\t\t\telse if (r_rdStrm%s_bOpenRsp%s && r_rdStrm%s_bLast%s)\n",
					strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
				else
					strmPostInstr.Append("\t\t\telse\n");
				strmPostInstr.Append("\t\t\t\tc_rdStrm%s_tag%s = r_rdStrm%s_rspTag%s;\n",
					strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
			}

			strmPostInstr.Append("\t\t}\n");
			strmPostInstr.Append("\n");

			strmPostInstr.Append("\t\tc_rdStrm%s_bPreValid%s = r_rdStrm%s_bPreValid%s && r_rdStrm%s_bValid%s && c_rdStrm%s_bValid%s ||\n",
				strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
			strmPostInstr.Append("\t\t\t!r_rdStrm%s_bPreValid%s && r_rdStrm%s_bValid%s && c_rdStrm%s_bValid%s && c_rdStrm%s_bRspValid%s;\n",
				strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());

			if (!pStrm->m_bClose)
				strmPostInstr.Append("\t\tc_rdStrm%s_bValid%s = c_rdStrm%s_bValid%s || c_rdStrm%s_bRspValid%s || r_rdStrm%s_bPreValid%s;\n",
				strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(),
				strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
			else
				strmPostInstr.Append("\t\tc_rdStrm%s_bValid%s = !c_rdStrm%s_bClosingRsp%s && (c_rdStrm%s_bValid%s || c_rdStrm%s_bRspValid%s || r_rdStrm%s_bPreValid%s);\n",
				strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(),
				strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());

			strmPostInstr.Append("\n");


			strmPostInstr.Append("\t\tswitch (c_rdStrm%s_rspRdPtrSel%s) {\n", strmName.c_str(), strmIdxStr.c_str());
			strmPostInstr.Append("\t\tcase 0:\n");
			strmPostInstr.Append("\t\t\tc_rdStrm%s_rspRdPtr%s = r_rdStrm%s_nextRspRdPtr%s;\n",
				strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
			strmPostInstr.Append("\t\t\tbreak;\n");

			strmPostInstr.Append("\t\tcase 1:\n");
			strmPostInstr.Append("\t\t\tc_rdStrm%s_rspRdPtr%s = r_rdStrm%s_rspRdPtr2%s;\n",
				strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
			strmPostInstr.Append("\t\t\tbreak;\n");

			strmPostInstr.Append("\t\tcase 2:\n");
			strmPostInstr.Append("\t\t\tc_rdStrm%s_rspRdPtr%s = r_rdStrm%s_rspRdPtr%s;\n",
				strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
			strmPostInstr.Append("\t\t\tbreak;\n");

			strmPostInstr.Append("\t\tcase 3:\n");
			strmPostInstr.Append("\t\t\tc_rdStrm%s_rspRdPtr%s = r_rdStrm%s_rspRdPtr%s + 1;\n",
				strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
			strmPostInstr.Append("\t\t\tbreak;\n");

			strmPostInstr.Append("\t\t}\n");
			strmPostInstr.Append("\n");

			if (pStrm->m_elemByteW == 3)
				strmPostInstr.Append("\t\tm_rdStrm%s_buf%s.read_addr(c_rdStrm%s_rspRdPtr%s(%d,%d));\n",
				strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), bufPtrW - 2, 3 - pStrm->m_elemByteW);
			else
				strmPostInstr.Append("\t\tm_rdStrm%s_buf%s.read_addr(c_rdStrm%s_rspRdPtr%s(%d,0), c_rdStrm%s_rspRdPtr%s(%d,%d));\n",
				strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), 3 - pStrm->m_elemByteW - 1, strmName.c_str(), strmIdxStr.c_str(), bufPtrW - 2, 3 - pStrm->m_elemByteW);
			strmPostInstr.Append("\n");

			strmPostInstr.Append("\t\tif (r_rdStrm%s_bufInitPtr(%d,%d) == 0) {\n", strmName.c_str(), bufPtrW - 1, bufPtrW - 1);
			strmPostInstr.Append("\n");

			strmPostInstr.Append("\t\t\tCRdStrm%s_buf rdStrm%s_rspData;\n", strmName.c_str(), strmName.c_str());
			strmPostInstr.Append("\t\t\trdStrm%s_rspData.m_data = 0;\n", strmName.c_str());
			strmPostInstr.Append("\t\t\trdStrm%s_rspData.m_toggle = 0;\n", strmName.c_str());
			strmPostInstr.Append("\n");

			strmPostInstr.Append("\t\t\tm_rdStrm%s_buf%s.write_addr( r_rdStrm%s_bufInitPtr(%d,%d) );\n",
				strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), bufPtrW - 2, 3 - pStrm->m_elemByteW);
			for (int idx = 0; idx < qwElemCnt; idx += 1) {
				if (pStrm->m_elemByteW == 3)
					strmPostInstr.Append("\t\t\tm_rdStrm%s_buf%s.write_mem(rdStrm%s_rspData);\n", strmName.c_str(), strmIdxStr.c_str(), strmName.c_str());
				else
					strmPostInstr.Append("\t\t\tm_rdStrm%s_buf%s.write_mem(%d, rdStrm%s_rspData);\n", strmName.c_str(), strmIdxStr.c_str(), idx, strmName.c_str());
			}
			strmPostInstr.Append("\t\t\tc_rdStrm%s_bufInitPtr = r_rdStrm%s_bufInitPtr + %d;\n", strmName.c_str(), strmName.c_str(), qwElemCnt);
			strmPostInstr.Append("\t\t}\n");
			strmPostInstr.Append("\n");

			if (mod.m_clkRate == eClk1x)
				strmPostInstr.Append("\t\tc_rdStrm%s_bOpenBusy%s = c_rdStrm%s_bOpenReq%s || m_rdStrm%s_nextRspQue%s.full() || r_rdStrm%s_bufInitPtr(%d,%d) != 1;\n",
				strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), bufPtrW - 1, bufPtrW - 1);
			else
				strmPostInstr.Append("\t\tc_rdStrm%s_bOpenBusy%s = c_rdStrm%s_bOpenReq%s || r_rdStrm%s_bOpenReq%s || m_rdStrm%s_nextRspQue%s.full() || r_rdStrm%s_bufInitPtr(%d,%d) != 1;\n",
				strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(),
				strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), bufPtrW - 1, bufPtrW - 1);

			strmPostInstr.Append("\t}\n");
			strmPostInstr.Append("\n");

			strmPostInstr.Append("\tunion CRdStrm%s_rdTid {\n", strmName.c_str());
			strmPostInstr.Append("\t\tstruct {\n");
			strmPostInstr.Append("\t\t\tuint32_t m_mifStrmId : 4;\n");
			strmPostInstr.Append("\t\t\tuint32_t m_rspBufWrPtr : %d;\n", bufPtrW);
			if (bMultiQwReq) {
				strmPostInstr.Append("\t\t\tuint32_t m_fstQwIdx : 3;\n");
				strmPostInstr.Append("\t\t\tuint32_t m_lstQwIdx : 3;\n");
				strmPostInstr.Append("\t\t\tuint32_t m_pad1 : %d;\n", 19 - bufPtrW);
				strmPostInstr.Append("\t\t\tuint32_t m_reqQwCntM1 : 3;\n");
			} else {
				strmPostInstr.Append("\t\t\tuint32_t m_pad1 : %d;\n", 25 - bufPtrW);
				strmPostInstr.Append("\t\t\tuint32_t m_reqQwCntM1 : 3;\n");
			}
			strmPostInstr.Append("\t\t};\n");
			strmPostInstr.Append("\t\tstruct {\n");
			strmPostInstr.Append("\t\t\tuint32_t m_pad2 : 29;\n");
			strmPostInstr.Append("\t\t\tuint32_t m_rspQwIdx : 3;\n");
			strmPostInstr.Append("\t\t};\n");
			strmPostInstr.Append("\t\tuint32_t m_tid;\n");
			strmPostInstr.Append("\t};\n");
			strmPostInstr.Append("\n");

			for (int strmIdx = 0; strmIdx < pStrm->m_strmCnt.AsInt(); strmIdx += 1) {
				string strmIdxStr = pStrm->m_strmCnt.size() == 0 ? "" : VA("[%d]", strmIdx);

				strmPostInstr.Append("\t{\n");

				if (!pStrm->m_bClose)
					strmPostInstr.Append("\t\tc_rdStrm%s_bReqCntZ%s = false;\n", strmName.c_str(), strmIdxStr.c_str());

				//if (bMultiQwReq)
				//	strmPostInstr.Append("\t\tc_rdStrm%s_reqQwCnt%s = 8u - ((r_rdStrm%s_addr%s >> 3) & 0x7);\n", strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
				//else
				//	strmPostInstr.Append("\t\tc_rdStrm%s_reqQwCnt%s = 1;\n", strmName.c_str(), strmIdxStr.c_str());

				if (bMultiQwReq)
					strmPostInstr.Append("\t\tc_rdStrm%s_reqElemCnt%s = %du - ((r_rdStrm%s_addr%s >> %d) & 0x%x);\n",
					strmName.c_str(), strmIdxStr.c_str(), qwElemCnt * 8,
					strmName.c_str(), strmIdxStr.c_str(), pStrm->m_elemByteW, qwElemCnt * 8 - 1);
				else
					strmPostInstr.Append("\t\tc_rdStrm%s_reqElemCnt%s = %du - ((r_rdStrm%s_addr%s >> %d) & 0x%x);\n",
					strmName.c_str(), strmIdxStr.c_str(), qwElemCnt,
					strmName.c_str(), strmIdxStr.c_str(), pStrm->m_elemByteW, qwElemCnt - 1);

				strmPostInstr.Append("\t\tif (c_rdStrm%s_reqElemCnt%s >= r_rdStrm%s_reqCnt%s) {\n",
					strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
				strmPostInstr.Append("\t\t\tc_rdStrm%s_reqElemCnt%s = r_rdStrm%s_reqCnt%s & 0x%x;\n",
					strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), (1 << (4 + 3 - pStrm->m_elemByteW)) - 1);
				if (!pStrm->m_bClose)
					strmPostInstr.Append("\t\t\tc_rdStrm%s_bReqCntZ%s = true;\n", strmName.c_str(), strmIdxStr.c_str());
				strmPostInstr.Append("\t\t}\n");

				strmPostInstr.Append("\t\tc_rdStrm%s_reqQwCnt%s = ((r_rdStrm%s_addr%s & 7) + (c_rdStrm%s_reqElemCnt%s << %d) + 7) >> 3;\n",
					strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(),
					pStrm->m_elemByteW);
				strmPostInstr.Append("\n");

				if (mod.m_clkRate == eClk1x)
					strmPostInstr.Append("\t\tif (c_t1_rdStrm%s_bReqSel%s && !r_%sP%dToMif_reqAvlCntBusy) {\n",
					strmName.c_str(), strmIdxStr.c_str(), mod.m_modName.Lc().c_str(), pStrm->m_memPort[strmIdx]);
				else
					strmPostInstr.Append("\t\tif (r_t2_rdStrm%s_bReqSel%s && !r_%sP%dToMif_reqAvlCntBusy) {\n",
					strmName.c_str(), strmIdxStr.c_str(), mod.m_modName.Lc().c_str(), pStrm->m_memPort[strmIdx]);
				strmPostInstr.Append("\n");

				if (!pStrm->m_bClose) {
					strmPostInstr.Append("\t\t\tif (%c_rdStrm%s_bReqCntZ%s)\n",
						mod.m_clkRate == eClk2x ? 'r' : 'c', strmName.c_str(), strmIdxStr.c_str());
					strmPostInstr.Append("\t\t\t\tc_rdStrm%s_bOpenReq%s = false;\n", strmName.c_str(), strmIdxStr.c_str());
					strmPostInstr.Append("\n");
				}

				strmPostInstr.Append("\t\t\tCRdStrm%s_rdTid rdTid;\n", strmName.c_str());
				strmPostInstr.Append("\t\t\trdTid.m_mifStrmId = %d;\n", pStrm->m_arbRr[strmIdx]);

				if (bMultiQwReq) {
					strmPostInstr.Append("\t\t\trdTid.m_fstQwIdx = %c_rdStrm%s_reqQwCnt%s == 1 ? 0 : ((r_rdStrm%s_addr%s >> 3) & 7);\n",
						mod.m_clkRate == eClk2x ? 'r' : 'c', strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
					strmPostInstr.Append("\t\t\trdTid.m_lstQwIdx = (ht_uint3)(rdTid.m_fstQwIdx + %c_rdStrm%s_reqQwCnt%s - 1);\n",
						mod.m_clkRate == eClk2x ? 'r' : 'c', strmName.c_str(), strmIdxStr.c_str());
					strmPostInstr.Append("\t\t\trdTid.m_reqQwCntM1 = %c_rdStrm%s_reqQwCnt%s == 1 ? 0 : 7;\n",
						mod.m_clkRate == eClk2x ? 'r' : 'c', strmName.c_str(), strmIdxStr.c_str());
					strmPostInstr.Append("\t\t\trdTid.m_rspBufWrPtr = r_rdStrm%s_reqWrPtr%s - (rdTid.m_fstQwIdx << %d);\n", strmName.c_str(), strmIdxStr.c_str(), 3 - pStrm->m_elemByteW);
				} else {
					strmPostInstr.Append("\t\t\trdTid.m_reqQwCntM1 = (%c_rdStrm%s_reqQwCnt%s - 1) & 7;\n",
						mod.m_clkRate == eClk2x ? 'r' : 'c', strmName.c_str(), strmIdxStr.c_str());
					strmPostInstr.Append("\t\t\trdTid.m_rspBufWrPtr = r_rdStrm%s_reqWrPtr%s;\n", strmName.c_str(), strmIdxStr.c_str());
				}

				strmPostInstr.Append("\n");

				strmPostInstr.Append("\t\t\tc_t2_%sP%dToMif_reqRdy = true;\n", mod.m_modName.Lc().c_str(), pStrm->m_memPort[strmIdx]);
				strmPostInstr.Append("\t\t\tc_t2_%sP%dToMif_req.m_host = %s;\n",
					mod.m_modName.Lc().c_str(), pStrm->m_memPort[strmIdx], pStrm->m_memSrc.AsStr() == "host" ? "true" : "false");
				strmPostInstr.Append("\t\t\tc_t2_%sP%dToMif_req.m_type = MEM_REQ_RD;\n", mod.m_modName.Lc().c_str(), pStrm->m_memPort[strmIdx]);

				if (bMultiQwReq)
					strmPostInstr.Append("\t\t\tc_t2_%sP%dToMif_req.m_addr = r_rdStrm%s_addr%s & (%c_rdStrm%s_reqQwCnt%s == 1 ? ~0x7ULL : ~0x3fULL);\n",
					mod.m_modName.Lc().c_str(), pStrm->m_memPort[strmIdx], strmName.c_str(), strmIdxStr.c_str(),
					mod.m_clkRate == eClk2x ? 'r' : 'c', strmName.c_str(), strmIdxStr.c_str());
				else
					strmPostInstr.Append("\t\t\tc_t2_%sP%dToMif_req.m_addr = r_rdStrm%s_addr%s & ~0x7ULL;\n",
					mod.m_modName.Lc().c_str(), pStrm->m_memPort[strmIdx], strmName.c_str(), strmIdxStr.c_str());
				strmPostInstr.Append("\t\t\tc_t2_%sP%dToMif_req.m_size = MEM_REQ_U64;\n", mod.m_modName.Lc().c_str(), pStrm->m_memPort[strmIdx]);
				strmPostInstr.Append("\t\t\tc_t2_%sP%dToMif_req.m_tid = rdTid.m_tid;\n", mod.m_modName.Lc().c_str(), pStrm->m_memPort[strmIdx]);
				strmPostInstr.Append("#\t\t\tifndef _HTV\n");
				strmPostInstr.Append("\t\t\tc_t2_%sP%dToMif_req.m_file = __FILE__;\n", mod.m_modName.Lc().c_str(), pStrm->m_memPort[strmIdx]);
				strmPostInstr.Append("\t\t\tc_t2_%sP%dToMif_req.m_line = __LINE__;\n", mod.m_modName.Lc().c_str(), pStrm->m_memPort[strmIdx]);
				strmPostInstr.Append("\t\t\tc_t2_%sP%dToMif_req.m_time = sc_time_stamp().value();\n",
					mod.m_modName.Lc().c_str(), pStrm->m_memPort[strmIdx]);
				strmPostInstr.Append("\t\t\tc_t2_%sP%dToMif_req.m_orderChk = true;\n", mod.m_modName.Lc().c_str(), pStrm->m_memPort[strmIdx]);
				strmPostInstr.Append("\n");

				strmPostInstr.Append("\t\t\tHt::g_syscMon.UpdateModuleMemReads(m_htMonModId, %d, 1);\n", pStrm->m_memPort[strmIdx]);
				if (bMultiQwReq)
					strmPostInstr.Append("\t\t\tHt::g_syscMon.UpdateModuleMemRead64s(m_htMonModId, %d, 1);\n", pStrm->m_memPort[strmIdx]);
				strmPostInstr.Append("\t\t\tHt::g_syscMon.UpdateModuleMemReadBytes(m_htMonModId, %d, %d);\n", pStrm->m_memPort[strmIdx],
					bMultiQwReq ? 64 : 8);

				strmPostInstr.Append("#\t\t\tendif\n");
				strmPostInstr.Append("\n");
				strmPostInstr.Append("\t\t\tc_rdStrm%s_reqWrPtr%s(%d,%d) = r_rdStrm%s_reqWrPtr%s(%d,%d) + %c_rdStrm%s_reqQwCnt%s;\n",
					strmName.c_str(), strmIdxStr.c_str(), bufPtrW - 1, 3 - pStrm->m_elemByteW, strmName.c_str(), strmIdxStr.c_str(), bufPtrW - 1, 3 - pStrm->m_elemByteW,
					mod.m_clkRate == eClk2x ? 'r' : 'c', strmName.c_str(), strmIdxStr.c_str());

				if (3 - pStrm->m_elemByteW - 1 >= 0)
					strmPostInstr.Append("\t\t\tc_rdStrm%s_reqWrPtr%s(%d,0) = 0;\n",
					strmName.c_str(), strmIdxStr.c_str(), 3 - pStrm->m_elemByteW - 1);

				if (mod.m_clkRate == eClk1x)
					strmPostInstr.Append("\t\t\tc_rdStrm%s_addr%s = r_rdStrm%s_addr%s + (%c_rdStrm%s_reqElemCnt%s << %d);\n",
					strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(),
					mod.m_clkRate == eClk2x ? 'r' : 'c', strmName.c_str(), strmIdxStr.c_str(), pStrm->m_elemByteW);
				else {
					strmPostInstr.Append("\t\t\tht_uint25 lowSum = r_rdStrm%s_addr%s(23,0) + (%c_rdStrm%s_reqElemCnt%s << %d);\n",
						strmName.c_str(), strmIdxStr.c_str(),
						mod.m_clkRate == eClk2x ? 'r' : 'c', strmName.c_str(), strmIdxStr.c_str(), pStrm->m_elemByteW);
					strmPostInstr.Append("\t\t\tc_rdStrm%s_addr%s(23,0) = lowSum(23,0);\n",
						strmName.c_str(), strmIdxStr.c_str());
					strmPostInstr.Append("\t\t\tc_rdStrm%s_sumCy%s = lowSum(24,24);\n",
						strmName.c_str(), strmIdxStr.c_str());
				}

				strmPostInstr.Append("\t\t\tc_rdStrm%s_reqCnt%s = r_rdStrm%s_reqCnt%s - (ht_uint%d)%c_rdStrm%s_reqElemCnt%s;\n",
					strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), pStrm->m_elemCntW.AsInt(),
					mod.m_clkRate == eClk2x ? 'r' : 'c', strmName.c_str(), strmIdxStr.c_str());

				if (mod.m_memPortList[pStrm->m_memPort[strmIdx]]->m_strmList.size() > 1) {
					strmPostInstr.Append("\n");
					strmPostInstr.Append("\t\t\tc_%sP%dToMif_reqRr = 0x%x;\n",
						mod.m_modName.Lc().c_str(), pStrm->m_memPort[strmIdx],
						1 << ((pStrm->m_arbRr[strmIdx] + 1) % mod.m_memPortList[pStrm->m_memPort[strmIdx]]->m_strmList.size()));
				}
				strmPostInstr.Append("\t\t}\n");
				strmPostInstr.Append("\t}\n");
				strmPostInstr.Append("\n");
			}

		} else {
			// write stream

			if (pStrm->m_bClose && pStrm->m_reserve.AsInt() > 0) {
				m_strmRegDecl.Append("\tstruct CWrStrm%s_pipeQue {\n", strmName.c_str());
				m_strmRegDecl.Append("#\t\tifndef _HTV\n");
				m_strmRegDecl.Append("\t\tfriend void sc_trace(sc_trace_file *tf, const CWrStrm%s_pipeQue & v, const std::string & NAME ) {\n", strmName.c_str());
				m_strmRegDecl.Append("\t\t\tsc_trace(tf, v.m_data, NAME + \".m_data\");\n");
				m_strmRegDecl.Append("\t\t\tsc_trace(tf, v.m_bData, NAME + \".m_bData\");\n");
				m_strmRegDecl.Append("\t\t\tsc_trace(tf, v.m_bClose, NAME + \".m_bClose\");\n");
				m_strmRegDecl.Append("\t\t}\n");
				m_strmRegDecl.Append("\t\tvoid operator = (int zero) {\n");
				m_strmRegDecl.Append("\t\t\tassert(zero == 0);\n");
				m_strmRegDecl.Append("\t\t\tm_data = 0;\n");
				m_strmRegDecl.Append("\t\t\tm_bData = false;\n");
				m_strmRegDecl.Append("\t\t\tm_bClose = false;\n");
				m_strmRegDecl.Append("\t\t}\n");
				m_strmRegDecl.Append("#\t\tendif\n");
				m_strmRegDecl.Append("\t\t%s m_data;\n", pStrm->m_pType->m_typeName.c_str());
				m_strmRegDecl.Append("\t\tbool m_bData;\n");
				m_strmRegDecl.Append("\t\tbool m_bClose;\n");
				m_strmRegDecl.Append("\t};\n");
				m_strmRegDecl.Append("\n");
			}

			GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, "bool", VA("r_wrStrm%s_bOpenBusy", strmName.c_str()), strmIdVec);
			m_strmRegDecl.Append("\tbool c_wrStrm%s_bOpenBusy%s;\n", strmName.c_str(), strmIdDecl.c_str());
			if (pStrm->m_strmCnt.size() == 0) {
				if (pStrm->m_bClose)
					strmPreInstr.Append("\tc_wrStrm%s_bOpenBusy = r_wrStrm%s_bOpenBusy && (r_wrStrm%s_bOpenBufWr || r_wrStrm%s_bOpenBufRd || r_wrStrm%s_bClosingBufRd);\n",
					strmName.c_str(), strmName.c_str(), strmName.c_str(), strmName.c_str(), strmName.c_str());
				else
					strmPreInstr.Append("\tc_wrStrm%s_bOpenBusy = r_wrStrm%s_bOpenBusy && (r_wrStrm%s_bOpenBufWr || r_wrStrm%s_bOpenBufRd);\n",
					strmName.c_str(), strmName.c_str(), strmName.c_str(), strmName.c_str());
				strmReg.Append("\tr_wrStrm%s_bOpenBusy = !r_reset1x && c_wrStrm%s_bOpenBusy;\n", strmName.c_str(), strmName.c_str());
			} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
				if (pStrm->m_bClose)
					strmPreInstr.Append("\tc_wrStrm%s_bOpenBusy[%d] = r_wrStrm%s_bOpenBusy[%d] && (r_wrStrm%s_bOpenBufWr[%d] || r_wrStrm%s_bOpenBufRd[%d] || r_wrStrm%s_bClosingBufRd[%d]);\n",
					strmName.c_str(), i, strmName.c_str(), i, strmName.c_str(), i, strmName.c_str(), i, strmName.c_str(), i);
				else
					strmPreInstr.Append("\tc_wrStrm%s_bOpenBusy[%d] = r_wrStrm%s_bOpenBusy[%d] && (r_wrStrm%s_bOpenBufWr[%d] || r_wrStrm%s_bOpenBufRd[%d]);\n",
					strmName.c_str(), i, strmName.c_str(), i, strmName.c_str(), i, strmName.c_str(), i);
				strmReg.Append("\tr_wrStrm%s_bOpenBusy[%d] = !r_reset1x && c_wrStrm%s_bOpenBusy[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
			}

			m_strmRegDecl.Append("\tbool ht_noload c_wrStrm%s_bOpenAvail%s;\n", strmName.c_str(), strmIdDecl.c_str());
			if (pStrm->m_strmCnt.size() == 0) {
				strmPreInstr.Append("\tc_wrStrm%s_bOpenAvail = false;\n", strmName.c_str());
			} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
				strmPreInstr.Append("\tc_wrStrm%s_bOpenAvail[%d] = false;\n", strmName.c_str(), i);
			}

			GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, "bool", VA("r_wrStrm%s_bOpenBufWr", strmName.c_str()), strmIdVec);
			m_strmRegDecl.Append("\tbool c_wrStrm%s_bOpenBufWr%s;\n", strmName.c_str(), strmIdDecl.c_str());
			if (pStrm->m_strmCnt.size() == 0) {
				strmPreInstr.Append("\tc_wrStrm%s_bOpenBufWr = r_wrStrm%s_bOpenBufWr;\n", strmName.c_str(), strmName.c_str());
				strmReg.Append("\tr_wrStrm%s_bOpenBufWr = !r_reset1x && c_wrStrm%s_bOpenBufWr;\n", strmName.c_str(), strmName.c_str());
			} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
				strmPreInstr.Append("\tc_wrStrm%s_bOpenBufWr[%d] = r_wrStrm%s_bOpenBufWr[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				strmReg.Append("\tr_wrStrm%s_bOpenBufWr[%d] = !r_reset1x && c_wrStrm%s_bOpenBufWr[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
			}

			GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, "bool", VA("r_wrStrm%s_bOpenBufRd", strmName.c_str(), strmIdDecl.c_str()), strmIdVec);
			m_strmRegDecl.Append("\tbool c_wrStrm%s_bOpenBufRd%s;\n", strmName.c_str(), strmIdDecl.c_str());
			if (pStrm->m_strmCnt.size() == 0) {
				strmPreInstr.Append("\tc_wrStrm%s_bOpenBufRd = r_wrStrm%s_bOpenBufRd;\n", strmName.c_str(), strmName.c_str());
				strmReg.Append("\tr_wrStrm%s_bOpenBufRd = !r_reset1x && c_wrStrm%s_bOpenBufRd;\n", strmName.c_str(), strmName.c_str());
			} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
				strmPreInstr.Append("\tc_wrStrm%s_bOpenBufRd[%d] = r_wrStrm%s_bOpenBufRd[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				strmReg.Append("\tr_wrStrm%s_bOpenBufRd[%d] = !r_reset1x && c_wrStrm%s_bOpenBufRd[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
			}

			GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, "bool", VA("r_wrStrm%s_bBufRd", strmName.c_str()), strmIdVec);
			m_strmRegDecl.Append("\tbool c_wrStrm%s_bBufRd%s;\n", strmName.c_str(), strmIdDecl.c_str());
			if (pStrm->m_strmCnt.size() == 0) {
				strmPreInstr.Append("\tc_wrStrm%s_bBufRd = r_wrStrm%s_bBufRd;\n", strmName.c_str(), strmName.c_str());
				strmReg.Append("\tr_wrStrm%s_bBufRd = !r_reset1x && c_wrStrm%s_bBufRd;\n", strmName.c_str(), strmName.c_str());
			} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
				strmPreInstr.Append("\tc_wrStrm%s_bBufRd[%d] = r_wrStrm%s_bBufRd[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				strmReg.Append("\tr_wrStrm%s_bBufRd[%d] = !r_reset1x && c_wrStrm%s_bBufRd[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
			}

			GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, "bool", VA("r_wrStrm%s_bPaused", strmName.c_str()), strmRspGrpVec);
			m_strmRegDecl.Append("\tbool c_wrStrm%s_bPaused%s;\n", strmName.c_str(), strmRspGrpDecl.c_str());
			if (pStrm->m_rspGrpW.AsInt() == 0) {
				strmPreInstr.Append("\tc_wrStrm%s_bPaused = r_wrStrm%s_bPaused;\n", strmName.c_str(), strmName.c_str());
				strmReg.Append("\tr_wrStrm%s_bPaused = !r_reset1x && c_wrStrm%s_bPaused;\n", strmName.c_str(), strmName.c_str());
			} else for (int i = 0; i < strmRspGrpCnt; i += 1) {
				strmPreInstr.Append("\tc_wrStrm%s_bPaused[%d] = r_wrStrm%s_bPaused[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				strmReg.Append("\tr_wrStrm%s_bPaused[%d] = !r_reset1x && c_wrStrm%s_bPaused[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
			}

			if (mod.m_threads.m_htIdW.AsInt() > 0 && pStrm->m_rspGrpW.size() > 0) {
				if (pStrm->m_rspGrpW.AsInt() == 0) {
					GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, VA("sc_uint<%s_HTID_W>", mod.m_modName.Upper().c_str()), VA("r_wrStrm%s_rsmHtId", strmName.c_str()), strmRspGrpVec);
					m_strmRegDecl.Append("\tsc_uint<%s_HTID_W> c_wrStrm%s_rsmHtId%s;\n", mod.m_modName.Upper().c_str(), strmName.c_str(), strmRspGrpDecl.c_str());
					strmPreInstr.Append("\tc_wrStrm%s_rsmHtId = r_wrStrm%s_rsmHtId;\n", strmName.c_str(), strmName.c_str());
					strmReg.Append("\tr_wrStrm%s_rsmHtId = c_wrStrm%s_rsmHtId;\n", strmName.c_str(), strmName.c_str());
				} else {
					m_strmRegDecl.Append("\tht_dist_ram<sc_uint<%s_HTID_W>, %d> m_wrStrm%s_rsmHtId;\n", mod.m_modName.Upper().c_str(), pStrm->m_rspGrpW.AsInt(), strmName.c_str());
					strmReg.Append("\tm_wrStrm%s_rsmHtId.clock();\n", strmName.c_str());
				}
			}

			if (pStrm->m_rspGrpW.AsInt() == 0) {
				GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, VA("sc_uint<%s_INSTR_W>", mod.m_modName.Upper().c_str()), VA("r_wrStrm%s_rsmHtInstr", strmName.c_str()), strmRspGrpVec);
				m_strmRegDecl.Append("\tsc_uint<%s_INSTR_W> c_wrStrm%s_rsmHtInstr%s;\n", mod.m_modName.Upper().c_str(), strmName.c_str(), strmRspGrpDecl.c_str());
				strmPreInstr.Append("\tc_wrStrm%s_rsmHtInstr = r_wrStrm%s_rsmHtInstr;\n", strmName.c_str(), strmName.c_str());
				strmReg.Append("\tr_wrStrm%s_rsmHtInstr = c_wrStrm%s_rsmHtInstr;\n", strmName.c_str(), strmName.c_str());
			} else {
				m_strmRegDecl.Append("\tht_dist_ram<sc_uint<%s_INSTR_W>, %d> m_wrStrm%s_rsmHtInstr;\n", mod.m_modName.Upper().c_str(), pStrm->m_rspGrpW.AsInt(), strmName.c_str());
				strmReg.Append("\tm_wrStrm%s_rsmHtInstr.clock();\n", strmName.c_str());
			}

			GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, "ht_uint48", VA("r_wrStrm%s_openAddr", strmName.c_str()), strmIdVec);
			m_strmRegDecl.Append("\tht_uint48 c_wrStrm%s_openAddr%s;\n", strmName.c_str(), strmIdDecl.c_str());
			if (pStrm->m_strmCnt.size() == 0) {
				strmPreInstr.Append("\tc_wrStrm%s_openAddr = r_wrStrm%s_openAddr;\n", strmName.c_str(), strmName.c_str());
				strmReg.Append("\tr_wrStrm%s_openAddr = c_wrStrm%s_openAddr;\n", strmName.c_str(), strmName.c_str());
			} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
				strmPreInstr.Append("\tc_wrStrm%s_openAddr[%d] = r_wrStrm%s_openAddr[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				strmReg.Append("\tr_wrStrm%s_openAddr[%d] = c_wrStrm%s_openAddr[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
			}

			if (!pStrm->m_bClose) {
				GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, VA("ht_uint%d", elemCntW), VA("r_wrStrm%s_openCnt", strmName.c_str()), strmIdVec);
				m_strmRegDecl.Append("\tht_uint%d c_wrStrm%s_openCnt%s;\n", elemCntW, strmName.c_str(), strmIdDecl.c_str());
				if (pStrm->m_strmCnt.size() == 0) {
					strmPreInstr.Append("\tc_wrStrm%s_openCnt = r_wrStrm%s_openCnt;\n", strmName.c_str(), strmName.c_str());
					strmReg.Append("\tr_wrStrm%s_openCnt = c_wrStrm%s_openCnt;\n", strmName.c_str(), strmName.c_str());
				} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
					strmPreInstr.Append("\tc_wrStrm%s_openCnt[%d] = r_wrStrm%s_openCnt[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
					strmReg.Append("\tr_wrStrm%s_openCnt[%d] = c_wrStrm%s_openCnt[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				}
			}

			GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, "ht_uint48", VA("r_wrStrm%s_bufRdAddr", strmName.c_str()), strmIdVec);
			m_strmRegDecl.Append("\tht_uint48 c_wrStrm%s_bufRdAddr%s;\n", strmName.c_str(), strmIdDecl.c_str());
			if (pStrm->m_strmCnt.size() == 0) {

				if (mod.m_clkRate == eClk1x)
					strmPreInstr.Append("\tc_wrStrm%s_bufRdAddr = r_wrStrm%s_bufRdAddr;\n", strmName.c_str(), strmName.c_str());
				else
					strmPreInstr.Append("\tc_wrStrm%s_bufRdAddr = r_wrStrm%s_bufRdAddr + (r_wrStrm%s_sumCy << 24);\n",
					strmName.c_str(), strmName.c_str(), strmName.c_str());

				strmReg.Append("\tr_wrStrm%s_bufRdAddr = c_wrStrm%s_bufRdAddr;\n", strmName.c_str(), strmName.c_str());
			} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {

				if (mod.m_clkRate == eClk1x)
					strmPreInstr.Append("\tc_wrStrm%s_bufRdAddr[%d] = r_wrStrm%s_bufRdAddr[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				else
					strmPreInstr.Append("\tc_wrStrm%s_bufRdAddr[%d] = r_wrStrm%s_bufRdAddr[%d] + (r_wrStrm%s_sumCy[%d] << 24);\n",
					strmName.c_str(), i, strmName.c_str(), i, strmName.c_str(), i);

				strmReg.Append("\tr_wrStrm%s_bufRdAddr[%d] = c_wrStrm%s_bufRdAddr[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
			}

			if (mod.m_clkRate == eClk2x) {
				GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, "ht_uint1", VA("r_wrStrm%s_sumCy", strmName.c_str()), strmIdVec);
				m_strmRegDecl.Append("\tht_uint1 c_wrStrm%s_sumCy%s;\n", strmName.c_str(), strmIdDecl.c_str());
				if (pStrm->m_strmCnt.size() == 0) {
					strmPreInstr.Append("\tc_wrStrm%s_sumCy = 0;\n", strmName.c_str());
					strmReg.Append("\tr_wrStrm%s_sumCy = c_wrStrm%s_sumCy;\n", strmName.c_str(), strmName.c_str());
				} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
					strmPreInstr.Append("\tc_wrStrm%s_sumCy[%d] = 0;\n", strmName.c_str(), i);
					strmReg.Append("\tr_wrStrm%s_sumCy[%d] = c_wrStrm%s_sumCy[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				}
			}

			if (!pStrm->m_bClose) {
				GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, VA("ht_uint%d", elemCntW), VA("r_wrStrm%s_bufWrRem", strmName.c_str()), strmIdVec);
				m_strmRegDecl.Append("\tht_uint%d c_wrStrm%s_bufWrRem%s;\n", elemCntW, strmName.c_str(), strmIdDecl.c_str());
				if (pStrm->m_strmCnt.size() == 0) {
					strmPreInstr.Append("\tc_wrStrm%s_bufWrRem = r_wrStrm%s_bufWrRem;\n", strmName.c_str(), strmName.c_str());
					strmReg.Append("\tr_wrStrm%s_bufWrRem = c_wrStrm%s_bufWrRem;\n", strmName.c_str(), strmName.c_str());
				} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
					strmPreInstr.Append("\tc_wrStrm%s_bufWrRem[%d] = r_wrStrm%s_bufWrRem[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
					strmReg.Append("\tr_wrStrm%s_bufWrRem[%d] = c_wrStrm%s_bufWrRem[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				}
			}

			if (!pStrm->m_bClose) {
				GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, VA("ht_uint%d", elemCntW), VA("r_wrStrm%s_bufRdRem", strmName.c_str()), strmIdVec);
				m_strmRegDecl.Append("\tht_uint%d c_wrStrm%s_bufRdRem%s;\n", elemCntW, strmName.c_str(), strmIdDecl.c_str());
				if (pStrm->m_strmCnt.size() == 0) {
					strmPreInstr.Append("\tc_wrStrm%s_bufRdRem = r_wrStrm%s_bufRdRem;\n", strmName.c_str(), strmName.c_str());
					strmReg.Append("\tr_wrStrm%s_bufRdRem = r_reset1x ? (ht_uint%d)0 : c_wrStrm%s_bufRdRem;\n", strmName.c_str(), elemCntW, strmName.c_str());
				} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
					strmPreInstr.Append("\tc_wrStrm%s_bufRdRem[%d] = r_wrStrm%s_bufRdRem[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
					strmReg.Append("\tr_wrStrm%s_bufRdRem[%d] = r_reset1x ? (ht_uint%d)0 : c_wrStrm%s_bufRdRem[%d];\n", strmName.c_str(), i, elemCntW, strmName.c_str(), i);
				}
			}

			if (pStrm->m_rspGrpW.AsInt() > 0) {
				GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, VA("ht_uint%d", pStrm->m_rspGrpW.AsInt()), VA("r_wrStrm%s_openRspGrpId", strmName.c_str()), strmIdVec);
				m_strmRegDecl.Append("\tht_uint%d c_wrStrm%s_openRspGrpId%s;\n", pStrm->m_rspGrpW.AsInt(), strmName.c_str(), strmIdDecl.c_str());
				if (pStrm->m_strmCnt.size() == 0) {
					strmPreInstr.Append("\tc_wrStrm%s_openRspGrpId = r_wrStrm%s_openRspGrpId;\n", strmName.c_str(), strmName.c_str());
					strmReg.Append("\tr_wrStrm%s_openRspGrpId = c_wrStrm%s_openRspGrpId;\n", strmName.c_str(), strmName.c_str());
				} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
					strmPreInstr.Append("\tc_wrStrm%s_openRspGrpId[%d] = r_wrStrm%s_openRspGrpId[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
					strmReg.Append("\tr_wrStrm%s_openRspGrpId[%d] = c_wrStrm%s_openRspGrpId[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				}

				GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, VA("ht_uint%d", pStrm->m_rspGrpW.AsInt()), VA("r_wrStrm%s_rspGrpId", strmName.c_str()), strmIdVec);
				m_strmRegDecl.Append("\tht_uint%d c_wrStrm%s_rspGrpId%s;\n", pStrm->m_rspGrpW.AsInt(), strmName.c_str(), strmIdDecl.c_str());
				if (pStrm->m_strmCnt.size() == 0) {
					strmPreInstr.Append("\tc_wrStrm%s_rspGrpId = r_wrStrm%s_rspGrpId;\n", strmName.c_str(), strmName.c_str());
					strmReg.Append("\tr_wrStrm%s_rspGrpId = c_wrStrm%s_rspGrpId;\n", strmName.c_str(), strmName.c_str());
				} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
					strmPreInstr.Append("\tc_wrStrm%s_rspGrpId[%d] = r_wrStrm%s_rspGrpId[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
					strmReg.Append("\tr_wrStrm%s_rspGrpId[%d] = c_wrStrm%s_rspGrpId[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				}
			}

			if (pStrm->m_bClose) {
				if (pStrm->m_reserve.AsInt() == 0) {
					GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, "bool", VA("r_wrStrm%s_bClose", strmName.c_str(), strmIdDecl.c_str()), strmIdVec);
					m_strmRegDecl.Append("\tbool c_wrStrm%s_bClose%s;\n", strmName.c_str(), strmIdDecl.c_str());
					if (pStrm->m_strmCnt.size() == 0) {
						strmPreInstr.Append("\tc_wrStrm%s_bClose = false;\n", strmName.c_str());
						strmReg.Append("\tr_wrStrm%s_bClose = c_wrStrm%s_bClose;\n", strmName.c_str(), strmName.c_str());
					} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
						strmPreInstr.Append("\tc_wrStrm%s_bClose[%d] = false;\n", strmName.c_str(), i);
						strmReg.Append("\tr_wrStrm%s_bClose[%d] = c_wrStrm%s_bClose[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
					}
				}

				GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, "bool", VA("r_wrStrm%s_bClosingBufRd", strmName.c_str(), strmIdDecl.c_str()), strmIdVec);
				m_strmRegDecl.Append("\tbool c_wrStrm%s_bClosingBufRd%s;\n", strmName.c_str(), strmIdDecl.c_str());
				if (pStrm->m_strmCnt.size() == 0) {
					if (pStrm->m_reserve.AsInt() == 0)
						strmPreInstr.Append("\tc_wrStrm%s_bClosingBufRd = r_wrStrm%s_bClosingBufRd || r_wrStrm%s_bClose && (r_wrStrm%s_bufWrEn || r_wrStrm%s_bufRdElemCnt > 0);\n",
						strmName.c_str(), strmName.c_str(), strmName.c_str(), strmName.c_str(), strmName.c_str());
					else
						strmPreInstr.Append("\tc_wrStrm%s_bClosingBufRd = r_wrStrm%s_bClosingBufRd;\n", strmName.c_str(), strmName.c_str());

					strmReg.Append("\tr_wrStrm%s_bClosingBufRd = !r_reset1x && c_wrStrm%s_bClosingBufRd;\n", strmName.c_str(), strmName.c_str());
				} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
					if (pStrm->m_reserve.AsInt() == 0)
						strmPreInstr.Append("\tc_wrStrm%s_bClosingBufRd[%d] = r_wrStrm%s_bClosingBufRd[%d] || r_wrStrm%s_bClose[%d] && (r_wrStrm%s_bufWrEn[%d] || r_wrStrm%s_bufRdElemCnt[%d] > 0);\n",
						strmName.c_str(), i, strmName.c_str(), i, strmName.c_str(), i, strmName.c_str(), i, strmName.c_str(), i);
					else
						strmPreInstr.Append("\tc_wrStrm%s_bClosingBufRd[%d] = r_wrStrm%s_bClosingBufRd[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
					strmReg.Append("\tr_wrStrm%s_bClosingBufRd[%d] = !r_reset1x && c_wrStrm%s_bClosingBufRd[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				}
			}

			GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, VA("ht_uint%d", bufPtrW), VA("r_wrStrm%s_bufWrElemCnt", strmName.c_str()), strmIdVec);
			m_strmRegDecl.Append("\tht_uint%d c_wrStrm%s_bufWrElemCnt%s;\n", bufPtrW, strmName.c_str(), strmIdDecl.c_str());
			if (pStrm->m_strmCnt.size() == 0) {
				strmPreInstr.Append("\tc_wrStrm%s_bufWrElemCnt = r_wrStrm%s_bufWrElemCnt;\n", strmName.c_str(), strmName.c_str());
				strmReg.Append("\tr_wrStrm%s_bufWrElemCnt = r_reset1x ? (ht_uint%d)0 : c_wrStrm%s_bufWrElemCnt;\n", strmName.c_str(), bufPtrW, strmName.c_str());
			} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
				strmPreInstr.Append("\tc_wrStrm%s_bufWrElemCnt[%d] = r_wrStrm%s_bufWrElemCnt[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				strmReg.Append("\tr_wrStrm%s_bufWrElemCnt[%d] = r_reset1x ? (ht_uint%d)0 : c_wrStrm%s_bufWrElemCnt[%d];\n", strmName.c_str(), i, bufPtrW, strmName.c_str(), i);
			}

			GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, VA("ht_uint%d", bufPtrW), VA("r_wrStrm%s_bufRdElemCnt", strmName.c_str()), strmIdVec);
			m_strmRegDecl.Append("\tht_uint%d c_wrStrm%s_bufRdElemCnt%s;\n", bufPtrW, strmName.c_str(), strmIdDecl.c_str());
			if (pStrm->m_strmCnt.size() == 0) {
				strmPreInstr.Append("\tc_wrStrm%s_bufRdElemCnt = r_wrStrm%s_bufRdElemCnt;\n", strmName.c_str(), strmName.c_str());
				strmReg.Append("\tr_wrStrm%s_bufRdElemCnt = c_wrStrm%s_bufRdElemCnt;\n", strmName.c_str(), strmName.c_str());
			} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
				strmPreInstr.Append("\tc_wrStrm%s_bufRdElemCnt[%d] = r_wrStrm%s_bufRdElemCnt[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				strmReg.Append("\tr_wrStrm%s_bufRdElemCnt[%d] = c_wrStrm%s_bufRdElemCnt[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
			}

			GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, VA("ht_uint%d", bufPtrW), VA("r_wrStrm%s_bufWrPtr", strmName.c_str()), strmIdVec);
			m_strmRegDecl.Append("\tht_uint%d c_wrStrm%s_bufWrPtr%s;\n", bufPtrW, strmName.c_str(), strmIdDecl.c_str());
			if (pStrm->m_strmCnt.size() == 0) {
				strmPreInstr.Append("\tc_wrStrm%s_bufWrPtr = r_wrStrm%s_bufWrPtr;\n", strmName.c_str(), strmName.c_str());
				strmReg.Append("\tr_wrStrm%s_bufWrPtr = r_reset1x ? (ht_uint%d)0 : c_wrStrm%s_bufWrPtr;\n", strmName.c_str(), bufPtrW, strmName.c_str());
			} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
				strmPreInstr.Append("\tc_wrStrm%s_bufWrPtr[%d] = r_wrStrm%s_bufWrPtr[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				strmReg.Append("\tr_wrStrm%s_bufWrPtr[%d] = r_reset1x ? (ht_uint%d)0 : c_wrStrm%s_bufWrPtr[%d];\n", strmName.c_str(), i, bufPtrW, strmName.c_str(), i);
			}

			GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, VA("ht_uint%d", bufPtrW), VA("r_wrStrm%s_bufRdPtr", strmName.c_str()), strmIdVec);
			m_strmRegDecl.Append("\tht_uint%d c_wrStrm%s_bufRdPtr%s;\n", bufPtrW, strmName.c_str(), strmIdDecl.c_str());
			if (pStrm->m_strmCnt.size() == 0) {
				strmPreInstr.Append("\tc_wrStrm%s_bufRdPtr = r_wrStrm%s_bufRdPtr;\n", strmName.c_str(), strmName.c_str());
				strmReg.Append("\tr_wrStrm%s_bufRdPtr = r_reset1x ? (ht_uint%d)0 : c_wrStrm%s_bufRdPtr;\n", strmName.c_str(), bufPtrW, strmName.c_str());
			} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
				strmPreInstr.Append("\tc_wrStrm%s_bufRdPtr[%d] = r_wrStrm%s_bufRdPtr[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				strmReg.Append("\tr_wrStrm%s_bufRdPtr[%d] = r_reset1x ? (ht_uint%d)0 : c_wrStrm%s_bufRdPtr[%d];\n", strmName.c_str(), i, bufPtrW, strmName.c_str(), i);
			}

			GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, "bool", VA("r_wrStrm%s_bBufFull", strmName.c_str()), strmIdVec);
			m_strmRegDecl.Append("\tbool c_wrStrm%s_bBufFull%s;\n", strmName.c_str(), strmIdDecl.c_str());
			if (pStrm->m_strmCnt.size() == 0) {
				strmReg.Append("\tr_wrStrm%s_bBufFull = c_wrStrm%s_bBufFull;\n", strmName.c_str(), strmName.c_str());
			} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
				strmReg.Append("\tr_wrStrm%s_bBufFull[%d] = c_wrStrm%s_bBufFull[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
			}

			if (pStrm->m_reserve.AsInt() > 0) {
				GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, "bool", VA("r_wrStrm%s_bPipeQueFull", strmName.c_str()), strmIdVec);
				m_strmRegDecl.Append("\tbool c_wrStrm%s_bPipeQueFull%s;\n", strmName.c_str(), strmIdDecl.c_str());
				if (pStrm->m_strmCnt.size() == 0) {
					strmReg.Append("\tr_wrStrm%s_bPipeQueFull = c_wrStrm%s_bPipeQueFull;\n", strmName.c_str(), strmName.c_str());
				} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
					strmReg.Append("\tr_wrStrm%s_bPipeQueFull[%d] = c_wrStrm%s_bPipeQueFull[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				}
			}

			GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, "ht_uint3", VA("r_t3_wrStrm%s_reqQwRem", strmName.c_str()), strmIdVec);
			m_strmRegDecl.Append("\tht_uint3 c_t2_wrStrm%s_reqQwRem%s;\n", strmName.c_str(), strmIdDecl.c_str());
			if (pStrm->m_strmCnt.size() == 0) {
				strmPreInstr.Append("\tc_t2_wrStrm%s_reqQwRem = r_t3_wrStrm%s_reqQwRem;\n", strmName.c_str(), strmName.c_str());
				strmReg.Append("\tr_t3_wrStrm%s_reqQwRem = r_reset1x ? (ht_uint3)0 : c_t2_wrStrm%s_reqQwRem;\n", strmName.c_str(), strmName.c_str());
			} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
				strmPreInstr.Append("\tc_t2_wrStrm%s_reqQwRem[%d] = r_t3_wrStrm%s_reqQwRem[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				strmReg.Append("\tr_t3_wrStrm%s_reqQwRem[%d] = r_reset1x ? (ht_uint3)0 : c_t2_wrStrm%s_reqQwRem[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
			}

			int grpRspCntW = ((bIsHc2 || bIsWx) ? 7 : 10) + pStrm->m_strmCntW;
			GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, VA("ht_uint%d", grpRspCntW), VA("r_wrStrm%s_rspGrpCnt", strmName.c_str()), strmRspGrpVec);
			m_strmRegDecl.Append("\tht_uint%d c_wrStrm%s_rspGrpCnt%s;\n", grpRspCntW, strmName.c_str(), strmRspGrpDecl.c_str());
			if (pStrm->m_rspGrpW.AsInt() == 0) {
				strmPreInstr.Append("\tc_wrStrm%s_rspGrpCnt = r_wrStrm%s_rspGrpCnt;\n", strmName.c_str(), strmName.c_str());
				strmReg.Append("\tr_wrStrm%s_rspGrpCnt = r_reset1x ? (ht_uint%d)0 : c_wrStrm%s_rspGrpCnt;\n", strmName.c_str(), grpRspCntW, strmName.c_str());
			} else for (int i = 0; i < strmRspGrpCnt; i += 1) {
				strmPreInstr.Append("\tc_wrStrm%s_rspGrpCnt[%d] = r_wrStrm%s_rspGrpCnt[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				strmReg.Append("\tr_wrStrm%s_rspGrpCnt[%d] = r_reset1x ? (ht_uint%d)0 : c_wrStrm%s_rspGrpCnt[%d];\n", strmName.c_str(), i, grpRspCntW, strmName.c_str(), i);
			}

			GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, "bool", VA("r_wrStrm%s_bRspGrpCntFull", strmName.c_str()), strmRspGrpVec);
			m_strmRegDecl.Append("\tbool c_wrStrm%s_bRspGrpCntFull%s;\n", strmName.c_str(), strmRspGrpDecl.c_str());
			if (pStrm->m_rspGrpW.AsInt() == 0) {
				strmReg.Append("\tr_wrStrm%s_bRspGrpCntFull = c_wrStrm%s_bRspGrpCntFull;\n", strmName.c_str(), strmName.c_str());
			} else for (int i = 0; i < strmRspGrpCnt; i += 1) {
				strmReg.Append("\tr_wrStrm%s_bRspGrpCntFull[%d] = c_wrStrm%s_bRspGrpCntFull[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
			}

			if (pStrm->m_reserve.AsInt() == 0)
				GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, "bool", VA("r_wrStrm%s_bufWrEn", strmName.c_str()), strmIdVec);
			m_strmRegDecl.Append("\tbool c_wrStrm%s_bufWrEn%s;\n", strmName.c_str(), strmIdDecl.c_str());
			if (pStrm->m_strmCnt.size() == 0) {
				strmPreInstr.Append("\tc_wrStrm%s_bufWrEn = false;\n", strmName.c_str());
				if (pStrm->m_reserve.AsInt() == 0)
					strmReg.Append("\tr_wrStrm%s_bufWrEn = c_wrStrm%s_bufWrEn;\n", strmName.c_str(), strmName.c_str());
			} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
				strmPreInstr.Append("\tc_wrStrm%s_bufWrEn[%d] = false;\n", strmName.c_str(), i);
				if (pStrm->m_reserve.AsInt() == 0)
					strmReg.Append("\tr_wrStrm%s_bufWrEn[%d] = c_wrStrm%s_bufWrEn[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
			}

			if (pStrm->m_reserve.AsInt() > 0) {
				GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, "bool", VA("r_wrStrm%s_strmWrEn", strmName.c_str()), strmIdVec);
				m_strmRegDecl.Append("\tbool c_wrStrm%s_strmWrEn%s;\n", strmName.c_str(), strmIdDecl.c_str());
				if (pStrm->m_strmCnt.size() == 0) {
					strmPreInstr.Append("\tc_wrStrm%s_strmWrEn = false;\n", strmName.c_str());
					strmReg.Append("\tr_wrStrm%s_strmWrEn = c_wrStrm%s_strmWrEn;\n", strmName.c_str(), strmName.c_str());
				} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
					strmPreInstr.Append("\tc_wrStrm%s_strmWrEn[%d] = false;\n", strmName.c_str(), i);
					strmReg.Append("\tr_wrStrm%s_strmWrEn[%d] = c_wrStrm%s_strmWrEn[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				}
			}

			if (pStrm->m_reserve.AsInt() > 0) {
				if (pStrm->m_bClose) {
					GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, VA("CWrStrm%s_pipeQue", strmName.c_str()), VA("r_wrStrm%s_strmWrData", strmName.c_str()), strmIdVec);
					m_strmRegDecl.Append("\tCWrStrm%s_pipeQue c_wrStrm%s_strmWrData%s;\n", strmName.c_str(), strmName.c_str(), strmIdDecl.c_str());
					if (pStrm->m_strmCnt.size() == 0) {
						strmPreInstr.Append("\tc_wrStrm%s_strmWrData = 0;\n", strmName.c_str());
						strmReg.Append("\tr_wrStrm%s_strmWrData = c_wrStrm%s_strmWrData;\n", strmName.c_str(), strmName.c_str());
					} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
						strmPreInstr.Append("\tc_wrStrm%s_strmWrData[%d] = 0;\n", strmName.c_str(), i);
						strmReg.Append("\tr_wrStrm%s_strmWrData[%d] = c_wrStrm%s_strmWrData[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
					}
				} else {
					GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, pStrm->m_pType->m_typeName, VA("r_wrStrm%s_strmWrData", strmName.c_str()), strmIdVec);
					m_strmRegDecl.Append("\t%s c_wrStrm%s_strmWrData%s;\n", pStrm->m_pType->m_typeName.c_str(), strmName.c_str(), strmIdDecl.c_str());
					if (pStrm->m_strmCnt.size() == 0) {
						strmPreInstr.Append("\tc_wrStrm%s_strmWrData = 0;\n", strmName.c_str());
						strmReg.Append("\tr_wrStrm%s_strmWrData = c_wrStrm%s_strmWrData;\n", strmName.c_str(), strmName.c_str());
					} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
						strmPreInstr.Append("\tc_wrStrm%s_strmWrData[%d] = 0;\n", strmName.c_str(), i);
						strmReg.Append("\tr_wrStrm%s_strmWrData[%d] = c_wrStrm%s_strmWrData[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
					}
				}
			} else {
				GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, pStrm->m_pType->m_typeName, VA("r_wrStrm%s_bufWrData", strmName.c_str()), strmIdVec);
				m_strmRegDecl.Append("\t%s c_wrStrm%s_bufWrData%s;\n", pStrm->m_pType->m_typeName.c_str(), strmName.c_str(), strmIdDecl.c_str());
				if (pStrm->m_strmCnt.size() == 0) {
					strmPreInstr.Append("\tc_wrStrm%s_bufWrData = 0;\n", strmName.c_str());
					strmReg.Append("\tr_wrStrm%s_bufWrData = c_wrStrm%s_bufWrData;\n", strmName.c_str(), strmName.c_str());
				} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
					strmPreInstr.Append("\tc_wrStrm%s_bufWrData[%d] = 0;\n", strmName.c_str(), i);
					strmReg.Append("\tr_wrStrm%s_bufWrData[%d] = c_wrStrm%s_bufWrData[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				}
			}

			if (pStrm->m_reserve.AsInt() > 0) {
				GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, "bool", VA("r_wrStrm%s_pipeQueEmpty", strmName.c_str()), strmIdVec);
				m_strmRegDecl.Append("\tbool c_wrStrm%s_pipeQueEmpty%s;\n", strmName.c_str(), strmIdDecl.c_str());
				if (pStrm->m_strmCnt.size() == 0) {
					strmPreInstr.Append("\tc_wrStrm%s_pipeQueEmpty = r_wrStrm%s_pipeQueEmpty;\n", strmName.c_str(), strmName.c_str());
					strmReg.Append("\tr_wrStrm%s_pipeQueEmpty = r_reset1x || c_wrStrm%s_pipeQueEmpty;\n", strmName.c_str(), strmName.c_str());
				} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
					strmPreInstr.Append("\tc_wrStrm%s_pipeQueEmpty[%d] = r_wrStrm%s_pipeQueEmpty[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
					strmReg.Append("\tr_wrStrm%s_pipeQueEmpty[%d] = r_reset1x || c_wrStrm%s_pipeQueEmpty[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				}
			}

			if (pStrm->m_reserve.AsInt() > 0) {
				if (pStrm->m_bClose) {
					GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, VA("CWrStrm%s_pipeQue", strmName.c_str()), VA("r_wrStrm%s_pipeQueFront", strmName.c_str()), strmIdVec);
					m_strmRegDecl.Append("\tCWrStrm%s_pipeQue c_wrStrm%s_pipeQueFront%s;\n", strmName.c_str(), strmName.c_str(), strmIdDecl.c_str());
					if (pStrm->m_strmCnt.size() == 0) {
						strmPreInstr.Append("\tc_wrStrm%s_pipeQueFront = r_wrStrm%s_pipeQueFront;\n", strmName.c_str(), strmName.c_str());
						strmReg.Append("\tr_wrStrm%s_pipeQueFront = c_wrStrm%s_pipeQueFront;\n", strmName.c_str(), strmName.c_str());
					} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
						strmPreInstr.Append("\tc_wrStrm%s_pipeQueFront[%d] = r_wrStrm%s_pipeQueFront[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
						strmReg.Append("\tr_wrStrm%s_pipeQueFront[%d] = c_wrStrm%s_pipeQueFront[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
					}
				} else {
					GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, pStrm->m_pType->m_typeName.c_str(), VA("r_wrStrm%s_pipeQueFront", strmName.c_str()), strmIdVec);
					m_strmRegDecl.Append("\t%s c_wrStrm%s_pipeQueFront%s;\n", pStrm->m_pType->m_typeName.c_str(), strmName.c_str(), strmIdDecl.c_str());
					if (pStrm->m_strmCnt.size() == 0) {
						strmPreInstr.Append("\tc_wrStrm%s_pipeQueFront = r_wrStrm%s_pipeQueFront;\n", strmName.c_str(), strmName.c_str());
						strmReg.Append("\tr_wrStrm%s_pipeQueFront = c_wrStrm%s_pipeQueFront;\n", strmName.c_str(), strmName.c_str());
					} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
						strmPreInstr.Append("\tc_wrStrm%s_pipeQueFront[%d] = r_wrStrm%s_pipeQueFront[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
						strmReg.Append("\tr_wrStrm%s_pipeQueFront[%d] = c_wrStrm%s_pipeQueFront[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
					}
				}
			}

			if (mod.m_clkRate == eClk1x) {
				GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, "bool", VA("r_wrStrm%s_bCollision", strmName.c_str()), strmIdVec);
				m_strmRegDecl.Append("\tbool c_wrStrm%s_bCollision%s;\n", strmName.c_str(), strmIdDecl.c_str());
				if (pStrm->m_strmCnt.size() == 0) {
					strmPreInstr.Append("\tc_wrStrm%s_bCollision = false;\n", strmName.c_str());
					strmReg.Append("\tr_wrStrm%s_bCollision = !r_reset1x && c_wrStrm%s_bCollision;\n", strmName.c_str(), strmName.c_str());
				} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
					strmPreInstr.Append("\tc_wrStrm%s_bCollision[%d] = false;\n", strmName.c_str(), i);
					strmReg.Append("\tr_wrStrm%s_bCollision[%d] = !r_reset1x && c_wrStrm%s_bCollision[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				}
			}

			m_strmRegDecl.Append("\tbool c_wrStrm%s_bReqRdy%s;\n", strmName.c_str(), strmIdDecl.c_str());

			if (mod.m_clkRate == eClk2x) {
				GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, "bool", VA("r_t2_wrStrm%s_bReqSel", strmName.c_str()), strmIdVec);
				m_strmRegDecl.Append("\tbool c_t1_wrStrm%s_bReqSel%s;\n", strmName.c_str(), strmIdDecl.c_str());
				if (pStrm->m_strmCnt.size() == 0) {
					strmReg.Append("\tr_t2_wrStrm%s_bReqSel = c_t1_wrStrm%s_bReqSel;\n", strmName.c_str(), strmName.c_str());
				} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
					strmReg.Append("\tr_t2_wrStrm%s_bReqSel[%d] = c_t1_wrStrm%s_bReqSel[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				}
			} else
				m_strmRegDecl.Append("\tbool c_t1_wrStrm%s_bReqSel%s;\n", strmName.c_str(), strmIdDecl.c_str());

			if (mod.m_clkRate == eClk2x) {
				GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, "bool", VA("r_t3_wrStrm%s_bReqSel", strmName.c_str()), strmIdVec);
				m_strmRegDecl.Append("\tbool c_t2_wrStrm%s_bReqSel%s;\n", strmName.c_str(), strmIdDecl.c_str());
				if (pStrm->m_strmCnt.size() == 0) {
					strmPreInstr.Append("\tc_t2_wrStrm%s_bReqSel = false;\n", strmName.c_str());
					strmReg.Append("\tr_t3_wrStrm%s_bReqSel = c_t2_wrStrm%s_bReqSel;\n", strmName.c_str(), strmName.c_str());
				} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
					strmPreInstr.Append("\tc_t2_wrStrm%s_bReqSel[%d] = false;\n", strmName.c_str(), i);
					strmReg.Append("\tr_t3_wrStrm%s_bReqSel[%d] = c_t2_wrStrm%s_bReqSel[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				}
			} else {
				m_strmRegDecl.Append("\tbool c_t2_wrStrm%s_bReqSel%s;\n", strmName.c_str(), strmIdDecl.c_str());
				if (pStrm->m_strmCnt.size() == 0) {
					strmPreInstr.Append("\tc_t2_wrStrm%s_bReqSel = false;\n", strmName.c_str());
				} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
					strmPreInstr.Append("\tc_t2_wrStrm%s_bReqSel[%d] = false;\n", strmName.c_str(), i);
				}
			}

			if (mod.m_clkRate == eClk2x) {
				GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, "bool", VA("r_t4_wrStrm%s_bReqSel", strmName.c_str()), strmIdVec);
				m_strmRegDecl.Append("\tbool c_t3_wrStrm%s_bReqSel%s;\n", strmName.c_str(), strmIdDecl.c_str());
				if (pStrm->m_strmCnt.size() == 0) {
					strmPreInstr.Append("\tc_t3_wrStrm%s_bReqSel = r_t3_wrStrm%s_bReqSel;\n", strmName.c_str(), strmName.c_str());
					strmReg.Append("\tr_t4_wrStrm%s_bReqSel = c_t3_wrStrm%s_bReqSel;\n", strmName.c_str(), strmName.c_str());
				} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
					strmPreInstr.Append("\tc_t3_wrStrm%s_bReqSel[%d] = r_t3_wrStrm%s_bReqSel[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
					strmReg.Append("\tr_t4_wrStrm%s_bReqSel[%d] = c_t3_wrStrm%s_bReqSel[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				}
			}

			if (bMultiQwReq) {
				if (mod.m_clkRate == eClk2x) {
					GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, "bool", VA("r_t2_wrStrm%s_bReqQwRem", strmName.c_str()), strmIdVec);
					m_strmRegDecl.Append("\tbool c_t1_wrStrm%s_bReqQwRem%s;\n", strmName.c_str(), strmIdDecl.c_str());
					if (pStrm->m_strmCnt.size() == 0) {
						strmReg.Append("\tr_t2_wrStrm%s_bReqQwRem = c_t1_wrStrm%s_bReqQwRem;\n", strmName.c_str(), strmName.c_str());
					} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
						strmReg.Append("\tr_t2_wrStrm%s_bReqQwRem[%d] = c_t1_wrStrm%s_bReqQwRem[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
					}
				} else
					m_strmRegDecl.Append("\tbool c_t1_wrStrm%s_bReqQwRem%s;\n", strmName.c_str(), strmIdDecl.c_str());
			}

			if (pStrm->m_elemByteW == 0) {
				if (mod.m_clkRate == eClk2x) {
					GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, "bool", VA("r_t2_wrStrm%s_bReqUint8", strmName.c_str()), strmIdVec);
					m_strmRegDecl.Append("\tbool c_t1_wrStrm%s_bReqUint8%s;\n", strmName.c_str(), strmIdDecl.c_str());
					if (pStrm->m_strmCnt.size() == 0) {
						strmReg.Append("\tr_t2_wrStrm%s_bReqUint8 = c_t1_wrStrm%s_bReqUint8;\n", strmName.c_str(), strmName.c_str());
					} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
						strmReg.Append("\tr_t2_wrStrm%s_bReqUint8[%d] = c_t1_wrStrm%s_bReqUint8[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
					}
				} else
					m_strmRegDecl.Append("\tbool c_t1_wrStrm%s_bReqUint8%s;\n", strmName.c_str(), strmIdDecl.c_str());
			}

			if (pStrm->m_elemByteW <= 1) {
				if (mod.m_clkRate == eClk2x) {
					GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, "bool", VA("r_t2_wrStrm%s_bReqUint16", strmName.c_str()), strmIdVec);
					m_strmRegDecl.Append("\tbool c_t1_wrStrm%s_bReqUint16%s;\n", strmName.c_str(), strmIdDecl.c_str());
					if (pStrm->m_strmCnt.size() == 0) {
						strmReg.Append("\tr_t2_wrStrm%s_bReqUint16 = c_t1_wrStrm%s_bReqUint16;\n", strmName.c_str(), strmName.c_str());
					} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
						strmReg.Append("\tr_t2_wrStrm%s_bReqUint16[%d] = c_t1_wrStrm%s_bReqUint16[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
					}
				} else
					m_strmRegDecl.Append("\tbool c_t1_wrStrm%s_bReqUint16%s;\n", strmName.c_str(), strmIdDecl.c_str());
			}

			if (pStrm->m_elemByteW <= 2) {
				if (mod.m_clkRate == eClk2x) {
					GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, "bool", VA("r_t2_wrStrm%s_bReqUint32", strmName.c_str()), strmIdVec);
					m_strmRegDecl.Append("\tbool c_t1_wrStrm%s_bReqUint32%s;\n", strmName.c_str(), strmIdDecl.c_str());
					if (pStrm->m_strmCnt.size() == 0) {
						strmReg.Append("\tr_t2_wrStrm%s_bReqUint32 = c_t1_wrStrm%s_bReqUint32;\n", strmName.c_str(), strmName.c_str());
					} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
						strmReg.Append("\tr_t2_wrStrm%s_bReqUint32[%d] = c_t1_wrStrm%s_bReqUint32[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
					}
				} else
					m_strmRegDecl.Append("\tbool c_t1_wrStrm%s_bReqUint32%s;\n", strmName.c_str(), strmIdDecl.c_str());
			}

			int reqSelW;
			switch (pStrm->m_elemBitW) {
			default: assert(0);
			case 8: reqSelW = 3; break;
			case 16: reqSelW = 2; break;
			case 32: reqSelW = 2; break;
			case 64: reqSelW = 1; break;
			}

			if (mod.m_clkRate == eClk2x) {
				GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, VA("ht_uint%d", reqSelW), VA("r_t3_wrStrm%s_reqSel", strmName.c_str()), strmIdVec);
				m_strmRegDecl.Append("\tht_uint%d c_t2_wrStrm%s_reqSel%s;\n", reqSelW, strmName.c_str(), strmIdDecl.c_str());
				if (pStrm->m_strmCnt.size() == 0) {
					strmPreInstr.Append("\tc_t2_wrStrm%s_reqSel = 0;\n", strmName.c_str());
					strmReg.Append("\tr_t3_wrStrm%s_reqSel = c_t2_wrStrm%s_reqSel;\n", strmName.c_str(), strmName.c_str());
				} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
					strmPreInstr.Append("\tc_t2_wrStrm%s_reqSel[%d] = 0;\n", strmName.c_str(), i);
					strmReg.Append("\tr_t3_wrStrm%s_reqSel[%d] = c_t2_wrStrm%s_reqSel[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				}
			} else {
				m_strmRegDecl.Append("\tht_uint%d c_t2_wrStrm%s_reqSel%s;\n", reqSelW, strmName.c_str(), strmIdDecl.c_str());
				if (pStrm->m_strmCnt.size() == 0) {
					strmPreInstr.Append("\tc_t2_wrStrm%s_reqSel = 0;\n", strmName.c_str());
				} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
					strmPreInstr.Append("\tc_t2_wrStrm%s_reqSel[%d] = 0;\n", strmName.c_str(), i);
				}
			}

			if (mod.m_clkRate == eClk2x) {
				GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, VA("ht_uint%d", reqSelW), VA("r_t4_wrStrm%s_reqSel", strmName.c_str()), strmIdVec);
				m_strmRegDecl.Append("\tht_uint%d c_t3_wrStrm%s_reqSel%s;\n", reqSelW, strmName.c_str(), strmIdDecl.c_str());
				if (pStrm->m_strmCnt.size() == 0) {
					strmPreInstr.Append("\tc_t3_wrStrm%s_reqSel = r_t3_wrStrm%s_reqSel;\n", strmName.c_str(), strmName.c_str());
					strmReg.Append("\tr_t4_wrStrm%s_reqSel = c_t3_wrStrm%s_reqSel;\n", strmName.c_str(), strmName.c_str());
				} else for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
					strmPreInstr.Append("\tc_t3_wrStrm%s_reqSel[%d] = r_t3_wrStrm%s_reqSel[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
					strmReg.Append("\tr_t4_wrStrm%s_reqSel[%d] = c_t3_wrStrm%s_reqSel[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
				}
			}

			int rdSelCnt, rdSelW;
			switch (pStrm->m_elemBitW) {
			default: assert(0);
			case 8: rdSelCnt = 4; rdSelW = 3; break;
			case 16: rdSelCnt = 2; rdSelW = 2; break;
			case 32: rdSelCnt = 1; rdSelW = 1; break;
			case 64: rdSelCnt = 0; rdSelW = 0; break;
			}

			if (mod.m_clkRate == eClk2x) {
				for (int i = 0; i < rdSelCnt; i += 1) {
					GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, VA("ht_uint%d", rdSelW), VA("r_t3_wrStrm%s_rdSel%d", strmName.c_str(), i), strmIdVec);
					m_strmRegDecl.Append("\tht_uint%d c_t2_wrStrm%s_rdSel%d%s;\n", rdSelW, strmName.c_str(), i, strmIdDecl.c_str());
					if (pStrm->m_strmCnt.size() == 0) {
						strmPreInstr.Append("\tc_t2_wrStrm%s_rdSel%d = 0;\n", strmName.c_str(), i);
						strmReg.Append("\tr_t3_wrStrm%s_rdSel%d = c_t2_wrStrm%s_rdSel%d;\n", strmName.c_str(), i, strmName.c_str(), i);
					} else for (int j = 0; j < pStrm->m_strmCnt.AsInt(); j += 1) {
						strmPreInstr.Append("\tc_t2_wrStrm%s_rdSel%d[%d] = 0;\n", strmName.c_str(), i, j);
						strmReg.Append("\tr_t3_wrStrm%s_rdSel%d[%d] = c_t2_wrStrm%s_rdSel%d[%d];\n", strmName.c_str(), i, j, strmName.c_str(), i, j);
					}
				}
			} else {
				for (int i = 0; i < rdSelCnt; i += 1) {
					m_strmRegDecl.Append("\tht_uint%d c_t2_wrStrm%s_rdSel%d%s;\n", rdSelW, strmName.c_str(), i, strmIdDecl.c_str());
					if (pStrm->m_strmCnt.size() == 0) {
						strmPreInstr.Append("\tc_t2_wrStrm%s_rdSel%d = 0;\n", strmName.c_str(), i);
					} else for (int j = 0; j < pStrm->m_strmCnt.AsInt(); j += 1) {
						strmPreInstr.Append("\tc_t2_wrStrm%s_rdSel%d[%d] = 0;\n", strmName.c_str(), i, j);
					}
				}
			}

			if (mod.m_clkRate == eClk2x) {
				for (int i = 0; i < rdSelCnt; i += 1) {
					GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, VA("ht_uint%d", rdSelW), VA("r_t4_wrStrm%s_rdSel%d", strmName.c_str(), i), strmIdVec);
					m_strmRegDecl.Append("\tht_uint%d c_t3_wrStrm%s_rdSel%d%s;\n", rdSelW, strmName.c_str(), i, strmIdDecl.c_str());
					if (pStrm->m_strmCnt.size() == 0) {
						strmPreInstr.Append("\tc_t3_wrStrm%s_rdSel%d = r_t3_wrStrm%s_rdSel%d;\n", strmName.c_str(), i, strmName.c_str(), i);
						strmReg.Append("\tr_t4_wrStrm%s_rdSel%d = c_t3_wrStrm%s_rdSel%d;\n", strmName.c_str(), i, strmName.c_str(), i);
					} else for (int j = 0; j < pStrm->m_strmCnt.AsInt(); j += 1) {
						strmPreInstr.Append("\tc_t3_wrStrm%s_rdSel%d[%d] = r_t3_wrStrm%s_rdSel%d[%d];\n", strmName.c_str(), i, j, strmName.c_str(), i, j);
						strmReg.Append("\tr_t4_wrStrm%s_rdSel%d[%d] = c_t3_wrStrm%s_rdSel%d[%d];\n", strmName.c_str(), i, j, strmName.c_str(), i, j);
					}
				}
			}

			if (mod.m_clkRate == eClk2x) {
				vector<CHtString> strmBufIdVec;
				if (pStrm->m_strmCnt.size() > 0) {
					CHtString strmIdDimen;
					strmIdDimen.SetValue(pStrm->m_strmCnt.AsInt());
					strmBufIdVec.push_back(strmIdDimen);
				}

				int qwElemCnt = 64 / pStrm->m_elemBitW;
				CHtString elemCntDimen;
				elemCntDimen.SetValue(qwElemCnt);
				strmBufIdVec.push_back(elemCntDimen);

				GenModDecl(eVcdAll, m_strmRegDecl, vcdModName, pStrm->m_pType->m_typeName.c_str(), VA("r_t4_wrStrm%s_buf", strmName.c_str()), strmBufIdVec);
				m_strmRegDecl.Append("\t%s c_t3_wrStrm%s_buf%s[%d];\n", pStrm->m_pType->m_typeName.c_str(), strmName.c_str(), strmIdDecl.c_str(), qwElemCnt);
				for (int i = 0; i < qwElemCnt; i += 1) {
					string elemIdxStr = qwElemCnt == 1 ? "" : VA("%d", i);
					if (pStrm->m_strmCnt.size() == 0) {
						strmPreInstr.Append("\tc_t3_wrStrm%s_buf[%d] = m_wrStrm%s_buf.read_mem(%s);\n", strmName.c_str(), i, strmName.c_str(), elemIdxStr.c_str());
						strmReg.Append("\tr_t4_wrStrm%s_buf[%d] = c_t3_wrStrm%s_buf[%d];\n", strmName.c_str(), i, strmName.c_str(), i);
					} else for (int j = 0; j < pStrm->m_strmCnt.AsInt(); j += 1) {
						strmPreInstr.Append("\tc_t3_wrStrm%s_buf[%d][%d] = m_wrStrm%s_buf[%d].read_mem(%s);\n", strmName.c_str(), j, i, strmName.c_str(), j, elemIdxStr.c_str());
						strmReg.Append("\tr_t4_wrStrm%s_buf[%d][%d] = c_t3_wrStrm%s_buf[%d][%d];\n", strmName.c_str(), j, i, strmName.c_str(), j, i);
					}
				}
			}

			if (pStrm->m_elemByteW == 3)
				m_strmRegDecl.Append("\tht_block_ram<%s, %d> m_wrStrm%s_buf%s;\n", pStrm->m_pType->m_typeName.c_str(), bufPtrW - 1, strmName.c_str(), strmIdDecl.c_str());
			else
				m_strmRegDecl.Append("\tht_mrd_block_ram<%s, %d, %d> m_wrStrm%s_buf%s;\n",
				pStrm->m_pType->m_typeName.c_str(), 3 - pStrm->m_elemByteW, bufPtrW - (3 - pStrm->m_elemByteW) - 1, strmName.c_str(), strmIdDecl.c_str());

			for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
				string strmIdx = pStrm->m_strmCnt.size() == 0 ? "" : VA("[%d]", i);

				strmReg.Append("\tm_wrStrm%s_buf%s.clock();\n", strmName.c_str(), strmIdx.c_str());
			}
			if (mod.m_threads.m_htIdW.AsInt() > 0 && mod.m_rsmSrcCnt > 1) {
				m_strmRegDecl.Append("\tht_dist_que<CHtCmd, %s_HTID_W> m_wrStrm%s_rsmQue;\n", mod.m_modName.Upper().c_str(), strmName.c_str());
				strmReg.Append("\tm_wrStrm%s_rsmQue.clock(%s);\n", strmName.c_str(), reset.c_str());
			}

			if (pStrm->m_reserve.AsInt() > 0) {
				if (pStrm->m_bClose)
					m_strmRegDecl.Append("\tht_dist_que<CWrStrm%s_pipeQue, %d> m_wrStrm%s_pipeQue%s;\n",
					strmName.c_str(), FindLg2(pStrm->m_reserve.AsInt() + 6),
					strmName.c_str(), strmIdDecl.c_str());
				else
					m_strmRegDecl.Append("\tht_dist_que<%s, %d> m_wrStrm%s_pipeQue%s;\n",
					pStrm->m_pType->m_typeName.c_str(), FindLg2(pStrm->m_reserve.AsInt() + 6),
					strmName.c_str(), strmIdDecl.c_str());

				for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1) {
					string strmIdx = pStrm->m_strmCnt.size() == 0 ? "" : VA("[%d]", i);

					strmReg.Append("\tm_wrStrm%s_pipeQue%s.clock(%s);\n", strmName.c_str(), strmIdx.c_str(), reset.c_str());
				}
			}

			strmPostInstr.Append("\t// wrStrm%s\n", strmName.c_str());
			strmPostInstr.Append("\tunion CWrStrm%s_wrTid {\n", strmName.c_str());
			strmPostInstr.Append("\t\tstruct {\n");
			strmPostInstr.Append("\t\t\tuint32_t m_mifStrmId : 4;\n");
			if (pStrm->m_rspGrpW.AsInt() > 0)
				strmPostInstr.Append("\t\t\tuint32_t m_rspGrpId : %d;\n", pStrm->m_rspGrpW.AsInt());
			strmPostInstr.Append("\t\t\tuint32_t m_pad1 : %d;\n", 25 - pStrm->m_rspGrpW.AsInt());
			strmPostInstr.Append("\t\t\tuint32_t m_reqQwCntM1 : 3;\n");
			strmPostInstr.Append("\t\t};\n");
			strmPostInstr.Append("\t\tuint32_t m_tid;\n");
			strmPostInstr.Append("\t};\n");
			strmPostInstr.Append("\n");

			//if (pStrm->m_strmCnt.size() == 0)
			//	strmPostInstr.Append("\tif (r_wrStrm%s_bPaused && !r_wrStrm%s_bOpenBufWr && !r_wrStrm%s_bOpenBufRd && r_wrStrm%s_wrRspCnt == 0) {\n", 
			//		strmName.c_str(), strmName.c_str(), strmName.c_str(), strmName.c_str());
			//else {
			//	strmPostInstr.Append("\tif (r_wrStrm%s_bPaused", strmName.c_str());
			//	for (int i = 0; i < pStrm->m_strmCnt.AsInt(); i += 1)
			//		strmPostInstr.Append("\n\t\t&& (r_wrStrm%s_pauseMask[%d] == 0 || !r_wrStrm%s_bOpenBufWr[%d] && !r_wrStrm%s_bOpenBufRd[%d] && r_wrStrm%s_wrRspCnt[%d] == 0)", 
			//			strmName.c_str(), i, strmName.c_str(), i, strmName.c_str(), i, strmName.c_str(), i);
			//	strmPostInstr.Append(")\n");
			//	strmPostInstr.Append("\t{\n");
			//}

			if (pStrm->m_rspGrpW.AsInt() > 0) {
				strmPostInstr.Append("\tbool c_wrStrm%s_bRspGrpRsm = false;\n", strmName.c_str());
				strmPostInstr.Append("\tht_uint%d c_wrStrm%s_rsmRspGrpId = 0;\n", pStrm->m_rspGrpW.AsInt(), strmName.c_str());
				strmPostInstr.Append("\tfor (int rspGrpId = 0; rspGrpId < %d; rspGrpId += 1) {\n", 1 << pStrm->m_rspGrpW.AsInt());
				strmPostInstr.Append("\t\tif (!c_wrStrm%s_bRspGrpRsm && r_wrStrm%s_bPaused[rspGrpId] && r_wrStrm%s_rspGrpCnt[rspGrpId] == 0) {\n",
					strmName.c_str(), strmName.c_str(), strmName.c_str());
				strmPostInstr.Append("\t\t\tc_wrStrm%s_bRspGrpRsm = true;\n", strmName.c_str());
				strmPostInstr.Append("\t\t\tc_wrStrm%s_rsmRspGrpId = rspGrpId;\n", strmName.c_str());
				strmPostInstr.Append("\t\t}\n");
				strmPostInstr.Append("\n");

				strmPostInstr.Append("\t\tc_wrStrm%s_bRspGrpCntFull[rspGrpId] = r_wrStrm%s_rspGrpCnt[rspGrpId] > %d;\n",
					strmName.c_str(), strmName.c_str(), (1 << (((bIsHc2 || bIsWx) ? 7 : 10) + pStrm->m_strmCntW)) - (2 << pStrm->m_strmCntW) - 3);
				strmPostInstr.Append("\t}\n");
			} else {
				strmPostInstr.Append("\tbool c_wrStrm%s_bRspGrpRsm = r_wrStrm%s_bPaused && r_wrStrm%s_rspGrpCnt == 0;\n",
					strmName.c_str(), strmName.c_str(), strmName.c_str());
				strmPostInstr.Append("\tc_wrStrm%s_bRspGrpCntFull = r_wrStrm%s_rspGrpCnt > %d;\n",
					strmName.c_str(), strmName.c_str(), ((bIsHc2 || bIsWx) ? 128 : 1024) - (1 << pStrm->m_rspGrpW.AsInt()) - 1);
			}
			strmPostInstr.Append("\n");

			string rsmRspGrpName;
			string rsmRspGrpIdx;
			if (pStrm->m_rspGrpW.AsInt() > 0) {
				rsmRspGrpName = VA("c_wrStrm%s_rsmRspGrpId", strmName.c_str());
				rsmRspGrpIdx = "[" + rsmRspGrpName + "]";
			}

			if (pStrm->m_rspGrpW.AsInt() > 0) {
				if (mod.m_threads.m_htIdW.AsInt() > 0 && pStrm->m_rspGrpW.size() > 0)
					strmPostInstr.Append("\tm_wrStrm%s_rsmHtId.read_addr(c_wrStrm%s_rsmRspGrpId);\n", strmName.c_str(), strmName.c_str());
				strmPostInstr.Append("\tm_wrStrm%s_rsmHtInstr.read_addr(c_wrStrm%s_rsmRspGrpId);\n", strmName.c_str(), strmName.c_str());
				strmPostInstr.Append("\n");
			}

			strmPostInstr.Append("\tif (c_wrStrm%s_bRspGrpRsm) {\n", strmName.c_str());
			if (mod.m_threads.m_htIdW.AsInt() > 0) {
				if (mod.m_rsmSrcCnt > 1) {
					strmPostInstr.Append("\t\tCHtCmd rsm;\n");
					if (pStrm->m_rspGrpW.size() == 0)
						strmPostInstr.Append("\t\trsm.m_htId = %s;\n", rsmRspGrpName.c_str());
					else {
						if (pStrm->m_rspGrpW.AsInt() == 0)
							strmPostInstr.Append("\t\trsm.m_htId = r_wrStrm%s_rsmHtId;\n", strmName.c_str());
						else
							strmPostInstr.Append("\t\trsm.m_htId = m_wrStrm%s_rsmHtId.read_mem();\n", strmName.c_str());
					}

					if (pStrm->m_rspGrpW.AsInt() == 0)
						strmPostInstr.Append("\t\trsm.m_htInstr = r_wrStrm%s_rsmHtInstr;\n", strmName.c_str());
					else
						strmPostInstr.Append("\t\trsm.m_htInstr = m_wrStrm%s_rsmHtInstr.read_mem();\n", strmName.c_str());

					strmPostInstr.Append("\t\tm_wrStrm%s_rsmQue.push(rsm);\n", strmName.c_str());
					strmPostInstr.NewLine();
					strmPostInstr.Append("#\t\tifndef _HTV\n");
					strmPostInstr.Append("\t\tassert(%s || m_htRsmWait.read_mem_debug(rsm.m_htId) == true);\n", reset.c_str());
					strmPostInstr.Append("\t\tm_htRsmWait.write_mem_debug(rsm.m_htId) = false;\n");
					strmPostInstr.Append("#\t\tendif\n");
				} else {
					strmPostInstr.Append("\t\tc_t0_rsmHtRdy = true;\n");
					if (pStrm->m_rspGrpW.size() == 0)
						strmPostInstr.Append("\t\tc_t0_rsmHtId = %s;\n", rsmRspGrpName.c_str());
					else {
						if (pStrm->m_rspGrpW.AsInt() == 0)
							strmPostInstr.Append("\t\tc_t0_rsmHtId = r_wrStrm%s_rsmHtId;\n", strmName.c_str());
						else
							strmPostInstr.Append("\t\tc_t0_rsmHtId = m_wrStrm%s_rsmHtId.read_mem();\n", strmName.c_str());
					}

					if (pStrm->m_rspGrpW.AsInt() == 0)
						strmPostInstr.Append("\t\tc_t0_rsmHtInstr = r_wrStrm%s_rsmHtInstr;\n", strmName.c_str());
					else
						strmPostInstr.Append("\t\tc_t0_rsmHtInstr = m_wrStrm%s_rsmHtInstr.read_mem();\n", strmName.c_str());

					strmPostInstr.Append("#\t\tifndef _HTV\n");
					strmPostInstr.Append("\t\tassert(%s || m_htRsmWait.read_mem_debug(c_t0_rsmHtId) == true);\n", reset.c_str());
					strmPostInstr.Append("\t\tm_htRsmWait.write_mem_debug(c_t0_rsmHtId) = false;\n");
					strmPostInstr.Append("#\t\tendif\n");
				}
			} else {
				strmPostInstr.Append("\t\tc_t0_rsmHtRdy = true;\n");

				if (pStrm->m_rspGrpW.AsInt() == 0)
					strmPostInstr.Append("\t\tc_t0_rsmHtInstr = r_wrStrm%s_rsmHtInstr;\n", strmName.c_str());
				else
					strmPostInstr.Append("\t\tc_t0_rsmHtInstr = m_wrStrm%s_rsmHtInstr.read_mem();\n", strmName.c_str());

				strmPostInstr.Append("#\t\tifndef _HTV\n");
				strmPostInstr.Append("\t\tassert(%s || r_htRsmWait == true);\n", reset.c_str());
				strmPostInstr.Append("\t\tc_htRsmWait = false;\n");
				strmPostInstr.Append("#\t\tendif\n");
			}
			strmPostInstr.Append("\t\tc_wrStrm%s_bPaused%s = false;\n", strmName.c_str(), rsmRspGrpIdx.c_str());
			strmPostInstr.Append("\t}\n");
			strmPostInstr.Append("\n");

			if (mod.m_threads.m_htIdW.AsInt() > 0 && mod.m_rsmSrcCnt > 1) {
				strmPostInstr.Append("\tif (!m_wrStrm%s_rsmQue.empty() && !c_t0_rsmHtRdy) {\n", strmName.c_str());
				strmPostInstr.Append("\t\tc_t0_rsmHtRdy = true;\n");
				strmPostInstr.Append("\t\tc_t0_rsmHtId = m_wrStrm%s_rsmQue.front().m_htId;\n", strmName.c_str());
				strmPostInstr.Append("\t\tc_t0_rsmHtInstr = m_wrStrm%s_rsmQue.front().m_htInstr;\n", strmName.c_str());
				strmPostInstr.Append("\t\tm_wrStrm%s_rsmQue.pop();\n", strmName.c_str());
				strmPostInstr.Append("\t}\n");
				strmPostInstr.Append("\n");
			}

			{
				string strmIdxStr;
				if (pStrm->m_strmCnt.size() == 0)
					strmPostInstr.Append("\t{ // wrStrm%s\n", strmName.c_str());
				else {
					strmPostInstr.Append("\tfor (int idx = 0; idx < %d; idx += 1) { // wrStrm%s\n", pStrm->m_strmCnt.AsInt(), strmName.c_str());
					strmIdxStr = "[idx]";
				}

				if (pStrm->m_reserve.AsInt() == 0) {
					strmPostInstr.Append("\t\tif (r_wrStrm%s_bufWrEn%s) {\n", strmName.c_str(), strmIdxStr.c_str());
				} else {
					strmPostInstr.Append("\t\tif (r_wrStrm%s_strmWrEn%s)\n", strmName.c_str(), strmIdxStr.c_str());
					strmPostInstr.Append("\t\t\tm_wrStrm%s_pipeQue%s.push(r_wrStrm%s_strmWrData%s);\n",
						strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
					strmPostInstr.Append("\n");

					strmPostInstr.Append("\t\tc_wrStrm%s_bPipeQueFull%s = m_wrStrm%s_pipeQue%s.size() > %d;\n",
						strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), (1 << FindLg2(pStrm->m_reserve.AsInt() + 6)) - (3 + pStrm->m_reserve.AsInt()));
					strmPostInstr.Append("\n");

					strmPostInstr.Append("\t\tif (!r_wrStrm%s_pipeQueEmpty%s && r_wrStrm%s_bOpenBufWr%s && !r_wrStrm%s_bBufFull%s) {\n",
						strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
				}

				if (pStrm->m_elemByteW == 3)
					strmPostInstr.Append("\t\t\tm_wrStrm%s_buf%s.write_addr(r_wrStrm%s_bufWrPtr%s(%d,%d));\n",
					strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), bufPtrW - 2, 3 - pStrm->m_elemByteW);
				else
					strmPostInstr.Append("\t\t\tm_wrStrm%s_buf%s.write_addr(r_wrStrm%s_bufWrPtr%s(%d,0), r_wrStrm%s_bufWrPtr%s(%d,%d));\n",
					strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), 2 - pStrm->m_elemByteW,
					strmName.c_str(), strmIdxStr.c_str(), bufPtrW - 2, 3 - pStrm->m_elemByteW);

				string tabs;
				if (pStrm->m_reserve.AsInt() == 0) {
					strmPostInstr.Append("\t\t\tm_wrStrm%s_buf%s.write_mem(r_wrStrm%s_bufWrData%s);\n",
						strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
				} else {
					if (pStrm->m_bClose) {
						tabs = "\t";
						strmPostInstr.Append("\t\t\tif (r_wrStrm%s_pipeQueFront%s.m_bData) {\n",
							strmName.c_str(), strmIdxStr.c_str());
						strmPostInstr.Append("\t\t\t\tm_wrStrm%s_buf%s.write_mem(r_wrStrm%s_pipeQueFront%s.m_data);\n",
							strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
					} else {
						strmPostInstr.Append("\t\t\tm_wrStrm%s_buf%s.write_mem(r_wrStrm%s_pipeQueFront%s);\n",
							strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
					}
				}

				strmPostInstr.Append("%s\t\t\tc_wrStrm%s_bufWrPtr%s = r_wrStrm%s_bufWrPtr%s + 1;\n", tabs.c_str(),
					strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
				strmPostInstr.Append("%s\t\t\tif (r_wrStrm%s_bOpenBufRd%s)\n", tabs.c_str(), strmName.c_str(), strmIdxStr.c_str());
				strmPostInstr.Append("%s\t\t\t\tc_wrStrm%s_bufWrElemCnt%s = r_wrStrm%s_bufWrElemCnt%s + 1;\n", tabs.c_str(),
					strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
				strmPostInstr.Append("%s\t\t\telse\n", tabs.c_str());
				strmPostInstr.Append("%s\t\t\t\tc_wrStrm%s_bufRdElemCnt%s = r_wrStrm%s_bufRdElemCnt%s + 1;\n", tabs.c_str(),
					strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());

				if (pStrm->m_reserve.AsInt() != 0) {
					if (pStrm->m_bClose) {
						strmPostInstr.Append("\t\t\t}\n");
						strmPostInstr.Append("\t\t\tif (r_wrStrm%s_pipeQueFront%s.m_bClose) {\n",
							strmName.c_str(), strmIdxStr.c_str());
						strmPostInstr.Append("\t\t\t\tc_wrStrm%s_bOpenBufWr%s = false;\n",
							strmName.c_str(), strmIdxStr.c_str());
						strmPostInstr.Append("\t\t\t\tc_wrStrm%s_bClosingBufRd%s = true;\n",
							strmName.c_str(), strmIdxStr.c_str());
						strmPostInstr.Append("\t\t\t}\n");
					} else {
						strmPostInstr.Append("\t\t\tc_wrStrm%s_bufWrRem%s = r_wrStrm%s_bufWrRem%s - 1;\n",
							strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
						strmPostInstr.Append("\n");

						strmPostInstr.Append("\t\t\tif (r_wrStrm%s_bufWrRem%s == 1)\n", strmName.c_str(), strmIdxStr.c_str());
						strmPostInstr.Append("\t\t\t\tc_wrStrm%s_bOpenBufWr%s = false;\n", strmName.c_str(), strmIdxStr.c_str());
						strmPostInstr.Append("\n");
					}

					strmPostInstr.Append("\t\t\tc_wrStrm%s_bufWrEn%s = true;\n", strmName.c_str(), strmIdxStr.c_str());
					strmPostInstr.Append("\t\t}\n");
					strmPostInstr.Append("\n");

					strmPostInstr.Append("\t\tif (r_wrStrm%s_pipeQueEmpty%s || c_wrStrm%s_bufWrEn%s) {\n",
						strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
					strmPostInstr.Append("\t\t\tc_wrStrm%s_pipeQueEmpty%s = m_wrStrm%s_pipeQue%s.empty();\n",
						strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
					strmPostInstr.Append("\t\t\tc_wrStrm%s_pipeQueFront%s = m_wrStrm%s_pipeQue%s.front();\n",
						strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
					strmPostInstr.Append("\t\t\tif (!m_wrStrm%s_pipeQue%s.empty())\n",
						strmName.c_str(), strmIdxStr.c_str());
					strmPostInstr.Append("\t\t\t\tm_wrStrm%s_pipeQue%s.pop();\n",
						strmName.c_str(), strmIdxStr.c_str());
				}
				strmPostInstr.Append("\t\t}\n");
				strmPostInstr.Append("\n");

				strmPostInstr.Append("\t\tif (r_wrStrm%s_bOpenBufRd%s && !r_wrStrm%s_bBufRd%s) {\n", strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
				strmPostInstr.Append("\n");

				strmPostInstr.Append("\t\t\tc_wrStrm%s_bufRdAddr%s = r_wrStrm%s_openAddr%s;\n", strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
				if (pStrm->m_elemByteW < 3) {
					strmPostInstr.Append("\t\t\tif (r_wrStrm%s_bufRdPtr%s(%d,0) != 0)\n",
						strmName.c_str(), strmIdxStr.c_str(), 3 - pStrm->m_elemByteW - 1);
					strmPostInstr.Append("\t\t\t\tc_wrStrm%s_bufRdPtr%s(%d,%d) = r_wrStrm%s_bufRdPtr%s(%d,%d) + 1;\n",
						strmName.c_str(), strmIdxStr.c_str(), bufPtrW - 1, 3 - pStrm->m_elemByteW,
						strmName.c_str(), strmIdxStr.c_str(), bufPtrW - 1, 3 - pStrm->m_elemByteW);
					strmPostInstr.Append("\t\t\tc_wrStrm%s_bufRdPtr%s(%d,0) = r_wrStrm%s_openAddr%s(2,%d);\n",
						strmName.c_str(), strmIdxStr.c_str(), 2 - pStrm->m_elemByteW,
						strmName.c_str(), strmIdxStr.c_str(), pStrm->m_elemByteW);
				}
				if (!pStrm->m_bClose)
					strmPostInstr.Append("\t\t\tc_wrStrm%s_bufRdRem%s = r_wrStrm%s_openCnt%s;\n", strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
				strmPostInstr.Append("\t\t\tc_wrStrm%s_bufRdElemCnt%s = c_wrStrm%s_bufWrElemCnt%s;\n", strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
				strmPostInstr.Append("\t\t\tc_wrStrm%s_bufWrElemCnt%s = 0;\n", strmName.c_str(), strmIdxStr.c_str());
				if (pStrm->m_rspGrpW.AsInt() > 0)
					strmPostInstr.Append("\t\t\tc_wrStrm%s_rspGrpId%s = r_wrStrm%s_openRspGrpId%s;\n",
					strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
				strmPostInstr.Append("\n");

				strmPostInstr.Append("\t\t\tc_wrStrm%s_bOpenBufRd%s = false;\n", strmName.c_str(), strmIdxStr.c_str());
				strmPostInstr.Append("\t\t\tc_wrStrm%s_bBufRd%s = true;\n", strmName.c_str(), strmIdxStr.c_str());
				strmPostInstr.Append("\t\t}\n");
				strmPostInstr.Append("\n");

				strmPostInstr.Append("\t\tif (r_wrStrm%s_bOpenBusy%s && !c_wrStrm%s_bOpenBusy%s)\n",
					strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
				if (pStrm->m_rspGrpW.AsInt() > 0)
					strmPostInstr.Append("\t\t\tc_wrStrm%s_rspGrpCnt[r_wrStrm%s_rspGrpId%s] -= 1u;\n", strmName.c_str(), strmName.c_str(), strmIdxStr.c_str());
				else
					strmPostInstr.Append("\t\t\tc_wrStrm%s_rspGrpCnt -= 1u;\n", strmName.c_str());

				if (bMultiQwReq)
					strmPostInstr.Append("\t\tc_t1_wrStrm%s_bReqQwRem%s = r_t3_wrStrm%s_reqQwRem%s > 0;\n",
					strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());

				if (pStrm->m_elemByteW == 0) {
					strmPostInstr.Append("\t\tc_t1_wrStrm%s_bReqUint8%s = r_wrStrm%s_bufRdAddr%s[0] != 0 && r_wrStrm%s_bufRdElemCnt%s >= %d ||\n",
						strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), 1 << (0 - pStrm->m_elemByteW));

					if (pStrm->m_bClose)
						strmPostInstr.Append("\t\t\tr_wrStrm%s_bClosingBufRd%s && r_wrStrm%s_bufRdElemCnt%s == 1;\n",
						strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
					else
						strmPostInstr.Append("\t\t\tr_wrStrm%s_bufRdRem%s <= r_wrStrm%s_bufRdElemCnt%s && r_wrStrm%s_bufRdElemCnt%s == 1;\n",
						strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
				}

				if (pStrm->m_elemByteW <= 1) {
					strmPostInstr.Append("\t\tc_t1_wrStrm%s_bReqUint16%s = r_wrStrm%s_bufRdAddr%s[1] != 0 && r_wrStrm%s_bufRdElemCnt%s >= %d ||\n",
						strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), 1 << (1 - pStrm->m_elemByteW));

					if (pStrm->m_bClose)
						strmPostInstr.Append("\t\t\tr_wrStrm%s_bClosingBufRd%s && r_wrStrm%s_bufRdElemCnt%s <= %d;\n",
						strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), (1 << (2 - pStrm->m_elemByteW)) - 1);
					else
						strmPostInstr.Append("\t\t\tr_wrStrm%s_bufRdRem%s <= r_wrStrm%s_bufRdElemCnt%s && r_wrStrm%s_bufRdElemCnt%s <= %d;\n",
						strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), (1 << (2 - pStrm->m_elemByteW)) - 1);
				}

				if (pStrm->m_elemByteW <= 2) {
					strmPostInstr.Append("\t\tc_t1_wrStrm%s_bReqUint32%s = r_wrStrm%s_bufRdAddr%s[2] != 0 && r_wrStrm%s_bufRdElemCnt%s >= %d ||\n",
						strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), 1 << (2 - pStrm->m_elemByteW));

					if (pStrm->m_bClose)
						strmPostInstr.Append("\t\t\tr_wrStrm%s_bClosingBufRd%s && r_wrStrm%s_bufRdElemCnt%s <= %d;\n",
						strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), qwElemCnt - 1);
					else
						strmPostInstr.Append("\t\t\tr_wrStrm%s_bufRdRem%s <= r_wrStrm%s_bufRdElemCnt%s && r_wrStrm%s_bufRdElemCnt%s <= %d;\n",
						strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), qwElemCnt - 1);
				}

				strmPostInstr.Append("\t}\n");
				strmPostInstr.Append("\n");
			}

			string c1_or_r2 = mod.m_clkRate == eClk1x ? "c_t1" : "r_t2";
			char clkCh = mod.m_clkRate == eClk1x ? 'c' : 'r';

			for (int strmIdx = 0; strmIdx < pStrm->m_strmCnt.AsInt(); strmIdx += 1) {
				string strmIdxStr = pStrm->m_strmCnt.size() == 0 ? "" : VA("[%d]", strmIdx);
				string strmRspGrpIdStr = pStrm->m_rspGrpW.AsInt() == 0 ? "" : VA("[r_wrStrm%s_rspGrpId%s]", strmName.c_str(), strmIdxStr.c_str());

				strmPostInstr.Append("\tif (%s_wrStrm%s_bReqSel%s && !r_%sP%dToMif_reqAvlCntBusy) {\n",
					c1_or_r2.c_str(), strmName.c_str(), strmIdxStr.c_str(), mod.m_modName.Lc().c_str(), pStrm->m_memPort[strmIdx]);
				strmPostInstr.Append("\n");

				strmPostInstr.Append("\t\tCWrStrm%s_wrTid wrTid;\n", strmName.c_str());
				strmPostInstr.Append("\t\twrTid.m_tid = c_t2_%sP%dToMif_req.m_tid;\n", mod.m_modName.Lc().c_str(), pStrm->m_memPort[strmIdx]);
				strmPostInstr.Append("\t\twrTid.m_mifStrmId = %d;\n", pStrm->m_arbRr[strmIdx]);
				strmPostInstr.Append("\n");

				strmPostInstr.Append("\t\tc_t2_wrStrm%s_bReqSel%s = true;\n", strmName.c_str(), strmIdxStr.c_str());
				strmPostInstr.Append("\n");

				int reqSelIdx = 0;
				string preLine;

				if (bMultiQwReq) {
					strmPostInstr.Append("\t\tif (%s_wrStrm%s_bReqQwRem%s) {\n", c1_or_r2.c_str(), strmName.c_str(), strmIdxStr.c_str());
					strmPostInstr.Append("\n");

					strmPostInstr.Append("\t\t\tc_t2_wrStrm%s_reqSel%s = %d;\n", strmName.c_str(), strmIdxStr.c_str(), reqSelIdx++);
					strmPostInstr.Append("\n");

					strmPostInstr.Append("\t\t\tc_t2_wrStrm%s_reqQwRem%s = r_t3_wrStrm%s_reqQwRem%s - 1;\n",
						strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
					strmPostInstr.Append("\t\t\tc_wrStrm%s_bufRdPtr%s = r_wrStrm%s_bufRdPtr%s + %d;\n",
						strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), qwElemCnt);
					strmPostInstr.Append("\n");

					if (mod.m_clkRate == eClk1x)
						strmPostInstr.Append("\t\t\tc_wrStrm%s_bufRdAddr%s = r_wrStrm%s_bufRdAddr%s + 8;\n",
						strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
					else {
						strmPostInstr.Append("\t\t\tht_uint25 lowSum = r_wrStrm%s_bufRdAddr%s(23,0) + 8;\n",
							strmName.c_str(), strmIdxStr.c_str());
						strmPostInstr.Append("\t\t\tc_wrStrm%s_bufRdAddr%s(23,0) = lowSum(23,0);\n",
							strmName.c_str(), strmIdxStr.c_str());
						strmPostInstr.Append("\t\t\tc_wrStrm%s_sumCy%s = lowSum(24,24);\n",
							strmName.c_str(), strmIdxStr.c_str());
					}

					if (!pStrm->m_bClose)
						strmPostInstr.Append("\t\t\tc_wrStrm%s_bufRdRem%s = r_wrStrm%s_bufRdRem%s - %d;\n",
						strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), qwElemCnt);
					strmPostInstr.Append("\n");

					if (pStrm->m_bClose) {
						strmPostInstr.Append("\t\t\tif (r_wrStrm%s_bClosingBufRd%s && c_wrStrm%s_bufRdElemCnt%s == %d) {\n",
							strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), qwElemCnt);
						strmPostInstr.Append("\t\t\t\tc_wrStrm%s_bBufRd%s = false;\n", strmName.c_str(), strmIdxStr.c_str());
						strmPostInstr.Append("\t\t\t\tc_wrStrm%s_bClosingBufRd%s = false;\n", strmName.c_str(), strmIdxStr.c_str());
						strmPostInstr.Append("\t\t\t}\n");
					} else {
						strmPostInstr.Append("\t\t\tif (r_wrStrm%s_bufRdRem%s == %d)\n", strmName.c_str(), strmIdxStr.c_str(), qwElemCnt);
						strmPostInstr.Append("\t\t\t\tc_wrStrm%s_bBufRd%s = false;\n", strmName.c_str(), strmIdxStr.c_str());
					}
					strmPostInstr.Append("\t\t\tc_wrStrm%s_bufRdElemCnt%s -= %d;\n", strmName.c_str(), strmIdxStr.c_str(), qwElemCnt);
					strmPostInstr.Append("\n");
					preLine = "\t\t} else ";

				} else {
					reqSelIdx += 1;
					preLine = "\t\t";
				}

				if (pStrm->m_elemByteW == 0) {
					strmPostInstr.Append("%sif (%s_wrStrm%s_bReqUint8%s) {\n", preLine.c_str(), c1_or_r2.c_str(), strmName.c_str(), strmIdxStr.c_str());
					strmPostInstr.Append("\n");

					strmPostInstr.Append("\t\t\tc_t2_wrStrm%s_reqSel%s = %d;\n", strmName.c_str(), strmIdxStr.c_str(), reqSelIdx++);
					strmPostInstr.Append("\t\t\tc_t2_wrStrm%s_rdSel0%s = r_wrStrm%s_bufRdAddr%s(2,0);\n",
						strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
					strmPostInstr.Append("\n");

					strmPostInstr.Append("\t\t\tc_t2_wrStrm%s_reqQwRem%s = 0;\n", strmName.c_str(), strmIdxStr.c_str());
					strmPostInstr.Append("\t\t\tc_wrStrm%s_bufRdPtr%s = r_wrStrm%s_bufRdPtr%s + 1;\n",
						strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
					strmPostInstr.Append("\n");

					if (mod.m_clkRate == eClk1x)
						strmPostInstr.Append("\t\t\tc_wrStrm%s_bufRdAddr%s = r_wrStrm%s_bufRdAddr%s + 1;\n",
						strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
					else {
						strmPostInstr.Append("\t\t\tht_uint25 lowSum = r_wrStrm%s_bufRdAddr%s(23,0) + 1;\n",
							strmName.c_str(), strmIdxStr.c_str());
						strmPostInstr.Append("\t\t\tc_wrStrm%s_bufRdAddr%s(23,0) = lowSum(23,0);\n",
							strmName.c_str(), strmIdxStr.c_str());
						strmPostInstr.Append("\t\t\tc_wrStrm%s_sumCy%s = lowSum(24,24);\n",
							strmName.c_str(), strmIdxStr.c_str());
					}

					if (!pStrm->m_bClose)
						strmPostInstr.Append("\t\t\tc_wrStrm%s_bufRdRem%s = r_wrStrm%s_bufRdRem%s - 1;\n",
						strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());

					strmPostInstr.Append("\t\t\tc_wrStrm%s_rspGrpCnt%s += 1u;\n", strmName.c_str(), strmRspGrpIdStr.c_str());
					strmPostInstr.Append("\n");

					strmPostInstr.Append("\t\t\twrTid.m_reqQwCntM1 = c_t2_wrStrm%s_reqQwRem%s;\n", strmName.c_str(), strmIdxStr.c_str());
					if (pStrm->m_rspGrpW.AsInt() > 0)
						strmPostInstr.Append("\t\t\twrTid.m_rspGrpId = r_wrStrm%s_rspGrpId%s;\n", strmName.c_str(), strmIdxStr.c_str());
					strmPostInstr.Append("\n");

					if (pStrm->m_bClose) {
						strmPostInstr.Append("\t\t\tif (r_wrStrm%s_bClosingBufRd%s && c_wrStrm%s_bufRdElemCnt%s == 1) {\n",
							strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
						strmPostInstr.Append("\t\t\t\tc_wrStrm%s_bBufRd%s = false;\n", strmName.c_str(), strmIdxStr.c_str());
						strmPostInstr.Append("\t\t\t\tc_wrStrm%s_bClosingBufRd%s = false;\n", strmName.c_str(), strmIdxStr.c_str());
						strmPostInstr.Append("\t\t\t}\n");
					} else {
						strmPostInstr.Append("\t\t\tif (r_wrStrm%s_bufRdRem%s == 1)\n", strmName.c_str(), strmIdxStr.c_str());
						strmPostInstr.Append("\t\t\t\tc_wrStrm%s_bBufRd%s = false;\n", strmName.c_str(), strmIdxStr.c_str());
					}
					strmPostInstr.Append("\t\t\tc_wrStrm%s_bufRdElemCnt%s -= 1;\n", strmName.c_str(), strmIdxStr.c_str());
					strmPostInstr.Append("\n");

					strmPostInstr.Append("#\t\t\tifndef _HTV\n");
					strmPostInstr.Append("\t\t\tHt::g_syscMon.UpdateModuleMemWrites(m_htMonModId, %d, 1);\n", pStrm->m_memPort[strmIdx]);
					strmPostInstr.Append("\t\t\tHt::g_syscMon.UpdateModuleMemWriteBytes(m_htMonModId, %d, 1);\n", pStrm->m_memPort[strmIdx]);
					strmPostInstr.Append("#\t\t\tendif\n");
					strmPostInstr.Append("\n");

					preLine = "\t\t} else ";
				}

				if (pStrm->m_elemByteW <= 1) {
					strmPostInstr.Append("%sif (%s_wrStrm%s_bReqUint16%s) {\n", preLine.c_str(), c1_or_r2.c_str(), strmName.c_str(), strmIdxStr.c_str());
					strmPostInstr.Append("\n");

					strmPostInstr.Append("\t\t\tc_t2_wrStrm%s_reqSel%s = %d;\n", strmName.c_str(), strmIdxStr.c_str(), reqSelIdx++);

					switch (pStrm->m_elemByteW) {
					case 0:
						strmPostInstr.Append("\t\t\tc_t2_wrStrm%s_rdSel0%s = r_wrStrm%s_bufRdAddr%s(2,0) & 6;\n",
							strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
						strmPostInstr.Append("\t\t\tc_t2_wrStrm%s_rdSel1%s = (r_wrStrm%s_bufRdAddr%s(2,0) & 6) | 1;\n",
							strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
						break;
					case 1:
						strmPostInstr.Append("\t\t\tc_t2_wrStrm%s_rdSel0%s = r_wrStrm%s_bufRdAddr%s(2,1);\n",
							strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
						break;
					default:
						assert(0);
					}
					strmPostInstr.Append("\n");

					strmPostInstr.Append("\t\t\tc_t2_wrStrm%s_reqQwRem%s = 0;\n", strmName.c_str(), strmIdxStr.c_str());
					strmPostInstr.Append("\t\t\tc_wrStrm%s_bufRdPtr%s = r_wrStrm%s_bufRdPtr%s + %d;\n",
						strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), qwElemCnt / 4);
					strmPostInstr.Append("\n");

					if (mod.m_clkRate == eClk1x)
						strmPostInstr.Append("\t\t\tc_wrStrm%s_bufRdAddr%s = r_wrStrm%s_bufRdAddr%s + 2;\n",
						strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
					else {
						strmPostInstr.Append("\t\t\tht_uint25 lowSum = r_wrStrm%s_bufRdAddr%s(23,0) + 2;\n",
							strmName.c_str(), strmIdxStr.c_str());
						strmPostInstr.Append("\t\t\tc_wrStrm%s_bufRdAddr%s(23,0) = lowSum(23,0);\n",
							strmName.c_str(), strmIdxStr.c_str());
						strmPostInstr.Append("\t\t\tc_wrStrm%s_sumCy%s = lowSum(24,24);\n",
							strmName.c_str(), strmIdxStr.c_str());
					}

					if (!pStrm->m_bClose)
						strmPostInstr.Append("\t\t\tc_wrStrm%s_bufRdRem%s = r_wrStrm%s_bufRdRem%s - %d;\n",
						strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), qwElemCnt / 4);
					strmPostInstr.Append("\t\t\tc_wrStrm%s_rspGrpCnt%s += 1u;\n", strmName.c_str(), strmRspGrpIdStr.c_str());
					strmPostInstr.Append("\n");

					strmPostInstr.Append("\t\t\twrTid.m_reqQwCntM1 = c_t2_wrStrm%s_reqQwRem%s;\n", strmName.c_str(), strmIdxStr.c_str());
					if (pStrm->m_rspGrpW.AsInt() > 0)
						strmPostInstr.Append("\t\t\twrTid.m_rspGrpId = r_wrStrm%s_rspGrpId%s;\n", strmName.c_str(), strmIdxStr.c_str());
					strmPostInstr.Append("\n");

					if (pStrm->m_bClose) {
						strmPostInstr.Append("\t\t\tif (r_wrStrm%s_bClosingBufRd%s && c_wrStrm%s_bufRdElemCnt%s == %d) {\n",
							strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), qwElemCnt / 4);
						strmPostInstr.Append("\t\t\t\tc_wrStrm%s_bBufRd%s = false;\n", strmName.c_str(), strmIdxStr.c_str());
						strmPostInstr.Append("\t\t\t\tc_wrStrm%s_bClosingBufRd%s = false;\n", strmName.c_str(), strmIdxStr.c_str());
						strmPostInstr.Append("\t\t\t}\n");
					} else {
						strmPostInstr.Append("\t\t\tif (r_wrStrm%s_bufRdRem%s == %d)\n", strmName.c_str(), strmIdxStr.c_str(), qwElemCnt / 4);
						strmPostInstr.Append("\t\t\t\tc_wrStrm%s_bBufRd%s = false;\n", strmName.c_str(), strmIdxStr.c_str());
					}
					strmPostInstr.Append("\t\t\tc_wrStrm%s_bufRdElemCnt%s -= %d;\n", strmName.c_str(), strmIdxStr.c_str(), qwElemCnt / 4);
					strmPostInstr.Append("\n");

					strmPostInstr.Append("#\t\t\tifndef _HTV\n");
					strmPostInstr.Append("\t\t\tHt::g_syscMon.UpdateModuleMemWrites(m_htMonModId, %d, 1);\n", pStrm->m_memPort[strmIdx]);
					strmPostInstr.Append("\t\t\tHt::g_syscMon.UpdateModuleMemWriteBytes(m_htMonModId, %d, 2);\n", pStrm->m_memPort[strmIdx]);
					strmPostInstr.Append("#\t\t\tendif\n");
					strmPostInstr.Append("\n");

					preLine = "\t\t} else ";
				}

				if (pStrm->m_elemByteW <= 2) {
					strmPostInstr.Append("%sif (%s_wrStrm%s_bReqUint32%s) {\n", preLine.c_str(), c1_or_r2.c_str(), strmName.c_str(), strmIdxStr.c_str());
					strmPostInstr.Append("\n");

					strmPostInstr.Append("\t\t\tc_t2_wrStrm%s_reqSel%s = %d;\n", strmName.c_str(), strmIdxStr.c_str(), reqSelIdx++);

					switch (pStrm->m_elemByteW) {
					case 0:
						strmPostInstr.Append("\t\t\tc_t2_wrStrm%s_rdSel0%s = r_wrStrm%s_bufRdAddr%s(2,0) & 4;\n",
							strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
						strmPostInstr.Append("\t\t\tc_t2_wrStrm%s_rdSel1%s = (r_wrStrm%s_bufRdAddr%s(2,0) & 4) | 1;\n",
							strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
						strmPostInstr.Append("\t\t\tc_t2_wrStrm%s_rdSel2%s = (r_wrStrm%s_bufRdAddr%s(2,0) & 4) | 2;\n",
							strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
						strmPostInstr.Append("\t\t\tc_t2_wrStrm%s_rdSel3%s = (r_wrStrm%s_bufRdAddr%s(2,0) & 4) | 3;\n",
							strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
						break;
					case 1:
						strmPostInstr.Append("\t\t\tc_t2_wrStrm%s_rdSel0%s = r_wrStrm%s_bufRdAddr%s(2,1) & 2;\n",
							strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
						strmPostInstr.Append("\t\t\tc_t2_wrStrm%s_rdSel1%s = r_wrStrm%s_bufRdAddr%s(2,1) | 1;\n",
							strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
						break;
					case 2:
						strmPostInstr.Append("\t\t\tc_t2_wrStrm%s_rdSel0%s = r_wrStrm%s_bufRdAddr%s(2,2);\n",
							strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
						break;
					default:
						assert(0);
					}
					strmPostInstr.Append("\n");

					strmPostInstr.Append("\t\t\tc_t2_wrStrm%s_reqQwRem%s = 0;\n", strmName.c_str(), strmIdxStr.c_str());
					strmPostInstr.Append("\t\t\tc_wrStrm%s_bufRdPtr%s = r_wrStrm%s_bufRdPtr%s + %d;\n",
						strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), qwElemCnt / 2);
					strmPostInstr.Append("\n");

					if (mod.m_clkRate == eClk1x)
						strmPostInstr.Append("\t\t\tc_wrStrm%s_bufRdAddr%s = r_wrStrm%s_bufRdAddr%s + 4;\n",
						strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
					else {
						strmPostInstr.Append("\t\t\tht_uint25 lowSum = r_wrStrm%s_bufRdAddr%s(23,0) + 4;\n",
							strmName.c_str(), strmIdxStr.c_str());
						strmPostInstr.Append("\t\t\tc_wrStrm%s_bufRdAddr%s(23,0) = lowSum(23,0);\n",
							strmName.c_str(), strmIdxStr.c_str());
						strmPostInstr.Append("\t\t\tc_wrStrm%s_sumCy%s = lowSum(24,24);\n",
							strmName.c_str(), strmIdxStr.c_str());
					}

					if (!pStrm->m_bClose)
						strmPostInstr.Append("\t\t\tc_wrStrm%s_bufRdRem%s = r_wrStrm%s_bufRdRem%s - %d;\n",
						strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), qwElemCnt / 2);
					strmPostInstr.Append("\t\t\tc_wrStrm%s_rspGrpCnt%s += 1u;\n", strmName.c_str(), strmRspGrpIdStr.c_str());
					strmPostInstr.Append("\n");

					strmPostInstr.Append("\t\t\twrTid.m_reqQwCntM1 = c_t2_wrStrm%s_reqQwRem%s;\n", strmName.c_str(), strmIdxStr.c_str());
					if (pStrm->m_rspGrpW.AsInt() > 0)
						strmPostInstr.Append("\t\t\twrTid.m_rspGrpId = r_wrStrm%s_rspGrpId%s;\n", strmName.c_str(), strmIdxStr.c_str());
					strmPostInstr.Append("\n");

					if (pStrm->m_bClose) {
						strmPostInstr.Append("\t\t\tif (r_wrStrm%s_bClosingBufRd%s && c_wrStrm%s_bufRdElemCnt%s == %d) {\n",
							strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), qwElemCnt / 2);
						strmPostInstr.Append("\t\t\t\tc_wrStrm%s_bBufRd%s = false;\n", strmName.c_str(), strmIdxStr.c_str());
						strmPostInstr.Append("\t\t\t\tc_wrStrm%s_bClosingBufRd%s = false;\n", strmName.c_str(), strmIdxStr.c_str());
						strmPostInstr.Append("\t\t\t}\n");
					} else {
						strmPostInstr.Append("\t\t\tif (r_wrStrm%s_bufRdRem%s == %d)\n", strmName.c_str(), strmIdxStr.c_str(), qwElemCnt / 2);
						strmPostInstr.Append("\t\t\t\tc_wrStrm%s_bBufRd%s = false;\n", strmName.c_str(), strmIdxStr.c_str());
					}
					strmPostInstr.Append("\t\t\tc_wrStrm%s_bufRdElemCnt%s -= %d;\n", strmName.c_str(), strmIdxStr.c_str(), qwElemCnt / 2);
					strmPostInstr.Append("\n");

					strmPostInstr.Append("#\t\t\tifndef _HTV\n");
					strmPostInstr.Append("\t\t\tHt::g_syscMon.UpdateModuleMemWrites(m_htMonModId, %d, 1);\n", pStrm->m_memPort[strmIdx]);
					strmPostInstr.Append("\t\t\tHt::g_syscMon.UpdateModuleMemWriteBytes(m_htMonModId, %d, 4);\n", pStrm->m_memPort[strmIdx]);
					strmPostInstr.Append("#\t\t\tendif\n");
					strmPostInstr.Append("\n");

					preLine = "\t\t} else ";
				}

				strmPostInstr.Append("%s{\n", preLine.c_str());

				strmPostInstr.Append("\t\t\tc_t2_wrStrm%s_reqSel%s = 0;\n", strmName.c_str(), strmIdxStr.c_str());
				strmPostInstr.Append("\n");

				if (bIsWx) {
					strmPostInstr.Append("\t\t\tht_uint4 reqQwCnt;\n");
					strmPostInstr.Append("\t\t\tif (r_wrStrm%s_bufRdAddr%s(5,3) + r_wrStrm%s_bufRdElemCnt%s(%d,%d) >= 8)\n",
						strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), bufPtrW - 1, 3 - pStrm->m_elemByteW);
					strmPostInstr.Append("\t\t\t\treqQwCnt = 8u - r_wrStrm%s_bufRdAddr%s(5,3);\n", strmName.c_str(), strmIdxStr.c_str());
					strmPostInstr.Append("\t\t\telse\n");

					strmPostInstr.Append("\t\t\t\treqQwCnt = r_wrStrm%s_bufRdElemCnt%s(%d,%d);\n", strmName.c_str(), strmIdxStr.c_str(), 3 - pStrm->m_elemByteW + 2, 3 - pStrm->m_elemByteW);
					strmPostInstr.Append("\n");
				} else if (bIsHc2 && pStrm->m_memSrc == "host") {
					strmPostInstr.Append("\t\t\tht_uint4 reqQwCnt;\n");
					strmPostInstr.Append("\t\t\tif (r_wrStrm%s_bufRdAddr%s(5,3) == 0 && r_wrStrm%s_bufRdElemCnt%s(%d,%d) >= 8)\n",
						strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), bufPtrW - 1, 3 - pStrm->m_elemByteW);
					strmPostInstr.Append("\t\t\t\treqQwCnt = 8u;\n");
					strmPostInstr.Append("\t\t\telse\n");

					strmPostInstr.Append("\t\t\t\treqQwCnt = 1u;\n");
					strmPostInstr.Append("\n");
				} else
					strmPostInstr.Append("\t\t\tht_uint4 reqQwCnt = 1;\n");

				strmPostInstr.Append("\t\t\tc_t2_wrStrm%s_reqQwRem%s = (ht_uint3)(reqQwCnt - 1);\n", strmName.c_str(), strmIdxStr.c_str());
				strmPostInstr.Append("\t\t\tc_wrStrm%s_bufRdPtr%s = r_wrStrm%s_bufRdPtr%s + %d;\n",
					strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), qwElemCnt);
				strmPostInstr.Append("\n");

				if (mod.m_clkRate == eClk1x)
					strmPostInstr.Append("\t\t\tc_wrStrm%s_bufRdAddr%s = r_wrStrm%s_bufRdAddr%s + 8;\n",
					strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str());
				else {
					strmPostInstr.Append("\t\t\tht_uint25 lowSum = r_wrStrm%s_bufRdAddr%s(23,0) + 8;\n",
						strmName.c_str(), strmIdxStr.c_str());
					strmPostInstr.Append("\t\t\tc_wrStrm%s_bufRdAddr%s(23,0) = lowSum(23,0);\n",
						strmName.c_str(), strmIdxStr.c_str());
					strmPostInstr.Append("\t\t\tc_wrStrm%s_sumCy%s = lowSum(24,24);\n",
						strmName.c_str(), strmIdxStr.c_str());
				}

				if (!pStrm->m_bClose)
					strmPostInstr.Append("\t\t\tc_wrStrm%s_bufRdRem%s = r_wrStrm%s_bufRdRem%s - %d;\n",
					strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), qwElemCnt);
				strmPostInstr.Append("\t\t\tc_wrStrm%s_rspGrpCnt%s += 1u;\n", strmName.c_str(), strmRspGrpIdStr.c_str());
				strmPostInstr.Append("\n");

				strmPostInstr.Append("\t\t\twrTid.m_reqQwCntM1 = c_t2_wrStrm%s_reqQwRem%s;\n", strmName.c_str(), strmIdxStr.c_str());
				if (pStrm->m_rspGrpW.AsInt() > 0)
					strmPostInstr.Append("\t\t\twrTid.m_rspGrpId = r_wrStrm%s_rspGrpId%s;\n", strmName.c_str(), strmIdxStr.c_str());
				strmPostInstr.Append("\n");

				if (pStrm->m_bClose) {
					strmPostInstr.Append("\t\t\tif (r_wrStrm%s_bClosingBufRd%s && c_wrStrm%s_bufRdElemCnt%s == %d) {\n",
						strmName.c_str(), strmIdxStr.c_str(), strmName.c_str(), strmIdxStr.c_str(), qwElemCnt);
					strmPostInstr.Append("\t\t\t\tc_wrStrm%s_bBufRd%s = false;\n", strmName.c_str(), strmIdxStr.c_str());
					strmPostInstr.Append("\t\t\t\tc_wrStrm%s_bClosingBufRd%s = false;\n", strmName.c_str(), strmIdxStr.c_str());
					strmPostInstr.Append("\t\t\t}\n");
				} else {
					strmPostInstr.Append("\t\t\tif (r_wrStrm%s_bufRdRem%s == %d)\n", strmName.c_str(), strmIdxStr.c_str(), qwElemCnt);
					strmPostInstr.Append("\t\t\t\tc_wrStrm%s_bBufRd%s = false;\n", strmName.c_str(), strmIdxStr.c_str());
				}

				strmPostInstr.Append("\t\t\tc_wrStrm%s_bufRdElemCnt%s -= %d;\n", strmName.c_str(), strmIdxStr.c_str(), qwElemCnt);
				strmPostInstr.Append("\n");

				strmPostInstr.Append("#\t\t\tifndef _HTV\n");
				strmPostInstr.Append("\t\t\tHt::g_syscMon.UpdateModuleMemWrites(m_htMonModId, %d, 1);\n", pStrm->m_memPort[strmIdx]);
				if (bMultiQwReq)
					strmPostInstr.Append("\t\t\tHt::g_syscMon.UpdateModuleMemWrite64s(m_htMonModId, %d, reqQwCnt == 1 ? 0 : 1);\n", pStrm->m_memPort[strmIdx]);
				strmPostInstr.Append("\t\t\tHt::g_syscMon.UpdateModuleMemWriteBytes(m_htMonModId, %d, reqQwCnt * 8);\n", pStrm->m_memPort[strmIdx]);
				strmPostInstr.Append("#\t\t\tendif\n");

				strmPostInstr.Append("\t\t}\n");
				strmPostInstr.Append("\n");

				strmPostInstr.Append("\t\tc_t2_%sP%dToMif_reqRdy = true;\n", mod.m_modName.Lc().c_str(), pStrm->m_memPort[strmIdx]);
				strmPostInstr.Append("\t\tc_t2_%sP%dToMif_req.m_tid = wrTid.m_tid;\n", mod.m_modName.Lc().c_str(), pStrm->m_memPort[strmIdx]);
				strmPostInstr.Append("\t\tc_t2_%sP%dToMif_req.m_host = %s;\n",
					mod.m_modName.Lc().c_str(), pStrm->m_memPort[strmIdx], pStrm->m_memSrc.AsStr() == "host" ? "true" : "false");
				strmPostInstr.Append("\t\tc_t2_%sP%dToMif_req.m_type = MEM_REQ_WR;\n", mod.m_modName.Lc().c_str(), pStrm->m_memPort[strmIdx]);
				strmPostInstr.Append("\t\tc_t2_%sP%dToMif_req.m_addr = r_wrStrm%s_bufRdAddr%s;\n",
					mod.m_modName.Lc().c_str(), pStrm->m_memPort[strmIdx], strmName.c_str(), strmIdxStr.c_str());
				strmPostInstr.Append("\n");

				strmPostInstr.Append("#\t\tifndef _HTV\n");
				strmPostInstr.Append("\t\tc_t2_%sP%dToMif_req.m_file = __FILE__;\n", mod.m_modName.Lc().c_str(), pStrm->m_memPort[strmIdx]);
				strmPostInstr.Append("\t\tc_t2_%sP%dToMif_req.m_line = __LINE__;\n", mod.m_modName.Lc().c_str(), pStrm->m_memPort[strmIdx]);
				strmPostInstr.Append("\t\tc_t2_%sP%dToMif_req.m_time = sc_time_stamp().value();\n", mod.m_modName.Lc().c_str(), pStrm->m_memPort[strmIdx]);
				strmPostInstr.Append("\t\tc_t2_%sP%dToMif_req.m_orderChk = true;\n", mod.m_modName.Lc().c_str(), pStrm->m_memPort[strmIdx]);
				strmPostInstr.Append("#\t\tendif\n");
				strmPostInstr.Append("\n");

				int memPortIdx = pStrm->m_memPort[strmIdx];
				if (mod.m_memPortList[memPortIdx]->m_strmList.size() > 1) {
					if (bMultiQwReq) {
						strmPostInstr.Append("\t\tif (c_t2_wrStrm%s_reqQwRem%s > 0)\n", strmName.c_str(), strmIdxStr.c_str());
						strmPostInstr.Append("\t\t\tc_%sP%dToMif_reqRr = 0x%x;\n",
							mod.m_modName.Lc().c_str(), memPortIdx,
							1 << pStrm->m_arbRr[strmIdx]);
						strmPostInstr.Append("\t\telse\n");
						strmPostInstr.Append("\t\t\tc_%sP%dToMif_reqRr = 0x%x;\n",
							mod.m_modName.Lc().c_str(), memPortIdx,
							1 << ((pStrm->m_arbRr[strmIdx] + 1) % mod.m_memPortList[memPortIdx]->m_strmList.size()));
					} else
						strmPostInstr.Append("\t\tc_%sP%dToMif_reqRr = 0x%x;\n",
						mod.m_modName.Lc().c_str(), memPortIdx,
						1 << ((pStrm->m_arbRr[strmIdx] + 1) % mod.m_memPortList[memPortIdx]->m_strmList.size()));
				}
				strmPostInstr.Append("\t}\n");
				strmPostInstr.Append("\n");

				if (mod.m_clkRate == eClk1x)
					strmPostInstr.Append("\tif (c_t2_wrStrm%s_bReqSel%s) {\n", strmName.c_str(), strmIdxStr.c_str());
				else
					strmPostInstr.Append("\tif (r_t4_wrStrm%s_bReqSel%s) {\n", strmName.c_str(), strmIdxStr.c_str());

				string c2_or_r3 = mod.m_clkRate == eClk1x ? "c_t2" : "r_t4";
				strmPostInstr.Append("\t\tswitch (%s_wrStrm%s_reqSel%s) {\n", c2_or_r3.c_str(), strmName.c_str(), strmIdxStr.c_str());
				strmPostInstr.Append("\t\tdefault:\n");

				reqSelIdx = 0;

				int wrStrmMemPort = pStrm->m_memPort[strmIdx];
				int wrStrmDataStg = (mod.m_clkRate == eClk1x || !mod.m_memPortList[wrStrmMemPort]->m_bWrite) ? 2 : 4;

				if (true || bMultiQwReq) {
					strmPostInstr.Append("\t\tcase %d:\n", reqSelIdx++);
					strmPostInstr.Append("\t\t\tc_t%d_%sP%dToMif_req.m_size = MEM_REQ_U64;\n", wrStrmDataStg, mod.m_modName.Lc().c_str(), pStrm->m_memPort[strmIdx]);

					for (int idx = 0; idx < qwElemCnt; idx += 1) {
						if (mod.m_clkRate == eClk1x) {
							if (pStrm->m_elemByteW == 3)
								strmPostInstr.Append("\t\t\tc_t%d_%sP%dToMif_req.m_data(%d,%d) = m_wrStrm%s_buf%s.read_mem();\n",
								wrStrmDataStg, mod.m_modName.Lc().c_str(), pStrm->m_memPort[strmIdx], 64 / qwElemCnt*(idx + 1) - 1, 64 / qwElemCnt*idx,
								strmName.c_str(), strmIdxStr.c_str());
							else
								strmPostInstr.Append("\t\t\tc_t%d_%sP%dToMif_req.m_data(%d,%d) = m_wrStrm%s_buf%s.read_mem(%d);\n",
								wrStrmDataStg, mod.m_modName.Lc().c_str(), pStrm->m_memPort[strmIdx], 64 / qwElemCnt*(idx + 1) - 1, 64 / qwElemCnt*idx,
								strmName.c_str(), strmIdxStr.c_str(), idx);
						} else {
							if (pStrm->m_elemByteW == 3)
								strmPostInstr.Append("\t\t\tc_t%d_%sP%dToMif_req.m_data(%d,%d) = r_t4_wrStrm%s_buf%s[0];\n",
								wrStrmDataStg, mod.m_modName.Lc().c_str(), pStrm->m_memPort[strmIdx], 64 / qwElemCnt*(idx + 1) - 1, 64 / qwElemCnt*idx,
								strmName.c_str(), strmIdxStr.c_str());
							else
								strmPostInstr.Append("\t\t\tc_t%d_%sP%dToMif_req.m_data(%d,%d) = r_t4_wrStrm%s_buf%s[%d];\n",
								wrStrmDataStg, mod.m_modName.Lc().c_str(), pStrm->m_memPort[strmIdx], 64 / qwElemCnt*(idx + 1) - 1, 64 / qwElemCnt*idx,
								strmName.c_str(), strmIdxStr.c_str(), idx);
						}
					}
					strmPostInstr.Append("\t\t\tbreak;\n");
				}

				if (pStrm->m_elemByteW == 0) {
					strmPostInstr.Append("\t\tcase %d:\n", reqSelIdx++);
					strmPostInstr.Append("\t\t\tc_t%d_%sP%dToMif_req.m_size = MEM_REQ_U8;\n", wrStrmDataStg, mod.m_modName.Lc().c_str(), pStrm->m_memPort[strmIdx]);
					for (int idx = 0; idx < qwElemCnt; idx += 1)
						if (mod.m_clkRate == eClk1x) {
						strmPostInstr.Append("\t\t\tc_t%d_%sP%dToMif_req.m_data(%d,%d) = m_wrStrm%s_buf%s.read_mem(%s_wrStrm%s_rdSel0%s);\n",
							wrStrmDataStg, mod.m_modName.Lc().c_str(), pStrm->m_memPort[strmIdx], 64 / qwElemCnt*(idx + 1) - 1, 64 / qwElemCnt*idx,
							strmName.c_str(), strmIdxStr.c_str(), c2_or_r3.c_str(), strmName.c_str(), strmIdxStr.c_str());
						} else {
						strmPostInstr.Append("\t\t\tc_t%d_%sP%dToMif_req.m_data(%d,%d) = r_t4_wrStrm%s_buf%s[r_t4_wrStrm%s_rdSel0%s];\n",
							wrStrmDataStg, mod.m_modName.Lc().c_str(), pStrm->m_memPort[strmIdx], 64 / qwElemCnt*(idx + 1) - 1, 64 / qwElemCnt*idx,
							strmName.c_str(), strmIdxStr.c_str(),
							strmName.c_str(), strmIdxStr.c_str());
						}
					strmPostInstr.Append("\t\t\tbreak;\n");
				}

				if (pStrm->m_elemByteW <= 1) {
					strmPostInstr.Append("\t\tcase %d:\n", reqSelIdx++);
					strmPostInstr.Append("\t\t\tc_t%d_%sP%dToMif_req.m_size = MEM_REQ_U16;\n", wrStrmDataStg, mod.m_modName.Lc().c_str(), pStrm->m_memPort[strmIdx]);
					for (int idx = 0; idx < qwElemCnt; idx += 1)
						if (mod.m_clkRate == eClk1x) {
						strmPostInstr.Append("\t\t\tc_t%d_%sP%dToMif_req.m_data(%d,%d) = m_wrStrm%s_buf%s.read_mem(%s_wrStrm%s_rdSel%d%s);\n",
							wrStrmDataStg, mod.m_modName.Lc().c_str(), pStrm->m_memPort[strmIdx], 64 / qwElemCnt*(idx + 1) - 1, 64 / qwElemCnt*idx,
							strmName.c_str(), strmIdxStr.c_str(),
							c2_or_r3.c_str(), strmName.c_str(), idx & ((1 << (1 - pStrm->m_elemByteW)) - 1), strmIdxStr.c_str());
						} else {
						strmPostInstr.Append("\t\t\tc_t%d_%sP%dToMif_req.m_data(%d,%d) = r_t4_wrStrm%s_buf%s[r_t4_wrStrm%s_rdSel%d%s];\n",
							wrStrmDataStg, mod.m_modName.Lc().c_str(), pStrm->m_memPort[strmIdx], 64 / qwElemCnt*(idx + 1) - 1, 64 / qwElemCnt*idx,
							strmName.c_str(), strmIdxStr.c_str(),
							strmName.c_str(), idx & ((1 << (1 - pStrm->m_elemByteW)) - 1), strmIdxStr.c_str());
						}
					strmPostInstr.Append("\t\t\tbreak;\n");
				}

				if (pStrm->m_elemByteW <= 2) {
					strmPostInstr.Append("\t\tcase %d:\n", reqSelIdx++);
					strmPostInstr.Append("\t\t\tc_t%d_%sP%dToMif_req.m_size = MEM_REQ_U32;\n", wrStrmDataStg, mod.m_modName.Lc().c_str(), pStrm->m_memPort[strmIdx]);
					for (int idx = 0; idx < qwElemCnt; idx += 1)
						if (mod.m_clkRate == eClk1x) {
						strmPostInstr.Append("\t\t\tc_t%d_%sP%dToMif_req.m_data(%d,%d) = m_wrStrm%s_buf%s.read_mem(%s_wrStrm%s_rdSel%d%s);\n",
							wrStrmDataStg, mod.m_modName.Lc().c_str(), pStrm->m_memPort[strmIdx], 64 / qwElemCnt*(idx + 1) - 1, 64 / qwElemCnt*idx,
							strmName.c_str(), strmIdxStr.c_str(),
							c2_or_r3.c_str(), strmName.c_str(), idx & ((1 << (2 - pStrm->m_elemByteW)) - 1), strmIdxStr.c_str());
						} else {
						strmPostInstr.Append("\t\t\tc_t%d_%sP%dToMif_req.m_data(%d,%d) = r_t4_wrStrm%s_buf%s[r_t4_wrStrm%s_rdSel%d%s];\n",
							wrStrmDataStg, mod.m_modName.Lc().c_str(), pStrm->m_memPort[strmIdx], 64 / qwElemCnt*(idx + 1) - 1, 64 / qwElemCnt*idx,
							strmName.c_str(), strmIdxStr.c_str(),
							strmName.c_str(), idx & ((1 << (2 - pStrm->m_elemByteW)) - 1), strmIdxStr.c_str());
						}
					strmPostInstr.Append("\t\t\tbreak;\n");
				}

				strmPostInstr.Append("\t\t}\n");
				strmPostInstr.Append("\t}\n");
				strmPostInstr.Append("\n");
			}

			{
				string strmIdxStr;
				if (pStrm->m_strmCnt.size() == 0)
					strmPostInstr.Append("\t{ // wrStrm%s\n", strmName.c_str());
				else {
					strmPostInstr.Append("\tfor (int idx = 0; idx < %d; idx += 1) { // wrStrm%s\n", pStrm->m_strmCnt.AsInt(), strmName.c_str());
					strmIdxStr = "[idx]";
				}

				strmPostInstr.Append("\t\tm_wrStrm%s_buf%s.read_addr( %c_wrStrm%s_bufRdPtr%s(%d,%d) );\n",
					strmName.c_str(), strmIdxStr.c_str(), clkCh, strmName.c_str(), strmIdxStr.c_str(), bufPtrW - 2, 3 - pStrm->m_elemByteW);

				if (mod.m_clkRate == eClk1x)
					strmPostInstr.Append("\t\tc_wrStrm%s_bCollision%s = %c_wrStrm%s_bufWrEn%s && c_wrStrm%s_bufRdPtr%s(%d,%d) == r_wrStrm%s_bufWrPtr%s(%d,%d);\n",
					strmName.c_str(), strmIdxStr.c_str(),
					pStrm->m_reserve.AsInt() == 0 ? 'r' : 'c', strmName.c_str(), strmIdxStr.c_str(),
					strmName.c_str(), strmIdxStr.c_str(), bufPtrW - 2, 3 - pStrm->m_elemByteW, strmName.c_str(), strmIdxStr.c_str(), bufPtrW - 2, 3 - pStrm->m_elemByteW);

				int fullLimit = 0;
				switch (pStrm->m_elemBitW) {
				case 8: fullLimit = 15; break;
				case 16: fullLimit = 7; break;
				case 32: fullLimit = 3; break;
				case 64: fullLimit = 2; break;
				}
				strmPostInstr.Append("\t\tc_wrStrm%s_bBufFull%s = ((%c_wrStrm%s_bufWrPtr%s - %c_wrStrm%s_bufRdPtr%s) & 0x%x) >= %d;\n",
					strmName.c_str(), strmIdxStr.c_str(),
					clkCh, strmName.c_str(), strmIdxStr.c_str(),
					clkCh, strmName.c_str(), strmIdxStr.c_str(),
					(1 << (bufPtrW - 1)) - 1, (1 << (bufPtrW - 1)) - 1 - fullLimit);

				strmPostInstr.Append("\t}\n");
				strmPostInstr.Append("\n");
			}
		}
	}

	for (size_t memPortIdx = 0; memPortIdx < mod.m_memPortList.size(); memPortIdx += 1) {
		CModMemPort & modMemPort = *mod.m_memPortList[memPortIdx];

		if (!modMemPort.m_bIsStrm) continue;

		strmPostInstr.Append("\tif (c_t2_%sP%dToMif_reqRdy != i_mifTo%sP%d_reqAvl) {\n",
			mod.m_modName.Lc().c_str(), memPortIdx, mod.m_modName.Uc().c_str(), memPortIdx);
		strmPostInstr.Append("\t\tc_%sP%dToMif_reqAvlCnt = r_%sP%dToMif_reqAvlCnt + (i_mifTo%sP%d_reqAvl.read() ? 1 : -1);\n",
			mod.m_modName.Lc().c_str(), memPortIdx, mod.m_modName.Lc().c_str(), memPortIdx, mod.m_modName.Uc().c_str(), memPortIdx);
		strmPostInstr.Append("\t\tc_%sP%dToMif_reqAvlCntBusy = r_%sP%dToMif_reqAvlCnt == 1 && !i_mifTo%sP%d_reqAvl.read();\n",
			mod.m_modName.Lc().c_str(), memPortIdx, mod.m_modName.Lc().c_str(), memPortIdx, mod.m_modName.Uc().c_str(), memPortIdx);
		strmPostInstr.Append("\t}\n");
		strmPostInstr.Append("\n");

		if (modMemPort.m_bRead) {

			if (mod.m_clkRate == eClk2x) {
				for (size_t mifStrmIdx = 0; mifStrmIdx < modMemPort.m_strmList.size(); mifStrmIdx += 1) {
					CMemPortStrm & memPortStrm = modMemPort.m_strmList[mifStrmIdx];

					bool bMultiQwReq = bIsWx || bIsHc2 && memPortStrm.m_pStrm->m_memSrc == "host";

					if (!memPortStrm.m_bRead || !bMultiQwReq) continue;

					int elemByteW = memPortStrm.m_pStrm->m_elemByteW;

					string strmName = memPortStrm.m_pStrm->m_name.size() == 0 ? "" : "_" + memPortStrm.m_pStrm->m_name.AsStr();
					string strmIdStr = memPortStrm.m_pStrm->m_strmCnt.size() == 0 ? "" : VA("[%d]", memPortStrm.m_strmIdx);

					strmPostInstr.Append("\t{ // rdStrm%s%s\n", strmName.c_str(), strmIdStr.c_str());

					strmPostInstr.Append("\t\tCRdStrm%s_rdTid rdRspTid;\n", strmName.c_str());
					strmPostInstr.Append("\t\trdRspTid.m_tid = i_mifTo%sP%d_rdRsp.read().m_tid;\n", mod.m_modName.Uc().c_str(), memPortIdx);
					strmPostInstr.Append("\t\tc_rdStrm%s_rspWrPtr1%s = rdRspTid.m_rspBufWrPtr + (rdRspTid.m_rspQwIdx << %d);\n",
						strmName.c_str(), strmIdStr.c_str(), 3 - elemByteW);
					strmPostInstr.Append("\t\tc_rdStrm%s_rspWrPtr%s = r_rdStrm%s_rspWrPtr1%s;\n",
						strmName.c_str(), strmIdStr.c_str(), strmName.c_str(), strmIdStr.c_str());

					strmPostInstr.Append("\t}\n");
					strmPostInstr.Append("\n");
				}
			}

			strmPostInstr.Append("\tif (r_mifTo%sP%d_rdRspRdy) {\n", mod.m_modName.Uc().c_str(), memPortIdx);

			if (modMemPort.m_rdStrmCnt > 1) {
				strmPostInstr.Append("\n");
				strmPostInstr.Append("\t\tswitch (r_mifTo%sP%d_rdRsp.m_tid(3,0)) {\n", mod.m_modName.Uc().c_str(), memPortIdx);
			}

			string tabs = modMemPort.m_rdStrmCnt == 1 ? "" : "\t\t";

			for (size_t mifStrmIdx = 0; mifStrmIdx < modMemPort.m_strmList.size(); mifStrmIdx += 1) {
				CMemPortStrm & memPortStrm = modMemPort.m_strmList[mifStrmIdx];

				if (!memPortStrm.m_bRead) continue;

				bool bMultiQwReq = bIsWx || bIsHc2 && memPortStrm.m_pStrm->m_memSrc == "host";

				int elemByteW = memPortStrm.m_pStrm->m_elemByteW;
				int qwElemCnt = memPortStrm.m_pStrm->m_qwElemCnt;
				int bufPtrW = memPortStrm.m_pStrm->m_bufPtrW;

				string strmName = memPortStrm.m_pStrm->m_name.size() == 0 ? "" : "_" + memPortStrm.m_pStrm->m_name.AsStr();
				string strmIdStr = memPortStrm.m_pStrm->m_strmCnt.size() == 0 ? "" : VA("[%d]", memPortStrm.m_strmIdx);

				if (modMemPort.m_rdStrmCnt > 1) {
					strmPostInstr.Append("\t\tcase %d:\n", mifStrmIdx);
					strmPostInstr.Append("\t\t\t{\n");
				}

				strmPostInstr.Append("%s\t\tCRdStrm%s_rdTid rdRspTid;\n", tabs.c_str(), strmName.c_str());
				strmPostInstr.Append("%s\t\trdRspTid.m_tid = r_mifTo%sP%d_rdRsp.m_tid;\n", tabs.c_str(), mod.m_modName.Uc().c_str(), memPortIdx);
				if (mod.m_clkRate == eClk1x || !bMultiQwReq)
					strmPostInstr.Append("%s\t\tc_rdStrm%s_rspWrPtr%s = rdRspTid.m_rspBufWrPtr + (rdRspTid.m_rspQwIdx << %d);\n",
					tabs.c_str(), strmName.c_str(), strmIdStr.c_str(), 3 - elemByteW);
				if (modMemPort.m_rdStrmCnt == 1)
					strmPostInstr.Append("\t\tassert(rdRspTid.m_mifStrmId == %d);\n", mifStrmIdx);
				strmPostInstr.Append("\n");

				strmPostInstr.Append("%s\t\tht_uint64 rdRspData = r_mifTo%sP%d_rdRsp.m_data;\n", tabs.c_str(), mod.m_modName.Uc().c_str(), memPortIdx);
				strmPostInstr.Append("\n");

				strmPostInstr.Append("%s\t\tCRdStrm%s_buf rdStrm%s_rspData[%d];\n", tabs.c_str(), strmName.c_str(), strmName.c_str(), qwElemCnt);
				for (int idx = 0; idx < qwElemCnt; idx += 1) {
					strmPostInstr.Append("%s\t\trdStrm%s_rspData[%d].m_data = rdRspData(%d,%d);\n",
						tabs.c_str(), strmName.c_str(), idx, 64 / qwElemCnt*(idx + 1) - 1, 64 / qwElemCnt*idx);
					strmPostInstr.Append("%s\t\trdStrm%s_rspData[%d].m_toggle = c_rdStrm%s_rspWrPtr%s(%d,%d) ^ 1;\n",
						tabs.c_str(), strmName.c_str(), idx, strmName.c_str(), strmIdStr.c_str(), bufPtrW - 1, bufPtrW - 1);
				}
				strmPostInstr.Append("\n");

				strmPostInstr.Append("%s\t\tm_rdStrm%s_buf%s.write_addr( c_rdStrm%s_rspWrPtr%s(%d,%d) );\n",
					tabs.c_str(), strmName.c_str(), strmIdStr.c_str(), strmName.c_str(), strmIdStr.c_str(), bufPtrW - 2, 3 - elemByteW);

				if (bMultiQwReq) {
					strmPostInstr.Append("\n");
					strmPostInstr.Append("%s\t\tif (rdRspTid.m_fstQwIdx <= rdRspTid.m_rspQwIdx && rdRspTid.m_rspQwIdx <= rdRspTid.m_lstQwIdx) {\n", tabs.c_str());
					tabs += "\t";
				}

				strmPostInstr.Append("%s\t\tc_rdStrm%s_rspWrRdy%s = true;\n", tabs.c_str(), strmName.c_str(), strmIdStr.c_str());

				for (int idx = 0; idx < qwElemCnt; idx += 1) {
					if (elemByteW == 3)
						strmPostInstr.Append("%s\t\tm_rdStrm%s_buf%s.write_mem(rdStrm%s_rspData[%d]);\n",
						tabs.c_str(), strmName.c_str(), strmIdStr.c_str(), strmName.c_str(), idx);
					else
						strmPostInstr.Append("%s\t\tm_rdStrm%s_buf%s.write_mem(%d, rdStrm%s_rspData[%d]);\n",
						tabs.c_str(), strmName.c_str(), strmIdStr.c_str(), idx, strmName.c_str(), idx);
				}

				if (bMultiQwReq) {
					tabs.erase(0, 1);
					strmPostInstr.Append("%s\t\t}\n", tabs.c_str());
				}

				if (modMemPort.m_rdStrmCnt > 1) {
					strmPostInstr.Append("\t\t\t}\n");
					strmPostInstr.Append("\t\t\tbreak;\n");
				}
			}

			if (modMemPort.m_rdStrmCnt > 1) {
				strmPostInstr.Append("\t\tdefault:\n");
				strmPostInstr.Append("\t\t\tassert(0);\n");
				strmPostInstr.Append("\t\t}\n");
			}

			strmPostInstr.Append("\t}\n");
			strmPostInstr.Append("\n");
			strmPostInstr.Append("\n");
		}

		if (modMemPort.m_bWrite) {

			strmPostInstr.Append("\tif (i_mifTo%sP%d_wrRspRdy) {\n", mod.m_modName.Uc().c_str(), memPortIdx);
			strmPostInstr.Append("\t\tht_uint32 %swrRspTid = i_mifTo%sP%d_wrRspTid.read();\n",
				modMemPort.m_wrStrmCnt > 1 ? "" : "ht_noload ", mod.m_modName.Uc().c_str(), memPortIdx);

			if (modMemPort.m_wrStrmCnt > 1) {
				strmPostInstr.Append("\n");
				strmPostInstr.Append("\t\tswitch (wrRspTid(3,0)) {\n");
			}

			for (size_t mifStrmIdx = 0; mifStrmIdx < modMemPort.m_strmList.size(); mifStrmIdx += 1) {
				CMemPortStrm & memPortStrm = modMemPort.m_strmList[mifStrmIdx];

				if (memPortStrm.m_bRead) continue;

				string strmName = memPortStrm.m_pStrm->m_name.size() == 0 ? "" : "_" + memPortStrm.m_pStrm->m_name.AsStr();
				string strmIdStr = memPortStrm.m_pStrm->m_strmCnt.size() == 0 ? "" : VA("[%d]", memPortStrm.m_strmIdx);

				int rspGrpBits = memPortStrm.m_pStrm->m_rspGrpW.AsInt();

				if (modMemPort.m_wrStrmCnt == 1) {
					strmPostInstr.Append("\t\tassert(wrRspTid(3,0) == %d);\n", mifStrmIdx);
					if (rspGrpBits > 0) {
						strmPostInstr.Append("\t\tht_uint%d rspGrpId = wrRspTid(%d, 4);\n", rspGrpBits, 4 + rspGrpBits - 1);
						strmPostInstr.Append("\t\tc_wrStrm%s_rspGrpCnt[rspGrpId] -= 1u;\n", strmName.c_str());
					} else
						strmPostInstr.Append("\t\tc_wrStrm%s_rspGrpCnt -= 1u;\n", strmName.c_str());
				} else {
					strmPostInstr.Append("\t\tcase %d:\n", mifStrmIdx);
					strmPostInstr.Append("\t\t{\n");
					if (rspGrpBits > 0) {
						strmPostInstr.Append("\t\t\tht_uint%d rspGrpId = wrRspTid(%d, 4);\n", rspGrpBits, 4 + rspGrpBits - 1);
						strmPostInstr.Append("\t\t\tc_wrStrm%s_rspGrpCnt[rspGrpId] -= 1u;\n", strmName.c_str());
					} else
						strmPostInstr.Append("\t\t\tc_wrStrm%s_rspGrpCnt -= 1u;\n", strmName.c_str());
					strmPostInstr.Append("\t\t\tbreak;\n");
					strmPostInstr.Append("\t\t}\n");
				}
			}

			if (modMemPort.m_wrStrmCnt > 1) {
				strmPostInstr.Append("\t\tdefault:\n");
				strmPostInstr.Append("\t\t\tassert(0);\n");
				strmPostInstr.Append("\t\t}\n");
			}

			strmPostInstr.Append("\t}\n");
			strmPostInstr.Append("\n");
		}
	}

	g_appArgs.GetDsnRpt().EndLevel();
}
