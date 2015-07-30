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

// Generate a module's call/xfer/return interfaces

void CDsnInfo::InitAndValidateModCxr()
{
	// Initialize HtString values needed for Cxr
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];

		for (size_t callIdx = 0; callIdx < mod.m_cxrCallList.size(); callIdx += 1) {
			CCxrCall & cxrCall = mod.m_cxrCallList[callIdx];

			cxrCall.m_queueW.InitValue(cxrCall.m_lineInfo);
		}

		if (mod.m_bHasThreads)
			mod.m_threads.m_htIdW.InitValue(mod.m_threads.m_lineInfo, false);
	}

	// Perform consistency checks on required modules
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];

		mod.m_instIdCnt = 0;

		mod.m_cxrEntryReserveCnt = 0;
		for (size_t entryIdx = 0; entryIdx < mod.m_cxrEntryList.size(); entryIdx += 1) {
			CCxrEntry * pCxrEntry = mod.m_cxrEntryList[entryIdx];

			pCxrEntry->m_reserveCnt.InitValue(pCxrEntry->m_lineInfo, false, 0);
			mod.m_cxrEntryReserveCnt += pCxrEntry->m_reserveCnt.AsInt();

			pCxrEntry->m_pGroup = &mod.m_threads;

			// check that entry parameters are declared as htPriv also
			for (size_t prmIdx = 0; prmIdx < pCxrEntry->m_paramList.size(); prmIdx += 1) {
				CField * pParam = pCxrEntry->m_paramList[prmIdx];

				bool bFound = false;
				vector<CField *> &fieldList = pCxrEntry->m_pGroup->m_htPriv.m_fieldList;
				for (size_t fldIdx = 0; fldIdx < fieldList.size(); fldIdx += 1) {
					CField * pField = fieldList[fldIdx];

					if (pField->m_name == pParam->m_name) {
						if (pField->m_type != pParam->m_type)
							ParseMsg(Error, pParam->m_lineInfo, "parameter and private variable have different types");
						bFound = true;
					}
				}
				if (!bFound)
					ParseMsg(Error, pParam->m_lineInfo, "parameter '%s' was not declared as a private variable",
					pParam->m_name.c_str());
			}
		}

		if (mod.m_cxrEntryReserveCnt >= (1 << mod.m_threads.m_htIdW.AsInt()))
			CPreProcess::ParseMsg(Error, mod.m_threads.m_lineInfo,
			"sum of AddEntry reserve values (%d) must be less than number of module threads (%d)",
			mod.m_cxrEntryReserveCnt, (1 << mod.m_threads.m_htIdW.AsInt()));

		for (size_t callIdx = 0; callIdx < mod.m_cxrCallList.size(); callIdx += 1) {
			CCxrCall & cxrCall = mod.m_cxrCallList[callIdx];

			// find associated group for call/transfer
			cxrCall.m_pGroup = &mod.m_threads;
		}
	}

	// verify that all AddReturns associated with an AddEntry have the same return parameters
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];

		for (size_t rtnIdx = 0; rtnIdx < mod.m_cxrReturnList.size(); rtnIdx += 1) {
			CCxrReturn &rtn = mod.m_cxrReturnList[rtnIdx];

			// check all other modules for a return with the same entry name
			for (size_t mod2Idx = modIdx + 1; mod2Idx < m_modList.size(); mod2Idx += 1) {
				CModule &mod2 = *m_modList[mod2Idx];

				for (size_t rtn2Idx = 0; rtn2Idx < mod2.m_cxrReturnList.size(); rtn2Idx += 1) {
					CCxrReturn &rtn2 = mod2.m_cxrReturnList[rtn2Idx];

					if (rtn.m_funcName != rtn2.m_funcName) continue;

					// found matching returns, check parameter list
					if (rtn.m_paramList.size() != rtn2.m_paramList.size()) {
						CPreProcess::ParseMsg(Error, rtn2.m_lineInfo, "AddReturn commands with matching function names have different returning parameter");
						CPreProcess::ParseMsg(Info, rtn.m_lineInfo, "previous AddReturn command");
						break;
					}

					for (size_t paramIdx = 0; paramIdx < rtn.m_paramList.size(); paramIdx += 1) {
						CField * pParam = rtn.m_paramList[paramIdx];
						CField * pParam2 = rtn2.m_paramList[paramIdx];

						if (pParam->m_name != pParam2->m_name || pParam->m_type != pParam2->m_type) {
							CPreProcess::ParseMsg(Error, rtn2.m_lineInfo, "AddReturn commands with matching function names have different returning parameter");
							CPreProcess::ParseMsg(Info, rtn.m_lineInfo, "previous AddReturn command");
							break;
						}
					}
				}
			}
		}
	}

	// Start with HIF and decend call tree checking that an entry point is defined for each call
	//	Mark each call with the target module/entryId
	//	Mark each target module as needed

	int unitTypeCnt = getUnitTypeCnt();

	// iterate through unit types checking call stack and creating module instance lists
	for (int unitTypeIdx = 0; unitTypeIdx < unitTypeCnt; unitTypeIdx += 1) {

		// Check that the HIF entry point was specified on the command line
		CModule &hifMod = *m_modList[unitTypeIdx];
		hifMod.m_bIsUsed = true;
		HtlAssert(hifMod.m_modName.Lc() == "hif");

		if (hifMod.m_cxrCallList.size() == 0)
			ParseMsg(Fatal, "Unit entry function name must be specified (i.e. use AddEntry(host=true) )");

		string unitName = getUnitName(unitTypeIdx);

		vector<CModIdx> callStk;

		callStk.push_back(CModIdx(hifMod, 0, unitName + ":"));

		hifMod.m_bActiveCall = true;	// check for recursion

		CheckRequiredEntryNames(callStk);

		HtlAssert(callStk.size() == 1);
	}

	if (CLex::GetParseErrorCnt() > 0)
		ParseMsg(Fatal, "Previous errors prevent generation of files");

	// All required entry points are present and we have marked the required modules

	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];

		if (!mod.m_bIsUsed) continue;

		// verify that returns have at least one paired call
		//for (size_t rtnIdx = 0; rtnIdx < mod.m_cxrReturnList.size(); rtnIdx += 1) {
		//	CCxrReturn & cxrReturn = mod.m_cxrReturnList[rtnIdx];

		//	if (cxrReturn.m_pairedCallList.size() == 0)
		//		ParseMsg(Error, cxrReturn.m_lineInfo, "no caller found that uses return");
		//}

		for (size_t entryIdx = 0; entryIdx < mod.m_cxrEntryList.size(); entryIdx += 1) {
			CCxrEntry * pCxrEntry = mod.m_cxrEntryList[entryIdx];

			if (pCxrEntry->m_pairedCallList.size() == 0) continue;

			// verify that entry instruction is in list for module
			size_t instrIdx;
			for (instrIdx = 0; instrIdx < mod.m_instrList.size(); instrIdx += 1) {

				if (mod.m_instrList[instrIdx] == pCxrEntry->m_entryInstr)
					break;
			}

			if (instrIdx == mod.m_instrList.size())
				ParseMsg(Error, pCxrEntry->m_lineInfo, "entry instruction is not defined for module, '%s'",
				pCxrEntry->m_entryInstr.c_str());
		}
	}

	// Initialize HIF CModule's m_modInstList
	//   All other modInstLists were initialized while walking the call chain
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];
		if (!mod.m_bIsUsed) continue;	// only check required modules

		if (mod.m_modInstList.size() == 0) {
			HtlAssert(mod.m_modName == "hif");

			mod.m_modInstList.resize(1);
			mod.m_modInstList[0].m_pMod = &mod;
			mod.m_modInstList[0].m_instName = mod.m_modName;
			mod.m_modInstList[0].m_instParams.m_instId = 0;
			mod.m_modInstList[0].m_instParams.m_memPortList.push_back(0);
			mod.m_modInstList[0].m_cxrSrcCnt = (mod.m_bHasThreads ? 1 : 0) + mod.m_resetInstrCnt;
			mod.m_modInstList[0].m_replCnt = 1;
			mod.m_modInstList[0].m_replId = 0;
		}

		// set module instance names
		for (size_t i = 0; i < mod.m_modInstList.size(); i += 1) {
			char instName[256];
			if (mod.m_modInstList.size() > 1) {
				sprintf(instName, "%s%d", mod.m_modName.c_str(), (int)i);
				mod.m_modInstList[i].m_instName = instName;
			} else
				mod.m_modInstList[i].m_instName = mod.m_modName;
		}
	}

	// create list of instances that are linked to modules
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];

		if (!mod.m_bIsUsed) continue;

		string instNum = mod.m_modInstList.size() == 1 ? "" : "0";
		for (int modInstIdx = 0; modInstIdx < (int)mod.m_modInstList.size(); modInstIdx += 1) {
			CModInst & modInst = mod.m_modInstList[modInstIdx];

			m_dsnInstList.push_back(modInst);

			if (mod.m_modInstList.size() > 1)
				instNum[0] += 1;
		}
	}

	if (CLex::GetParseErrorCnt() > 0)
		ParseMsg(Fatal, "Previous errors prevent generation of files");

	InitCxrIntfInfo();
}

void CDsnInfo::CheckRequiredEntryNames(vector<CModIdx> &callStk)
{
	CModIdx & modCall = callStk.back();
	CCxrCall & cxrCallee = modCall.m_pMod->m_cxrCallList[modCall.m_idx];
	CHtString & funcName = cxrCallee.m_funcName;

	modCall.m_pMod->m_bCallFork |= cxrCallee.m_bCallFork;
	modCall.m_pMod->m_bXferFork |= cxrCallee.m_bXferFork;

	cxrCallee.m_pGroup->m_bCallFork |= cxrCallee.m_bCallFork;
	cxrCallee.m_pGroup->m_bXferFork |= cxrCallee.m_bXferFork;

	// search for matching entry in list of modules
	size_t modIdx;
	size_t entryIdx;
	bool bFound = false;
	for (modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];

		for (entryIdx = 0; entryIdx < mod.m_cxrEntryList.size(); entryIdx += 1) {
			CCxrEntry * pCxrEntry = mod.m_cxrEntryList[entryIdx];

			if (pCxrEntry->m_funcName == funcName) {
				pCxrEntry->m_bCallFork |= cxrCallee.m_bCallFork;
				pCxrEntry->m_bXferFork |= cxrCallee.m_bXferFork;

				pCxrEntry->m_pGroup->m_bRtnJoin |= cxrCallee.m_bCallFork || cxrCallee.m_bXferFork;

				cxrCallee.m_pPairedEntry = pCxrEntry;
				bFound = true;
				break;
			}
		}

		if (bFound)
			break;
	}

	if (!bFound) {
		if (cxrCallee.m_lineInfo.m_lineNum == 0)
			ParseMsg(Error, "Personality entry function name was not found, '%s'", funcName.c_str());
		else
			ParseMsg(Error, cxrCallee.m_lineInfo, "function name was not found, '%s'", funcName.c_str());
	} else if (m_modList[modIdx]->m_bActiveCall) {
		ParseMsg(Error, cxrCallee.m_lineInfo, "found recursion in call chain");
		for (size_t stkIdx = 1; stkIdx < callStk.size(); stkIdx += 1) {
			CCxrCall &cxrCaller = callStk[stkIdx - 1].m_pMod->m_cxrCallList[callStk[stkIdx - 1].m_idx];
			CCxrCall &cxrCallee = callStk[stkIdx].m_pMod->m_cxrCallList[callStk[stkIdx].m_idx];

			ParseMsg(Info, cxrCaller.m_lineInfo, "    Module %s, entry %s, called %s\n",
				callStk[stkIdx].m_pMod->m_modName.Lc().c_str(),
				cxrCaller.m_funcName.c_str(),
				cxrCallee.m_funcName.c_str());
		}
	} else {
		CModule &mod = *m_modList[modIdx];

		string modPath = callStk.back().m_modPath + "/" + mod.m_modName.AsStr();

		{	// Found the modPath, check if any per instance params were specified.
			// This section takes the modPath as input, appends to the modInstList,
			// and creats the modFileList.
			//
			// Every module instance is pushed onto the modInstList. For replicated
			// modules, one entry for each replicated instance.
			//
			// An instance creates a new modFileList entry
			// if it has a 'not seen before' modInstId param value, or has a replCnt > 1,
			// or uses the default modInst and this is the first time it was used.

			CInstanceParams modInstParams;
			getModInstParams(modPath, modInstParams);

			if (callStk.size() <= 1 && modInstParams.m_replCnt > 1)
				ParseMsg(Error, modInstParams.m_lineInfo, "replication not supported for host interface entry module");

			m_maxInstId = max(m_maxInstId, modInstParams.m_instId);
			m_maxReplCnt = max(m_maxReplCnt, modInstParams.m_replCnt);

			// add to list of unique instance IDs
			size_t ii;
			for (ii = 0; ii < m_instIdList.size(); ii += 1)
				if (m_instIdList[ii] == modInstParams.m_instId)
					break;

			if (ii == m_instIdList.size())
				m_instIdList.push_back(modInstParams.m_instId);

			// replicated module instance must have a unique instId

			for (size_t i = 0; i < mod.m_modInstList.size(); i += 1) {
				if (modInstParams.m_instId != mod.m_modInstList[i].m_instParams.m_instId) {
					//continue;

					// temporary limitation until figure out how to follow call/return through different instances
					ParseMsg(Error, modInstParams.m_lineInfo, "Different instIds for same module not currently supported");
					ParseMsg(Info, modInstParams.m_lineInfo, "module path: %s has replCnt=%d, instId=%d",
						mod.m_modInstList[i].m_modPaths[0].c_str(),
						mod.m_modInstList[i].m_instParams.m_replCnt,
						mod.m_modInstList[i].m_instParams.m_instId);
					ParseMsg(Info, modInstParams.m_lineInfo, "module path: %s has replCnt=%d, instId=%d",
						modPath.c_str(),
						modInstParams.m_replCnt,
						modInstParams.m_instId);
					break;
				}
			}

			if (modInstParams.m_replCnt > 1) {
				// Push replCnt on modInstList
				for (int i = 0; i < modInstParams.m_replCnt; i += 1) {
					// check for replicated instance parameters
					CInstanceParams replModInstParams;
					char buf[16];
					sprintf(buf, "[%d]", (int)i);
					string replModPath = modPath + buf;
					getModInstParams(replModPath, replModInstParams);

					m_maxInstId = max(m_maxInstId, replModInstParams.m_instId);
					m_maxReplCnt = max(m_maxReplCnt, replModInstParams.m_replCnt);

					replModInstParams.m_instId = modInstParams.m_instId;
					replModInstParams.m_replCnt = modInstParams.m_replCnt;

					if (replModInstParams.m_lineInfo.m_fileName.size() == 0)
						replModInstParams.m_lineInfo = modInstParams.m_lineInfo;

					if (modInstParams.m_memPortList.size() > 0 && replModInstParams.m_memPortList.size() > 0)
						CPreProcess::ParseMsg(Error,
						"memPort was specified for modPath '%s' and replicated modPath '%s'",
						modPath.c_str(), replModPath.c_str());

					else if (modInstParams.m_memPortList.size() > 0)
						replModInstParams.m_memPortList = modInstParams.m_memPortList;

					else if (mod.m_memPortList.size() > 0 && replModInstParams.m_memPortList.size() == 0) {
						replModInstParams.m_memPortList.resize(mod.m_memPortList.size());
						for (size_t memPortIdx = 0; memPortIdx < mod.m_memPortList.size(); memPortIdx += 1)
							replModInstParams.m_memPortList[memPortIdx] = memPortIdx;
					}

					if (replModInstParams.m_memPortList.size() != mod.m_memPortList.size())
						ParseMsg(Error, replModInstParams.m_lineInfo, "module memory port count (%d) does not match instance memory port count (%d)\n",
						(int)mod.m_memPortList.size(), (int)replModInstParams.m_memPortList.size());

					int cxrSrcCnt = (mod.m_bHasThreads ? 1 : 0) + mod.m_resetInstrCnt;
					mod.m_modInstList.push_back(CModInst(&mod, modPath, replModInstParams, cxrSrcCnt, modInstParams.m_replCnt, i));
				}
				mod.m_instIdCnt += 1;
			} else {

				if (mod.m_memPortList.size() > 0 && modInstParams.m_memPortList.size() == 0) {
					modInstParams.m_memPortList.resize(mod.m_memPortList.size());
					for (size_t memPortIdx = 0; memPortIdx < mod.m_memPortList.size(); memPortIdx += 1)
						modInstParams.m_memPortList[memPortIdx] = memPortIdx;
				}

				if (modInstParams.m_memPortList.size() != mod.m_memPortList.size())
					ParseMsg(Error, modInstParams.m_lineInfo, "module memory port count (%d) does not match instance memory port count (%d)\n",
					(int)mod.m_memPortList.size(), (int)modInstParams.m_memPortList.size());

				// check if requested instId is new
				size_t i;
				for (i = 0; i < mod.m_modInstList.size(); i += 1) {
					if (mod.m_modInstList[i].m_instParams.m_instId == modInstParams.m_instId)
						break;
				}

				if (i == mod.m_modInstList.size()) {
					// new instId specified
					int cxrSrcCnt = (mod.m_bHasThreads ? 1 : 0) + mod.m_resetInstrCnt;
					mod.m_modInstList.push_back(CModInst(&mod, modPath, modInstParams, cxrSrcCnt));
					mod.m_instIdCnt += 1;
				} else {
					// add new modPath to modInst
					mod.m_modInstList[i].m_modPaths.push_back(modPath);
				}
			}
		}

		mod.m_bRtnJoin |= cxrCallee.m_bCallFork || cxrCallee.m_bXferFork;

		CCxrEntry * pCxrEntry = mod.m_cxrEntryList[entryIdx];

		modCall.m_pMod->m_cxrCallList[modCall.m_idx].m_pairedFunc = CModIdx(mod, entryIdx, modPath);
		pCxrEntry->m_pairedCallList.Insert(modCall);

		mod.m_bIsUsed = true;
		mod.m_bActiveCall = true;

		// find previous call (skip transfers)
		int callIdx;
		CModule * pPrevCxrCallMod = 0;
		CCxrCall * pPrevCxrCall = 0;
		for (callIdx = (int)callStk.size() - 1; callIdx >= 0; callIdx -= 1) {
			pPrevCxrCallMod = callStk[callIdx].m_pMod;
			int prevIdx = callStk[callIdx].m_idx;
			pPrevCxrCall = &pPrevCxrCallMod->m_cxrCallList[prevIdx];
			if (!pPrevCxrCall->m_bXfer)
				break;
		}
		HtlAssert(callIdx >= 0);

		// pair returns with previous call
		bool bFoundRtn = false;
		for (size_t rtnIdx = 0; rtnIdx < m_modList[modIdx]->m_cxrReturnList.size(); rtnIdx += 1) {
			CCxrReturn & cxrReturn = m_modList[modIdx]->m_cxrReturnList[rtnIdx];

			if (cxrReturn.m_funcName == pPrevCxrCall->m_funcName) {
				pPrevCxrCall->m_pairedReturnList.push_back(CModIdx(*m_modList[modIdx], rtnIdx, modPath));
				cxrReturn.m_pairedCallList.Insert(callStk[callIdx]);

				cxrReturn.m_pGroup = pCxrEntry->m_pGroup;

				cxrReturn.m_bRtnJoin |= cxrCallee.m_bCallFork || cxrCallee.m_bXferFork;

				bFoundRtn = true;

				if (pPrevCxrCallMod->m_bHostIntf) continue;

				// verify that caller has private variables for return parameters
				for (size_t prmIdx = 0; prmIdx < cxrReturn.m_paramList.size(); prmIdx += 1) {
					CField * pParam = cxrReturn.m_paramList[prmIdx];

					bool bFound = false;
					vector<CField *> &fieldList = pPrevCxrCall->m_pGroup->m_htPriv.m_fieldList;
					for (size_t fldIdx = 0; fldIdx < fieldList.size(); fldIdx += 1) {
						CField * pField = fieldList[fldIdx];

						if (pField->m_name == pParam->m_name) {
							if (pField->m_type != pParam->m_type)
								ParseMsg(Error, pParam->m_lineInfo, "parameter and caller's (%s) private variable have different types",
								pPrevCxrCallMod->m_modName.c_str());
							bFound = true;
						}
					}
					if (!bFound)
						ParseMsg(Error, pParam->m_lineInfo, "parameter was not declared as a private variable in caller (%s)",
						pPrevCxrCallMod->m_modName.c_str());
				}
			}
		}

		// follow each call
		bool bFoundXfer = false;
		for (size_t callIdx = 0; callIdx < mod.m_cxrCallList.size(); callIdx += 1) {
			CCxrCall & cxrCall = mod.m_cxrCallList[callIdx];

			if (cxrCall.m_bXfer)
				cxrCall.m_bXferFork |= cxrCallee.m_bCallFork || cxrCallee.m_bXferFork;

			cxrCall.m_pairedEntry = CModIdx(mod, entryIdx, modPath);

			callStk.push_back(CModIdx(mod, callIdx, modPath));

			// for xfer, check that a return to previous call exists
			//   It may not exist and be okay if multiple calls to same
			//   module exists.
			if (!cxrCall.m_bXfer || CheckTransferReturn(callStk)) {

				CheckRequiredEntryNames(callStk);
				bFoundXfer = bFoundXfer || cxrCall.m_bXfer;
			}

			callStk.pop_back();
		}

		if (!bFoundRtn && !bFoundXfer) {
			// no exit found for module entry
			if (cxrCallee.m_bXfer)
				ParseMsg(Error, pCxrEntry->m_lineInfo,
				"expected an outbound return or transfer for original call (%s) that was transfered to entry (%s)",
				pPrevCxrCall->m_funcName.c_str(), pCxrEntry->m_funcName.c_str());
			else
				ParseMsg(Error, pCxrEntry->m_lineInfo,
				"expected a return for call to entry (%s)",
				pCxrEntry->m_funcName.c_str());
		}

		mod.m_bActiveCall = false;
	}
}

