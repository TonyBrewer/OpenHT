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
		CModule * pMod = m_modList[modIdx];

		for (size_t callIdx = 0; callIdx < pMod->m_cxrCallList.size(); callIdx += 1) {
			CCxrCall * pCxrCall = pMod->m_cxrCallList[callIdx];

			pCxrCall->m_queueW.InitValue(pCxrCall->m_lineInfo);
		}

		if (pMod->m_bHasThreads)
			pMod->m_threads.m_htIdW.InitValue(pMod->m_threads.m_lineInfo, false);
	}

	// Perform consistency checks on required modules
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule * pMod = m_modList[modIdx];

		pMod->m_cxrEntryReserveCnt = 0;
		for (size_t entryIdx = 0; entryIdx < pMod->m_cxrEntryList.size(); entryIdx += 1) {
			CCxrEntry * pCxrEntry = pMod->m_cxrEntryList[entryIdx];

			pCxrEntry->m_reserveCnt.InitValue(pCxrEntry->m_lineInfo, false, 0);
			pMod->m_cxrEntryReserveCnt += pCxrEntry->m_reserveCnt.AsInt();

			pCxrEntry->m_pGroup = &pMod->m_threads;

			// check that entry parameters are declared as htPriv also
			for (size_t prmIdx = 0; prmIdx < pCxrEntry->m_paramList.size(); prmIdx += 1) {
				CField * pParam = pCxrEntry->m_paramList[prmIdx];

				bool bFound = false;
				vector<CField *> &fieldList = pCxrEntry->m_pGroup->m_htPriv.m_fieldList;
				for (size_t fldIdx = 0; fldIdx < fieldList.size(); fldIdx += 1) {
					CField * pField = fieldList[fldIdx];

					if (pField->m_name == pParam->m_name) {
						if (pField->m_pType->m_typeName != pParam->m_pType->m_typeName)
							ParseMsg(Error, pParam->m_lineInfo, "parameter and private variable have different types");
						bFound = true;
					}
				}
				if (!bFound)
					ParseMsg(Error, pParam->m_lineInfo, "parameter '%s' was not declared as a private variable",
					pParam->m_name.c_str());
			}
		}

		if (pMod->m_cxrEntryReserveCnt >= (1 << pMod->m_threads.m_htIdW.AsInt()))
			CPreProcess::ParseMsg(Error, pMod->m_threads.m_lineInfo,
			"sum of AddEntry reserve values (%d) must be less than number of module threads (%d)",
			pMod->m_cxrEntryReserveCnt, (1 << pMod->m_threads.m_htIdW.AsInt()));

		for (size_t callIdx = 0; callIdx < pMod->m_cxrCallList.size(); callIdx += 1) {
			CCxrCall * pCxrCall = pMod->m_cxrCallList[callIdx];

			// find associated group for call/transfer
			pCxrCall->m_pGroup = &pMod->m_threads;
		}
	}

	// verify that all AddReturns associated with an AddEntry have the same return parameters
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule * pMod = m_modList[modIdx];

		for (size_t rtnIdx = 0; rtnIdx < pMod->m_cxrReturnList.size(); rtnIdx += 1) {
			CCxrReturn * pCxrRtn = pMod->m_cxrReturnList[rtnIdx];

			// check all other modules for a return with the same entry name
			for (size_t mod2Idx = modIdx + 1; mod2Idx < m_modList.size(); mod2Idx += 1) {
				CModule &mod2 = *m_modList[mod2Idx];

				for (size_t rtn2Idx = 0; rtn2Idx < mod2.m_cxrReturnList.size(); rtn2Idx += 1) {
					CCxrReturn * pCxrRtn2 = mod2.m_cxrReturnList[rtn2Idx];

					if (pCxrRtn->m_modEntry != pCxrRtn2->m_modEntry) continue;

					// found matching returns, check parameter list
					if (pCxrRtn->m_paramList.size() != pCxrRtn2->m_paramList.size()) {
						CPreProcess::ParseMsg(Error, pCxrRtn2->m_lineInfo, "AddReturn commands with matching function names have different returning parameter");
						CPreProcess::ParseMsg(Info, pCxrRtn->m_lineInfo, "previous AddReturn command");
						break;
					}

					for (size_t paramIdx = 0; paramIdx < pCxrRtn->m_paramList.size(); paramIdx += 1) {
						CField * pParam = pCxrRtn->m_paramList[paramIdx];
						CField * pParam2 = pCxrRtn2->m_paramList[paramIdx];

						if (pParam->m_name != pParam2->m_name || pParam->m_pType->m_typeName != pParam2->m_pType->m_typeName) {
							CPreProcess::ParseMsg(Error, pCxrRtn2->m_lineInfo, "AddReturn commands with matching function names have different returning parameter");
							CPreProcess::ParseMsg(Info, pCxrRtn->m_lineInfo, "previous AddReturn command");
							break;
						}
					}
				}
			}
		}
	}

	// Initialize module instance for HIF
	{
		CModule * pHifMod = m_modList[0];
		HtlAssert(pHifMod->m_modName == "hif");

		CModInst * pModInst = new CModInst;
		pModInst->m_pMod = pHifMod;
		pModInst->m_replInstName = pHifMod->m_modName;
		pModInst->m_instName = pHifMod->m_modName;
		pModInst->m_instParams.m_memPortList.push_back(0);
		pModInst->m_cxrSrcCnt = (pHifMod->m_bHasThreads ? 1 : 0) + pHifMod->m_resetInstrCnt;
		pModInst->m_replCnt = 1;
		pModInst->m_replId = 0;

		pHifMod->m_instSet.AddInst(pModInst);
	}

	// Start with HIF and decend call tree checking that an entry point is defined for each call
	//	Mark each call with the target module/entryId
	//	Mark each target module as needed

	{
		// Check that the HIF entry point was specified on the command line
		CModule * pHifMod = m_modList[0];
		pHifMod->m_bIsUsed = true;
		HtlAssert(pHifMod->m_modName.Lc() == "hif");

		if (pHifMod->m_cxrCallList.size() == 0)
			ParseMsg(Fatal, "Unit entry function name must be specified (i.e. use AddEntry(host=true) )");

		string unitName = getUnitName(0);

		vector<CModIdx> callStk;

		callStk.push_back(CModIdx(pHifMod->m_instSet.GetInst(0, 0), pHifMod->m_cxrCallList[0], 0, unitName + ":"));

		pHifMod->m_bActiveCall = true;	// check for recursion

		CheckRequiredEntryNames(callStk);

		HtlAssert(callStk.size() == 1);
	}

	if (CLex::GetParseErrorCnt() > 0)
		ParseMsg(Fatal, "Previous errors prevent generation of files");

	// All required entry points are present and we have marked the required modules

	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule * pMod = m_modList[modIdx];

		if (!pMod->m_bIsUsed) continue;

		// verify that returns have at least one paired call
		//for (size_t rtnIdx = 0; rtnIdx < pMod->m_cxrReturnList.size(); rtnIdx += 1) {
		//	CCxrReturn & cxrReturn = pMod->m_cxrReturnList[rtnIdx];

		//	if (pCxrReturn->m_pairedCallList.size() == 0)
		//		ParseMsg(Error, pCxrReturn->m_lineInfo, "no caller found that uses return");
		//}

		for (size_t entryIdx = 0; entryIdx < pMod->m_cxrEntryList.size(); entryIdx += 1) {
			CCxrEntry * pCxrEntry = pMod->m_cxrEntryList[entryIdx];

			if (!pCxrEntry->m_bIsUsed) continue;

			// verify that entry instruction is in list for module
			size_t instrIdx;
			for (instrIdx = 0; instrIdx < pMod->m_instrList.size(); instrIdx += 1) {

				if (pMod->m_instrList[instrIdx] == pCxrEntry->m_entryInstr)
					break;
			}

			if (instrIdx == pMod->m_instrList.size())
				ParseMsg(Error, pCxrEntry->m_lineInfo, "entry instruction is not defined for module, '%s'",
				pCxrEntry->m_entryInstr.c_str());
		}
	}

	// Initialize HIF CModule's m_modInstList
	//   All other modInstLists were initialized while walking the call chain
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule * pMod = m_modList[modIdx];
		if (!pMod->m_bIsUsed) continue;	// only check required modules

		// set module instance names
		for (int instIdx = 0; instIdx < pMod->m_instSet.GetInstCnt(); instIdx += 1) {
			for (int replIdx = 0; replIdx < pMod->m_instSet.GetReplCnt(instIdx); replIdx += 1) {
				CModInst * pModInst = pMod->m_instSet.GetInst(instIdx, replIdx);

				if (pModInst->m_replCnt > 1) {
					pModInst->m_replInstName = VA("%s%d", pModInst->m_instName.c_str(), pModInst->m_replId);
				} else
					pModInst->m_replInstName = pModInst->m_instName;
			}
		}
	}

	// create list of instances that are linked to modules
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule * pMod = m_modList[modIdx];

		if (!pMod->m_bIsUsed) continue;

		for (int instIdx = 0; instIdx < pMod->m_instSet.GetInstCnt(); instIdx += 1) {
			for (int replIdx = 0; replIdx < pMod->m_instSet.GetReplCnt(instIdx); replIdx += 1) {
				CModInst * pModInst = pMod->m_instSet.GetInst(instIdx, replIdx);

				m_dsnInstList.push_back(pModInst);
			}
		}
	}

	if (CLex::GetParseErrorCnt() > 0)
		ParseMsg(Fatal, "Previous errors prevent generation of files");

	InitCxrIntfInfo();
}

