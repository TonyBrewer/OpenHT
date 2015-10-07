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

// Generate a module's message interface code

void CDsnInfo::InitAndValidateModMsg()
{
	// first initialize HtStrings
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];

		if (!mod.m_bIsUsed) continue;

		for (size_t outIdx = 0; outIdx < mod.m_msgIntfList.size(); outIdx += 1) {
			CMsgIntf * pMsgIntf = mod.m_msgIntfList[outIdx];

			pMsgIntf->m_dimen.InitValue(pMsgIntf->m_lineInfo, false, 0);
			pMsgIntf->m_fanCnt.InitValue(pMsgIntf->m_lineInfo, false, 0);
			pMsgIntf->m_queueW.InitValue(pMsgIntf->m_lineInfo, false, 0);
			pMsgIntf->m_reserve.InitValue(pMsgIntf->m_lineInfo, false, 0);
		}
	}

	// check that for each message interface there is a single writer and at least one reader
	//   also check that at most one reader has a queueW > 0
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];

		if (!mod.m_bIsUsed) continue;

		for (size_t outIdx = 0; outIdx < mod.m_msgIntfList.size(); outIdx += 1) {
			CMsgIntf * pOutMsgIntf = mod.m_msgIntfList[outIdx];

			for (size_t recordIdx = 0; recordIdx < m_recordList.size(); recordIdx += 1) {
				CRecord * pRecord = m_recordList[recordIdx];

				if (pRecord->m_typeName == pOutMsgIntf->m_pType->m_typeName)
					pRecord->m_bNeedIntf = true;
			}

			if (pOutMsgIntf->m_dir != "out") continue;

			pOutMsgIntf->m_srcFanout = pOutMsgIntf->m_fanCnt.AsInt() > 0 ? pOutMsgIntf->m_fanCnt.AsInt() : 1;
			pOutMsgIntf->m_srcReplCnt = (int)mod.m_modInstList.size();
			pOutMsgIntf->m_srcClkRate = mod.m_clkRate;

			if (!pOutMsgIntf->m_bAutoConn) continue;

			if (pOutMsgIntf->m_pIntfList == 0) {
				pOutMsgIntf->m_pIntfList = new vector<CMsgIntf *>;
				(*pOutMsgIntf->m_pIntfList).push_back(pOutMsgIntf);
			}

			// found an outbound message interface, check other interfaces
			pOutMsgIntf->m_bInboundQueue = false;
			bool bFoundIn = false;
			for (size_t modIdx2 = 0; modIdx2 < m_modList.size(); modIdx2 += 1) {
				CModule &mod2 = *m_modList[modIdx2];

				if (!mod2.m_bIsUsed) continue;

				for (size_t msgIdx = 0; msgIdx < mod2.m_msgIntfList.size(); msgIdx += 1) {
					CMsgIntf * pMsgIntf = mod2.m_msgIntfList[msgIdx];

					if (pMsgIntf->m_name != pOutMsgIntf->m_name || !pMsgIntf->m_bAutoConn) continue;

					if (pMsgIntf->m_pIntfList == 0) {
						pMsgIntf->m_pIntfList = pOutMsgIntf->m_pIntfList;
						(*pOutMsgIntf->m_pIntfList).push_back(pMsgIntf);
					}

					if (pMsgIntf->m_pType->m_typeName != pOutMsgIntf->m_pType->m_typeName) {
						ParseMsg(Error, pMsgIntf->m_lineInfo, "message interface name=%s has inconsistent type specified\n", pOutMsgIntf->m_name.c_str());
						ParseMsg(Info, pOutMsgIntf->m_lineInfo, "  previous message interface");
					}

					if (pMsgIntf->m_dimen.AsInt() != pOutMsgIntf->m_dimen.AsInt()) {
						ParseMsg(Error, pMsgIntf->m_lineInfo, "message interface name=%s has inconsistent dimen specified\n", pOutMsgIntf->m_name.c_str());
						ParseMsg(Info, pOutMsgIntf->m_lineInfo, "  previous message interface");
					}

					if (pMsgIntf->m_dir == "out") {
						if (pMsgIntf != pOutMsgIntf) {
							ParseMsg(Error, pMsgIntf->m_lineInfo,
								"message interface name=%s has multiple modules with dir=out",
								pMsgIntf->m_name.c_str());
							ParseMsg(Info, pOutMsgIntf->m_lineInfo, "  previous message interface");
						}
						pMsgIntf->m_srcFanout = pMsgIntf->m_fanCnt.AsInt() > 0 ? pMsgIntf->m_fanCnt.AsInt() : 1;
						pMsgIntf->m_srcReplCnt = (int)mod.m_modInstList.size();
						pMsgIntf->m_srcClkRate = mod2.m_clkRate;

						if (pMsgIntf->m_queueW.AsInt() > 0)
							ParseMsg(Error, pMsgIntf->m_lineInfo, "queueW > 0 for outbound message interfaces is not supported");
					} else {
						bFoundIn = true;
						pMsgIntf->m_outModName = mod.m_modName.AsStr();

						pMsgIntf->m_srcFanout = pOutMsgIntf->m_srcFanout;
						pMsgIntf->m_srcReplCnt = pOutMsgIntf->m_srcReplCnt;
						pMsgIntf->m_srcClkRate = pOutMsgIntf->m_srcClkRate;

						int dstFanin = pMsgIntf->m_fanCnt.AsInt() > 0 ? pMsgIntf->m_fanCnt.AsInt() : 1;
						int dstReplCnt = (int)mod2.m_modInstList.size();

						int totalReserve = pOutMsgIntf->m_reserve.AsInt() + 6;

						if (pMsgIntf->m_queueW.AsInt() > 0) {
							if (pOutMsgIntf->m_bInboundQueue)
								ParseMsg(Error, pMsgIntf->m_lineInfo, "queueW > 0 for multiple inbound message interfaces is not supported");
							else if (totalReserve > (1 << pMsgIntf->m_queueW.AsInt()))
								ParseMsg(Warning, pMsgIntf->m_lineInfo, "outbound reserved (%d) + internal reserved (6) exceeds queue size (%d)",
								pOutMsgIntf->m_reserve.AsInt(), 1 << pMsgIntf->m_queueW.AsInt());

							pOutMsgIntf->m_bInboundQueue = true;
						}

						if (pMsgIntf->m_srcFanout > 1 || pMsgIntf->m_srcReplCnt > 1 ||
							pOutMsgIntf->m_srcFanout > 1 || pOutMsgIntf->m_srcReplCnt > 1) {
							int srcFanout = pOutMsgIntf->m_srcFanout;
							int srcReplCnt = pOutMsgIntf->m_srcReplCnt;

#define SRC_FO 8
#define SRC_RC 4
#define DST_FO 2
#define DST_RC 1
							int mask = 0;
							mask |= srcFanout > 1 ? SRC_FO : 0;
							mask |= srcReplCnt > 1 ? SRC_RC : 0;
							mask |= dstFanin > 1 ? DST_FO : 0;
							mask |= dstReplCnt > 1 ? DST_RC : 0;

							switch (mask) {
							case 0:
							case DST_RC:
								break;
							case SRC_RC | DST_RC:
							case SRC_RC | DST_FO:
							case SRC_RC | DST_FO | DST_RC:
							case SRC_FO | DST_RC:
							case SRC_FO | SRC_RC | DST_RC:
							case SRC_FO | SRC_RC | DST_FO | DST_RC:
								if ((dstReplCnt * dstFanin) % (srcReplCnt * srcFanout) == 0)
									break;
								// else fall into error message
							case DST_FO:
							case DST_FO | DST_RC:
							case SRC_RC:
							case SRC_FO:
							case SRC_FO | DST_FO:
							case SRC_FO | DST_FO | DST_RC:
							case SRC_FO | SRC_RC:
							case SRC_FO | SRC_RC | DST_FO:
								ParseMsg(Error, pOutMsgIntf->m_lineInfo,
									"message interface name=%s has unsupported replCnt/fanin, replCnt/fanout values\n", pOutMsgIntf->m_name.c_str());
								ParseMsg(Info, pOutMsgIntf->m_lineInfo, "AddMsgIntf fanout=%d, module replication cnt=%d",
									srcFanout, srcReplCnt);
								ParseMsg(Info, pMsgIntf->m_lineInfo, "AddMsgIntf fanin=%d, module replication cnt=%d",
									dstFanin, dstReplCnt);
								break;
							default:
								assert(0);
							}
						}
					}
				}
			}

			if (!bFoundIn)
				ParseMsg(Error, pOutMsgIntf->m_lineInfo,
				"message interface name=%s has a module with dir=out, but no modules with dir=in",
				pOutMsgIntf->m_name.c_str());
		}
	}

	// now verify that all inputs have an output interface
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];

		if (!mod.m_bIsUsed) continue;

		for (size_t msgIdx = 0; msgIdx < mod.m_msgIntfList.size(); msgIdx += 1) {
			CMsgIntf * pMsgIntf = mod.m_msgIntfList[msgIdx];

			if (pMsgIntf->m_dir != "in" || !pMsgIntf->m_bAutoConn) continue;

			if (pMsgIntf->m_outModName.size() == 0)
				ParseMsg(Error, pMsgIntf->m_lineInfo,
				"message interface name=%s has a module with dir=in, but no modules with dir=out",
				pMsgIntf->m_name.c_str());
		}
	}

	// Now verify that non-autoConn message interfaces have connections specified in the HTI file
	//   first walk hti message interface connection list and link module instance message interfaces
	for (size_t connIdx = 0; connIdx < getMsgIntfConnListSize(); connIdx += 1) {
		HtiFile::CMsgIntfConn * pMsgIntfConn = getMsgIntfConn(connIdx);

		for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
			CModule &mod = *m_modList[modIdx];

			if (!mod.m_bIsUsed) continue;

			for (size_t msgIdx = 0; msgIdx < mod.m_msgIntfList.size(); msgIdx += 1) {
				CMsgIntf * pMsgIntf = mod.m_msgIntfList[msgIdx];

				if (pMsgIntf->m_dir != "out") continue;

				if (HtiFile::isMsgPathMatch(pMsgIntfConn->m_lineInfo, pMsgIntfConn->m_outMsgIntf, mod, pMsgIntf))
					SetMsgIntfConnUsedFlags(false, pMsgIntfConn, mod, pMsgIntf);
			}
		}
	}

	for (size_t connIdx = 0; connIdx < getMsgIntfConnListSize(); connIdx += 1) {
		HtiFile::CMsgIntfConn * pMsgIntfConn = getMsgIntfConn(connIdx);

		for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
			CModule &mod = *m_modList[modIdx];

			if (!mod.m_bIsUsed) continue;

			for (size_t msgIdx = 0; msgIdx < mod.m_msgIntfList.size(); msgIdx += 1) {
				CMsgIntf * pMsgIntf = mod.m_msgIntfList[msgIdx];

				if (pMsgIntf->m_dir != "in") continue;

				if (HtiFile::isMsgPathMatch(pMsgIntfConn->m_lineInfo, pMsgIntfConn->m_inMsgIntf, mod, pMsgIntf))
					SetMsgIntfConnUsedFlags(true, pMsgIntfConn, mod, pMsgIntf);
			}
		}
	}

	if (GetParseErrorCnt() > 0)
		ParseMsg(Fatal, "Previous errors prevent generation of files");

	// check that at most one aeNext and one aePrev is specified
	bool aePrev = false;
	bool aeNext = false;
	for (size_t connIdx = 0; connIdx < getMsgIntfConnListSize(); connIdx += 1) {
		HtiFile::CMsgIntfConn * pMsgIntfConn = getMsgIntfConn(connIdx);

		if (pMsgIntfConn->m_aeNext && pMsgIntfConn->m_aePrev)
			ParseMsg(Error, pMsgIntfConn->m_lineInfo, "AddMsgIntfConn with aeNext=true and aePrev=true is not supported");

		if (pMsgIntfConn->m_aeNext) {
			if (aeNext)
				ParseMsg(Error, pMsgIntfConn->m_lineInfo, "At most one AddMsgIntfConn with aeNext=true is supported");
			aeNext = true;
		}

		if (pMsgIntfConn->m_aePrev) {
			if (aePrev)
				ParseMsg(Error, pMsgIntfConn->m_lineInfo, "At most one AddMsgIntfConn with aePrev=true is supported");
			aePrev = true;
		}

		if ((pMsgIntfConn->m_aePrev || pMsgIntfConn->m_aeNext) && pMsgIntfConn->m_inMsgIntf.m_pMsgIntf->m_dimen.size() != 0)
			ParseMsg(Error, pMsgIntfConn->m_lineInfo, "message connection with aePrev=true or aeNext=true using a message interface with dimen specified not supported");

		if ((pMsgIntfConn->m_aePrev || pMsgIntfConn->m_aeNext) && pMsgIntfConn->m_inMsgIntf.m_pMsgIntf->m_fanCnt.size() != 0)
			ParseMsg(Error, pMsgIntfConn->m_lineInfo, "message connection with aePrev=true or aeNext=true using a message interface with fanIn or fanOut specified not supported");

		if ((pMsgIntfConn->m_aePrev || pMsgIntfConn->m_aeNext) && pMsgIntfConn->m_outMsgIntf.m_pMsgIntf->m_fanCnt.size() != 0)
			ParseMsg(Error, pMsgIntfConn->m_lineInfo, "message connection with aePrev=true or aeNext=true using a message interface with fanIn or fanOut specified not supported");

		if ((pMsgIntfConn->m_aePrev || pMsgIntfConn->m_aeNext) && pMsgIntfConn->m_inMsgIntf.m_pMod->m_modInstList.size() != 1)
			ParseMsg(Error, pMsgIntfConn->m_lineInfo, "message connection with aePrev=true or aeNext=true to a replicated module not supported");

		if ((pMsgIntfConn->m_aePrev || pMsgIntfConn->m_aeNext) && pMsgIntfConn->m_outMsgIntf.m_pMod->m_modInstList.size() != 1)
			ParseMsg(Error, pMsgIntfConn->m_lineInfo, "message connection with aePrev=true or aeNext=true to a replicated module not supported");

		if ((pMsgIntfConn->m_aePrev || pMsgIntfConn->m_aeNext) && pMsgIntfConn->m_inMsgIntf.m_pMod->m_clkRate == eClk2x)
			ParseMsg(Error, pMsgIntfConn->m_lineInfo, "message connection with aePrev=true or aeNext=true to a module with 2x clock not supported");

		if ((pMsgIntfConn->m_aePrev || pMsgIntfConn->m_aeNext) && pMsgIntfConn->m_outMsgIntf.m_pMod->m_clkRate == eClk2x)
			ParseMsg(Error, pMsgIntfConn->m_lineInfo, "message connection with aePrev=true or aeNext=true to a module with 2x clock not supported");
	}

	// check that all specified message interface connects were used
	for (size_t connIdx = 0; connIdx < getMsgIntfConnListSize(); connIdx += 1) {
		HtiFile::CMsgIntfConn * pMsgIntfConn = getMsgIntfConn(connIdx);

		if (!pMsgIntfConn->m_inMsgIntf.m_pMsgIntf || !pMsgIntfConn->m_outMsgIntf.m_pMsgIntf) {
			if (!pMsgIntfConn->m_outMsgIntf.m_pMsgIntf) {
				ParseMsg(Error, pMsgIntfConn->m_lineInfo, "outbound message unit/path was not found in design");
				static bool bErrorMsg = true;
				if (bErrorMsg) {
					bErrorMsg = false;
					ParseMsg(Info, pMsgIntfConn->m_lineInfo, "  unit: %s path: %s",
						pMsgIntfConn->m_outUnit.c_str(), pMsgIntfConn->m_outPath.c_str());
					ParseMsg(Info, pMsgIntfConn->m_lineInfo, "List of outbound message interfaces");
					for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
						CModule * pMod = m_modList[modIdx];

						for (size_t msgIdx = 0; msgIdx < pMod->m_msgIntfList.size(); msgIdx += 1) {
							CMsgIntf * pMsgIntf = pMod->m_msgIntfList[msgIdx];

							if (pMsgIntf->m_dir == "in") continue;

							string &modPath = pMod->m_modInstList[0]->m_modPaths[0];

							char const * pStr = modPath.c_str();
							while (*pStr != ':' && *pStr != '\0') pStr += 1;
							string unitName(modPath.c_str(), pStr - modPath.c_str());
							pStr += 1;
							string pathName = pStr;

							string unitIdxStr;
							if (g_appArgs.GetAeUnitCnt() > 1)
								unitIdxStr = VA("[0-%d]", g_appArgs.GetAeUnitCnt() - 1);

							int replCnt = pMod->m_modInstList[0]->m_replCnt;
							string replStr;
							if (replCnt > 1)
								replStr = VA("[0-%d]", replCnt - 1);

							string msgIdxStr;
							if (pMsgIntf->m_fanCnt.size() > 0)
								msgIdxStr = pMsgIntf->m_fanCnt.AsInt() == 1 ? "[0]" : VA("[0-%d]", pMsgIntf->m_fanCnt.AsInt() - 1);
							if (pMsgIntf->m_dimen.size() > 0)
								msgIdxStr += pMsgIntf->m_dimen.AsInt() == 1 ? "[0]" : VA("[0-%d]", pMsgIntf->m_dimen.AsInt() - 1);

							ParseMsg(Info, pMsgIntfConn->m_lineInfo, "  unit: %s%s path: %s%s/%s%s",
								unitName.c_str(), unitIdxStr.c_str(),
								pathName.c_str(), replStr.c_str(),
								pMsgIntf->m_name.c_str(), msgIdxStr.c_str());
						}
					}
				}
			}
			if (!pMsgIntfConn->m_inMsgIntf.m_pMsgIntf) {
				ParseMsg(Error, pMsgIntfConn->m_lineInfo, "inbound message unit/path was not found in design");
				static bool bErrorMsg = true;
				if (bErrorMsg) {
					bErrorMsg = false;
					ParseMsg(Info, pMsgIntfConn->m_lineInfo, "  unit: %s path: %s",
						pMsgIntfConn->m_inUnit.c_str(), pMsgIntfConn->m_inPath.c_str());
					ParseMsg(Info, pMsgIntfConn->m_lineInfo, "List of outbound message interfaces");
					for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
						CModule * pMod = m_modList[modIdx];

						for (size_t msgIdx = 0; msgIdx < pMod->m_msgIntfList.size(); msgIdx += 1) {
							CMsgIntf * pMsgIntf = pMod->m_msgIntfList[msgIdx];

							if (pMsgIntf->m_dir != "in") continue;

							string &modPath = pMod->m_modInstList[0]->m_modPaths[0];

							char const * pStr = modPath.c_str();
							while (*pStr != ':' && *pStr != '\0') pStr += 1;
							string unitName(modPath.c_str(), pStr - modPath.c_str());
							pStr += 1;
							string pathName = pStr;

							string unitIdxStr;
							if (g_appArgs.GetAeUnitCnt() > 1)
								unitIdxStr = VA("[0-%d]", g_appArgs.GetAeUnitCnt() - 1);

							int replCnt = pMod->m_modInstList[0]->m_replCnt;
							string replStr;
							if (replCnt > 1)
								replStr = VA("[0-%d]", replCnt - 1);

							string msgIdxStr;
							if (pMsgIntf->m_fanCnt.size() > 0)
								msgIdxStr = pMsgIntf->m_fanCnt.AsInt() == 1 ? "[0]" : VA("[0-%d]", pMsgIntf->m_fanCnt.AsInt() - 1);
							if (pMsgIntf->m_dimen.size() > 0)
								msgIdxStr += pMsgIntf->m_dimen.AsInt() == 1 ? "[0]" : VA("[0-%d]", pMsgIntf->m_dimen.AsInt() - 1);

							ParseMsg(Info, pMsgIntfConn->m_lineInfo, "  unit: %s%s path: %s%s/%s%s",
								unitName.c_str(), unitIdxStr.c_str(),
								pathName.c_str(), replStr.c_str(),
								pMsgIntf->m_name.c_str(), msgIdxStr.c_str());
						}
					}
				}
			}
		}
	}

	// check that if an inbound interface has a queue then it is the only inbound interface
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];

		if (!mod.m_bIsUsed) continue;

		for (size_t msgIdx = 0; msgIdx < mod.m_msgIntfList.size(); msgIdx += 1) {
			CMsgIntf * pMsgIntf = mod.m_msgIntfList[msgIdx];

			if (pMsgIntf->m_bAutoConn || pMsgIntf->m_dir == "in") continue;

			bool bError = false;
			for (size_t i = 0; i < pMsgIntf->m_msgIntfInstList.size(); i += 1) {
				vector<HtiFile::CMsgIntfConn *> & msgConnList = pMsgIntf->m_msgIntfInstList[i];
				for (size_t j = 0; j < msgConnList.size(); j += 1) {
					CMsgIntfConn * pConn = msgConnList[j];

					if (pConn->m_inMsgIntf.m_pMsgIntf->m_queueW.AsInt() > 0 && msgConnList.size() > 1)
						bError = true;
					break;
				}
			}

			if (bError)
				ParseMsg(Error, pMsgIntf->m_lineInfo, "outbound message interface connected to multiple inbound interfaces with queues");
		}
	}

	// check that all message interfaces are connected
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];

		if (!mod.m_bIsUsed) continue;

		for (size_t msgIdx = 0; msgIdx < mod.m_msgIntfList.size(); msgIdx += 1) {
			CMsgIntf * pMsgIntf = mod.m_msgIntfList[msgIdx];

			if (pMsgIntf->m_bAutoConn) continue;

			int unitCnt = g_appArgs.GetAeUnitCnt();
			int modReplCnt = mod.m_modInstList[0]->m_replCnt;
			int msgFanCnt = max(1, pMsgIntf->m_fanCnt.AsInt());
			int msgDimenCnt = max(1, pMsgIntf->m_dimen.AsInt());

			int msgIntfInstCnt = modReplCnt * msgFanCnt * msgDimenCnt;
			if (pMsgIntf->m_bAeConn) msgIntfInstCnt *= unitCnt;

			for (int idx = 0; idx < msgIntfInstCnt; idx += 1) {
				if (pMsgIntf->m_msgIntfInstList.size() >(size_t)idx && pMsgIntf->m_msgIntfInstList[idx].size() > 0) continue;

				int msgDimenIdx = idx % msgDimenCnt;
				int msgFanIdx = (idx / msgDimenCnt) % msgFanCnt;
				int modReplIdx = (idx / (msgDimenCnt * msgFanCnt)) % modReplCnt;
				int unitIdx = idx / (msgDimenCnt * msgFanCnt * modReplCnt);

				string &path = mod.m_modInstList[0]->m_modPaths[0];
				char const * pStr = path.c_str();
				while (*pStr != ':' && *pStr != '\0') pStr += 1;
				string unitStr = path.substr(0, pStr - path.c_str());
				pStr += 1;

				string msgUnit = VA("%s[%d]", unitStr.c_str(), unitIdx);
				string msgPath = pStr;
				if (modReplCnt > 1) msgPath += VA("[%d]", modReplIdx);
				msgPath += "/" + pMsgIntf->m_name;
				if (pMsgIntf->m_fanCnt.size() > 0) msgPath += VA("[%d]", msgFanIdx);
				if (pMsgIntf->m_dimen.size() > 0) msgPath += VA("[%d]", msgDimenIdx);

				ParseMsg(Error, pMsgIntf->m_lineInfo, "message interface not connected: unit %s, path %s", msgUnit.c_str(), msgPath.c_str());
			}
		}
	}
}