bool CDsnInfo::CheckTransferReturn(vector<CModIdx> &callStk)
{
	CModIdx & modCall = callStk.back();
	CCxrCall & cxrCallee = modCall.m_pMod->m_cxrCallList[modCall.m_idx];
	CHtString & funcName = cxrCallee.m_funcName;

	// search for matching entry in list of modules
	size_t modIdx;
	size_t entryIdx;
	bool bFound = false;
	for (modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];

		for (entryIdx = 0; entryIdx < mod.m_cxrEntryList.size(); entryIdx += 1) {
			CCxrEntry * pCxrEntry = mod.m_cxrEntryList[entryIdx];

			if (pCxrEntry->m_funcName == funcName) {
				bFound = true;
				break;
			}
		}

		if (bFound)
			break;
	}

	if (!bFound) {
		if (cxrCallee.m_lineInfo.m_lineNum == 0)
			ParseMsg(Error, "Personality entry function name was not found, '%s'", funcName.c_str());
		else
			ParseMsg(Error, cxrCallee.m_lineInfo, "function name was not found, '%s'", funcName.c_str());
		return false;
	} else if (m_modList[modIdx]->m_bActiveCall) {
		ParseMsg(Error, cxrCallee.m_lineInfo, "found recursion in call chain");
		for (size_t stkIdx = 1; stkIdx < callStk.size(); stkIdx += 1) {
			CCxrCall &cxrCaller = callStk[stkIdx - 1].m_pMod->m_cxrCallList[callStk[stkIdx - 1].m_idx];
			CCxrCall &cxrCallee = callStk[stkIdx].m_pMod->m_cxrCallList[callStk[stkIdx].m_idx];

			ParseMsg(Info, cxrCaller.m_lineInfo, "    Module %s, entry %s, called %s\n",
				callStk[stkIdx].m_pMod->m_modName.Lc().c_str(),
				cxrCaller.m_funcName.c_str(),
				cxrCallee.m_funcName.c_str());
		}
		return false;
	} else {
		CModule &mod = *m_modList[modIdx];

		string modPath = callStk.back().m_modPath + "/" + mod.m_modName.AsStr();

		mod.m_bIsUsed = true;

		// find previous call (skip transfers)
		int callIdx;
		CModule * pPrevCxrCallMod = 0;
		CCxrCall * pPrevCxrCall = 0;
		for (callIdx = (int)callStk.size() - 1; callIdx >= 0; callIdx -= 1) {
			pPrevCxrCallMod = callStk[callIdx].m_pMod;
			int prevIdx = callStk[callIdx].m_idx;
			pPrevCxrCall = &pPrevCxrCallMod->m_cxrCallList[prevIdx];
			if (!pPrevCxrCall->m_bXfer)
				break;
		}
		HtlAssert(callIdx >= 0);

		// pair returns with previous call
		for (size_t rtnIdx = 0; rtnIdx < m_modList[modIdx]->m_cxrReturnList.size(); rtnIdx += 1) {
			CCxrReturn & cxrReturn = m_modList[modIdx]->m_cxrReturnList[rtnIdx];

			if (cxrReturn.m_funcName == pPrevCxrCall->m_funcName)
				return true;
		}

		// follow each call
		for (size_t callIdx = 0; callIdx < mod.m_cxrCallList.size(); callIdx += 1) {
			CCxrCall & cxrCall = mod.m_cxrCallList[callIdx];

			if (cxrCall.m_bXfer) {
				callStk.push_back(CModIdx(mod, callIdx, modPath));
				mod.m_bActiveCall = true;

				bool bFoundRtn = CheckTransferReturn(callStk);

				mod.m_bActiveCall = false;
				callStk.pop_back();

				if (bFoundRtn)
					return true;
			}
		}
	}

	return false;
}

void CDsnInfo::InitCxrIntfInfo()
{
	char defineStr[64];
	char valueStr[64];
	char typeName[64];

	// Initialize m_pGroup
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];

		if (!mod.m_bIsUsed) continue;	// only check required modules

		// Initialize cxrCall.m_pGroup
		for (size_t callIdx = 0; callIdx < mod.m_cxrCallList.size(); callIdx += 1) {
			CCxrCall & cxrCall = mod.m_cxrCallList[callIdx];

			cxrCall.m_pGroup = &mod.m_threads;
		}
	}

	// initialize instruction widths
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];

		if (!mod.m_bIsUsed) continue;

		if (mod.m_instrList.size() == 0) {
			mod.m_instrW = 0;
			continue;
		}

		int maxHtInstrValue = (int)mod.m_instrList.size() - 1;

		mod.m_instrW = FindLg2(maxHtInstrValue);
	}

	// Initialize htInstType, htIdW and htIdType
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];

		if (!mod.m_bIsUsed) continue;	// only check required modules

		// Initialize pCxrEntry->m_pGroup

		// generate types for rtnInstr and rtnHtId
		if (mod.m_instrList.size() > 0) {
			sprintf(typeName, "%sInstr_t", mod.m_modName.Uc().c_str());
			mod.m_pInstrType = FindHtIntType(eUnsigned, FindLg2(mod.m_instrList.size()-1));
		}

		if (mod.m_bHasThreads) {
			mod.m_threads.m_htIdW.InitValue(mod.m_threads.m_lineInfo);

			if (mod.m_threads.m_htIdW.AsInt() > 0) {
				sprintf(typeName, "%sHtId_t", mod.m_modName.Uc().c_str());
				mod.m_threads.m_pHtIdType = FindHtIntType(eUnsigned, mod.m_threads.m_htIdW.AsInt());;
			}
		}
	}

	// Initialize values within group
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];

		if (!mod.m_bIsUsed) continue;	// only check required modules

		// Transfer info from module's entry list
		for (size_t entryIdx = 0; entryIdx < mod.m_cxrEntryList.size(); entryIdx += 1) {
			CCxrEntry * pCxrEntry = mod.m_cxrEntryList[entryIdx];

			if (pCxrEntry->m_pairedCallList.size() > 0)
				pCxrEntry->m_pGroup->m_rtnSelW = max(pCxrEntry->m_pGroup->m_rtnSelW, FindLg2(pCxrEntry->m_pairedCallList.size() - 1, false));

			for (size_t callIdx = 0; callIdx < pCxrEntry->m_pairedCallList.size(); callIdx += 1) {
				CModIdx & pairedCall = pCxrEntry->m_pairedCallList[callIdx];
				CCxrCall & cxrCall = pairedCall.m_pMod->m_cxrCallList[pairedCall.m_idx];

				if (cxrCall.m_bXfer) {
					CCxrEntry * pCxrXferEntry = cxrCall.m_pairedEntry.m_pMod->m_cxrEntryList[cxrCall.m_pairedEntry.m_idx];

					pCxrEntry->m_pGroup->m_rtnSelW = pCxrXferEntry->m_pGroup->m_rtnSelW;
					pCxrEntry->m_pGroup->m_rtnInstrW = pCxrXferEntry->m_pGroup->m_rtnInstrW;
					pCxrEntry->m_pGroup->m_rtnHtIdW = pCxrXferEntry->m_pGroup->m_rtnHtIdW;
				} else {
					pCxrEntry->m_pGroup->m_rtnInstrW = max(pCxrEntry->m_pGroup->m_rtnInstrW, pairedCall.m_pMod->m_instrW);
					pCxrEntry->m_pGroup->m_rtnHtIdW = max(pCxrEntry->m_pGroup->m_rtnHtIdW, cxrCall.m_pGroup->m_htIdW.AsInt());
				}
			}
		}

		if (mod.m_instrW > 0) {
			sprintf(defineStr, "%s_INSTR_W", mod.m_modName.Upper().c_str());
			sprintf(typeName, "%sInstr_t", mod.m_modName.Uc().c_str());

			sprintf(valueStr, "%d", mod.m_instrW);

			AddDefine(&mod, defineStr, valueStr);
			AddTypeDef(typeName, "uint32_t", defineStr, true);
		}

		// Declare return private variables
		if (mod.m_bHasThreads) {
			if (mod.m_threads.m_rtnSelW > 0) {
				sprintf(defineStr, "%s_RTN_SEL_W", mod.m_modName.Upper().c_str());
				sprintf(typeName, "%sRtnSel_t", mod.m_modName.Uc().c_str());

				mod.m_threads.m_pRtnSelType = FindHtIntType(eUnsigned, mod.m_threads.m_rtnSelW);

				sprintf(valueStr, "%d", mod.m_threads.m_rtnSelW);

				AddDefine(&mod, defineStr, valueStr);
				AddTypeDef(typeName, "uint32_t", defineStr, true);
				mod.m_threads.m_htPriv.AddStructField(mod.m_threads.m_pRtnSelType, "rtnSel");
			}

			if (mod.m_threads.m_rtnInstrW > 0) {
				sprintf(defineStr, "%s_RTN_INSTR_W", mod.m_modName.Upper().c_str());
				sprintf(typeName, "%sRtnInstr_t", mod.m_modName.Uc().c_str());

				mod.m_threads.m_pRtnInstrType = FindHtIntType(eUnsigned, mod.m_threads.m_rtnInstrW);

				sprintf(valueStr, "%d", mod.m_threads.m_rtnInstrW);

				AddDefine(&mod, defineStr, valueStr);
				AddTypeDef(typeName, "uint32_t", defineStr, true);

				mod.m_threads.m_htPriv.AddStructField(mod.m_threads.m_pRtnInstrType, "rtnInstr");
			}

			if (mod.m_threads.m_rtnHtIdW > 0) {
				sprintf(defineStr, "%s_RTN_HTID_W", mod.m_modName.Upper().c_str());
				sprintf(typeName, "%sRtnHtId_t", mod.m_modName.Uc().c_str());

				mod.m_threads.m_pRtnHtIdType = FindHtIntType(eUnsigned, mod.m_threads.m_rtnHtIdW);

				sprintf(valueStr, "%d", mod.m_threads.m_rtnHtIdW);

				AddDefine(&mod, defineStr, valueStr);
				AddTypeDef(typeName, "uint32_t", defineStr, true);

				mod.m_threads.m_htPriv.AddStructField(mod.m_threads.m_pRtnHtIdType, "rtnHtId");
			}

			if (mod.m_threads.m_bRtnJoin)
				mod.m_threads.m_htPriv.AddStructField2(&g_bool, "rtnJoin");
		}
	}

	// Create the cxr interface links between modules
	bool bError = false;
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &srcMod = *m_modList[modIdx];
		if (!srcMod.m_bIsUsed) continue;	// only check required modules

		for (size_t callIdx = 0; callIdx < srcMod.m_cxrCallList.size(); callIdx += 1) {
			CCxrCall & cxrCall = srcMod.m_cxrCallList[callIdx];
			CThreads *pSrcGroup = cxrCall.m_pGroup;

			ECxrType cxrType = cxrCall.m_bXfer ? CxrTransfer : CxrCall;

			CModule & dstMod = *cxrCall.m_pairedFunc.m_pMod;
			int dstEntryIdx = cxrCall.m_pairedFunc.m_idx;
			CCxrEntry * pDstCxrEntry = dstMod.m_cxrEntryList[dstEntryIdx];
			CThreads * pDstGroup = pDstCxrEntry->m_pGroup;
			vector<CField *> * pFieldList = &pDstCxrEntry->m_paramList;

			// find return select ID
			int rtnSelId = -1;
			for (size_t callListIdx = 0; callListIdx < pDstCxrEntry->m_pairedCallList.size(); callListIdx += 1) {
				CModIdx & callModIdx = pDstCxrEntry->m_pairedCallList[callListIdx];

				if (callModIdx.m_pMod == &srcMod && callModIdx.m_idx == callIdx)
					rtnSelId = (int)callListIdx;
			}
			HtlAssert(rtnSelId >= 0);

			// check if interface structure is empty
			bool bCxrIntfFields;
			if (cxrType == CxrCall) {

				bCxrIntfFields = pSrcGroup->m_htIdW.AsInt() > 0 || srcMod.m_pInstrType != 0
					|| pFieldList->size() > 0;

			} else { // else Transfer

				bCxrIntfFields = pSrcGroup->m_rtnSelW > 0 || pSrcGroup->m_rtnHtIdW > 0 || pSrcGroup->m_rtnInstrW > 0
					|| pFieldList->size() > 0;

			}

			// Loop through instances for both src and dst
			bool bFoundLinkInfo = false;
			for (size_t srcIdx = 0; srcIdx < srcMod.m_modInstList.size(); srcIdx += 1) {
				CModInst & srcModInst = srcMod.m_modInstList[srcIdx];

				// Find number of destination instances
				int instCnt = 0;
				for (size_t dstIdx = 0; dstIdx < dstMod.m_modInstList.size(); dstIdx += 1) {

					// check if instance file had information for this link
					if (AreModInstancesLinked(srcModInst, dstMod.m_modInstList[dstIdx]))
						instCnt += 1;
				}

				int instIdx = 0;
				for (size_t dstIdx = 0; dstIdx < dstMod.m_modInstList.size(); dstIdx += 1) {
					CModInst & dstModInst = dstMod.m_modInstList[dstIdx];

					// check if instance file had information for this link
					if (!AreModInstancesLinked(srcModInst, dstModInst))
						continue;

					// found link info
					bFoundLinkInfo = true;

					// insert call link
					srcModInst.m_cxrIntfList.push_back(
						CCxrIntf(cxrCall.m_funcName, srcMod, srcIdx, pSrcGroup, dstMod, dstIdx, pDstGroup,
						cxrType, CxrOut, cxrCall.m_queueW.AsInt(), pFieldList));

					CCxrIntf & srcCxrIntf = srcModInst.m_cxrIntfList.back();
					srcCxrIntf.m_callIdx = callIdx;
					srcCxrIntf.m_instCnt = instCnt;
					srcCxrIntf.m_instIdx = instIdx++;
					srcCxrIntf.m_bCxrIntfFields = bCxrIntfFields;
					srcCxrIntf.m_bRtnJoin = cxrCall.m_bCallFork || cxrCall.m_bXferFork;

					dstModInst.m_cxrIntfList.push_back(
						CCxrIntf(cxrCall.m_funcName, srcMod, srcIdx, pSrcGroup, dstMod, dstIdx, pDstGroup,
						cxrType, CxrIn, cxrCall.m_queueW.AsInt(), pFieldList));

					CCxrIntf & dstCxrIntf = dstModInst.m_cxrIntfList.back();
					dstCxrIntf.m_entryInstr = pDstCxrEntry->m_entryInstr;
					dstCxrIntf.m_reserveCnt = pDstCxrEntry->m_reserveCnt.AsInt();
					dstCxrIntf.m_rtnSelId = rtnSelId;
					dstCxrIntf.m_rtnReplId = srcModInst.m_replId;
					dstCxrIntf.m_bCxrIntfFields = bCxrIntfFields;
					dstCxrIntf.m_bRtnJoin = cxrCall.m_bCallFork || cxrCall.m_bXferFork;

					dstModInst.m_cxrSrcCnt += 1;
				}
			}

			if (!bFoundLinkInfo) {
				// make HIF connection
				HtlAssert(srcMod.m_modInstList.size() == 1 && dstMod.m_modInstList.size() == 1);

				for (size_t srcIdx = 0; srcIdx < srcMod.m_modInstList.size(); srcIdx += 1) {
					CModInst & srcModInst = srcMod.m_modInstList[srcIdx];

					int instCnt = (int)dstMod.m_modInstList.size();
					int instIdx = 0;
					for (size_t dstIdx = 0; dstIdx < dstMod.m_modInstList.size(); dstIdx += 1) {
						CModInst & dstModInst = dstMod.m_modInstList[dstIdx];

						// insert call link
						srcModInst.m_cxrIntfList.push_back(
							CCxrIntf(cxrCall.m_funcName, srcMod, srcIdx, pSrcGroup, dstMod, dstIdx, pDstGroup,
							cxrType, CxrOut, cxrCall.m_queueW.AsInt(), pFieldList));

						CCxrIntf & srcCxrIntf = srcModInst.m_cxrIntfList.back();
						srcCxrIntf.m_callIdx = callIdx;
						srcCxrIntf.m_instCnt = instCnt;
						srcCxrIntf.m_instIdx = instIdx++;
						srcCxrIntf.m_bCxrIntfFields = bCxrIntfFields;
						srcCxrIntf.m_bRtnJoin = cxrCall.m_bCallFork || cxrCall.m_bXferFork;

						dstModInst.m_cxrIntfList.push_back(
							CCxrIntf(cxrCall.m_funcName, srcMod, srcIdx, pSrcGroup, dstMod, dstIdx, pDstGroup,
							cxrType, CxrIn, cxrCall.m_queueW.AsInt(), pFieldList));

						CCxrIntf & dstCxrIntf = dstModInst.m_cxrIntfList.back();
						dstCxrIntf.m_entryInstr = pDstCxrEntry->m_entryInstr;
						dstCxrIntf.m_rtnSelId = rtnSelId;
						dstCxrIntf.m_rtnReplId = srcModInst.m_replId;
						dstCxrIntf.m_bCxrIntfFields = bCxrIntfFields;
						dstCxrIntf.m_bRtnJoin = cxrCall.m_bCallFork || cxrCall.m_bXferFork;

						dstModInst.m_cxrSrcCnt += 1;
					}
				}
			}
		}

		for (size_t rtnIdx = 0; rtnIdx < srcMod.m_cxrReturnList.size(); rtnIdx += 1) {
			CCxrReturn &cxrReturn = srcMod.m_cxrReturnList[rtnIdx];
			CThreads * pSrcGroup = cxrReturn.m_pGroup;
			vector<CField *> * pFieldList = &cxrReturn.m_paramList;

			for (size_t callIdx = 0; callIdx < cxrReturn.m_pairedCallList.size(); callIdx += 1) {
				CModule &dstMod = *cxrReturn.m_pairedCallList[callIdx].m_pMod;
				int dstCallIdx = cxrReturn.m_pairedCallList[callIdx].m_idx;
				CCxrCall &dstCxrCall = dstMod.m_cxrCallList[dstCallIdx];
				CThreads * pDstGroup = dstCxrCall.m_pGroup;

				bool bCxrIntfFields = pDstGroup->m_htIdW.AsInt() > 0 || dstMod.m_pInstrType != 0
					|| pFieldList->size() > 0;

				// Loop through instances for both src and dst
				bool bFoundLinkInfo = false;
				for (size_t srcIdx = 0; srcIdx < srcMod.m_modInstList.size(); srcIdx += 1) {
					CModInst & srcModInst = srcMod.m_modInstList[srcIdx];

					int instIdx = 0;
					for (size_t dstIdx = 0; dstIdx < dstMod.m_modInstList.size(); dstIdx += 1) {
						CModInst & dstModInst = dstMod.m_modInstList[dstIdx];

						// check if instance file had information for this link
						if (AreModInstancesLinked(dstModInst, srcModInst)) {

							// found link info
							bFoundLinkInfo = true;

							// record replCnt for return dst
							if (dstModInst.m_replCnt > srcModInst.m_replCnt)
								srcMod.m_maxRtnReplCnt = max(srcMod.m_maxRtnReplCnt, dstModInst.m_replCnt);

							// insert return link
							srcModInst.m_cxrIntfList.push_back(
								CCxrIntf(dstCxrCall.m_funcName, srcMod, srcIdx, pSrcGroup, dstMod, dstIdx, pDstGroup,
								CxrReturn, CxrOut, dstCxrCall.m_queueW.AsInt(), pFieldList));

							CCxrIntf & srcCxrIntf = srcModInst.m_cxrIntfList.back();
							srcCxrIntf.m_rtnIdx = rtnIdx;
							srcCxrIntf.m_callIdx = callIdx;
							srcCxrIntf.m_instIdx = instIdx++;
							srcCxrIntf.m_rtnReplId = dstModInst.m_replId;
							srcCxrIntf.m_bCxrIntfFields = bCxrIntfFields;
							srcCxrIntf.m_bRtnJoin = cxrReturn.m_bRtnJoin;

							dstModInst.m_cxrIntfList.push_back(
								CCxrIntf(dstCxrCall.m_funcName, srcMod, srcIdx, pSrcGroup, dstMod, dstIdx, pDstGroup,
								CxrReturn, CxrIn, dstCxrCall.m_queueW.AsInt(), pFieldList));

							CCxrIntf & dstCxrIntf = dstModInst.m_cxrIntfList.back();
							dstCxrIntf.m_bCxrIntfFields = bCxrIntfFields;
							dstCxrIntf.m_bRtnJoin = cxrReturn.m_bRtnJoin;

							dstModInst.m_cxrSrcCnt += 1;
						}
					}
				}

				if (!bFoundLinkInfo) {
					// make HIF connection
					HtlAssert(srcMod.m_modInstList.size() == 1 && dstMod.m_modInstList.size() == 1);

					// 1->1
					for (size_t srcIdx = 0; srcIdx < srcMod.m_modInstList.size(); srcIdx += 1) {
						CModInst & srcModInst = srcMod.m_modInstList[srcIdx];

						for (size_t dstIdx = 0; dstIdx < dstMod.m_modInstList.size(); dstIdx += 1) {
							CModInst & dstModInst = dstMod.m_modInstList[dstIdx];

							// insert return link
							srcModInst.m_cxrIntfList.push_back(
								CCxrIntf(dstCxrCall.m_funcName, srcMod, srcIdx, pSrcGroup, dstMod, dstIdx, pDstGroup,
								CxrReturn, CxrOut, dstCxrCall.m_queueW.AsInt(), pFieldList));

							CCxrIntf & srcCxrIntf = srcModInst.m_cxrIntfList.back();
							srcCxrIntf.m_rtnIdx = rtnIdx;
							srcCxrIntf.m_callIdx = callIdx;
							srcCxrIntf.m_rtnReplId = dstModInst.m_replId;
							srcCxrIntf.m_bCxrIntfFields = bCxrIntfFields;
							srcCxrIntf.m_bRtnJoin = cxrReturn.m_bRtnJoin;

							dstModInst.m_cxrIntfList.push_back(
								CCxrIntf(dstCxrCall.m_funcName, srcMod, srcIdx, pSrcGroup, dstMod, dstIdx, pDstGroup,
								CxrReturn, CxrIn, dstCxrCall.m_queueW.AsInt(), pFieldList));

							CCxrIntf & dstCxrIntf = dstModInst.m_cxrIntfList.back();
							dstCxrIntf.m_bCxrIntfFields = bCxrIntfFields;
							dstCxrIntf.m_bRtnJoin = cxrReturn.m_bRtnJoin;

							dstModInst.m_cxrSrcCnt += 1;
						}
					}
				}
			}
		}
	}

	// for each instance's inbound return intf, find the outbound call intf
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];
		if (!mod.m_bIsUsed) continue;	// only check required modules

		// loop through the instances for each module
		for (size_t modInstIdx = 0; modInstIdx < mod.m_modInstList.size(); modInstIdx += 1) {
			CModInst &modInst = mod.m_modInstList[modInstIdx];

			// loop through the cxr interfaces looking for inbound return
			for (size_t rtnIntfIdx = 0; rtnIntfIdx < modInst.m_cxrIntfList.size(); rtnIntfIdx += 1) {
				CCxrIntf &cxrRtnIntf = modInst.m_cxrIntfList[rtnIntfIdx];

				if (cxrRtnIntf.m_cxrDir != CxrIn) continue;
				if (cxrRtnIntf.m_cxrType != CxrReturn) continue;

				// loop through csr interfaces looking for matching outbound call
				size_t callIntfIdx;
				for (callIntfIdx = 0; callIntfIdx < modInst.m_cxrIntfList.size(); callIntfIdx += 1) {
					CCxrIntf &cxrCallIntf = modInst.m_cxrIntfList[callIntfIdx];

					if (cxrCallIntf.m_cxrDir != CxrOut) continue;
					if (cxrCallIntf.m_cxrType != CxrCall) continue;
					if (cxrCallIntf.m_funcName != cxrRtnIntf.m_funcName) continue;

					cxrRtnIntf.m_callIntfIdx = callIntfIdx;
					break;
				}

				HtlAssert(callIntfIdx < modInst.m_cxrIntfList.size());
			}
		}
	}

	// Now that the instance connectivity is filled in, assign module interface names
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];
		if (!mod.m_bIsUsed) continue;	// only check required modules

		// loop through the instances for each module
		for (size_t modInstIdx = 0; modInstIdx < mod.m_modInstList.size(); modInstIdx += 1) {
			CModInst &modInst = mod.m_modInstList[modInstIdx];

			// loop through the cxr interfaces for a module's instance
			for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
				CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];
				if (cxrIntf.m_dstInstName.size() > 0) continue;


				// we are filling in the instance names for the interfaces
				//   if there is only one interface with a specific module name, then dst inst name is module name
				//   else inst name is module name with appended inst idx

				if (cxrIntf.m_cxrDir == CxrIn) {
					int modCnt = 0;
					for (size_t intfIdx2 = intfIdx; intfIdx2 < modInst.m_cxrIntfList.size(); intfIdx2 += 1) {
						CCxrIntf &cxrIntf2 = modInst.m_cxrIntfList[intfIdx2];

						if (cxrIntf2.m_cxrDir != CxrIn) continue;
						if (cxrIntf2.m_srcInstName.size() > 0) continue;

						if (cxrIntf.m_pSrcMod == cxrIntf2.m_pSrcMod && cxrIntf.m_funcName == cxrIntf2.m_funcName)
							modCnt += 1;
					}

					for (size_t intfIdx2 = intfIdx; intfIdx2 < modInst.m_cxrIntfList.size(); intfIdx2 += 1) {
						CCxrIntf &cxrIntf2 = modInst.m_cxrIntfList[intfIdx2];

						if (cxrIntf2.m_cxrDir != CxrIn) continue;
						if (cxrIntf2.m_srcInstName.size() > 0) continue;

						if (cxrIntf.m_pSrcMod == cxrIntf2.m_pSrcMod) {
							cxrIntf2.m_dstInstName = cxrIntf.m_pDstMod->m_modName;
							cxrIntf2.m_srcInstName = cxrIntf.m_pSrcMod->m_modName;
							if (modCnt > 1) {
								cxrIntf2.m_srcInstName += ('0' + cxrIntf2.m_srcInstIdx % modCnt);
								cxrIntf2.m_srcReplId = cxrIntf2.m_srcInstIdx % modCnt;
								cxrIntf2.m_srcReplCnt = modCnt;
							}
						}
					}
				} else {
					// CxrOut
					int modCnt = 0;
					for (size_t intfIdx2 = intfIdx; intfIdx2 < modInst.m_cxrIntfList.size(); intfIdx2 += 1) {
						CCxrIntf &cxrIntf2 = modInst.m_cxrIntfList[intfIdx2];

						if (cxrIntf2.m_cxrDir != CxrOut) continue;
						if (cxrIntf2.m_dstInstName.size() > 0) continue;

						if (cxrIntf.m_pDstMod == cxrIntf2.m_pDstMod && cxrIntf.m_funcName == cxrIntf2.m_funcName)
							modCnt += 1;
					}

					for (size_t intfIdx2 = intfIdx; intfIdx2 < modInst.m_cxrIntfList.size(); intfIdx2 += 1) {
						CCxrIntf &cxrIntf2 = modInst.m_cxrIntfList[intfIdx2];

						if (cxrIntf2.m_cxrDir != CxrOut) continue;
						if (cxrIntf2.m_dstInstName.size() > 0) continue;

						if (cxrIntf.m_pDstMod == cxrIntf2.m_pDstMod) {
							cxrIntf2.m_dstInstName = cxrIntf.m_pDstMod->m_modName;
							cxrIntf2.m_srcInstName = cxrIntf.m_pSrcMod->m_modName;
							if (modCnt > 1) {
								cxrIntf2.m_dstInstName += ('0' + cxrIntf2.m_dstInstIdx % modCnt);
								cxrIntf2.m_dstReplId = cxrIntf2.m_dstInstIdx % modCnt;
								cxrIntf2.m_dstReplCnt = modCnt;
							}
						}
					}

				}
			}
		}
	}

	// determine width of async call count variable
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];
		if (!mod.m_bIsUsed) continue;	// only check required modules

		mod.m_bHtIdInit = false;

		for (size_t callIdx = 0; callIdx < mod.m_cxrCallList.size(); callIdx += 1) {

			CCxrCall & cxrCall = mod.m_cxrCallList[callIdx];

			if (!cxrCall.m_bCallFork) continue;

			CCxrEntry * pCxrEntry = cxrCall.m_pPairedEntry;//cxrCall.m_pairedEntry.m_pMod->m_cxrEntryList[cxrCall.m_pairedEntry.m_idx];
			cxrCall.m_forkCntW = max(cxrCall.m_queueW.AsInt(), pCxrEntry->m_pGroup->m_htIdW.AsInt()) + 1;

			mod.m_bHtIdInit |= cxrCall.m_pGroup->m_htIdW.AsInt() > 0;
		}
	}

	// Initialize rtnReplId within groups
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];

		if (!mod.m_bIsUsed) continue;	// only check required modules

		bool bReplReturn = false;
		int maxRtnReplId = -1;

		// loop through each modInst and find returns with multiple destinations
		for (size_t modInstIdx = 0; modInstIdx < mod.m_modInstList.size(); modInstIdx += 1) {
			CModInst & modInst = mod.m_modInstList[modInstIdx];

			bool bAsync = false;
			do {

				///////////////////////////////////////////////////////////
				// generate Return_func routine

				for (size_t rtnIdx = 0; rtnIdx < mod.m_cxrReturnList.size(); rtnIdx += 1) {

					int rtnReplCnt = 0;
					for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
						CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

						if (cxrIntf.m_cxrDir == CxrIn) continue;
						if (cxrIntf.IsCallOrXfer()) continue;
						if (cxrIntf.m_rtnIdx != (int)rtnIdx) continue;

						rtnReplCnt += 1;
						maxRtnReplId = max(maxRtnReplId, cxrIntf.m_rtnReplId);
					}

					if (rtnReplCnt > 1) {
						bReplReturn = true;

						// mark cxrIntf as needing rtnReplId compare
						for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
							CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

							if (cxrIntf.m_cxrDir == CxrIn) continue;
							if (cxrIntf.IsCallOrXfer()) continue;
							if (cxrIntf.m_rtnIdx != (int)rtnIdx) continue;

							cxrIntf.m_bMultiInstRtn = true;
						}
					}
				}

				bAsync = !bAsync;
			} while (!bAsync);
		}

		// Declare return private variables
		if (bReplReturn) {
			if (mod.m_bHasThreads) {

				string typeName;
				switch (FindLg2(maxRtnReplId)) {
				case 1: typeName = "ht_uint1"; break;
				case 2: typeName = "ht_uint2"; break;
				case 3: typeName = "ht_uint3"; break;
				case 4: typeName = "ht_uint4"; break;
				default: HtlAssert(0);
				}

				mod.m_threads.m_htPriv.AddStructField(FindType(typeName), "rtnReplSel");
			}
		}
	}

	checkModInstParamsUsed();

	if (bError)
		printf("Fatal - previous errors prevent file generation\n");
}