void CDsnInfo::CheckRequiredEntryNames(vector<CModIdx> &callStk)
{
	CModIdx & modCall = callStk.back();
	CCxrCall * pCxrCallee = modCall.GetCxrCall();
	CHtString & modEntry = pCxrCallee->m_modEntry;

	modCall.m_pModInst->m_pMod->m_bCallFork |= pCxrCallee->m_bCallFork;
	modCall.m_pModInst->m_pMod->m_bXferFork |= pCxrCallee->m_bXferFork;

	pCxrCallee->m_pGroup->m_bCallFork |= pCxrCallee->m_bCallFork;
	pCxrCallee->m_pGroup->m_bXferFork |= pCxrCallee->m_bXferFork;

	// search for matching entry in list of modules
	size_t modIdx;
	size_t entryIdx;
	bool bFound = false;
	for (modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule * pMod = m_modList[modIdx];

		for (entryIdx = 0; entryIdx < pMod->m_cxrEntryList.size(); entryIdx += 1) {
			CCxrEntry * pCxrEntry = pMod->m_cxrEntryList[entryIdx];

			if (pCxrEntry->m_modEntry == modEntry) {
				pCxrEntry->m_bCallFork |= pCxrCallee->m_bCallFork;
				pCxrEntry->m_bXferFork |= pCxrCallee->m_bXferFork;

				pCxrEntry->m_pGroup->m_bRtnJoin |= pCxrCallee->m_bCallFork || pCxrCallee->m_bXferFork;

				pCxrCallee->m_pPairedEntry = pCxrEntry;
				bFound = true;
				break;
			}
		}

		if (bFound)
			break;
	}

	if (!bFound) {
		if (pCxrCallee->m_lineInfo.m_lineNum == 0)
			ParseMsg(Error, "Personality entry function name was not found, '%s'", modEntry.c_str());
		else
			ParseMsg(Error, pCxrCallee->m_lineInfo, "function name was not found, '%s'", modEntry.c_str());
	} else if (m_modList[modIdx]->m_bActiveCall) {
		ParseMsg(Error, pCxrCallee->m_lineInfo, "found recursion in call chain");
		for (size_t stkIdx = 1; stkIdx < callStk.size(); stkIdx += 1) {
			CCxrCall * pCxrCaller = callStk[stkIdx - 1].GetCxrCall();// m_pModInst->m_pMod->m_cxrCallList[callStk[stkIdx - 1].m_idx];
			CCxrCall * pCxrCallee = callStk[stkIdx].GetCxrCall();// m_pModInst->m_pMod->m_cxrCallList[callStk[stkIdx].m_idx];

			ParseMsg(Info, pCxrCaller->m_lineInfo, "    Module %s, entry %s, called %s\n",
				callStk[stkIdx].m_pModInst->m_pMod->m_modName.Lc().c_str(),
				pCxrCaller->m_modEntry.c_str(),
				pCxrCallee->m_modEntry.c_str());
		}
	} else {
		CModule * pMod = m_modList[modIdx];

		if (pCxrCallee->m_instName.size() == 0)
			pCxrCallee->m_instName = pMod->m_modName;

		string modPath = callStk.back().m_modPath + "/" + pCxrCallee->m_instName;

		int instIdx = 0;
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

			m_maxReplCnt = max(m_maxReplCnt, modInstParams.m_replCnt);

			if (modInstParams.m_replCnt > 1) {
				// Push replCnt on modInstList
				for (int replIdx = 0; replIdx < modInstParams.m_replCnt; replIdx += 1) {
					// check for replicated instance parameters
					CInstanceParams replModInstParams;
					char buf[16];
					sprintf(buf, "[%d]", (int)replIdx);
					string replModPath = modPath + buf;
					getModInstParams(replModPath, replModInstParams);

					m_maxReplCnt = max(m_maxReplCnt, replModInstParams.m_replCnt);

					replModInstParams.m_replCnt = modInstParams.m_replCnt;

					if (replModInstParams.m_lineInfo.m_fileName.size() == 0)
						replModInstParams.m_lineInfo = modInstParams.m_lineInfo;

					if (modInstParams.m_memPortList.size() > 0 && replModInstParams.m_memPortList.size() > 0)
						CPreProcess::ParseMsg(Error,
						"memPort was specified for modPath '%s' and replicated modPath '%s'",
						modPath.c_str(), replModPath.c_str());

					else if (modInstParams.m_memPortList.size() > 0)
						replModInstParams.m_memPortList = modInstParams.m_memPortList;

					else if (pMod->m_memPortList.size() > 0 && replModInstParams.m_memPortList.size() == 0) {
						replModInstParams.m_memPortList.resize(pMod->m_memPortList.size());
						for (size_t memPortIdx = 0; memPortIdx < pMod->m_memPortList.size(); memPortIdx += 1)
							replModInstParams.m_memPortList[memPortIdx] = memPortIdx;
					}

					if (replModInstParams.m_memPortList.size() != pMod->m_memPortList.size())
						ParseMsg(Error, replModInstParams.m_lineInfo, "module memory port count (%d) does not match instance memory port count (%d)\n",
						(int)pMod->m_memPortList.size(), (int)replModInstParams.m_memPortList.size());

					// check if requested instName is new
					CModInst * pModInst = 0;
					for (instIdx = 0; instIdx < pMod->m_instSet.GetInstCnt(); instIdx += 1) {
						if (pMod->m_instSet.GetInst(instIdx)->m_instName == pCxrCallee->m_instName) {
							if (pMod->m_instSet.GetReplCnt(instIdx) > replIdx) {
								HtlAssert(pMod->m_instSet.GetInst(instIdx, replIdx)->m_replId == replIdx);
								pModInst = pMod->m_instSet.GetInst(instIdx, replIdx);
							}
							break;
						}
					}

					if (pModInst == 0) {
						// new instName specified

						int cxrSrcCnt = (pMod->m_bHasThreads ? 1 : 0) + pMod->m_resetInstrCnt;
						pModInst = new CModInst(pMod, pCxrCallee->m_instName, modPath, replModInstParams,
							cxrSrcCnt, pMod->m_cxrReturnList.size(), pMod->m_cxrEntryList.size(), modInstParams.m_replCnt, replIdx);
						pMod->m_instSet.AddInst(pModInst);

						for (size_t modPrmIdx = 0; modPrmIdx < pMod->m_instParamList.size(); modPrmIdx += 1) {
							CModuleInstParam & modInstParam = pMod->m_instParamList[modPrmIdx];

							size_t callPrmIdx;
							for (callPrmIdx = 0; callPrmIdx < pCxrCallee->m_instParamList.size(); callPrmIdx += 1)
								if (modInstParam.m_name == pCxrCallee->m_instParamList[callPrmIdx].m_name)
									break;

							if (callPrmIdx < pCxrCallee->m_instParamList.size()) {
								CCallInstParam &callInstParam = pCxrCallee->m_instParamList[callPrmIdx];
								pModInst->m_callInstParamList.push_back(callInstParam);
							} else if (modInstParam.m_default.size() > 0)
								pModInst->m_callInstParamList.push_back(CCallInstParam(modInstParam.m_name, modInstParam.m_default));
							else
								ParseMsg(Error, pCxrCallee->m_lineInfo, "instParam must have either a default value or AddCall specified value");
						}
					} else {
						// existing instName

						// add new modPath to modInst
						pModInst->m_modPaths.push_back(modPath);

						// verify callInstParams are the same
						for (size_t modPrmIdx = 0; modPrmIdx < pMod->m_instParamList.size(); modPrmIdx += 1) {
							CModuleInstParam & modInstParam = pMod->m_instParamList[modPrmIdx];

							size_t callPrmIdx;
							for (callPrmIdx = 0; callPrmIdx < pCxrCallee->m_instParamList.size(); callPrmIdx += 1)
								if (modInstParam.m_name == pCxrCallee->m_instParamList[callPrmIdx].m_name)
									break;

							if (callPrmIdx < pCxrCallee->m_instParamList.size()) {
								CCallInstParam &callInstParam = pCxrCallee->m_instParamList[callPrmIdx];
								if (callInstParam.m_value != pModInst->m_callInstParamList[modPrmIdx].m_value) {
									ParseMsg(Error, pCxrCallee->m_lineInfo, "inconsistent instParam value for param %s on instance %s",
										callInstParam.m_name.c_str(), pModInst->m_instName.c_str());
								}
							} else if (modInstParam.m_default.size() > 0) {
								if (modInstParam.m_default != pModInst->m_callInstParamList[modPrmIdx].m_value) {
									ParseMsg(Error, pCxrCallee->m_lineInfo, "inconsistent instParam value for param %s on instance %s",
										modInstParam.m_default.c_str(), pModInst->m_instName.c_str());
								}
							} else
								ParseMsg(Error, pCxrCallee->m_lineInfo, "instParam must have either a default value or AddCall specified value");
						}
					}
				}

			} else {

				if (pMod->m_memPortList.size() > 0 && modInstParams.m_memPortList.size() == 0) {
					modInstParams.m_memPortList.resize(pMod->m_memPortList.size());
					for (size_t memPortIdx = 0; memPortIdx < pMod->m_memPortList.size(); memPortIdx += 1)
						modInstParams.m_memPortList[memPortIdx] = memPortIdx;
				}

				if (modInstParams.m_memPortList.size() != pMod->m_memPortList.size()) {
					ParseMsg(Error, modInstParams.m_lineInfo, "module memory port count (%d) does not match instance memory port count (%d)\n",
						(int)pMod->m_memPortList.size(), (int)modInstParams.m_memPortList.size());
				}

				// check if requested instName is new
				int replIdx;
				bool bFound = false;
				for (instIdx = 0; instIdx < pMod->m_instSet.GetInstCnt(); instIdx += 1) {
					for (replIdx = 0; replIdx < pMod->m_instSet.GetReplCnt(instIdx); replIdx += 1) {
						if (pMod->m_instSet.GetInst(instIdx, replIdx)->m_instName == pCxrCallee->m_instName) {
							bFound = true;
							break;
						}
					}
					if (bFound) break;
				}

				if (!bFound) {
					// new instName specified
					int cxrSrcCnt = (pMod->m_bHasThreads ? 1 : 0) + pMod->m_resetInstrCnt;
					CModInst * pModInst = new CModInst(pMod, pCxrCallee->m_instName, modPath, modInstParams, cxrSrcCnt, pMod->m_cxrReturnList.size(), pMod->m_cxrEntryList.size());
					pMod->m_instSet.AddInst(pModInst);

					for (size_t modPrmIdx = 0; modPrmIdx < pMod->m_instParamList.size(); modPrmIdx += 1) {
						CModuleInstParam & modInstParam = pMod->m_instParamList[modPrmIdx];

						size_t callPrmIdx;
						for (callPrmIdx = 0; callPrmIdx < pCxrCallee->m_instParamList.size(); callPrmIdx += 1)
							if (modInstParam.m_name == pCxrCallee->m_instParamList[callPrmIdx].m_name)
								break;

						if (callPrmIdx < pCxrCallee->m_instParamList.size()) {
							CCallInstParam &callInstParam = pCxrCallee->m_instParamList[callPrmIdx];
							pModInst->m_callInstParamList.push_back(callInstParam);
						} else if (modInstParam.m_default.size() > 0)
							pModInst->m_callInstParamList.push_back(CCallInstParam(modInstParam.m_name, modInstParam.m_default));
						else
							ParseMsg(Error, pCxrCallee->m_lineInfo, "instParam must have either a default value or AddCall specified value");
					}
				} else {
					// add new modPath to modInst
					CModInst * pModInst = pMod->m_instSet.GetInst(instIdx, replIdx);
					pModInst->m_modPaths.push_back(modPath);

					// verify callInstParams are the same
					for (size_t modPrmIdx = 0; modPrmIdx < pMod->m_instParamList.size(); modPrmIdx += 1) {
						CModuleInstParam & modInstParam = pMod->m_instParamList[modPrmIdx];

						size_t callPrmIdx;
						for (callPrmIdx = 0; callPrmIdx < pCxrCallee->m_instParamList.size(); callPrmIdx += 1)
							if (modInstParam.m_name == pCxrCallee->m_instParamList[callPrmIdx].m_name)
								break;

						if (callPrmIdx < pCxrCallee->m_instParamList.size()) {
							CCallInstParam &callInstParam = pCxrCallee->m_instParamList[callPrmIdx];
							if (callInstParam.m_value != pModInst->m_callInstParamList[modPrmIdx].m_value) {
								ParseMsg(Error, pCxrCallee->m_lineInfo, "inconsistent instParam value for param %s on instance %s",
									callInstParam.m_name.c_str(), pModInst->m_instName.c_str());
							}
						} else if (modInstParam.m_default.size() > 0) {
							if (modInstParam.m_default != pModInst->m_callInstParamList[modPrmIdx].m_value) {
								ParseMsg(Error, pCxrCallee->m_lineInfo, "inconsistent instParam value for param %s on instance %s",
									modInstParam.m_default.c_str(), pModInst->m_instName.c_str());
							}
						} else
							ParseMsg(Error, pCxrCallee->m_lineInfo, "instParam must have either a default value or AddCall specified value");
					}
				}
			}
		}

		pMod->m_bRtnJoin |= pCxrCallee->m_bCallFork || pCxrCallee->m_bXferFork;

		CCxrEntry * pCxrEntry = pMod->m_cxrEntryList[entryIdx];

		modCall.GetCxrCall()->m_pairedFunc = CModIdx(pMod->m_instSet.GetInst(instIdx), pCxrEntry, entryIdx, modPath);

		vector<CModIdx> & callerList = pMod->m_instSet.GetInst(instIdx)->m_cxrInstEntryList[entryIdx].m_callerList;
		callerList.push_back(modCall);
		pMod->m_threads.m_rtnSelW = max(pMod->m_threads.m_rtnSelW, FindLg2(callerList.size() - 1, false));

		pCxrEntry->m_bIsUsed = true;

		pMod->m_bIsUsed = true;
		pMod->m_bActiveCall = true;

		// find previous call (skip transfers)
		int callStkIdx;
		CModule * pPrevCxrCallMod = 0;
		CCxrCall * pPrevCxrCall = 0;
		for (callStkIdx = (int)callStk.size() - 1; callStkIdx >= 0; callStkIdx -= 1) {
			pPrevCxrCallMod = callStk[callStkIdx].m_pModInst->m_pMod;
			int prevCallIdx = callStk[callStkIdx].m_callIdx;
			pPrevCxrCall = pPrevCxrCallMod->m_cxrCallList[prevCallIdx];
			if (!pPrevCxrCall->m_bXfer)
				break;
		}
		HtlAssert(callStkIdx >= 0);

		// pair returns with previous call
		bool bFoundRtn = false;
		for (size_t rtnIdx = 0; rtnIdx < m_modList[modIdx]->m_cxrReturnList.size(); rtnIdx += 1) {
			CCxrReturn * pCxrReturn = m_modList[modIdx]->m_cxrReturnList[rtnIdx];
			CCxrInstReturn & cxrInstReturn = m_modList[modIdx]->m_instSet.GetInst(instIdx)->m_cxrInstReturnList[rtnIdx];

			if (pCxrReturn->m_modEntry == pPrevCxrCall->m_modEntry) {
				pPrevCxrCall->m_pairedReturnList.push_back(CModIdx(m_modList[modIdx]->m_instSet.GetInst(0), pCxrReturn, rtnIdx, modPath));
				pCxrReturn->m_pairedCallList.Insert(callStk[callStkIdx]);

				cxrInstReturn.m_callerList.push_back(callStk[callStkIdx]);

				pCxrReturn->m_pGroup = pCxrEntry->m_pGroup;

				pCxrReturn->m_bRtnJoin |= pCxrCallee->m_bCallFork || pCxrCallee->m_bXferFork;

				bFoundRtn = true;

				if (pPrevCxrCallMod->m_bHostIntf) continue;

				// verify that caller has private variables for return parameters
				for (size_t prmIdx = 0; prmIdx < pCxrReturn->m_paramList.size(); prmIdx += 1) {
					CField * pParam = pCxrReturn->m_paramList[prmIdx];

					bool bFound = false;
					vector<CField *> &fieldList = pPrevCxrCall->m_pGroup->m_htPriv.m_fieldList;
					for (size_t fldIdx = 0; fldIdx < fieldList.size(); fldIdx += 1) {
						CField * pField = fieldList[fldIdx];

						if (pField->m_name == pParam->m_name) {
							if (pField->m_pType->m_typeName != pParam->m_pType->m_typeName)
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
		for (size_t callIdx = 0; callIdx < pMod->m_cxrCallList.size(); callIdx += 1) {
			CCxrCall * pCxrCall = pMod->m_cxrCallList[callIdx];

			if (pCxrCall->m_bXfer)
				pCxrCall->m_bXferFork |= pCxrCallee->m_bCallFork || pCxrCallee->m_bXferFork;

			pCxrCall->m_pairedEntry = CModIdx(pMod->m_instSet.GetInst(0), pCxrEntry, entryIdx, modPath);

			callStk.push_back(CModIdx(pMod->m_instSet.GetInst(0), pCxrCall, callIdx, modPath));

			// for xfer, check that a return to previous call exists
			//   It may not exist and be okay if multiple calls to same
			//   module exists.
			if (!pCxrCall->m_bXfer || CheckTransferReturn(callStk)) {

				CheckRequiredEntryNames(callStk);
				bFoundXfer = bFoundXfer || pCxrCall->m_bXfer;
			}

			callStk.pop_back();
		}

		if (!bFoundRtn && !bFoundXfer) {
			// no exit found for module entry
			if (pCxrCallee->m_bXfer)
				ParseMsg(Error, pCxrEntry->m_lineInfo,
				"expected an outbound return or transfer for original call (%s) that was transfered to entry (%s)",
				pPrevCxrCall->m_modEntry.c_str(), pCxrEntry->m_modEntry.c_str());
			else
				ParseMsg(Error, pCxrEntry->m_lineInfo,
				"expected a return for call to entry (%s)",
				pCxrEntry->m_modEntry.c_str());
		}

		pMod->m_bActiveCall = false;
	}
}

bool CDsnInfo::CheckTransferReturn(vector<CModIdx> &callStk)
{
	CModIdx & modCall = callStk.back();
	CCxrCall * pCxrCallee = modCall.GetCxrCall();// m_pModInst->m_pMod->m_cxrCallList[modCall.m_idx];
	CHtString & modEntry = pCxrCallee->m_modEntry;

	// search for matching entry in list of modules
	size_t modIdx;
	size_t entryIdx;
	bool bFound = false;
	for (modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule * pMod = m_modList[modIdx];

		for (entryIdx = 0; entryIdx < pMod->m_cxrEntryList.size(); entryIdx += 1) {
			CCxrEntry * pCxrEntry = pMod->m_cxrEntryList[entryIdx];

			if (pCxrEntry->m_modEntry == modEntry) {
				bFound = true;
				break;
			}
		}

		if (bFound)
			break;
	}

	if (!bFound) {
		if (pCxrCallee->m_lineInfo.m_lineNum == 0)
			ParseMsg(Error, "Personality entry function name was not found, '%s'", modEntry.c_str());
		else
			ParseMsg(Error, pCxrCallee->m_lineInfo, "function name was not found, '%s'", modEntry.c_str());
		return false;
	} else if (m_modList[modIdx]->m_bActiveCall) {
		ParseMsg(Error, pCxrCallee->m_lineInfo, "found recursion in call chain");
		for (size_t stkIdx = 1; stkIdx < callStk.size(); stkIdx += 1) {
			CCxrCall * pCxrCaller = callStk[stkIdx - 1].GetCxrCall();// m_pModInst->m_pMod->m_cxrCallList[callStk[stkIdx - 1].m_idx];
			CCxrCall * pCxrCallee = callStk[stkIdx].GetCxrCall();// m_pModInst->m_pMod->m_cxrCallList[callStk[stkIdx].m_idx];

			ParseMsg(Info, pCxrCaller->m_lineInfo, "    Module %s, entry %s, called %s\n",
				callStk[stkIdx].m_pModInst->m_pMod->m_modName.Lc().c_str(),
				pCxrCaller->m_modEntry.c_str(),
				pCxrCallee->m_modEntry.c_str());
		}
		return false;
	} else {
		CModule * pMod = m_modList[modIdx];

		string modPath = callStk.back().m_modPath + "/" + pMod->m_modName.AsStr();

		pMod->m_bIsUsed = true;

		// find previous call (skip transfers)
		int callStkIdx;
		CModule * pPrevCxrCallMod = 0;
		CCxrCall * pPrevCxrCall = 0;
		for (callStkIdx = (int)callStk.size() - 1; callStkIdx >= 0; callStkIdx -= 1) {
			pPrevCxrCallMod = callStk[callStkIdx].m_pModInst->m_pMod;
			int prevCallIdx = callStk[callStkIdx].m_callIdx;
			pPrevCxrCall = pPrevCxrCallMod->m_cxrCallList[prevCallIdx];
			if (!pPrevCxrCall->m_bXfer)
				break;
		}
		HtlAssert(callStkIdx >= 0);

		// pair returns with previous call
		for (size_t rtnIdx = 0; rtnIdx < m_modList[modIdx]->m_cxrReturnList.size(); rtnIdx += 1) {
			CCxrReturn * pCxrReturn = m_modList[modIdx]->m_cxrReturnList[rtnIdx];

			if (pCxrReturn->m_modEntry == pPrevCxrCall->m_modEntry)
				return true;
		}

		// follow each call
		for (size_t callIdx = 0; callIdx < pMod->m_cxrCallList.size(); callIdx += 1) {
			CCxrCall * pCxrCall = pMod->m_cxrCallList[callIdx];

			if (pCxrCall->m_bXfer) {
				callStk.push_back(CModIdx(pMod->m_instSet.GetInst(0), pCxrCall, callIdx, modPath));
				pMod->m_bActiveCall = true;

				bool bFoundRtn = CheckTransferReturn(callStk);

				pMod->m_bActiveCall = false;
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
		CModule * pMod = m_modList[modIdx];

		if (!pMod->m_bIsUsed) continue;	// only check required modules

		// Initialize pCxrCall->m_pGroup
		for (size_t callIdx = 0; callIdx < pMod->m_cxrCallList.size(); callIdx += 1) {
			CCxrCall * pCxrCall = pMod->m_cxrCallList[callIdx];

			pCxrCall->m_pGroup = &pMod->m_threads;
		}
	}

	// initialize instruction widths
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule * pMod = m_modList[modIdx];

		if (!pMod->m_bIsUsed) continue;

		if (pMod->m_instrList.size() == 0) {
			pMod->m_instrW = 0;
			continue;
		}

		int maxHtInstrValue = (int)pMod->m_instrList.size() - 1;

		pMod->m_instrW = FindLg2(maxHtInstrValue);
	}

	// Initialize htInstType, htIdW and htIdType
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule * pMod = m_modList[modIdx];

		if (!pMod->m_bIsUsed) continue;	// only check required modules

		// Initialize pCxrEntry->m_pGroup

		// generate types for rtnInstr and rtnHtId
		if (pMod->m_instrList.size() > 0) {
			sprintf(typeName, "%sInstr_t", pMod->m_modName.Uc().c_str());
			pMod->m_pInstrType = FindHtIntType(eUnsigned, FindLg2(pMod->m_instrList.size() - 1));
		}

		if (pMod->m_bHasThreads) {
			pMod->m_threads.m_htIdW.InitValue(pMod->m_threads.m_lineInfo);

			if (pMod->m_threads.m_htIdW.AsInt() > 0) {
				sprintf(typeName, "%sHtId_t", pMod->m_modName.Uc().c_str());
				pMod->m_threads.m_pHtIdType = FindHtIntType(eUnsigned, pMod->m_threads.m_htIdW.AsInt());;
			}
		}
	}

	// Initialize values within group
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule * pMod = m_modList[modIdx];

		if (!pMod->m_bIsUsed) continue;	// only check required modules

		for (int instIdx = 0; instIdx < pMod->m_instSet.GetInstCnt(); instIdx += 1) {
			CModInst * pModInst = pMod->m_instSet.GetInst(instIdx);

			// Transfer info from module's entry list
			for (size_t entryIdx = 0; entryIdx < pModInst->m_cxrInstEntryList.size(); entryIdx += 1) {
				CCxrInstEntry & cxrInstEntry = pModInst->m_cxrInstEntryList[entryIdx];
				CCxrEntry * pCxrEntry = pMod->m_cxrEntryList[entryIdx];

				for (size_t callIdx = 0; callIdx < cxrInstEntry.m_callerList.size(); callIdx += 1) {
					CModIdx & pairedCall = cxrInstEntry.m_callerList[callIdx];
					CCxrCall * pCxrCall = pairedCall.GetCxrCall();

					if (pCxrCall->m_bXfer) {
						CCxrEntry * pCxrXferEntry = pCxrCall->m_pairedEntry.GetCxrEntry();

						pCxrEntry->m_pGroup->m_rtnSelW = pCxrXferEntry->m_pGroup->m_rtnSelW;
						pCxrEntry->m_pGroup->m_rtnInstrW = pCxrXferEntry->m_pGroup->m_rtnInstrW;
						pCxrEntry->m_pGroup->m_rtnHtIdW = pCxrXferEntry->m_pGroup->m_rtnHtIdW;
					} else {
						pCxrEntry->m_pGroup->m_rtnInstrW = max(pCxrEntry->m_pGroup->m_rtnInstrW, pairedCall.m_pModInst->m_pMod->m_instrW);
						pCxrEntry->m_pGroup->m_rtnHtIdW = max(pCxrEntry->m_pGroup->m_rtnHtIdW, pCxrCall->m_pGroup->m_htIdW.AsInt());
					}
				}
			}
		}

		if (pMod->m_instrW > 0) {
			sprintf(defineStr, "%s_INSTR_W", pMod->m_modName.Upper().c_str());
			sprintf(typeName, "%sInstr_t", pMod->m_modName.Uc().c_str());

			sprintf(valueStr, "%d", pMod->m_instrW);

			AddDefine(pMod, defineStr, valueStr);
			AddTypeDef(typeName, "uint32_t", defineStr, true);
		}

		// Declare return private variables
		if (pMod->m_bHasThreads) {
			if (pMod->m_threads.m_rtnSelW > 0) {
				sprintf(defineStr, "%s_RTN_SEL_W", pMod->m_modName.Upper().c_str());
				sprintf(typeName, "%sRtnSel_t", pMod->m_modName.Uc().c_str());

				pMod->m_threads.m_pRtnSelType = FindHtIntType(eUnsigned, pMod->m_threads.m_rtnSelW);

				sprintf(valueStr, "%d", pMod->m_threads.m_rtnSelW);

				AddDefine(pMod, defineStr, valueStr);
				AddTypeDef(typeName, "uint32_t", defineStr, true);
				pMod->m_threads.m_htPriv.AddStructField(pMod->m_threads.m_pRtnSelType, "rtnSel");
			}

			if (pMod->m_threads.m_rtnInstrW > 0) {
				sprintf(defineStr, "%s_RTN_INSTR_W", pMod->m_modName.Upper().c_str());
				sprintf(typeName, "%sRtnInstr_t", pMod->m_modName.Uc().c_str());

				pMod->m_threads.m_pRtnInstrType = FindHtIntType(eUnsigned, pMod->m_threads.m_rtnInstrW);

				sprintf(valueStr, "%d", pMod->m_threads.m_rtnInstrW);

				AddDefine(pMod, defineStr, valueStr);
				AddTypeDef(typeName, "uint32_t", defineStr, true);

				pMod->m_threads.m_htPriv.AddStructField(pMod->m_threads.m_pRtnInstrType, "rtnInstr");
			}

			if (pMod->m_threads.m_rtnHtIdW > 0) {
				sprintf(defineStr, "%s_RTN_HTID_W", pMod->m_modName.Upper().c_str());
				sprintf(typeName, "%sRtnHtId_t", pMod->m_modName.Uc().c_str());

				pMod->m_threads.m_pRtnHtIdType = FindHtIntType(eUnsigned, pMod->m_threads.m_rtnHtIdW);

				sprintf(valueStr, "%d", pMod->m_threads.m_rtnHtIdW);

				AddDefine(pMod, defineStr, valueStr);
				AddTypeDef(typeName, "uint32_t", defineStr, true);

				pMod->m_threads.m_htPriv.AddStructField(pMod->m_threads.m_pRtnHtIdType, "rtnHtId");
			}

			if (pMod->m_threads.m_bRtnJoin)
				pMod->m_threads.m_htPriv.AddStructField2(&g_bool, "rtnJoin");
		}
	}

	// Create the cxr interface links between modules
	bool bError = false;
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule * pSrcMod = m_modList[modIdx];
		if (!pSrcMod->m_bIsUsed) continue;	// only check required modules

		for (size_t callIdx = 0; callIdx < pSrcMod->m_cxrCallList.size(); callIdx += 1) {
			CCxrCall * pCxrCall = pSrcMod->m_cxrCallList[callIdx];
			CThreads *pSrcGroup = pCxrCall->m_pGroup;

			ECxrType cxrType = pCxrCall->m_bXfer ? CxrTransfer : CxrCall;

			CModInst * pDstModInst = pCxrCall->m_pairedFunc.m_pModInst;

			CModule * pDstMod = pCxrCall->m_pairedFunc.m_pModInst->m_pMod;
			int dstEntryIdx = pCxrCall->m_pairedFunc.m_entryIdx;
			CCxrEntry * pDstCxrEntry = pDstMod->m_cxrEntryList[dstEntryIdx];
			CCxrInstEntry & dstCxrEntry = pDstModInst->m_cxrInstEntryList[dstEntryIdx];
			CThreads * pDstGroup = pDstCxrEntry->m_pGroup;
			vector<CField *> * pFieldList = &pDstCxrEntry->m_paramList;

			if (pCxrCall->m_instName.size() == 0)
				pCxrCall->m_instName = pDstMod->m_modName;

			// find return select ID
			int rtnSelId = -1;
			for (size_t callListIdx = 0; callListIdx < dstCxrEntry.m_callerList.size(); callListIdx += 1) {
				CModIdx & callModIdx = dstCxrEntry.m_callerList[callListIdx];

				if (callModIdx.m_pModInst->m_pMod == pSrcMod && callModIdx.m_callIdx == callIdx)
					rtnSelId = (int)callListIdx;
			}
			HtlAssert(rtnSelId >= 0);

			// check if interface structure is empty
			bool bCxrIntfFields;
			if (cxrType == CxrCall) {

				bCxrIntfFields = pSrcGroup->m_htIdW.AsInt() > 0 || pSrcMod->m_pInstrType != 0
					|| pFieldList->size() > 0;

			} else { // else Transfer

				bCxrIntfFields = pSrcGroup->m_rtnSelW > 0 || pSrcGroup->m_rtnHtIdW > 0 || pSrcGroup->m_rtnInstrW > 0
					|| pFieldList->size() > 0;

			}

			// Loop through instances for both src and dst
			bool bFoundLinkInfo = false;
			for (int srcInstIdx = 0; srcInstIdx < pSrcMod->m_instSet.GetInstCnt(); srcInstIdx += 1) {
				for (int srcReplIdx = 0; srcReplIdx < pSrcMod->m_instSet.GetReplCnt(srcInstIdx); srcReplIdx += 1) {
					CModInst * pSrcModInst = pSrcMod->m_instSet.GetInst(srcInstIdx, srcReplIdx);

					// Find number of destination instances
					int instCnt = 0;
					for (int dstInstIdx = 0; dstInstIdx < pDstMod->m_instSet.GetInstCnt(); dstInstIdx += 1) {
						for (int dstReplIdx = 0; dstReplIdx < pDstMod->m_instSet.GetReplCnt(dstInstIdx); dstReplIdx += 1) {
							CModInst * pDstModInst = pDstMod->m_instSet.GetInst(dstInstIdx, dstReplIdx);

							// check if instance file had information for this link
							if (pDstModInst->m_instName != pCxrCall->m_instName) continue;
							if (!IsCallDest(pSrcModInst, pDstModInst)) continue;

							instCnt += 1;
						}
					}

					int instIdx = 0;
					for (int dstInstIdx = 0; dstInstIdx < pDstMod->m_instSet.GetInstCnt(); dstInstIdx += 1) {
						for (int dstReplIdx = 0; dstReplIdx < pDstMod->m_instSet.GetReplCnt(dstInstIdx); dstReplIdx += 1) {
							CModInst * pDstModInst = pDstMod->m_instSet.GetInst(dstInstIdx, dstReplIdx);

							//// check if instance file had information for this link
							if (pDstModInst->m_instName != pCxrCall->m_instName) continue;
							if (!IsCallDest(pSrcModInst, pDstModInst)) continue;

							// found link info
							bFoundLinkInfo = true;

							// insert call link
							pSrcModInst->m_cxrIntfList.push_back(
								new CCxrIntf(pCxrCall->m_modEntry, pSrcModInst, pSrcGroup, pDstModInst, pDstGroup,
								cxrType, CxrOut, pCxrCall->m_queueW.AsInt(), pFieldList));

							CCxrIntf * pSrcCxrIntf = pSrcModInst->m_cxrIntfList.back();
							pSrcCxrIntf->m_callIdx = callIdx;
							pSrcCxrIntf->m_instCnt = instCnt;
							pSrcCxrIntf->m_instIdx = instIdx++;
							pSrcCxrIntf->m_rtnSelId = rtnSelId;
							pSrcCxrIntf->m_bCxrIntfFields = bCxrIntfFields;
							pSrcCxrIntf->m_bRtnJoin = pCxrCall->m_bCallFork || pCxrCall->m_bXferFork;

							pDstModInst->m_cxrIntfList.push_back(
								new CCxrIntf(pCxrCall->m_modEntry, pSrcModInst, pSrcGroup, pDstModInst, pDstGroup,
								cxrType, CxrIn, pCxrCall->m_queueW.AsInt(), pFieldList));

							CCxrIntf * pDstCxrIntf = pDstModInst->m_cxrIntfList.back();
							pDstCxrIntf->m_entryInstr = pDstCxrEntry->m_entryInstr;
							pDstCxrIntf->m_reserveCnt = pDstCxrEntry->m_reserveCnt.AsInt();
							pDstCxrIntf->m_rtnSelId = rtnSelId;
							pDstCxrIntf->m_rtnReplId = pSrcModInst->m_replId;
							pDstCxrIntf->m_bCxrIntfFields = bCxrIntfFields;
							pDstCxrIntf->m_bRtnJoin = pCxrCall->m_bCallFork || pCxrCall->m_bXferFork;

							pDstModInst->m_cxrSrcCnt += 1;
						}
					}
				}
			}

			if (!bFoundLinkInfo) {
				// make HIF connection
				HtlAssert(pSrcMod->m_instSet.GetTotalCnt() == 1 && pDstMod->m_instSet.GetTotalCnt() == 1);

				int srcInstIdx = 0;
				int srcReplIdx = 0;
				CModInst * pSrcModInst = pSrcMod->m_instSet.GetInst(srcInstIdx, srcReplIdx);

				int instCnt = 1;
				int instIdx = 0;
				int dstInstIdx = 0;
				int dstReplIdx = 0;
				CModInst * pDstModInst = pDstMod->m_instSet.GetInst(dstInstIdx, dstReplIdx);

				// insert call link
				pSrcModInst->m_cxrIntfList.push_back(
					new CCxrIntf(pCxrCall->m_modEntry, pSrcModInst, pSrcGroup, pDstModInst, pDstGroup,
					cxrType, CxrOut, pCxrCall->m_queueW.AsInt(), pFieldList));

				CCxrIntf * pSrcCxrIntf = pSrcModInst->m_cxrIntfList.back();
				pSrcCxrIntf->m_callIdx = callIdx;
				pSrcCxrIntf->m_instCnt = instCnt;
				pSrcCxrIntf->m_instIdx = instIdx++;
				pSrcCxrIntf->m_rtnSelId = rtnSelId;
				pSrcCxrIntf->m_bCxrIntfFields = bCxrIntfFields;
				pSrcCxrIntf->m_bRtnJoin = pCxrCall->m_bCallFork || pCxrCall->m_bXferFork;

				pDstModInst->m_cxrIntfList.push_back(
					new CCxrIntf(pCxrCall->m_modEntry, pSrcModInst, pSrcGroup, pDstModInst, pDstGroup,
					cxrType, CxrIn, pCxrCall->m_queueW.AsInt(), pFieldList));

				CCxrIntf * pDstCxrIntf = pDstModInst->m_cxrIntfList.back();
				pDstCxrIntf->m_entryInstr = pDstCxrEntry->m_entryInstr;
				pDstCxrIntf->m_rtnSelId = rtnSelId;
				pDstCxrIntf->m_rtnReplId = pSrcModInst->m_replId;
				pDstCxrIntf->m_bCxrIntfFields = bCxrIntfFields;
				pDstCxrIntf->m_bRtnJoin = pCxrCall->m_bCallFork || pCxrCall->m_bXferFork;

				pDstModInst->m_cxrSrcCnt += 1;
			}
		}

		for (size_t rtnIdx = 0; rtnIdx < pSrcMod->m_cxrReturnList.size(); rtnIdx += 1) {
			CCxrReturn * pCxrReturn = pSrcMod->m_cxrReturnList[rtnIdx];

			CThreads * pSrcGroup = pCxrReturn->m_pGroup;
			vector<CField *> * pFieldList = &pCxrReturn->m_paramList;

			for (size_t callIdx = 0; callIdx < pCxrReturn->m_pairedCallList.size(); callIdx += 1) {
				CModule * pDstMod = pCxrReturn->m_pairedCallList[callIdx].m_pModInst->m_pMod;
				int dstCallIdx = pCxrReturn->m_pairedCallList[callIdx].m_callIdx;
				CCxrCall * pDstCxrCall = pDstMod->m_cxrCallList[dstCallIdx];
				CThreads * pDstGroup = pDstCxrCall->m_pGroup;

				bool bCxrIntfFields = pDstGroup->m_htIdW.AsInt() > 0 || pDstMod->m_pInstrType != 0
					|| pFieldList->size() > 0;

				// Loop through instances for both src and dst
				bool bFoundLinkInfo = false;
				for (int srcInstIdx = 0; srcInstIdx < pSrcMod->m_instSet.GetInstCnt(); srcInstIdx += 1) {
					for (int srcReplIdx = 0; srcReplIdx < pSrcMod->m_instSet.GetReplCnt(srcInstIdx); srcReplIdx += 1) {
						CModInst * pSrcModInst = pSrcMod->m_instSet.GetInst(srcInstIdx, srcReplIdx);

						if (pSrcModInst->m_instName != pDstCxrCall->m_instName) continue;

						int instIdx = 0;
						for (int dstInstIdx = 0; dstInstIdx < pDstMod->m_instSet.GetInstCnt(); dstInstIdx += 1) {
							for (int dstReplIdx = 0; dstReplIdx < pDstMod->m_instSet.GetReplCnt(dstInstIdx); dstReplIdx += 1) {
								CModInst * pDstModInst = pDstMod->m_instSet.GetInst(dstInstIdx, dstReplIdx);

								// check if instance file had information for this link
								if (IsCallDest(pDstModInst, pSrcModInst)) {

									// found link info
									bFoundLinkInfo = true;

									// record replCnt for return dst
									if (pDstModInst->m_replCnt > pSrcModInst->m_replCnt)
										pSrcMod->m_maxRtnReplCnt = max(pSrcMod->m_maxRtnReplCnt, pDstModInst->m_replCnt);

									// insert return link
									pSrcModInst->m_cxrIntfList.push_back(
										new CCxrIntf(pDstCxrCall->m_modEntry, pSrcModInst, pSrcGroup, pDstModInst, pDstGroup,
										CxrReturn, CxrOut, pDstCxrCall->m_queueW.AsInt(), pFieldList));

									CCxrIntf * pSrcCxrIntf = pSrcModInst->m_cxrIntfList.back();
									pSrcCxrIntf->m_rtnIdx = rtnIdx;
									pSrcCxrIntf->m_callIdx = callIdx;
									pSrcCxrIntf->m_instIdx = instIdx++;
									pSrcCxrIntf->m_rtnSelId = callIdx;
									pSrcCxrIntf->m_rtnReplId = pDstModInst->m_replId;
									pSrcCxrIntf->m_bCxrIntfFields = bCxrIntfFields;
									pSrcCxrIntf->m_bRtnJoin = pCxrReturn->m_bRtnJoin;

									pDstModInst->m_cxrIntfList.push_back(
										new CCxrIntf(pDstCxrCall->m_modEntry, pSrcModInst, pSrcGroup, pDstModInst, pDstGroup,
										CxrReturn, CxrIn, pDstCxrCall->m_queueW.AsInt(), pFieldList));

									CCxrIntf * pDstCxrIntf = pDstModInst->m_cxrIntfList.back();
									pDstCxrIntf->m_bCxrIntfFields = bCxrIntfFields;
									pDstCxrIntf->m_bRtnJoin = pCxrReturn->m_bRtnJoin;

									pDstModInst->m_cxrSrcCnt += 1;
								}
							}
						}
					}
				}

				if (!bFoundLinkInfo) {
					// make HIF connection
					HtlAssert(pSrcMod->m_instSet.GetTotalCnt() == 1 && pDstMod->m_instSet.GetTotalCnt() == 1);

					// 1->1
					int srcInstIdx = 0;
					int srcReplIdx = 0;
					CModInst * pSrcModInst = pSrcMod->m_instSet.GetInst(srcInstIdx, srcReplIdx);

					int dstInstIdx = 0;
					int dstReplIdx = 0;
					CModInst * pDstModInst = pDstMod->m_instSet.GetInst(dstInstIdx, dstReplIdx);

					// insert return link
					pSrcModInst->m_cxrIntfList.push_back(
						new CCxrIntf(pDstCxrCall->m_modEntry, pSrcModInst, pSrcGroup, pDstModInst, pDstGroup,
						CxrReturn, CxrOut, pDstCxrCall->m_queueW.AsInt(), pFieldList));

					CCxrIntf * pSrcCxrIntf = pSrcModInst->m_cxrIntfList.back();
					pSrcCxrIntf->m_rtnIdx = rtnIdx;
					pSrcCxrIntf->m_callIdx = callIdx;
					pSrcCxrIntf->m_rtnSelId = callIdx;
					pSrcCxrIntf->m_rtnReplId = pDstModInst->m_replId;
					pSrcCxrIntf->m_bCxrIntfFields = bCxrIntfFields;
					pSrcCxrIntf->m_bRtnJoin = pCxrReturn->m_bRtnJoin;

					pDstModInst->m_cxrIntfList.push_back(
						new CCxrIntf(pDstCxrCall->m_modEntry, pSrcModInst, pSrcGroup, pDstModInst, pDstGroup,
						CxrReturn, CxrIn, pDstCxrCall->m_queueW.AsInt(), pFieldList));

					CCxrIntf * pDstCxrIntf = pDstModInst->m_cxrIntfList.back();
					pDstCxrIntf->m_bCxrIntfFields = bCxrIntfFields;
					pDstCxrIntf->m_bRtnJoin = pCxrReturn->m_bRtnJoin;

					pDstModInst->m_cxrSrcCnt += 1;
				}
			}
		}
	}

	// for each instance's inbound return intf, find the outbound call intf
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule * pMod = m_modList[modIdx];
		if (!pMod->m_bIsUsed) continue;	// only check required modules

		// loop through the instances for each module
		for (int instIdx = 0; instIdx < pMod->m_instSet.GetInstCnt(); instIdx += 1) {
			for (int replIdx = 0; replIdx < pMod->m_instSet.GetReplCnt(instIdx); replIdx += 1) {
				CModInst * pModInst = pMod->m_instSet.GetInst(instIdx, replIdx);

				// loop through the cxr interfaces looking for inbound return
				for (size_t rtnIntfIdx = 0; rtnIntfIdx < pModInst->m_cxrIntfList.size(); rtnIntfIdx += 1) {
					CCxrIntf * pCxrRtnIntf = pModInst->m_cxrIntfList[rtnIntfIdx];

					if (pCxrRtnIntf->m_cxrDir != CxrIn) continue;
					if (pCxrRtnIntf->m_cxrType != CxrReturn) continue;

					// loop through csr interfaces looking for matching outbound call
					size_t callIntfIdx;
					for (callIntfIdx = 0; callIntfIdx < pModInst->m_cxrIntfList.size(); callIntfIdx += 1) {
						CCxrIntf * pCxrCallIntf = pModInst->m_cxrIntfList[callIntfIdx];

						if (pCxrCallIntf->m_cxrDir != CxrOut) continue;
						if (pCxrCallIntf->m_cxrType != CxrCall) continue;
						if (pCxrCallIntf->m_modEntry != pCxrRtnIntf->m_modEntry) continue;

						pCxrRtnIntf->m_callIntfIdx = callIntfIdx;
						break;
					}

					HtlAssert(callIntfIdx < pModInst->m_cxrIntfList.size());
				}
			}
		}
	}

	// Now that the instance connectivity is filled in, assign module interface names
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule * pMod = m_modList[modIdx];
		if (!pMod->m_bIsUsed) continue;	// only check required modules

		// loop through the instances for each module
		for (int instIdx = 0; instIdx < pMod->m_instSet.GetInstCnt(); instIdx += 1) {
			for (int replIdx = 0; replIdx < pMod->m_instSet.GetReplCnt(instIdx); replIdx += 1) {
				CModInst * pModInst = pMod->m_instSet.GetInst(instIdx, replIdx);

				// loop through the cxr interfaces for a module's instance
				for (size_t intfIdx = 0; intfIdx < pModInst->m_cxrIntfList.size(); intfIdx += 1) {
					CCxrIntf * pCxrIntf = pModInst->m_cxrIntfList[intfIdx];
					if (pCxrIntf->m_dstInstName.size() > 0) continue;


					// we are filling in the instance names for the interfaces
					//   if there is only one interface with a specific module name, then dst inst name is module name
					//   else inst name is module name with appended inst idx

					if (pCxrIntf->m_cxrDir == CxrIn) {
						int modCnt = 0;
						for (size_t intfIdx2 = intfIdx; intfIdx2 < pModInst->m_cxrIntfList.size(); intfIdx2 += 1) {
							CCxrIntf * pCxrIntf2 = pModInst->m_cxrIntfList[intfIdx2];

							if (pCxrIntf2->m_cxrDir != CxrIn) continue;
							if (pCxrIntf2->m_srcInstName.size() > 0) continue;

							if (pCxrIntf->m_pSrcModInst->m_instName == pCxrIntf2->m_pSrcModInst->m_instName && pCxrIntf->m_modEntry == pCxrIntf2->m_modEntry)
								modCnt += 1;
						}

						for (size_t intfIdx2 = intfIdx; intfIdx2 < pModInst->m_cxrIntfList.size(); intfIdx2 += 1) {
							CCxrIntf * pCxrIntf2 = pModInst->m_cxrIntfList[intfIdx2];

							if (pCxrIntf2->m_cxrDir != CxrIn) continue;
							if (pCxrIntf2->m_srcInstName.size() > 0) continue;

							if (pCxrIntf->m_pSrcModInst->m_instName == pCxrIntf2->m_pSrcModInst->m_instName) {
								pCxrIntf2->m_dstInstName = pCxrIntf->m_pDstModInst->m_instName;
								pCxrIntf2->m_srcInstName = pCxrIntf->m_pSrcModInst->m_instName;

								if (modCnt > 1) {
									pCxrIntf2->m_srcInstName += ('0' + pCxrIntf2->m_pSrcModInst->m_replId % modCnt);
									pCxrIntf2->m_srcReplId = pCxrIntf2->m_pSrcModInst->m_replId % modCnt;
									pCxrIntf2->m_srcReplCnt = modCnt;
								}
							}
						}
					} else {
						// CxrOut
						int modCnt = 0;
						for (size_t intfIdx2 = intfIdx; intfIdx2 < pModInst->m_cxrIntfList.size(); intfIdx2 += 1) {
							CCxrIntf * pCxrIntf2 = pModInst->m_cxrIntfList[intfIdx2];

							if (pCxrIntf2->m_cxrDir != CxrOut) continue;
							if (pCxrIntf2->m_dstInstName.size() > 0) continue;

							if (pCxrIntf->m_pDstModInst->m_instName == pCxrIntf2->m_pDstModInst->m_instName && pCxrIntf->m_modEntry == pCxrIntf2->m_modEntry)
								modCnt += 1;
						}

						for (size_t intfIdx2 = intfIdx; intfIdx2 < pModInst->m_cxrIntfList.size(); intfIdx2 += 1) {
							CCxrIntf * pCxrIntf2 = pModInst->m_cxrIntfList[intfIdx2];

							if (pCxrIntf2->m_cxrDir != CxrOut) continue;
							if (pCxrIntf2->m_dstInstName.size() > 0) continue;

							if (pCxrIntf->m_pDstModInst->m_instName == pCxrIntf2->m_pDstModInst->m_instName) {
								pCxrIntf2->m_dstInstName = pCxrIntf->m_pDstModInst->m_instName;
								pCxrIntf2->m_srcInstName = pCxrIntf->m_pSrcModInst->m_instName;

								if (modCnt > 1) {
									pCxrIntf2->m_dstInstName += ('0' + pCxrIntf2->m_pDstModInst->m_replId % modCnt);
									pCxrIntf2->m_dstReplId = pCxrIntf2->m_pDstModInst->m_replId % modCnt;
									pCxrIntf2->m_dstReplCnt = modCnt;
								}
							}
						}

					}
				}
			}
		}
	}

	// determine width of async call count variable
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule * pMod = m_modList[modIdx];
		if (!pMod->m_bIsUsed) continue;	// only check required modules

		pMod->m_bHtIdInit = false;

		for (size_t callIdx = 0; callIdx < pMod->m_cxrCallList.size(); callIdx += 1) {

			CCxrCall * pCxrCall = pMod->m_cxrCallList[callIdx];

			if (!pCxrCall->m_bCallFork) continue;

			CCxrEntry * pCxrEntry = pCxrCall->m_pPairedEntry;
			pCxrCall->m_forkCntW = max(pCxrCall->m_queueW.AsInt(), pCxrEntry->m_pGroup->m_htIdW.AsInt()) + 1;

			pMod->m_bHtIdInit |= pCxrCall->m_pGroup->m_htIdW.AsInt() > 0;
		}
	}

	// Initialize rtnReplId within groups
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule * pMod = m_modList[modIdx];

		if (!pMod->m_bIsUsed) continue;	// only check required modules

		bool bReplReturn = false;
		int maxRtnReplId = -1;

		// loop through each modInst and find returns with multiple destinations
		for (int instIdx = 0; instIdx < pMod->m_instSet.GetInstCnt(); instIdx += 1) {
			for (int replIdx = 0; replIdx < pMod->m_instSet.GetReplCnt(instIdx); replIdx += 1) {
				CModInst * pModInst = pMod->m_instSet.GetInst(instIdx, replIdx);

				///////////////////////////////////////////////////////////
				// generate Return_func routine

				bool bSingleReplId = true;
				int singleReplId = -1;
				for (size_t rtnIdx = 0; rtnIdx < pMod->m_cxrReturnList.size(); rtnIdx += 1) {

					int rtnReplCnt = 0;
					for (size_t intfIdx = 0; intfIdx < pModInst->m_cxrIntfList.size(); intfIdx += 1) {
						CCxrIntf * pCxrIntf = pModInst->m_cxrIntfList[intfIdx];

						if (pCxrIntf->m_cxrDir == CxrIn) continue;
						if (pCxrIntf->IsCallOrXfer()) continue;
						if (pCxrIntf->m_rtnIdx != (int)rtnIdx) continue;

						rtnReplCnt += 1;
						maxRtnReplId = max(maxRtnReplId, pCxrIntf->m_rtnReplId);

						if (singleReplId == -1)
							singleReplId = pCxrIntf->m_rtnReplId;
						else if (singleReplId != pCxrIntf->m_rtnReplId)
							singleReplId = -2;
					}

					if (singleReplId == -2) {
						bReplReturn = true;

						// mark cxrIntf as needing rtnReplId compare
						for (size_t intfIdx = 0; intfIdx < pModInst->m_cxrIntfList.size(); intfIdx += 1) {
							CCxrIntf * pCxrIntf = pModInst->m_cxrIntfList[intfIdx];

							if (pCxrIntf->m_cxrDir == CxrIn) continue;
							if (pCxrIntf->IsCallOrXfer()) continue;
							if (pCxrIntf->m_rtnIdx != (int)rtnIdx) continue;

							pCxrIntf->m_bMultiInstRtn = true;
						}
					}
				}
			}
		}

		// Declare return private variables
		if (bReplReturn) {
			if (pMod->m_bHasThreads) {

				string typeName;
				switch (FindLg2(maxRtnReplId)) {
				case 1: typeName = "ht_uint1"; break;
				case 2: typeName = "ht_uint2"; break;
				case 3: typeName = "ht_uint3"; break;
				case 4: typeName = "ht_uint4"; break;
				default: HtlAssert(0);
				}

				pMod->m_threads.m_htPriv.AddStructField(FindType(typeName), "rtnReplSel");
			}
		}
	}

	checkModInstParamsUsed();

	if (bError)
		printf("Fatal - previous errors prevent file generation\n");
}

