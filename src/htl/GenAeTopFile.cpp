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

void CDsnInfo::GenerateAeTopFile()
{
	string fileName = g_appArgs.GetOutputFolder() + "/PersAeTop.sc";

	CHtFile scFile(fileName, "w");

	GenerateBanner(scFile, fileName.c_str(), false);

	HtiFile::CMsgIntfConn * pMicAeNext = 0;
	HtiFile::CMsgIntfConn * pMicAePrev = 0;
	for (size_t connIdx = 0; connIdx < getMsgIntfConnListSize(); connIdx += 1) {
		HtiFile::CMsgIntfConn * pMsgIntfConn = getMsgIntfConn(connIdx);

		if (pMsgIntfConn->m_aeNext)
			pMicAeNext = pMsgIntfConn;
		if (pMsgIntfConn->m_aePrev)
			pMicAePrev = pMsgIntfConn;
	}

	fprintf(scFile, "#include \"Ht.h\"\n");
	fprintf(scFile, "#include \"Pers%sTop.h\"\n", m_unitName.Uc().c_str());

	if (pMicAeNext) {
		fprintf(scFile, "#include \"PersMonSb.h\"\n");
		fprintf(scFile, "#include \"PersMipSb.h\"\n");
	}

	if (pMicAePrev) {
		fprintf(scFile, "#include \"PersMopSb.h\"\n");
		fprintf(scFile, "#include \"PersMinSb.h\"\n");
	}

	//if (!pMicAeNext || !pMicAePrev) {
	//	fprintf(scFile, "#include \"sysc/PersMiStub.h\"\n");
	//	fprintf(scFile, "#include \"sysc/PersMoStub.h\"\n");
	//}

	fprintf(scFile, "\n");

	fprintf(scFile, "#define %s_MIF_CNT %d\n", m_unitName.Upper().c_str(), (int)m_mifInstList.size());
	fprintf(scFile, "#define AE_XBAR_STUB_START (HT_UNIT_CNT * %s_MIF_CNT)\n", m_unitName.Upper().c_str());
	fprintf(scFile, "#define AE_XBAR_STUB_CNT (SYSC_AE_MEM_CNT - AE_XBAR_STUB_START)\n");
	fprintf(scFile, "\n");

	fprintf(scFile, "SC_MODULE(CPersAeTop) {\n");
	fprintf(scFile, "\n");
	fprintf(scFile, "\tht_attrib(keep_hierarchy, CPersAeTop, \"true\");\n");
	fprintf(scFile, "\n");
	fprintf(scFile, "\tsc_signal<uint8_t> aeUnitId[HT_UNIT_CNT];\n");
	fprintf(scFile, "\n");

	fprintf(scFile, "\tCPers%sTop * pPers%sTop[HT_UNIT_CNT];\n", m_unitName.Uc().c_str(), m_unitName.Uc().c_str());
	fprintf(scFile, "\t#if (AE_XBAR_STUB_CNT > 0)\n");
    fprintf(scFile, "\tCPersXbarStub * pPersXbarStub[AE_XBAR_STUB_CNT];\n");
	fprintf(scFile, "\t#endif\n");
    fprintf(scFile, "\tCPersUnitCnt * pPersUnitCnt;\n");

	if (pMicAeNext) {
		fprintf(scFile, "\tCPersMonSb * pPersMonSb;\n");
		fprintf(scFile, "\tCPersMipSb * pPersMipSb;\n");
	} else {
		fprintf(scFile, "\tCPersMoStub * pPersMonStub;\n");
		fprintf(scFile, "\tCPersMiStub * pPersMipStub;\n");
	}

	if (pMicAePrev) {
		fprintf(scFile, "\tCPersMopSb * pPersMopSb;\n");
		fprintf(scFile, "\tCPersMinSb * pPersMinSb;\n");
	} else {
		fprintf(scFile, "\tCPersMoStub * pPersMopStub;\n");
		fprintf(scFile, "\tCPersMiStub * pPersMinStub;\n");
	}

	fprintf(scFile, "\n");

    fprintf(scFile, "\tSC_CTOR(CPersAeTop) {\n");
	fprintf(scFile, "\n");

	fprintf(scFile, "\t\tpPersUnitCnt = new CPersUnitCnt(\"PersUnitCnt\");\n");
	fprintf(scFile, "\n");

	fprintf(scFile, "\t\tfor (int i = 0; i < HT_UNIT_CNT; i += 1) {\n");
	fprintf(scFile, "\t\t\taeUnitId[i] = i;\n");
	fprintf(scFile, "\n");

	fprintf(scFile, "\t\t\tpPers%sTop[i] = new CPers%sTop(\"Pers%sTop[%%i]\");\n",
		m_unitName.Uc().c_str(), m_unitName.Uc().c_str(), m_unitName.Uc().c_str());

	fprintf(scFile, "\t\t\tpPers%sTop[i]->i_aeUnitId(aeUnitId[i]);\n", m_unitName.Uc().c_str());
	fprintf(scFile, "\t\t\tpPers%sTop[i]->o_hifToDisp_busy(hifToDisp_busy[i]);\n", m_unitName.Uc().c_str());

	for (int mifIdx = 0; mifIdx < (int)m_mifInstList.size(); mifIdx += 1) {
		int mifId = m_mifInstList[mifIdx].m_mifId;

		fprintf(scFile, "\t\t\tpPers%sTop[i]->i_xbar%dToMif%d_rdRsp(i_xbarToMif_rdRsp[i * %s_MIF_CNT + %d]);\n",
			m_unitName.Uc().c_str(), mifId, mifId, m_unitName.Upper().c_str(), mifIdx);
		fprintf(scFile, "\t\t\tpPers%sTop[i]->i_xbar%dToMif%d_rdRspRdy(i_xbarToMif_rdRspRdy[i * %s_MIF_CNT + %d]);\n",
			m_unitName.Uc().c_str(), mifId, mifId, m_unitName.Upper().c_str(), mifIdx);
		fprintf(scFile, "\t\t\tpPers%sTop[i]->i_xbar%dToMif%d_reqFull(i_xbarToMif_reqFull[i * %s_MIF_CNT + %d]);\n",
			m_unitName.Uc().c_str(), mifId, mifId, m_unitName.Upper().c_str(), mifIdx);
		fprintf(scFile, "\t\t\tpPers%sTop[i]->i_xbar%dToMif%d_wrRspRdy(i_xbarToMif_wrRspRdy[i * %s_MIF_CNT + %d]);\n",
			m_unitName.Uc().c_str(), mifId, mifId, m_unitName.Upper().c_str(), mifIdx);
		fprintf(scFile, "\t\t\tpPers%sTop[i]->i_xbar%dToMif%d_wrRspTid(i_xbarToMif_wrRspTid[i * %s_MIF_CNT + %d]);\n",
			m_unitName.Uc().c_str(), mifId, mifId, m_unitName.Upper().c_str(), mifIdx);
		fprintf(scFile, "\t\t\tpPers%sTop[i]->o_mif%dToXbar%d_rdRspFull(o_mifToXbar_rdRspFull[i * %s_MIF_CNT + %d]);\n",
			m_unitName.Uc().c_str(), mifId, mifId, m_unitName.Upper().c_str(), mifIdx);
		fprintf(scFile, "\t\t\tpPers%sTop[i]->o_mif%dToXbar%d_req(o_mifToXbar_req[i * %s_MIF_CNT + %d]);\n",
			m_unitName.Uc().c_str(), mifId, mifId, m_unitName.Upper().c_str(), mifIdx);
		fprintf(scFile, "\t\t\tpPers%sTop[i]->o_mif%dToXbar%d_reqRdy(o_mifToXbar_reqRdy[i * %s_MIF_CNT + %d]);\n",
			m_unitName.Uc().c_str(), mifId, mifId, m_unitName.Upper().c_str(), mifIdx);
		fprintf(scFile, "\t\t\tpPers%sTop[i]->o_mif%dToXbar%d_wrRspFull(o_mifToXbar_wrRspFull[i * %s_MIF_CNT + %d]);\n",
			m_unitName.Uc().c_str(), mifId, mifId, m_unitName.Upper().c_str(), mifIdx);
		fprintf(scFile, "\n");
	}
	fprintf(scFile, "\t\t}\n");
	fprintf(scFile, "\n");

	// generated HTI connection message interfaces
	string monMsgRdy, monMsg, monMsgFull;
	string mopMsgRdy, mopMsg, mopMsgFull;
	string minMsgRdy, minMsg, minMsgFull;
	string mipMsgRdy, mipMsg, mipMsgFull;
	for (int unitIdx = 0; unitIdx < g_appArgs.GetAeUnitCnt(); unitIdx += 1) {
		for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
			CModule * pMod = m_modList[modIdx];

			for (size_t msgIdx = 0; msgIdx < pMod->m_msgIntfList.size(); msgIdx += 1) {
				CMsgIntf * pMsgIntf = pMod->m_msgIntfList[msgIdx];

				if (pMsgIntf->m_bAutoConn || !pMsgIntf->m_bAeConn) continue;

				int unitCnt = g_appArgs.GetAeUnitCnt();
				int modReplCnt = pMod->m_modInstList[0].m_replCnt;
				int msgFanCnt = max(1, pMsgIntf->m_fanCnt.AsInt());
				int msgDimenCnt = max(1, pMsgIntf->m_dimen.AsInt());

				int msgIntfInstCnt = unitCnt * modReplCnt * msgFanCnt * msgDimenCnt;

				HtlAssert(msgIntfInstCnt == (int)pMsgIntf->m_msgIntfInstList.size());

				if (pMsgIntf->m_dir == "out") {

					vector<CHtString> dimenList;
					int dimenCntIdx = -1;
					if (pMsgIntf->m_dimen.AsInt() > 0) {
						dimenCntIdx = 0;
						dimenList.push_back(pMsgIntf->m_dimen);
					}
					int fanCntIdx = -1;
					if (pMsgIntf->m_fanCnt.AsInt() > 0) {
						fanCntIdx = (int)dimenList.size();
						dimenList.push_back(pMsgIntf->m_fanCnt);
					}

					for (size_t modInstIdx = 0; modInstIdx < pMod->m_modInstList.size(); modInstIdx += 1) {
						CHtString &modInstName = pMod->m_modInstList[modInstIdx].m_instName;

						vector<int> portRefList(dimenList.size());
						do {
							string portIdx = IndexStr(portRefList);

							int msgDimenIdx = dimenCntIdx < 0 ? 0 : portRefList[dimenCntIdx];
							int msgFanIdx = fanCntIdx < 0 ? 0 : portRefList[fanCntIdx];
							int msgIntfInstIdx = ((unitIdx * modReplCnt + modInstIdx) * msgFanCnt + msgFanIdx) * msgDimenCnt + msgDimenIdx;
							CMsgIntfConn * pConn = pMsgIntf->m_msgIntfInstList[msgIntfInstIdx][0];

							string sigMsgRdy, sigMsg, sigMsgFull;
							if (pConn->m_aeNext) {
								sigMsgRdy = monMsgRdy = VA("%s%d%sToMonSb_miMsgRdy", m_unitName.Lc().c_str(), unitIdx, modInstName.Uc().c_str());
								sigMsg = monMsg = VA("%s%d%sToMonSb_miMsg", m_unitName.Lc().c_str(), unitIdx, modInstName.Uc().c_str());
								sigMsgFull = monMsgFull = VA("monSbTo%s%d%s_miMsgFull", m_unitName.Uc().c_str(), unitIdx, modInstName.Uc().c_str());
							} else if (pConn->m_aePrev) {
								sigMsgRdy = mopMsgRdy = VA("%s%d%sToMopSb_miMsgRdy", m_unitName.Lc().c_str(), unitIdx, modInstName.Uc().c_str());
								sigMsg = mopMsg = VA("%s%d%sToMopSb_miMsg", m_unitName.Lc().c_str(), unitIdx, modInstName.Uc().c_str());
								sigMsgFull = mopMsgFull = VA("mopSbTo%s%d%s_miMsgFull", m_unitName.Uc().c_str(), unitIdx, modInstName.Uc().c_str());
							} else {
								sigMsgRdy = VA("%s%d%sToAe_%sMsgRdy%s", m_unitName.Lc().c_str(), unitIdx, modInstName.Lc().c_str(), pMsgIntf->m_name.c_str(), portIdx.c_str());
								sigMsg = VA("%s%d%sToAe_%sMsg%s", m_unitName.Lc().c_str(), unitIdx, modInstName.Lc().c_str(), pMsgIntf->m_name.c_str(), portIdx.c_str());
								sigMsgFull = VA("%s%d%sToAe_%sMsgFull%s", m_unitName.Lc().c_str(), unitIdx, modInstName.Uc().c_str(), pMsgIntf->m_name.c_str(), portIdx.c_str());
							}

							fprintf(scFile, "\t\tpPers%sTop[%d]->o_%sToAe_%sMsgRdy%s(%s);\n",
								m_unitName.Uc().c_str(), unitIdx,
								modInstName.Lc().c_str(), pMsgIntf->m_name.c_str(), portIdx.c_str(),
								sigMsgRdy.c_str());

							fprintf(scFile, "\t\tpPers%sTop[%d]->o_%sToAe_%sMsg%s(%s);\n",
								m_unitName.Uc().c_str(), unitIdx,
								modInstName.Lc().c_str(), pMsgIntf->m_name.c_str(), portIdx.c_str(),
								sigMsg.c_str());

							if (pMsgIntf->m_bInboundQueue)
								fprintf(scFile, "\t\tpPers%sTop[%d]->i_aeTo%s_%sMsgFull%s(%s);\n",
									m_unitName.Uc().c_str(), unitIdx,
									modInstName.Uc().c_str(), pMsgIntf->m_name.c_str(), portIdx.c_str(),
									sigMsgFull.c_str());

						} while (DimenIter(dimenList, portRefList));

						fprintf(scFile, "\n");
					}
				} else {
					int msgInstSize = modReplCnt * msgFanCnt * msgDimenCnt;
					int msgInstIdx = unitIdx * msgInstSize;
					int msgInstCnt = msgInstIdx + msgInstSize;

					for ( ; msgInstIdx < msgInstCnt; msgInstIdx += 1) {
						CMsgIntfConn * pConn = pMsgIntf->m_msgIntfInstList[msgInstIdx][0];

						int inModInstIdx = max(0, pConn->m_inMsgIntf.m_replIdx);
						int outModInstIdx = max(0, pConn->m_outMsgIntf.m_replIdx);

						CHtString & inModInstName = pConn->m_inMsgIntf.m_pMod->m_modInstList[inModInstIdx].m_instName;
						CHtString & outModInstName = pConn->m_outMsgIntf.m_pMod->m_modInstList[outModInstIdx].m_instName;

						CMsgIntf * pInMsgIntf = pConn->m_inMsgIntf.m_pMsgIntf;
						CMsgIntf * pOutMsgIntf = pConn->m_outMsgIntf.m_pMsgIntf;

						int dimenIdx = pConn->m_inMsgIntf.m_msgIntfIdx.size()-1;
						string portIdx;
						if (pInMsgIntf->m_dimen.size() > 0)
							portIdx += VA("[%d]", pConn->m_inMsgIntf.m_msgIntfIdx[dimenIdx--]);
						if (pInMsgIntf->m_fanCnt.size() > 0)
							portIdx += VA("[%d]", pConn->m_inMsgIntf.m_msgIntfIdx[dimenIdx--]);

						dimenIdx = pConn->m_outMsgIntf.m_msgIntfIdx.size()-1;
						string signalIdx;
						if (pOutMsgIntf->m_dimen.size() > 0)
							signalIdx += VA("[%d]", pConn->m_outMsgIntf.m_msgIntfIdx[dimenIdx--]);
						if (pOutMsgIntf->m_fanCnt.size() > 0)
							signalIdx += VA("[%d]", pConn->m_outMsgIntf.m_msgIntfIdx[dimenIdx--]);

						int outUnitId = pInMsgIntf->m_msgIntfInstList[msgInstIdx][0]->m_outMsgIntf.m_unitIdx;

						string sigMsgRdy, sigMsg, sigMsgFull;
						if (pConn->m_aeNext) {
							sigMsgRdy = mipMsgRdy = VA("mipSbTo%s%d%s_miMsgRdy", m_unitName.Uc().c_str(), unitIdx, inModInstName.Uc().c_str());
							sigMsg = mipMsg = VA("mipSbTo%s%d%s_miMsg", m_unitName.Uc().c_str(), unitIdx, inModInstName.Uc().c_str());
							sigMsgFull = mipMsgFull = VA("%s%d%sToMipSb_miMsgFull", m_unitName.Lc().c_str(), unitIdx, inModInstName.Uc().c_str());
						} else if (pConn->m_aePrev) {
							sigMsgRdy = minMsgRdy = VA("minSbTo%s%d%s_miMsgRdy", m_unitName.Uc().c_str(), unitIdx, inModInstName.Uc().c_str());
							sigMsg = minMsg = VA("minSbTo%s%d%s_miMsg", m_unitName.Uc().c_str(), unitIdx, inModInstName.Uc().c_str());
							sigMsgFull = minMsgFull = VA("%s%d%sToMinSb_miMsgFull", m_unitName.Lc().c_str(), unitIdx, inModInstName.Uc().c_str());
						} else {
							sigMsgRdy = VA("%s%d%sToAe_%sMsgRdy%s", m_unitName.Lc().c_str(), outUnitId, outModInstName.Lc().c_str(), pOutMsgIntf->m_name.c_str(), signalIdx.c_str());
							sigMsg = VA("%s%d%sToAe_%sMsg%s", m_unitName.Lc().c_str(), outUnitId, outModInstName.Lc().c_str(), pOutMsgIntf->m_name.c_str(), signalIdx.c_str());
							sigMsgFull = VA("%s%d%sToAe_%sMsgFull%s", m_unitName.Lc().c_str(), outUnitId, outModInstName.Uc().c_str(), pOutMsgIntf->m_name.c_str(), signalIdx.c_str());
						}

						fprintf(scFile, "\t\tpPers%sTop[%d]->i_aeTo%s_%sMsgRdy%s(%s);\n",
							m_unitName.Uc().c_str(), unitIdx,
							inModInstName.Uc().c_str(), pMsgIntf->m_name.c_str(), portIdx.c_str(),
							sigMsgRdy.c_str());

						fprintf(scFile, "\t\tpPers%sTop[%d]->i_aeTo%s_%sMsg%s(%s);\n",
							m_unitName.Uc().c_str(), unitIdx,
							inModInstName.Uc().c_str(), pMsgIntf->m_name.c_str(), portIdx.c_str(),
							sigMsg.c_str());

						if (pOutMsgIntf->m_bInboundQueue)
							fprintf(scFile, "\t\tpPers%sTop[%d]->o_%sToAe_%sMsgFull%s(%s);\n",
								m_unitName.Uc().c_str(), unitIdx,
								inModInstName.Lc().c_str(), pMsgIntf->m_name.c_str(), portIdx.c_str(),
								sigMsgFull.c_str());
					}
					fprintf(scFile, "\n");
				}
			}
		}
	}

	if (pMicAeNext) {
		fprintf(scFile, "\t\tpPersMonSb = new CPersMonSb(\"PersMonSb\");\n");
		fprintf(scFile, "\t\tpPersMonSb->i_msgRdy(%s);\n", monMsgRdy.c_str());
		fprintf(scFile, "\t\tpPersMonSb->i_msg(%s);\n", monMsg.c_str());
		fprintf(scFile, "\t\tpPersMonSb->o_msgFull(%s);\n", monMsgFull.c_str());
		fprintf(scFile, "\t\tpPersMonSb->o_mon_valid(o_mon_valid);\n");
		fprintf(scFile, "\t\tpPersMonSb->o_mon_data(o_mon_data);\n");
		fprintf(scFile, "\t\tpPersMonSb->i_mon_stall(i_mon_stall);\n");
		fprintf(scFile, "\n");

		fprintf(scFile, "\t\tpPersMipSb = new CPersMipSb(\"PersMipSb\");\n");
		fprintf(scFile, "\t\tpPersMipSb->o_msgRdy(%s);\n", mipMsgRdy.c_str());
		fprintf(scFile, "\t\tpPersMipSb->o_msg(%s);\n", mipMsg.c_str());
		fprintf(scFile, "\t\tpPersMipSb->i_msgFull(%s);\n", mipMsgFull.c_str());
		fprintf(scFile, "\t\tpPersMipSb->i_mip_valid(i_mip_valid);\n");
		fprintf(scFile, "\t\tpPersMipSb->i_mip_data(i_mip_data);\n");
		fprintf(scFile, "\t\tpPersMipSb->o_mip_stall(o_mip_stall);\n");
		fprintf(scFile, "\n");

		GenAeNextMsgIntf(pMicAeNext);

	} else {
		fprintf(scFile, "\t\tpPersMonStub = new CPersMoStub(\"PersMonStub\");\n");
		fprintf(scFile, "\t\tpPersMonStub->o_mo_valid(o_mon_valid);\n");
		fprintf(scFile, "\t\tpPersMonStub->o_mo_data(o_mon_data);\n");
		fprintf(scFile, "\t\tpPersMonStub->i_mo_stall(i_mon_stall);\n");
		fprintf(scFile, "\n");

		fprintf(scFile, "\t\tpPersMipStub = new CPersMiStub(\"PersMipStub\");\n");
		fprintf(scFile, "\t\tpPersMipStub->i_mi_valid(i_mip_valid);\n");
		fprintf(scFile, "\t\tpPersMipStub->i_mi_data(i_mip_data);\n");
		fprintf(scFile, "\t\tpPersMipStub->o_mi_stall(o_mip_stall);\n");
		fprintf(scFile, "\n");
	}

	if (pMicAePrev) {
		fprintf(scFile, "\t\tpPersMopSb = new CPersMopSb(\"PersMopSb\");\n");
		fprintf(scFile, "\t\tpPersMopSb->i_msgRdy(%s);\n", mopMsgRdy.c_str());
		fprintf(scFile, "\t\tpPersMopSb->i_msg(%s);\n", mopMsg.c_str());
		fprintf(scFile, "\t\tpPersMopSb->o_msgFull(%s);\n", mopMsgFull.c_str());
		fprintf(scFile, "\t\tpPersMopSb->o_mop_valid(o_mop_valid);\n");
		fprintf(scFile, "\t\tpPersMopSb->o_mop_data(o_mop_data);\n");
		fprintf(scFile, "\t\tpPersMopSb->i_mop_stall(i_mop_stall);\n");
		fprintf(scFile, "\n");

		fprintf(scFile, "\t\tpPersMinSb = new CPersMinSb(\"PersMinSb\");\n");
		fprintf(scFile, "\t\tpPersMinSb->o_msgRdy(%s);\n", minMsgRdy.c_str());
		fprintf(scFile, "\t\tpPersMinSb->o_msg(%s);\n", minMsg.c_str());
		fprintf(scFile, "\t\tpPersMinSb->i_msgFull(%s);\n", minMsgFull.c_str());
		fprintf(scFile, "\t\tpPersMinSb->i_min_valid(i_min_valid);\n");
		fprintf(scFile, "\t\tpPersMinSb->i_min_data(i_min_data);\n");
		fprintf(scFile, "\t\tpPersMinSb->o_min_stall(o_min_stall);\n");
		fprintf(scFile, "\n");

		GenAePrevMsgIntf(pMicAePrev);

	} else {
		fprintf(scFile, "\t\tpPersMopStub = new CPersMoStub(\"PersMopStub\");\n");
		fprintf(scFile, "\t\tpPersMopStub->o_mo_valid(o_mop_valid);\n");
		fprintf(scFile, "\t\tpPersMopStub->o_mo_data(o_mop_data);\n");
		fprintf(scFile, "\t\tpPersMopStub->i_mo_stall(i_mop_stall);\n");
		fprintf(scFile, "\n");

		fprintf(scFile, "\t\tpPersMinStub = new CPersMiStub(\"PersMinStub\");\n");
		fprintf(scFile, "\t\tpPersMinStub->i_mi_valid(i_min_valid);\n");
		fprintf(scFile, "\t\tpPersMinStub->i_mi_data(i_min_data);\n");
		fprintf(scFile, "\t\tpPersMinStub->o_mi_stall(o_min_stall);\n");
		fprintf(scFile, "\n");
	}

	fprintf(scFile, "\t\tfor (int i = 0; i < AE_XBAR_STUB_CNT; i += 1) {\n");
	fprintf(scFile, "\t\t\tpPersXbarStub[i] = new CPersXbarStub(\"PersXbarStub[%%i]\");\n");

	fprintf(scFile, "\t\t\tpPersXbarStub[i]->i_xbarToStub_rdRsp(i_xbarToMif_rdRsp[i + AE_XBAR_STUB_START]);\n");
	fprintf(scFile, "\t\t\tpPersXbarStub[i]->i_xbarToStub_rdRspRdy(i_xbarToMif_rdRspRdy[i + AE_XBAR_STUB_START]);\n");
	fprintf(scFile, "\t\t\tpPersXbarStub[i]->i_xbarToStub_reqFull(i_xbarToMif_reqFull[i + AE_XBAR_STUB_START]);\n");
	fprintf(scFile, "\t\t\tpPersXbarStub[i]->i_xbarToStub_wrRspRdy(i_xbarToMif_wrRspRdy[i + AE_XBAR_STUB_START]);\n");
	fprintf(scFile, "\t\t\tpPersXbarStub[i]->i_xbarToStub_wrRspTid(i_xbarToMif_wrRspTid[i + AE_XBAR_STUB_START]);\n");
	fprintf(scFile, "\t\t\tpPersXbarStub[i]->o_stubToXbar_rdRspFull(o_mifToXbar_rdRspFull[i + AE_XBAR_STUB_START]);\n");
	fprintf(scFile, "\t\t\tpPersXbarStub[i]->o_stubToXbar_req(o_mifToXbar_req[i + AE_XBAR_STUB_START]);\n");
	fprintf(scFile, "\t\t\tpPersXbarStub[i]->o_stubToXbar_reqRdy(o_mifToXbar_reqRdy[i + AE_XBAR_STUB_START]);\n");
	fprintf(scFile, "\t\t\tpPersXbarStub[i]->o_stubToXbar_wrRspFull(o_mifToXbar_wrRspFull[i + AE_XBAR_STUB_START]);\n");
	fprintf(scFile, "\t\t}\n");
	fprintf(scFile, "\t}\n");
	fprintf(scFile, "};\n");

	scFile.FileClose();
}