bool CDsnInfo::AreModInstancesLinked(CModInst & srcModInst, CModInst & dstModInst)
{
	static bool bErrorReported = false;

	for (size_t srcIdx = 0; srcIdx < srcModInst.m_modPaths.size(); srcIdx += 1) {
		string & srcPath = srcModInst.m_modPaths[srcIdx];

		for (size_t dstIdx = 0; dstIdx < dstModInst.m_modPaths.size(); dstIdx += 1) {
			string & dstPath = dstModInst.m_modPaths[dstIdx];

			if (strncmp(srcPath.c_str(), dstPath.c_str(), srcPath.length()) == 0) {

				// found a path match
				//   check if replicated instances should be linked
				int srcReplCnt = srcModInst.m_replCnt;
				int dstReplCnt = dstModInst.m_replCnt;
				if (srcReplCnt < dstReplCnt) {
					int dstRangeSize = dstReplCnt / srcReplCnt;
					if (!bErrorReported && dstRangeSize * srcReplCnt != dstReplCnt) { // make sure divides evenly
						ParseMsg(Error, "source and destination of call/return must have replication counts that are integer multiples");
						ParseMsg(Info, "modPath=%s replCnt=%d", srcPath.c_str(), srcReplCnt);
						ParseMsg(Info, "modPath=%s replCnt=%d", dstPath.c_str(), dstReplCnt);
						bErrorReported = true;
					}

					int dstRangeLow = srcModInst.m_replId * dstRangeSize;
					int dstRangeHigh = (srcModInst.m_replId + 1) * dstRangeSize - 1;

					return dstRangeLow <= dstModInst.m_replId && dstModInst.m_replId <= dstRangeHigh;

				} else {
					int srcRangeSize = srcReplCnt / dstReplCnt;
					if (!bErrorReported && srcRangeSize * dstReplCnt != srcReplCnt) { // make sure divides evenly
						ParseMsg(Error, "source and destination of call/return must have replication counts that are integer multiples");
						ParseMsg(Info, "modPath=%s replCnt=%d", srcPath.c_str(), srcReplCnt);
						ParseMsg(Info, "modPath=%s replCnt=%d", dstPath.c_str(), dstReplCnt);
						bErrorReported = true;
					}

					int srcRangeLow = dstModInst.m_replId * srcRangeSize;
					int srcRangeHigh = (dstModInst.m_replId + 1) * srcRangeSize - 1;

					return srcRangeLow <= srcModInst.m_replId && srcModInst.m_replId <= srcRangeHigh;
				}
			}
		}
	}
	return false;
}