bool CDsnInfo::IsCallDest(CModInst * pSrcModInst, CModInst * pDstModInst)
{
	static bool bErrorReported = false;

	// found a path match
	//   check if replicated instances should be linked
	int srcReplCnt = pSrcModInst->m_replCnt;
	int dstReplCnt = pDstModInst->m_replCnt;
	if (srcReplCnt < dstReplCnt) {
		int dstRangeSize = dstReplCnt / srcReplCnt;
		if (!bErrorReported && dstRangeSize * srcReplCnt != dstReplCnt) { // make sure divides evenly
			ParseMsg(Error, "source and destination of call/return must have replication counts that are integer multiples");
			ParseMsg(Info, "modInst=%s replCnt=%d", pSrcModInst->m_instName.c_str(), srcReplCnt);
			ParseMsg(Info, "modInst=%s replCnt=%d", pDstModInst->m_instName.c_str(), dstReplCnt);
			bErrorReported = true;
		}

		int dstRangeLow = pSrcModInst->m_replId * dstRangeSize;
		int dstRangeHigh = (pSrcModInst->m_replId + 1) * dstRangeSize - 1;

		return dstRangeLow <= pDstModInst->m_replId && pDstModInst->m_replId <= dstRangeHigh;

	} else {
		int srcRangeSize = srcReplCnt / dstReplCnt;
		if (!bErrorReported && srcRangeSize * dstReplCnt != srcReplCnt) { // make sure divides evenly
			ParseMsg(Error, "source and destination of call/return must have replication counts that are integer multiples");
			ParseMsg(Info, "modInst=%s replCnt=%d", pSrcModInst->m_instName.c_str(), srcReplCnt);
			ParseMsg(Info, "modInst=%s replCnt=%d", pDstModInst->m_instName.c_str(), dstReplCnt);
			bErrorReported = true;
		}

		int srcRangeLow = pDstModInst->m_replId * srcRangeSize;
		int srcRangeHigh = (pDstModInst->m_replId + 1) * srcRangeSize - 1;

		return srcRangeLow <= pSrcModInst->m_replId && pSrcModInst->m_replId <= srcRangeHigh;
	}
}