void CDsnInfo::SetMsgIntfConnUsedFlags(bool bInBound, CMsgIntfConn * pConn, CModule &mod, CMsgIntf * pMsgIntf)
{
	int unitCnt = g_appArgs.GetAeUnitCnt();
	int modReplCnt = mod.m_modInstList[0]->m_replCnt;
	int msgFanCnt = max(1, pMsgIntf->m_fanCnt.AsInt());
	int msgDimenCnt = max(1, pMsgIntf->m_dimen.AsInt());

	int msgIntfInstCnt = unitCnt * modReplCnt * msgFanCnt * msgDimenCnt;

	if (pMsgIntf->m_msgIntfInstList.size() == 0)
		pMsgIntf->m_msgIntfInstList.resize(msgIntfInstCnt);
	else
		HtlAssert((int)pMsgIntf->m_msgIntfInstList.size() == msgIntfInstCnt);

	CMsgIntfInfo & msgIntfInfo = bInBound ? pConn->m_inMsgIntf : pConn->m_outMsgIntf;

	int unitIdx = max(0, msgIntfInfo.m_unitIdx);
	int modReplIdx = max(0, msgIntfInfo.m_replIdx);
	int msgFanIdx = 0;

	int reqIdxCnt = (pMsgIntf->m_fanCnt.size() > 0 ? 1 : 0);

	if ((int)msgIntfInfo.m_msgIntfIdx.size() != reqIdxCnt) {
		string errorMsg;
		if (pMsgIntf->m_fanCnt.size() > 0)
			errorMsg = "expected fanin/fanout index";
		else
			errorMsg = "unexpected index";
		ParseMsg(Error, pConn->m_lineInfo, errorMsg.c_str());
	}

	if (pMsgIntf->m_fanCnt.size() > 0)
		msgFanIdx = msgIntfInfo.m_msgIntfIdx[0];

	if (unitIdx >= unitCnt) {
		ParseMsg(Error, pConn->m_lineInfo, "unit index out of range");
		return;
	}

	if (modReplIdx >= modReplCnt) {
		ParseMsg(Error, pConn->m_lineInfo, "module replication index out of range");
		return;
	}

	if (msgFanIdx >= msgFanCnt) {
		ParseMsg(Error, pConn->m_lineInfo, "message fanin/fanout index out of range");
		return;
	}

	for (int msgDimenIdx = 0; msgDimenIdx < msgDimenCnt; msgDimenIdx += 1) {
		int msgIntfInstIdx = ((unitIdx * modReplCnt + modReplIdx) * msgFanCnt + msgFanIdx) * msgDimenCnt + msgDimenIdx;

		if (bInBound && pMsgIntf->m_msgIntfInstList[msgIntfInstIdx].size() > 0)
			ParseMsg(Error, pConn->m_lineInfo, "connection to inbound message interface previously specified");

		pMsgIntf->m_msgIntfInstList[msgIntfInstIdx].push_back(pConn);
	}

	msgIntfInfo.m_pMsgIntf = pMsgIntf;
	msgIntfInfo.m_pMod = &mod;

	if (!bInBound && (pConn->m_inMsgIntf.m_unitIdx >= 0) != (pConn->m_outMsgIntf.m_unitIdx >= 0))
		ParseMsg(Error, pConn->m_lineInfo, "inconsistent specification of unit indexing (either both have a unit index or neither)");

	if (pConn->m_outMsgIntf.m_pMsgIntf == 0)
		// error cases may have zero m_pMsgIntf
		return;

	if (bInBound && (pConn->m_inMsgIntf.m_unitIdx >= 0 || pConn->m_outMsgIntf.m_unitIdx >= 0)) {
		pConn->m_inMsgIntf.m_pMsgIntf->m_bAeConn = true;
		pConn->m_outMsgIntf.m_pMsgIntf->m_bAeConn = true;
	}

	if (bInBound && pMsgIntf->m_queueW.AsInt() > 0)
		pConn->m_outMsgIntf.m_pMsgIntf->m_bInboundQueue = true;

	if (bInBound) {
		pMsgIntf->m_outModName = pConn->m_outMsgIntf.m_pMod->m_modName;
		if (pConn->m_pType && pConn->m_pType->m_typeName != pMsgIntf->m_pType->m_typeName)
			ParseMsg(Error, pConn->m_lineInfo, "incompatible message types: %s and %s", pConn->m_pType->m_typeName.c_str(), pMsgIntf->m_pType->m_typeName.c_str());
	} else {
		pConn->m_pType = pMsgIntf->m_pType;
	}
}