void CDsnInfo::GenModCxrStatements(CModule &mod, int modInstIdx)
{
	HtlAssert(mod.m_modInstList.size() > 0);
	CModInst & modInst = mod.m_modInstList[modInstIdx];

	string vcdModName = VA("Pers%s", mod.m_modName.Uc().c_str());

	string instIdStr = GenIndexStr(mod.m_instIdCnt > 1, "%d", modInst.m_instParams.m_instId);
	string unitNameUc = !g_appArgs.IsModuleUnitNamesEnabled() ? m_unitName.Uc() : "";

	CHtCode & iplPostInstr = mod.m_clkRate == eClk1x ? m_iplPostInstr1x : m_iplPostInstr2x;
	CHtCode & iplReg = mod.m_clkRate == eClk1x ? m_iplReg1x : m_iplReg2x;
	CHtCode	& cxrPreInstr = mod.m_clkRate == eClk1x ? m_cxrPreInstr1x : m_cxrPreInstr2x;
	CHtCode	& cxrT0Stage = mod.m_clkRate == eClk1x ? m_cxrT0Stage1x : m_cxrT0Stage2x;
	CHtCode	& cxrTsStage = mod.m_clkRate == eClk1x ? m_cxrTsStage1x : m_cxrTsStage2x;
	CHtCode	& cxrPostInstr = mod.m_clkRate == eClk1x ? m_cxrPostInstr1x : m_cxrPostInstr2x;
	CHtCode	& cxrReg = mod.m_clkRate == eClk1x ? m_cxrReg1x : m_cxrReg2x;
	CHtCode	& cxrPostReg = mod.m_clkRate == eClk1x ? m_cxrPostReg1x : m_cxrPostReg2x;
	CHtCode	& cxrOut = mod.m_clkRate == eClk1x ? m_cxrOut1x : m_cxrOut2x;

	string reset = mod.m_clkRate == eClk2x ? "c_reset2x" : "r_reset1x";

	if (mod.m_instrList.size() > 0) {
		m_cxrDefines.Append("//////////////////////////////////\n");
		m_cxrDefines.Append("// Instruction defines\n");
		m_cxrDefines.Append("\n");
		m_cxrDefines.Append("#if !defined(TOPOLOGY_HEADER)\n");

		for (size_t instrIdx = 0; instrIdx < mod.m_instrList.size(); instrIdx += 1) {

			m_cxrDefines.Append("#define %s\t\t%du\n", mod.m_instrList[instrIdx].c_str(), (int)instrIdx);
		}
		m_cxrDefines.Append("#endif\n");
		m_cxrDefines.Append("\n");
	}

	///////////////////////////////////////////////////////////////////////
	// Generate instruction call/transfer/return function and busy macros

	g_appArgs.GetDsnRpt().AddLevel("Call/Return\n");

	///////////////////////////////////////////////////////////
	// generate ReturnBusy_func routine

	for (size_t rtnIdx = 0; rtnIdx < mod.m_cxrReturnList.size(); rtnIdx += 1) {
		if (!mod.m_bIsUsed) continue;

		CCxrReturn & cxrReturn = mod.m_cxrReturnList[rtnIdx];

		CHtString &funcName = cxrReturn.m_funcName;

		m_cxrRegDecl.Append("\tbool ht_noload c_SendReturnBusy_%s;\n", funcName.c_str());
		m_cxrRegDecl.Append("\tbool ht_noload c_SendReturnAvail_%s;\n", funcName.c_str());

		cxrPreInstr.Append("\tc_SendReturnAvail_%s = false;\n", funcName.c_str());

		string vcdModName = VA("Pers%s", mod.m_modName.Uc().c_str());
		GenModTrace(eVcdUser, vcdModName, VA("SendReturnBusy_%s()", funcName.c_str()),
			VA("c_SendReturnBusy_%s", funcName.c_str()));

		if (cxrReturn.m_pairedCallList.size() > 0) {
			cxrPostReg.Append("#\tifdef _HTV\n");

			const char * pSeparator = "";
			cxrPostReg.Append("\tc_SendReturnBusy_%s = ", funcName.c_str());
			for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
				CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

				if (cxrIntf.m_cxrDir == CxrIn) continue;
				if (cxrIntf.IsCallOrXfer()) continue;
				if (cxrIntf.m_rtnIdx != (int)rtnIdx) continue;

				if (cxrReturn.m_pairedCallList.size() == 1) {

					cxrPostReg.Append("%sr_%s_%sAvlCntZero%s", pSeparator,
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());

				} else {
					cxrPostReg.Append("%sr_t%d_htPriv.m_rtnSel == %d && r_%s_%sAvlCntZero%s",
						pSeparator, mod.m_execStg, cxrIntf.m_callIdx,
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());

				}
				pSeparator = " || ";
			}
			cxrPostReg.Append(";\n");

			cxrPostReg.Append("#\telse\n");

			pSeparator = "";
			cxrPostReg.Append("\tc_SendReturnBusy_%s = ", funcName.c_str());
			for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
				CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

				if (cxrIntf.m_cxrDir == CxrIn) continue;
				if (cxrIntf.IsCallOrXfer()) continue;
				if (cxrIntf.m_rtnIdx != (int)rtnIdx) continue;

				if (cxrReturn.m_pairedCallList.size() == 1) {

					cxrPostReg.Append("%sr_%s_%sAvlCntZero%s\n", pSeparator,
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());

				} else {
					cxrPostReg.Append("%s(r_t%d_htPriv.m_rtnSel == %d && r_%s_%sAvlCntZero%s)\n",
						pSeparator,
						mod.m_execStg,
						cxrIntf.m_callIdx,
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());

				}
				pSeparator = "\t\t|| ";
			}
			cxrPostReg.Append("\t\t|| (%s_RND_RETRY && !!(g_rndRetry() & 1));\n", mod.m_modName.Upper().c_str());

			cxrPostReg.Append("#\tendif\n");

		} else {
			cxrPostReg.Append("\tc_SendReturnBusy_%s = false;\n", funcName.c_str());
		}

		g_appArgs.GetDsnRpt().AddLevel("bool SendReturnBusy_%s()\n", funcName.c_str());
		g_appArgs.GetDsnRpt().EndLevel();

		m_cxrFuncDecl.Append("\tbool SendReturnBusy_%s();\n", funcName.c_str());

		m_cxrMacros.Append("bool CPers%s%s%s::SendReturnBusy_%s()\n",
			unitNameUc.c_str(), mod.m_modName.Uc().c_str(), instIdStr.c_str(), funcName.c_str());
		m_cxrMacros.Append("{\n");

		m_cxrMacros.Append("\tassert_msg(r_t%d_htValid, \"Runtime check failed in CPers%s::SendReturnBusy_%s()"
			" - thread is not valid\");\n", mod.m_execStg, mod.m_modName.Uc().c_str(), funcName.c_str());
		m_cxrMacros.Append("\n");

		m_cxrMacros.Append("\tc_SendReturnAvail_%s = !c_SendReturnBusy_%s;\n", funcName.c_str(), funcName.c_str());
		m_cxrMacros.Append("\treturn c_SendReturnBusy_%s;\n", funcName.c_str());

		m_cxrMacros.Append("}\n");
		m_cxrMacros.Append("\n");
	}

	///////////////////////////////////////////////////////////
	// generate Return_func routine

	for (size_t rtnIdx = 0; rtnIdx < mod.m_cxrReturnList.size(); rtnIdx += 1) {
		if (!mod.m_bIsUsed) continue;

		CCxrReturn & cxrReturn = mod.m_cxrReturnList[rtnIdx];

		CHtString &funcName = cxrReturn.m_funcName;//m_pairedCallList[0].m_pMod->m_cxrCallList[cxrReturn.m_pairedCallList[0].m_idx].m_funcName;

		g_appArgs.GetDsnRpt().AddLevel("bool SendReturn_%s(", funcName.c_str());

		m_cxrFuncDecl.Append("\tvoid SendReturn_%s(", funcName.c_str());
		char * pSeparater = "";
		for (size_t fldIdx = 0; fldIdx < cxrReturn.m_paramList.size(); fldIdx += 1) {
			CField * pField = cxrReturn.m_paramList[fldIdx];

			g_appArgs.GetDsnRpt().AddText("%s%s %s", pSeparater, pField->m_pType->m_typeName.c_str(), pField->m_name.c_str());

			m_cxrFuncDecl.Append("%s%s %s", pSeparater, pField->m_pType->m_typeName.c_str(), pField->m_name.c_str());
			pSeparater = ", ";
		}

		g_appArgs.GetDsnRpt().AddText(")\n");
		g_appArgs.GetDsnRpt().EndLevel();

		m_cxrFuncDecl.Append(");\n");

		m_cxrMacros.Append("void CPers%s%s%s::SendReturn_%s(",
			unitNameUc.c_str(), mod.m_modName.Uc().c_str(), instIdStr.c_str(), funcName.c_str());
		pSeparater = "";
		for (size_t fldIdx = 0; fldIdx < cxrReturn.m_paramList.size(); fldIdx += 1) {
			CField * pField = cxrReturn.m_paramList[fldIdx];

			m_cxrMacros.Append("%s%s %s", pSeparater, pField->m_pType->m_typeName.c_str(), pField->m_name.c_str());
			pSeparater = ", ";
		}
		m_cxrMacros.Append(")\n");

		m_cxrMacros.Append("{\n");

		m_cxrMacros.Append("\tassert_msg(r_t%d_htValid, \"Runtime check failed in CPers%s::SendReturn_%s()"
			" - thread is not valid\");\n", mod.m_execStg, mod.m_modName.Uc().c_str(), funcName.c_str());
		m_cxrMacros.Append("\tassert_msg(c_t%d_htCtrl == HT_INVALID, \"Runtime check failed in CPers%s::SendReturn_%s()"
			" - an Ht control routine was already called\");\n", mod.m_execStg, mod.m_modName.Uc().c_str(), funcName.c_str());
		m_cxrMacros.Append("\tassert_msg(c_SendReturnAvail_%s, \"Runtime check failed in CPers%s::SendReturn_%s()"
			" - expected SendReturnBusy_%s() to have been called and not busy\");\n",
			funcName.c_str(), mod.m_modName.Uc().c_str(), funcName.c_str(), funcName.c_str());
		m_cxrMacros.Append("\n");

		if (cxrReturn.m_pairedCallList.size() > 0) {
			for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
				CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

				if (cxrIntf.m_cxrDir == CxrIn) continue;
				if (cxrIntf.IsCallOrXfer()) continue;
				if (cxrIntf.m_rtnIdx != (int)rtnIdx) continue;

				if (cxrIntf.m_bMultiInstRtn && cxrReturn.m_pairedCallList.size() > 1) {

					m_cxrMacros.Append("\tc_%s_%sRdy%s = r_t%d_htPriv.m_rtnSel == %d && r_t%d_htPriv.m_rtnReplSel == %d;\n",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
						mod.m_execStg,
						cxrIntf.m_callIdx,
						mod.m_execStg,
						cxrIntf.m_rtnReplId);

				} else if (cxrIntf.m_bMultiInstRtn) {

					m_cxrMacros.Append("\tc_%s_%sRdy%s = r_t%d_htPriv.m_rtnReplSel == %d;\n",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
						mod.m_execStg,
						cxrIntf.m_rtnReplId);

				} else if (cxrReturn.m_pairedCallList.size() > 1) {

					m_cxrMacros.Append("\tc_%s_%sRdy%s = r_t%d_htPriv.m_rtnSel == %d;\n",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
						mod.m_execStg,
						cxrIntf.m_callIdx);

				} else {
					m_cxrMacros.Append("\tc_%s_%sRdy%s = true;\n",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
				}

				char typeName[64];
				sprintf(typeName, "%sHtId_t", cxrIntf.m_pDstMod->m_modName.Uc().c_str());

				CTypeDef * pTypeDef = FindTypeDef(typeName);

				if (pTypeDef/* && pTypeDef->m_width.AsInt() > 0*/)
					m_cxrMacros.Append("\tc_%s_%s%s.m_rtnHtId = (%s)r_t%d_htPriv.m_rtnHtId;\n",
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
					typeName, mod.m_execStg);

				sprintf(typeName, "%sInstr_t", cxrIntf.m_pDstMod->m_modName.Uc().c_str());

				pTypeDef = FindTypeDef(typeName);

				if (pTypeDef && pTypeDef->m_width.AsInt() > 0)
					m_cxrMacros.Append("\tc_%s_%s%s.m_rtnInstr = (%s)r_t%d_htPriv.m_rtnInstr;\n",
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
					typeName, mod.m_execStg);

				if (cxrReturn.m_bRtnJoin) {
					m_cxrMacros.Append("\tc_%s_%s%s.m_rtnJoin = r_t%d_htPriv.m_rtnJoin;\n",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
						mod.m_execStg);
				}

				for (size_t fldIdx = 0; fldIdx < cxrReturn.m_paramList.size(); fldIdx += 1) {
					CField * pField = cxrReturn.m_paramList[fldIdx];

					m_cxrMacros.Append("\tc_%s_%s%s.m_%s = %s;\n",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
						pField->m_name.c_str(), pField->m_name.c_str());
				}
			}

			m_cxrMacros.Append("\n");
			m_cxrMacros.Append("\tc_t%d_htCtrl = HT_RTN;\n", mod.m_execStg);

		} else {
			m_cxrMacros.Append("\tassert_msg(false, \"Runtime check failed in CPers%s::SendReturn_%s()"
				" - return does not have a paired call\");\n", mod.m_modName.Uc().c_str(), funcName.c_str());
		}

		if (g_appArgs.IsInstrTraceEnabled()) {
			m_cxrMacros.Append("\n");
			m_cxrMacros.Append("#\tifndef _HTV\n");
			m_cxrMacros.Append("\tif (Ht::g_instrTraceFp)\n");
			m_cxrMacros.Append("\t\tfprintf(Ht::g_instrTraceFp, \"SendReturn_%s(", funcName.c_str());

			pSeparater = "";
			for (size_t fldIdx = 0; fldIdx < cxrReturn.m_paramList.size(); fldIdx += 1) {
				CField * pField = cxrReturn.m_paramList[fldIdx];

				bool bBasicType;
				CTypeDef * pTypeDef;
				string type = pField->m_type;
				while (!(bBasicType = IsBaseType(type))) {
					if (!(pTypeDef = FindTypeDef(pField->m_type)))
						break;
					type = pTypeDef->m_type;
				}

				if (bBasicType)
					m_cxrMacros.Append("%s%s = 0x%%llx",
					pSeparater, pField->m_name.c_str());
				else
					m_cxrMacros.Append("%s%s = ???",
					pSeparater, pField->m_name.c_str());

				pSeparater = ", ";
			}
			m_cxrMacros.Append(") @ %%lld\\n\"");

			pSeparater = ",\n\t\t\t";
			for (size_t fldIdx = 0; fldIdx < cxrReturn.m_paramList.size(); fldIdx += 1) {
				CField * pField = cxrReturn.m_paramList[fldIdx];

				bool bBasicType;
				CTypeDef * pTypeDef;
				string type = pField->m_type;
				while (!(bBasicType = IsBaseType(type))) {
					if (!(pTypeDef = FindTypeDef(pField->m_type)))
						break;
					type = pTypeDef->m_type;
				}

				if (!bBasicType) continue;

				m_cxrMacros.Append("%s(long long)%s",
					pSeparater, pField->m_name.c_str());

				pSeparater = ", ";
			}
			m_cxrMacros.Append(", (long long)sc_time_stamp().value() / 10000);\n");
			m_cxrMacros.Append("#\tendif\n");
		}

		m_cxrMacros.Append("}\n");
		m_cxrMacros.Append("\n");
	}

	///////////////////////////////////////////////////////////
	// generate CallBusy_func or TransferBusy_func routines

	for (size_t callIdx = 0; callIdx < mod.m_cxrCallList.size(); callIdx += 1) {
		if (!mod.m_bIsUsed) continue;

		CCxrCall & cxrCall = mod.m_cxrCallList[callIdx];

		if (!cxrCall.m_bXfer && cxrCall.m_bCallFork) continue;

		string callType = cxrCall.m_bXfer ? "Transfer" : "Call";
		string funcName = cxrCall.m_funcName.AsStr();
		bool bDestAuto = cxrCall.m_dest == "auto";

		int replCnt = 1;
		for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
			CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

			if (cxrIntf.m_cxrDir == CxrIn) continue;
			if (!cxrIntf.IsCallOrXfer()) continue;
			if (cxrIntf.m_callIdx != (int)callIdx) continue;

			if (cxrIntf.m_instCnt != 1) {
				replCnt = cxrIntf.m_instCnt;
			}
		}
		int replCntW = FindLg2(replCnt - 1);

		string vcdModName = VA("Pers%s", mod.m_modName.Uc().c_str());
		if (bDestAuto) {
			m_cxrRegDecl.Append("\tbool ht_noload c_Send%sBusy_%s;\n", callType.c_str(), funcName.c_str());
			m_cxrRegDecl.Append("\tbool ht_noload c_Send%sAvail_%s;\n", callType.c_str(), funcName.c_str());

			cxrPreInstr.Append("\tc_Send%sAvail_%s = false;\n", callType.c_str(), funcName.c_str());

			GenModTrace(eVcdUser, vcdModName, VA("Send%sBusy_%s()", callType.c_str(), funcName.c_str()),
				VA("c_Send%sBusy_%s", callType.c_str(), funcName.c_str()));
		} else {
			m_cxrRegDecl.Append("\tbool ht_noload c_Send%sBusy_%s[%d];\n", callType.c_str(), funcName.c_str(), replCnt);
			m_cxrRegDecl.Append("\tbool ht_noload c_Send%sAvail_%s[%d];\n", callType.c_str(), funcName.c_str(), replCnt);

			for (int replIdx = 0; replIdx < replCnt; replIdx += 1) {
				cxrPreInstr.Append("\tc_Send%sAvail_%s[%d] = false;\n", callType.c_str(), funcName.c_str(), replIdx);

				GenModTrace(eVcdUser, vcdModName, VA("Send%sBusy_%s(%d)", callType.c_str(), funcName.c_str(), replIdx),
					VA("c_Send%sBusy_%s[%d]", callType.c_str(), funcName.c_str(), replIdx));
			}
		}

		// generate busy signal
		cxrPostReg.Append("#\tifdef _HTV\n");

		for (int replIdx = 0; replIdx < replCnt; replIdx += 1) {
			string indexStr = (bDestAuto || replCnt == 1) ? "" : VA("[%d]", replIdx);

			cxrPostReg.Append("\tc_Send%sBusy_%s%s = ", callType.c_str(), funcName.c_str(), indexStr.c_str());

			const char * pSeparator = "";
			for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
				CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

				if (cxrIntf.m_cxrDir == CxrIn) continue;
				if (!cxrIntf.IsCallOrXfer()) continue;
				if (cxrIntf.m_callIdx != (int)callIdx) continue;

				if (cxrCall.m_dest == "auto") {
					cxrPostReg.Append("%sr_%s_%sAvlCntZero%s",
						pSeparator, cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());

					if (cxrIntf.m_pDstMod->m_clkRate == eClk1x && cxrIntf.m_pDstMod->m_clkRate != mod.m_clkRate)
						cxrPostReg.Append(" || (r_%s_%sRdy%s & r_phase)",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
				} else {
					cxrPostReg.Append("%sr_%s_%sAvlCntZero%s",
						pSeparator, cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), indexStr.c_str());

					if (cxrIntf.m_pDstMod->m_clkRate == eClk1x && cxrIntf.m_pDstMod->m_clkRate != mod.m_clkRate)
						cxrPostReg.Append(" || (r_%s_%sRdy%s & r_phase)",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), indexStr.c_str());

					break;
				}

				pSeparator = " || ";
			}
			cxrPostReg.Append(";\n");

			if (cxrCall.m_dest == "auto")
				break;
		}

		cxrPostReg.Append("#\telse\n");

		for (int replIdx = 0; replIdx < replCnt; replIdx += 1) {
			string indexStr = (bDestAuto || replCnt == 1) ? "" : VA("[%d]", replIdx);

			cxrPostReg.Append("\tc_Send%sBusy_%s%s = ", callType.c_str(), funcName.c_str(), indexStr.c_str());

			const char * pSeparator = "";
			for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
				CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

				if (cxrIntf.m_cxrDir == CxrIn) continue;
				if (!cxrIntf.IsCallOrXfer()) continue;
				if (cxrIntf.m_callIdx != (int)callIdx) continue;

				if (cxrCall.m_dest == "auto") {
					cxrPostReg.Append("%sr_%s_%sAvlCntZero%s",
						pSeparator,
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());

					if (cxrIntf.m_pDstMod->m_clkRate == eClk1x && cxrIntf.m_pDstMod->m_clkRate != mod.m_clkRate)
						cxrPostReg.Append(" || (r_%s_%sRdy%s & r_phase)",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
				} else {
					cxrPostReg.Append("%sr_%s_%sAvlCntZero%s",
						pSeparator,
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), indexStr.c_str());

					if (cxrIntf.m_pDstMod->m_clkRate == eClk1x && cxrIntf.m_pDstMod->m_clkRate != mod.m_clkRate)
						cxrPostReg.Append("%s(r_%s_%sRdy%s & r_phase)",
						pSeparator,
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), indexStr.c_str());
				}

				pSeparator = " || ";
			}

			cxrPostReg.Append("\n");
			cxrPostReg.Append("\t\t|| (%s_RND_RETRY && !!(g_rndRetry() & 1));\n", mod.m_modName.Upper().c_str());

			if (cxrCall.m_dest == "auto")
				break;
		}

		cxrPostReg.Append("#\tendif\n");

		// generate Busy routine
		string paramStr = bDestAuto ? "" : VA("ht_uint%d%s destId", replCntW == 0 ? 1 : replCntW, replCntW == 0 ? " ht_noload" : "");
		string indexStr = (bDestAuto || replCntW == 0) ? "" : "[destId]";

		g_appArgs.GetDsnRpt().AddLevel("bool Send%sBusy_%s(%s)\n",
			cxrCall.m_bXfer ? "Transfer" : "Call", cxrCall.m_funcName.c_str(), paramStr.c_str());
		g_appArgs.GetDsnRpt().EndLevel();

		m_cxrFuncDecl.Append("\tbool Send%sBusy_%s(%s);\n",
			cxrCall.m_bXfer ? "Transfer" : "Call", cxrCall.m_funcName.c_str(), paramStr.c_str());

		m_cxrMacros.Append("bool CPers%s%s%s::Send%sBusy_%s(%s)\n",
			unitNameUc.c_str(), mod.m_modName.Uc().c_str(), instIdStr.c_str(),
			cxrCall.m_bXfer ? "Transfer" : "Call", cxrCall.m_funcName.c_str(), paramStr.c_str());
		m_cxrMacros.Append("{\n");

		m_cxrMacros.Append("\tassert_msg(r_t%d_htValid, \"Runtime check failed in CPers%s::Send%sBusy_%s()"
			" - thread is not valid\");\n", mod.m_execStg, mod.m_modName.Uc().c_str(), cxrCall.m_bXfer ? "Transfer" : "Call", cxrCall.m_funcName.c_str());
		m_cxrMacros.Append("\n");

		m_cxrMacros.Append("\tc_Send%sAvail_%s%s = !c_Send%sBusy_%s%s;\n",
			callType.c_str(), funcName.c_str(), indexStr.c_str(),
			callType.c_str(), funcName.c_str(), indexStr.c_str());
		m_cxrMacros.Append("\treturn c_Send%sBusy_%s%s;\n",
			callType.c_str(), funcName.c_str(), indexStr.c_str());

		m_cxrMacros.Append("}\n");

		m_cxrMacros.Append("\n");
	}

	///////////////////////////////////////////////////////////
	// generate Call_func or Transfer_func routines

	for (size_t callIdx = 0; callIdx < mod.m_cxrCallList.size(); callIdx += 1) {
		if (!mod.m_bIsUsed) continue;

		CCxrCall & cxrCall = mod.m_cxrCallList[callIdx];

		if (!cxrCall.m_bCall && !cxrCall.m_bXfer) continue;

		CModIdx & pairedFunc = cxrCall.m_pairedFunc;
		CCxrEntry * pCxrEntry = pairedFunc.m_pMod->m_cxrEntryList[pairedFunc.m_idx];

		bool bReplIntf = false;
		int replCnt = 1;
		for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
			CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

			if (cxrIntf.m_cxrDir == CxrIn) continue;
			if (!cxrIntf.IsCallOrXfer()) continue;
			if (cxrIntf.m_callIdx != (int)callIdx) continue;

			if (cxrIntf.m_instCnt != 1) {
				bReplIntf = true;
				replCnt = cxrIntf.m_instCnt;
			}
		}

		g_appArgs.GetDsnRpt().AddLevel("void Send%s_%s(",
			cxrCall.m_bXfer ? "Transfer" : "Call", cxrCall.m_funcName.c_str());

		m_cxrFuncDecl.Append("\tvoid Send%s_%s(",
			cxrCall.m_bXfer ? "Transfer" : "Call", cxrCall.m_funcName.c_str());

		m_cxrMacros.Append("void CPers%s%s%s::Send%s_%s(",
			unitNameUc.c_str(), mod.m_modName.Uc().c_str(), instIdStr.c_str(),
			cxrCall.m_bXfer ? "Transfer" : "Call", cxrCall.m_funcName.c_str());

		char const * pSeparater;
		if (!cxrCall.m_bXfer) {
			g_appArgs.GetDsnRpt().AddText("ht_uint%d rtnInstr", mod.m_instrW);
			m_cxrFuncDecl.Append("ht_uint%d rtnInstr", mod.m_instrW);
			m_cxrMacros.Append("ht_uint%d rtnInstr", mod.m_instrW);
			pSeparater = ", ";
		} else
			pSeparater = "";

		if (cxrCall.m_dest == "user") {
			int replCntW = FindLg2(replCnt - 1);

			string paramStr = VA("ht_uint%d%s destId", replCnt == 1 ? 1 : replCntW, replCnt == 1 ? " ht_noload" : "");

			g_appArgs.GetDsnRpt().AddText("%s%s", pSeparater, paramStr.c_str());
			m_cxrFuncDecl.Append("%s%s", pSeparater, paramStr.c_str());
			m_cxrMacros.Append("%s%s", pSeparater, paramStr.c_str());
			pSeparater = ", ";
		}

		for (size_t fldIdx = 0; fldIdx < pCxrEntry->m_paramList.size(); fldIdx += 1) {
			CField * pField = pCxrEntry->m_paramList[fldIdx];

			g_appArgs.GetDsnRpt().AddText("%s%s %s", pSeparater, pField->m_pType->m_typeName.c_str(), pField->m_name.c_str());
			m_cxrFuncDecl.Append("%s%s %s", pSeparater, pField->m_pType->m_typeName.c_str(), pField->m_name.c_str());
			m_cxrMacros.Append("%s%s %s", pSeparater, pField->m_pType->m_typeName.c_str(), pField->m_name.c_str());

			pSeparater = ", ";
		}

		g_appArgs.GetDsnRpt().AddText(")\n");
		g_appArgs.GetDsnRpt().EndLevel();

		m_cxrFuncDecl.Append(");\n");

		m_cxrMacros.Append(")\n");
		m_cxrMacros.Append("{\n");

		m_cxrMacros.Append("\tassert_msg(r_t%d_htValid, \"Runtime check failed in CPers%s::Send%s_%s()"
			" - thread is not valid\");\n", mod.m_execStg, mod.m_modName.Uc().c_str(), cxrCall.m_bXfer ? "Transfer" : "Call", cxrCall.m_funcName.c_str());
		m_cxrMacros.Append("\n");

		for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
			CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

			if (cxrIntf.m_cxrDir == CxrIn) continue;
			if (!cxrIntf.IsCallOrXfer()) continue;
			if (cxrIntf.m_callIdx != (int)callIdx) continue;

			if (cxrIntf.m_instCnt == 1)
				m_cxrMacros.Append("\tc_%s_%sRdy%s = true;\n",
				cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
			else {
				if (cxrCall.m_dest == "auto")
					m_cxrMacros.Append("\tc_%s_%sRdy%s = r_%sCall_roundRobin == %dUL;\n",
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
					cxrCall.m_funcName.c_str(), cxrIntf.m_instIdx);
				else
					m_cxrMacros.Append("\tc_%s_%sRdy%s = destId == %dUL;\n",
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
					cxrIntf.m_instIdx);
			}

			if (cxrCall.m_bXfer) {
				if (cxrCall.m_pGroup->m_rtnSelW > 0)
					m_cxrMacros.Append("\tc_%s_%s%s.m_rtnSel = r_t%d_htPriv.m_rtnSel;\n",
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
					mod.m_execStg);

				if (cxrCall.m_pGroup->m_rtnHtIdW > 0)
					m_cxrMacros.Append("\tc_%s_%s%s.m_rtnHtId = r_t%d_htPriv.m_rtnHtId;\n",
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
					mod.m_execStg);

				if (cxrCall.m_pGroup->m_rtnInstrW > 0)
					m_cxrMacros.Append("\tc_%s_%s%s.m_rtnInstr = r_t%d_htPriv.m_rtnInstr;\n",
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
					mod.m_execStg);

				if (cxrCall.m_bCallFork || cxrCall.m_bXferFork) {
					m_cxrMacros.Append("\tc_%s_%s%s.m_rtnJoin = r_t%d_htPriv.m_rtnJoin;\n",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
						mod.m_execStg);
				}

			} else {
				if (cxrCall.m_pGroup->m_htIdW.AsInt() > 0)
					m_cxrMacros.Append("\tc_%s_%s%s.m_rtnHtId = r_t%d_htId;\n",
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
					mod.m_execStg);

				if (mod.m_instrW > 0)
					m_cxrMacros.Append("\tc_%s_%s%s.m_rtnInstr = rtnInstr;\n",
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());

				if (cxrCall.m_bCallFork || cxrCall.m_bXferFork) {
					m_cxrMacros.Append("\tc_%s_%s%s.m_rtnJoin = false;\n",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
				}
			}

			for (size_t fldIdx = 0; fldIdx < pCxrEntry->m_paramList.size(); fldIdx += 1) {
				CField * pField = pCxrEntry->m_paramList[fldIdx];

				m_cxrMacros.Append("\tc_%s_%s%s.m_%s = %s;\n",
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
					pField->m_name.c_str(), pField->m_name.c_str());
			}
		}
		m_cxrMacros.Append("\n");

		if (bReplIntf && cxrCall.m_dest == "auto") {
			m_cxrMacros.Append("\tc_%sCall_roundRobin = (r_%sCall_roundRobin + 1U) %% %dU;\n",
				cxrCall.m_funcName.c_str(), cxrCall.m_funcName.c_str(), replCnt);
			m_cxrMacros.Append("\n");
		}

		m_cxrMacros.Append("\tassert_msg(c_t%d_htCtrl == HT_INVALID, \"Runtime check failed in CPers%s%s::Send%s_%s()"
			" - an Ht control routine was already called\");\n", mod.m_execStg, mod.m_modName.Uc().c_str(), instIdStr.c_str(),
			cxrCall.m_bXfer ? "Transfer" : "Call", cxrCall.m_funcName.c_str());
		m_cxrMacros.Append("\n");

		m_cxrMacros.Append("\tc_t%d_htCtrl = HT_CALL;\n", mod.m_execStg);

		if (g_appArgs.IsInstrTraceEnabled()) {
			m_cxrMacros.Append("\n");
			m_cxrMacros.Append("#\tifndef _HTV\n");
			m_cxrMacros.Append("\tif (Ht::g_instrTraceFp)\n");
			m_cxrMacros.Append("\t\tfprintf(Ht::g_instrTraceFp, \"Send%s_%s(",
				cxrCall.m_bXfer ? "Transfer" : "Call", cxrCall.m_funcName.c_str());

			pSeparater = "";
			for (size_t fldIdx = 0; fldIdx < pCxrEntry->m_paramList.size(); fldIdx += 1) {
				CField * pField = pCxrEntry->m_paramList[fldIdx];

				bool bBasicType;
				CTypeDef * pTypeDef;
				string type = pField->m_type;
				while (!(bBasicType = IsBaseType(type))) {
					if (!(pTypeDef = FindTypeDef(pField->m_type)))
						break;
					type = pTypeDef->m_type;
				}

				if (bBasicType)
					m_cxrMacros.Append("%s%s = 0x%%llx",
					pSeparater, pField->m_name.c_str());
				else
					m_cxrMacros.Append("%s%s = ???",
					pSeparater, pField->m_name.c_str());

				pSeparater = ", ";
			}
			m_cxrMacros.Append(") @ %%lld\\n\"");

			pSeparater = ",\n\t\t\t";
			for (size_t fldIdx = 0; fldIdx < pCxrEntry->m_paramList.size(); fldIdx += 1) {
				CField * pField = pCxrEntry->m_paramList[fldIdx];

				bool bBasicType;
				CTypeDef * pTypeDef;
				string type = pField->m_type;
				while (!(bBasicType = IsBaseType(type))) {
					if (!(pTypeDef = FindTypeDef(pField->m_type)))
						break;
					type = pTypeDef->m_type;
				}

				if (!bBasicType) continue;

				m_cxrMacros.Append("%s(long long)%s",
					pSeparater, pField->m_name.c_str());

				pSeparater = ", ";
			}
			m_cxrMacros.Append(", (long long)sc_time_stamp().value() / 10000);\n");
			m_cxrMacros.Append("#\tendif\n");
		}

		m_cxrMacros.Append("}\n");
		m_cxrMacros.Append("\n");
	}

	///////////////////////////////////////////////////////////////////////
	// Generate I/O signal declarations

	for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
		CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

		vector<CHtString> replDim;
		if (cxrIntf.GetPortReplCnt() > 1) {
			CHtString str = cxrIntf.GetPortReplCnt();
			replDim.push_back(str);
		}

		if (cxrIntf.m_cxrDir == CxrIn) {
			if (cxrIntf.m_srcReplCnt > 1 && cxrIntf.m_srcReplId != 0) continue;

			m_cxrIoDecl.Append("\t// Inbound %s interface from %s\n",
				cxrIntf.m_cxrType == CxrCall ? "call" : (cxrIntf.m_cxrType == CxrReturn ? "return" : "xfer"),
				cxrIntf.m_pSrcMod->m_modName.c_str());

			GenModDecl(eVcdAll, m_cxrIoDecl, vcdModName, "sc_in<bool>",
				VA("i_%s_%sRdy", cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName()), replDim);

			if (cxrIntf.m_pSrcMod->m_bGvIwComp)
				m_cxrIoDecl.Append("\tsc_in<bool> i_%s_%sCompRdy%s;\n", cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());

			if (cxrIntf.m_bCxrIntfFields)
				m_cxrIoDecl.Append("\tsc_in<C%s_%sIntf> i_%s_%s%s;\n",
				cxrIntf.GetSrcToDstUc(), cxrIntf.GetIntfName(),
				cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());

			m_cxrIoDecl.Append("\tsc_out<bool> o_%s_%sAvl%s;\n",
				cxrIntf.GetPortNameDstToSrcLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());

		} else {
			if (cxrIntf.m_dstReplCnt > 1 && cxrIntf.m_dstReplId != 0) continue;

			m_cxrIoDecl.Append("\t// Outbound %s interace to %s\n",
				cxrIntf.m_cxrType == CxrCall ? "call" : (cxrIntf.m_cxrType == CxrReturn ? "return" : "xfer"),
				cxrIntf.m_pDstMod->m_modName.c_str());

			GenModDecl(eVcdAll, m_cxrIoDecl, vcdModName, "sc_out<bool>",
				VA("o_%s_%sRdy", cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName()), replDim);

			if (cxrIntf.m_pSrcMod->m_bGvIwComp && cxrIntf.m_pDstMod->m_modName.AsStr() != "hif") {
				m_cxrIoDecl.Append("\tsc_out<bool> o_%s_%sCompRdy%s;\n",
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());
			}

			if (cxrIntf.m_bCxrIntfFields)
				m_cxrIoDecl.Append("\tsc_out<C%s_%sIntf> o_%s_%s%s;\n",
				cxrIntf.GetSrcToDstUc(), cxrIntf.GetIntfName(),
				cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());

			m_cxrIoDecl.Append("\tsc_in<bool> i_%s_%sAvl%s;\n",
				cxrIntf.GetPortNameDstToSrcLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());

		}

		m_cxrIoDecl.Append("\n");
	}

	///////////////////////////////////////////////////////////////////////
	// declare and initialize Async Call variables

	for (size_t callIdx = 0; callIdx < mod.m_cxrCallList.size(); callIdx += 1) {
		if (!mod.m_bIsUsed) continue;

		CCxrCall & cxrCall = mod.m_cxrCallList[callIdx];

		if (cxrCall.m_bXfer || !cxrCall.m_bCallFork) continue;

		CModIdx & pairedFunc = cxrCall.m_pairedFunc;
		CCxrEntry * pCxrEntry = pairedFunc.m_pMod->m_cxrEntryList[pairedFunc.m_idx];

		int replCnt = 0;
		for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
			CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

			if (cxrIntf.m_cxrDir == CxrIn) continue;
			if (!cxrIntf.IsCallOrXfer()) continue;
			if (cxrIntf.m_callIdx != (int)callIdx) continue;

			replCnt = cxrIntf.m_instCnt;
			break;
		}
		int replCntW = FindLg2(replCnt, false, true);

		int htIdW = cxrCall.m_pGroup->m_htIdW.AsInt();
		int forkCntW = cxrCall.m_forkCntW + replCntW;
		string funcName = cxrCall.m_funcName.AsStr();

		char rtnCntTypeStr[64];
		sprintf(rtnCntTypeStr, "ht_uint%d", forkCntW);

		bool bDestAuto = cxrCall.m_dest == "auto";

		string declStr = (bDestAuto || replCnt == 1) ? "" : VA("[%d]", replCnt);
		vector<CHtString> replDim;
		if (!bDestAuto && replCnt != 1) {
			char buf[16];
			sprintf(buf, "%d", replCnt);
			CHtString str = (string)buf;
			CLineInfo li;
			str.InitValue(li);
			replDim.push_back(str);
		}

		// declarations
		if (htIdW == 0) {

			GenModDecl(eVcdAll, m_cxrRegDecl, vcdModName, "bool", VA("r_asyncCall%s_waiting", cxrCall.m_funcName.c_str()), replDim);
			m_cxrRegDecl.Append("\tbool c_asyncCall%s_waiting%s;\n", cxrCall.m_funcName.c_str(), declStr.c_str());
			GenModDecl(eVcdAll, m_cxrRegDecl, vcdModName, "bool", VA("r_asyncCall%s_rtnCntFull", cxrCall.m_funcName.c_str()), replDim);
			m_cxrRegDecl.Append("\tbool c_asyncCall%s_rtnCntFull%s;\n", cxrCall.m_funcName.c_str(), declStr.c_str());
			GenModDecl(eVcdAll, m_cxrRegDecl, vcdModName, rtnCntTypeStr, VA("r_asyncCall%s_rtnCnt", cxrCall.m_funcName.c_str()), replDim);
			m_cxrRegDecl.Append("\tht_uint%d c_asyncCall%s_rtnCnt%s;\n", forkCntW, cxrCall.m_funcName.c_str(), declStr.c_str());
			m_cxrRegDecl.Append("\tht_uint%d r_asyncCall%s_rsmInstr%s;\n", mod.m_instrW, cxrCall.m_funcName.c_str(), declStr.c_str());
			m_cxrRegDecl.Append("\tht_uint%d c_asyncCall%s_rsmInstr%s;\n", mod.m_instrW, cxrCall.m_funcName.c_str(), declStr.c_str());

		} else {

			m_cxrRegDecl.Append("\tht_dist_ram<bool, %d> m_asyncCall%s_waiting%s;\n",
				htIdW, cxrCall.m_funcName.c_str(), declStr.c_str());
			m_cxrRegDecl.Append("\tht_dist_ram<bool, %d> m_asyncCall%s_rtnCntFull%s;\n",
				htIdW, cxrCall.m_funcName.c_str(), declStr.c_str());
			m_cxrRegDecl.Append("\tht_dist_ram<ht_uint%d, %d> m_asyncCall%s_rtnCnt%s;\n",
				forkCntW, htIdW, cxrCall.m_funcName.c_str(), declStr.c_str());
			m_cxrRegDecl.Append("\tht_dist_ram<ht_uint%d, %d> m_asyncCall%s_rsmInstr%s;\n",
				mod.m_instrW, htIdW, cxrCall.m_funcName.c_str(), declStr.c_str());
			m_cxrRegDecl.Append("\n");

			m_cxrRegDecl.Append("\tbool c_t%d_asyncCall%s_rtnCntFull%s;\n",
				mod.m_execStg - 1, cxrCall.m_funcName.c_str(), declStr.c_str());
			GenModDecl(eVcdAll, m_cxrRegDecl, vcdModName, "bool",
				VA("r_t%d_asyncCall%s_rtnCntFull",
				mod.m_execStg, cxrCall.m_funcName.c_str()), replDim);
			m_cxrRegDecl.Append("\n");
		}

		for (int replIdx = 0; replIdx < replCnt; replIdx += 1) {
			string replStr = (bDestAuto || replCnt == 1) ? "" : VA("[%d]", replIdx);

			if (htIdW == 0) {

				cxrTsStage.Append("\tc_asyncCall%s_waiting%s = r_asyncCall%s_waiting%s;\n",
					cxrCall.m_funcName.c_str(), replStr.c_str(), cxrCall.m_funcName.c_str(), replStr.c_str());
				cxrTsStage.Append("\tc_asyncCall%s_rtnCnt%s = r_asyncCall%s_rtnCnt%s;\n",
					cxrCall.m_funcName.c_str(), replStr.c_str(), cxrCall.m_funcName.c_str(), replStr.c_str());
				cxrTsStage.Append("\tc_asyncCall%s_rtnCntFull%s = r_asyncCall%s_rtnCntFull%s;\n",
					cxrCall.m_funcName.c_str(), replStr.c_str(), cxrCall.m_funcName.c_str(), replStr.c_str());
				cxrTsStage.Append("\tc_asyncCall%s_rsmInstr%s = r_asyncCall%s_rsmInstr%s;\n",
					cxrCall.m_funcName.c_str(), replStr.c_str(), cxrCall.m_funcName.c_str(), replStr.c_str());

				cxrReg.Append("\tr_asyncCall%s_waiting%s = !%s && c_asyncCall%s_waiting%s;\n",
					cxrCall.m_funcName.c_str(), replStr.c_str(), reset.c_str(), cxrCall.m_funcName.c_str(), replStr.c_str());
				cxrReg.Append("\tr_asyncCall%s_rtnCnt%s = %s ? (ht_uint%d)0 : c_asyncCall%s_rtnCnt%s;\n",
					cxrCall.m_funcName.c_str(), replStr.c_str(), reset.c_str(), forkCntW, cxrCall.m_funcName.c_str(), replStr.c_str());
				cxrReg.Append("\tr_asyncCall%s_rtnCntFull%s = !%s && c_asyncCall%s_rtnCntFull%s;\n",
					cxrCall.m_funcName.c_str(), replStr.c_str(), reset.c_str(), cxrCall.m_funcName.c_str(), replStr.c_str());
				cxrReg.Append("\tr_asyncCall%s_rsmInstr%s = c_asyncCall%s_rsmInstr%s;\n",
					cxrCall.m_funcName.c_str(), replStr.c_str(), cxrCall.m_funcName.c_str(), replStr.c_str());

			} else {

				cxrTsStage.Append("\tm_asyncCall%s_waiting%s.read_addr(r_t%d_htId);\n",
					cxrCall.m_funcName.c_str(), replStr.c_str(), mod.m_execStg);
				cxrTsStage.Append("\tm_asyncCall%s_waiting%s.write_addr(r_t%d_htId);\n",
					cxrCall.m_funcName.c_str(), replStr.c_str(), mod.m_execStg);
				cxrTsStage.Append("\tm_asyncCall%s_rtnCnt%s.read_addr(r_t%d_htId);\n",
					cxrCall.m_funcName.c_str(), replStr.c_str(), mod.m_execStg);
				cxrTsStage.Append("\tm_asyncCall%s_rtnCnt%s.write_addr(r_t%d_htId);\n",
					cxrCall.m_funcName.c_str(), replStr.c_str(), mod.m_execStg);
				cxrTsStage.Append("\tm_asyncCall%s_rtnCntFull%s.read_addr(r_t%d_htId);\n",
					cxrCall.m_funcName.c_str(), replStr.c_str(), mod.m_execStg - 1);
				cxrTsStage.Append("\tm_asyncCall%s_rtnCntFull%s.write_addr(r_t%d_htId);\n",
					cxrCall.m_funcName.c_str(), replStr.c_str(), mod.m_execStg);
				cxrTsStage.Append("\tm_asyncCall%s_rsmInstr%s.read_addr(r_t%d_htId);\n",
					cxrCall.m_funcName.c_str(), replStr.c_str(), mod.m_execStg);
				cxrTsStage.Append("\tm_asyncCall%s_rsmInstr%s.write_addr(r_t%d_htId);\n",
					cxrCall.m_funcName.c_str(), replStr.c_str(), mod.m_execStg);
				cxrTsStage.Append("\n");
				cxrTsStage.Append("\tc_t%d_asyncCall%s_rtnCntFull%s = m_asyncCall%s_rtnCntFull%s.read_mem();\n",
					mod.m_execStg - 1, cxrCall.m_funcName.c_str(), replStr.c_str(), cxrCall.m_funcName.c_str(), replStr.c_str());
				cxrTsStage.Append("\n");

				cxrReg.Append("\tm_asyncCall%s_waiting%s.clock();\n",
					cxrCall.m_funcName.c_str(), replStr.c_str());
				cxrReg.Append("\tm_asyncCall%s_rtnCnt%s.clock();\n",
					cxrCall.m_funcName.c_str(), replStr.c_str());
				cxrReg.Append("\tm_asyncCall%s_rtnCntFull%s.clock();\n",
					cxrCall.m_funcName.c_str(), replStr.c_str());
				cxrReg.Append("\tm_asyncCall%s_rsmInstr%s.clock();\n",
					cxrCall.m_funcName.c_str(), replStr.c_str());
				cxrReg.Append("\n");

				cxrPostInstr.Append("\tif (r_t%d_htIdInit) {\n", mod.m_execStg);
				cxrPostInstr.Append("\t\tm_asyncCall%s_waiting%s.write_mem(false);\n",
					cxrCall.m_funcName.c_str(), replStr.c_str());
				cxrPostInstr.Append("\t\tm_asyncCall%s_rtnCnt%s.write_mem(0);\n",
					cxrCall.m_funcName.c_str(), replStr.c_str());
				cxrPostInstr.Append("\t\tm_asyncCall%s_rtnCntFull%s.write_mem(false);\n",
					cxrCall.m_funcName.c_str(), replStr.c_str());
				cxrPostInstr.Append("\t}\n");
				cxrPostInstr.Append("\n");

				cxrReg.Append("\tr_t%d_asyncCall%s_rtnCntFull%s = c_t%d_asyncCall%s_rtnCntFull%s;\n",
					mod.m_execStg, cxrCall.m_funcName.c_str(), replStr.c_str(), mod.m_execStg - 1, cxrCall.m_funcName.c_str(), replStr.c_str());
				m_cxrRegDecl.Append("\n");
			}

			if (bDestAuto)
				break;
		}

		// Generate Async Call Busy Function

		string vcdModName = VA("Pers%s", mod.m_modName.Uc().c_str());
		if (bDestAuto) {
			m_cxrRegDecl.Append("\tbool ht_noload c_SendCallBusy_%s;\n", funcName.c_str());
			m_cxrRegDecl.Append("\tbool ht_noload c_SendCallAvail_%s;\n", funcName.c_str());

			cxrPreInstr.Append("\tc_SendCallAvail_%s = false;\n", funcName.c_str());

			GenModTrace(eVcdUser, vcdModName, VA("SendCallBusy_%s()", funcName.c_str()),
				VA("c_SendCallBusy_%s", funcName.c_str()));
		} else {
			m_cxrRegDecl.Append("\tbool ht_noload c_SendCallBusy_%s[%d];\n", funcName.c_str(), replCnt);
			m_cxrRegDecl.Append("\tbool ht_noload c_SendCallAvail_%s[%d];\n", funcName.c_str(), replCnt);

			for (int replIdx = 0; replIdx < replCnt; replIdx += 1) {
				cxrPreInstr.Append("\tc_SendCallAvail_%s[%d] = false;\n", funcName.c_str(), replIdx);

				GenModTrace(eVcdUser, vcdModName, VA("SendCallBusy_%s(%d)", funcName.c_str(), replIdx),
					VA("c_SendCallBusy_%s[%d]", funcName.c_str(), replIdx));
			}
		}

		cxrPostReg.Append("#\tifdef _HTV\n");

		for (int replIdx = 0; replIdx < replCnt; replIdx += 1) {
			string indexStr = (bDestAuto || replCnt == 1) ? "" : VA("[%d]", replIdx);

			if (htIdW == 0)
				cxrPostReg.Append("\t\tc_SendCallBusy_%s%s = r_asyncCall%s_rtnCntFull%s",
				cxrCall.m_funcName.c_str(), indexStr.c_str(),
				cxrCall.m_funcName.c_str(), indexStr.c_str());
			else
				cxrPostReg.Append("\t\tc_SendCallBusy_%s%s = r_t%d_asyncCall%s_rtnCntFull%s",
				cxrCall.m_funcName.c_str(), indexStr.c_str(),
				mod.m_execStg, cxrCall.m_funcName.c_str(), indexStr.c_str());

			const char * pSeparator = "\n\t\t\t|| ";

			for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
				CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

				if (cxrIntf.m_cxrDir == CxrIn) continue;
				if (!cxrIntf.IsCallOrXfer()) continue;
				if (cxrIntf.m_callIdx != (int)callIdx) continue;

				if (cxrCall.m_dest == "auto")
					cxrPostReg.Append("%sr_%s_%sAvlCntZero%s",
					pSeparator,
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
				else {
					cxrPostReg.Append("%sr_%s_%sAvlCntZero%s",
						pSeparator,
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), indexStr.c_str(),
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), indexStr.c_str());
					break;
				}

				pSeparator = " || ";
			}
			cxrPostReg.Append(";\n");

			if (cxrCall.m_dest == "auto")
				break;
		}

		cxrPostReg.Append("#\telse\n");

		for (int replIdx = 0; replIdx < replCnt; replIdx += 1) {
			string indexStr = (bDestAuto || replCnt == 1) ? "" : VA("[%d]", replIdx);

			if (htIdW == 0)
				cxrPostReg.Append("\t\tc_SendCallBusy_%s%s = r_asyncCall%s_rtnCntFull%s",
				cxrCall.m_funcName.c_str(), indexStr.c_str(),
				cxrCall.m_funcName.c_str(), indexStr.c_str());
			else
				cxrPostReg.Append("\t\tc_SendCallBusy_%s%s = r_t%d_asyncCall%s_rtnCntFull%s",
				cxrCall.m_funcName.c_str(), indexStr.c_str(),
				mod.m_execStg, cxrCall.m_funcName.c_str(), indexStr.c_str());

			const char * pSeparator = "\n\t\t\t|| ";
			for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
				CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

				if (cxrIntf.m_cxrDir == CxrIn) continue;
				if (!cxrIntf.IsCallOrXfer()) continue;
				if (cxrIntf.m_callIdx != (int)callIdx) continue;

				if (cxrCall.m_dest == "auto")
					cxrPostReg.Append("%sr_%s_%sAvlCntZero%s",
					pSeparator,
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
				else {
					cxrPostReg.Append("%sr_%s_%sAvlCntZero%s",
						pSeparator,
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), indexStr.c_str(),
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), indexStr.c_str());
					break;
				}
				pSeparator = " || ";
			}
			cxrPostReg.Append("\n\t\t\t|| (%s_RND_RETRY && !!(g_rndRetry() & 1));\n", mod.m_modName.Upper().c_str());

			if (cxrCall.m_dest == "auto")
				break;
		}
		cxrPostReg.Append("#\tendif\n");

		string paramStr = bDestAuto ? "" : VA("ht_uint%d%s destId", replCntW == 0 ? 1 : replCntW, replCntW == 0 ? " ht_noload" : "");
		string indexStr = (bDestAuto || replCntW == 0) ? "" : "[destId]";

		g_appArgs.GetDsnRpt().AddLevel("bool SendCallBusy_%s(%s)\n",
			cxrCall.m_funcName.c_str(), paramStr.c_str());
		g_appArgs.GetDsnRpt().EndLevel();

		m_cxrFuncDecl.Append("\tbool SendCallBusy_%s(%s);\n",
			cxrCall.m_funcName.c_str(), paramStr.c_str());
		m_cxrMacros.Append("bool CPers%s%s%s::SendCallBusy_%s(%s)\n",
			unitNameUc.c_str(), mod.m_modName.Uc().c_str(), instIdStr.c_str(),
			cxrCall.m_funcName.c_str(), paramStr.c_str());
		m_cxrMacros.Append("{\n");

		m_cxrMacros.Append("\tassert_msg(r_t%d_htValid, \"Runtime check failed in CPers%s::SendCallBusy_%s()"
			" - thread is not valid\");\n", mod.m_execStg, mod.m_modName.Uc().c_str(), cxrCall.m_funcName.c_str());
		m_cxrMacros.Append("\n");

		m_cxrMacros.Append("\tc_SendCallAvail_%s%s = !c_SendCallBusy_%s%s;\n",
			cxrCall.m_funcName.c_str(), indexStr.c_str(),
			cxrCall.m_funcName.c_str(), indexStr.c_str());
		m_cxrMacros.Append("\treturn c_SendCallBusy_%s%s;\n",
			cxrCall.m_funcName.c_str(), indexStr.c_str());

		m_cxrMacros.Append("}\n");
		m_cxrMacros.Append("\n");

		// Generate Async Call Function

		paramStr = bDestAuto ? "" : VA(", ht_uint%d%s destId", replCntW == 0 ? 1 : replCntW, replCntW == 0 ? " ht_noload" : "");

		g_appArgs.GetDsnRpt().AddLevel("void SendCallFork_%s(ht_uint%d rtnInstr%s",
			cxrCall.m_funcName.c_str(), mod.m_instrW, paramStr.c_str());
		m_cxrFuncDecl.Append("\tvoid SendCallFork_%s(ht_uint%d rtnInstr%s",
			cxrCall.m_funcName.c_str(), mod.m_instrW, paramStr.c_str());
		m_cxrMacros.Append("void CPers%s%s%s::SendCallFork_%s(ht_uint%d rtnInstr%s",
			unitNameUc.c_str(), mod.m_modName.Uc().c_str(), instIdStr.c_str(),
			cxrCall.m_funcName.c_str(), mod.m_instrW, paramStr.c_str());

		for (size_t fldIdx = 0; fldIdx < pCxrEntry->m_paramList.size(); fldIdx += 1) {
			CField * pField = pCxrEntry->m_paramList[fldIdx];

			g_appArgs.GetDsnRpt().AddText(", %s %s", pField->m_pType->m_typeName.c_str(), pField->m_name.c_str());
			m_cxrFuncDecl.Append(", %s %s", pField->m_pType->m_typeName.c_str(), pField->m_name.c_str());
			m_cxrMacros.Append(", %s %s", pField->m_pType->m_typeName.c_str(), pField->m_name.c_str());
		}
		g_appArgs.GetDsnRpt().AddText(")\n");
		g_appArgs.GetDsnRpt().EndLevel();

		m_cxrFuncDecl.Append(");\n");
		m_cxrMacros.Append(")\n{\n");

		m_cxrMacros.Append("\tassert_msg(r_t%d_htValid, \"Runtime check failed in CPers%s::SendCallFork_%s()"
			" - thread is not valid\");\n", mod.m_execStg, mod.m_modName.Uc().c_str(), cxrCall.m_funcName.c_str());
		m_cxrMacros.Append("\tassert_msg(c_SendCallAvail_%s%s, \"Runtime check failed in CPers%s::SendCallFork_%s()"
			" - expected SendCallBusy_%s() to be called and not busy\");\n",
			funcName.c_str(), indexStr.c_str(), mod.m_modName.Uc().c_str(), funcName.c_str(), funcName.c_str());
		m_cxrMacros.Append("\n");

		bool bReplIntf = false;
		size_t callIntfIdx = 0;
		for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
			CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

			if (cxrIntf.m_cxrDir == CxrIn) continue;
			if (!cxrIntf.IsCallOrXfer()) continue;
			if (cxrIntf.m_callIdx != (int)callIdx) continue;

			callIntfIdx = intfIdx;

			if (cxrIntf.m_instCnt == 1)
				m_cxrMacros.Append("\tc_%s_%sRdy = true;\n",
				cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName());
			else {
				bReplIntf = true;

				if (cxrCall.m_dest == "auto")
					m_cxrMacros.Append("\tc_%s_%sRdy%s = r_%sCall_roundRobin == %dUL;\n",
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
					cxrCall.m_funcName.c_str(), cxrIntf.GetPortReplId());
				else
					m_cxrMacros.Append("\tc_%s_%sRdy%s = destId == %dUL;\n",
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
					cxrIntf.GetPortReplId());
			}

			if (cxrCall.m_bXfer) {
				if (cxrCall.m_pGroup->m_rtnSelW > 0)
					m_cxrMacros.Append("\tc_%s_%s%s.m_rtnSel = r_t%d_htPriv.m_rtnSel;\n",
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
					mod.m_execStg);

				if (cxrCall.m_pGroup->m_rtnHtIdW > 0)
					m_cxrMacros.Append("\tc_%s_%s%s.m_rtnHtId = r_t%d_htPriv.m_rtnHtId;\n",
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
					mod.m_execStg);

				if (cxrCall.m_pGroup->m_rtnInstrW > 0)
					m_cxrMacros.Append("\tc_%s_%s%s.m_rtnInstr = r_t%d_htPriv.m_rtnInstr;\n",
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
					mod.m_execStg);
			} else {
				if (cxrCall.m_pGroup->m_htIdW.AsInt() > 0)
					m_cxrMacros.Append("\tc_%s_%s%s.m_rtnHtId = r_t%d_htId;\n",
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
					mod.m_execStg);

				if (mod.m_instrW > 0)
					m_cxrMacros.Append("\tc_%s_%s%s.m_rtnInstr = rtnInstr;\n",
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());

				if (cxrCall.m_bCallFork || cxrCall.m_bXferFork) {
					m_cxrMacros.Append("\tc_%s_%s%s.m_rtnJoin = true;\n",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
				}
			}

			for (size_t fldIdx = 0; fldIdx < pCxrEntry->m_paramList.size(); fldIdx += 1) {
				CField * pField = pCxrEntry->m_paramList[fldIdx];

				m_cxrMacros.Append("\tc_%s_%s%s.m_%s = %s;\n",
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
					pField->m_name.c_str(), pField->m_name.c_str());
			}
		}
		m_cxrMacros.Append("\n");

		if (bReplIntf && cxrCall.m_dest == "auto") {
			m_cxrMacros.Append("\tc_%sCall_roundRobin = (r_%sCall_roundRobin + 1U) %% %dU;\n",
				cxrCall.m_funcName.c_str(), cxrCall.m_funcName.c_str(), replCnt);
			m_cxrMacros.Append("\n");
		}

		indexStr = (bDestAuto || !bReplIntf) ? "" : "[destId]";
		if (htIdW == 0) {
			m_cxrMacros.Append("\t// Should not happen - verify generated code properly limits outstanding calls\n");
			m_cxrMacros.Append("\tassert(c_asyncCall%s_rtnCnt%s < 0x%x);\n",
				cxrCall.m_funcName.c_str(), indexStr.c_str(),
				(1 << forkCntW) - 1);
			m_cxrMacros.Append("\tc_asyncCall%s_rtnCntFull%s = c_asyncCall%s_rtnCnt%s == 0x%x;\n",
				cxrCall.m_funcName.c_str(), indexStr.c_str(),
				cxrCall.m_funcName.c_str(), indexStr.c_str(),
				(1 << forkCntW) - 2);
			m_cxrMacros.Append("\tc_asyncCall%s_rtnCnt%s += 1u;\n",
				cxrCall.m_funcName.c_str(), indexStr.c_str());
		} else {
			m_cxrMacros.Append("\tif (r_t%d_htId == r_t%d_htId && r_t%d_htValid)\n",
				mod.m_execStg - 1, mod.m_execStg, mod.m_execStg - 1);
			m_cxrMacros.Append("\t\tc_t%d_asyncCall%s_rtnCntFull%s = m_asyncCall%s_rtnCnt%s.read_mem() == 0x%x;\n",
				mod.m_execStg - 1, cxrCall.m_funcName.c_str(), indexStr.c_str(),
				cxrCall.m_funcName.c_str(), indexStr.c_str(),
				(1 << forkCntW) - 2);
			m_cxrMacros.Append("\n");

			m_cxrMacros.Append("\t// Should not happen - verify generated code properly limits outstanding calls\n");
			m_cxrMacros.Append("\tassert(m_asyncCall%s_rtnCnt%s.read_mem() < 0x%x);\n",
				cxrCall.m_funcName.c_str(), indexStr.c_str(),
				(1 << forkCntW) - 1);
			m_cxrMacros.Append("\tm_asyncCall%s_rtnCnt%s.write_mem(m_asyncCall%s_rtnCnt%s.read_mem() + 1u);\n",
				cxrCall.m_funcName.c_str(), indexStr.c_str(),
				cxrCall.m_funcName.c_str(), indexStr.c_str());
			m_cxrMacros.Append("\tm_asyncCall%s_rtnCntFull%s.write_mem(m_asyncCall%s_rtnCnt%s.read_mem() == 0x%x);\n",
				cxrCall.m_funcName.c_str(), indexStr.c_str(),
				cxrCall.m_funcName.c_str(), indexStr.c_str(),
				(1 << forkCntW) - 2);
		}

		if (g_appArgs.IsInstrTraceEnabled()) {
			m_cxrMacros.Append("\n");
			m_cxrMacros.Append("#\tifndef _HTV\n");
			m_cxrMacros.Append("\tif (Ht::g_instrTraceFp)\n");
			m_cxrMacros.Append("\t\tfprintf(Ht::g_instrTraceFp, \"Send%s_%s(",
				cxrCall.m_bXfer ? "Transfer" : "Call", cxrCall.m_funcName.c_str());

			char const * pSeparater = "";
			for (size_t fldIdx = 0; fldIdx < pCxrEntry->m_paramList.size(); fldIdx += 1) {
				CField * pField = pCxrEntry->m_paramList[fldIdx];

				bool bBasicType;
				CTypeDef * pTypeDef;
				string type = pField->m_type;
				while (!(bBasicType = IsBaseType(type))) {
					if (!(pTypeDef = FindTypeDef(pField->m_type)))
						break;
					type = pTypeDef->m_type;
				}

				if (bBasicType)
					m_cxrMacros.Append("%s%s = 0x%%llx",
					pSeparater, pField->m_name.c_str());
				else
					m_cxrMacros.Append("%s%s = ???",
					pSeparater, pField->m_name.c_str());

				pSeparater = ", ";
			}
			m_cxrMacros.Append(") @ %%lld\\n\"");

			pSeparater = ",\n\t\t\t";
			for (size_t fldIdx = 0; fldIdx < pCxrEntry->m_paramList.size(); fldIdx += 1) {
				CField * pField = pCxrEntry->m_paramList[fldIdx];

				bool bBasicType;
				CTypeDef * pTypeDef;
				string type = pField->m_type;
				while (!(bBasicType = IsBaseType(type))) {
					if (!(pTypeDef = FindTypeDef(pField->m_type)))
						break;
					type = pTypeDef->m_type;
				}

				if (!bBasicType) continue;

				m_cxrMacros.Append("%s(long long)%s",
					pSeparater, pField->m_name.c_str());

				pSeparater = ", ";
			}
			m_cxrMacros.Append(", (long long)sc_time_stamp().value() / 10000);\n");
			m_cxrMacros.Append("#\tendif\n");
		}

		m_cxrMacros.Append("}\n");
		m_cxrMacros.Append("\n");

		// Generate AsyncCallPause function

		paramStr = bDestAuto ? "" : VA(", ht_uint%d%s destId", replCntW == 0 ? 1 : replCntW, replCntW == 0 ? " ht_noload" : "");
		indexStr = (bDestAuto || replCntW == 0) ? "" : "[destId]";

		g_appArgs.GetDsnRpt().AddLevel("void RecvReturnPause_%s(ht_uint%d rsmInstr%s)\n",
			cxrCall.m_funcName.c_str(), mod.m_instrW, paramStr.c_str());
		g_appArgs.GetDsnRpt().EndLevel();

		m_cxrFuncDecl.Append("\tvoid RecvReturnPause_%s(ht_uint%d rsmInstr%s);\n",
			cxrCall.m_funcName.c_str(), mod.m_instrW, paramStr.c_str());
		m_cxrMacros.Append("void CPers%s%s%s::RecvReturnPause_%s(ht_uint%d rsmInstr%s)\n",
			unitNameUc.c_str(), mod.m_modName.Uc().c_str(), instIdStr.c_str(),
			cxrCall.m_funcName.c_str(), mod.m_instrW, paramStr.c_str());
		m_cxrMacros.Append("{\n");

		m_cxrMacros.Append("\tassert_msg(r_t%d_htValid, \"Runtime check failed in CPers%s::RecvReturnPause_%s()"
			" - thread is not valid\");\n", mod.m_execStg, mod.m_modName.Uc().c_str(), cxrCall.m_funcName.c_str());
		m_cxrMacros.Append("\n");

		if (htIdW == 0) {
			m_cxrMacros.Append("\tassert_msg(c_t%d_htCtrl == HT_INVALID, \"Runtime check failed in CPers%s%s::RecvReturnPause_%s()"
				" - an Ht control routine was already called\");\n", mod.m_execStg, mod.m_modName.Uc().c_str(), instIdStr.c_str(), cxrCall.m_funcName.c_str());
			m_cxrMacros.Append("\tassert (!c_asyncCall%s_waiting%s);\n",
				cxrCall.m_funcName.c_str(), indexStr.c_str());
			m_cxrMacros.Append("\tif (c_asyncCall%s_rtnCnt%s == 0)\n",
				cxrCall.m_funcName.c_str(), indexStr.c_str());
			m_cxrMacros.Append("\t\tHtContinue(rsmInstr);\n");
			m_cxrMacros.Append("\telse {\n");
			m_cxrMacros.Append("\t\tc_asyncCall%s_waiting%s = true;\n",
				cxrCall.m_funcName.c_str(), indexStr.c_str());
			m_cxrMacros.Append("\t\tc_asyncCall%s_rsmInstr%s = rsmInstr;\n",
				cxrCall.m_funcName.c_str(), indexStr.c_str());
			m_cxrMacros.Append("\t\tc_t%d_htCtrl = HT_PAUSE;\n", mod.m_execStg);
			m_cxrMacros.Append("\t}\n");
		} else {
			CCxrIntf &cxrIntf = modInst.m_cxrIntfList[callIntfIdx];

			m_cxrMacros.Append("\tassert_msg(c_t%d_htCtrl == HT_INVALID, \"Runtime check failed in CPers%s%s::RecvReturnPause_%s()"
				" - an Ht control routine was already called\");\n", mod.m_execStg, mod.m_modName.Uc().c_str(), instIdStr.c_str(), cxrCall.m_funcName.c_str());

			m_cxrMacros.Append("\tassert (");

			if (cxrCall.m_dest == "auto") {
				m_cxrMacros.Append("c_%s_%sRdy%s || ",
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), indexStr.c_str());
			} else {
				for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
					CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

					if (cxrIntf.m_cxrDir == CxrIn) continue;
					if (!cxrIntf.IsCallOrXfer()) continue;
					if (cxrIntf.m_callIdx != (int)callIdx) continue;

					m_cxrMacros.Append("c_%s_%sRdy%s || ",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
				}
			}
			m_cxrMacros.Append("!m_asyncCall%s_waiting%s.read_mem());\n",
				cxrCall.m_funcName.c_str(), indexStr.c_str());

			m_cxrMacros.Append("\tif (");

			if (cxrCall.m_dest == "auto") {
				for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
					CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

					if (cxrIntf.m_cxrDir == CxrIn) continue;
					if (!cxrIntf.IsCallOrXfer()) continue;
					if (cxrIntf.m_callIdx != (int)callIdx) continue;

					m_cxrMacros.Append("!c_%s_%sRdy%s && ",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
				}
			} else {
				m_cxrMacros.Append("!c_%s_%sRdy%s && ",
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), indexStr.c_str());
			}

			m_cxrMacros.Append("m_asyncCall%s_rtnCnt%s.read_mem() == 0) {\n",
				cxrCall.m_funcName.c_str(), indexStr.c_str());

			m_cxrMacros.Append("\t\tHtContinue(rsmInstr);\n");
			m_cxrMacros.Append("\t} else {\n");
			m_cxrMacros.Append("\t\tm_asyncCall%s_waiting%s.write_mem(true);\n",
				cxrCall.m_funcName.c_str(), indexStr.c_str());
			m_cxrMacros.Append("\t\tm_asyncCall%s_rsmInstr%s.write_mem(rsmInstr);\n",
				cxrCall.m_funcName.c_str(), indexStr.c_str());
			m_cxrMacros.Append("\t\tc_t%d_htCtrl = HT_PAUSE;\n", mod.m_execStg);
			m_cxrMacros.Append("\t}\n");
		}

		m_cxrMacros.Append("}\n");
		m_cxrMacros.Append("\n");

		// Generate AsyncCallJoin function

		paramStr = bDestAuto ? "" : VA("ht_uint%d%s destId", replCntW == 0 ? 1 : replCntW, replCntW == 0 ? " ht_noload" : "");
		indexStr = (bDestAuto || replCntW == 0) ? "" : "[destId]";

		g_appArgs.GetDsnRpt().AddLevel("void RecvReturnJoin_%s(%s)\n",
			cxrCall.m_funcName.c_str(), paramStr.c_str());
		g_appArgs.GetDsnRpt().EndLevel();

		m_cxrFuncDecl.Append("\tvoid RecvReturnJoin_%s(%s);\n",
			cxrCall.m_funcName.c_str(), paramStr.c_str());
		m_cxrMacros.Append("void CPers%s%s%s::RecvReturnJoin_%s(%s)\n",
			unitNameUc.c_str(), mod.m_modName.Uc().c_str(), instIdStr.c_str(),
			cxrCall.m_funcName.c_str(), paramStr.c_str());
		m_cxrMacros.Append("{\n");

		m_cxrMacros.Append("\tassert_msg(r_t%d_htValid, \"Runtime check failed in CPers%s::RecvReturnJoin_%s()"
			" - thread is not valid\");\n", mod.m_execStg, mod.m_modName.Uc().c_str(), cxrCall.m_funcName.c_str());
		m_cxrMacros.Append("\n");

		if (htIdW == 0) {
			m_cxrMacros.Append("\t// verify that an async call is outstanding\n");
			m_cxrMacros.Append("\tassert (r_asyncCall%s_rtnCnt%s > 0);\n",
				cxrCall.m_funcName.c_str(), indexStr.c_str());
			m_cxrMacros.Append("\tc_asyncCall%s_rtnCntFull%s = false;\n",
				cxrCall.m_funcName.c_str(), indexStr.c_str());
			m_cxrMacros.Append("\tc_asyncCall%s_rtnCnt%s -= 1u;\n",
				cxrCall.m_funcName.c_str(), indexStr.c_str());
			m_cxrMacros.Append("\n");
			m_cxrMacros.Append("\tassert_msg(c_t%d_htCtrl == HT_INVALID, \"Runtime check failed in CPers%s%s::RecvReturnJoin_%s()"
				" - an Ht control routine was already called\");\n", mod.m_execStg, mod.m_modName.Uc().c_str(), instIdStr.c_str(), cxrCall.m_funcName.c_str());
			m_cxrMacros.Append("\tif (c_asyncCall%s_rtnCnt%s == 0 && r_asyncCall%s_waiting%s) {\n",
				cxrCall.m_funcName.c_str(), indexStr.c_str(),
				cxrCall.m_funcName.c_str(), indexStr.c_str());
			m_cxrMacros.Append("\t\tc_asyncCall%s_waiting%s = false;\n",
				cxrCall.m_funcName.c_str(), indexStr.c_str());
			m_cxrMacros.Append("\t\tc_t%d_htNextInstr = r_asyncCall%s_rsmInstr%s;\n",
				mod.m_execStg, cxrCall.m_funcName.c_str(), indexStr.c_str());
			m_cxrMacros.Append("\t\tc_t%d_htCtrl = HT_JOIN_AND_CONT;\n", mod.m_execStg);
			m_cxrMacros.Append("\t} else\n");
			m_cxrMacros.Append("\t\tc_t%d_htCtrl = HT_JOIN;\n", mod.m_execStg);
		} else {
			m_cxrMacros.Append("\t// verify that an async call is outstanding\n");
			m_cxrMacros.Append("\tassert (m_asyncCall%s_rtnCnt%s.read_mem() > 0);\n",
				cxrCall.m_funcName.c_str(), indexStr.c_str());
			m_cxrMacros.Append("\tm_asyncCall%s_rtnCnt%s.write_mem(m_asyncCall%s_rtnCnt%s.read_mem() - 1u);\n",
				cxrCall.m_funcName.c_str(), indexStr.c_str(),
				cxrCall.m_funcName.c_str(), indexStr.c_str());
			m_cxrMacros.Append("\tm_asyncCall%s_rtnCntFull%s.write_mem(false);\n",
				cxrCall.m_funcName.c_str(), indexStr.c_str());
			m_cxrMacros.Append("\n");
			m_cxrMacros.Append("\tif (r_t%d_htId == r_t%d_htId && r_t%d_htValid)\n",
				mod.m_execStg - 1, mod.m_execStg, mod.m_execStg - 1);
			m_cxrMacros.Append("\t\tc_t%d_asyncCall%s_rtnCntFull%s = false;\n",
				mod.m_execStg - 1, cxrCall.m_funcName.c_str(), indexStr.c_str());
			m_cxrMacros.Append("\n");

			m_cxrMacros.Append("\tassert_msg(c_t%d_htCtrl == HT_INVALID, \"Runtime check failed in CPers%s%s::RecvReturnJoin_%s()"
				" - an Ht control routine was already called\");\n", mod.m_execStg, mod.m_modName.Uc().c_str(), instIdStr.c_str(), cxrCall.m_funcName.c_str());
			m_cxrMacros.Append("\tif (m_asyncCall%s_rtnCnt%s.read_mem() == 1 && m_asyncCall%s_waiting%s.read_mem()) {\n",
				cxrCall.m_funcName.c_str(), indexStr.c_str(),
				cxrCall.m_funcName.c_str(), indexStr.c_str());
			m_cxrMacros.Append("\t\tm_asyncCall%s_waiting%s.write_mem(false);\n",
				cxrCall.m_funcName.c_str(), indexStr.c_str());
			m_cxrMacros.Append("\t\tc_t%d_htNextInstr = m_asyncCall%s_rsmInstr%s.read_mem();\n",
				mod.m_execStg, cxrCall.m_funcName.c_str(), indexStr.c_str());
			m_cxrMacros.Append("\t\tc_t%d_htCtrl = HT_JOIN_AND_CONT;\n", mod.m_execStg);
			m_cxrMacros.Append("\t} else\n");
			m_cxrMacros.Append("\t\tc_t%d_htCtrl = HT_JOIN;\n", mod.m_execStg);
		}

		m_cxrMacros.Append("}\n");
		m_cxrMacros.Append("\n");
	}

	g_appArgs.GetDsnRpt().EndLevel();

	///////////////////////////////////////////////////////////////////////
	// Handle inbound call/transfer/return interfaces

	for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
		CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

		if (cxrIntf.m_cxrDir == CxrOut) continue;

		HtdFile::EClkRate srcClkRate = cxrIntf.m_pSrcMod->m_clkRate;
		CHtCode & srcCxrT0Stage = srcClkRate == eClk1x ? m_cxrT0Stage1x : m_cxrT0Stage2x;

		if (cxrIntf.GetQueDepthW() == 0) {

			m_cxrRegDecl.Append("\tbool r_t1_%s_%sRdy%s;\n",
				cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());

			if (mod.m_clkRate != srcClkRate && srcClkRate == eClk2x && !cxrIntf.IsCallOrXfer()) {

				m_cxrRegDecl.Append("\tsc_signal<bool> r_%s_%sRdy_2x%s;\n",
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());

				m_cxrRegDecl.Append("\tsc_signal<C%s_%sIntf> r_%s_%s_2x%s;\n",
					cxrIntf.GetSrcToDstUc(), cxrIntf.GetIntfName(),
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());

				if (cxrIntf.GetPortReplCnt() <= 1)
					cxrT0Stage.Append("\tbool c_t0_%s_%sRdy = i_%s_%sRdy.read()\n\t\t|| r_%s_%sRdy_2x.read() || r_t1_%s_%sRdy;\n",
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(),
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(),
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(),
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName());
				else {
					if (cxrIntf.GetPortReplId() == 0)
						cxrT0Stage.Append("\tbool c_t0_%s_%sRdy%s;\n",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());

					cxrT0Stage.Append("\tc_t0_%s_%sRdy%s = i_%s_%sRdy%s.read()\n\t\t|| r_%s_%sRdy_2x%s.read() || r_t1_%s_%sRdy%s;\n",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
				}

				m_cxrT0Stage2x.Append("\tr_%s_%sRdy_2x%s = i_%s_%sRdy%s;\n",
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
			} else {
				if (cxrIntf.GetPortReplCnt() <= 1)
					cxrT0Stage.Append("\tbool c_t0_%s_%sRdy = i_%s_%sRdy.read() || r_t1_%s_%sRdy;\n",
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(),
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(),
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName());
				else {
					if (cxrIntf.GetPortReplId() == 0)
						cxrT0Stage.Append("\tbool c_t0_%s_%sRdy%s;\n",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());

					cxrT0Stage.Append("\tc_t0_%s_%sRdy%s = i_%s_%sRdy%s.read() || r_t1_%s_%sRdy%s;\n",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
				}
			}

			if (cxrIntf.GetPortReplCnt() <= 1)
				cxrT0Stage.Append("\tbool c_%s_%sAvl = false;\n",
				cxrIntf.GetPortNameDstToSrcLc(), cxrIntf.GetIntfName());
			else {
				if (cxrIntf.GetPortReplId() == 0)
					cxrT0Stage.Append("\tbool c_%s_%sAvl%s;\n",
					cxrIntf.GetPortNameDstToSrcLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());
				cxrT0Stage.Append("\tc_%s_%sAvl%s = false;\n",
					cxrIntf.GetPortNameDstToSrcLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
			}

			cxrT0Stage.NewLine();
			srcCxrT0Stage.NewLine();

			if (cxrIntf.m_bCxrIntfFields) {
				if (cxrIntf.GetPortReplCnt() <= 1)
					cxrT0Stage.Append("\tC%s_%s c_t0_%s_%s = r_t1_%s_%s;\n",
					cxrIntf.GetSrcToDstUc(), cxrIntf.GetIntfName(),
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(),
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName());
				else {
					if (cxrIntf.GetPortReplId() == 0)
						cxrT0Stage.Append("\tC%s_%s c_t0_%s_%s%s;\n",
						cxrIntf.GetSrcToDstUc(), cxrIntf.GetIntfName(),
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());

					cxrT0Stage.Append("\tc_t0_%s_%s%s = r_t1_%s_%s%s;\n",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
				}

				m_cxrRegDecl.Append("\tC%s_%s r_t1_%s_%s%s;\n",
					cxrIntf.GetSrcToDstUc(), cxrIntf.GetIntfName(),
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());

				cxrT0Stage.Append("\tif (i_%s_%sRdy%s.read())\n",
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());

				cxrT0Stage.Append("\t\tc_t0_%s_%s%s = i_%s_%s%s;\n",
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());

				if (mod.m_clkRate != srcClkRate && srcClkRate == eClk2x && !cxrIntf.IsCallOrXfer()) {
					cxrT0Stage.Append("\telse if (r_%s_%sRdy_2x%s.read())\n",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());

					cxrT0Stage.Append("\t\tc_t0_%s_%s%s = r_%s_%s_2x%s;\n",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());

					m_cxrT0Stage2x.Append("\tr_%s_%s_2x%s = i_%s_%s%s;\n",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
				}
				m_cxrT0Stage1x.NewLine();
				m_cxrT0Stage2x.NewLine();
			}

		} else {
			if (cxrIntf.m_bCxrIntfFields) {

				if (cxrIntf.GetPortReplCnt() < 2 || cxrIntf.GetPortReplId() == 0)
					m_cxrRegDecl.Append("\tht_dist_que<C%s_%s, %d> m_%s_%sQue%s;\n",
					cxrIntf.GetSrcToDstUc(), cxrIntf.GetIntfName(),
					cxrIntf.GetQueDepthW(),
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());

				if (cxrIntf.IsCallOrXfer()) {
					cxrT0Stage.Append("\tif (i_%s_%sRdy%s.read())\n",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());

					cxrT0Stage.Append("\t\tm_%s_%sQue%s.push(i_%s_%s%s);\n",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
				} else {
					srcCxrT0Stage.Append("\tif (i_%s_%sRdy%s.read())\n",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());

					srcCxrT0Stage.Append("\t\tm_%s_%sQue%s.push(i_%s_%s%s);\n",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
				}

				if (cxrIntf.GetPortReplCnt() <= 1)
					cxrT0Stage.Append("\tbool c_%s_%sAvl = false;\n",
					cxrIntf.GetPortNameDstToSrcLc(), cxrIntf.GetIntfName());
				else {
					if (cxrIntf.GetPortReplId() == 0)
						cxrT0Stage.Append("\tbool c_%s_%sAvl%s;\n",
						cxrIntf.GetPortNameDstToSrcLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());
					cxrT0Stage.Append("\tc_%s_%sAvl%s = false;\n",
						cxrIntf.GetPortNameDstToSrcLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
				}

				if (mod.m_clkRate != eClk1x || cxrIntf.m_pSrcMod->m_clkRate != eClk1x) {
					if (cxrIntf.GetPortReplCnt() <= 1) {
						cxrT0Stage.Append("\tbool c_%s_%sQueEmpty%s =\n",
							cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
						cxrT0Stage.Append("\t\tr_%s_%sQueEmpty%s && m_%s_%sQue%s.empty();\n",
							cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
							cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
					} else {
						if (cxrIntf.GetPortReplId() == 0)
							cxrT0Stage.Append("\tbool c_%s_%sQueEmpty%s;\n",
							cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());
						cxrT0Stage.Append("\tc_%s_%sQueEmpty%s =\n",
							cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
						cxrT0Stage.Append("\t\tr_%s_%sQueEmpty%s && m_%s_%sQue%s.empty();\n",
							cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
							cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
					}
					if (cxrIntf.GetPortReplCnt() <= 1) {
						cxrT0Stage.Append("\tC%s_%s c_%s_%sQueFront%s = r_%s_%sQueEmpty%s ?\n",
							cxrIntf.GetSrcToDstUc(), cxrIntf.GetIntfName(),
							cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
							cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
						cxrT0Stage.Append("\t\tm_%s_%sQue%s.front() : r_%s_%sQueFront%s;\n",
							cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
							cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
					} else {
						if (cxrIntf.GetPortReplId() == 0)
							cxrT0Stage.Append("\tC%s_%s c_%s_%sQueFront%s;\n",
							cxrIntf.GetSrcToDstUc(), cxrIntf.GetIntfName(),
							cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());
						cxrT0Stage.Append("\tc_%s_%sQueFront%s = r_%s_%sQueEmpty%s ?\n",
							cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
							cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
						cxrT0Stage.Append("\t\tm_%s_%sQue%s.front() : r_%s_%sQueFront%s;\n",
							cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
							cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
					}

					cxrT0Stage.Append("\tif (r_%s_%sQueEmpty%s && !m_%s_%sQue%s.empty())\n",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
					cxrT0Stage.Append("\t\tm_%s_%sQue%s.pop();\n",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
					cxrT0Stage.Append("\n");
				}

				if (mod.m_clkRate == srcClkRate || cxrIntf.IsCallOrXfer())
					cxrReg.Append("\tm_%s_%sQue%s.clock(%s);\n",
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(), reset.c_str());

				else {
					if (srcClkRate == eClk1x) {
						cxrReg.Append("\tm_%s_%sQue%s.pop_clock(%s);\n",
							cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(), reset.c_str());
						m_cxrReg1x.Append("\tm_%s_%sQue%s.push_clock(%s);\n",
							cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(), reset.c_str());
					} else {
						cxrReg.Append("\tm_%s_%sQue%s.pop_clock(%s);\n",
							cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(), reset.c_str());
						m_cxrReg2x.Append("\tm_%s_%sQue%s.push_clock(%s);\n",
							cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(), reset.c_str());
					}
				}

			} else {
				m_cxrRegDecl.Append("\tsc_uint<%d+1> r_%s_%sCnt%s;\n",
					cxrIntf.GetQueDepthW(),
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());

				if (cxrIntf.GetPortReplCnt() <= 1)
					srcCxrT0Stage.Append("\tsc_uint<%d+1> c_%s_%sCnt = r_%s_%sCnt;\n",
					cxrIntf.GetQueDepthW(),
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(),
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName());
				else {
					if (cxrIntf.GetPortReplId() == 0)
						srcCxrT0Stage.Append("\tsc_uint<%d+1> c_%s_%sCnt%s;\n",
						cxrIntf.GetQueDepthW(),
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());

					srcCxrT0Stage.Append("\tc_%s_%sCnt%s = r_%s_%sCnt%s;\n",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
				}

				if (cxrIntf.IsCallOrXfer()) {
					cxrT0Stage.Append("\tif (i_%s_%sRdy%s.read())\n",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());

					cxrT0Stage.Append("\t\tc_%s_%sCnt%s += 1u;\n",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
				} else {
					srcCxrT0Stage.Append("\tif (i_%s_%sRdy%s.read())\n",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());

					srcCxrT0Stage.Append("\t\tc_%s_%sCnt%s += 1u;\n",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
				}
				if (cxrIntf.GetPortReplCnt() <= 1)
					cxrT0Stage.Append("\tbool c_%s_%sAvl = false;\n",
					cxrIntf.GetPortNameDstToSrcLc(), cxrIntf.GetIntfName());
				else {
					if (cxrIntf.GetPortReplId() == 0)
						cxrT0Stage.Append("\tbool c_%s_%sAvl%s;\n",
						cxrIntf.GetPortNameDstToSrcLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());
					cxrT0Stage.Append("\tc_%s_%sAvl%s = false;\n",
						cxrIntf.GetPortNameDstToSrcLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
				}

				m_cxrReg1x.Append("\t\tr_%s_%sCnt%s = %s ? (sc_uint<%d+1>)0 : c_%s_%sCnt%s;\n",
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(), 
					reset.c_str(),
					cxrIntf.GetQueDepthW(),
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
			}
		}
	}

	m_cxrT0Stage1x.NewLine();
	m_cxrT0Stage2x.NewLine();
	cxrT0Stage.Append("\n");
	m_cxrReg1x.Append("\n");

	///////////////////////////////////////////////////////////////////////
	// Handle TS stage assignments

	// initialize outbound call/xfer interfaces
	for (size_t callIdx = 0; callIdx < mod.m_cxrCallList.size(); callIdx += 1) {
		if (!mod.m_bIsUsed) continue;

		CCxrCall & cxrCall = mod.m_cxrCallList[callIdx];

		int replCnt = 0;
		for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
			CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

			if (cxrIntf.m_cxrDir == CxrIn) continue;
			if (!cxrIntf.IsCallOrXfer()) continue;
			if (cxrIntf.m_callIdx != (int)callIdx) continue;

			replCnt = cxrIntf.m_instCnt;

			if (cxrIntf.GetPortReplCnt() <= 1 || cxrIntf.GetPortReplId() == 0)
				m_cxrRegDecl.Append("\tbool c_%s_%sRdy%s;\n",
				cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());

			cxrTsStage.Append("\tc_%s_%sRdy%s = false;\n",
				cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());

			if (mod.m_bGvIwComp) {
				for (int stg = mod.m_execStg + 2; stg <= mod.m_gvIwCompStg; stg += 1) {
					m_cxrRegDecl.Append("\tbool c_t%d_%s_%sRdy%s;\n",
						stg - 1, cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());

					if (stg == mod.m_execStg + 2) {
						cxrTsStage.Append("\tc_t%d_%s_%sRdy%s = r_%s_%sRdy%s;\n",
							stg - 1, cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
							cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
					} else {
						cxrTsStage.Append("\tc_t%d_%s_%sRdy%s = r_t%d_%s_%sRdy%s;\n",
							stg - 1, cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
							stg - 1, cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
					}
				}
			}

			if (cxrIntf.m_bCxrIntfFields) {
				if (cxrIntf.GetPortReplCnt() <= 1 || cxrIntf.GetPortReplId() == 0)
					m_cxrRegDecl.Append("\tC%s_%sIntf c_%s_%s%s;\n",
					cxrIntf.GetSrcToDstUc(), cxrIntf.GetIntfName(),
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());

				if (mod.m_clkRate == eClk1x)
					cxrTsStage.Append("\tc_%s_%s%s = 0;\n",
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
				else
					cxrTsStage.Append("\tc_%s_%s%s = r_%s_%s%s;\n",
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
			}
		}

		if (replCnt > 1 && cxrCall.m_dest == "auto") {
			int replIdxW = FindLg2(replCnt - 1);
			m_cxrRegDecl.Append("\tht_uint%d c_%sCall_roundRobin;\n",
				replIdxW, cxrCall.m_funcName.c_str());
			m_cxrRegDecl.Append("\tht_uint%d r_%sCall_roundRobin;\n",
				replIdxW, cxrCall.m_funcName.c_str());

			cxrTsStage.Append("\tc_%sCall_roundRobin = r_%sCall_roundRobin;\n",
				cxrCall.m_funcName.c_str(), cxrCall.m_funcName.c_str());

			iplReg.Append("\tr_%sCall_roundRobin = %s ? (ht_uint%d)0 : c_%sCall_roundRobin;\n",
				cxrCall.m_funcName.c_str(), reset.c_str(), replIdxW, cxrCall.m_funcName.c_str());
		}

		cxrTsStage.Append("\n");
	}

	// initialize outbound return interfaces
	for (size_t rtnIdx = 0; rtnIdx < mod.m_cxrReturnList.size(); rtnIdx += 1) {
		if (!mod.m_bIsUsed) continue;

		for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
			CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

			if (cxrIntf.m_cxrDir == CxrIn) continue;
			if (cxrIntf.IsCallOrXfer()) continue;
			if (cxrIntf.m_rtnIdx != (int)rtnIdx) continue;

			if (cxrIntf.GetPortReplCnt() <= 1 || cxrIntf.GetPortReplId() == 0)
				m_cxrRegDecl.Append("\tbool c_%s_%sRdy%s;\n",
				cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());

			cxrTsStage.Append("\tc_%s_%sRdy%s = false;\n",
				cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());

			if (mod.m_bGvIwComp) {
				for (int stg = mod.m_execStg + 2; stg <= mod.m_gvIwCompStg; stg += 1) {
					m_cxrRegDecl.Append("\tbool c_t%d_%s_%sRdy%s;\n",
						stg - 1, cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());

					if (stg == mod.m_execStg + 2) {
						cxrTsStage.Append("\tc_t%d_%s_%sRdy%s = r_%s_%sRdy%s;\n",
							stg - 1, cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
							cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
					} else {
						cxrTsStage.Append("\tc_t%d_%s_%sRdy%s = r_t%d_%s_%sRdy%s;\n",
							stg - 1, cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
							stg - 1, cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
					}
				}
			}

			if (cxrIntf.m_bCxrIntfFields) {
				if (cxrIntf.GetPortReplCnt() <= 1 || cxrIntf.GetPortReplId() == 0)
					m_cxrRegDecl.Append("\tC%s_%sIntf c_%s_%s%s;\n",
					cxrIntf.GetSrcToDstUc(), cxrIntf.GetIntfName(),
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());

				cxrTsStage.Append("\tc_%s_%s%s = 0;\n",
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
			}
		}

		cxrTsStage.Append("\n");
	}

	///////////////////////////////////////////////////////////////////////
	// Handle register declarations

	for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
		CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

		if (cxrIntf.m_cxrDir == CxrIn) {
			if (cxrIntf.GetPortReplCnt() <= 1 || cxrIntf.GetPortReplId() == 0) {
				HtdFile::EClkRate srcClkRate = cxrIntf.m_pSrcMod->m_clkRate;

				if (mod.m_clkRate == srcClkRate || cxrIntf.IsCallOrXfer())
					m_cxrRegDecl.Append("\tbool r_%s_%sAvl%s;\n",
					cxrIntf.GetPortNameDstToSrcLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());
				else if (mod.m_clkRate == eClk1x) {
					m_cxrRegDecl.Append("\tsc_signal<bool> r_%s_%sAvl_1x%s;\n",
						cxrIntf.GetPortNameDstToSrcLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());
					m_cxrRegDecl.Append("\tbool r_%s_%sAvl_2x%s;\n",
						cxrIntf.GetPortNameDstToSrcLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());
				} else {
					m_cxrRegDecl.Append("\tbool r_%s_%sAvl_1x%s;\n",
						cxrIntf.GetPortNameDstToSrcLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());
					m_cxrRegDecl.Append("\tsc_signal<ht_uint%d > r_%s_%sAvlCnt_2x%s;\n",
						cxrIntf.GetQueDepthW() + 1,
						cxrIntf.GetPortNameDstToSrcLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());
				}
			}

			if (cxrIntf.GetQueDepthW() > 0 && cxrIntf.GetPortReplId() == 0) {
				if (cxrIntf.m_bCxrIntfFields) {
					if (mod.m_clkRate != eClk1x || cxrIntf.m_pSrcMod->m_clkRate != eClk1x) {
						m_cxrRegDecl.Append("\tbool r_%s_%sQueEmpty%s;\n",
							cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());
						m_cxrRegDecl.Append("\tC%s_%s r_%s_%sQueFront%s;\n",
							cxrIntf.GetSrcToDstUc(), cxrIntf.GetIntfName(),
							cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());
					}
				}
			}

		} else {
			if (cxrIntf.GetPortReplCnt() <= 1 || cxrIntf.GetPortReplId() == 0) {
				HtdFile::EClkRate dstClkRate = cxrIntf.m_pDstMod->m_clkRate;

				if (mod.m_clkRate == dstClkRate || !cxrIntf.IsCallOrXfer())
					m_cxrRegDecl.Append("\tbool r_%s_%sRdy%s;\n",
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());
				else if (mod.m_clkRate == eClk1x) {
					m_cxrRegDecl.Append("\tsc_signal<bool> r_%s_%sRdy%s;\n",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());
					m_cxrRegDecl.Append("\tbool r_%s_%sRdy_2x%s;\n",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());
				} else {
					m_cxrRegDecl.Append("\tsc_signal<bool> r_%s_%sRdy%s;\n",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());
					if (cxrIntf.IsXfer())
						m_cxrRegDecl.Append("\tbool r_%s_%sHold%s;\n",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());
					m_cxrRegDecl.Append("\tbool r_%s_%sRdy_1x%s;\n",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());
				}

				m_cxrRegDecl.Append("\tht_uint%d r_%s_%sAvlCnt%s;\n",
					cxrIntf.GetQueDepthW() + 1,
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());

				m_cxrRegDecl.Append("\tbool r_%s_%sAvlCntZero%s;\n",
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());

				if (mod.m_clkRate == eClk1x && dstClkRate == eClk2x && cxrIntf.IsCallOrXfer()) {
					m_cxrRegDecl.Append("\tsc_signal<bool> r_%s_%sAvl_2x1%s;\n",
						cxrIntf.GetPortNameDstToSrcLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());
					m_cxrRegDecl.Append("\tsc_signal<bool> r_%s_%sAvl_2x2%s;\n",
						cxrIntf.GetPortNameDstToSrcLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());
				}
			}

			m_cxrAvlCntChk.Append("\t\tassert(r_%s_%sAvlCnt%s == %d);\n",
				cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
				1 << cxrIntf.GetQueDepthW());
		}
		m_cxrRegDecl.Append("\n");
	}

	for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
		CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

		if (cxrIntf.m_cxrDir == CxrIn) continue;
		if (!cxrIntf.m_bCxrIntfFields) continue;

		if (cxrIntf.GetPortReplCnt() <= 1 || cxrIntf.GetPortReplId() == 0) {
			HtdFile::EClkRate dstClkRate = cxrIntf.m_pDstMod->m_clkRate;

			if (mod.m_clkRate == dstClkRate || !cxrIntf.IsCallOrXfer())
				m_cxrRegDecl.Append("\tC%s_%sIntf r_%s_%s%s;\n",
				cxrIntf.GetSrcToDstUc(), cxrIntf.GetIntfName(),
				cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());
			else if (mod.m_clkRate == eClk1x) {
				m_cxrRegDecl.Append("\tsc_signal<C%s_%sIntf> r_%s_%s%s;\n",
					cxrIntf.GetSrcToDstUc(), cxrIntf.GetIntfName(),
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());
				m_cxrRegDecl.Append("\tC%s_%sIntf r_%s_%s_2x%s;\n",
					cxrIntf.GetSrcToDstUc(), cxrIntf.GetIntfName(),
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());
			} else {
				m_cxrRegDecl.Append("\tsc_signal<C%s_%sIntf> r_%s_%s%s;\n",
					cxrIntf.GetSrcToDstUc(), cxrIntf.GetIntfName(),
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());
				m_cxrRegDecl.Append("\tC%s_%sIntf r_%s_%s_1x%s;\n",
					cxrIntf.GetSrcToDstUc(), cxrIntf.GetIntfName(),
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());
			}
		}
	}
	m_cxrRegDecl.Append("\n");

	///////////////////////////////////////////////////////////////////////
	// Handle register assignments

	for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
		CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

		if (cxrIntf.m_cxrDir == CxrIn) {
			HtdFile::EClkRate srcClkRate = cxrIntf.m_pSrcMod->m_clkRate;

			if (mod.m_clkRate == srcClkRate || cxrIntf.IsCallOrXfer())
				iplReg.Append("\tr_%s_%sAvl%s = !%s && c_%s_%sAvl%s;\n",
				cxrIntf.GetPortNameDstToSrcLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
				reset.c_str(),
				cxrIntf.GetPortNameDstToSrcLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
			else if (mod.m_clkRate == eClk1x)
				iplReg.Append("\tr_%s_%sAvl_1x%s = !%s && c_%s_%sAvl%s;\n",
				cxrIntf.GetPortNameDstToSrcLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
				reset.c_str(),
				cxrIntf.GetPortNameDstToSrcLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
			else {
				if (cxrIntf.GetPortReplCnt() <= 1)
					m_cxrPostInstr2x.Append("\tht_uint%d c_%s_%sAvlCnt_2x = r_%s_%sAvlCnt_2x.read() + c_%s_%sAvl\n",
					cxrIntf.GetQueDepthW() + 1,
					cxrIntf.GetPortNameDstToSrcLc(), cxrIntf.GetIntfName(),
					cxrIntf.GetPortNameDstToSrcLc(), cxrIntf.GetIntfName(),
					cxrIntf.GetPortNameDstToSrcLc(), cxrIntf.GetIntfName());
				else {
					if (cxrIntf.GetPortReplId() == 0)
						m_cxrPostInstr2x.Append("\tht_uint%d c_%s_%sAvlCnt_2x%s;\n",
						cxrIntf.GetQueDepthW() + 1,
						cxrIntf.GetPortNameDstToSrcLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());

					m_cxrPostInstr2x.Append("\tht_uint%d c_%s_%sAvlCnt_2x%s = r_%s_%sAvlCnt_2x%s.read() + c_%s_%sAvl%s\n",
						cxrIntf.GetQueDepthW() + 1,
						cxrIntf.GetPortNameDstToSrcLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
						cxrIntf.GetPortNameDstToSrcLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
						cxrIntf.GetPortNameDstToSrcLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
				}

				m_cxrPostInstr2x.Append("\t\t- (r_%s_%sAvlCnt_2x%s.read() > 0 && !r_phase);\n",
					cxrIntf.GetPortNameDstToSrcLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
				iplReg.Append("\tr_%s_%sAvlCnt_2x%s = %s ? (ht_uint%d) 0 : c_%s_%sAvlCnt_2x%s;\n",
					cxrIntf.GetPortNameDstToSrcLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
					reset.c_str(),
					cxrIntf.GetQueDepthW() + 1,
					cxrIntf.GetPortNameDstToSrcLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
			}

			if (cxrIntf.GetQueDepthW() > 0) {
				if (cxrIntf.m_bCxrIntfFields) {
					if (mod.m_clkRate != eClk1x || cxrIntf.m_pSrcMod->m_clkRate != eClk1x) {
						iplReg.Append("\tr_%s_%sQueEmpty%s = %s || c_%s_%sQueEmpty%s;\n",
							cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
							reset.c_str(),
							cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
						iplReg.Append("\tr_%s_%sQueFront%s = c_%s_%sQueFront%s;\n",
							cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
							cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
						iplReg.Append("\n");
					}
				}
			}

		} else {
			HtdFile::EClkRate dstClkRate = cxrIntf.m_pDstMod->m_clkRate;

			if (cxrIntf.m_bCxrIntfFields)
				iplReg.Append("\tr_%s_%s%s = c_%s_%s%s;\n",
				cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
				cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());

			if (cxrIntf.GetPortReplCnt() <= 1)
				iplPostInstr.Append("\tht_uint%d c_%s_%sAvlCnt = r_%s_%sAvlCnt\n",
				cxrIntf.GetQueDepthW() + 1,
				cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(),
				cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName());
			else {
				if (cxrIntf.GetPortReplId() == 0)
					iplPostInstr.Append("\tht_uint%d c_%s_%sAvlCnt%s;\n",
					cxrIntf.GetQueDepthW() + 1,
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());

				iplPostInstr.Append("\tc_%s_%sAvlCnt%s = r_%s_%sAvlCnt%s\n",
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
			}

			if (dstClkRate == mod.m_clkRate || !cxrIntf.IsCallOrXfer())
				iplPostInstr.Append("\t\t+ i_%s_%sAvl%s.read() - c_%s_%sRdy%s;\n",
				cxrIntf.GetPortNameDstToSrcLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
				cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());

			else if (dstClkRate == eClk2x) {
				iplPostInstr.Append("\t\t+ r_%s_%sAvl_2x1%s.read() + r_%s_%sAvl_2x2%s.read() - c_%s_%sRdy%s;\n",
					cxrIntf.GetPortNameDstToSrcLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
					cxrIntf.GetPortNameDstToSrcLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());

				m_iplReg2x.Append("\tr_%s_%sAvl_2x2%s = r_%s_%sAvl_2x1%s;\n",
					cxrIntf.GetPortNameDstToSrcLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
					cxrIntf.GetPortNameDstToSrcLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
				m_iplReg2x.Append("\tr_%s_%sAvl_2x1%s = i_%s_%sAvl%s.read();\n",
					cxrIntf.GetPortNameDstToSrcLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
					cxrIntf.GetPortNameDstToSrcLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
				m_iplReg2x.Append("\n");

			} else
				iplPostInstr.Append("\t\t+ (r_phase & i_%s_%sAvl%s.read()) - c_%s_%sRdy%s;\n",
				cxrIntf.GetPortNameDstToSrcLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
				cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());

			if (cxrIntf.GetPortReplCnt() <= 1)
				iplPostInstr.Append("\tbool c_%s_%sAvlCntZero = c_%s_%sAvlCnt == 0;\n",
				cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(),
				cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName());
			else {
				if (cxrIntf.GetPortReplId() == 0)
					iplPostInstr.Append("\tbool c_%s_%sAvlCntZero%s;\n",
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());

				iplPostInstr.Append("\tc_%s_%sAvlCntZero%s = c_%s_%sAvlCnt%s == 0;\n",
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
			}

			iplReg.Append("\tr_%s_%sRdy%s = !%s && c_%s_%sRdy%s;\n",
				cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
				reset.c_str(),
				cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());

			if (mod.m_bGvIwComp) {
				for (int stg = mod.m_execStg + 2; stg <= mod.m_gvIwCompStg; stg += 1) {
					m_cxrRegDecl.Append("\tbool r_t%d_%s_%sRdy%s;\n",
						stg, cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());

					iplReg.Append("\tr_t%d_%s_%sRdy%s = c_t%d_%s_%sRdy%s;\n",
						stg, cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
						stg - 1, cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
				}
			}

			if (dstClkRate != mod.m_clkRate && mod.m_clkRate == eClk2x && cxrIntf.IsCallOrXfer()) {
				if (cxrIntf.GetPortReplCnt() <= 1)
					iplPostInstr.Append("\tbool c_%s_%sHold = r_%s_%sRdy & r_phase;\n",
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(),
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName());
				else {
					if (cxrIntf.GetPortReplId() == 0)
						iplPostInstr.Append("\tbool c_%s_%sHold%s;\n",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());

					iplPostInstr.Append("\tc_%s_%sHold%s = r_%s_%sRdy%s & r_phase;\n",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
				}

				iplPostInstr.Append("\tc_%s_%sRdy%s |= c_%s_%sHold%s;\n",
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());

				if (cxrIntf.IsXfer())
					iplReg.Append("\tr_%s_%sHold%s = c_%s_%sHold%s;\n",
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
			}

			int avlCnt = 1ul << cxrIntf.GetQueDepthW();

			iplReg.Append("\tr_%s_%sAvlCnt%s = %s ? (ht_uint%d)%d : c_%s_%sAvlCnt%s;\n",
				cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
				reset.c_str(),
				cxrIntf.GetQueDepthW() + 1, avlCnt,
				cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());

			iplReg.Append("\tr_%s_%sAvlCntZero%s = !%s && c_%s_%sAvlCntZero%s;\n",
				cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(), 
				reset.c_str(),
				cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());

			m_cxrPostInstr1x.NewLine();
			m_cxrPostInstr2x.NewLine();
		}
		iplReg.Append("\n");
	}

	///////////////////////////////////////////////////////////////////////
	// Handle output signal assignments

	for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
		CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

		if (cxrIntf.m_cxrDir == CxrIn) {
			HtdFile::EClkRate srcClkRate = cxrIntf.m_pSrcMod->m_clkRate;

			if (mod.m_clkRate == srcClkRate || cxrIntf.IsCallOrXfer())
				cxrOut.Append("\to_%s_%sAvl%s = r_%s_%sAvl%s;\n",
				cxrIntf.GetPortNameDstToSrcLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
				cxrIntf.GetPortNameDstToSrcLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
			else if (mod.m_clkRate == eClk1x) {
				m_cxrReg2x.Append("\tr_%s_%sAvl_2x%s = r_%s_%sAvl_1x%s.read() & r_phase;\n",
					cxrIntf.GetPortNameDstToSrcLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
					cxrIntf.GetPortNameDstToSrcLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
				m_cxrOut2x.Append("\to_%s_%sAvl%s = r_%s_%sAvl_2x%s;\n",
					cxrIntf.GetPortNameDstToSrcLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
					cxrIntf.GetPortNameDstToSrcLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
			} else {
				m_cxrReg1x.Append("\tr_%s_%sAvl_1x%s = r_%s_%sAvlCnt_2x%s.read() > 0;\n",
					cxrIntf.GetPortNameDstToSrcLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
					cxrIntf.GetPortNameDstToSrcLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
				m_cxrOut1x.Append("\to_%s_%sAvl%s = r_%s_%sAvl_1x%s;\n",
					cxrIntf.GetPortNameDstToSrcLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
					cxrIntf.GetPortNameDstToSrcLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
			}
		} else {
			HtdFile::EClkRate dstClkRate = cxrIntf.m_pDstMod->m_clkRate;

			if (mod.m_clkRate == dstClkRate || !cxrIntf.IsCallOrXfer()) {
				cxrOut.Append("\to_%s_%sRdy%s = r_%s_%sRdy%s;\n",
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());

				if (mod.m_bGvIwComp && cxrIntf.m_pDstMod->m_modName.AsStr() != "hif") {
					cxrOut.Append("\to_%s_%sCompRdy%s = r_%s_%sCompRdy%s;\n",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
				}

				if (cxrIntf.m_bCxrIntfFields)
					cxrOut.Append("\to_%s_%s%s = r_%s_%s%s;\n",
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
			} else if (mod.m_clkRate == eClk2x) {
				m_cxrReg1x.Append("\tr_%s_%sRdy_1x%s = r_%s_%sRdy%s;\n",
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());

				m_cxrOut1x.Append("\to_%s_%sRdy%s = r_%s_%sRdy_1x%s;\n",
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());

				if (cxrIntf.m_bCxrIntfFields) {
					m_cxrReg1x.Append("\tr_%s_%s_1x%s = r_%s_%s%s;\n",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());

					m_cxrOut1x.Append("\to_%s_%s%s = r_%s_%s_1x%s;\n",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
				}
			} else {
				m_cxrReg2x.Append("\tr_%s_%sRdy_2x%s = r_%s_%sRdy%s & r_phase;\n",
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());

				m_cxrOut2x.Append("\to_%s_%sRdy%s = r_%s_%sRdy_2x%s;\n",
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());

				if (cxrIntf.m_bCxrIntfFields) {
					m_cxrReg2x.Append("\tr_%s_%s_2x%s = r_%s_%s%s;\n",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());

					m_cxrOut2x.Append("\to_%s_%s%s = r_%s_%s_2x%s;\n",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
				}
			}
		}
	}

	m_cxrReg1x.NewLine();
	m_cxrReg2x.NewLine();
	m_cxrOut1x.NewLine();
	m_cxrOut2x.NewLine();

	iplPostInstr.NewLine();
	iplReg.NewLine();
}

void
CDsnInfo::ZeroStruct(CHtCode &code, string fieldBase, vector<CField *> &fieldList)
{
	for (size_t fldIdx = 0; fldIdx < fieldList.size(); fldIdx += 1) {
		CField * pField = fieldList[fldIdx];

		size_t recordIdx;
		for (recordIdx = 0; recordIdx < m_recordList.size(); recordIdx += 1) {
			if (m_recordList[recordIdx]->m_typeName == pField->m_type)
				break;
		}
		if (recordIdx < m_recordList.size()) {
			size_t prefixLen = fieldBase.size();
			if (pField->m_name.size() > 0)
				fieldBase += ".m_" + pField->m_name;

			ZeroStruct(code, fieldBase, m_recordList[recordIdx]->m_fieldList);

			fieldBase.erase(prefixLen);

		} else
			code.Append("\t%s.m_%s = 0;\n", fieldBase.c_str(), pField->m_name.c_str());
	}
}