void CDsnInfo::GenModCxrStatements(CModule &mod, int modInstIdx)
{
	HtlAssert(mod.m_instSet.GetTotalCnt() > 0);
	CModInst * pModInst = mod.m_instSet.GetInst(modInstIdx);

	string vcdModName = VA("Pers%s", pModInst->m_instName.Uc().c_str());

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

		CCxrReturn * pCxrReturn = mod.m_cxrReturnList[rtnIdx];
		CCxrInstReturn & cxrInstReturn = pModInst->m_cxrInstReturnList[rtnIdx];

		CHtString &modEntry = pCxrReturn->m_modEntry;

		m_cxrRegDecl.Append("\tbool ht_noload c_SendReturnBusy_%s;\n", modEntry.c_str());
		m_cxrRegDecl.Append("\tbool ht_noload c_SendReturnAvail_%s;\n", modEntry.c_str());

		cxrPreInstr.Append("\tc_SendReturnAvail_%s = false;\n", modEntry.c_str());

		string vcdModName = VA("Pers%s", pModInst->m_instName.Uc().c_str());
		GenModTrace(eVcdUser, vcdModName, VA("SendReturnBusy_%s()", modEntry.c_str()),
			VA("c_SendReturnBusy_%s", modEntry.c_str()));

		if (cxrInstReturn.m_callerList.size() > 0) {
			cxrPostReg.Append("#\tifdef _HTV\n");

			const char * pSeparator = "";
			cxrPostReg.Append("\tc_SendReturnBusy_%s = ", modEntry.c_str());
			for (size_t intfIdx = 0; intfIdx < pModInst->m_cxrIntfList.size(); intfIdx += 1) {
				CCxrIntf * pCxrIntf = pModInst->m_cxrIntfList[intfIdx];

				if (pCxrIntf->m_cxrDir == CxrIn) continue;
				if (pCxrIntf->IsCallOrXfer()) continue;
				if (pCxrIntf->m_rtnIdx != (int)rtnIdx) continue;

				if (cxrInstReturn.m_callerList.size() == 1) {

					cxrPostReg.Append("%sr_%s_%sAvlCntZero%s", pSeparator,
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());

				} else {
					cxrPostReg.Append("%sr_t%d_htPriv.m_rtnSel == %d && r_%s_%sAvlCntZero%s",
						pSeparator, mod.m_execStg, pCxrIntf->m_callIdx,
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());

				}
				pSeparator = " || ";
			}
			cxrPostReg.Append(";\n");

			cxrPostReg.Append("#\telse\n");

			pSeparator = "";
			cxrPostReg.Append("\tc_SendReturnBusy_%s = ", modEntry.c_str());
			for (size_t intfIdx = 0; intfIdx < pModInst->m_cxrIntfList.size(); intfIdx += 1) {
				CCxrIntf * pCxrIntf = pModInst->m_cxrIntfList[intfIdx];

				if (pCxrIntf->m_cxrDir == CxrIn) continue;
				if (pCxrIntf->IsCallOrXfer()) continue;
				if (pCxrIntf->m_rtnIdx != (int)rtnIdx) continue;

				if (cxrInstReturn.m_callerList.size() == 1) {

					cxrPostReg.Append("%sr_%s_%sAvlCntZero%s\n", pSeparator,
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());

				} else {
					cxrPostReg.Append("%s(r_t%d_htPriv.m_rtnSel == %d && r_%s_%sAvlCntZero%s)\n",
						pSeparator,
						mod.m_execStg,
						pCxrIntf->m_callIdx,
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());

				}
				pSeparator = "\t\t|| ";
			}
			cxrPostReg.Append("\t\t|| (%s_RND_RETRY && !!(g_rndRetry() & 1));\n", mod.m_modName.Upper().c_str());

			cxrPostReg.Append("#\tendif\n");

		} else {
			cxrPostReg.Append("\tc_SendReturnBusy_%s = false;\n", modEntry.c_str());
		}

		g_appArgs.GetDsnRpt().AddLevel("bool SendReturnBusy_%s()\n", modEntry.c_str());
		g_appArgs.GetDsnRpt().EndLevel();

		m_cxrFuncDecl.Append("\tbool SendReturnBusy_%s();\n", modEntry.c_str());

		m_cxrMacros.Append("bool CPers%s%s::SendReturnBusy_%s()\n",
			unitNameUc.c_str(), pModInst->m_instName.Uc().c_str(), modEntry.c_str());
		m_cxrMacros.Append("{\n");

		m_cxrMacros.Append("\tassert_msg(r_t%d_htValid, \"Runtime check failed in CPers%s::SendReturnBusy_%s()"
			" - thread is not valid\");\n", mod.m_execStg, pModInst->m_instName.Uc().c_str(), modEntry.c_str());
		m_cxrMacros.Append("\n");

		m_cxrMacros.Append("\tc_SendReturnAvail_%s = !c_SendReturnBusy_%s;\n", modEntry.c_str(), modEntry.c_str());
		m_cxrMacros.Append("\treturn c_SendReturnBusy_%s;\n", modEntry.c_str());

		m_cxrMacros.Append("}\n");
		m_cxrMacros.Append("\n");
	}

	///////////////////////////////////////////////////////////
	// generate Return_func routine

	for (size_t rtnIdx = 0; rtnIdx < mod.m_cxrReturnList.size(); rtnIdx += 1) {
		if (!mod.m_bIsUsed) continue;

		CCxrReturn * pCxrReturn = mod.m_cxrReturnList[rtnIdx];
		CCxrInstReturn & cxrInstReturn = pModInst->m_cxrInstReturnList[rtnIdx];

		CHtString &modEntry = pCxrReturn->m_modEntry;

		g_appArgs.GetDsnRpt().AddLevel("bool SendReturn_%s(", modEntry.c_str());

		m_cxrFuncDecl.Append("\tvoid SendReturn_%s(", modEntry.c_str());
		char * pSeparater = "";
		for (size_t fldIdx = 0; fldIdx < pCxrReturn->m_paramList.size(); fldIdx += 1) {
			CField * pField = pCxrReturn->m_paramList[fldIdx];

			g_appArgs.GetDsnRpt().AddText("%s%s %s", pSeparater, pField->m_pType->m_typeName.c_str(), pField->m_name.c_str());

			m_cxrFuncDecl.Append("%s%s %s", pSeparater, pField->m_pType->m_typeName.c_str(), pField->m_name.c_str());
			pSeparater = ", ";
		}

		g_appArgs.GetDsnRpt().AddText(")\n");
		g_appArgs.GetDsnRpt().EndLevel();

		m_cxrFuncDecl.Append(");\n");

		m_cxrMacros.Append("void CPers%s%s::SendReturn_%s(",
			unitNameUc.c_str(), pModInst->m_instName.Uc().c_str(), modEntry.c_str());
		pSeparater = "";
		for (size_t fldIdx = 0; fldIdx < pCxrReturn->m_paramList.size(); fldIdx += 1) {
			CField * pField = pCxrReturn->m_paramList[fldIdx];

			m_cxrMacros.Append("%s%s %s", pSeparater, pField->m_pType->m_typeName.c_str(), pField->m_name.c_str());
			pSeparater = ", ";
		}
		m_cxrMacros.Append(")\n");

		m_cxrMacros.Append("{\n");

		m_cxrMacros.Append("\tassert_msg(r_t%d_htValid, \"Runtime check failed in CPers%s::SendReturn_%s()"
			" - thread is not valid\");\n", mod.m_execStg, pModInst->m_instName.Uc().c_str(), modEntry.c_str());
		m_cxrMacros.Append("\tassert_msg(c_t%d_htCtrl == HT_INVALID, \"Runtime check failed in CPers%s::SendReturn_%s()"
			" - an Ht control routine was already called\");\n", mod.m_execStg, pModInst->m_instName.Uc().c_str(), modEntry.c_str());
		m_cxrMacros.Append("\tassert_msg(c_SendReturnAvail_%s, \"Runtime check failed in CPers%s::SendReturn_%s()"
			" - expected SendReturnBusy_%s() to have been called and not busy\");\n",
			modEntry.c_str(), pModInst->m_instName.Uc().c_str(), modEntry.c_str(), modEntry.c_str());
		m_cxrMacros.Append("\n");

		if (cxrInstReturn.m_callerList.size() > 0) {
			for (size_t intfIdx = 0; intfIdx < pModInst->m_cxrIntfList.size(); intfIdx += 1) {
				CCxrIntf * pCxrIntf = pModInst->m_cxrIntfList[intfIdx];

				if (pCxrIntf->m_cxrDir == CxrIn) continue;
				if (pCxrIntf->IsCallOrXfer()) continue;
				if (pCxrIntf->m_rtnIdx != (int)rtnIdx) continue;

				if (pCxrIntf->m_bMultiInstRtn && cxrInstReturn.m_callerList.size() > 1) {

					m_cxrMacros.Append("\tc_%s_%sRdy%s = r_t%d_htPriv.m_rtnSel == %d && r_t%d_htPriv.m_rtnReplSel == %d;\n",
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
						mod.m_execStg,
						pCxrIntf->m_rtnSelId,
						mod.m_execStg,
						pCxrIntf->m_rtnReplId);

				} else if (pCxrIntf->m_bMultiInstRtn) {

					m_cxrMacros.Append("\tc_%s_%sRdy%s = r_t%d_htPriv.m_rtnReplSel == %d;\n",
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
						mod.m_execStg,
						pCxrIntf->m_rtnReplId);

				} else if (cxrInstReturn.m_callerList.size() > 1) {

					m_cxrMacros.Append("\tc_%s_%sRdy%s = r_t%d_htPriv.m_rtnSel == %d;\n",
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
						mod.m_execStg,
						pCxrIntf->m_rtnSelId);

				} else {
					m_cxrMacros.Append("\tc_%s_%sRdy%s = true;\n",
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
				}

				char typeName[64];
				sprintf(typeName, "%sHtId_t", pCxrIntf->m_pDstModInst->m_pMod->m_modName.Uc().c_str());

				CTypeDef * pTypeDef = FindTypeDef(typeName);

				if (pTypeDef/* && pTypeDef->m_width.AsInt() > 0*/)
					m_cxrMacros.Append("\tc_%s_%s%s.m_rtnHtId = (%s)r_t%d_htPriv.m_rtnHtId;\n",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
					typeName, mod.m_execStg);

				sprintf(typeName, "%sInstr_t", pCxrIntf->m_pDstModInst->m_pMod->m_modName.Uc().c_str());

				pTypeDef = FindTypeDef(typeName);

				if (pTypeDef && pTypeDef->m_width.AsInt() > 0)
					m_cxrMacros.Append("\tc_%s_%s%s.m_rtnInstr = (%s)r_t%d_htPriv.m_rtnInstr;\n",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
					typeName, mod.m_execStg);

				if (pCxrReturn->m_bRtnJoin) {
					m_cxrMacros.Append("\tc_%s_%s%s.m_rtnJoin = r_t%d_htPriv.m_rtnJoin;\n",
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
						mod.m_execStg);
				}

				for (size_t fldIdx = 0; fldIdx < pCxrReturn->m_paramList.size(); fldIdx += 1) {
					CField * pField = pCxrReturn->m_paramList[fldIdx];

					m_cxrMacros.Append("\tc_%s_%s%s.m_%s = %s;\n",
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
						pField->m_name.c_str(), pField->m_name.c_str());
				}
			}

			m_cxrMacros.Append("\n");
			m_cxrMacros.Append("\tc_t%d_htCtrl = HT_RTN;\n", mod.m_execStg);

		} else {
			m_cxrMacros.Append("\tassert_msg(false, \"Runtime check failed in CPers%s::SendReturn_%s()"
				" - return does not have a paired call\");\n", pModInst->m_instName.Uc().c_str(), modEntry.c_str());
		}

		if (g_appArgs.IsInstrTraceEnabled()) {
			m_cxrMacros.Append("\n");
			m_cxrMacros.Append("#\tifndef _HTV\n");
			m_cxrMacros.Append("\tif (Ht::g_instrTraceFp)\n");
			m_cxrMacros.Append("\t\tfprintf(Ht::g_instrTraceFp, \"SendReturn_%s(", modEntry.c_str());

			pSeparater = "";
			for (size_t fldIdx = 0; fldIdx < pCxrReturn->m_paramList.size(); fldIdx += 1) {
				CField * pField = pCxrReturn->m_paramList[fldIdx];

				bool bBasicType;
				CTypeDef * pTypeDef;
				string type = pField->m_pType->m_typeName;
				while (!(bBasicType = IsBaseType(type))) {
					if (!(pTypeDef = FindTypeDef(pField->m_pType->m_typeName)))
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
			for (size_t fldIdx = 0; fldIdx < pCxrReturn->m_paramList.size(); fldIdx += 1) {
				CField * pField = pCxrReturn->m_paramList[fldIdx];

				bool bBasicType;
				CTypeDef * pTypeDef;
				string type = pField->m_pType->m_typeName;
				while (!(bBasicType = IsBaseType(type))) {
					if (!(pTypeDef = FindTypeDef(pField->m_pType->m_typeName)))
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

		CCxrCall * pCxrCall = mod.m_cxrCallList[callIdx];

		if (!pCxrCall->m_bXfer && pCxrCall->m_bCallFork) continue;

		string callType = pCxrCall->m_bXfer ? "Transfer" : "Call";
		string callName = pCxrCall->m_callName.AsStr();
		bool bDestAuto = pCxrCall->m_dest == "auto";

		int replCnt = 1;
		for (size_t intfIdx = 0; intfIdx < pModInst->m_cxrIntfList.size(); intfIdx += 1) {
			CCxrIntf * pCxrIntf = pModInst->m_cxrIntfList[intfIdx];

			if (pCxrIntf->m_cxrDir == CxrIn) continue;
			if (!pCxrIntf->IsCallOrXfer()) continue;
			if (pCxrIntf->m_callIdx != (int)callIdx) continue;

			if (pCxrIntf->m_instCnt != 1) {
				replCnt = pCxrIntf->m_instCnt;
			}
		}
		int replCntW = FindLg2(replCnt - 1);

		string vcdModName = VA("Pers%s", pModInst->m_instName.Uc().c_str());
		if (bDestAuto) {
			m_cxrRegDecl.Append("\tbool ht_noload c_Send%sBusy_%s;\n", callType.c_str(), callName.c_str());
			m_cxrRegDecl.Append("\tbool ht_noload c_Send%sAvail_%s;\n", callType.c_str(), callName.c_str());

			cxrPreInstr.Append("\tc_Send%sAvail_%s = false;\n", callType.c_str(), callName.c_str());

			GenModTrace(eVcdUser, vcdModName, VA("Send%sBusy_%s()", callType.c_str(), callName.c_str()),
				VA("c_Send%sBusy_%s", callType.c_str(), callName.c_str()));
		} else {
			m_cxrRegDecl.Append("\tbool ht_noload c_Send%sBusy_%s[%d];\n", callType.c_str(), callName.c_str(), replCnt);
			m_cxrRegDecl.Append("\tbool ht_noload c_Send%sAvail_%s[%d];\n", callType.c_str(), callName.c_str(), replCnt);

			for (int replIdx = 0; replIdx < replCnt; replIdx += 1) {
				cxrPreInstr.Append("\tc_Send%sAvail_%s[%d] = false;\n", callType.c_str(), callName.c_str(), replIdx);

				GenModTrace(eVcdUser, vcdModName, VA("Send%sBusy_%s(%d)", callType.c_str(), callName.c_str(), replIdx),
					VA("c_Send%sBusy_%s[%d]", callType.c_str(), callName.c_str(), replIdx));
			}
		}

		// generate busy signal
		cxrPostReg.Append("#\tifdef _HTV\n");

		for (int replIdx = 0; replIdx < replCnt; replIdx += 1) {
			string indexStr = bDestAuto ? "" : VA("[%d]", replIdx);

			cxrPostReg.Append("\tc_Send%sBusy_%s%s = ", callType.c_str(), callName.c_str(), indexStr.c_str());

			const char * pSeparator = "";
			for (size_t intfIdx = 0; intfIdx < pModInst->m_cxrIntfList.size(); intfIdx += 1) {
				CCxrIntf * pCxrIntf = pModInst->m_cxrIntfList[intfIdx];

				if (pCxrIntf->m_cxrDir == CxrIn) continue;
				if (!pCxrIntf->IsCallOrXfer()) continue;
				if (pCxrIntf->m_callIdx != (int)callIdx) continue;

				if (pCxrCall->m_dest == "auto") {
					cxrPostReg.Append("%sr_%s_%sAvlCntZero%s",
						pSeparator, pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());

					if (pCxrIntf->m_pDstModInst->m_pMod->m_clkRate == eClk1x && pCxrIntf->m_pDstModInst->m_pMod->m_clkRate != mod.m_clkRate)
						cxrPostReg.Append(" || (r_%s_%sRdy%s & r_phase)",
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
				} else {
					cxrPostReg.Append("%sr_%s_%sAvlCntZero%s",
						pSeparator, pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), indexStr.c_str());

					if (pCxrIntf->m_pDstModInst->m_pMod->m_clkRate == eClk1x && pCxrIntf->m_pDstModInst->m_pMod->m_clkRate != mod.m_clkRate)
						cxrPostReg.Append(" || (r_%s_%sRdy%s & r_phase)",
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), indexStr.c_str());

					break;
				}

				pSeparator = " || ";
			}
			cxrPostReg.Append(";\n");

			if (pCxrCall->m_dest == "auto")
				break;
		}

		cxrPostReg.Append("#\telse\n");

		for (int replIdx = 0; replIdx < replCnt; replIdx += 1) {
			string indexStr = bDestAuto ? "" : VA("[%d]", replIdx);

			cxrPostReg.Append("\tc_Send%sBusy_%s%s = ", callType.c_str(), callName.c_str(), indexStr.c_str());

			const char * pSeparator = "";
			for (size_t intfIdx = 0; intfIdx < pModInst->m_cxrIntfList.size(); intfIdx += 1) {
				CCxrIntf * pCxrIntf = pModInst->m_cxrIntfList[intfIdx];

				if (pCxrIntf->m_cxrDir == CxrIn) continue;
				if (!pCxrIntf->IsCallOrXfer()) continue;
				if (pCxrIntf->m_callIdx != (int)callIdx) continue;

				if (pCxrCall->m_dest == "auto") {
					cxrPostReg.Append("%sr_%s_%sAvlCntZero%s",
						pSeparator,
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());

					if (pCxrIntf->m_pDstModInst->m_pMod->m_clkRate == eClk1x && pCxrIntf->m_pDstModInst->m_pMod->m_clkRate != mod.m_clkRate)
						cxrPostReg.Append(" || (r_%s_%sRdy%s & r_phase)",
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
				} else {
					cxrPostReg.Append("%sr_%s_%sAvlCntZero%s",
						pSeparator,
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), indexStr.c_str());

					if (pCxrIntf->m_pDstModInst->m_pMod->m_clkRate == eClk1x && pCxrIntf->m_pDstModInst->m_pMod->m_clkRate != mod.m_clkRate)
						cxrPostReg.Append("%s(r_%s_%sRdy%s & r_phase)",
						pSeparator,
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), indexStr.c_str());
				}

				pSeparator = " || ";
			}

			cxrPostReg.Append("\n");
			cxrPostReg.Append("\t\t|| (%s_RND_RETRY && !!(g_rndRetry() & 1));\n", mod.m_modName.Upper().c_str());

			if (pCxrCall->m_dest == "auto")
				break;
		}

		cxrPostReg.Append("#\tendif\n");

		// generate Busy routine
		string paramStr = bDestAuto ? "" : VA("ht_uint%d%s destId", replCntW == 0 ? 1 : replCntW, replCntW == 0 ? " ht_noload" : "");
		string indexStr = bDestAuto ? "" : "[destId]";

		g_appArgs.GetDsnRpt().AddLevel("bool Send%sBusy_%s(%s)\n",
			pCxrCall->m_bXfer ? "Transfer" : "Call", callName.c_str(), paramStr.c_str());
		g_appArgs.GetDsnRpt().EndLevel();

		m_cxrFuncDecl.Append("\tbool Send%sBusy_%s(%s);\n",
			pCxrCall->m_bXfer ? "Transfer" : "Call", callName.c_str(), paramStr.c_str());

		m_cxrMacros.Append("bool CPers%s%s::Send%sBusy_%s(%s)\n",
			unitNameUc.c_str(), pModInst->m_instName.Uc().c_str(),
			pCxrCall->m_bXfer ? "Transfer" : "Call", callName.c_str(), paramStr.c_str());
		m_cxrMacros.Append("{\n");

		m_cxrMacros.Append("\tassert_msg(r_t%d_htValid, \"Runtime check failed in CPers%s::Send%sBusy_%s()"
			" - thread is not valid\");\n", mod.m_execStg, pModInst->m_instName.Uc().c_str(), pCxrCall->m_bXfer ? "Transfer" : "Call", callName.c_str());
		m_cxrMacros.Append("\n");

		m_cxrMacros.Append("\tc_Send%sAvail_%s%s = !c_Send%sBusy_%s%s;\n",
			callType.c_str(), callName.c_str(), indexStr.c_str(),
			callType.c_str(), callName.c_str(), indexStr.c_str());
		m_cxrMacros.Append("\treturn c_Send%sBusy_%s%s;\n",
			callType.c_str(), callName.c_str(), indexStr.c_str());

		m_cxrMacros.Append("}\n");

		m_cxrMacros.Append("\n");
	}

	///////////////////////////////////////////////////////////
	// generate Call_func or Transfer_func routines

	for (size_t callIdx = 0; callIdx < mod.m_cxrCallList.size(); callIdx += 1) {
		if (!mod.m_bIsUsed) continue;

		CCxrCall * pCxrCall = mod.m_cxrCallList[callIdx];
		string &callName = pCxrCall->m_callName;

		if (!pCxrCall->m_bCall && !pCxrCall->m_bXfer) continue;

		CModIdx & pairedFunc = pCxrCall->m_pairedFunc;
		CCxrEntry * pCxrEntry = pairedFunc.GetCxrEntry();

		bool bReplIntf = false;
		int replCnt = 1;
		for (size_t intfIdx = 0; intfIdx < pModInst->m_cxrIntfList.size(); intfIdx += 1) {
			CCxrIntf * pCxrIntf = pModInst->m_cxrIntfList[intfIdx];

			if (pCxrIntf->m_cxrDir == CxrIn) continue;
			if (!pCxrIntf->IsCallOrXfer()) continue;
			if (pCxrIntf->m_callIdx != (int)callIdx) continue;

			if (pCxrIntf->m_instCnt != 1) {
				bReplIntf = true;
				replCnt = pCxrIntf->m_instCnt;
			}
		}

		g_appArgs.GetDsnRpt().AddLevel("void Send%s_%s(",
			pCxrCall->m_bXfer ? "Transfer" : "Call", callName.c_str());

		m_cxrFuncDecl.Append("\tvoid Send%s_%s(",
			pCxrCall->m_bXfer ? "Transfer" : "Call", callName.c_str());

		m_cxrMacros.Append("void CPers%s%s::Send%s_%s(",
			unitNameUc.c_str(), pModInst->m_instName.Uc().c_str(),
			pCxrCall->m_bXfer ? "Transfer" : "Call", callName.c_str());

		char const * pSeparater;
		if (!pCxrCall->m_bXfer) {
			g_appArgs.GetDsnRpt().AddText("ht_uint%d rtnInstr", mod.m_instrW);
			m_cxrFuncDecl.Append("ht_uint%d rtnInstr", mod.m_instrW);
			m_cxrMacros.Append("ht_uint%d rtnInstr", mod.m_instrW);
			pSeparater = ", ";
		} else
			pSeparater = "";

		if (pCxrCall->m_dest == "user") {
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
			" - thread is not valid\");\n", mod.m_execStg, pModInst->m_instName.Uc().c_str(), pCxrCall->m_bXfer ? "Transfer" : "Call", callName.c_str());
		m_cxrMacros.Append("\n");

		for (size_t intfIdx = 0; intfIdx < pModInst->m_cxrIntfList.size(); intfIdx += 1) {
			CCxrIntf * pCxrIntf = pModInst->m_cxrIntfList[intfIdx];

			if (pCxrIntf->m_cxrDir == CxrIn) continue;
			if (!pCxrIntf->IsCallOrXfer()) continue;
			if (pCxrIntf->m_callIdx != (int)callIdx) continue;

			if (pCxrIntf->m_instCnt == 1)
				m_cxrMacros.Append("\tc_%s_%sRdy%s = true;\n",
				pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
			else {
				if (pCxrCall->m_dest == "auto")
					m_cxrMacros.Append("\tc_%s_%sRdy%s = r_%sCall_roundRobin == %dUL;\n",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
					pCxrCall->m_callName.c_str(), pCxrIntf->m_instIdx);
				else
					m_cxrMacros.Append("\tc_%s_%sRdy%s = destId == %dUL;\n",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
					pCxrIntf->m_instIdx);
			}

			if (pCxrCall->m_bXfer) {
				if (pCxrCall->m_pGroup->m_rtnSelW > 0)
					m_cxrMacros.Append("\tc_%s_%s%s.m_rtnSel = r_t%d_htPriv.m_rtnSel;\n",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
					mod.m_execStg);

				if (pCxrCall->m_pGroup->m_rtnHtIdW > 0)
					m_cxrMacros.Append("\tc_%s_%s%s.m_rtnHtId = r_t%d_htPriv.m_rtnHtId;\n",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
					mod.m_execStg);

				if (pCxrCall->m_pGroup->m_rtnInstrW > 0)
					m_cxrMacros.Append("\tc_%s_%s%s.m_rtnInstr = r_t%d_htPriv.m_rtnInstr;\n",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
					mod.m_execStg);

				if (pCxrCall->m_bCallFork || pCxrCall->m_bXferFork) {
					m_cxrMacros.Append("\tc_%s_%s%s.m_rtnJoin = r_t%d_htPriv.m_rtnJoin;\n",
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
						mod.m_execStg);
				}

			} else {
				if (pCxrCall->m_pGroup->m_htIdW.AsInt() > 0)
					m_cxrMacros.Append("\tc_%s_%s%s.m_rtnHtId = r_t%d_htId;\n",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
					mod.m_execStg);

				if (mod.m_instrW > 0)
					m_cxrMacros.Append("\tc_%s_%s%s.m_rtnInstr = rtnInstr;\n",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());

				if (pCxrCall->m_bCallFork || pCxrCall->m_bXferFork) {
					m_cxrMacros.Append("\tc_%s_%s%s.m_rtnJoin = false;\n",
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
				}
			}

			for (size_t fldIdx = 0; fldIdx < pCxrEntry->m_paramList.size(); fldIdx += 1) {
				CField * pField = pCxrEntry->m_paramList[fldIdx];

				m_cxrMacros.Append("\tc_%s_%s%s.m_%s = %s;\n",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
					pField->m_name.c_str(), pField->m_name.c_str());
			}
		}
		m_cxrMacros.Append("\n");

		if (bReplIntf && pCxrCall->m_dest == "auto") {
			m_cxrMacros.Append("\tc_%sCall_roundRobin = (r_%sCall_roundRobin + 1U) %% %dU;\n",
				pCxrCall->m_callName.c_str(), pCxrCall->m_callName.c_str(), replCnt);
			m_cxrMacros.Append("\n");
		}

		m_cxrMacros.Append("\tassert_msg(c_t%d_htCtrl == HT_INVALID, \"Runtime check failed in CPers%s::Send%s_%s()"
			" - an Ht control routine was already called\");\n", mod.m_execStg, pModInst->m_instName.Uc().c_str(),
			pCxrCall->m_bXfer ? "Transfer" : "Call", callName.c_str());
		m_cxrMacros.Append("\n");

		m_cxrMacros.Append("\tc_t%d_htCtrl = HT_CALL;\n", mod.m_execStg);

		if (g_appArgs.IsInstrTraceEnabled()) {
			m_cxrMacros.Append("\n");
			m_cxrMacros.Append("#\tifndef _HTV\n");
			m_cxrMacros.Append("\tif (Ht::g_instrTraceFp)\n");
			m_cxrMacros.Append("\t\tfprintf(Ht::g_instrTraceFp, \"Send%s_%s(",
				pCxrCall->m_bXfer ? "Transfer" : "Call", pCxrCall->m_modEntry.c_str());

			pSeparater = "";
			for (size_t fldIdx = 0; fldIdx < pCxrEntry->m_paramList.size(); fldIdx += 1) {
				CField * pField = pCxrEntry->m_paramList[fldIdx];

				bool bBasicType;
				CTypeDef * pTypeDef;
				string type = pField->m_pType->m_typeName;
				while (!(bBasicType = IsBaseType(type))) {
					if (!(pTypeDef = FindTypeDef(pField->m_pType->m_typeName)))
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
				string type = pField->m_pType->m_typeName;
				while (!(bBasicType = IsBaseType(type))) {
					if (!(pTypeDef = FindTypeDef(pField->m_pType->m_typeName)))
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

	for (size_t intfIdx = 0; intfIdx < pModInst->m_cxrIntfList.size(); intfIdx += 1) {
		CCxrIntf * pCxrIntf = pModInst->m_cxrIntfList[intfIdx];

		vector<CHtString> replDim;
		if (pCxrIntf->GetPortReplCnt() > 1) {
			CHtString str = pCxrIntf->GetPortReplCnt();
			replDim.push_back(str);
		}

		if (pCxrIntf->m_cxrDir == CxrIn) {
			if (pCxrIntf->m_srcReplCnt > 1 && pCxrIntf->m_srcReplId != 0) continue;

			m_cxrIoDecl.Append("\t// Inbound %s interface from %s\n",
				pCxrIntf->m_cxrType == CxrCall ? "call" : (pCxrIntf->m_cxrType == CxrReturn ? "return" : "xfer"),
				pCxrIntf->m_pSrcModInst->m_instName.c_str());

			GenModDecl(eVcdAll, m_cxrIoDecl, vcdModName, "sc_in<bool>",
				VA("i_%s_%sRdy", pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName()), replDim);

			if (pCxrIntf->m_pSrcModInst->m_pMod->m_bGvIwComp)
				m_cxrIoDecl.Append("\tsc_in<bool> i_%s_%sCompRdy%s;\n", pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());

			if (pCxrIntf->m_bCxrIntfFields)
				m_cxrIoDecl.Append("\tsc_in<C%s_%sIntf> i_%s_%s%s;\n",
				pCxrIntf->GetSrcToDstUc(), pCxrIntf->GetIntfName(),
				pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());

			m_cxrIoDecl.Append("\tsc_out<bool> o_%s_%sAvl%s;\n",
				pCxrIntf->GetPortNameDstToSrcLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());

		} else {
			if (pCxrIntf->m_dstReplCnt > 1 && pCxrIntf->m_dstReplId != 0) continue;

			m_cxrIoDecl.Append("\t// Outbound %s interace to %s\n",
				pCxrIntf->m_cxrType == CxrCall ? "call" : (pCxrIntf->m_cxrType == CxrReturn ? "return" : "xfer"),
				pCxrIntf->m_pDstModInst->m_instName.c_str());

			GenModDecl(eVcdAll, m_cxrIoDecl, vcdModName, "sc_out<bool>",
				VA("o_%s_%sRdy", pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName()), replDim);

			if (pCxrIntf->m_pSrcModInst->m_pMod->m_bGvIwComp && pCxrIntf->m_pDstModInst->m_pMod->m_modName.AsStr() != "hif") {
				m_cxrIoDecl.Append("\tsc_out<bool> o_%s_%sCompRdy%s;\n",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());
			}

			if (pCxrIntf->m_bCxrIntfFields)
				m_cxrIoDecl.Append("\tsc_out<C%s_%sIntf> o_%s_%s%s;\n",
				pCxrIntf->GetSrcToDstUc(), pCxrIntf->GetIntfName(),
				pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());

			m_cxrIoDecl.Append("\tsc_in<bool> i_%s_%sAvl%s;\n",
				pCxrIntf->GetPortNameDstToSrcLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());

		}

		m_cxrIoDecl.Append("\n");
	}

	///////////////////////////////////////////////////////////////////////
	// declare and initialize Async Call variables

	for (size_t callIdx = 0; callIdx < mod.m_cxrCallList.size(); callIdx += 1) {
		if (!mod.m_bIsUsed) continue;

		CCxrCall * pCxrCall = mod.m_cxrCallList[callIdx];

		if (pCxrCall->m_bXfer || !pCxrCall->m_bCallFork) continue;

		CModIdx & pairedFunc = pCxrCall->m_pairedFunc;
		CCxrEntry * pCxrEntry = pairedFunc.GetCxrEntry();

		int replCnt = 0;
		for (size_t intfIdx = 0; intfIdx < pModInst->m_cxrIntfList.size(); intfIdx += 1) {
			CCxrIntf * pCxrIntf = pModInst->m_cxrIntfList[intfIdx];

			if (pCxrIntf->m_cxrDir == CxrIn) continue;
			if (!pCxrIntf->IsCallOrXfer()) continue;
			if (pCxrIntf->m_callIdx != (int)callIdx) continue;

			replCnt = pCxrIntf->m_instCnt;
			break;
		}
		int replCntW = FindLg2(replCnt, false, true);

		int htIdW = pCxrCall->m_pGroup->m_htIdW.AsInt();
		int forkCntW = pCxrCall->m_forkCntW + replCntW;
		string callName = pCxrCall->m_callName;
		string modEntry = pCxrCall->m_modEntry.AsStr();

		char rtnCntTypeStr[64];
		sprintf(rtnCntTypeStr, "ht_uint%d", forkCntW);

		bool bDestAuto = pCxrCall->m_dest == "auto";

		string declStr = bDestAuto ? "" : VA("[%d]", replCnt);
		vector<CHtString> replDim;
		if (!bDestAuto) {
			char buf[16];
			sprintf(buf, "%d", replCnt);
			CHtString str = (string)buf;
			CLineInfo li;
			str.InitValue(li);
			replDim.push_back(str);
		}

		// declarations
		if (htIdW == 0) {

			GenModDecl(eVcdAll, m_cxrRegDecl, vcdModName, "bool", VA("r_asyncCall%s_waiting", pCxrCall->m_modEntry.c_str()), replDim);
			m_cxrRegDecl.Append("\tbool c_asyncCall%s_waiting%s;\n", pCxrCall->m_modEntry.c_str(), declStr.c_str());
			GenModDecl(eVcdAll, m_cxrRegDecl, vcdModName, "bool", VA("r_asyncCall%s_rtnCntFull", pCxrCall->m_modEntry.c_str()), replDim);
			m_cxrRegDecl.Append("\tbool c_asyncCall%s_rtnCntFull%s;\n", pCxrCall->m_modEntry.c_str(), declStr.c_str());
			GenModDecl(eVcdAll, m_cxrRegDecl, vcdModName, rtnCntTypeStr, VA("r_asyncCall%s_rtnCnt", pCxrCall->m_modEntry.c_str()), replDim);
			m_cxrRegDecl.Append("\tht_uint%d c_asyncCall%s_rtnCnt%s;\n", forkCntW, pCxrCall->m_modEntry.c_str(), declStr.c_str());
			m_cxrRegDecl.Append("\tht_uint%d r_asyncCall%s_rsmInstr%s;\n", mod.m_instrW, pCxrCall->m_modEntry.c_str(), declStr.c_str());
			m_cxrRegDecl.Append("\tht_uint%d c_asyncCall%s_rsmInstr%s;\n", mod.m_instrW, pCxrCall->m_modEntry.c_str(), declStr.c_str());

		} else {

			m_cxrRegDecl.Append("\tht_dist_ram<bool, %d> m_asyncCall%s_waiting%s;\n",
				htIdW, pCxrCall->m_modEntry.c_str(), declStr.c_str());
			m_cxrRegDecl.Append("\tht_dist_ram<bool, %d> m_asyncCall%s_rtnCntFull%s;\n",
				htIdW, pCxrCall->m_modEntry.c_str(), declStr.c_str());
			m_cxrRegDecl.Append("\tht_dist_ram<ht_uint%d, %d> m_asyncCall%s_rtnCnt%s;\n",
				forkCntW, htIdW, pCxrCall->m_modEntry.c_str(), declStr.c_str());
			m_cxrRegDecl.Append("\tht_dist_ram<ht_uint%d, %d> m_asyncCall%s_rsmInstr%s;\n",
				mod.m_instrW, htIdW, pCxrCall->m_modEntry.c_str(), declStr.c_str());
			m_cxrRegDecl.Append("\n");

			m_cxrRegDecl.Append("\tbool c_t%d_asyncCall%s_rtnCntFull%s;\n",
				mod.m_execStg - 1, pCxrCall->m_modEntry.c_str(), declStr.c_str());
			GenModDecl(eVcdAll, m_cxrRegDecl, vcdModName, "bool",
				VA("r_t%d_asyncCall%s_rtnCntFull",
				mod.m_execStg, pCxrCall->m_modEntry.c_str()), replDim);
			m_cxrRegDecl.Append("\n");
		}

		for (int replIdx = 0; replIdx < replCnt; replIdx += 1) {
			string replStr = bDestAuto ? "" : VA("[%d]", replIdx);

			if (htIdW == 0) {

				cxrTsStage.Append("\tc_asyncCall%s_waiting%s = r_asyncCall%s_waiting%s;\n",
					pCxrCall->m_modEntry.c_str(), replStr.c_str(), pCxrCall->m_modEntry.c_str(), replStr.c_str());
				cxrTsStage.Append("\tc_asyncCall%s_rtnCnt%s = r_asyncCall%s_rtnCnt%s;\n",
					pCxrCall->m_modEntry.c_str(), replStr.c_str(), pCxrCall->m_modEntry.c_str(), replStr.c_str());
				cxrTsStage.Append("\tc_asyncCall%s_rtnCntFull%s = r_asyncCall%s_rtnCntFull%s;\n",
					pCxrCall->m_modEntry.c_str(), replStr.c_str(), pCxrCall->m_modEntry.c_str(), replStr.c_str());
				cxrTsStage.Append("\tc_asyncCall%s_rsmInstr%s = r_asyncCall%s_rsmInstr%s;\n",
					pCxrCall->m_modEntry.c_str(), replStr.c_str(), pCxrCall->m_modEntry.c_str(), replStr.c_str());

				cxrReg.Append("\tr_asyncCall%s_waiting%s = !%s && c_asyncCall%s_waiting%s;\n",
					pCxrCall->m_modEntry.c_str(), replStr.c_str(), reset.c_str(), pCxrCall->m_modEntry.c_str(), replStr.c_str());
				cxrReg.Append("\tr_asyncCall%s_rtnCnt%s = %s ? (ht_uint%d)0 : c_asyncCall%s_rtnCnt%s;\n",
					pCxrCall->m_modEntry.c_str(), replStr.c_str(), reset.c_str(), forkCntW, pCxrCall->m_modEntry.c_str(), replStr.c_str());
				cxrReg.Append("\tr_asyncCall%s_rtnCntFull%s = !%s && c_asyncCall%s_rtnCntFull%s;\n",
					pCxrCall->m_modEntry.c_str(), replStr.c_str(), reset.c_str(), pCxrCall->m_modEntry.c_str(), replStr.c_str());
				cxrReg.Append("\tr_asyncCall%s_rsmInstr%s = c_asyncCall%s_rsmInstr%s;\n",
					pCxrCall->m_modEntry.c_str(), replStr.c_str(), pCxrCall->m_modEntry.c_str(), replStr.c_str());

			} else {

				cxrTsStage.Append("\tm_asyncCall%s_waiting%s.read_addr(r_t%d_htId);\n",
					pCxrCall->m_modEntry.c_str(), replStr.c_str(), mod.m_execStg);
				cxrTsStage.Append("\tm_asyncCall%s_waiting%s.write_addr(r_t%d_htId);\n",
					pCxrCall->m_modEntry.c_str(), replStr.c_str(), mod.m_execStg);
				cxrTsStage.Append("\tm_asyncCall%s_rtnCnt%s.read_addr(r_t%d_htId);\n",
					pCxrCall->m_modEntry.c_str(), replStr.c_str(), mod.m_execStg);
				cxrTsStage.Append("\tm_asyncCall%s_rtnCnt%s.write_addr(r_t%d_htId);\n",
					pCxrCall->m_modEntry.c_str(), replStr.c_str(), mod.m_execStg);
				cxrTsStage.Append("\tm_asyncCall%s_rtnCntFull%s.read_addr(r_t%d_htId);\n",
					pCxrCall->m_modEntry.c_str(), replStr.c_str(), mod.m_execStg - 1);
				cxrTsStage.Append("\tm_asyncCall%s_rtnCntFull%s.write_addr(r_t%d_htId);\n",
					pCxrCall->m_modEntry.c_str(), replStr.c_str(), mod.m_execStg);
				cxrTsStage.Append("\tm_asyncCall%s_rsmInstr%s.read_addr(r_t%d_htId);\n",
					pCxrCall->m_modEntry.c_str(), replStr.c_str(), mod.m_execStg);
				cxrTsStage.Append("\tm_asyncCall%s_rsmInstr%s.write_addr(r_t%d_htId);\n",
					pCxrCall->m_modEntry.c_str(), replStr.c_str(), mod.m_execStg);
				cxrTsStage.Append("\n");
				cxrTsStage.Append("\tc_t%d_asyncCall%s_rtnCntFull%s = m_asyncCall%s_rtnCntFull%s.read_mem();\n",
					mod.m_execStg - 1, pCxrCall->m_modEntry.c_str(), replStr.c_str(), pCxrCall->m_modEntry.c_str(), replStr.c_str());
				cxrTsStage.Append("\n");

				cxrReg.Append("\tm_asyncCall%s_waiting%s.clock();\n",
					pCxrCall->m_modEntry.c_str(), replStr.c_str());
				cxrReg.Append("\tm_asyncCall%s_rtnCnt%s.clock();\n",
					pCxrCall->m_modEntry.c_str(), replStr.c_str());
				cxrReg.Append("\tm_asyncCall%s_rtnCntFull%s.clock();\n",
					pCxrCall->m_modEntry.c_str(), replStr.c_str());
				cxrReg.Append("\tm_asyncCall%s_rsmInstr%s.clock();\n",
					pCxrCall->m_modEntry.c_str(), replStr.c_str());
				cxrReg.Append("\n");

				cxrPostInstr.Append("\tif (r_t%d_htIdInit) {\n", mod.m_execStg);
				cxrPostInstr.Append("\t\tm_asyncCall%s_waiting%s.write_mem(false);\n",
					pCxrCall->m_modEntry.c_str(), replStr.c_str());
				cxrPostInstr.Append("\t\tm_asyncCall%s_rtnCnt%s.write_mem(0);\n",
					pCxrCall->m_modEntry.c_str(), replStr.c_str());
				cxrPostInstr.Append("\t\tm_asyncCall%s_rtnCntFull%s.write_mem(false);\n",
					pCxrCall->m_modEntry.c_str(), replStr.c_str());
				cxrPostInstr.Append("\t}\n");
				cxrPostInstr.Append("\n");

				cxrReg.Append("\tr_t%d_asyncCall%s_rtnCntFull%s = c_t%d_asyncCall%s_rtnCntFull%s;\n",
					mod.m_execStg, pCxrCall->m_modEntry.c_str(), replStr.c_str(), mod.m_execStg - 1, pCxrCall->m_modEntry.c_str(), replStr.c_str());
				m_cxrRegDecl.Append("\n");
			}

			if (bDestAuto)
				break;
		}

		// Generate Async Call Busy Function

		string vcdModName = VA("Pers%s", pModInst->m_instName.Uc().c_str());
		if (bDestAuto) {
			m_cxrRegDecl.Append("\tbool ht_noload c_SendCallBusy_%s;\n", callName.c_str());
			m_cxrRegDecl.Append("\tbool ht_noload c_SendCallAvail_%s;\n", callName.c_str());

			cxrPreInstr.Append("\tc_SendCallAvail_%s = false;\n", callName.c_str());

			GenModTrace(eVcdUser, vcdModName, VA("SendCallBusy_%s()", callName.c_str()),
				VA("c_SendCallBusy_%s", callName.c_str()));
		} else {
			m_cxrRegDecl.Append("\tbool ht_noload c_SendCallBusy_%s[%d];\n", callName.c_str(), replCnt);
			m_cxrRegDecl.Append("\tbool ht_noload c_SendCallAvail_%s[%d];\n", callName.c_str(), replCnt);

			for (int replIdx = 0; replIdx < replCnt; replIdx += 1) {
				cxrPreInstr.Append("\tc_SendCallAvail_%s[%d] = false;\n", callName.c_str(), replIdx);

				GenModTrace(eVcdUser, vcdModName, VA("SendCallBusy_%s(%d)", callName.c_str(), replIdx),
					VA("c_SendCallBusy_%s[%d]", callName.c_str(), replIdx));
			}
		}

		cxrPostReg.Append("#\tifdef _HTV\n");

		for (int replIdx = 0; replIdx < replCnt; replIdx += 1) {
			string indexStr = bDestAuto ? "" : VA("[%d]", replIdx);

			if (htIdW == 0) {
				cxrPostReg.Append("\t\tc_SendCallBusy_%s%s = r_asyncCall%s_rtnCntFull%s",
					callName.c_str(), indexStr.c_str(),
					callName.c_str(), indexStr.c_str());
			} else {
				cxrPostReg.Append("\t\tc_SendCallBusy_%s%s = r_t%d_asyncCall%s_rtnCntFull%s",
					callName.c_str(), indexStr.c_str(),
					mod.m_execStg, callName.c_str(), indexStr.c_str());
			}

			const char * pSeparator = "\n\t\t\t|| ";

			for (size_t intfIdx = 0; intfIdx < pModInst->m_cxrIntfList.size(); intfIdx += 1) {
				CCxrIntf * pCxrIntf = pModInst->m_cxrIntfList[intfIdx];

				if (pCxrIntf->m_cxrDir == CxrIn) continue;
				if (!pCxrIntf->IsCallOrXfer()) continue;
				if (pCxrIntf->m_callIdx != (int)callIdx) continue;

				if (pCxrCall->m_dest == "auto")
					cxrPostReg.Append("%sr_%s_%sAvlCntZero%s",
					pSeparator,
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
				else {
					cxrPostReg.Append("%sr_%s_%sAvlCntZero%s",
						pSeparator,
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), indexStr.c_str(),
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), indexStr.c_str());
					break;
				}

				pSeparator = " || ";
			}
			cxrPostReg.Append(";\n");

			if (pCxrCall->m_dest == "auto")
				break;
		}

		cxrPostReg.Append("#\telse\n");

		for (int replIdx = 0; replIdx < replCnt; replIdx += 1) {
			string indexStr = bDestAuto ? "" : VA("[%d]", replIdx);

			if (htIdW == 0) {
				cxrPostReg.Append("\t\tc_SendCallBusy_%s%s = r_asyncCall%s_rtnCntFull%s",
					callName.c_str(), indexStr.c_str(),
					callName.c_str(), indexStr.c_str());
			} else {
				cxrPostReg.Append("\t\tc_SendCallBusy_%s%s = r_t%d_asyncCall%s_rtnCntFull%s",
					callName.c_str(), indexStr.c_str(),
					mod.m_execStg, callName.c_str(), indexStr.c_str());
			}

			const char * pSeparator = "\n\t\t\t|| ";
			for (size_t intfIdx = 0; intfIdx < pModInst->m_cxrIntfList.size(); intfIdx += 1) {
				CCxrIntf * pCxrIntf = pModInst->m_cxrIntfList[intfIdx];

				if (pCxrIntf->m_cxrDir == CxrIn) continue;
				if (!pCxrIntf->IsCallOrXfer()) continue;
				if (pCxrIntf->m_callIdx != (int)callIdx) continue;

				if (pCxrCall->m_dest == "auto")
					cxrPostReg.Append("%sr_%s_%sAvlCntZero%s",
					pSeparator,
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
				else {
					cxrPostReg.Append("%sr_%s_%sAvlCntZero%s",
						pSeparator,
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), indexStr.c_str(),
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), indexStr.c_str());
					break;
				}
				pSeparator = " || ";
			}
			cxrPostReg.Append("\n\t\t\t|| (%s_RND_RETRY && !!(g_rndRetry() & 1));\n", mod.m_modName.Upper().c_str());

			if (pCxrCall->m_dest == "auto")
				break;
		}
		cxrPostReg.Append("#\tendif\n");

		string paramStr = bDestAuto ? "" : VA("ht_uint%d%s destId", replCntW == 0 ? 1 : replCntW, replCntW == 0 ? " ht_noload" : "");
		string indexStr = bDestAuto ? "" : "[destId]";

		g_appArgs.GetDsnRpt().AddLevel("bool SendCallBusy_%s(%s)\n",
			pCxrCall->m_modEntry.c_str(), paramStr.c_str());
		g_appArgs.GetDsnRpt().EndLevel();

		m_cxrFuncDecl.Append("\tbool SendCallBusy_%s(%s);\n",
			pCxrCall->m_modEntry.c_str(), paramStr.c_str());
		m_cxrMacros.Append("bool CPers%s%s::SendCallBusy_%s(%s)\n",
			unitNameUc.c_str(), pModInst->m_instName.Uc().c_str(),
			pCxrCall->m_modEntry.c_str(), paramStr.c_str());
		m_cxrMacros.Append("{\n");

		m_cxrMacros.Append("\tassert_msg(r_t%d_htValid, \"Runtime check failed in CPers%s::SendCallBusy_%s()"
			" - thread is not valid\");\n", mod.m_execStg, pModInst->m_instName.Uc().c_str(), callName.c_str());
		m_cxrMacros.Append("\n");

		m_cxrMacros.Append("\tc_SendCallAvail_%s%s = !c_SendCallBusy_%s%s;\n",
			callName.c_str(), indexStr.c_str(),
			callName.c_str(), indexStr.c_str());
		m_cxrMacros.Append("\treturn c_SendCallBusy_%s%s;\n",
			callName.c_str(), indexStr.c_str());

		m_cxrMacros.Append("}\n");
		m_cxrMacros.Append("\n");

		// Generate Async Call Function

		paramStr = bDestAuto ? "" : VA(", ht_uint%d%s destId", replCntW == 0 ? 1 : replCntW, replCntW == 0 ? " ht_noload" : "");

		g_appArgs.GetDsnRpt().AddLevel("void SendCallFork_%s(ht_uint%d rtnInstr%s",
			pCxrCall->m_modEntry.c_str(), mod.m_instrW, paramStr.c_str());
		m_cxrFuncDecl.Append("\tvoid SendCallFork_%s(ht_uint%d rtnInstr%s",
			callName.c_str(), mod.m_instrW, paramStr.c_str());
		m_cxrMacros.Append("void CPers%s%s::SendCallFork_%s(ht_uint%d rtnInstr%s",
			unitNameUc.c_str(), pModInst->m_instName.Uc().c_str(),
			callName.c_str(), mod.m_instrW, paramStr.c_str());

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
			" - thread is not valid\");\n", mod.m_execStg, pModInst->m_instName.Uc().c_str(), callName.c_str());
		m_cxrMacros.Append("\tassert_msg(c_SendCallAvail_%s%s, \"Runtime check failed in CPers%s::SendCallFork_%s()"
			" - expected SendCallBusy_%s() to be called and not busy\");\n",
			callName.c_str(), indexStr.c_str(), pModInst->m_instName.Uc().c_str(), callName.c_str(), callName.c_str());
		m_cxrMacros.Append("\n");

		bool bReplIntf = false;
		size_t callIntfIdx = 0;
		for (size_t intfIdx = 0; intfIdx < pModInst->m_cxrIntfList.size(); intfIdx += 1) {
			CCxrIntf * pCxrIntf = pModInst->m_cxrIntfList[intfIdx];

			if (pCxrIntf->m_cxrDir == CxrIn) continue;
			if (!pCxrIntf->IsCallOrXfer()) continue;
			if (pCxrIntf->m_callIdx != (int)callIdx) continue;

			callIntfIdx = intfIdx;

			if (pCxrIntf->m_instCnt == 1)
				m_cxrMacros.Append("\tc_%s_%sRdy = true;\n",
				pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName());
			else {
				bReplIntf = true;

				if (pCxrCall->m_dest == "auto")
					m_cxrMacros.Append("\tc_%s_%sRdy%s = r_%sCall_roundRobin == %dUL;\n",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
					pCxrCall->m_callName.c_str(), pCxrIntf->GetPortReplId());
				else
					m_cxrMacros.Append("\tc_%s_%sRdy%s = destId == %dUL;\n",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
					pCxrIntf->GetPortReplId());
			}

			if (pCxrCall->m_bXfer) {
				if (pCxrCall->m_pGroup->m_rtnSelW > 0)
					m_cxrMacros.Append("\tc_%s_%s%s.m_rtnSel = r_t%d_htPriv.m_rtnSel;\n",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
					mod.m_execStg);

				if (pCxrCall->m_pGroup->m_rtnHtIdW > 0)
					m_cxrMacros.Append("\tc_%s_%s%s.m_rtnHtId = r_t%d_htPriv.m_rtnHtId;\n",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
					mod.m_execStg);

				if (pCxrCall->m_pGroup->m_rtnInstrW > 0)
					m_cxrMacros.Append("\tc_%s_%s%s.m_rtnInstr = r_t%d_htPriv.m_rtnInstr;\n",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
					mod.m_execStg);
			} else {
				if (pCxrCall->m_pGroup->m_htIdW.AsInt() > 0)
					m_cxrMacros.Append("\tc_%s_%s%s.m_rtnHtId = r_t%d_htId;\n",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
					mod.m_execStg);

				if (mod.m_instrW > 0)
					m_cxrMacros.Append("\tc_%s_%s%s.m_rtnInstr = rtnInstr;\n",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());

				if (pCxrCall->m_bCallFork || pCxrCall->m_bXferFork) {
					m_cxrMacros.Append("\tc_%s_%s%s.m_rtnJoin = true;\n",
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
				}
			}

			for (size_t fldIdx = 0; fldIdx < pCxrEntry->m_paramList.size(); fldIdx += 1) {
				CField * pField = pCxrEntry->m_paramList[fldIdx];

				m_cxrMacros.Append("\tc_%s_%s%s.m_%s = %s;\n",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
					pField->m_name.c_str(), pField->m_name.c_str());
			}
		}
		m_cxrMacros.Append("\n");

		if (bReplIntf && pCxrCall->m_dest == "auto") {
			m_cxrMacros.Append("\tc_%sCall_roundRobin = (r_%sCall_roundRobin + 1U) %% %dU;\n",
				pCxrCall->m_callName.c_str(), pCxrCall->m_callName.c_str(), replCnt);
			m_cxrMacros.Append("\n");
		}

		indexStr = bDestAuto ? "" : "[destId]";
		if (htIdW == 0) {
			m_cxrMacros.Append("\t// Should not happen - verify generated code properly limits outstanding calls\n");
			m_cxrMacros.Append("\tassert(c_asyncCall%s_rtnCnt%s < 0x%x);\n",
				pCxrCall->m_modEntry.c_str(), indexStr.c_str(),
				(1 << forkCntW) - 1);
			m_cxrMacros.Append("\tc_asyncCall%s_rtnCntFull%s = c_asyncCall%s_rtnCnt%s == 0x%x;\n",
				pCxrCall->m_modEntry.c_str(), indexStr.c_str(),
				pCxrCall->m_modEntry.c_str(), indexStr.c_str(),
				(1 << forkCntW) - 2);
			m_cxrMacros.Append("\tc_asyncCall%s_rtnCnt%s += 1u;\n",
				pCxrCall->m_modEntry.c_str(), indexStr.c_str());
		} else {
			m_cxrMacros.Append("\tif (r_t%d_htId == r_t%d_htId && r_t%d_htValid)\n",
				mod.m_execStg - 1, mod.m_execStg, mod.m_execStg - 1);
			m_cxrMacros.Append("\t\tc_t%d_asyncCall%s_rtnCntFull%s = m_asyncCall%s_rtnCnt%s.read_mem() == 0x%x;\n",
				mod.m_execStg - 1, pCxrCall->m_modEntry.c_str(), indexStr.c_str(),
				pCxrCall->m_modEntry.c_str(), indexStr.c_str(),
				(1 << forkCntW) - 2);
			m_cxrMacros.Append("\n");

			m_cxrMacros.Append("\t// Should not happen - verify generated code properly limits outstanding calls\n");
			m_cxrMacros.Append("\tassert(m_asyncCall%s_rtnCnt%s.read_mem() < 0x%x);\n",
				pCxrCall->m_modEntry.c_str(), indexStr.c_str(),
				(1 << forkCntW) - 1);
			m_cxrMacros.Append("\tm_asyncCall%s_rtnCnt%s.write_mem(m_asyncCall%s_rtnCnt%s.read_mem() + 1u);\n",
				pCxrCall->m_modEntry.c_str(), indexStr.c_str(),
				pCxrCall->m_modEntry.c_str(), indexStr.c_str());
			m_cxrMacros.Append("\tm_asyncCall%s_rtnCntFull%s.write_mem(m_asyncCall%s_rtnCnt%s.read_mem() == 0x%x);\n",
				pCxrCall->m_modEntry.c_str(), indexStr.c_str(),
				pCxrCall->m_modEntry.c_str(), indexStr.c_str(),
				(1 << forkCntW) - 2);
		}

		if (g_appArgs.IsInstrTraceEnabled()) {
			m_cxrMacros.Append("\n");
			m_cxrMacros.Append("#\tifndef _HTV\n");
			m_cxrMacros.Append("\tif (Ht::g_instrTraceFp)\n");
			m_cxrMacros.Append("\t\tfprintf(Ht::g_instrTraceFp, \"Send%s_%s(",
				pCxrCall->m_bXfer ? "Transfer" : "Call", pCxrCall->m_modEntry.c_str());

			char const * pSeparater = "";
			for (size_t fldIdx = 0; fldIdx < pCxrEntry->m_paramList.size(); fldIdx += 1) {
				CField * pField = pCxrEntry->m_paramList[fldIdx];

				bool bBasicType;
				CTypeDef * pTypeDef;
				string type = pField->m_pType->m_typeName;
				while (!(bBasicType = IsBaseType(type))) {
					if (!(pTypeDef = FindTypeDef(pField->m_pType->m_typeName)))
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
				string type = pField->m_pType->m_typeName;
				while (!(bBasicType = IsBaseType(type))) {
					if (!(pTypeDef = FindTypeDef(pField->m_pType->m_typeName)))
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
		indexStr = bDestAuto ? "" : "[destId]";

		g_appArgs.GetDsnRpt().AddLevel("void RecvReturnPause_%s(ht_uint%d rsmInstr%s)\n",
			pCxrCall->m_modEntry.c_str(), mod.m_instrW, paramStr.c_str());
		g_appArgs.GetDsnRpt().EndLevel();

		m_cxrFuncDecl.Append("\tvoid RecvReturnPause_%s(ht_uint%d rsmInstr%s);\n",
			pCxrCall->m_modEntry.c_str(), mod.m_instrW, paramStr.c_str());
		m_cxrMacros.Append("void CPers%s%s::RecvReturnPause_%s(ht_uint%d rsmInstr%s)\n",
			unitNameUc.c_str(), pModInst->m_instName.Uc().c_str(),
			pCxrCall->m_modEntry.c_str(), mod.m_instrW, paramStr.c_str());
		m_cxrMacros.Append("{\n");

		m_cxrMacros.Append("\tassert_msg(r_t%d_htValid, \"Runtime check failed in CPers%s::RecvReturnPause_%s()"
			" - thread is not valid\");\n", mod.m_execStg, pModInst->m_instName.Uc().c_str(), pCxrCall->m_modEntry.c_str());
		m_cxrMacros.Append("\n");

		if (htIdW == 0) {
			m_cxrMacros.Append("\tassert_msg(c_t%d_htCtrl == HT_INVALID, \"Runtime check failed in CPers%s::RecvReturnPause_%s()"
				" - an Ht control routine was already called\");\n", mod.m_execStg, pModInst->m_instName.Uc().c_str(), pCxrCall->m_modEntry.c_str());
			m_cxrMacros.Append("\tassert (!c_asyncCall%s_waiting%s);\n",
				pCxrCall->m_modEntry.c_str(), indexStr.c_str());
			m_cxrMacros.Append("\tif (c_asyncCall%s_rtnCnt%s == 0)\n",
				pCxrCall->m_modEntry.c_str(), indexStr.c_str());
			m_cxrMacros.Append("\t\tHtContinue(rsmInstr);\n");
			m_cxrMacros.Append("\telse {\n");
			m_cxrMacros.Append("\t\tc_asyncCall%s_waiting%s = true;\n",
				pCxrCall->m_modEntry.c_str(), indexStr.c_str());
			m_cxrMacros.Append("\t\tc_asyncCall%s_rsmInstr%s = rsmInstr;\n",
				pCxrCall->m_modEntry.c_str(), indexStr.c_str());
			m_cxrMacros.Append("\t\tc_t%d_htCtrl = HT_PAUSE;\n", mod.m_execStg);
			m_cxrMacros.Append("\t}\n");
		} else {
			CCxrIntf * pCxrIntf = pModInst->m_cxrIntfList[callIntfIdx];

			m_cxrMacros.Append("\tassert_msg(c_t%d_htCtrl == HT_INVALID, \"Runtime check failed in CPers%s::RecvReturnPause_%s()"
				" - an Ht control routine was already called\");\n", mod.m_execStg, pModInst->m_instName.Uc().c_str(), pCxrCall->m_modEntry.c_str());

			m_cxrMacros.Append("\tassert (");

			if (pCxrCall->m_dest == "auto") {
				m_cxrMacros.Append("c_%s_%sRdy%s || ",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), indexStr.c_str());
			} else {
				for (size_t intfIdx = 0; intfIdx < pModInst->m_cxrIntfList.size(); intfIdx += 1) {
					CCxrIntf * pCxrIntf = pModInst->m_cxrIntfList[intfIdx];

					if (pCxrIntf->m_cxrDir == CxrIn) continue;
					if (!pCxrIntf->IsCallOrXfer()) continue;
					if (pCxrIntf->m_callIdx != (int)callIdx) continue;

					m_cxrMacros.Append("c_%s_%sRdy%s || ",
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
				}
			}
			m_cxrMacros.Append("!m_asyncCall%s_waiting%s.read_mem());\n",
				pCxrCall->m_modEntry.c_str(), indexStr.c_str());

			m_cxrMacros.Append("\tif (");

			if (pCxrCall->m_dest == "auto") {
				for (size_t intfIdx = 0; intfIdx < pModInst->m_cxrIntfList.size(); intfIdx += 1) {
					CCxrIntf * pCxrIntf = pModInst->m_cxrIntfList[intfIdx];

					if (pCxrIntf->m_cxrDir == CxrIn) continue;
					if (!pCxrIntf->IsCallOrXfer()) continue;
					if (pCxrIntf->m_callIdx != (int)callIdx) continue;

					m_cxrMacros.Append("!c_%s_%sRdy%s && ",
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
				}
			} else {
				m_cxrMacros.Append("!c_%s_%sRdy%s && ",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), indexStr.c_str());
			}

			m_cxrMacros.Append("m_asyncCall%s_rtnCnt%s.read_mem() == 0) {\n",
				pCxrCall->m_modEntry.c_str(), indexStr.c_str());

			m_cxrMacros.Append("\t\tHtContinue(rsmInstr);\n");
			m_cxrMacros.Append("\t} else {\n");
			m_cxrMacros.Append("\t\tm_asyncCall%s_waiting%s.write_mem(true);\n",
				pCxrCall->m_modEntry.c_str(), indexStr.c_str());
			m_cxrMacros.Append("\t\tm_asyncCall%s_rsmInstr%s.write_mem(rsmInstr);\n",
				pCxrCall->m_modEntry.c_str(), indexStr.c_str());
			m_cxrMacros.Append("\t\tc_t%d_htCtrl = HT_PAUSE;\n", mod.m_execStg);
			m_cxrMacros.Append("\t}\n");
		}

		m_cxrMacros.Append("}\n");
		m_cxrMacros.Append("\n");

		// Generate AsyncCallJoin function

		paramStr = bDestAuto ? "" : VA("ht_uint%d%s destId", replCntW == 0 ? 1 : replCntW, replCntW == 0 ? " ht_noload" : "");
		indexStr = bDestAuto ? "" : "[destId]";

		g_appArgs.GetDsnRpt().AddLevel("void RecvReturnJoin_%s(%s)\n",
			pCxrCall->m_modEntry.c_str(), paramStr.c_str());
		g_appArgs.GetDsnRpt().EndLevel();

		m_cxrFuncDecl.Append("\tvoid RecvReturnJoin_%s(%s);\n",
			pCxrCall->m_modEntry.c_str(), paramStr.c_str());
		m_cxrMacros.Append("void CPers%s%s::RecvReturnJoin_%s(%s)\n",
			unitNameUc.c_str(), pModInst->m_instName.Uc().c_str(),
			pCxrCall->m_modEntry.c_str(), paramStr.c_str());
		m_cxrMacros.Append("{\n");

		m_cxrMacros.Append("\tassert_msg(r_t%d_htValid, \"Runtime check failed in CPers%s::RecvReturnJoin_%s()"
			" - thread is not valid\");\n", mod.m_execStg, pModInst->m_instName.Uc().c_str(), pCxrCall->m_modEntry.c_str());
		m_cxrMacros.Append("\n");

		if (htIdW == 0) {
			m_cxrMacros.Append("\t// verify that an async call is outstanding\n");
			m_cxrMacros.Append("\tassert (r_asyncCall%s_rtnCnt%s > 0);\n",
				pCxrCall->m_modEntry.c_str(), indexStr.c_str());
			m_cxrMacros.Append("\tc_asyncCall%s_rtnCntFull%s = false;\n",
				pCxrCall->m_modEntry.c_str(), indexStr.c_str());
			m_cxrMacros.Append("\tc_asyncCall%s_rtnCnt%s -= 1u;\n",
				pCxrCall->m_modEntry.c_str(), indexStr.c_str());
			m_cxrMacros.Append("\n");
			m_cxrMacros.Append("\tassert_msg(c_t%d_htCtrl == HT_INVALID, \"Runtime check failed in CPers%s::RecvReturnJoin_%s()"
				" - an Ht control routine was already called\");\n", mod.m_execStg, pModInst->m_instName.Uc().c_str(), pCxrCall->m_modEntry.c_str());
			m_cxrMacros.Append("\tif (c_asyncCall%s_rtnCnt%s == 0 && r_asyncCall%s_waiting%s) {\n",
				pCxrCall->m_modEntry.c_str(), indexStr.c_str(),
				pCxrCall->m_modEntry.c_str(), indexStr.c_str());
			m_cxrMacros.Append("\t\tc_asyncCall%s_waiting%s = false;\n",
				pCxrCall->m_modEntry.c_str(), indexStr.c_str());
			m_cxrMacros.Append("\t\tc_t%d_htNextInstr = r_asyncCall%s_rsmInstr%s;\n",
				mod.m_execStg, pCxrCall->m_modEntry.c_str(), indexStr.c_str());
			m_cxrMacros.Append("\t\tc_t%d_htCtrl = HT_JOIN_AND_CONT;\n", mod.m_execStg);
			m_cxrMacros.Append("\t} else\n");
			m_cxrMacros.Append("\t\tc_t%d_htCtrl = HT_JOIN;\n", mod.m_execStg);
		} else {
			m_cxrMacros.Append("\t// verify that an async call is outstanding\n");
			m_cxrMacros.Append("\tassert (m_asyncCall%s_rtnCnt%s.read_mem() > 0);\n",
				pCxrCall->m_modEntry.c_str(), indexStr.c_str());
			m_cxrMacros.Append("\tm_asyncCall%s_rtnCnt%s.write_mem(m_asyncCall%s_rtnCnt%s.read_mem() - 1u);\n",
				pCxrCall->m_modEntry.c_str(), indexStr.c_str(),
				pCxrCall->m_modEntry.c_str(), indexStr.c_str());
			m_cxrMacros.Append("\tm_asyncCall%s_rtnCntFull%s.write_mem(false);\n",
				pCxrCall->m_modEntry.c_str(), indexStr.c_str());
			m_cxrMacros.Append("\n");
			m_cxrMacros.Append("\tif (r_t%d_htId == r_t%d_htId && r_t%d_htValid)\n",
				mod.m_execStg - 1, mod.m_execStg, mod.m_execStg - 1);
			m_cxrMacros.Append("\t\tc_t%d_asyncCall%s_rtnCntFull%s = false;\n",
				mod.m_execStg - 1, pCxrCall->m_modEntry.c_str(), indexStr.c_str());
			m_cxrMacros.Append("\n");

			m_cxrMacros.Append("\tassert_msg(c_t%d_htCtrl == HT_INVALID, \"Runtime check failed in CPers%s::RecvReturnJoin_%s()"
				" - an Ht control routine was already called\");\n", mod.m_execStg, pModInst->m_instName.Uc().c_str(), pCxrCall->m_modEntry.c_str());
			m_cxrMacros.Append("\tif (m_asyncCall%s_rtnCnt%s.read_mem() == 1 && m_asyncCall%s_waiting%s.read_mem()) {\n",
				pCxrCall->m_modEntry.c_str(), indexStr.c_str(),
				pCxrCall->m_modEntry.c_str(), indexStr.c_str());
			m_cxrMacros.Append("\t\tm_asyncCall%s_waiting%s.write_mem(false);\n",
				pCxrCall->m_modEntry.c_str(), indexStr.c_str());
			m_cxrMacros.Append("\t\tc_t%d_htNextInstr = m_asyncCall%s_rsmInstr%s.read_mem();\n",
				mod.m_execStg, pCxrCall->m_modEntry.c_str(), indexStr.c_str());
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

	for (size_t intfIdx = 0; intfIdx < pModInst->m_cxrIntfList.size(); intfIdx += 1) {
		CCxrIntf * pCxrIntf = pModInst->m_cxrIntfList[intfIdx];

		if (pCxrIntf->m_cxrDir == CxrOut) continue;

		HtdFile::EClkRate srcClkRate = pCxrIntf->m_pSrcModInst->m_pMod->m_clkRate;
		CHtCode & srcCxrT0Stage = srcClkRate == eClk1x ? m_cxrT0Stage1x : m_cxrT0Stage2x;

		if (pCxrIntf->GetQueDepthW() == 0) {

			m_cxrRegDecl.Append("\tbool r_t1_%s_%sRdy%s;\n",
				pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());

			if (mod.m_clkRate != srcClkRate && srcClkRate == eClk2x && !pCxrIntf->IsCallOrXfer()) {

				m_cxrRegDecl.Append("\tsc_signal<bool> r_%s_%sRdy_2x%s;\n",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());

				m_cxrRegDecl.Append("\tsc_signal<C%s_%sIntf> r_%s_%s_2x%s;\n",
					pCxrIntf->GetSrcToDstUc(), pCxrIntf->GetIntfName(),
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());

				if (pCxrIntf->GetPortReplCnt() <= 1)
					cxrT0Stage.Append("\tbool c_t0_%s_%sRdy = i_%s_%sRdy.read()\n\t\t|| r_%s_%sRdy_2x.read() || r_t1_%s_%sRdy;\n",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(),
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(),
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(),
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName());
				else {
					if (pCxrIntf->GetPortReplId() == 0)
						cxrT0Stage.Append("\tbool c_t0_%s_%sRdy%s;\n",
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());

					cxrT0Stage.Append("\tc_t0_%s_%sRdy%s = i_%s_%sRdy%s.read()\n\t\t|| r_%s_%sRdy_2x%s.read() || r_t1_%s_%sRdy%s;\n",
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
				}

				m_cxrT0Stage2x.Append("\tr_%s_%sRdy_2x%s = i_%s_%sRdy%s;\n",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
			} else {
				if (pCxrIntf->GetPortReplCnt() <= 1)
					cxrT0Stage.Append("\tbool c_t0_%s_%sRdy = i_%s_%sRdy.read() || r_t1_%s_%sRdy;\n",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(),
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(),
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName());
				else {
					if (pCxrIntf->GetPortReplId() == 0)
						cxrT0Stage.Append("\tbool c_t0_%s_%sRdy%s;\n",
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());

					cxrT0Stage.Append("\tc_t0_%s_%sRdy%s = i_%s_%sRdy%s.read() || r_t1_%s_%sRdy%s;\n",
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
				}
			}

			if (pCxrIntf->GetPortReplCnt() <= 1)
				cxrT0Stage.Append("\tbool c_%s_%sAvl = false;\n",
				pCxrIntf->GetPortNameDstToSrcLc(), pCxrIntf->GetIntfName());
			else {
				if (pCxrIntf->GetPortReplId() == 0)
					cxrT0Stage.Append("\tbool c_%s_%sAvl%s;\n",
					pCxrIntf->GetPortNameDstToSrcLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());
				cxrT0Stage.Append("\tc_%s_%sAvl%s = false;\n",
					pCxrIntf->GetPortNameDstToSrcLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
			}

			cxrT0Stage.NewLine();
			srcCxrT0Stage.NewLine();

			if (pCxrIntf->m_bCxrIntfFields) {
				if (pCxrIntf->GetPortReplCnt() <= 1)
					cxrT0Stage.Append("\tC%s_%s c_t0_%s_%s = r_t1_%s_%s;\n",
					pCxrIntf->GetSrcToDstUc(), pCxrIntf->GetIntfName(),
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(),
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName());
				else {
					if (pCxrIntf->GetPortReplId() == 0)
						cxrT0Stage.Append("\tC%s_%s c_t0_%s_%s%s;\n",
						pCxrIntf->GetSrcToDstUc(), pCxrIntf->GetIntfName(),
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());

					cxrT0Stage.Append("\tc_t0_%s_%s%s = r_t1_%s_%s%s;\n",
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
				}

				m_cxrRegDecl.Append("\tC%s_%s r_t1_%s_%s%s;\n",
					pCxrIntf->GetSrcToDstUc(), pCxrIntf->GetIntfName(),
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());

				cxrT0Stage.Append("\tif (i_%s_%sRdy%s.read())\n",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());

				cxrT0Stage.Append("\t\tc_t0_%s_%s%s = i_%s_%s%s;\n",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());

				if (mod.m_clkRate != srcClkRate && srcClkRate == eClk2x && !pCxrIntf->IsCallOrXfer()) {
					cxrT0Stage.Append("\telse if (r_%s_%sRdy_2x%s.read())\n",
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());

					cxrT0Stage.Append("\t\tc_t0_%s_%s%s = r_%s_%s_2x%s;\n",
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());

					m_cxrT0Stage2x.Append("\tr_%s_%s_2x%s = i_%s_%s%s;\n",
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
				}
				m_cxrT0Stage1x.NewLine();
				m_cxrT0Stage2x.NewLine();
			}

		} else {
			if (pCxrIntf->m_bCxrIntfFields) {

				if (pCxrIntf->GetPortReplCnt() < 2 || pCxrIntf->GetPortReplId() == 0)
					m_cxrRegDecl.Append("\tht_dist_que<C%s_%s, %d> m_%s_%sQue%s;\n",
					pCxrIntf->GetSrcToDstUc(), pCxrIntf->GetIntfName(),
					pCxrIntf->GetQueDepthW(),
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());

				if (pCxrIntf->IsCallOrXfer()) {
					cxrT0Stage.Append("\tif (i_%s_%sRdy%s.read())\n",
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());

					cxrT0Stage.Append("\t\tm_%s_%sQue%s.push(i_%s_%s%s);\n",
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
				} else {
					srcCxrT0Stage.Append("\tif (i_%s_%sRdy%s.read())\n",
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());

					srcCxrT0Stage.Append("\t\tm_%s_%sQue%s.push(i_%s_%s%s);\n",
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
				}

				if (pCxrIntf->GetPortReplCnt() <= 1)
					cxrT0Stage.Append("\tbool c_%s_%sAvl = false;\n",
					pCxrIntf->GetPortNameDstToSrcLc(), pCxrIntf->GetIntfName());
				else {
					if (pCxrIntf->GetPortReplId() == 0)
						cxrT0Stage.Append("\tbool c_%s_%sAvl%s;\n",
						pCxrIntf->GetPortNameDstToSrcLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());
					cxrT0Stage.Append("\tc_%s_%sAvl%s = false;\n",
						pCxrIntf->GetPortNameDstToSrcLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
				}

				if (mod.m_clkRate != eClk1x || pCxrIntf->m_pSrcModInst->m_pMod->m_clkRate != eClk1x) {
					if (pCxrIntf->GetPortReplCnt() <= 1) {
						cxrT0Stage.Append("\tbool c_%s_%sQueEmpty%s =\n",
							pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
						cxrT0Stage.Append("\t\tr_%s_%sQueEmpty%s && m_%s_%sQue%s.empty();\n",
							pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
							pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
					} else {
						if (pCxrIntf->GetPortReplId() == 0)
							cxrT0Stage.Append("\tbool c_%s_%sQueEmpty%s;\n",
							pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());
						cxrT0Stage.Append("\tc_%s_%sQueEmpty%s =\n",
							pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
						cxrT0Stage.Append("\t\tr_%s_%sQueEmpty%s && m_%s_%sQue%s.empty();\n",
							pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
							pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
					}
					if (pCxrIntf->GetPortReplCnt() <= 1) {
						cxrT0Stage.Append("\tC%s_%s c_%s_%sQueFront%s = r_%s_%sQueEmpty%s ?\n",
							pCxrIntf->GetSrcToDstUc(), pCxrIntf->GetIntfName(),
							pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
							pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
						cxrT0Stage.Append("\t\tm_%s_%sQue%s.front() : r_%s_%sQueFront%s;\n",
							pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
							pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
					} else {
						if (pCxrIntf->GetPortReplId() == 0)
							cxrT0Stage.Append("\tC%s_%s c_%s_%sQueFront%s;\n",
							pCxrIntf->GetSrcToDstUc(), pCxrIntf->GetIntfName(),
							pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());
						cxrT0Stage.Append("\tc_%s_%sQueFront%s = r_%s_%sQueEmpty%s ?\n",
							pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
							pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
						cxrT0Stage.Append("\t\tm_%s_%sQue%s.front() : r_%s_%sQueFront%s;\n",
							pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
							pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
					}

					cxrT0Stage.Append("\tif (r_%s_%sQueEmpty%s && !m_%s_%sQue%s.empty())\n",
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
					cxrT0Stage.Append("\t\tm_%s_%sQue%s.pop();\n",
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
					cxrT0Stage.Append("\n");
				}

				if (mod.m_clkRate == srcClkRate || pCxrIntf->IsCallOrXfer())
					cxrReg.Append("\tm_%s_%sQue%s.clock(%s);\n",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(), reset.c_str());

				else {
					if (srcClkRate == eClk1x) {
						cxrReg.Append("\tm_%s_%sQue%s.pop_clock(%s);\n",
							pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(), reset.c_str());
						m_cxrReg1x.Append("\tm_%s_%sQue%s.push_clock(%s);\n",
							pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(), reset.c_str());
					} else {
						cxrReg.Append("\tm_%s_%sQue%s.pop_clock(%s);\n",
							pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(), reset.c_str());
						m_cxrReg2x.Append("\tm_%s_%sQue%s.push_clock(%s);\n",
							pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(), reset.c_str());
					}
				}

			} else {
				m_cxrRegDecl.Append("\tsc_uint<%d+1> r_%s_%sCnt%s;\n",
					pCxrIntf->GetQueDepthW(),
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());

				if (pCxrIntf->GetPortReplCnt() <= 1)
					srcCxrT0Stage.Append("\tsc_uint<%d+1> c_%s_%sCnt = r_%s_%sCnt;\n",
					pCxrIntf->GetQueDepthW(),
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(),
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName());
				else {
					if (pCxrIntf->GetPortReplId() == 0)
						srcCxrT0Stage.Append("\tsc_uint<%d+1> c_%s_%sCnt%s;\n",
						pCxrIntf->GetQueDepthW(),
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());

					srcCxrT0Stage.Append("\tc_%s_%sCnt%s = r_%s_%sCnt%s;\n",
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
				}

				if (pCxrIntf->IsCallOrXfer()) {
					cxrT0Stage.Append("\tif (i_%s_%sRdy%s.read())\n",
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());

					cxrT0Stage.Append("\t\tc_%s_%sCnt%s += 1u;\n",
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
				} else {
					srcCxrT0Stage.Append("\tif (i_%s_%sRdy%s.read())\n",
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());

					srcCxrT0Stage.Append("\t\tc_%s_%sCnt%s += 1u;\n",
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
				}
				if (pCxrIntf->GetPortReplCnt() <= 1)
					cxrT0Stage.Append("\tbool c_%s_%sAvl = false;\n",
					pCxrIntf->GetPortNameDstToSrcLc(), pCxrIntf->GetIntfName());
				else {
					if (pCxrIntf->GetPortReplId() == 0)
						cxrT0Stage.Append("\tbool c_%s_%sAvl%s;\n",
						pCxrIntf->GetPortNameDstToSrcLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());
					cxrT0Stage.Append("\tc_%s_%sAvl%s = false;\n",
						pCxrIntf->GetPortNameDstToSrcLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
				}

				m_cxrReg1x.Append("\t\tr_%s_%sCnt%s = %s ? (sc_uint<%d+1>)0 : c_%s_%sCnt%s;\n",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(), 
					reset.c_str(),
					pCxrIntf->GetQueDepthW(),
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
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

		CCxrCall * pCxrCall = mod.m_cxrCallList[callIdx];

		int replCnt = 0;
		for (size_t intfIdx = 0; intfIdx < pModInst->m_cxrIntfList.size(); intfIdx += 1) {
			CCxrIntf * pCxrIntf = pModInst->m_cxrIntfList[intfIdx];

			if (pCxrIntf->m_cxrDir == CxrIn) continue;
			if (!pCxrIntf->IsCallOrXfer()) continue;
			if (pCxrIntf->m_callIdx != (int)callIdx) continue;

			replCnt = pCxrIntf->m_instCnt;

			if (pCxrIntf->GetPortReplCnt() <= 1 || pCxrIntf->GetPortReplId() == 0)
				m_cxrRegDecl.Append("\tbool c_%s_%sRdy%s;\n",
				pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());

			cxrTsStage.Append("\tc_%s_%sRdy%s = false;\n",
				pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());

			if (mod.m_bGvIwComp) {
				for (int stg = mod.m_execStg + 2; stg <= mod.m_gvIwCompStg; stg += 1) {
					m_cxrRegDecl.Append("\tbool c_t%d_%s_%sRdy%s;\n",
						stg - 1, pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());

					if (stg == mod.m_execStg + 2) {
						cxrTsStage.Append("\tc_t%d_%s_%sRdy%s = r_%s_%sRdy%s;\n",
							stg - 1, pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
							pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
					} else {
						cxrTsStage.Append("\tc_t%d_%s_%sRdy%s = r_t%d_%s_%sRdy%s;\n",
							stg - 1, pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
							stg - 1, pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
					}
				}
			}

			if (pCxrIntf->m_bCxrIntfFields) {
				if (pCxrIntf->GetPortReplCnt() <= 1 || pCxrIntf->GetPortReplId() == 0)
					m_cxrRegDecl.Append("\tC%s_%sIntf c_%s_%s%s;\n",
					pCxrIntf->GetSrcToDstUc(), pCxrIntf->GetIntfName(),
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());

				if (mod.m_clkRate == eClk1x)
					cxrTsStage.Append("\tc_%s_%s%s = 0;\n",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
				else
					cxrTsStage.Append("\tc_%s_%s%s = r_%s_%s%s;\n",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
			}
		}

		if (replCnt > 1 && pCxrCall->m_dest == "auto") {
			int replIdxW = FindLg2(replCnt - 1);
			m_cxrRegDecl.Append("\tht_uint%d c_%sCall_roundRobin;\n",
				replIdxW, pCxrCall->m_callName.c_str());
			m_cxrRegDecl.Append("\tht_uint%d r_%sCall_roundRobin;\n",
				replIdxW, pCxrCall->m_callName.c_str());

			cxrTsStage.Append("\tc_%sCall_roundRobin = r_%sCall_roundRobin;\n",
				pCxrCall->m_callName.c_str(), pCxrCall->m_callName.c_str());

			iplReg.Append("\tr_%sCall_roundRobin = %s ? (ht_uint%d)0 : c_%sCall_roundRobin;\n",
				pCxrCall->m_callName.c_str(), reset.c_str(), replIdxW, pCxrCall->m_callName.c_str());
		}

		cxrTsStage.Append("\n");
	}

	// initialize outbound return interfaces
	for (size_t rtnIdx = 0; rtnIdx < mod.m_cxrReturnList.size(); rtnIdx += 1) {
		if (!mod.m_bIsUsed) continue;

		for (size_t intfIdx = 0; intfIdx < pModInst->m_cxrIntfList.size(); intfIdx += 1) {
			CCxrIntf * pCxrIntf = pModInst->m_cxrIntfList[intfIdx];

			if (pCxrIntf->m_cxrDir == CxrIn) continue;
			if (pCxrIntf->IsCallOrXfer()) continue;
			if (pCxrIntf->m_rtnIdx != (int)rtnIdx) continue;

			if (pCxrIntf->GetPortReplCnt() <= 1 || pCxrIntf->GetPortReplId() == 0)
				m_cxrRegDecl.Append("\tbool c_%s_%sRdy%s;\n",
				pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());

			cxrTsStage.Append("\tc_%s_%sRdy%s = false;\n",
				pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());

			if (mod.m_bGvIwComp) {
				for (int stg = mod.m_execStg + 2; stg <= mod.m_gvIwCompStg; stg += 1) {
					m_cxrRegDecl.Append("\tbool c_t%d_%s_%sRdy%s;\n",
						stg - 1, pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());

					if (stg == mod.m_execStg + 2) {
						cxrTsStage.Append("\tc_t%d_%s_%sRdy%s = r_%s_%sRdy%s;\n",
							stg - 1, pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
							pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
					} else {
						cxrTsStage.Append("\tc_t%d_%s_%sRdy%s = r_t%d_%s_%sRdy%s;\n",
							stg - 1, pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
							stg - 1, pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
					}
				}
			}

			if (pCxrIntf->m_bCxrIntfFields) {
				if (pCxrIntf->GetPortReplCnt() <= 1 || pCxrIntf->GetPortReplId() == 0)
					m_cxrRegDecl.Append("\tC%s_%sIntf c_%s_%s%s;\n",
					pCxrIntf->GetSrcToDstUc(), pCxrIntf->GetIntfName(),
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());

				cxrTsStage.Append("\tc_%s_%s%s = 0;\n",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
			}
		}

		cxrTsStage.Append("\n");
	}

	///////////////////////////////////////////////////////////////////////
	// Handle register declarations

	for (size_t intfIdx = 0; intfIdx < pModInst->m_cxrIntfList.size(); intfIdx += 1) {
		CCxrIntf * pCxrIntf = pModInst->m_cxrIntfList[intfIdx];

		if (pCxrIntf->m_cxrDir == CxrIn) {
			if (pCxrIntf->GetPortReplCnt() <= 1 || pCxrIntf->GetPortReplId() == 0) {
				HtdFile::EClkRate srcClkRate = pCxrIntf->m_pSrcModInst->m_pMod->m_clkRate;

				if (mod.m_clkRate == srcClkRate || pCxrIntf->IsCallOrXfer())
					m_cxrRegDecl.Append("\tbool r_%s_%sAvl%s;\n",
					pCxrIntf->GetPortNameDstToSrcLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());
				else if (mod.m_clkRate == eClk1x) {
					m_cxrRegDecl.Append("\tsc_signal<bool> r_%s_%sAvl_1x%s;\n",
						pCxrIntf->GetPortNameDstToSrcLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());
					m_cxrRegDecl.Append("\tbool r_%s_%sAvl_2x%s;\n",
						pCxrIntf->GetPortNameDstToSrcLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());
				} else {
					m_cxrRegDecl.Append("\tbool r_%s_%sAvl_1x%s;\n",
						pCxrIntf->GetPortNameDstToSrcLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());
					m_cxrRegDecl.Append("\tsc_signal<ht_uint%d > r_%s_%sAvlCnt_2x%s;\n",
						pCxrIntf->GetQueDepthW() + 1,
						pCxrIntf->GetPortNameDstToSrcLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());
				}
			}

			if (pCxrIntf->GetQueDepthW() > 0 && pCxrIntf->GetPortReplId() == 0) {
				if (pCxrIntf->m_bCxrIntfFields) {
					if (mod.m_clkRate != eClk1x || pCxrIntf->m_pSrcModInst->m_pMod->m_clkRate != eClk1x) {
						m_cxrRegDecl.Append("\tbool r_%s_%sQueEmpty%s;\n",
							pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());
						m_cxrRegDecl.Append("\tC%s_%s r_%s_%sQueFront%s;\n",
							pCxrIntf->GetSrcToDstUc(), pCxrIntf->GetIntfName(),
							pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());
					}
				}
			}

		} else {
			if (pCxrIntf->GetPortReplCnt() <= 1 || pCxrIntf->GetPortReplId() == 0) {
				HtdFile::EClkRate dstClkRate = pCxrIntf->m_pDstModInst->m_pMod->m_clkRate;

				if (mod.m_clkRate == dstClkRate || !pCxrIntf->IsCallOrXfer())
					m_cxrRegDecl.Append("\tbool r_%s_%sRdy%s;\n",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());
				else if (mod.m_clkRate == eClk1x) {
					m_cxrRegDecl.Append("\tsc_signal<bool> r_%s_%sRdy%s;\n",
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());
					m_cxrRegDecl.Append("\tbool r_%s_%sRdy_2x%s;\n",
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());
				} else {
					m_cxrRegDecl.Append("\tsc_signal<bool> r_%s_%sRdy%s;\n",
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());
					if (pCxrIntf->IsXfer())
						m_cxrRegDecl.Append("\tbool r_%s_%sHold%s;\n",
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());
					m_cxrRegDecl.Append("\tbool r_%s_%sRdy_1x%s;\n",
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());
				}

				m_cxrRegDecl.Append("\tht_uint%d r_%s_%sAvlCnt%s;\n",
					pCxrIntf->GetQueDepthW() + 1,
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());

				m_cxrRegDecl.Append("\tbool r_%s_%sAvlCntZero%s;\n",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());

				if (mod.m_clkRate == eClk1x && dstClkRate == eClk2x && pCxrIntf->IsCallOrXfer()) {
					m_cxrRegDecl.Append("\tsc_signal<bool> r_%s_%sAvl_2x1%s;\n",
						pCxrIntf->GetPortNameDstToSrcLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());
					m_cxrRegDecl.Append("\tsc_signal<bool> r_%s_%sAvl_2x2%s;\n",
						pCxrIntf->GetPortNameDstToSrcLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());
				}
			}

			m_cxrAvlCntChk.Append("\t\tassert(r_%s_%sAvlCnt%s == %d);\n",
				pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
				1 << pCxrIntf->GetQueDepthW());
		}
		m_cxrRegDecl.Append("\n");
	}

	for (size_t intfIdx = 0; intfIdx < pModInst->m_cxrIntfList.size(); intfIdx += 1) {
		CCxrIntf * pCxrIntf = pModInst->m_cxrIntfList[intfIdx];

		if (pCxrIntf->m_cxrDir == CxrIn) continue;
		if (!pCxrIntf->m_bCxrIntfFields) continue;

		if (pCxrIntf->GetPortReplCnt() <= 1 || pCxrIntf->GetPortReplId() == 0) {
			HtdFile::EClkRate dstClkRate = pCxrIntf->m_pDstModInst->m_pMod->m_clkRate;

			if (mod.m_clkRate == dstClkRate || !pCxrIntf->IsCallOrXfer())
				m_cxrRegDecl.Append("\tC%s_%sIntf r_%s_%s%s;\n",
				pCxrIntf->GetSrcToDstUc(), pCxrIntf->GetIntfName(),
				pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());
			else if (mod.m_clkRate == eClk1x) {
				m_cxrRegDecl.Append("\tsc_signal<C%s_%sIntf> r_%s_%s%s;\n",
					pCxrIntf->GetSrcToDstUc(), pCxrIntf->GetIntfName(),
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());
				m_cxrRegDecl.Append("\tC%s_%sIntf r_%s_%s_2x%s;\n",
					pCxrIntf->GetSrcToDstUc(), pCxrIntf->GetIntfName(),
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());
			} else {
				m_cxrRegDecl.Append("\tsc_signal<C%s_%sIntf> r_%s_%s%s;\n",
					pCxrIntf->GetSrcToDstUc(), pCxrIntf->GetIntfName(),
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());
				m_cxrRegDecl.Append("\tC%s_%sIntf r_%s_%s_1x%s;\n",
					pCxrIntf->GetSrcToDstUc(), pCxrIntf->GetIntfName(),
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());
			}
		}
	}
	m_cxrRegDecl.Append("\n");

	///////////////////////////////////////////////////////////////////////
	// Handle register assignments

	for (size_t intfIdx = 0; intfIdx < pModInst->m_cxrIntfList.size(); intfIdx += 1) {
		CCxrIntf * pCxrIntf = pModInst->m_cxrIntfList[intfIdx];

		if (pCxrIntf->m_cxrDir == CxrIn) {
			HtdFile::EClkRate srcClkRate = pCxrIntf->m_pSrcModInst->m_pMod->m_clkRate;

			if (mod.m_clkRate == srcClkRate || pCxrIntf->IsCallOrXfer())
				iplReg.Append("\tr_%s_%sAvl%s = !%s && c_%s_%sAvl%s;\n",
				pCxrIntf->GetPortNameDstToSrcLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
				reset.c_str(),
				pCxrIntf->GetPortNameDstToSrcLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
			else if (mod.m_clkRate == eClk1x)
				iplReg.Append("\tr_%s_%sAvl_1x%s = !%s && c_%s_%sAvl%s;\n",
				pCxrIntf->GetPortNameDstToSrcLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
				reset.c_str(),
				pCxrIntf->GetPortNameDstToSrcLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
			else {
				if (pCxrIntf->GetPortReplCnt() <= 1)
					m_cxrPostInstr2x.Append("\tht_uint%d c_%s_%sAvlCnt_2x = r_%s_%sAvlCnt_2x.read() + c_%s_%sAvl\n",
					pCxrIntf->GetQueDepthW() + 1,
					pCxrIntf->GetPortNameDstToSrcLc(), pCxrIntf->GetIntfName(),
					pCxrIntf->GetPortNameDstToSrcLc(), pCxrIntf->GetIntfName(),
					pCxrIntf->GetPortNameDstToSrcLc(), pCxrIntf->GetIntfName());
				else {
					if (pCxrIntf->GetPortReplId() == 0)
						m_cxrPostInstr2x.Append("\tht_uint%d c_%s_%sAvlCnt_2x%s;\n",
						pCxrIntf->GetQueDepthW() + 1,
						pCxrIntf->GetPortNameDstToSrcLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());

					m_cxrPostInstr2x.Append("\tht_uint%d c_%s_%sAvlCnt_2x%s = r_%s_%sAvlCnt_2x%s.read() + c_%s_%sAvl%s\n",
						pCxrIntf->GetQueDepthW() + 1,
						pCxrIntf->GetPortNameDstToSrcLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
						pCxrIntf->GetPortNameDstToSrcLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
						pCxrIntf->GetPortNameDstToSrcLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
				}

				m_cxrPostInstr2x.Append("\t\t- (r_%s_%sAvlCnt_2x%s.read() > 0 && !r_phase);\n",
					pCxrIntf->GetPortNameDstToSrcLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
				iplReg.Append("\tr_%s_%sAvlCnt_2x%s = %s ? (ht_uint%d) 0 : c_%s_%sAvlCnt_2x%s;\n",
					pCxrIntf->GetPortNameDstToSrcLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
					reset.c_str(),
					pCxrIntf->GetQueDepthW() + 1,
					pCxrIntf->GetPortNameDstToSrcLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
			}

			if (pCxrIntf->GetQueDepthW() > 0) {
				if (pCxrIntf->m_bCxrIntfFields) {
					if (mod.m_clkRate != eClk1x || pCxrIntf->m_pSrcModInst->m_pMod->m_clkRate != eClk1x) {
						iplReg.Append("\tr_%s_%sQueEmpty%s = %s || c_%s_%sQueEmpty%s;\n",
							pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
							reset.c_str(),
							pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
						iplReg.Append("\tr_%s_%sQueFront%s = c_%s_%sQueFront%s;\n",
							pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
							pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
						iplReg.Append("\n");
					}
				}
			}

		} else {
			HtdFile::EClkRate dstClkRate = pCxrIntf->m_pDstModInst->m_pMod->m_clkRate;

			if (pCxrIntf->m_bCxrIntfFields)
				iplReg.Append("\tr_%s_%s%s = c_%s_%s%s;\n",
				pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
				pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());

			if (pCxrIntf->GetPortReplCnt() <= 1)
				iplPostInstr.Append("\tht_uint%d c_%s_%sAvlCnt = r_%s_%sAvlCnt\n",
				pCxrIntf->GetQueDepthW() + 1,
				pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(),
				pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName());
			else {
				if (pCxrIntf->GetPortReplId() == 0)
					iplPostInstr.Append("\tht_uint%d c_%s_%sAvlCnt%s;\n",
					pCxrIntf->GetQueDepthW() + 1,
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());

				iplPostInstr.Append("\tc_%s_%sAvlCnt%s = r_%s_%sAvlCnt%s\n",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
			}

			if (dstClkRate == mod.m_clkRate || !pCxrIntf->IsCallOrXfer())
				iplPostInstr.Append("\t\t+ i_%s_%sAvl%s.read() - c_%s_%sRdy%s;\n",
				pCxrIntf->GetPortNameDstToSrcLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
				pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());

			else if (dstClkRate == eClk2x) {
				iplPostInstr.Append("\t\t+ r_%s_%sAvl_2x1%s.read() + r_%s_%sAvl_2x2%s.read() - c_%s_%sRdy%s;\n",
					pCxrIntf->GetPortNameDstToSrcLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
					pCxrIntf->GetPortNameDstToSrcLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());

				m_iplReg2x.Append("\tr_%s_%sAvl_2x2%s = r_%s_%sAvl_2x1%s;\n",
					pCxrIntf->GetPortNameDstToSrcLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
					pCxrIntf->GetPortNameDstToSrcLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
				m_iplReg2x.Append("\tr_%s_%sAvl_2x1%s = i_%s_%sAvl%s.read();\n",
					pCxrIntf->GetPortNameDstToSrcLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
					pCxrIntf->GetPortNameDstToSrcLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
				m_iplReg2x.Append("\n");

			} else
				iplPostInstr.Append("\t\t+ (r_phase & i_%s_%sAvl%s.read()) - c_%s_%sRdy%s;\n",
				pCxrIntf->GetPortNameDstToSrcLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
				pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());

			if (pCxrIntf->GetPortReplCnt() <= 1)
				iplPostInstr.Append("\tbool c_%s_%sAvlCntZero = c_%s_%sAvlCnt == 0;\n",
				pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(),
				pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName());
			else {
				if (pCxrIntf->GetPortReplId() == 0)
					iplPostInstr.Append("\tbool c_%s_%sAvlCntZero%s;\n",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());

				iplPostInstr.Append("\tc_%s_%sAvlCntZero%s = c_%s_%sAvlCnt%s == 0;\n",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
			}

			iplReg.Append("\tr_%s_%sRdy%s = !%s && c_%s_%sRdy%s;\n",
				pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
				reset.c_str(),
				pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());

			if (mod.m_bGvIwComp) {
				for (int stg = mod.m_execStg + 2; stg <= mod.m_gvIwCompStg; stg += 1) {
					m_cxrRegDecl.Append("\tbool r_t%d_%s_%sRdy%s;\n",
						stg, pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());

					iplReg.Append("\tr_t%d_%s_%sRdy%s = c_t%d_%s_%sRdy%s;\n",
						stg, pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
						stg - 1, pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
				}
			}

			if (dstClkRate != mod.m_clkRate && mod.m_clkRate == eClk2x && pCxrIntf->IsCallOrXfer()) {
				if (pCxrIntf->GetPortReplCnt() <= 1)
					iplPostInstr.Append("\tbool c_%s_%sHold = r_%s_%sRdy & r_phase;\n",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(),
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName());
				else {
					if (pCxrIntf->GetPortReplId() == 0)
						iplPostInstr.Append("\tbool c_%s_%sHold%s;\n",
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());

					iplPostInstr.Append("\tc_%s_%sHold%s = r_%s_%sRdy%s & r_phase;\n",
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
				}

				iplPostInstr.Append("\tc_%s_%sRdy%s |= c_%s_%sHold%s;\n",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());

				if (pCxrIntf->IsXfer())
					iplReg.Append("\tr_%s_%sHold%s = c_%s_%sHold%s;\n",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
			}

			int avlCnt = 1ul << pCxrIntf->GetQueDepthW();

			iplReg.Append("\tr_%s_%sAvlCnt%s = %s ? (ht_uint%d)%d : c_%s_%sAvlCnt%s;\n",
				pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
				reset.c_str(),
				pCxrIntf->GetQueDepthW() + 1, avlCnt,
				pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());

			iplReg.Append("\tr_%s_%sAvlCntZero%s = !%s && c_%s_%sAvlCntZero%s;\n",
				pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(), 
				reset.c_str(),
				pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());

			m_cxrPostInstr1x.NewLine();
			m_cxrPostInstr2x.NewLine();
		}
		iplReg.Append("\n");
	}

	///////////////////////////////////////////////////////////////////////
	// Handle output signal assignments

	for (size_t intfIdx = 0; intfIdx < pModInst->m_cxrIntfList.size(); intfIdx += 1) {
		CCxrIntf * pCxrIntf = pModInst->m_cxrIntfList[intfIdx];

		if (pCxrIntf->m_cxrDir == CxrIn) {
			HtdFile::EClkRate srcClkRate = pCxrIntf->m_pSrcModInst->m_pMod->m_clkRate;

			if (mod.m_clkRate == srcClkRate || pCxrIntf->IsCallOrXfer())
				cxrOut.Append("\to_%s_%sAvl%s = r_%s_%sAvl%s;\n",
				pCxrIntf->GetPortNameDstToSrcLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
				pCxrIntf->GetPortNameDstToSrcLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
			else if (mod.m_clkRate == eClk1x) {
				m_cxrReg2x.Append("\tr_%s_%sAvl_2x%s = r_%s_%sAvl_1x%s.read() & r_phase;\n",
					pCxrIntf->GetPortNameDstToSrcLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
					pCxrIntf->GetPortNameDstToSrcLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
				m_cxrOut2x.Append("\to_%s_%sAvl%s = r_%s_%sAvl_2x%s;\n",
					pCxrIntf->GetPortNameDstToSrcLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
					pCxrIntf->GetPortNameDstToSrcLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
			} else {
				m_cxrReg1x.Append("\tr_%s_%sAvl_1x%s = r_%s_%sAvlCnt_2x%s.read() > 0;\n",
					pCxrIntf->GetPortNameDstToSrcLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
					pCxrIntf->GetPortNameDstToSrcLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
				m_cxrOut1x.Append("\to_%s_%sAvl%s = r_%s_%sAvl_1x%s;\n",
					pCxrIntf->GetPortNameDstToSrcLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
					pCxrIntf->GetPortNameDstToSrcLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
			}
		} else {
			HtdFile::EClkRate dstClkRate = pCxrIntf->m_pDstModInst->m_pMod->m_clkRate;

			if (mod.m_clkRate == dstClkRate || !pCxrIntf->IsCallOrXfer()) {
				cxrOut.Append("\to_%s_%sRdy%s = r_%s_%sRdy%s;\n",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());

				if (mod.m_bGvIwComp && pCxrIntf->m_pDstModInst->m_pMod->m_modName.AsStr() != "hif") {
					cxrOut.Append("\to_%s_%sCompRdy%s = r_%s_%sCompRdy%s;\n",
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
				}

				if (pCxrIntf->m_bCxrIntfFields)
					cxrOut.Append("\to_%s_%s%s = r_%s_%s%s;\n",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
			} else if (mod.m_clkRate == eClk2x) {
				m_cxrReg1x.Append("\tr_%s_%sRdy_1x%s = r_%s_%sRdy%s;\n",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());

				m_cxrOut1x.Append("\to_%s_%sRdy%s = r_%s_%sRdy_1x%s;\n",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());

				if (pCxrIntf->m_bCxrIntfFields) {
					m_cxrReg1x.Append("\tr_%s_%s_1x%s = r_%s_%s%s;\n",
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());

					m_cxrOut1x.Append("\to_%s_%s%s = r_%s_%s_1x%s;\n",
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
				}
			} else {
				m_cxrReg2x.Append("\tr_%s_%sRdy_2x%s = r_%s_%sRdy%s & r_phase;\n",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());

				m_cxrOut2x.Append("\to_%s_%sRdy%s = r_%s_%sRdy_2x%s;\n",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());

				if (pCxrIntf->m_bCxrIntfFields) {
					m_cxrReg2x.Append("\tr_%s_%s_2x%s = r_%s_%s%s;\n",
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());

					m_cxrOut2x.Append("\to_%s_%s%s = r_%s_%s_2x%s;\n",
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
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
			if (m_recordList[recordIdx]->m_typeName == pField->m_pType->m_typeName)
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