void CDsnInfo::GenModMsgStatements(CModule &mod)
{
	if (mod.m_msgIntfList.size() == 0)
		return;

	// message interface
	string unitNameUc = !g_appArgs.IsModuleUnitNamesEnabled() ? m_unitName.Uc() : "";

	bool bIn = false;
	for (size_t intfIdx = 0; intfIdx < mod.m_msgIntfList.size(); intfIdx += 1) {
		CMsgIntf * pMsgIntf = mod.m_msgIntfList[intfIdx];

		CHtCode & msgPreInstr = mod.m_clkRate == eClk2x ? m_msgPreInstr2x : m_msgPreInstr1x;
		CHtCode & msgPostInstr = mod.m_clkRate == eClk2x ? m_msgPostInstr2x : m_msgPostInstr1x;
		CHtCode & srcMsgPostInstr = pMsgIntf->m_srcClkRate == eClk2x ? m_msgPostInstr2x : m_msgPostInstr1x;
		CHtCode & msgReg = mod.m_clkRate == eClk2x ? m_msgReg2x : m_msgReg1x;
		CHtCode & msgPostReg = mod.m_clkRate == eClk2x ? m_msgPostReg2x : m_msgPostReg1x;
		CHtCode & msgOut = mod.m_clkRate == eClk2x ? m_msgOut2x : m_msgOut1x;

		string reset = mod.m_clkRate == eClk1x ? "r_reset1x" : "c_reset2x";

		if (pMsgIntf->m_dir == "in") {

			if (!bIn) {
				m_msgIoDecl.Append("\t// Inbound Message interfaces\n");
				g_appArgs.GetDsnRpt().AddLevel("Inbound Message\n");
				bIn = true;
			}

			string intfDecl;
			string intfParam;
			string intfParamIdx;
			vector<CHtString> dimenList;
			if (pMsgIntf->m_dimen.AsInt() > 0) {
				intfDecl = VA("[%d]", pMsgIntf->m_dimen.AsInt());
				intfParam = VA("ht_uint%d dimenIdx", FindLg2(pMsgIntf->m_dimen.AsInt() - 1));
				intfParamIdx = "[dimenIdx]";
				dimenList.push_back(pMsgIntf->m_dimen);
			}
			if (pMsgIntf->m_fanCnt.AsInt() > 0) {
				intfDecl += VA("[%d]", pMsgIntf->m_fanCnt.AsInt());
				if (intfParam.size() > 0) intfParam += ", ";
				intfParam += VA("ht_uint%d replIdx", FindLg2(pMsgIntf->m_fanCnt.AsInt() - 1));
				intfParamIdx += "[replIdx]";
				dimenList.push_back(pMsgIntf->m_fanCnt);
			}

			m_msgIoDecl.Append("\tsc_in<bool> i_%sToAu_%sMsgRdy%s;\n",
				pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfDecl.c_str());
			m_msgIoDecl.Append("\tsc_in<%s> i_%sToAu_%sMsg%s;\n",
				pMsgIntf->m_pType->m_typeName.c_str(), pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfDecl.c_str());

			if (pMsgIntf->m_queueW.AsInt() > 0)
				m_msgIoDecl.Append("\tsc_out<bool> o_auTo%s_%sMsgFull%s;\n",
				pMsgIntf->m_outModName.Uc().c_str(), pMsgIntf->m_name.c_str(), intfDecl.c_str());

			m_msgIoDecl.Append("\n");

			if (pMsgIntf->m_queueW.AsInt() == 0) {

				m_msgRegDecl.Append("\tbool r_%sToAu_%sMsgInRdy%s;\n",
					pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfDecl.c_str());
				m_msgRegDecl.Append("\t%s r_%sToAu_%sMsgIn%s;\n",
					pMsgIntf->m_pType->m_typeName.c_str(), pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfDecl.c_str());

			} else {
				m_msgRegDecl.Append("\tht_%s_que<%s, %d> m_%sToAu_%sMsgQue%s;\n",
					pMsgIntf->m_queueW.AsInt() > 6 ? "block" : "dist",
					pMsgIntf->m_pType->m_typeName.c_str(), pMsgIntf->m_queueW.AsInt(),
					pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfDecl.c_str());
				m_msgRegDecl.Append("\tbool r_auTo%s_%sMsgInFull%s;\n",
					pMsgIntf->m_outModName.Uc().c_str(), pMsgIntf->m_name.c_str(), intfDecl.c_str());
				m_msgRegDecl.Append("\tbool r_%sToAu_%sMsgRdy%s;\n",
					pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfDecl.c_str());
				m_msgRegDecl.Append("\t%s r_%sToAu_%sMsg%s;\n",
					pMsgIntf->m_pType->m_typeName.c_str(),
					pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfDecl.c_str());
				m_msgRegDecl.Append("\tbool c_%sToAu_%sMsgPop%s;\n",
					pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfDecl.c_str());
			}

			m_msgRegDecl.Append("\n");

			if (pMsgIntf->m_queueW.AsInt() == 0) {
					{
						vector<int> refList(dimenList.size());
						do {
							string intfIdx = IndexStr(refList);

							msgReg.Append("\tr_%sToAu_%sMsgInRdy%s = i_%sToAu_%sMsgRdy%s;\n",
								pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfIdx.c_str(),
								pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfIdx.c_str());

						} while (DimenIter(dimenList, refList));
					}

				{
					vector<int> refList(dimenList.size());
					do {
						string intfIdx = IndexStr(refList);

						msgReg.Append("\tr_%sToAu_%sMsgIn%s = i_%sToAu_%sMsg%s;\n",
							pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfIdx.c_str(),
							pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfIdx.c_str());

					} while (DimenIter(dimenList, refList));
				}

			} else {

					{
						vector<int> refList(dimenList.size());
						do {
							string intfIdx = IndexStr(refList);

							msgPreInstr.Append("\tc_%sToAu_%sMsgPop%s = false;\n",
								pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfIdx.c_str());

						} while (DimenIter(dimenList, refList));
					}
				msgPreInstr.Append("\n");

				{
					vector<int> refList(dimenList.size());
					do {
						string intfIdx = IndexStr(refList);

						srcMsgPostInstr.Append("\tif (i_%sToAu_%sMsgRdy%s)\n",
							pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfIdx.c_str());
						srcMsgPostInstr.Append("\t\tm_%sToAu_%sMsgQue%s.push(i_%sToAu_%sMsg%s);\n",
							pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfIdx.c_str(),
							pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfIdx.c_str());

					} while (DimenIter(dimenList, refList));
				}

				{
					msgPostInstr.Append("\tbool c_auTo%s_%sMsgFull%s;\n",
						pMsgIntf->m_outModName.Uc().c_str(), pMsgIntf->m_name.c_str(), intfDecl.c_str());

					// find max outbound reserved parameter
					int maxReserved = 0;
					if (pMsgIntf->m_bAutoConn) {
						for (size_t i = 0; i < pMsgIntf->m_pIntfList->size(); i += 1)
							maxReserved = max(maxReserved, (*pMsgIntf->m_pIntfList)[i]->m_reserve.AsInt());
					} else {
						for (size_t i = 0; i < pMsgIntf->m_msgIntfInstList.size(); i += 1) {
							vector<HtiFile::CMsgIntfConn *> & msgConnList = pMsgIntf->m_msgIntfInstList[i];
							for (size_t j = 0; j < msgConnList.size(); j += 1) {
								CMsgIntfConn * pConn = msgConnList[j];

								maxReserved = max(maxReserved, pConn->m_outMsgIntf.m_pMsgIntf->m_reserve.AsInt());
							}
						}
					}

					HtlAssert(maxReserved < (1 << pMsgIntf->m_queueW.AsInt()));

					vector<int> refList(dimenList.size());
					do {
						string intfIdx = IndexStr(refList);

						msgPostInstr.Append("\tc_auTo%s_%sMsgFull%s = m_%sToAu_%sMsgQue%s.full(%d);\n",
							pMsgIntf->m_outModName.Uc().c_str(), pMsgIntf->m_name.c_str(), intfIdx.c_str(),
							pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfIdx.c_str(),
							maxReserved + 6);

					} while (DimenIter(dimenList, refList));
				}

				{
					msgPostInstr.Append("\t%s c_%sToAu_%sMsg%s;\n",
						pMsgIntf->m_pType->m_typeName.c_str(),
						pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfDecl.c_str());

					vector<int> refList(dimenList.size());
					do {
						string intfIdx = IndexStr(refList);

						msgPostInstr.Append("\tc_%sToAu_%sMsg%s = r_%sToAu_%sMsg%s;\n",
							pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfIdx.c_str(),
							pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfIdx.c_str());

					} while (DimenIter(dimenList, refList));
				}

				{
					msgPostInstr.Append("\tbool c_%sToAu_%sMsgRdy%s;\n",
						pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfDecl.c_str());

					vector<int> refList(dimenList.size());
					do {
						string intfIdx = IndexStr(refList);

						msgPostInstr.Append("\tc_%sToAu_%sMsgRdy%s = !m_%sToAu_%sMsgQue%s.empty() || (!c_%sToAu_%sMsgPop%s && r_%sToAu_%sMsgRdy%s);\n",
							pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfIdx.c_str(),
							pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfIdx.c_str(),
							pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfIdx.c_str(),
							pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfIdx.c_str());

					} while (DimenIter(dimenList, refList));
				}

				{
					vector<int> refList(dimenList.size());
					do {
						string intfIdx = IndexStr(refList);

						msgPostInstr.Append("\tif (c_%sToAu_%sMsgPop%s || !r_%sToAu_%sMsgRdy%s)\n",
							pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfIdx.c_str(),
							pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfIdx.c_str());

						msgPostInstr.Append("\t\tc_%sToAu_%sMsg%s = m_%sToAu_%sMsgQue%s.front();\n",
							pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfIdx.c_str(),
							pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfIdx.c_str());

					} while (DimenIter(dimenList, refList));
				}

				{
					vector<int> refList(dimenList.size());
					do {
						string intfIdx = IndexStr(refList);

						msgPostInstr.Append("\tif (!m_%sToAu_%sMsgQue%s.empty() && (!r_%sToAu_%sMsgRdy%s || c_%sToAu_%sMsgPop%s))\n",
							pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfIdx.c_str(),
							pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfIdx.c_str(),
							pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfIdx.c_str());

						msgPostInstr.Append("\t\tm_%sToAu_%sMsgQue%s.pop();\n",
							pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfIdx.c_str());

					} while (DimenIter(dimenList, refList));
				}

				{
					HtdFile::EClkRate srcClkRate = eClkUnknown;
					if (pMsgIntf->m_bAutoConn) {
						srcClkRate = pMsgIntf->m_srcClkRate;
					} else {
						for (size_t i = 0; i < pMsgIntf->m_msgIntfInstList.size(); i += 1) {
							vector<HtiFile::CMsgIntfConn *> & msgConnList = pMsgIntf->m_msgIntfInstList[i];
							for (size_t j = 0; j < msgConnList.size(); j += 1) {
								CMsgIntfConn * pConn = msgConnList[j];

								if (srcClkRate == eClkUnknown)
									srcClkRate = pConn->m_outMsgIntf.m_pMod->m_clkRate;
								else if (srcClkRate != pConn->m_outMsgIntf.m_pMod->m_clkRate)
									ParseMsg(Fatal, pMsgIntf->m_lineInfo, "modules with connected output message interfaces have inconsistent clock rate");
							}
						}
					}

					vector<int> refList(dimenList.size());
					do {
						string intfIdx = IndexStr(refList);

						if (srcClkRate == mod.m_clkRate)
							msgReg.Append("\tm_%sToAu_%sMsgQue%s.clock(%s);\n",
							pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfIdx.c_str(), reset.c_str());
						else if (srcClkRate == eClk2x) {
							m_msgReg2x.Append("\tm_%sToAu_%sMsgQue%s.push_clock(c_reset2x);\n",
								pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfIdx.c_str());
							m_msgReg1x.Append("\tm_%sToAu_%sMsgQue%s.pop_clock(r_reset1x);\n",
								pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfIdx.c_str());
						} else {
							m_msgReg1x.Append("\tm_%sToAu_%sMsgQue%s.push_clock(r_reset1x);\n",
								pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfIdx.c_str());
							m_msgReg2x.Append("\tm_%sToAu_%sMsgQue%s.pop_clock(c_reset2x);\n",
								pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfIdx.c_str());
						}
					} while (DimenIter(dimenList, refList));
				}

				{
					vector<int> refList(dimenList.size());
					do {
						string intfIdx = IndexStr(refList);

						msgReg.Append("\tr_auTo%s_%sMsgInFull%s = c_auTo%s_%sMsgFull%s;\n",
							pMsgIntf->m_outModName.Uc().c_str(), pMsgIntf->m_name.c_str(), intfIdx.c_str(),
							pMsgIntf->m_outModName.Uc().c_str(), pMsgIntf->m_name.c_str(), intfIdx.c_str());

					} while (DimenIter(dimenList, refList));
				}

				{
					vector<int> refList(dimenList.size());
					do {
						string intfIdx = IndexStr(refList);

						msgReg.Append("\tr_%sToAu_%sMsgRdy%s = !%s && c_%sToAu_%sMsgRdy%s;\n",
							pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfIdx.c_str(),
							reset.c_str(),
							pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfIdx.c_str());

					} while (DimenIter(dimenList, refList));
				}

				{
					vector<int> refList(dimenList.size());
					do {
						string intfIdx = IndexStr(refList);

						msgReg.Append("\tr_%sToAu_%sMsg%s = c_%sToAu_%sMsg%s;\n",
							pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfIdx.c_str(),
							pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfIdx.c_str());

					} while (DimenIter(dimenList, refList));
				}
			}

			msgReg.Append("\n");

			if (pMsgIntf->m_queueW.AsInt() > 0) {
				vector<int> refList(dimenList.size());
				do {
					string intfIdx = IndexStr(refList);

					msgOut.Append("\to_auTo%s_%sMsgFull%s = r_auTo%s_%sMsgInFull%s;\n",
						pMsgIntf->m_outModName.Uc().c_str(), pMsgIntf->m_name.c_str(), intfIdx.c_str(),
						pMsgIntf->m_outModName.Uc().c_str(), pMsgIntf->m_name.c_str(), intfIdx.c_str());

				} while (DimenIter(dimenList, refList));

				msgOut.Append("\n");
			}

			string vcdModName = VA("Pers%s", mod.m_modName.Uc().c_str());
			m_msgRegDecl.Append("\tbool ht_noload c_RecvMsgBusy_%s%s;\n", pMsgIntf->m_name.c_str(), intfDecl.c_str());

			{
				vector<int> refList(dimenList.size());
				do {
					string intfIdx = IndexStr(refList);
					string intfParamIdx = IndexStr(refList, -1, 0, true);

					GenModTrace(eVcdUser, vcdModName, VA("RecvMsgBusy_%s(%s)", pMsgIntf->m_name.c_str(), intfParamIdx.c_str()),
						VA("c_RecvMsgBusy_%s%s", pMsgIntf->m_name.c_str(), intfIdx.c_str()));

				} while (DimenIter(dimenList, refList));
			}

			{
				vector<int> refList(dimenList.size());
				do {
					string intfIdx = IndexStr(refList);

					if (pMsgIntf->m_queueW.AsInt() == 0)
						msgPostReg.Append("\tc_RecvMsgBusy_%s%s = !r_%sToAu_%sMsgInRdy%s;\n",
						pMsgIntf->m_name.c_str(), intfIdx.c_str(),
						pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfIdx.c_str());
					else
						msgPostReg.Append("\tc_RecvMsgBusy_%s%s = !r_%sToAu_%sMsgRdy%s;\n",
						pMsgIntf->m_name.c_str(), intfIdx.c_str(),
						pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfIdx.c_str());

				} while (DimenIter(dimenList, refList));
			}

			g_appArgs.GetDsnRpt().AddItem("bool RecvMsgBusy_%s(%s)\n",
				pMsgIntf->m_name.c_str(), intfParam.c_str());
			m_msgFuncDecl.Append("\tbool RecvMsgBusy_%s(%s);\n",
				pMsgIntf->m_name.c_str(), intfParam.c_str());
			m_msgFuncDef.Append("bool CPers%s%s::RecvMsgBusy_%s(%s)\n",
				unitNameUc.c_str(), mod.m_modName.Uc().c_str(), pMsgIntf->m_name.c_str(), intfParam.c_str());
			m_msgFuncDef.Append("{\n");
			m_msgFuncDef.Append("\treturn c_RecvMsgBusy_%s%s;\n", pMsgIntf->m_name.c_str(), intfParamIdx.c_str());

			m_msgFuncDef.Append("}\n");
			m_msgFuncDef.Append("\n");

			{
				vector<int> refList(dimenList.size());
				do {
					string intfIdx = IndexStr(refList);
					string intfParamIdx = IndexStr(refList, -1, 0, true);

					if (pMsgIntf->m_queueW.AsInt() == 0)
						GenModTrace(eVcdUser, vcdModName, VA("RecvMsgReady_%s(%s)", pMsgIntf->m_name.c_str(), intfParamIdx.c_str()),
						VA("r_%sToAu_%sMsgInRdy%s", pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfIdx.c_str()));
					else
						GenModTrace(eVcdUser, vcdModName, VA("RecvMsgReady_%s(%s)", pMsgIntf->m_name.c_str(), intfParamIdx.c_str()),
						VA("r_%sToAu_%sMsgRdy%s", pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfIdx.c_str()));

				} while (DimenIter(dimenList, refList));
			}

			g_appArgs.GetDsnRpt().AddItem("bool RecvMsgReady_%s(%s)\n",
				pMsgIntf->m_name.c_str(), intfParam.c_str());
			m_msgFuncDecl.Append("\tbool RecvMsgReady_%s(%s);\n",
				pMsgIntf->m_name.c_str(), intfParam.c_str());
			m_msgFuncDef.Append("bool CPers%s%s::RecvMsgReady_%s(%s)\n",
				unitNameUc.c_str(), mod.m_modName.Uc().c_str(), pMsgIntf->m_name.c_str(), intfParam.c_str());
			m_msgFuncDef.Append("{\n");

			if (pMsgIntf->m_queueW.AsInt() == 0)
				m_msgFuncDef.Append("\treturn r_%sToAu_%sMsgInRdy%s;\n",
				pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfParamIdx.c_str());
			else
				m_msgFuncDef.Append("\treturn r_%sToAu_%sMsgRdy%s;\n",
				pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfParamIdx.c_str());

			m_msgFuncDef.Append("}\n");
			m_msgFuncDef.Append("\n");

			g_appArgs.GetDsnRpt().AddItem("%s PeekMsg_%s(%s)\n",
				pMsgIntf->m_pType->m_typeName.c_str(), pMsgIntf->m_name.c_str(), intfParam.c_str());
			m_msgFuncDecl.Append("\t%s PeekMsg_%s(%s);\n",
				pMsgIntf->m_pType->m_typeName.c_str(), pMsgIntf->m_name.c_str(), intfParam.c_str());
			m_msgFuncDef.Append("%s CPers%s%s::PeekMsg_%s(%s)\n",
				pMsgIntf->m_pType->m_typeName.c_str(), unitNameUc.c_str(), mod.m_modName.Uc().c_str(), pMsgIntf->m_name.c_str(), intfParam.c_str());
			m_msgFuncDef.Append("{\n");

			if (pMsgIntf->m_queueW.AsInt() == 0)
				m_msgFuncDef.Append("\treturn r_%sToAu_%sMsgIn%s;\n",
				pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfParamIdx.c_str());
			else {
				m_msgFuncDef.Append("\treturn r_%sToAu_%sMsg%s;\n",
					pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfParamIdx.c_str());
			}

			m_msgFuncDef.Append("}\n");
			m_msgFuncDef.Append("\n");

			g_appArgs.GetDsnRpt().AddItem("%s RecvMsg_%s(%s)\n",
				pMsgIntf->m_pType->m_typeName.c_str(), pMsgIntf->m_name.c_str(), intfParam.c_str());
			m_msgFuncDecl.Append("\t%s RecvMsg_%s(%s);\n",
				pMsgIntf->m_pType->m_typeName.c_str(), pMsgIntf->m_name.c_str(), intfParam.c_str());
			m_msgFuncDef.Append("%s CPers%s%s::RecvMsg_%s(%s)\n",
				pMsgIntf->m_pType->m_typeName.c_str(), unitNameUc.c_str(), mod.m_modName.Uc().c_str(), pMsgIntf->m_name.c_str(), intfParam.c_str());
			m_msgFuncDef.Append("{\n");

			if (pMsgIntf->m_queueW.AsInt() == 0)
				m_msgFuncDef.Append("\treturn r_%sToAu_%sMsgIn%s;\n",
				pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfParamIdx.c_str());
			else {
				m_msgFuncDef.Append("\tc_%sToAu_%sMsgPop%s = true;\n",
					pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfParamIdx.c_str());
				m_msgFuncDef.Append("\treturn r_%sToAu_%sMsg%s;\n",
					pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfParamIdx.c_str());
			}

			m_msgFuncDef.Append("}\n");
			m_msgFuncDef.Append("\n");
		}
	}

	if (bIn)
		g_appArgs.GetDsnRpt().EndLevel();


	bool bOut = false;
	for (size_t intfIdx = 0; intfIdx < mod.m_msgIntfList.size(); intfIdx += 1) {
		CMsgIntf * pMsgIntf = mod.m_msgIntfList[intfIdx];

		CHtCode & msgPreInstr = mod.m_clkRate == eClk2x ? m_msgPreInstr2x : m_msgPreInstr1x;
		CHtCode & msgReg = mod.m_clkRate == eClk2x ? m_msgReg2x : m_msgReg1x;
		CHtCode & msgOut = mod.m_clkRate == eClk2x ? m_msgOut2x : m_msgOut1x;

		if (pMsgIntf->m_dir != "in") {

			if (!bOut) {
				m_msgIoDecl.Append("\t// Outbound Message interfaces\n");
				g_appArgs.GetDsnRpt().AddLevel("Outbound Message\n");

				bOut = true;
			}

			string intfDecl;
			string intfParam;
			string intfParamNoload;
			string intfParamIdx;
			vector<CHtString> dimenList;
			if (pMsgIntf->m_dimen.AsInt() > 0) {
				intfDecl = VA("[%d]", pMsgIntf->m_dimen.AsInt());
				intfParam = VA("ht_uint%d dimenIdx", FindLg2(pMsgIntf->m_dimen.AsInt() - 1));
				intfParamNoload = VA("ht_noload ht_uint%d dimenIdx", FindLg2(pMsgIntf->m_dimen.AsInt() - 1));
				intfParamIdx = "[dimenIdx]";
				dimenList.push_back(pMsgIntf->m_dimen);
			}
			if (pMsgIntf->m_fanCnt.AsInt() > 0) {
				intfDecl += VA("[%d]", pMsgIntf->m_fanCnt.AsInt());
				if (intfParam.size() > 0) intfParam += ", ";
				intfParam += VA("ht_uint%d replIdx", FindLg2(pMsgIntf->m_fanCnt.AsInt() - 1));
				intfParamNoload += VA("ht_noload ht_uint%d replIdx", FindLg2(pMsgIntf->m_fanCnt.AsInt() - 1));
				intfParamIdx += "[replIdx]";
				dimenList.push_back(pMsgIntf->m_fanCnt);
			}

			m_msgIoDecl.Append("\tsc_out<bool> o_%sToAu_%sMsgRdy%s;\n",
				mod.m_modName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfDecl.c_str());
			m_msgIoDecl.Append("\tsc_out<%s> o_%sToAu_%sMsg%s;\n",
				pMsgIntf->m_pType->m_typeName.c_str(), mod.m_modName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfDecl.c_str());

			if (pMsgIntf->m_bInboundQueue)
				m_msgIoDecl.Append("\tsc_in<bool> i_auTo%s_%sMsgFull%s;\n",
				mod.m_modName.Uc().c_str(), pMsgIntf->m_name.c_str(), intfDecl.c_str());
			m_msgIoDecl.Append("\n");

			m_msgRegDecl.Append("\tbool c_%sToAu_%sMsgOutRdy%s;\n",
				mod.m_modName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfDecl.c_str());
			m_msgRegDecl.Append("\tbool r_%sToAu_%sMsgOutRdy%s;\n",
				mod.m_modName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfDecl.c_str());
			m_msgRegDecl.Append("\t%s c_%sToAu_%sMsgOut%s;\n",
				pMsgIntf->m_pType->m_typeName.c_str(), mod.m_modName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfDecl.c_str());
			m_msgRegDecl.Append("\t%s r_%sToAu_%sMsgOut%s;\n",
				pMsgIntf->m_pType->m_typeName.c_str(), mod.m_modName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfDecl.c_str());

			if (pMsgIntf->m_bInboundQueue)
				m_msgRegDecl.Append("\tbool r_auTo%s_%sMsgOutFull%s;\n",
				mod.m_modName.Uc().c_str(), pMsgIntf->m_name.c_str(), intfDecl.c_str());

			m_msgRegDecl.Append("\n");

			{
				vector<int> refList(dimenList.size());
				do {
					string intfIdx = IndexStr(refList);

					msgPreInstr.Append("\tc_%sToAu_%sMsgOutRdy%s = false;\n",
						mod.m_modName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfIdx.c_str());

				} while (DimenIter(dimenList, refList));
			}

			{
				vector<int> refList(dimenList.size());
				do {
					string intfIdx = IndexStr(refList);

					msgPreInstr.Append("\tc_%sToAu_%sMsgOut%s = 0;\n",
						mod.m_modName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfIdx.c_str());

				} while (DimenIter(dimenList, refList));
			}
			msgPreInstr.Append("\n");

			{
				vector<int> refList(dimenList.size());
				do {
					string intfIdx = IndexStr(refList);

					msgReg.Append("\tr_%sToAu_%sMsgOutRdy%s = c_%sToAu_%sMsgOutRdy%s;\n",
						mod.m_modName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfIdx.c_str(),
						mod.m_modName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfIdx.c_str());

				} while (DimenIter(dimenList, refList));
			}

			{
				vector<int> refList(dimenList.size());
				do {
					string intfIdx = IndexStr(refList);

					msgReg.Append("\tr_%sToAu_%sMsgOut%s = c_%sToAu_%sMsgOut%s;\n",
						mod.m_modName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfIdx.c_str(),
						mod.m_modName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfIdx.c_str());

				} while (DimenIter(dimenList, refList));
			}

			if (pMsgIntf->m_bInboundQueue) {
				vector<int> refList(dimenList.size());
				do {
					string intfIdx = IndexStr(refList);

					msgReg.Append("\tr_auTo%s_%sMsgOutFull%s = i_auTo%s_%sMsgFull%s;\n",
						mod.m_modName.Uc().c_str(), pMsgIntf->m_name.c_str(), intfIdx.c_str(),
						mod.m_modName.Uc().c_str(), pMsgIntf->m_name.c_str(), intfIdx.c_str());

				} while (DimenIter(dimenList, refList));
			}
			msgReg.Append("\n");

			{
				vector<int> refList(dimenList.size());
				do {
					string intfIdx = IndexStr(refList);

					msgOut.Append("\to_%sToAu_%sMsgRdy%s = r_%sToAu_%sMsgOutRdy%s;\n",
						mod.m_modName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfIdx.c_str(),
						mod.m_modName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfIdx.c_str());

				} while (DimenIter(dimenList, refList));
			}

			{
				vector<int> refList(dimenList.size());
				do {
					string intfIdx = IndexStr(refList);

					msgOut.Append("\to_%sToAu_%sMsg%s = r_%sToAu_%sMsgOut%s;\n",
						mod.m_modName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfIdx.c_str(),
						mod.m_modName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfIdx.c_str());

				} while (DimenIter(dimenList, refList));
			}
			msgOut.Append("\n");

			if (pMsgIntf->m_bInboundQueue) {
				string vcdModName = VA("Pers%s", mod.m_modName.Uc().c_str());
				vector<int> refList(dimenList.size());
				do {
					string intfIdx = IndexStr(refList);
					string intfParamIdx = IndexStr(refList, -1, 0, true);

					GenModTrace(eVcdUser, vcdModName, VA("SendMsgBusy_%s(%s)", pMsgIntf->m_name.c_str(), intfParamIdx.c_str()),
						VA("r_auTo%s_%sMsgOutFull%s", mod.m_modName.Uc().c_str(), pMsgIntf->m_name.c_str(), intfIdx.c_str()));

				} while (DimenIter(dimenList, refList));
			}

			g_appArgs.GetDsnRpt().AddItem("bool SendMsgBusy_%s(%s)\n",
				pMsgIntf->m_name.c_str(), intfParam.c_str());
			m_msgFuncDecl.Append("\tbool SendMsgBusy_%s(%s);\n",
				pMsgIntf->m_name.c_str(), intfParam.c_str());
			m_msgFuncDef.Append("bool CPers%s%s::SendMsgBusy_%s(%s)\n",
				unitNameUc.c_str(), mod.m_modName.Uc().c_str(), pMsgIntf->m_name.c_str(), intfParamNoload.c_str());
			m_msgFuncDef.Append("{\n");

			if (pMsgIntf->m_bInboundQueue)
				m_msgFuncDef.Append("\treturn r_auTo%s_%sMsgOutFull%s;\n",
				mod.m_modName.Uc().c_str(), pMsgIntf->m_name.c_str(), intfParamIdx.c_str());
			else
				m_msgFuncDef.Append("\treturn false;\n");

			m_msgFuncDef.Append("}\n");
			m_msgFuncDef.Append("\n");

			if (pMsgIntf->m_bInboundQueue) {
				string vcdModName = VA("Pers%s", mod.m_modName.Uc().c_str());
				vector<int> refList(dimenList.size());
				do {
					string intfIdx = IndexStr(refList);
					string intfParamIdx = IndexStr(refList, -1, 0, true);

					GenModTrace(eVcdUser, vcdModName, VA("SendMsgFull_%s(%s)", pMsgIntf->m_name.c_str(), intfParamIdx.c_str()),
						VA("r_auTo%s_%sMsgOutFull%s", mod.m_modName.Uc().c_str(), pMsgIntf->m_name.c_str(), intfIdx.c_str()));

				} while (DimenIter(dimenList, refList));
			}

			g_appArgs.GetDsnRpt().AddItem("bool SendMsgFull_%s(%s)\n",
				pMsgIntf->m_name.c_str(), intfParam.c_str());
			m_msgFuncDecl.Append("\tbool SendMsgFull_%s(%s);\n",
				pMsgIntf->m_name.c_str(), intfParam.c_str());
			m_msgFuncDef.Append("bool CPers%s%s::SendMsgFull_%s(%s)\n",
				unitNameUc.c_str(), mod.m_modName.Uc().c_str(), pMsgIntf->m_name.c_str(), intfParamNoload.c_str());
			m_msgFuncDef.Append("{\n");

			if (pMsgIntf->m_bInboundQueue)
				m_msgFuncDef.Append("\treturn r_auTo%s_%sMsgOutFull%s;\n",
				mod.m_modName.Uc().c_str(), pMsgIntf->m_name.c_str(), intfParamIdx.c_str());
			else
				m_msgFuncDef.Append("\treturn false;\n");

			m_msgFuncDef.Append("}\n");
			m_msgFuncDef.Append("\n");

			if (intfParam.size() > 0) intfParam += ", ";

			g_appArgs.GetDsnRpt().AddItem("void SendMsg_%s(%s%s msg)\n",
				pMsgIntf->m_name.c_str(), intfParam.c_str(), pMsgIntf->m_pType->m_typeName.c_str());
			m_msgFuncDecl.Append("\tvoid SendMsg_%s(%s%s msg);\n",
				pMsgIntf->m_name.c_str(), intfParam.c_str(), pMsgIntf->m_pType->m_typeName.c_str());
			m_msgFuncDef.Append("void CPers%s%s::SendMsg_%s(%s%s msg)\n",
				unitNameUc.c_str(), mod.m_modName.Uc().c_str(), pMsgIntf->m_name.c_str(), intfParam.c_str(), pMsgIntf->m_pType->m_typeName.c_str());
			m_msgFuncDef.Append("{\n");
			m_msgFuncDef.Append("\tc_%sToAu_%sMsgOutRdy%s = true;\n",
				mod.m_modName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfParamIdx.c_str());
			m_msgFuncDef.Append("\tc_%sToAu_%sMsgOut%s = msg;\n",
				mod.m_modName.Lc().c_str(), pMsgIntf->m_name.c_str(), intfParamIdx.c_str());
			m_msgFuncDef.Append("}\n");
			m_msgFuncDef.Append("\n");
		}
	}

	if (bOut)
		g_appArgs.GetDsnRpt().EndLevel();
}

void CDsnInfo::GenAeNextMsgIntf(HtiFile::CMsgIntfConn * pMicAeNext)
{
	string varName = "AeNextMsg";
	CHtString bitWidth;
	bitWidth.SetValue(0);

	int msgDwCnt = ((pMicAeNext->m_pType->GetPackedBitWidth()) + 31) / 32;
	int msgDwCntW = FindLg2(msgDwCnt - 1);
	{
		////////////////////////////////////////////////////////////////
		// Generate PersMonSb.h file

		string fileName = g_appArgs.GetOutputFolder() + "/PersMonSb.h";

		CHtFile incFile(fileName, "w");

		GenPersBanner(incFile, "", "MonSb", true);

		fprintf(incFile, "SC_MODULE(CPersMonSb) {\n");
		fprintf(incFile, "\tht_attrib(keep_hierarchy, CPersMonSb, \"true\");\n");
		fprintf(incFile, "\n");

		fprintf(incFile, "\tsc_in<bool> i_clock1x;\n");
		fprintf(incFile, "\tsc_in<bool> i_reset;\n");
		fprintf(incFile, "\n");

		fprintf(incFile, "\tsc_in<bool> i_msgRdy;\n");
		fprintf(incFile, "\tsc_in<%s> i_msg;\n", pMicAeNext->m_pType->m_typeName.c_str());
		fprintf(incFile, "\tsc_out<bool> o_msgFull;\n");
		fprintf(incFile, "\n");

		fprintf(incFile, "\tsc_out<bool> o_mon_valid;\n");
		fprintf(incFile, "\tsc_out<uint32_t> o_mon_data;\n");
		fprintf(incFile, "\tsc_in<bool> i_mon_stall;\n");
		fprintf(incFile, "\n");

		if (msgDwCnt > 1) {
			fprintf(incFile, "\tht_dist_que<%s, 5> m_msg;\n", pMicAeNext->m_pType->m_typeName.c_str());
			fprintf(incFile, "\tbool r_mon_valid;\n");
			fprintf(incFile, "\tuint32_t r_mon_data;\n");
			fprintf(incFile, "\tbool r_mon_stall;\n");
			fprintf(incFile, "\tht_uint%d r_msgIdx;\n", msgDwCntW);
			fprintf(incFile, "\tbool r_msgFull;\n");
			fprintf(incFile, "\tbool r_reset1x;\n");
		} else {
			fprintf(incFile, "\tbool r_mon_valid;\n");
			fprintf(incFile, "\t%s r_msg;\n", pMicAeNext->m_pType->m_typeName.c_str());
			fprintf(incFile, "\tbool r_msgFull;\n");
		}
		fprintf(incFile, "\n");

		fprintf(incFile, "\tvoid PersMonSb_1x();\n");
		fprintf(incFile, "\n");

		fprintf(incFile, "\tSC_CTOR(CPersMonSb) {\n");
		fprintf(incFile, "\t\tSC_METHOD(PersMonSb_1x);\n");
		fprintf(incFile, "\t\tsensitive << i_clock1x.pos();\n");
		fprintf(incFile, "\t}\n");

		if (g_appArgs.IsVcdAllEnabled()) {
			fprintf(incFile, "\n");

			fprintf(incFile, "#\tifndef _HTV\n");
			fprintf(incFile, "\tvoid start_of_simulation()\n");
			fprintf(incFile, "\t{\n");
			fprintf(incFile, "\t\tsc_trace(Ht::g_vcdp, r_mon_valid, (std::string)name() + \".r_mon_valid\");\n");
			if (msgDwCnt > 1) {
				fprintf(incFile, "\t\tsc_trace(Ht::g_vcdp, r_mon_data, (std::string)name() + \".r_mon_data\");\n");
				fprintf(incFile, "\t\tsc_trace(Ht::g_vcdp, r_mon_stall, (std::string)name() + \".r_mon_stall\");\n");
			} else
				fprintf(incFile, "\t\tsc_trace(Ht::g_vcdp, r_msg, (std::string)name() + \".r_msg\");\n");
			fprintf(incFile, "\t\tsc_trace(Ht::g_vcdp, r_msgFull, (std::string)name() + \".r_msgFull\");\n");
			if (msgDwCnt > 1) {
				fprintf(incFile, "\t\tsc_trace(Ht::g_vcdp, r_msgIdx, (std::string)name() + \".r_msgIdx\");\n");
				fprintf(incFile, "\t\tsc_trace(Ht::g_vcdp, r_reset1x, (std::string)name() + \".r_reset1x\");\n");
			}
			fprintf(incFile, "\t}\n");
			fprintf(incFile, "#\tendif\n");
		}

		fprintf(incFile, "};\n");

		incFile.FileClose();
	}

	{
		////////////////////////////////////////////////////////////////
		// Generate PersMonSb.cpp file

		string fileName = g_appArgs.GetOutputFolder() + "/PersMonSb.cpp";

		CHtFile cppFile(fileName, "w");

		GenPersBanner(cppFile, "", "MonSb", false);

		fprintf(cppFile, "void CPersMonSb::PersMonSb_1x()\n");
		fprintf(cppFile, "{\n");

		fprintf(cppFile, "\tunion %sUnion {\n", pMicAeNext->m_pType->m_typeName.c_str());
		fprintf(cppFile, "\t\t%s m_msg;\n", pMicAeNext->m_pType->m_typeName.c_str());
		if (msgDwCnt > 1)
			fprintf(cppFile, "\t\tuint32_t m_data[%d];\n", msgDwCnt);
		else
			fprintf(cppFile, "\t\tuint32_t m_data;\n");
		fprintf(cppFile, "\t};\n");
		fprintf(cppFile, "\n");

		if (msgDwCnt > 1) {
			fprintf(cppFile, "\tif (i_msgRdy)\n");
			fprintf(cppFile, "\t\tm_msg.push(i_msg);\n");
			fprintf(cppFile, "\n");

			fprintf(cppFile, "\tht_uint%d c_msgIdx = r_msgIdx;\n", msgDwCntW);
			fprintf(cppFile, "\tif (!m_msg.empty() && !r_mon_stall) {\n");
			fprintf(cppFile, "\t\tif (r_msgIdx == %d) {\n", msgDwCnt - 1);
			fprintf(cppFile, "\t\t\tc_msgIdx = 0;\n");
			fprintf(cppFile, "\t\t\tm_msg.pop();\n");
			fprintf(cppFile, "\t\t} else\n");
			fprintf(cppFile, "\t\t\tc_msgIdx = r_msgIdx + 1u;\n");
			fprintf(cppFile, "\t}\n");
		} else {
			fprintf(cppFile, "\tr_mon_valid = i_msgRdy;\n");
			fprintf(cppFile, "\tr_msg = i_msg;\n");
			fprintf(cppFile, "\tr_msgFull = i_mon_stall;\n");
		}
		fprintf(cppFile, "\n");

		fprintf(cppFile, "\t%sUnion %sUnion;\n", pMicAeNext->m_pType->m_typeName.c_str(), pMicAeNext->m_pType->m_typeName.c_str() + 1);
		if (msgDwCnt > 1)
			fprintf(cppFile, "\t%sUnion.m_msg = m_msg.front();\n", pMicAeNext->m_pType->m_typeName.c_str() + 1);
		else
			fprintf(cppFile, "\t%sUnion.m_msg = r_msg;\n", pMicAeNext->m_pType->m_typeName.c_str() + 1);
		fprintf(cppFile, "\n");

		if (msgDwCnt > 1) {
			fprintf(cppFile, "\tr_mon_valid = !m_msg.empty() && !r_mon_stall;\n");
			fprintf(cppFile, "\tr_mon_data = %sUnion.m_data[r_msgIdx];\n", pMicAeNext->m_pType->m_typeName.c_str() + 1);
			fprintf(cppFile, "\tr_mon_stall = i_mon_stall;\n");
			fprintf(cppFile, "\tr_msgFull = m_msg.size() > 28;\n");
			fprintf(cppFile, "\tr_msgIdx = r_reset1x ? (ht_uint%d)0 : c_msgIdx;\n", msgDwCntW);
			fprintf(cppFile, "\n");

			fprintf(cppFile, "\tm_msg.clock(r_reset1x);\n");
			fprintf(cppFile, "\n");

			fprintf(cppFile, "\tht_attrib(equivalent_register_removal, r_reset1x, \"no\");\n");
			fprintf(cppFile, "\tHtResetFlop(r_reset1x, i_reset.read());\n");
			fprintf(cppFile, "\n");
		}

		fprintf(cppFile, "\to_mon_valid = r_mon_valid;\n");
		if (msgDwCnt > 1)
			fprintf(cppFile, "\to_mon_data = r_mon_data;\n");
		else
			fprintf(cppFile, "\to_mon_data = %sUnion.m_data;\n", pMicAeNext->m_pType->m_typeName.c_str() + 1);
		fprintf(cppFile, "\to_msgFull = r_msgFull;\n");
		fprintf(cppFile, "}\n");

		cppFile.FileClose();
	}

	{
		////////////////////////////////////////////////////////////////
		// Generate PersMipSb.h file

		string fileName = g_appArgs.GetOutputFolder() + "/PersMipSb.h";

		CHtFile incFile(fileName, "w");

		GenPersBanner(incFile, "", "MipSb", true);

		fprintf(incFile, "SC_MODULE(CPersMipSb) {\n");
		fprintf(incFile, "\tht_attrib(keep_hierarchy, CPersMipSb, \"true\");\n");
		fprintf(incFile, "\n");

		fprintf(incFile, "\tsc_in<bool> i_clock1x;\n");
		fprintf(incFile, "\tsc_in<bool> i_reset;\n");
		fprintf(incFile, "\n");

		fprintf(incFile, "\tsc_out<bool> o_msgRdy;\n");
		fprintf(incFile, "\tsc_out<%s> o_msg;\n", pMicAeNext->m_pType->m_typeName.c_str());
		fprintf(incFile, "\tsc_in<bool> i_msgFull;\n");
		fprintf(incFile, "\n");

		fprintf(incFile, "\tsc_in<bool> i_mip_valid;\n");
		fprintf(incFile, "\tsc_in<uint32_t> i_mip_data;\n");
		fprintf(incFile, "\tsc_out<bool> o_mip_stall;\n");
		fprintf(incFile, "\n");

		fprintf(incFile, "\tbool r_msgFull;\n");
		fprintf(incFile, "\tbool r_msgRdy;\n");
		fprintf(incFile, "\t%s r_msg;\n", pMicAeNext->m_pType->m_typeName.c_str());
		if (msgDwCnt > 1) {
			fprintf(incFile, "\tht_uint%d r_msgIdx;\n", msgDwCntW);
			fprintf(incFile, "\tbool r_reset1x;\n");
		}
		fprintf(incFile, "\n");

		fprintf(incFile, "\tvoid PersMipSb_1x();\n");
		fprintf(incFile, "\n");

		fprintf(incFile, "\tSC_CTOR(CPersMipSb) {\n");
		fprintf(incFile, "\t\tSC_METHOD(PersMipSb_1x);\n");
		fprintf(incFile, "\t\tsensitive << i_clock1x.pos();\n");
		fprintf(incFile, "\t}\n");

		if (g_appArgs.IsVcdAllEnabled()) {
			fprintf(incFile, "\n");

			fprintf(incFile, "#\tifndef _HTV\n");
			fprintf(incFile, "\tvoid start_of_simulation()\n");
			fprintf(incFile, "\t{\n");
			fprintf(incFile, "\t\tsc_trace(Ht::g_vcdp, r_msgFull, (std::string)name() + \".r_msgFull\");\n");
			fprintf(incFile, "\t\tsc_trace(Ht::g_vcdp, r_msgRdy, (std::string)name() + \".r_msgRdy\");\n");
			fprintf(incFile, "\t\tsc_trace(Ht::g_vcdp, r_msg, (std::string)name() + \".r_msg\");\n");
			if (msgDwCnt > 1) {
				fprintf(incFile, "\t\tsc_trace(Ht::g_vcdp, r_msgIdx, (std::string)name() + \".r_msgIdx\");\n");
				fprintf(incFile, "\t\tsc_trace(Ht::g_vcdp, r_reset1x, (std::string)name() + \".r_reset1x\");\n");
			}
			fprintf(incFile, "\t}\n");
			fprintf(incFile, "#\tendif\n");
		}

		fprintf(incFile, "};\n");

		incFile.FileClose();
	}

	{
		////////////////////////////////////////////////////////////////
		// Generate PersMipSb.cpp file

		string fileName = g_appArgs.GetOutputFolder() + "/PersMipSb.cpp";

		CHtFile cppFile(fileName, "w");

		GenPersBanner(cppFile, "", "MipSb", false);

		fprintf(cppFile, "void CPersMipSb::PersMipSb_1x()\n");
		fprintf(cppFile, "{\n");

		fprintf(cppFile, "\tunion %sUnion {\n", pMicAeNext->m_pType->m_typeName.c_str());
		fprintf(cppFile, "\t\t%s m_msg;\n", pMicAeNext->m_pType->m_typeName.c_str());
		if (msgDwCnt > 1)
			fprintf(cppFile, "\t\tuint32_t m_data[%d];\n", msgDwCnt);
		else
			fprintf(cppFile, "\t\tuint32_t m_data;\n");
		fprintf(cppFile, "\t};\n");
		fprintf(cppFile, "\n");

		fprintf(cppFile, "\t%sUnion %sUnion;\n", pMicAeNext->m_pType->m_typeName.c_str(), pMicAeNext->m_pType->m_typeName.c_str() + 1);
		if (msgDwCnt > 1) {
			fprintf(cppFile, "\t%sUnion.m_msg = r_msg;\n", pMicAeNext->m_pType->m_typeName.c_str() + 1);
			fprintf(cppFile, "\t%sUnion.m_data[r_msgIdx] = i_mip_data;\n", pMicAeNext->m_pType->m_typeName.c_str() + 1);
		} else
			fprintf(cppFile, "\t%sUnion.m_data = i_mip_data;\n", pMicAeNext->m_pType->m_typeName.c_str() + 1);
		fprintf(cppFile, "\n");

		if (msgDwCnt > 1) {
			fprintf(cppFile, "\tht_uint%d c_msgIdx = r_msgIdx;\n", msgDwCntW);
			fprintf(cppFile, "\tbool c_msgRdy = false;\n");
			fprintf(cppFile, "\n");

			fprintf(cppFile, "\tif (i_mip_valid.read()) {\n");
			fprintf(cppFile, "\t\tif (r_msgIdx == %d) {\n", msgDwCnt - 1);
			fprintf(cppFile, "\t\t\tc_msgIdx = 0;\n");
			fprintf(cppFile, "\t\t\tc_msgRdy = true;\n");
			fprintf(cppFile, "\t\t} else\n");
			fprintf(cppFile, "\t\t\tc_msgIdx = r_msgIdx + 1u;\n");
			fprintf(cppFile, "\t}\n");
			fprintf(cppFile, "\n");

			fprintf(cppFile, "\tr_msgRdy = c_msgRdy;\n");
		} else {
			fprintf(cppFile, "\tr_msgRdy = i_mip_valid;\n");
		}

		fprintf(cppFile, "\tr_msg = %sUnion.m_msg;\n", pMicAeNext->m_pType->m_typeName.c_str() + 1);
		fprintf(cppFile, "\tr_msgFull = i_msgFull;\n");
		if (msgDwCnt > 1) {
			fprintf(cppFile, "\tr_msgIdx = r_reset1x ? (ht_uint%d)0 : c_msgIdx;\n", msgDwCntW);
			fprintf(cppFile, "\n");

			fprintf(cppFile, "\tht_attrib(equivalent_register_removal, r_reset1x, \"no\");\n");
			fprintf(cppFile, "\tHtResetFlop(r_reset1x, i_reset.read());\n");
		}
		fprintf(cppFile, "\n");

		fprintf(cppFile, "\to_msgRdy = r_msgRdy;\n");
		fprintf(cppFile, "\to_msg = r_msg;\n");
		fprintf(cppFile, "\to_mip_stall = r_msgFull;\n");
		fprintf(cppFile, "}\n");

		cppFile.FileClose();
	}
}

void CDsnInfo::GenAePrevMsgIntf(HtiFile::CMsgIntfConn * pMicAePrev)
{
	string varName = "AePrevMsg";
	CHtString bitWidth;
	bitWidth.SetValue(0);

	int msgDwCnt = ((pMicAePrev->m_pType->GetPackedBitWidth()) + 31) / 32;
	int msgDwCntW = FindLg2(msgDwCnt - 1);
	{
		////////////////////////////////////////////////////////////////
		// Generate PersMopSb.h file

		string fileName = g_appArgs.GetOutputFolder() + "/PersMopSb.h";

		CHtFile incFile(fileName, "w");

		GenPersBanner(incFile, "", "MopSb", true);

		fprintf(incFile, "SC_MODULE(CPersMopSb) {\n");
		fprintf(incFile, "\tht_attrib(keep_hierarchy, CPersMopSb, \"true\");\n");
		fprintf(incFile, "\n");

		fprintf(incFile, "\tsc_in<bool> i_clock1x;\n");
		fprintf(incFile, "\tsc_in<bool> i_reset;\n");
		fprintf(incFile, "\n");

		fprintf(incFile, "\tsc_in<bool> i_msgRdy;\n");
		fprintf(incFile, "\tsc_in<%s> i_msg;\n", pMicAePrev->m_pType->m_typeName.c_str());
		fprintf(incFile, "\tsc_out<bool> o_msgFull;\n");
		fprintf(incFile, "\n");

		fprintf(incFile, "\tsc_out<bool> o_mop_valid;\n");
		fprintf(incFile, "\tsc_out<uint32_t> o_mop_data;\n");
		fprintf(incFile, "\tsc_in<bool> i_mop_stall;\n");
		fprintf(incFile, "\n");

		if (msgDwCnt > 1) {
			fprintf(incFile, "\tht_dist_que<%s, 5> m_msg;\n", pMicAePrev->m_pType->m_typeName.c_str());
			fprintf(incFile, "\tbool r_mop_valid;\n");
			fprintf(incFile, "\tuint32_t r_mop_data;\n");
			fprintf(incFile, "\tbool r_mop_stall;\n");
			fprintf(incFile, "\tht_uint%d r_msgIdx;\n", msgDwCntW);
			fprintf(incFile, "\tbool r_msgFull;\n");
			fprintf(incFile, "\tbool r_reset1x;\n");
		} else {
			fprintf(incFile, "\tbool r_mop_valid;\n");
			fprintf(incFile, "\t%s r_msg;\n", pMicAePrev->m_pType->m_typeName.c_str());
			fprintf(incFile, "\tbool r_msgFull;\n");
		}
		fprintf(incFile, "\n");

		fprintf(incFile, "\tvoid PersMopSb_1x();\n");
		fprintf(incFile, "\n");

		fprintf(incFile, "\tSC_CTOR(CPersMopSb) {\n");
		fprintf(incFile, "\t\tSC_METHOD(PersMopSb_1x);\n");
		fprintf(incFile, "\t\tsensitive << i_clock1x.pos();\n");
		fprintf(incFile, "\t}\n");

		if (g_appArgs.IsVcdAllEnabled()) {
			fprintf(incFile, "\n");

			fprintf(incFile, "#\tifndef _HTV\n");
			fprintf(incFile, "\tvoid start_of_simulation()\n");
			fprintf(incFile, "\t{\n");
			fprintf(incFile, "\t\tsc_trace(Ht::g_vcdp, r_mop_valid, (std::string)name() + \".r_mop_valid\");\n");
			if (msgDwCnt > 1) {
				fprintf(incFile, "\t\tsc_trace(Ht::g_vcdp, r_mop_data, (std::string)name() + \".r_mop_data\");\n");
				fprintf(incFile, "\t\tsc_trace(Ht::g_vcdp, r_mop_stall, (std::string)name() + \".r_mop_stall\");\n");
			} else
				fprintf(incFile, "\t\tsc_trace(Ht::g_vcdp, r_msg, (std::string)name() + \".r_msg\");\n");
			fprintf(incFile, "\t\tsc_trace(Ht::g_vcdp, r_msgFull, (std::string)name() + \".r_msgFull\");\n");
			if (msgDwCnt > 1) {
				fprintf(incFile, "\t\tsc_trace(Ht::g_vcdp, r_msgIdx, (std::string)name() + \".r_msgIdx\");\n");
				fprintf(incFile, "\t\tsc_trace(Ht::g_vcdp, r_reset1x, (std::string)name() + \".r_reset1x\");\n");
			}
			fprintf(incFile, "\t}\n");
			fprintf(incFile, "#\tendif\n");
		}

		fprintf(incFile, "};\n");

		incFile.FileClose();
	}

	{
		////////////////////////////////////////////////////////////////
		// Generate PersMopSb.cpp file

		string fileName = g_appArgs.GetOutputFolder() + "/PersMopSb.cpp";

		CHtFile cppFile(fileName, "w");

		GenPersBanner(cppFile, "", "MopSb", false);

		fprintf(cppFile, "void CPersMopSb::PersMopSb_1x()\n");
		fprintf(cppFile, "{\n");

		fprintf(cppFile, "\tunion %sUnion {\n", pMicAePrev->m_pType->m_typeName.c_str());
		fprintf(cppFile, "\t\t%s m_msg;\n", pMicAePrev->m_pType->m_typeName.c_str());
		if (msgDwCnt > 1)
			fprintf(cppFile, "\t\tuint32_t m_data[%d];\n", msgDwCnt);
		else
			fprintf(cppFile, "\t\tuint32_t m_data;\n");
		fprintf(cppFile, "\t};\n");
		fprintf(cppFile, "\n");

		if (msgDwCnt > 1) {
			fprintf(cppFile, "\tif (i_msgRdy)\n");
			fprintf(cppFile, "\t\tm_msg.push(i_msg);\n");
			fprintf(cppFile, "\n");

			fprintf(cppFile, "\tht_uint%d c_msgIdx = r_msgIdx;\n", msgDwCntW);
			fprintf(cppFile, "\tif (!m_msg.empty() && !r_mop_stall) {\n");
			fprintf(cppFile, "\t\tif (r_msgIdx == %d) {\n", msgDwCnt - 1);
			fprintf(cppFile, "\t\t\tc_msgIdx = 0;\n");
			fprintf(cppFile, "\t\t\tm_msg.pop();\n");
			fprintf(cppFile, "\t\t} else\n");
			fprintf(cppFile, "\t\t\tc_msgIdx = r_msgIdx + 1u;\n");
			fprintf(cppFile, "\t}\n");
		} else {
			fprintf(cppFile, "\tr_mop_valid = i_msgRdy;\n");
			fprintf(cppFile, "\tr_msg = i_msg;\n");
			fprintf(cppFile, "\tr_msgFull = i_mop_stall;\n");
		}
		fprintf(cppFile, "\n");

		fprintf(cppFile, "\t%sUnion %sUnion;\n", pMicAePrev->m_pType->m_typeName.c_str(), pMicAePrev->m_pType->m_typeName.c_str() + 1);
		if (msgDwCnt > 1)
			fprintf(cppFile, "\t%sUnion.m_msg = m_msg.front();\n", pMicAePrev->m_pType->m_typeName.c_str() + 1);
		else
			fprintf(cppFile, "\t%sUnion.m_msg = r_msg;\n", pMicAePrev->m_pType->m_typeName.c_str() + 1);
		fprintf(cppFile, "\n");

		if (msgDwCnt > 1) {
			fprintf(cppFile, "\tr_mop_valid = !m_msg.empty() && !r_mop_stall;\n");
			fprintf(cppFile, "\tr_mop_data = %sUnion.m_data[r_msgIdx];\n", pMicAePrev->m_pType->m_typeName.c_str() + 1);
			fprintf(cppFile, "\tr_mop_stall = i_mop_stall;\n");
			fprintf(cppFile, "\tr_msgFull = m_msg.size() > 28;\n");
			fprintf(cppFile, "\tr_msgIdx = r_reset1x ? (ht_uint%d)0 : c_msgIdx;\n", msgDwCntW);
			fprintf(cppFile, "\n");

			fprintf(cppFile, "\tm_msg.clock(r_reset1x);\n");
			fprintf(cppFile, "\n");

			fprintf(cppFile, "\tht_attrib(equivalent_register_removal, r_reset1x, \"no\");\n");
			fprintf(cppFile, "\tHtResetFlop(r_reset1x, i_reset.read());\n");
			fprintf(cppFile, "\n");
		}

		fprintf(cppFile, "\to_mop_valid = r_mop_valid;\n");
		if (msgDwCnt > 1)
			fprintf(cppFile, "\to_mop_data = r_mop_data;\n");
		else
			fprintf(cppFile, "\to_mop_data = %sUnion.m_data;\n", pMicAePrev->m_pType->m_typeName.c_str() + 1);
		fprintf(cppFile, "\to_msgFull = r_msgFull;\n");
		fprintf(cppFile, "}\n");

		cppFile.FileClose();
	}

	{
		////////////////////////////////////////////////////////////////
		// Generate PersMinSb.h file

		string fileName = g_appArgs.GetOutputFolder() + "/PersMinSb.h";

		CHtFile incFile(fileName, "w");

		GenPersBanner(incFile, "", "MinSb", true);

		fprintf(incFile, "SC_MODULE(CPersMinSb) {\n");
		fprintf(incFile, "\tht_attrib(keep_hierarchy, CPersMinSb, \"true\");\n");
		fprintf(incFile, "\n");

		fprintf(incFile, "\tsc_in<bool> i_clock1x;\n");
		fprintf(incFile, "\tsc_in<bool> i_reset;\n");
		fprintf(incFile, "\n");

		fprintf(incFile, "\tsc_out<bool> o_msgRdy;\n");
		fprintf(incFile, "\tsc_out<%s> o_msg;\n", pMicAePrev->m_pType->m_typeName.c_str());
		fprintf(incFile, "\tsc_in<bool> i_msgFull;\n");
		fprintf(incFile, "\n");

		fprintf(incFile, "\tsc_in<bool> i_min_valid;\n");
		fprintf(incFile, "\tsc_in<uint32_t> i_min_data;\n");
		fprintf(incFile, "\tsc_out<bool> o_min_stall;\n");
		fprintf(incFile, "\n");

		fprintf(incFile, "\tbool r_msgFull;\n");
		fprintf(incFile, "\tbool r_msgRdy;\n");
		fprintf(incFile, "\t%s r_msg;\n", pMicAePrev->m_pType->m_typeName.c_str());
		if (msgDwCnt > 1) {
			fprintf(incFile, "\tht_uint%d r_msgIdx;\n", msgDwCntW);
			fprintf(incFile, "\tbool r_reset1x;\n");
		}
		fprintf(incFile, "\n");

		fprintf(incFile, "\tvoid PersMinSb_1x();\n");
		fprintf(incFile, "\n");

		fprintf(incFile, "\tSC_CTOR(CPersMinSb) {\n");
		fprintf(incFile, "\t\tSC_METHOD(PersMinSb_1x);\n");
		fprintf(incFile, "\t\tsensitive << i_clock1x.pos();\n");
		fprintf(incFile, "\t}\n");

		if (g_appArgs.IsVcdAllEnabled()) {
			fprintf(incFile, "\n");

			fprintf(incFile, "#\tifndef _HTV\n");
			fprintf(incFile, "\tvoid start_of_simulation()\n");
			fprintf(incFile, "\t{\n");
			fprintf(incFile, "\t\tsc_trace(Ht::g_vcdp, r_msgFull, (std::string)name() + \".r_msgFull\");\n");
			fprintf(incFile, "\t\tsc_trace(Ht::g_vcdp, r_msgRdy, (std::string)name() + \".r_msgRdy\");\n");
			fprintf(incFile, "\t\tsc_trace(Ht::g_vcdp, r_msg, (std::string)name() + \".r_msg\");\n");
			if (msgDwCnt > 1) {
				fprintf(incFile, "\t\tsc_trace(Ht::g_vcdp, r_msgIdx, (std::string)name() + \".r_msgIdx\");\n");
				fprintf(incFile, "\t\tsc_trace(Ht::g_vcdp, r_reset1x, (std::string)name() + \".r_reset1x\");\n");
			}
			fprintf(incFile, "\t}\n");
			fprintf(incFile, "#\tendif\n");
		}

		fprintf(incFile, "};\n");

		incFile.FileClose();
	}

	{
		////////////////////////////////////////////////////////////////
		// Generate PersMinSb.cpp file

		string fileName = g_appArgs.GetOutputFolder() + "/PersMinSb.cpp";

		CHtFile cppFile(fileName, "w");

		GenPersBanner(cppFile, "", "MinSb", false);

		fprintf(cppFile, "void CPersMinSb::PersMinSb_1x()\n");
		fprintf(cppFile, "{\n");

		fprintf(cppFile, "\tunion %sUnion {\n", pMicAePrev->m_pType->m_typeName.c_str());
		fprintf(cppFile, "\t\t%s m_msg;\n", pMicAePrev->m_pType->m_typeName.c_str());
		if (msgDwCnt > 1)
			fprintf(cppFile, "\t\tuint32_t m_data[%d];\n", msgDwCnt);
		else
			fprintf(cppFile, "\t\tuint32_t m_data;\n");
		fprintf(cppFile, "\t};\n");
		fprintf(cppFile, "\n");

		fprintf(cppFile, "\t%sUnion %sUnion;\n", pMicAePrev->m_pType->m_typeName.c_str(), pMicAePrev->m_pType->m_typeName.c_str() + 1);
		if (msgDwCnt > 1) {
			fprintf(cppFile, "\t%sUnion.m_msg = r_msg;\n", pMicAePrev->m_pType->m_typeName.c_str() + 1);
			fprintf(cppFile, "\t%sUnion.m_data[r_msgIdx] = i_min_data;\n", pMicAePrev->m_pType->m_typeName.c_str() + 1);
		} else
			fprintf(cppFile, "\t%sUnion.m_data = i_min_data;\n", pMicAePrev->m_pType->m_typeName.c_str() + 1);
		fprintf(cppFile, "\n");

		if (msgDwCnt > 1) {
			fprintf(cppFile, "\tht_uint%d c_msgIdx = r_msgIdx;\n", msgDwCntW);
			fprintf(cppFile, "\tbool c_msgRdy = false;\n");
			fprintf(cppFile, "\n");

			fprintf(cppFile, "\tif (i_min_valid.read()) {\n");
			fprintf(cppFile, "\t\tif (r_msgIdx == %d) {\n", msgDwCnt - 1);
			fprintf(cppFile, "\t\t\tc_msgIdx = 0;\n");
			fprintf(cppFile, "\t\t\tc_msgRdy = true;\n");
			fprintf(cppFile, "\t\t} else\n");
			fprintf(cppFile, "\t\t\tc_msgIdx = r_msgIdx + 1u;\n");
			fprintf(cppFile, "\t}\n");
			fprintf(cppFile, "\n");

			fprintf(cppFile, "\tr_msgRdy = c_msgRdy;\n");
		} else {
			fprintf(cppFile, "\tr_msgRdy = i_min_valid;\n");
		}

		fprintf(cppFile, "\tr_msg = %sUnion.m_msg;\n", pMicAePrev->m_pType->m_typeName.c_str() + 1);
		fprintf(cppFile, "\tr_msgFull = i_msgFull;\n");
		if (msgDwCnt > 1) {
			fprintf(cppFile, "\tr_msgIdx = r_reset1x ? (ht_uint%d)0 : c_msgIdx;\n", msgDwCntW);
			fprintf(cppFile, "\n");

			fprintf(cppFile, "\tht_attrib(equivalent_register_removal, r_reset1x, \"no\");\n");
			fprintf(cppFile, "\tHtResetFlop(r_reset1x, i_reset.read());\n");
		}
		fprintf(cppFile, "\n");

		fprintf(cppFile, "\to_msgRdy = r_msgRdy;\n");
		fprintf(cppFile, "\to_msg = r_msg;\n");
		fprintf(cppFile, "\to_min_stall = r_msgFull;\n");
		fprintf(cppFile, "}\n");

		cppFile.FileClose();
	}
}
