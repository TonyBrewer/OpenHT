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

void CDsnInfo::InitAndValidateModRam()
{
	// verify that internal rams have valid read and write stages
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];

		if (!mod.m_bIsUsed) continue;

		for (size_t ramIdx = 0; ramIdx < mod.m_intGblList.size(); ramIdx += 1) {
			CRam &intGbl = *mod.m_intGblList[ramIdx];

			if (!intGbl.m_bShAddr && (intGbl.m_rdStg.AsInt() < 1 || intGbl.m_rdStg.AsInt() > mod.m_stage.m_privWrStg.AsInt()))
				ParseMsg(Error, intGbl.m_lineInfo, "Invalid rdStg parameter, expected range (1-%d)", mod.m_stage.m_privWrStg.AsInt());
			if (intGbl.m_wrStg.AsInt() < 1 || intGbl.m_wrStg.AsInt() > mod.m_stage.m_privWrStg.AsInt())
				ParseMsg(Error, intGbl.m_lineInfo, "Invalid wrStg parameter, expected range (1-%d)", mod.m_stage.m_privWrStg.AsInt());
		}
	}

	// verify that external rams have valid read and write stages
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];

		if (!mod.m_bIsUsed) continue;

		for (size_t ramIdx = 0; ramIdx < mod.m_extRamList.size(); ramIdx += 1) {
			CRam &extRam = mod.m_extRamList[ramIdx];

			if (extRam.m_rdStg.AsInt() < 1 || extRam.m_rdStg.AsInt() > mod.m_stage.m_privWrStg.AsInt())
				ParseMsg(Error, extRam.m_lineInfo, "Invalid rdStg parameter, expected range (1-%d)", mod.m_stage.m_privWrStg.AsInt());
			if (extRam.m_wrStg.AsInt() < 1 || extRam.m_wrStg.AsInt() > mod.m_stage.m_privWrStg.AsInt())
				ParseMsg(Error, extRam.m_lineInfo, "Invalid wrStg parameter, expected range (1-%d)", mod.m_stage.m_privWrStg.AsInt());
		}
	}

	// verify that global rams have exactly one extern=false declaration
	for (size_t modIdx1 = 0; modIdx1 < m_modList.size(); modIdx1 += 1) {
		CModule &mod1 = *m_modList[modIdx1];

		if (!mod1.m_bIsUsed) continue;

		for (size_t ramIdx1 = 0; ramIdx1 < mod1.m_intGblList.size(); ramIdx1 += 1) {
			CRam &intRam1 = *mod1.m_intGblList[ramIdx1];

			// count number of rams with extern=false
			int foundCnt = 0;
			for (size_t modIdx2 = 0; modIdx2 < m_modList.size(); modIdx2 += 1) {
				CModule &mod2 = *m_modList[modIdx2];

				if (!mod2.m_bIsUsed) continue;

				for (size_t ramIdx2 = 0; ramIdx2 < mod2.m_intGblList.size(); ramIdx2 += 1) {
					CRam &intRam2 = *mod2.m_intGblList[ramIdx2];

					if (intRam2.m_gblName == intRam1.m_gblName)
						foundCnt += 1;
				}
			}

			if (foundCnt > 1)
				ParseMsg(Error, intRam1.m_lineInfo, "global variable was declared multiple times with extern=false, '%s'",
					intRam1.m_gblName.c_str());
		}
	}

	// verify that global rams with extern=true have a corresponding declaration with extern=false
	for (size_t modIdx1 = 0; modIdx1 < m_modList.size(); modIdx1 += 1) {
		CModule &mod1 = *m_modList[modIdx1];

		if (!mod1.m_bIsUsed) continue;

		for (size_t ramIdx1 = 0; ramIdx1 < mod1.m_extRamList.size(); ramIdx1 += 1) {
			CRam &extRam1 = mod1.m_extRamList[ramIdx1];

			// count number of rams with extern=false
			bool bFound = false;
			for (size_t modIdx2 = 0; !bFound && modIdx2 < m_modList.size(); modIdx2 += 1) {
				CModule &mod2 = *m_modList[modIdx2];

				if (!mod2.m_bIsUsed) continue;

				for (size_t ramIdx2 = 0; !bFound && ramIdx2 < mod2.m_intGblList.size(); ramIdx2 += 1) {
					CRam &intRam2 = *mod2.m_intGblList[ramIdx2];

					if (intRam2.m_gblName == extRam1.m_gblName) {
						bFound = true;

						if (mod1.m_modInstList.size() > 1)
							ParseMsg(Error, extRam1.m_lineInfo, "global variables with extern=true used by replicated/multi-instance module not currently supported");
						else if (mod2.m_modInstList.size() > 1)
							ParseMsg(Error, intRam2.m_lineInfo, "global variables with extern=true used by replicated/multi-instance module not currently supported");

						if (intRam2.m_addr1W.AsInt() != extRam1.m_addr1W.AsInt()) {
							ParseMsg(Error, extRam1.m_lineInfo, "global variable with extern=true declared with different addr1W value");
							ParseMsg(Info, intRam2.m_lineInfo, "see global variable with extern=false");
						}

						if (intRam2.m_addr2W.AsInt() != extRam1.m_addr2W.AsInt()) {
							ParseMsg(Error, extRam1.m_lineInfo, "global variable with extern=true declared with different addr2W value");
							ParseMsg(Info, intRam2.m_lineInfo, "see global variable with extern=false");
						}

						if (intRam2.m_dimenList.size() != extRam1.m_dimenList.size()) {
							ParseMsg(Error, extRam1.m_lineInfo, "global variable with extern=true declared with different number of dimensions");
							ParseMsg(Info, intRam2.m_lineInfo, "see global variable with extern=false");
						} else {
							if (intRam2.m_dimenList.size() > 0 && intRam2.m_dimenList[0].AsInt() != extRam1.m_dimenList[0].AsInt()) {
								ParseMsg(Error, extRam1.m_lineInfo, "global variable with extern=true declared with different dimen1 value");
								ParseMsg(Info, intRam2.m_lineInfo, "see global variable with extern=false");
							}

							if (intRam2.m_dimenList.size() > 1 && intRam2.m_dimenList[1].AsInt() != extRam1.m_dimenList[1].AsInt()) {
								ParseMsg(Error, extRam1.m_lineInfo, "global variable with extern=true declared with different dimen2 value");
								ParseMsg(Info, intRam2.m_lineInfo, "see global variable with extern=false");
							}
						}
					}
				}
			}

			if (!bFound)
				ParseMsg(Error, extRam1.m_lineInfo, "global variable declared with extern=true does not have corresponding declaration with extern=false, '%s'",
					extRam1.m_gblName.c_str());
		}
	}

	// Initialize read and write flags for internal rams
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];

		if (!mod.m_bIsUsed) continue;

		for (size_t ramIdx = 0; ramIdx < mod.m_intGblList.size(); ramIdx += 1) {
			CRam &intGbl = *mod.m_intGblList[ramIdx];

			intGbl.m_bSrcRead = false;
			intGbl.m_bSrcWrite = false;
            intGbl.m_bMifRead = false;
            intGbl.m_bMifWrite = false;

			for (size_t fieldIdx = 0; fieldIdx < intGbl.m_fieldList.size(); fieldIdx += 1) {
				CField &field = intGbl.m_fieldList[fieldIdx];

				intGbl.m_bSrcRead = intGbl.m_bSrcRead || field.m_bSrcRead;
				intGbl.m_bSrcWrite = intGbl.m_bSrcWrite || field.m_bSrcWrite;
                intGbl.m_bMifRead = intGbl.m_bMifRead || field.m_bMifRead;
                intGbl.m_bMifWrite = intGbl.m_bMifWrite || field.m_bMifWrite;
			}

			if (intGbl.m_bShAddr && intGbl.m_bMifRead)
				ParseMsg(Error, intGbl.m_lineInfo, "global variable with stage variable read address and memory write not supported");
		}
	}

	// Initialize read and write flags for external rams
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];

		if (!mod.m_bIsUsed) continue;

		for (size_t ramIdx = 0; ramIdx < mod.m_extRamList.size(); ramIdx += 1) {
			CRam &extRam = mod.m_extRamList[ramIdx];

			extRam.m_bSrcRead = false;
			extRam.m_bSrcWrite = false;
            extRam.m_bMifRead = false;
            extRam.m_bMifWrite = false;

			for (size_t fieldIdx = 0; fieldIdx < extRam.m_fieldList.size(); fieldIdx += 1) {
				CField &field = extRam.m_fieldList[fieldIdx];

				extRam.m_bSrcRead = extRam.m_bSrcRead || field.m_bSrcRead;
				extRam.m_bSrcWrite = extRam.m_bSrcWrite || field.m_bSrcWrite;
                extRam.m_bMifRead = extRam.m_bMifRead || field.m_bMifRead;
                extRam.m_bMifWrite = extRam.m_bMifWrite || field.m_bMifWrite;
			}

            if (extRam.m_bSrcRead && extRam.m_bMifRead) {
				ParseMsg(Error, extRam.m_lineInfo, "module where global variable is declared with extern=true can read variable");
                ParseMsg(Info, extRam.m_lineInfo, "  with instructions, or with memory writes but not both, '%s'",
					extRam.m_gblName.c_str());
            }

            if (extRam.m_bSrcWrite && extRam.m_bMifWrite) {
				ParseMsg(Error, extRam.m_lineInfo, "module where global variable is declared with extern=true can write variable");
                ParseMsg(Info, extRam.m_lineInfo, "  with instructions, or with memory reads but not both, '%s'",
					extRam.m_gblName.c_str());
            }
		}
	}

	// Insert interal rams as first port for ram port list
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];

		if (!mod.m_bIsUsed) continue;

		for (size_t ramIdx = 0; ramIdx < mod.m_intGblList.size(); ramIdx += 1) {
			CRam &intGbl = *mod.m_intGblList[ramIdx];

			if (intGbl.m_fieldList.size() == 0)
				continue;

			intGbl.m_pIntGbl = &intGbl;
			intGbl.m_pIntRamMod = &mod;
			intGbl.m_pExtRamMod = &mod;

			intGbl.m_gblPortList.push_back(CRamPort(mod, "", intGbl, 0, 0));
			intGbl.m_gblPortList.back().m_bIntIntf = true;
			intGbl.m_gblPortList.back().m_bSrcRead = intGbl.m_bSrcRead;
			intGbl.m_gblPortList.back().m_bSrcWrite = intGbl.m_bSrcWrite;
 
            if (intGbl.m_bMifRead || intGbl.m_bMifWrite) {

                // if nonSmRead or nonSmWrite then push another gblPort
                intGbl.m_gblPortList.push_back(CRamPort(mod, "", intGbl, 0, 0));
				intGbl.m_gblPortList.back().m_bIntIntf = true;
				intGbl.m_gblPortList.back().m_bMifRead = intGbl.m_bMifRead;
				intGbl.m_gblPortList.back().m_bMifWrite = intGbl.m_bMifWrite;
            }

			if (mod.m_clkRate == eClk2x && intGbl.m_bSrcRead && intGbl.m_bMifRead)
				ParseMsg(Error, intGbl.m_lineInfo, "global variable with instruction read and memory write within clk2x module not supported"); 
		}
	}

	// Insert external rams in port list
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];

		if (!mod.m_bIsUsed) continue;

		for (size_t ramIdx = 0; ramIdx < mod.m_extRamList.size(); ramIdx += 1) {
			CRam &extRam = mod.m_extRamList[ramIdx];

			extRam.m_pExtRamMod = & mod;

			// find cooresponding module with internal ram
			bool bFound = false;
			for (size_t modIdx2 = 0; !bFound && modIdx2 < m_modList.size(); modIdx2 += 1) {
				CModule &mod2 = *m_modList[modIdx2];

				if (!mod2.m_bIsUsed) continue;

				for (size_t ramIdx2 = 0; !bFound && ramIdx2 < mod2.m_intGblList.size(); ramIdx2 += 1) {
					CRam &intGbl = *mod2.m_intGblList[ramIdx2];

					if (intGbl.m_gblName != extRam.m_gblName)
						continue;

					extRam.m_pIntGbl = &intGbl;
					extRam.m_pIntRamMod = &mod2;
					extRam.m_modName = mod2.m_modName;
					intGbl.m_extRamList.push_back(&extRam);

					int pairedUnitCnt = (int)extRam.m_pIntRamMod->m_modInstList.size();

					if ((int)mod.m_modInstList.size() <= pairedUnitCnt) {
						intGbl.m_gblPortList.push_back(CRamPort(mod, "", extRam, 0, 0));
						intGbl.m_gblPortList.back().m_bSrcRead = extRam.m_bSrcRead;
						intGbl.m_gblPortList.back().m_bSrcWrite = extRam.m_bSrcWrite;

						if (extRam.m_bMifRead || extRam.m_bMifWrite) {
							// if nonSmRead or nonSmWrite then push another gblPort
							intGbl.m_gblPortList.push_back(CRamPort(mod, "", extRam, 0, 0));
							intGbl.m_gblPortList.back().m_bMifRead = extRam.m_bMifRead;
							intGbl.m_gblPortList.back().m_bMifWrite = extRam.m_bMifWrite;
						}

					} else {
						//HtlAssert(pairedUnitCnt % mod.m_replCnt == 0);
						int intfCnt = (int)mod.m_modInstList.size() / pairedUnitCnt;

						string instNum = "0";
						for (int inst = 0; inst < intfCnt; inst += 1, instNum[0] += 1) {
							intGbl.m_gblPortList.push_back(CRamPort(mod, instNum, extRam, FindLg2(intfCnt-1), inst));
							intGbl.m_gblPortList.back().m_bSrcRead = extRam.m_bSrcRead;
							intGbl.m_gblPortList.back().m_bSrcWrite = extRam.m_bSrcWrite;

							if (extRam.m_bMifRead || extRam.m_bMifWrite) {
								// if nonSmRead or nonSmWrite then push another gblPort
								intGbl.m_gblPortList.push_back(CRamPort(mod, instNum, extRam, FindLg2(intfCnt-1), inst));
								intGbl.m_gblPortList.back().m_bMifRead = extRam.m_bMifRead;
								intGbl.m_gblPortList.back().m_bMifWrite = extRam.m_bMifWrite;
							}
						}
					}

					bFound = true;
				}
			}

			if (!bFound)
				ParseMsg(Error, extRam.m_lineInfo, "did not find declaration for external global variable, '%s'",
					extRam.m_gblName.c_str());
		}
	}

	if (CLex::GetParseErrorCnt() > 0)
		ParseMsg(Fatal, "Previous errors prevent generation of files");

	// check that all internal rams that are read have a declared private or shared read address 
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];

		if (!mod.m_bIsUsed) continue;

		bool bPrAddr = false;
		for (size_t ramIdx = 0; ramIdx < mod.m_intGblList.size(); ramIdx += 1) {
			CRam &intGbl = *mod.m_intGblList[ramIdx];

			if (!intGbl.m_bSrcRead) continue;
			if (intGbl.m_addr0W.size() == 0 && intGbl.m_addr1W.size() == 0) continue;

			if (intGbl.m_addr1W.AsInt() > 0 && intGbl.m_addr1Name.size() == 0) {
				ParseMsg(Error, intGbl.m_lineInfo, "expected addr1 to be specified for global variable, '%s'",
					intGbl.m_gblName.c_str());
				continue;
			}

			if (intGbl.m_addr1Name == "htId") {
				bPrAddr = true;

				if (intGbl.m_addr1W.AsInt() != mod.m_threads.m_htIdW.AsInt())
					ParseMsg(Error, intGbl.m_lineInfo, "expected addr1W (%d) to be same as AddModule htIdW (%d)",
						intGbl.m_addr1W.AsInt(), mod.m_threads.m_htIdW.AsInt());

			} else if (intGbl.m_addr1Name.size() > 0) {
			    bool bFound = false;
				CField const * pBaseField, * pLastField;
			    if (mod.m_bHasThreads) {
				    if (IsInFieldList(intGbl.m_lineInfo, intGbl.m_addr1Name, mod.m_threads.m_htPriv.m_fieldList, false, true,
						pBaseField, pLastField, 0)) 
					{
					    bFound = true;
						bPrAddr = true;
					}
			    }

				if (IsInFieldList(intGbl.m_lineInfo, intGbl.m_addr1Name, mod.m_stage.m_fieldList, false, true,
					pBaseField, pLastField, 0)) 
				{
					if (bFound)
						ParseMsg(Error, intGbl.m_lineInfo, "read addr1 name ambiguous, could be private or stage variable");
					else {
						bFound = true;
						intGbl.m_bShAddr = true;

						if (intGbl.m_bShAddr && intGbl.m_rdStg.AsInt() >= 2 &&
							(intGbl.m_rdStg.AsInt()-1 < pBaseField->m_rngLow.AsInt() || intGbl.m_rdStg.AsInt()-1 > pBaseField->m_rngHigh.AsInt()))
						{
							ParseMsg(Error, intGbl.m_lineInfo,
								"stage variable range does not include required stage (%d) for global variable read address",
								intGbl.m_rdStg.AsInt()-1);
						}
					}
				}

			    if (!bFound)
					ParseMsg(Error, intGbl.m_lineInfo, "did not find read address in private or stage variables, '%s'",
						intGbl.m_addr1Name.c_str());
            }

			if (intGbl.m_addr2W.AsInt() > 0 && intGbl.m_addr2Name.size() == 0) {
				ParseMsg(Error, intGbl.m_lineInfo, "expected addr2 to be specified for global variable, '%s'",
					intGbl.m_gblName.c_str());
				continue;
			}

			if (intGbl.m_addr2Name == "htId") {
				bPrAddr = true;

				if (intGbl.m_addr2W.AsInt() != mod.m_threads.m_htIdW.AsInt())
					ParseMsg(Error, intGbl.m_lineInfo, "expected addr2W (%d) to be same as AddModule htIdW (%d)",
						intGbl.m_addr2W.AsInt(), mod.m_threads.m_htIdW.AsInt());

			} else if (intGbl.m_addr2Name.size() > 0) {
			    bool bFldFound = false;
				CField const * pBaseField, * pLastField;
			    if (mod.m_bHasThreads)
				    bFldFound = IsInFieldList(intGbl.m_lineInfo, intGbl.m_addr2Name, mod.m_threads.m_htPriv.m_fieldList, false, true, 
						pBaseField, pLastField, 0);

				if (IsInFieldList(intGbl.m_lineInfo, intGbl.m_addr2Name, mod.m_stage.m_fieldList, false, true,
					pBaseField, pLastField, 0)) 
				{
					if (bFldFound)
						ParseMsg(Error, intGbl.m_lineInfo, "read addr2 name ambiguous, could be private or stage variable");
					else {
						bFldFound = true;
						intGbl.m_bShAddr = true;

						if (intGbl.m_bShAddr && intGbl.m_rdStg.AsInt() >= 2 &&
							(intGbl.m_rdStg.AsInt()-1 < pBaseField->m_rngLow.AsInt() || intGbl.m_rdStg.AsInt()-1 > pBaseField->m_rngHigh.AsInt()))
						{
							ParseMsg(Error, intGbl.m_lineInfo,
								"stage variable range does not include required stage (%d) for global variable read address",
								intGbl.m_rdStg.AsInt()-1);
						}
					}
				}

			    if (!bFldFound)
					ParseMsg(Error, intGbl.m_lineInfo, "did not find read address in private or stage variables, '%s'",
						intGbl.m_addr2Name.c_str());
            }

			if (bPrAddr && intGbl.m_bShAddr)
				ParseMsg(Error, intGbl.m_lineInfo, "mixing stage and private variables for global variable read addresses not supported",
					intGbl.m_addr1Name.c_str());

			if (intGbl.m_bShAddr && intGbl.m_rdStg.AsInt() < 2)
				ParseMsg(Error, intGbl.m_lineInfo, "global variable must specify rdStg > 1 when read address is stage variable");
		}
	}

	// create list of all fields for internal rams, second process external ram's fields
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];

		if (!mod.m_bIsUsed) continue;

		for (size_t ramIdx = 0; ramIdx < mod.m_intGblList.size(); ramIdx += 1) {
			CRam &intGbl = *mod.m_intGblList[ramIdx];

			for (size_t ramPortIdx = 0; ramPortIdx < intGbl.m_gblPortList.size(); ramPortIdx += 1) {
				CRamPort &gblPort = intGbl.m_gblPortList[ramPortIdx];

				for (size_t fldIdx = 0; fldIdx < gblPort.m_pRam->m_fieldList.size(); fldIdx += 1) {
					CField &field = gblPort.m_pRam->m_fieldList[fldIdx];

					size_t intFldIdx;
					for (intFldIdx = 0; intFldIdx < intGbl.m_allFieldList.size(); intFldIdx += 1)
						if (intGbl.m_allFieldList[intFldIdx].m_name == field.m_name)
							break;

					if (intFldIdx == intGbl.m_allFieldList.size()) {
						intGbl.m_allFieldList.push_back(field);
						field.m_gblFieldIdx = (int)fldIdx;
					} else {
						// already in list, verify types are the same
						if (intGbl.m_allFieldList[intFldIdx].m_type != field.m_type) {
							ParseMsg(Error, field.m_lineInfo, "global ram field type conflict, '%s'",
								field.m_name.size() > 0 ? field.m_name.c_str() : intGbl.m_gblName.c_str());
							ParseMsg(Info, intGbl.m_allFieldList[intFldIdx].m_lineInfo, "    conflicting type declaration");
						}
					}

					if (field.m_ramType == eBlockRam || intGbl.m_ramType == eBlockRam)
						intGbl.m_allFieldList[intFldIdx].m_ramType = eBlockRam;

					if (gblPort.m_pRam->m_pExtRamMod->m_clkRate == eClk2x || gblPort.m_pRam->m_bShAddr) {
						if (gblPort.m_bSrcRead && field.m_bSrcRead || gblPort.m_bMifRead && field.m_bMifRead) {
							// align to even entry
							if ((intGbl.m_allFieldList[intFldIdx].m_readerList.size() & 1) == 1)
								intGbl.m_allFieldList[intFldIdx].m_readerList.push_back(0);
							intGbl.m_allFieldList[intFldIdx].m_readerList.push_back(CRamPortField(&gblPort));
						}
					}
					if (gblPort.m_pRam->m_pExtRamMod->m_clkRate == eClk2x) {
						if (gblPort.m_bSrcWrite && field.m_bSrcWrite || gblPort.m_bMifWrite && field.m_bMifWrite) {
							// align to even entry
							if ((intGbl.m_allFieldList[intFldIdx].m_writerList.size() & 1) == 1)
								intGbl.m_allFieldList[intFldIdx].m_writerList.push_back(0);
							intGbl.m_allFieldList[intFldIdx].m_writerList.push_back(&gblPort);
						}
					}

					if (gblPort.m_bSrcRead && field.m_bSrcRead)
						intGbl.m_allFieldList[intFldIdx].m_readerList.push_back(&gblPort);
					if (gblPort.m_bSrcWrite && field.m_bSrcWrite)
						intGbl.m_allFieldList[intFldIdx].m_writerList.push_back(&gblPort);
					if (gblPort.m_bMifRead && field.m_bMifRead)
						intGbl.m_allFieldList[intFldIdx].m_readerList.push_back(&gblPort);
					if (gblPort.m_bMifWrite && field.m_bMifWrite)
						intGbl.m_allFieldList[intFldIdx].m_writerList.push_back(&gblPort);
				}
			}
		}
	}

	// create list of outbound instance rams for each module
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];

		if (!mod.m_bIsUsed) continue;

		for (size_t ramIdx = 0; ramIdx < mod.m_extRamList.size(); ramIdx += 1) {
			CRam &extRam = mod.m_extRamList[ramIdx];

			int pairedUnitCnt = (int)extRam.m_pIntRamMod->m_modInstList.size();

			if ((int)mod.m_modInstList.size() >= pairedUnitCnt)
				mod.m_extInstRamList.push_back(CRamPort(*extRam.m_pIntRamMod, "", extRam, 0, 0));

			else {
				HtlAssert(pairedUnitCnt % mod.m_modInstList.size() == 0);
				int intfCnt = pairedUnitCnt / (int)mod.m_modInstList.size();

				string instNum = "0";
				for (int inst = 0; inst < intfCnt; inst += 1, instNum[0] += 1)
					mod.m_extInstRamList.push_back(CRamPort(*extRam.m_pIntRamMod, instNum, extRam, FindLg2(intfCnt-1), inst));
			}
		}
	}
	
	InitBramUsage();

	// examine ram fields and set flags to identify types of interfaces needed
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];

		if (!mod.m_bIsUsed) continue;

		for (size_t ramIdx = 0; ramIdx < mod.m_intGblList.size(); ramIdx += 1) {
			CRam &intGbl = *mod.m_intGblList[ramIdx];

			for (size_t fldIdx = 0; fldIdx < intGbl.m_allFieldList.size(); fldIdx += 1) {
				CField &field = intGbl.m_allFieldList[fldIdx];

				for (size_t rdIdx = 0; rdIdx < field.m_readerList.size(); rdIdx += 2) {
					CRamPortField &portField0 = field.m_readerList[rdIdx];

					if (rdIdx == 0 && portField0.m_pRamPort->m_pRam->m_bShAddr) {
						// shared variable read address
						portField0.m_bShAddr = true;

					} else if (rdIdx+1 == field.m_readerList.size() || field.m_readerList[rdIdx+1].m_pRamPort == 0) {
						// one valid entry, 1x read port
						if (m_bBlockRamDoReg && field.m_ramType == eBlockRam && field.m_readerList[rdIdx].m_pRamPort->m_bIntIntf) {
							portField0.m_pRamPort->m_bFirst1xReadIntf = true;
							portField0.m_bFirst1xReadIntf = true;
							field.m_bTwo1xReadIntf = true;
						} else {
							portField0.m_pRamPort->m_bOne1xReadIntf = true;
							portField0.m_bOne1xReadIntf = true;
							field.m_bOne1xReadIntf = true;
						}
					} else if (field.m_readerList[rdIdx].m_pRamPort != field.m_readerList[rdIdx+1].m_pRamPort) {
						// two valid entry, 2x read port from 1x modules
						CRamPortField &portField1 = field.m_readerList[rdIdx+1];
						portField0.m_pRamPort->m_bFirst1xReadIntf = true;
						portField0.m_bFirst1xReadIntf = true;
						portField1.m_pRamPort->m_bSecond1xReadIntf = true;
						portField1.m_bSecond1xReadIntf = true;
						field.m_bTwo1xReadIntf = true;
					} else {
						// two valid matching entries, 2x read port from a single 2x module
						portField0.m_pRamPort->m_bSrcRead = true;
						portField0.m_pRamPort->m_bOne2xReadIntf= true;
						portField0.m_bOne2xReadIntf = true;
						field.m_bOne2xReadIntf = true;
					}
				}

				for (size_t wrIdx = 0; wrIdx < field.m_writerList.size(); wrIdx += 2) {
					CRamPortField &portField0 = field.m_writerList[wrIdx];

					if (wrIdx+1 == field.m_writerList.size() || field.m_writerList[wrIdx+1].m_pRamPort == 0) {
						// one valid entry, 1x write port
						//portField0.m_pRamPort->m_bSrcWrite = true;
						portField0.m_pRamPort->m_bOne1xWriteIntf= true;
						portField0.m_bOne1xWriteIntf = true;
						field.m_bOne1xWriteIntf = true;
					} else if (field.m_writerList[wrIdx].m_pRamPort != field.m_writerList[wrIdx+1].m_pRamPort) {
						// two valid entry, 2x write port from 1x modules
						CRamPortField &portField1 = field.m_writerList[wrIdx+1];
						//portField0.m_pRamPort->m_bSrcWrite = true;
						portField0.m_pRamPort->m_bFirst1xWriteIntf= true;
						portField0.m_bFirst1xWriteIntf = true;
						//portField1.m_pRamPort->m_bSrcWrite = true;
						portField1.m_pRamPort->m_bSecond1xWriteIntf= true;
						portField1.m_bSecond1xWriteIntf = true;
						field.m_bTwo1xWriteIntf = true;
					} else {
						// two valid matching entries, 2x write port from a single 2x module
						//portField0.m_pRamPort->m_bSrcWrite = true;
						portField0.m_pRamPort->m_bOne2xWriteIntf= true;
						portField0.m_bOne2xWriteIntf = true;
						field.m_bOne2xWriteIntf = true;
					}
				}
			}
		}
	}

	// now that the allFieldList has the ramType correct, set the bAllBlockRam field for each gblPort
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &intRamModInfo = *m_modList[modIdx];

		if (!intRamModInfo.m_bIsUsed) continue;

		for (size_t ramIdx = 0; ramIdx < intRamModInfo.m_intGblList.size(); ramIdx += 1) {
			CRam &intGbl = *intRamModInfo.m_intGblList[ramIdx];

			for (size_t ramPortIdx = 0; ramPortIdx < intGbl.m_gblPortList.size(); ramPortIdx += 1) {
				CRamPort &gblPort = intGbl.m_gblPortList[ramPortIdx];

				gblPort.m_bAllBlockRam = true;
				for (size_t fldIdx = 0; fldIdx < gblPort.m_pRam->m_fieldList.size(); fldIdx += 1) {
					CField &field = gblPort.m_pRam->m_fieldList[fldIdx];

					size_t intFldIdx;
					for (intFldIdx = 0; intFldIdx < intGbl.m_allFieldList.size(); intFldIdx += 1)
						if (intGbl.m_allFieldList[intFldIdx].m_name == field.m_name)
							break;

					HtlAssert (intFldIdx < intGbl.m_allFieldList.size());

					if (intGbl.m_allFieldList[intFldIdx].m_bOne1xReadIntf && intGbl.m_allFieldList[intFldIdx].m_ramType != eBlockRam)
						gblPort.m_bAllBlockRam = false;
				}
			}
		}
	}

	// check that all internal rams have one or more readers and writers
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];

		if (!mod.m_bIsUsed) continue;

		for (size_t ramIdx = 0; ramIdx < mod.m_intGblList.size(); ramIdx += 1) {
			CRam &intGbl = *mod.m_intGblList[ramIdx];

			for (size_t fldIdx = 0; fldIdx < intGbl.m_allFieldList.size(); fldIdx += 1) {
				CField &field = intGbl.m_allFieldList[fldIdx];

				if (field.m_readerList.size() == 0) {
					ParseMsg(Error, field.m_lineInfo, "no reader for field found in linked modules, '%s'",
						field.m_name.size() > 0 ? field.m_name.c_str() : intGbl.m_gblName.c_str());
					ListIgnoredModules();
				}

				if (field.m_writerList.size() == 0) {
					ParseMsg(Error, field.m_lineInfo, "no writer for field found in linked modules, '%s'",
						field.m_name.size() > 0 ? field.m_name.c_str() : intGbl.m_gblName.c_str());
					ListIgnoredModules();
				}

				if (field.m_writerList.size() > 2) {
					ParseMsg(Error, field.m_lineInfo, "more than two write ports for field found in linked modules");

					ParseMsg(Info, field.m_lineInfo, "  Global variable: %s, field name: %s",
						intGbl.m_gblName.c_str(), field.m_name.c_str());

					for (size_t i = 0; i < field.m_writerList.size(); i += 1) {
						CRamPort * pRamPort = field.m_writerList[i].m_pRamPort;

						ParseMsg(Info, field.m_writerList[i].m_pRamPort->m_pRam->m_lineInfo,
							"  Module %s, clock rate %s, write type: %s",
							pRamPort->m_pModInfo->m_modName.c_str(),
							pRamPort->m_pModInfo->m_clkRate == eClk1x ? "1x (1 port)" : "2x (2 ports)",
							pRamPort->m_bMifWrite ? "mif write" : "src write");

						if (pRamPort->m_pModInfo->m_clkRate == eClk2x)
							i += 1;
					}
				}
			}
		}
	}

	if (CPreProcess::m_errorCnt > 0) {
		ParseMsg(Fatal, "previous error(s) prohibit generation of files");
	}

	// check if 2xClk is needed for internal rams
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];

		if (!mod.m_bIsUsed) continue;

		mod.m_bNeed2xClk = false;
		mod.m_bNeed2xReset = false;

		for (size_t ramIdx = 0; ramIdx < mod.m_intGblList.size(); ramIdx += 1) {
			CRam &intGbl = *mod.m_intGblList[ramIdx];

			for (size_t fldIdx = 0; fldIdx < intGbl.m_allFieldList.size(); fldIdx += 1) {
				CField &field = intGbl.m_allFieldList[fldIdx];

				mod.m_bNeed2xClk = mod.m_bNeed2xClk || mod.m_clkRate == eClk2x || field.m_readerList.size() > 1
					|| field.m_readerList.size() == 1 && field.m_readerList[0].m_pRamPort->m_bFirst1xReadIntf
					|| field.m_writerList.size() > 1

					|| (field.m_readerList.size() == 1 || field.m_readerList[1].m_pRamPort == 0)
					&& !(m_bBlockRamDoReg && field.m_ramType == eBlockRam && field.m_readerList[0].m_pRamPort->m_bIntIntf);

			}
		}
	}

	// check that all external rams that are read have a declared private read address 
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];

		if (!mod.m_bIsUsed) continue;

		for (size_t ramIdx = 0; ramIdx < mod.m_extRamList.size(); ramIdx += 1) {
			CRam &extRam = mod.m_extRamList[ramIdx];

			if (!extRam.m_bSrcRead && extRam.m_addr1Name.size() == 0) continue;
			if (extRam.m_bSrcRead && extRam.m_addr1W.size() == 0) continue;

			if (extRam.m_bSrcRead && extRam.m_addr1W.size() > 0 && extRam.m_addr1Name.size() == 0) {
				ParseMsg(Error, extRam.m_lineInfo, "no read address specified for global variable, '%s'",
					extRam.m_gblName.c_str());
				continue;
			}

			bool bFldFound = false;
			CField const * pBaseField, * pLastField;
			if (mod.m_bHasThreads)
				bFldFound = IsInFieldList(extRam.m_lineInfo, extRam.m_addr1Name, mod.m_threads.m_htPriv.m_fieldList, false, true, 
					pBaseField, pLastField, 0);

			if (!bFldFound) {
				if (IsInFieldList(extRam.m_lineInfo, extRam.m_addr1Name, mod.m_stage.m_fieldList, false, true, pBaseField, pLastField, 0))
					ParseMsg(Error, extRam.m_lineInfo, "extern global variable with stage variable as read address not supported, '%s'",
						extRam.m_addr1Name.c_str());
				else
					ParseMsg(Error, extRam.m_lineInfo, "did not find read address in private variables, '%s'",
						extRam.m_addr1Name.c_str());
 			}

            if (extRam.m_bSrcRead && extRam.m_addr2Name.size() > 0) {
			    bFldFound = false;
				CField const * pBaseField;
			    if (mod.m_bHasThreads)
				    bFldFound = IsInFieldList(extRam.m_lineInfo, extRam.m_addr2Name, mod.m_threads.m_htPriv.m_fieldList, false, true,
						pBaseField, pLastField, 0);

				if (!bFldFound) {
					if (IsInFieldList(extRam.m_lineInfo, extRam.m_addr2Name, mod.m_stage.m_fieldList, false, true, pBaseField, pLastField, 0))
						ParseMsg(Error, extRam.m_lineInfo, "extern global variable with stage variable as read address not supported, '%s'",
							extRam.m_addr2Name.c_str());
					else
						ParseMsg(Error, extRam.m_lineInfo, "did not find read address in private variables, '%s'",
							extRam.m_addr2Name.c_str());
				}
            }
		}
	}
}

void CDsnInfo::GenModRamStatements(CModInst &modInst)
{
	///////////////////////////////////////////////////////////////////////
	// Generate ram access macros

	CModule &mod = *modInst.m_pMod;

	if (mod.m_extRamList.size() == 0 && mod.m_intGblList.size() == 0)
		return;

	m_gblDefines.Append("// Ram access methods\n");
	m_gblDefines.Append("#if !defined(TOPOLOGY_HEADER)\n");

	g_appArgs.GetDsnRpt().AddLevel("Global Variables\n");

	for (size_t ramIdx = 0; ramIdx < mod.m_extRamList.size(); ramIdx += 1) {
		CRam &extRam = mod.m_extRamList[ramIdx];

		if (extRam.m_fieldList.size() > 0) {

			int wrStg = mod.m_tsStg + extRam.m_wrStg.AsInt() - 1;

			char varWrStg[64] = { '\0' };
			if (extRam.m_wrStg.size() > 0)
				sprintf(varWrStg, "%d", extRam.m_wrStg.AsInt());
			//else if (mod.m_stage.m_bStageNums)
			//	sprintf(varWrStg, "%d", mod.m_stage.m_privWrStg.AsInt());

			for (size_t fldIdx = 0; fldIdx < extRam.m_fieldList.size(); fldIdx += 1) {
				CField &field = extRam.m_fieldList[fldIdx];

				string apiAddrParams;
				string addrParams;
				string addrValue;
				char tmp[128];

				char htIdStr[32];
				sprintf(htIdStr, "r_t%d_htId", wrStg);

				if (extRam.m_pIntGbl->m_addr1W.size() > 0 || extRam.m_pIntGbl->m_addr1Name.size() > 0) {
					if (extRam.m_pIntGbl->m_addr1Name == "htId" && !g_appArgs.IsGlobalWriteHtidEnabled()) {
						if (extRam.m_pIntGbl->m_addr1W.AsInt() > 0)
							addrValue += htIdStr;
					} else {
						int wrAddr1W = extRam.m_pIntGbl->m_addr1W.AsInt() ? 1 : extRam.m_pIntGbl->m_addr1W.AsInt();
						sprintf(tmp, "ht_uint%d wrAddr1, ", wrAddr1W);
						apiAddrParams += tmp;

						addrParams += "wrAddr1, ";
						if (extRam.m_pIntGbl->m_addr1W.AsInt() > 0)
							addrValue += "wrAddr1";
					}
				}
				if (extRam.m_pIntGbl->m_addr2W.size() > 0 || extRam.m_pIntGbl->m_addr2Name.size() > 0) {
					bool bParens = addrValue.size() > 0;

					if (bParens) addrValue = "(" + addrValue + ", ";
					if (extRam.m_pIntGbl->m_addr2Name == "htId" && !g_appArgs.IsGlobalWriteHtidEnabled()) {
						if (extRam.m_pIntGbl->m_addr2W.AsInt() > 0)
							addrValue += htIdStr;
					} else {
						int wrAddr2W = extRam.m_pIntGbl->m_addr2W.AsInt() ? 1 : extRam.m_pIntGbl->m_addr2W.AsInt();
						sprintf(tmp, "ht_uint%d wrAddr2, ", wrAddr2W);
						apiAddrParams += tmp;

						addrParams += "wrAddr2, ";
						if (extRam.m_pIntGbl->m_addr2W.AsInt() > 0)
							addrValue += "wrAddr2";
					}
					if (bParens) addrValue += ")";
				}

				string apiVarIdxParams;
				const char *pVarIdxParams;
				const char *pVarIndexes;
				HtlAssert(extRam.m_pIntGbl->m_dimenList.size() <= 2);
				if (extRam.m_pIntGbl->m_dimenList.size() == 2) {
					sprintf(tmp, "ht_uint%d varIdx1, ht_uint%d varIdx2, ",
						FindLg2(extRam.m_pIntGbl->m_dimenList[0].AsInt()), FindLg2(extRam.m_pIntGbl->m_dimenList[1].AsInt()));
					apiVarIdxParams += tmp;

					pVarIdxParams = "varIdx1, varIdx2, ";
					pVarIndexes = "[varIdx1][varIdx2]";
				} else if (extRam.m_pIntGbl->m_dimenList.size() == 1) {
					sprintf(tmp, "ht_uint%d varIdx1, ",
						FindLg2(extRam.m_pIntGbl->m_dimenList[0].AsInt()));
					apiVarIdxParams += tmp;

					pVarIdxParams = "varIdx, ";
					pVarIndexes = "[varIdx]";
				} else {
					pVarIdxParams = "";
					pVarIndexes = "";
				}

				string apiFldIdxParams;
				const char *pFldIdxParams;
				const char *pFldIndexes;
				HtlAssert(field.m_dimenList.size() <= 2);
				if (field.m_dimenList.size() == 2) {
					sprintf(tmp, "ht_uint%d fldIdx1, ht_uint%d fldIdx2, ",
						FindLg2(field.m_dimenList[0].AsInt()), FindLg2(field.m_dimenList[1].AsInt()));
					apiFldIdxParams += tmp;

					pFldIdxParams = "fldIdx1, fldIdx2, ";
					pFldIndexes = "[fldIdx1][fldIdx2]";
				} else if (field.m_dimenList.size() == 1) {
					sprintf(tmp, "ht_uint%d fldIdx1, ",
						FindLg2(field.m_dimenList[0].AsInt()));
					apiFldIdxParams += tmp;

					pFldIdxParams = "fldIdx, ";
					pFldIndexes = "[fldIdx]";
				} else {
					pFldIdxParams = "";
					pFldIndexes = "";
				}

				if (field.m_bSrcWrite) {
					g_appArgs.GetDsnRpt().AddItem("GW%s_%s_%s(%s%s%s%s %s)\n",
						varWrStg, extRam.m_gblName.c_str(), field.m_name.c_str(),
						apiAddrParams.c_str(), apiVarIdxParams.c_str(), apiFldIdxParams.c_str(), field.m_type.c_str(), field.m_name.c_str());

					m_gblDefines.Append("#define GW%s_%s_%s(%s%s%s%s) \\\n",
						varWrStg, extRam.m_gblName.c_str(), field.m_name.c_str(),
						addrParams.c_str(), pVarIdxParams, pFldIdxParams, field.m_name.c_str());
					m_gblDefines.Append("do { \\\n");
					if (addrValue.size() > 0)
						m_gblDefines.Append("\tc_t%d_%sWrAddr%s = %s; \\\n",
							wrStg, extRam.m_gblName.c_str(), pVarIndexes, addrValue.c_str());
					m_gblDefines.Append("\tc_t%d_%sWrEn%s.m_%s%s = true; \\\n",
						wrStg, extRam.m_gblName.c_str(), pVarIndexes, field.m_name.c_str(), pFldIndexes);
					m_gblDefines.Append("\tc_t%d_%sWrData%s.m_%s%s = %s; \\\n",
						wrStg, extRam.m_gblName.c_str(), pVarIndexes, field.m_name.c_str(), pFldIndexes, field.m_name.c_str());
					m_gblDefines.Append("} while(false)\n");
					m_gblDefines.Append("\n");
				}
			}

			int rdStg = mod.m_tsStg + extRam.m_rdStg.AsInt() - 1;

			char varRdStg[64] = { '\0' };
			if (extRam.m_rdStg.size() > 0)
				sprintf(varRdStg, "%d", extRam.m_rdStg.AsInt());
			else if (mod.m_stage.m_bStageNums)
				sprintf(varRdStg, "1");

			for (size_t fldIdx = 0; fldIdx < extRam.m_fieldList.size(); fldIdx += 1) {
				CField &field = extRam.m_fieldList[fldIdx];

				if (!field.m_bSrcRead) continue;

				string apiVarIdxParams;
				const char *pVarIdxParams;
				const char *pVarIndexes;
				char tmp[128];

				HtlAssert(extRam.m_pIntGbl->m_dimenList.size() <= 2);
				if (extRam.m_pIntGbl->m_dimenList.size() == 2) {
					sprintf(tmp, "ht_uint%d varIdx1, ht_uint%d varIdx2",
						FindLg2(extRam.m_pIntGbl->m_dimenList[0].AsInt()), FindLg2(extRam.m_pIntGbl->m_dimenList[1].AsInt()));
					apiVarIdxParams += tmp;

					pVarIdxParams = "varIdx1, varIdx2";
					pVarIndexes = "[varIdx1][varIdx2]";
				} else if (extRam.m_pIntGbl->m_dimenList.size() == 1) {
					sprintf(tmp, "ht_uint%d varIdx1",
						FindLg2(extRam.m_pIntGbl->m_dimenList[0].AsInt()));
					apiVarIdxParams += tmp;

					pVarIdxParams = "varIdx";
					pVarIndexes = "[varIdx]";
				} else {
					pVarIdxParams = "";
					pVarIndexes = "";
				}

				string apiFldIdxParams;
				const char *pFldIdxParams;
				const char *pFldIndexes;
				HtlAssert(field.m_dimenList.size() <= 2);
				if (field.m_dimenList.size() == 2) {
					sprintf(tmp, "ht_uint%d fldIdx1, ht_uint%d fldIdx2",
						FindLg2(field.m_dimenList[0].AsInt()), FindLg2(field.m_dimenList[1].AsInt()));
					apiFldIdxParams += tmp;

					pFldIdxParams = "fldIdx1, fldIdx2";
					pFldIndexes = "[fldIdx1][fldIdx2]";
				} else if (field.m_dimenList.size() == 1) {
					sprintf(tmp, "ht_uint%d fldIdx1",
						FindLg2(field.m_dimenList[0].AsInt()));
					apiFldIdxParams += tmp;

					pFldIdxParams = "fldIdx";
					pFldIndexes = "[fldIdx]";
				} else {
					pFldIdxParams = "";
					pFldIndexes = "";
				}

				g_appArgs.GetDsnRpt().AddItem("%s GR%s_%s_%s", field.m_type.c_str(),
					varRdStg, extRam.m_gblName.c_str(), field.m_name.c_str());

				m_gblDefines.Append("\t#define GR%s_%s_%s",
					varRdStg, extRam.m_gblName.c_str(), field.m_name.c_str());

				if (extRam.m_pIntGbl->m_dimenList.size() > 0 || field.m_dimenList.size() > 0 || g_appArgs.IsGlobalReadParanEnabled()) {
					g_appArgs.GetDsnRpt().AddText("(%s%s%s)\n", apiVarIdxParams.c_str(),
						extRam.m_pIntGbl->m_dimenList.size() > 0 && field.m_dimenList.size() > 0 ? ", " : "", apiFldIdxParams.c_str());

					m_gblDefines.Append("(%s%s%s)", pVarIdxParams,
						extRam.m_pIntGbl->m_dimenList.size() > 0 && field.m_dimenList.size() > 0 ? ", " : "", pFldIdxParams);
				} else
					g_appArgs.GetDsnRpt().AddText("\n");

				m_gblDefines.Append(" r_t%d_%sRdData%s.m_%s%s\n",
					rdStg, extRam.m_gblName.c_str(), pVarIndexes, field.m_name.c_str(), pFldIndexes);
			}

			m_gblDefines.Append("\n");
		}
	}
		
	// generate internal ram interface
	for (size_t ramIdx = 0; ramIdx < mod.m_intGblList.size(); ramIdx += 1) {
		CRam &intGbl = *mod.m_intGblList[ramIdx];

		if (intGbl.m_fieldList.size() == 0) continue;
		if (intGbl.m_bPrivGbl) continue;

		int wrStg = mod.m_tsStg + intGbl.m_wrStg.AsInt() - 1;

		char varWrStg[64] = { '\0' };
		if (intGbl.m_wrStg.size() > 0)
			sprintf(varWrStg, "%d", intGbl.m_wrStg.AsInt());
		//else if (mod.m_stage.m_bStageNums)
		//	sprintf(varWrStg, "%d", mod.m_stage.m_privWrStg.AsInt());

		for (size_t fldIdx = 0; fldIdx < intGbl.m_fieldList.size(); fldIdx += 1) {
			CField &field = intGbl.m_fieldList[fldIdx];

			string apiAddrParams;
			string addrParams;
			string addrValue;
			char tmp[128];

			char htIdStr[32];
			sprintf(htIdStr, "r_t%d_htId", wrStg);

			if (intGbl.m_pIntGbl->m_addr1W.size() > 0 || intGbl.m_pIntGbl->m_addr1Name.size() > 0) {
				if (intGbl.m_pIntGbl->m_addr1Name == "htId" && !g_appArgs.IsGlobalWriteHtidEnabled()) {
					if (intGbl.m_pIntGbl->m_addr1W.AsInt() > 0)
						addrValue += htIdStr;
				} else {
					int wrAddr1W = intGbl.m_pIntGbl->m_addr1W.AsInt() > 0 ? 1 : intGbl.m_pIntGbl->m_addr1W.AsInt();
					sprintf(tmp, "ht_uint%d wrAddr1, ", wrAddr1W);
					apiAddrParams += tmp;

					addrParams += "wrAddr1, ";
					if (intGbl.m_pIntGbl->m_addr1W.AsInt() > 0)
						addrValue += "wrAddr1";
				}
			}
			if (intGbl.m_pIntGbl->m_addr2W.size() > 0 || intGbl.m_pIntGbl->m_addr2Name.size() > 0) {
				bool bParens = addrValue.size() > 0;

				if (bParens) addrValue = "(" + addrValue + ", ";
				if (intGbl.m_pIntGbl->m_addr2Name == "htId" && !g_appArgs.IsGlobalWriteHtidEnabled()) {
					if (intGbl.m_pIntGbl->m_addr2W.AsInt() > 0)
						addrValue += htIdStr;
				} else {
					int wrAddr2W = intGbl.m_pIntGbl->m_addr2W.AsInt() > 0 ? 1 : intGbl.m_pIntGbl->m_addr2W.AsInt();
					sprintf(tmp, "ht_uint%d wrAddr2, ", wrAddr2W);
					apiAddrParams += tmp;

					addrParams += "wrAddr2, ";
					if (intGbl.m_pIntGbl->m_addr2W.AsInt() > 0)
						addrValue += "wrAddr2";
				}
				if (bParens) addrValue += ")";
			}

			string apiVarIdxParams;
			const char *pVarIdxParams;
			const char *pVarIndexes;
			HtlAssert(intGbl.m_pIntGbl->m_dimenList.size() <= 2);
			if (intGbl.m_pIntGbl->m_dimenList.size() == 2) {
				sprintf(tmp, "ht_uint%d varIdx1, ht_uint%d varIdx2, ",
					FindLg2(intGbl.m_pIntGbl->m_dimenList[0].AsInt()), FindLg2(intGbl.m_pIntGbl->m_dimenList[1].AsInt()));
				apiVarIdxParams += tmp;

				pVarIdxParams = "varIdx1, varIdx2, ";
				pVarIndexes = "[varIdx1][varIdx2]";
			} else if (intGbl.m_pIntGbl->m_dimenList.size() == 1) {
				sprintf(tmp, "ht_uint%d varIdx1, ",
					FindLg2(intGbl.m_pIntGbl->m_dimenList[0].AsInt()));
				apiVarIdxParams += tmp;

				pVarIdxParams = "varIdx, ";
				pVarIndexes = "[varIdx]";
			} else {
				pVarIdxParams = "";
				pVarIndexes = "";
			}

			string apiFldIdxParams;
			const char *pFldIdxParams;
			const char *pFldIndexes;
			HtlAssert(field.m_dimenList.size() <= 2);
			if (field.m_dimenList.size() == 2) {
				sprintf(tmp, "ht_uint%d fldIdx1, ht_uint%d fldIdx2, ",
					FindLg2(field.m_dimenList[0].AsInt()), FindLg2(field.m_dimenList[1].AsInt()));
				apiFldIdxParams += tmp;

				pFldIdxParams = "fldIdx1, fldIdx2, ";
				pFldIndexes = "[fldIdx1][fldIdx2]";
			} else if (field.m_dimenList.size() == 1) {
				sprintf(tmp, "ht_uint%d fldIdx1, ",
					FindLg2(field.m_dimenList[0].AsInt()));
				apiFldIdxParams += tmp;

				pFldIdxParams = "fldIdx, ";
				pFldIndexes = "[fldIdx]";
			} else {
				pFldIdxParams = "";
				pFldIndexes = "";
			}

			if (field.m_bSrcWrite) {
				g_appArgs.GetDsnRpt().AddItem("void GW%s_%s_%s(%s%s%s%s %s)\n",
					varWrStg, intGbl.m_gblName.c_str(), field.m_name.c_str(),
					apiAddrParams.c_str(), apiVarIdxParams.c_str(), apiFldIdxParams.c_str(), field.m_type.c_str(), field.m_name.c_str());

				m_gblDefines.Append("#define GW%s_%s_%s(%s%s%s%s) \\\n",
					varWrStg, intGbl.m_gblName.c_str(), field.m_name.c_str(),
					addrParams.c_str(), pVarIdxParams, pFldIdxParams, field.m_name.c_str());
				m_gblDefines.Append("do { \\\n");
				if (addrValue.size() > 0)
					m_gblDefines.Append("\tc_t%d_%sWrAddr%s = %s; \\\n",
						wrStg, intGbl.m_gblName.c_str(), pVarIndexes, addrValue.c_str());
				m_gblDefines.Append("\tc_t%d_%sWrEn%s.m_%s%s = true; \\\n",
					wrStg, intGbl.m_gblName.c_str(), pVarIndexes, field.m_name.c_str(), pFldIndexes);
				m_gblDefines.Append("\tc_t%d_%sWrData%s.m_%s%s = %s; \\\n",
					wrStg, intGbl.m_gblName.c_str(), pVarIndexes, field.m_name.c_str(), pFldIndexes, field.m_name.c_str());
				m_gblDefines.Append("} while(false)\n");
				m_gblDefines.Append("\n");
			}
		}

		int rdStg = mod.m_tsStg + intGbl.m_rdStg.AsInt() - 1;

		char varRdStg[64] = { '\0' };
		if (intGbl.m_rdStg.size() > 0)
			sprintf(varRdStg, "%d", intGbl.m_rdStg.AsInt());
		else if (mod.m_stage.m_bStageNums)
			sprintf(varRdStg, "1");

		for (size_t fldIdx = 0; fldIdx < intGbl.m_fieldList.size(); fldIdx += 1) {
			CField &field = intGbl.m_fieldList[fldIdx];

			if (field.m_bSrcRead) {
				
				string apiVarIdxParams;
				const char *pVarIdxParams;
				const char *pVarIndexes;
				char tmp[128];

				HtlAssert(intGbl.m_pIntGbl->m_dimenList.size() <= 2);
				if (intGbl.m_pIntGbl->m_dimenList.size() == 2) {
					sprintf(tmp, "ht_uint%d varIdx1, ht_uint%d varIdx2",
						FindLg2(intGbl.m_pIntGbl->m_dimenList[0].AsInt()-1), FindLg2(intGbl.m_pIntGbl->m_dimenList[1].AsInt()-1));
					apiVarIdxParams += tmp;

					pVarIdxParams = "varIdx1, varIdx2";
					pVarIndexes = "[varIdx1][varIdx2]";
				} else if (intGbl.m_pIntGbl->m_dimenList.size() == 1) {
					sprintf(tmp, "ht_uint%d varIdx1",
						FindLg2(intGbl.m_pIntGbl->m_dimenList[0].AsInt()-1));
					apiVarIdxParams += tmp;

					pVarIdxParams = "varIdx";
					pVarIndexes = "[varIdx]";
				} else {
					pVarIdxParams = "";
					pVarIndexes = "";
				}

				string apiFldIdxParams;
				const char *pFldIdxParams;
				const char *pFldIndexes;

				HtlAssert(field.m_dimenList.size() <= 2);
				if (field.m_dimenList.size() == 2) {
					sprintf(tmp, "ht_uint%d fldIdx1, ht_uint%d fldIdx2",
						FindLg2(field.m_dimenList[0].AsInt()-1), FindLg2(field.m_dimenList[1].AsInt()-1));
					apiFldIdxParams += tmp;

					pFldIdxParams = "fldIdx1, fldIdx2";
					pFldIndexes = "[fldIdx1][fldIdx2]";
				} else if (field.m_dimenList.size() == 1) {
					sprintf(tmp, "ht_uint%d fldIdx1",
						FindLg2(field.m_dimenList[0].AsInt()-1));
					apiFldIdxParams += tmp;

					pFldIdxParams = "fldIdx";
					pFldIndexes = "[fldIdx]";
				} else {
					pFldIdxParams = "";
					pFldIndexes = "";
				}

				g_appArgs.GetDsnRpt().AddItem("%s GR%s_%s_%s", field.m_type.c_str(),
					varRdStg, intGbl.m_gblName.c_str(), field.m_name.c_str());

				m_gblDefines.Append("#define GR%s_%s_%s",
					varRdStg, intGbl.m_gblName.c_str(), field.m_name.c_str());

				if (intGbl.m_pIntGbl->m_dimenList.size() > 0 || field.m_dimenList.size() > 0 || g_appArgs.IsGlobalReadParanEnabled()) {
					g_appArgs.GetDsnRpt().AddText("(%s%s%s)\n", apiVarIdxParams.c_str(),
						intGbl.m_pIntGbl->m_dimenList.size() > 0 && field.m_dimenList.size() > 0 ? ", " : "", apiFldIdxParams.c_str());

					m_gblDefines.Append("(%s%s%s)", pVarIdxParams,
						intGbl.m_pIntGbl->m_dimenList.size() > 0 && field.m_dimenList.size() > 0 ? ", " : "", pFldIdxParams);
				} else
					g_appArgs.GetDsnRpt().AddText("\n");

				if (intGbl.m_bShAddr) {
					const char *pDupStr = intGbl.m_allFieldList[field.m_gblFieldIdx].m_readerList.size() <= 2 ? "" : "_a";
					string fieldName = field.m_name.size() > 0 ? (string("_") + field.m_name.c_str()) : "";

					m_gblDefines.Append(" m_%s%s%s%s%s.read_mem()\n",
						intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, pVarIndexes, pFldIndexes);

				} else
					m_gblDefines.Append(" r_t%d_%sRdData%s.m_%s%s\n",
						rdStg, intGbl.m_gblName.c_str(), pVarIndexes, field.m_name.c_str(), pFldIndexes);
			}
		}

	}

	g_appArgs.GetDsnRpt().EndLevel();
	m_gblDefines.Append("#endif\n");
	m_gblDefines.Append("\n");

	// external ram outbound interfaces
	for (size_t ramIdx = 0; ramIdx < modInst.m_pMod->m_extInstRamList.size(); ramIdx += 1) {
		CRamPort &instRam =  modInst.m_pMod->m_extInstRamList[ramIdx];
		CRam * pExtGbl = instRam.m_pRam;

		bool bHasAddr = pExtGbl->m_addr0W.AsInt() + pExtGbl->m_addr1W.AsInt() + pExtGbl->m_addr2W.AsInt() > 0;

        string addrW = pExtGbl->m_pIntGbl->m_addr1W.AsStr();
        if (pExtGbl->m_pIntGbl->m_addr2W.size() > 0)
            addrW = pExtGbl->m_pIntGbl->m_addr2W.AsStr() + " + " + addrW;

		// generate external ram interface
		m_gblIoDecl.Append("\t// %s external ram outbound interface\n", pExtGbl->m_gblName.c_str());

		if (pExtGbl->m_bSrcRead || pExtGbl->m_bMifRead) {
			if (bHasAddr)
				m_gblIoDecl.Append("\tsc_out<sc_uint<%s> > o_%sTo%s_%sRdAddr;\n",
					addrW.c_str(),
					mod.m_modName.Lc().c_str(), instRam.m_instName.Uc().c_str(), pExtGbl->m_gblName.c_str());
			m_gblIoDecl.Append("\tsc_in<C%sTo%s_%sRdData> i_%sTo%s_%sRdData%s;\n",
				pExtGbl->m_modName.Uc().c_str(), mod.m_modName.Uc().c_str(), pExtGbl->m_gblName.c_str(),
				instRam.m_instName.Lc().c_str(), mod.m_modName.Uc().c_str(), pExtGbl->m_gblName.c_str(), pExtGbl->m_pIntGbl->m_dimenDecl.c_str());
		}
		if (pExtGbl->m_bSrcWrite || pExtGbl->m_bMifWrite) {

			m_gblIoDecl.Append("\tsc_out<C%sTo%s_%sWrEn> o_%sTo%s_%sWrEn%s;\n",
				mod.m_modName.Uc().c_str(), pExtGbl->m_modName.Uc().c_str(), pExtGbl->m_gblName.c_str(),
				mod.m_modName.Lc().c_str(), instRam.m_instName.Uc().c_str(), pExtGbl->m_gblName.c_str(), pExtGbl->m_pIntGbl->m_dimenDecl.c_str());
			if (bHasAddr)
				m_gblIoDecl.Append("\tsc_out<sc_uint<%s> > o_%sTo%s_%sWrAddr%s;\n",
					addrW.c_str(),
					mod.m_modName.Lc().c_str(), instRam.m_instName.Uc().c_str(), pExtGbl->m_gblName.c_str(), pExtGbl->m_pIntGbl->m_dimenDecl.c_str());
			m_gblIoDecl.Append("\tsc_out<C%sTo%s_%sWrData> o_%sTo%s_%sWrData%s;\n",
				mod.m_modName.Uc().c_str(), pExtGbl->m_modName.Uc().c_str(), pExtGbl->m_gblName.c_str(),
				mod.m_modName.Lc().c_str(), instRam.m_instName.Uc().c_str(), pExtGbl->m_gblName.c_str(), pExtGbl->m_pIntGbl->m_dimenDecl.c_str());
		}
		m_gblIoDecl.Append("\n");
	}

	// Register input interface signals
	if (mod.m_intGblList.size() == 0)
		return;

	// Generate ram access statement lists

	CHtCode &ramRegSm = mod.m_clkRate == eClk1x ? m_gblReg1x : m_gblReg2x;

	// external ram inbound interfaces
	for (size_t ramIdx = 0; ramIdx < mod.m_intGblList.size(); ramIdx += 1) {
		CRam &intGbl = *mod.m_intGblList[ramIdx];

		bool bHasAddr = intGbl.m_addr0W.AsInt() + intGbl.m_addr1W.AsInt() + intGbl.m_addr2W.AsInt() > 0;

		for (size_t extIdx = 0; extIdx < intGbl.m_gblPortList.size(); extIdx += 1) {
			CRamPort &gblPort = intGbl.m_gblPortList[extIdx];
			CRam * pExtGbl = gblPort.m_pRam;

			if (gblPort.m_bIntIntf || gblPort.m_bMifRead || gblPort.m_bMifWrite) continue;

			// generate external ram interface
			m_gblIoDecl.Append("\t// %s external ram inbound interface\n", pExtGbl->m_gblName.c_str());

            string addrW = pExtGbl->m_pIntGbl->m_addr1W.AsStr();
            if (pExtGbl->m_pIntGbl->m_addr2W.size() > 0)
                addrW = pExtGbl->m_pIntGbl->m_addr2W.AsStr() + " + " + addrW;

			if (pExtGbl->m_bSrcRead || pExtGbl->m_bMifRead) {
				if (bHasAddr)
					m_gblIoDecl.Append("\tsc_in<sc_uint<%s> > i_%sTo%s_%sRdAddr;\n",
						addrW.c_str(),
						gblPort.m_instName.Lc().c_str(), mod.m_modName.Uc().c_str(), pExtGbl->m_gblName.c_str());
				m_gblIoDecl.Append("\tsc_out<C%sTo%s_%sRdData> o_%sTo%s_%sRdData%s;\n",
					mod.m_modName.Uc().c_str(), gblPort.m_pModInfo->m_modName.Uc().c_str(), pExtGbl->m_gblName.c_str(),
					mod.m_modName.Lc().c_str(), gblPort.m_instName.Uc().c_str(), pExtGbl->m_gblName.c_str(), pExtGbl->m_pIntGbl->m_dimenDecl.c_str());
			}
			if (pExtGbl->m_bSrcWrite || pExtGbl->m_bMifWrite) {
				m_gblIoDecl.Append("\tsc_in<C%sTo%s_%sWrEn> i_%sTo%s_%sWrEn%s;\n",
					gblPort.m_pModInfo->m_modName.Uc().c_str(), mod.m_modName.Uc().c_str(), pExtGbl->m_gblName.c_str(),
					gblPort.m_instName.Lc().c_str(), mod.m_modName.Uc().c_str(), pExtGbl->m_gblName.c_str(), pExtGbl->m_pIntGbl->m_dimenDecl.c_str());
				if (bHasAddr)
					m_gblIoDecl.Append("\tsc_in<sc_uint<%s> > i_%sTo%s_%sWrAddr%s;\n",
						addrW.c_str(),
						gblPort.m_instName.Lc().c_str(), mod.m_modName.Uc().c_str(), pExtGbl->m_gblName.c_str(), pExtGbl->m_pIntGbl->m_dimenDecl.c_str());
				m_gblIoDecl.Append("\tsc_in<C%sTo%s_%sWrData> i_%sTo%s_%sWrData%s;\n",
					gblPort.m_pModInfo->m_modName.Uc().c_str(), mod.m_modName.Uc().c_str(), pExtGbl->m_gblName.c_str(),
					gblPort.m_instName.Lc().c_str(), mod.m_modName.Uc().c_str(), pExtGbl->m_gblName.c_str(), pExtGbl->m_pIntGbl->m_dimenDecl.c_str());
			}
			m_gblIoDecl.Append("\n");
		}
	}

	// Declare internal rams

	if (mod.m_intGblList.size() > 0) {
		m_gblRegDecl.Append("\t// Internal global variables\n");

		for (size_t ramIdx = 0; ramIdx < mod.m_intGblList.size(); ramIdx += 1) {
			CRam &intGbl = *mod.m_intGblList[ramIdx];

			string addrW;
			if (intGbl.m_addr0W.size() > 0) {
				addrW = intGbl.m_addr0W.AsStr();
				if (intGbl.m_addr1W.size() > 0)
					addrW += "+";
			}
			addrW += intGbl.m_addr1W.AsStr();
			if (intGbl.m_addr2W.size() > 0)
				addrW += " + " + intGbl.m_addr2W.AsStr();

			for (size_t fldIdx = 0; fldIdx < intGbl.m_allFieldList.size(); fldIdx += 1) {
				CField field = intGbl.m_allFieldList[fldIdx];

				for (size_t rdIdx = 0; rdIdx < field.m_readerList.size(); rdIdx += 2) {

					char dupCh[3] = { '_', (char)('a' + (rdIdx/2)), '\0' };
					const char *pDupStr = field.m_readerList.size() <= 2 ? "" : dupCh;

					string elemType = FindFieldType(field);

					const char *pBlockRamDoReg = (m_bBlockRamDoReg && field.m_ramType == eBlockRam) ? ", 0, true" : "";

					string fieldName = field.m_name.size() > 0 ? (string("_") + field.m_name.c_str()) : "";
					if (intGbl.m_addr0W.AsInt() + intGbl.m_addr1W.AsInt() + intGbl.m_addr2W.AsInt() == 0) {
						if (field.m_bOne1xWriteIntf != field.m_readerList[rdIdx].m_bOne1xReadIntf)
							m_gblRegDecl.Append("\tsc_signal<%s > r_%s%s%s%s%s;\n",
								elemType.c_str(),
								intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, intGbl.m_dimenDecl.c_str(), field.m_dimenDecl.c_str());
						else
							m_gblRegDecl.Append("\t%s r_%s%s%s%s%s;\n",
								elemType.c_str(),
								intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, intGbl.m_dimenDecl.c_str(), field.m_dimenDecl.c_str());
					} else if (intGbl.m_bReplAccess && mod.m_modInstList.size() > 1)
						m_gblRegDecl.Append("\tht_%s_ram<%s, %s-%d%s> m_%s%s%s%s%s;\n",
							field.m_ramType == eDistRam ? "dist" : "block",
							elemType.c_str(), addrW.c_str(), FindLg2(mod.m_modInstList.size()-1, false), pBlockRamDoReg,
							intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, intGbl.m_dimenDecl.c_str(), field.m_dimenDecl.c_str());
					else
						m_gblRegDecl.Append("\tht_%s_ram<%s, %s%s> m_%s%s%s%s%s;\n",
							field.m_ramType == eDistRam ? "dist" : "block",
							elemType.c_str(), addrW.c_str(), pBlockRamDoReg,
							intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, intGbl.m_dimenDecl.c_str(), field.m_dimenDecl.c_str());
				}
			}
		}
		m_gblRegDecl.NewLine();
	}

	// Declare ram interface structures

	for (size_t ramIdx = 0; ramIdx < mod.m_intGblList.size(); ramIdx += 1) {
		CRam & intGbl = *mod.m_intGblList[ramIdx];

		if (intGbl.m_bSrcRead || intGbl.m_bMifRead)
			GenRamIntfStruct(m_gblRegDecl, "\t", "C"+intGbl.m_gblName.Uc()+"RdData", *mod.m_intGblList[ramIdx], eStructRamRdData);

		if (intGbl.m_bSrcWrite || intGbl.m_bMifWrite) {
			GenRamIntfStruct(m_gblRegDecl, "\t", "C"+intGbl.m_gblName.Uc()+"WrEn", *mod.m_intGblList[ramIdx], eGenRamWrEn);
			GenRamIntfStruct(m_gblRegDecl, "\t", "C"+intGbl.m_gblName.Uc()+"WrData", *mod.m_intGblList[ramIdx], eGenRamWrData);
		}
	}

	// Clock internal rams

	bool bComment1x = true;
	bool bComment2x = true;
	for (size_t ramIdx = 0; ramIdx < mod.m_intGblList.size(); ramIdx += 1) {
		CRam &intGbl = *mod.m_intGblList[ramIdx];

		bool bHasAddr = intGbl.m_addr0W.AsInt() + intGbl.m_addr1W.AsInt() + intGbl.m_addr2W.AsInt() > 0;

		for (size_t fldIdx = 0; fldIdx < intGbl.m_allFieldList.size(); fldIdx += 1) {
			CField field = intGbl.m_allFieldList[fldIdx];

			for (size_t rdIdx = 0; rdIdx < field.m_readerList.size(); rdIdx += 1) {

				string fieldName = field.m_name.size() > 0 ? (string("_") + field.m_name.c_str()) : "";

				char dupCh[3] = { '_', (char)('a' + (rdIdx/2)), '\0' };
				const char *pDupStr = field.m_readerList.size() <= 2 ? "" : dupCh;

				CRamPort *pRamPort = field.m_readerList[rdIdx].m_pRamPort;

				bool bOne1xReadIntf = field.m_readerList[rdIdx].m_bOne1xReadIntf;
				bool bOne2xReadIntf = (rdIdx & 1) == 0 && rdIdx+1 < field.m_readerList.size() && field.m_readerList[rdIdx].m_pRamPort == field.m_readerList[rdIdx+1].m_pRamPort;
				bool bFirst1xReadIntf = field.m_readerList[rdIdx].m_bFirst1xReadIntf;

				bool bForce2xRead = m_bBlockRamDoReg && field.m_ramType == eBlockRam && 
					!((pRamPort->m_bOne1xReadIntf || pRamPort->m_bFirst1xReadIntf) && pRamPort->m_bSecond1xReadIntf);

				bool bReadClock1x = false;
				bool bReadClock2x = false;
				if (!intGbl.m_bShAddr || rdIdx >= 2) {
					bReadClock1x = bOne1xReadIntf && !bForce2xRead && (rdIdx & 1) == 0;
					bReadClock2x = (bOne2xReadIntf || bFirst1xReadIntf || bOne1xReadIntf && bForce2xRead) && (rdIdx & 1) == 0;
				} else if (rdIdx == 0) {
					bReadClock1x = mod.m_clkRate == eClk1x;
					bReadClock2x = mod.m_clkRate == eClk2x;
				}
				bool bWriteClock1x = field.m_bOne1xWriteIntf && (rdIdx & 1) == 0;
				bool bWriteClock2x = (field.m_bOne2xWriteIntf || field.m_bTwo1xWriteIntf) && (rdIdx & 1) == 0;

				if ((bReadClock1x || bWriteClock1x) && bComment1x) {
					bComment1x = false;
					m_gblReg1x.Append("\t// Internal ram clock\n");
				}
			
				vector<int> dimenList;
				string dimenDecl;
				string dimenIndex;

				GenDimenInfo(intGbl, field, dimenList, dimenDecl, dimenIndex);

				if (bReadClock1x || bWriteClock1x) {

					if (bHasAddr) {
						if (bReadClock1x && bWriteClock1x) {
							GenRamIndexLoops(m_gblReg1x, dimenList);
							m_gblReg1x.Append("\tm_%s%s%s%s.clock();\n", intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, dimenIndex.c_str());

						} else if (bReadClock1x) {
							GenRamIndexLoops(m_gblReg1x, dimenList);
							m_gblReg1x.Append("\tm_%s%s%s%s.read_clock();\n", intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, dimenIndex.c_str());

						} else if (bWriteClock1x) {
							GenRamIndexLoops(m_gblReg1x, dimenList);
							m_gblReg1x.Append("\tm_%s%s%s%s.write_clock();\n", intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, dimenIndex.c_str());
						}
					} else {
						if (bWriteClock1x) {
							GenRamIndexLoops(m_gblReg1x, dimenList);
							m_gblReg1x.Append("\tr_%s%s%s%s = c_%s%s%s%s;\n",
								intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, dimenIndex.c_str(),
								intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, dimenIndex.c_str());
						}
					}
				}

				if ((bReadClock2x || bWriteClock2x) && bComment2x) {
					bComment2x = false;
					m_gblReg2x.Append("\t// Internal ram clock\n");
				}

				if (bReadClock2x || bWriteClock2x) {
					if (bHasAddr) {
						if (bReadClock2x && bWriteClock2x) {

							GenRamIndexLoops(m_gblReg2x, dimenList);
							m_gblReg2x.Append("\tm_%s%s%s%s.clock();\n", intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, dimenIndex.c_str());

						} else if (bReadClock2x) {
							GenRamIndexLoops(m_gblReg2x, dimenList);
							m_gblReg2x.Append("\tm_%s%s%s%s.read_clock();\n", intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, dimenIndex.c_str());

						} else if (bWriteClock2x) {
							GenRamIndexLoops(m_gblReg2x, dimenList);
							m_gblReg2x.Append("\tm_%s%s%s%s.write_clock();\n", intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, dimenIndex.c_str());
						}
					} else {
						if (bWriteClock2x) {
							GenRamIndexLoops(m_gblReg2x, dimenList);
							m_gblReg2x.Append("\tr_%s%s%s%s = c_%s%s%s%s;\n",
								intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, dimenIndex.c_str(),
								intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, dimenIndex.c_str());
						}
					}
				}
			}
		}
	}
	m_gblReg1x.NewLine();
	m_gblReg2x.NewLine();

	bool bRamPre2xComment = false;
	char phaseStr[4] = { 'A'-1, '\0', '\0', '\0' };

	vector<CRdAddr> prevRdAddrList;
	for (size_t ramIdx = 0; ramIdx < mod.m_intGblList.size(); ramIdx += 1) {
		CRam &intGbl = *mod.m_intGblList[ramIdx];

		if (intGbl.m_addr0W.AsInt() == 0 && intGbl.m_addr1W.AsInt() == 0)
			continue;

		string addrW;
		if (intGbl.m_addr0W.size() > 0) {
			addrW = intGbl.m_addr0W.AsStr();
			if (intGbl.m_addr1W.size() > 0)
				addrW += "+";
		}
		addrW += intGbl.m_addr1W.AsStr();
		if (intGbl.m_addr2W.size() > 0)
			addrW += " + " + intGbl.m_addr2W.AsStr();

		char replStr[16] = { '\0' };
		if (intGbl.m_bReplAccess && mod.m_modInstList.size() > 1)
			sprintf(replStr, " >> %d", FindLg2(mod.m_modInstList.size()-1, false));

		for (size_t fldIdx = 0; fldIdx < intGbl.m_allFieldList.size(); fldIdx += 1) {
			CField &field = intGbl.m_allFieldList[fldIdx];

			string fieldName = field.m_name.size() > 0 ? (string("_") + field.m_name) : "";

			for (size_t rdIdx = 0; rdIdx < field.m_readerList.size(); rdIdx += 2) {

				if (rdIdx+1 < field.m_readerList.size() && field.m_readerList[rdIdx+1].m_pRamPort != 0
					&& field.m_readerList[rdIdx].m_pRamPort != field.m_readerList[rdIdx+1].m_pRamPort) {

					char dupCh[3] = { '_', (char)('a' + (rdIdx/2)), '\0' };
					const char *pDupStr = field.m_readerList.size() <= 2 ? "" : dupCh;

					int rdStg = mod.m_tsStg + intGbl.m_rdStg.AsInt() - 3;

					char phase0RdAddr[128];
					if (field.m_readerList[rdIdx].m_pRamPort->m_bIntIntf)
						sprintf(phase0RdAddr, "c_t%dx_%sRdAddr%s%s.read()",
							rdStg, intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr);
					else
						sprintf(phase0RdAddr, "i_%sTo%s_%sRdAddr",
							field.m_readerList[rdIdx].m_pRamPort->m_instName.Lc().c_str(), mod.m_modName.Uc().c_str(), intGbl.m_gblName.c_str());

					mod.m_phaseCnt += 1;
					if (phaseStr[0] < 'Z')
						phaseStr[0] += 1;
					else {
						phaseStr[0] = 'A';
						if (phaseStr[1] == '\0')
							phaseStr[1] = 'A';
						else
							phaseStr[1] += 1;
					}

					m_gblRegDecl.Append("\tbool r_phaseReset%s;\n", phaseStr);
					m_gblRegDecl.Append("\tsc_signal<bool> c_phaseReset%s;\n", phaseStr);
					m_gblRegDecl.Append("\tbool r_phase%s;\n", phaseStr);
					m_gblPostReg1x.Append("\tc_phaseReset%s = r_phaseReset%s;\n", phaseStr, phaseStr);
					m_gblReg2x.Append("\tht_attrib(equivalent_register_removal, r_phase%s, \"no\");\n", phaseStr);
					m_gblReg2x.Append("\tr_phase%s = c_phaseReset%s.read() || !r_phase%s;\n", phaseStr, phaseStr, phaseStr);

					if (!bRamPre2xComment) {
						m_gblPostInstr2x.Append("\t// Internal ram read address select\n");
						bRamPre2xComment = true;
					}

					// four cases for two read ports
					// 1. Internal Src + Internal Mif
					// 2. Internal Mif + External
					// 3. Internal Src + External
					// 4. External + External
					if (field.m_readerList[rdIdx+1].m_pRamPort->m_bMifRead) {
						// 1. Internal Src + Internal Mif
						m_gblPostInstr2x.Append("\tsc_uint<%s> c_%sRdAddr%s%s = r_phase%s ? %s%s : r_%sMifRdAddr_2x;\n", 
							addrW.c_str(),
							intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, phaseStr,
							phase0RdAddr, field.m_readerList[0].m_pRamPort->m_bSecond1xReadIntf ? ".read()" : "",
							intGbl.m_gblName.c_str());

						m_gblReg1x.Append("\tr_%sMifRdAddr = c_%sMifRdAddr;\n",
							intGbl.m_gblName.c_str(),
							intGbl.m_gblName.c_str());

						m_gblReg2x.Append("\tr_%sMifRdAddr_2x = r_%sMifRdAddr;\n",
							intGbl.m_gblName.c_str(),
							intGbl.m_gblName.c_str());

					} else if (field.m_readerList[rdIdx].m_pRamPort->m_bMifRead) {
						// 2. Internal Mif + External

						if (fldIdx == 0) {
							m_gblRegDecl.Append("\tsc_signal<sc_uint<%s> > r_%sMifRdAddr;\n",
								addrW.c_str(),
								intGbl.m_gblName.c_str());

							m_gblReg1x.Append("\tr_%sMifRdAddr = c_%sMifRdAddr;\n",
								intGbl.m_gblName.c_str(),
								intGbl.m_gblName.c_str());
						}

						m_gblPostInstr2x.Append("\tsc_uint<%s> c_%sRdAddr%s%s = r_phase%s ? r_%sMifRdAddr : r_%sTo%s_%sRdAddr_2x;\n", 
							addrW.c_str(),
							intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr,
							phaseStr,
							intGbl.m_gblName.c_str(),
							field.m_readerList[rdIdx+1].m_pRamPort->m_instName.Lc().c_str(), mod.m_modName.Uc().c_str(), intGbl.m_gblName.c_str());

					} else {
						m_gblPostInstr2x.Append("\tsc_uint<%s> c_%sRdAddr%s%s = r_phase%s ? %s%s : r_%sTo%s_%sRdAddr_2x;\n", 
							addrW.c_str(),
							intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, phaseStr,
							phase0RdAddr, field.m_readerList[0].m_pRamPort->m_bSecond1xReadIntf ? ".read()" : "",
							field.m_readerList[rdIdx+1].m_pRamPort->m_instName.Lc().c_str(), mod.m_modName.Uc().c_str(), intGbl.m_gblName.c_str());
					}
				} else {
					if (field.m_readerList[rdIdx].m_pRamPort->m_bMifRead) {

						if (mod.m_clkRate == eClk2x) {
							// 2x clock, MifRead
							m_gblRegDecl.Append("\tsc_uint<%s> r_%sMifRdAddr;\n",
								addrW.c_str(),
								intGbl.m_gblName.c_str());

							m_gblReg2x.Append("\tr_%sMifRdAddr = c_%sMifRdAddr;\n",
								intGbl.m_gblName.c_str(),
								intGbl.m_gblName.c_str());
						} else
							m_gblReg1x.Append("\tr_%sMifRdAddr = c_%sMifRdAddr;\n",
								intGbl.m_gblName.c_str(),
								intGbl.m_gblName.c_str());

					}
				}
			}
		}
	}

	// Generate 1x read addresses used in 2x module
	if (mod.m_clkRate == eClk1x) {
		for (size_t ramIdx = 0; ramIdx < mod.m_intGblList.size(); ramIdx += 1) {
			CRam &intGbl = *mod.m_intGblList[ramIdx];

			if (intGbl.m_addr0W.AsInt() == 0 && intGbl.m_addr1W.AsInt() == 0)
				continue;

			string addrW;
			if (intGbl.m_addr0W.size() > 0) {
				addrW = intGbl.m_addr0W.AsStr();
				if (intGbl.m_addr1W.size() > 0)
					addrW += "+";
			}
			addrW += intGbl.m_addr1W.AsStr();
			if (intGbl.m_addr2W.size() > 0)
				addrW += " + " + intGbl.m_addr2W.AsStr();

			int rdStg = mod.m_tsStg + intGbl.m_rdStg.AsInt() - 3;

			for (size_t fldIdx = 0; fldIdx < intGbl.m_allFieldList.size(); fldIdx += 1) {
				CField &field = intGbl.m_allFieldList[fldIdx];

				string fieldName = field.m_name.size() > 0 ? (string("_") + field.m_name) : "";

				size_t rdIdx = 0;

				if (field.m_readerList[rdIdx].m_bFirst1xReadIntf) {

					if (field.m_readerList[rdIdx].m_pRamPort->m_bIntIntf && !field.m_readerList[rdIdx].m_pRamPort->m_bMifRead) {
						m_gblRegDecl.Append("\tsc_signal<sc_uint<%s> > c_t%dx_%sRdAddr%s%s;\n",
							addrW.c_str(),
							rdStg, intGbl.m_gblName.c_str(), fieldName.c_str(),
							field.m_readerList.size() > 2 ? "_a" : "");

						char inStg[32];
						sprintf(inStg, "r_t%d", rdStg);
						char outStg[32];
						sprintf(outStg, "c_t%dx", rdStg);

						string ramAddrName = string(outStg) + "_" + intGbl.m_gblName.AsStr() + "RdAddr" + fieldName.c_str();
						if (field.m_readerList.size() > 2)
							ramAddrName += "_a";
						string addrW = "";

                        GenRamAddr(modInst, intGbl, &m_gblPostReg1x, addrW, ramAddrName, inStg, outStg, false, true);
					}
				}
			}
		}
	}

	m_gblRegDecl.Append("\t// External interface ram registers\n");

	// ram 2x read address register
	for (size_t ramIdx = 0; ramIdx < mod.m_intGblList.size(); ramIdx += 1) {
		CRam &intGbl = *mod.m_intGblList[ramIdx];

		if (intGbl.m_addr0W.AsInt() == 0 && intGbl.m_addr1W.AsInt() == 0)
			continue;

		string addrW;
		if (intGbl.m_addr0W.size() > 0) {
			addrW = intGbl.m_addr0W.AsStr();
			if (intGbl.m_addr1W.size() > 0)
				addrW += "+";
		}
		addrW += intGbl.m_addr1W.AsStr();
		if (intGbl.m_addr2W.size() > 0)
			addrW += " + " + intGbl.m_addr2W.AsStr();

		for (size_t fldIdx = 0; fldIdx < intGbl.m_allFieldList.size(); fldIdx += 1) {
			CField &field = intGbl.m_allFieldList[fldIdx];

			for (size_t rdIdx = 0; rdIdx < field.m_readerList.size(); rdIdx += 2) {

				char dupCh[3] = { '_', (char)('a' + (rdIdx/2)), '\0' };
				const char *pDupStr = field.m_readerList.size() <= 2 ? "" : dupCh;

				bool bFirst1xReadIntf = field.m_readerList[rdIdx].m_bFirst1xReadIntf;
                bool bSecond1xReadIntf = (rdIdx & 1) == 1 && field.m_readerList[rdIdx].m_pRamPort != 0 &&
					field.m_readerList[rdIdx-1].m_pRamPort != field.m_readerList[rdIdx].m_pRamPort;

				if ((bFirst1xReadIntf && field.m_readerList.size() > 1) || bSecond1xReadIntf) {
					m_gblRegDecl.Append("\tsc_uint<%s> r_%sRdAddr1_%s%s;\n",
						addrW.c_str(), 
						intGbl.m_gblName.c_str(), field.m_name.c_str(), pDupStr);

					m_gblReg2x.Append("\tht_attrib(equivalent_register_removal, r_%sRdAddr1_%s%s, \"no\");\n",
						intGbl.m_gblName.c_str(), field.m_name.c_str(), pDupStr);

					m_gblReg2x.Append("\tr_%sRdAddr1_%s%s = c_%sRdAddr_%s%s;\n",
						intGbl.m_gblName.c_str(), field.m_name.c_str(), pDupStr,
						intGbl.m_gblName.c_str(), field.m_name.c_str(), pDupStr);

					if (field.m_ramType != eBlockRam) {
						m_gblPostInstr2x.Append("\tsc_uint<%s> c_%sRdAddr1_%s%s = r_%sRdAddr1_%s%s;\n",
							addrW.c_str(), 
							intGbl.m_gblName.c_str(), field.m_name.c_str(), pDupStr, 
							intGbl.m_gblName.c_str(), field.m_name.c_str(), pDupStr);

						m_gblRegDecl.Append("\tsc_uint<%s> r_%sRdAddr2_%s%s;\n",
							addrW.c_str(), 
							intGbl.m_gblName.c_str(), field.m_name.c_str(), pDupStr);

						m_gblReg2x.Append("\tht_attrib(equivalent_register_removal, r_%sRdAddr2_%s%s, \"no\");\n",
							intGbl.m_gblName.c_str(), field.m_name.c_str(), pDupStr);

						m_gblReg2x.Append("\tr_%sRdAddr2_%s%s = c_%sRdAddr1_%s%s;\n",
							intGbl.m_gblName.c_str(), field.m_name.c_str(), pDupStr,
							intGbl.m_gblName.c_str(), field.m_name.c_str(), pDupStr);
					}
				}
			}
		}
	}

	for (size_t ramIdx = 0; ramIdx < mod.m_intGblList.size(); ramIdx += 1) {
		CRam &intGbl = *mod.m_intGblList[ramIdx];

		if (intGbl.m_addr0W.AsInt() == 0 && intGbl.m_addr1W.AsInt() == 0)
			continue;

		for (size_t ramPortIdx = 0; ramPortIdx < intGbl.m_gblPortList.size(); ramPortIdx += 1) {
			CRamPort gblPort = intGbl.m_gblPortList[ramPortIdx];

			if (gblPort.m_bIntIntf && gblPort.m_bSrcRead)
				continue;

			string addrW;
			if (intGbl.m_addr0W.size() > 0) {
				addrW = intGbl.m_addr0W.AsStr();
				if (intGbl.m_addr1W.size() > 0)
					addrW += "+";
			}
			addrW += intGbl.m_addr1W.AsStr();
			if (intGbl.m_addr2W.size() > 0)
				addrW += " + " + intGbl.m_addr2W.AsStr();
			
			if (gblPort.m_bSecond1xReadIntf || gblPort.m_bOne2xReadIntf) {
				if (gblPort.m_bMifRead || gblPort.m_bMifWrite) {
					m_gblRegDecl.Append("\tsc_uint<%s> r_%sMifRdAddr_2x;\n",
						addrW.c_str(),
						gblPort.m_pRam->m_gblName.c_str());

					m_gblRegDecl.Append("\tsc_signal<sc_uint<%s> > r_%sMifRdAddr;\n",
						addrW.c_str(),
						gblPort.m_pRam->m_gblName.c_str());
				} else {
					m_gblRegDecl.Append("\tsc_uint<%s> r_%sTo%s_%sRdAddr_2x;\n",
						addrW.c_str(),
						gblPort.m_instName.Lc().c_str(), mod.m_modName.Uc().c_str(), gblPort.m_pRam->m_gblName.c_str());

					m_gblReg2x.Append("\tr_%sTo%s_%sRdAddr_2x = i_%sTo%s_%sRdAddr;\n",
						gblPort.m_instName.Lc().c_str(), mod.m_modName.Uc().c_str(), gblPort.m_pRam->m_gblName.c_str(),
						gblPort.m_instName.Lc().c_str(), mod.m_modName.Uc().c_str(), gblPort.m_pRam->m_gblName.c_str());
				}
			} else if (gblPort.m_bOne1xReadIntf) {
				if (gblPort.m_bMifRead || gblPort.m_bMifWrite) {
					m_gblRegDecl.Append("\tsc_uint<%s> r_%sMifRdAddr;\n",
						addrW.c_str(),
						gblPort.m_pRam->m_gblName.c_str());
				}
			}
		}

		m_gblReg1x.NewLine();
		m_gblReg2x.NewLine();
	}

	// read memories
	vector<string> prevRdAddr1xList;
	for (size_t ramIdx = 0; ramIdx < mod.m_intGblList.size(); ramIdx += 1) {
		CRam &intGbl = *mod.m_intGblList[ramIdx];

		bool bHasAddr = intGbl.m_addr0W.AsInt() + intGbl.m_addr1W.AsInt() + intGbl.m_addr2W.AsInt() > 0;

		string addrW;
		if (intGbl.m_addr0W.size() > 0) {
			addrW = intGbl.m_addr0W.AsStr();
			if (intGbl.m_addr1W.size() > 0)
				addrW += "+";
		}
		addrW += intGbl.m_addr1W.AsStr();
		if (intGbl.m_addr2W.size() > 0)
			addrW += " + " + intGbl.m_addr2W.AsStr();
			
		int rdStg = mod.m_tsStg + intGbl.m_rdStg.AsInt() - 3;

		char replStr[16] = { '\0' };
		if (intGbl.m_bReplAccess && mod.m_modInstList.size() > 1)
			sprintf(replStr, " >> %d", FindLg2(mod.m_modInstList.size()-1, false));

		for (size_t fldIdx = 0; fldIdx < intGbl.m_allFieldList.size(); fldIdx += 1) {
			CField &field = intGbl.m_allFieldList[fldIdx];

			vector<int> dimenList;
			string		dimenDecl;
			string		dimenIndex;

			GenDimenInfo(intGbl, field, dimenList, dimenDecl, dimenIndex);

			string addrW;
			if (intGbl.m_addr0W.size() > 0) {
				addrW = intGbl.m_addr0W.AsStr();
				if (intGbl.m_addr1W.size() > 0)
					addrW += "+";
			}
			addrW += intGbl.m_addr1W.AsStr();
			if (intGbl.m_addr2W.size() > 0)
				addrW += " + " + intGbl.m_addr2W.AsStr();

			string fieldName = field.m_name.size() > 0 ? (string("_") + field.m_name) : "";

			for (size_t rdIdx = 0; rdIdx < field.m_readerList.size(); rdIdx += 2) {

				char dupCh[3] = { '_', (char)('a' + (rdIdx/2)), '\0' };
				const char *pDupStr = field.m_readerList.size() <= 2 ? "" : dupCh;

                bool bOne1xReadIntf = field.m_readerList[rdIdx].m_bOne1xReadIntf;
				bool bFirst1xReadIntf = field.m_readerList[rdIdx].m_bFirst1xReadIntf;
				bool bSecond1xReadIntf = (rdIdx & 1) == 1 && field.m_readerList[rdIdx].m_pRamPort != 0 &&
					field.m_readerList[rdIdx-1].m_pRamPort != field.m_readerList[rdIdx].m_pRamPort;

				if (field.m_readerList[rdIdx].m_bShAddr) {
					int rdStg = mod.m_tsStg + intGbl.m_rdStg.AsInt() - 1 - (field.m_ramType == eBlockRam ? 1 : 0);
					if (field.m_ramType == eBlockRam) {
						if (mod.m_clkRate == eClk2x) {
							GenRamIndexLoops(m_gblPostInstr2x, dimenList);
							m_gblPostInstr2x.Append("\tm_%s%s%s%s.read_addr(",
								intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, dimenIndex.c_str());

							if (intGbl.m_addr1W.AsInt() > 0 && intGbl.m_addr2W.AsInt() > 0)
								m_gblPostInstr2x.Append(" ((ht_uint%d)c_t%d_%s, (ht_uint%d)c_t%d_%s) ",
									intGbl.m_addr1W.AsInt(), rdStg, intGbl.m_addr1Name.c_str(),
									intGbl.m_addr2W.AsInt(), rdStg, intGbl.m_addr2Name.c_str());
							else if (intGbl.m_addr1W.AsInt() > 0)
								m_gblPostInstr2x.Append("c_t%d_%s",
									rdStg, intGbl.m_addr1Name.c_str());
							else
								m_gblPostInstr2x.Append("c_t%d_%s",
									rdStg, intGbl.m_addr2Name.c_str());

							m_gblPostInstr2x.Append(");\n");
						} else {
							GenRamIndexLoops(m_gblPostInstr1x, dimenList);
							m_gblPostInstr1x.Append("\tm_%s%s%s%s.read_addr(",
								intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, dimenIndex.c_str());

							if (intGbl.m_addr1W.AsInt() > 0 && intGbl.m_addr2W.AsInt() > 0)
								m_gblPostInstr1x.Append(" ((ht_uint%d)c_t%d_%s, (ht_uint%d)c_t%d_%s) ",
									intGbl.m_addr1W.AsInt(), rdStg, intGbl.m_addr1Name.c_str(),
									intGbl.m_addr2W.AsInt(), rdStg, intGbl.m_addr2Name.c_str());
							else if (intGbl.m_addr1W.AsInt() > 0)
								m_gblPostInstr1x.Append("c_t%d_%s",
									rdStg, intGbl.m_addr1Name.c_str());
							else
								m_gblPostInstr1x.Append("c_t%d_%s",
									rdStg, intGbl.m_addr2Name.c_str());

							m_gblPostInstr1x.Append(");\n");
						}
					} else {
						if (mod.m_clkRate == eClk2x) {
							GenRamIndexLoops(m_gblPreInstr2x, dimenList);
							m_gblPreInstr2x.Append("\tm_%s%s%s%s.read_addr(",
								intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, dimenIndex.c_str());

							if (intGbl.m_addr1W.AsInt() > 0 && intGbl.m_addr2W.AsInt() > 0)
								m_gblPreInstr2x.Append(" ((ht_uint%d)c_t%d_%s, (ht_uint%d)c_t%d_%s) ",
									intGbl.m_addr1W.AsInt(), rdStg, intGbl.m_addr1Name.c_str(),
									intGbl.m_addr2W.AsInt(), rdStg, intGbl.m_addr2Name.c_str());
							else if (intGbl.m_addr1W.AsInt() > 0)
								m_gblPreInstr2x.Append("c_t%d_%s",
									rdStg, intGbl.m_addr1Name.c_str());
							else
								m_gblPreInstr2x.Append("c_t%d_%s",
									rdStg, intGbl.m_addr2Name.c_str());

							m_gblPreInstr2x.Append(");\n");
						} else {
							GenRamIndexLoops(m_gblPreInstr1x, dimenList);
							m_gblPreInstr1x.Append("\tm_%s%s%s%s.read_addr(",
								intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, dimenIndex.c_str());

							if (intGbl.m_addr1W.AsInt() > 0 && intGbl.m_addr2W.AsInt() > 0)
								m_gblPreInstr1x.Append(" ((ht_uint%d)c_t%d_%s, (ht_uint%d)c_t%d_%s) ",
									intGbl.m_addr1W.AsInt(), rdStg, intGbl.m_addr1Name.c_str(),
									intGbl.m_addr2W.AsInt(), rdStg, intGbl.m_addr2Name.c_str());
							else if (intGbl.m_addr1W.AsInt() > 0)
								m_gblPreInstr1x.Append("c_t%d_%s",
									rdStg, intGbl.m_addr1Name.c_str());
							else
								m_gblPreInstr1x.Append("c_t%d_%s",
									rdStg, intGbl.m_addr2Name.c_str());

							m_gblPreInstr1x.Append(");\n");
						}
					}
			
				} else if (bOne1xReadIntf) {

					if (bHasAddr) {
						string rdAddrName;
						if (field.m_readerList[rdIdx].m_pRamPort->m_bMifRead) {
							char buf[128];
							sprintf(buf, "r_%sMifRdAddr", intGbl.m_gblName.c_str());
							rdAddrName = buf;
						} else if (field.m_readerList[rdIdx].m_pRamPort->m_bIntIntf) {
							int rdStg = mod.m_tsStg + intGbl.m_rdStg.AsInt() - 1 - (field.m_ramType == eBlockRam ? 1 : 0);

							CRdAddr rdAddr(intGbl.m_addr1Name, intGbl.m_addr2Name, field.m_ramType, rdStg, addrW);
							size_t i;
							for (i = 0; i < prevRdAddrList.size(); i += 1) {
								if (prevRdAddrList[i] == rdAddr)
									break;
							}

							if (i < prevRdAddrList.size())
								rdAddrName = prevRdAddrList[i].m_rdAddrName;
							else {
								char inStg[32];
								sprintf(inStg, "r_t%d", rdStg-1);
								char outStg[32];
								sprintf(outStg, "c_t%d", rdStg-1);

								string accessSelName = string(outStg) + "_" + intGbl.m_gblName.AsStr() + "RdAddrSel";

								// if multiple groups have read address variable in htPriv then must select which to use
								rdAddrName = GenRamAddr(modInst, intGbl, &m_gblPostInstr1x, addrW, accessSelName, inStg, outStg, false);

								rdAddr.m_rdAddrName = rdAddrName;
								prevRdAddrList.push_back(rdAddr);
							}
						} else {

							char rdAddrStr[128];
							if (field.m_ramType == eBlockRam)
								sprintf(rdAddrStr, "i_%sTo%s_%sRdAddr.read()",
									field.m_readerList[rdIdx].m_pRamPort->m_instName.Lc().c_str(), mod.m_modName.Uc().c_str(), intGbl.m_gblName.c_str());
							else {
								sprintf(rdAddrStr, "r_%sTo%s_%sRdAddr_1x",
									field.m_readerList[rdIdx].m_pRamPort->m_instName.Lc().c_str(), mod.m_modName.Uc().c_str(), intGbl.m_gblName.c_str());

								size_t i;
								for (i = 0; i < prevRdAddr1xList.size(); i += 1) {
									if (prevRdAddr1xList[i] == rdAddrStr)
										break;
								}

								if (i == prevRdAddr1xList.size()) {
									m_gblRegDecl.Append("\tsc_uint<%s> r_%sTo%s_%sRdAddr_1x;\n",
										addrW.c_str(),
										field.m_readerList[rdIdx].m_pRamPort->m_instName.Lc().c_str(), mod.m_modName.Uc().c_str(), intGbl.m_gblName.c_str());

									m_gblReg1x.Append("\tr_%sTo%s_%sRdAddr_1x = i_%sTo%s_%sRdAddr;\n",
										field.m_readerList[rdIdx].m_pRamPort->m_instName.Lc().c_str(), mod.m_modName.Uc().c_str(), intGbl.m_gblName.c_str(),
										field.m_readerList[rdIdx].m_pRamPort->m_instName.Lc().c_str(), mod.m_modName.Uc().c_str(), intGbl.m_gblName.c_str());

									prevRdAddr1xList.push_back(rdAddrStr);
								}
							}

							rdAddrName = rdAddrStr;
						}

						CRamPort *pRamPort = field.m_readerList[rdIdx].m_pRamPort;
						bool bForce2xRead = m_bBlockRamDoReg && field.m_ramType == eBlockRam && 
							!((pRamPort->m_bOne1xReadIntf || pRamPort->m_bFirst1xReadIntf) && pRamPort->m_bSecond1xReadIntf);

						if (bForce2xRead) {
							GenRamIndexLoops(m_gblPostInstr2x, dimenList);
							m_gblPostInstr2x.Append("\tm_%s%s%s%s.read_addr(%s%s);\n",
								intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, dimenIndex.c_str(),
								rdAddrName.c_str(), replStr);
						} else {
							GenRamIndexLoops(m_gblPostInstr1x, dimenList);
							m_gblPostInstr1x.Append("\tm_%s%s%s%s.read_addr(%s%s);\n",
								intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, dimenIndex.c_str(),
								rdAddrName.c_str(), replStr);
						}
					}

				} else if (field.m_readerList[rdIdx].m_pRamPort->m_bOne2xReadIntf) {

					if (bHasAddr) {
						string rdAddrName;
						char rdAddrStr[128];
						if (field.m_readerList[rdIdx].m_pRamPort->m_bIntIntf) {
							if (field.m_readerList[rdIdx].m_pRamPort->m_bMifRead)
								rdAddrName = "r_" + intGbl.m_gblName.AsStr() + "MifRdAddr";
							else {
								int rdStg = mod.m_tsStg + intGbl.m_rdStg.AsInt() - 1 - (field.m_ramType == eBlockRam ? 1 : 0);

								CRdAddr rdAddr(intGbl.m_addr1Name, intGbl.m_addr2Name, field.m_ramType, rdStg, addrW);
								size_t i;
								for (i = 0; i < prevRdAddrList.size(); i += 1) {
									if (prevRdAddrList[i] == rdAddr)
										break;
								}

								if (i < prevRdAddrList.size())
									rdAddrName = prevRdAddrList[i].m_rdAddrName;
								else {
									// if multiple groups have read address variable in htPriv then must select which to use

									char inStg[32];
									sprintf(inStg, "r_t%d", rdStg-1);
									char outStg[32];
									sprintf(outStg, "c_t%d", rdStg-1);

									string accessSelName = string(outStg) + "_" + intGbl.m_gblName.AsStr() + "RdAddrSel";
									rdAddrName = GenRamAddr(modInst, intGbl, &m_gblPostInstr1x, addrW, accessSelName, inStg, outStg, false);

									rdAddr.m_rdAddrName = rdAddrName;
									prevRdAddrList.push_back(rdAddr);
								}
							}
						} else {
							if (field.m_ramType == eBlockRam) {
									sprintf(rdAddrStr, "i_%sTo%s_%sRdAddr.read()",
										field.m_readerList[rdIdx].m_pRamPort->m_instName.Lc().c_str(), mod.m_modName.Uc().c_str(), intGbl.m_gblName.c_str());
									rdAddrName = rdAddrStr;
							} else {
								sprintf(rdAddrStr, "r_%sTo%s_%sRdAddr_2x",
									field.m_readerList[rdIdx].m_pRamPort->m_instName.Lc().c_str(), mod.m_modName.Uc().c_str(), intGbl.m_gblName.c_str());
								rdAddrName = rdAddrStr;
							}
						}

						GenRamIndexLoops(m_gblPostInstr2x, dimenList);
						m_gblPostInstr2x.Append("\tm_%s%s%s%s.read_addr(%s%s);\n",
							intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, dimenIndex.c_str(),
							rdAddrName.c_str(), replStr);
					}

				} else if (bFirst1xReadIntf || bSecond1xReadIntf) {

					if (bHasAddr) {
						GenRamIndexLoops(m_gblPostInstr2x, dimenList);
						if (field.m_readerList.size() == 1)
							m_gblPostInstr2x.Append("\tm_%s%s%s%s.read_addr(c_t%dx_%sRdAddr%s.read()%s);\n",
								intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, dimenIndex.c_str(),
								rdStg, intGbl.m_gblName.c_str(), fieldName.c_str(), replStr);
						else
							m_gblPostInstr2x.Append("\tm_%s%s%s%s.read_addr(r_%sRdAddr%c%s%s%s);\n",
								intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, dimenIndex.c_str(),
								intGbl.m_gblName.c_str(), field.m_ramType == eBlockRam ? '1' : '2', fieldName.c_str(), pDupStr, replStr);
					}

					string elemType = FindFieldType(field);

					if (m_bBlockRamDoReg && field.m_ramType == eBlockRam) {
						m_gblRegDecl.Append("\tsc_signal<%s > c_%sRdData%s%s_2x%s;\n",
							elemType.c_str(), intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, dimenDecl.c_str());

						GenRamIndexLoops(m_gblPostInstr2x, dimenList);
						m_gblPostInstr2x.Append("\tc_%sRdData%s%s_2x%s = m_%s%s%s%s.read_mem();\n",
							intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, dimenIndex.c_str(),
							intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, dimenIndex.c_str());

					} else {

						if (dimenList.size() == 0) {

							if (bHasAddr)
								m_gblPostInstr2x.Append("\t%s c_%sRdData%s%s = m_%s%s%s.read_mem();\n",
									elemType.c_str(),
									intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr,
									intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr);
							else
								m_gblPostInstr2x.Append("\t%s c_%sRdData%s%s = r_%s%s%s;\n",
									elemType.c_str(),
									intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr,
									intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr);

						} else {

							m_gblPostInstr2x.Append("\t%s c_%sRdData%s%s%s;\n",
								elemType.c_str(),
								intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, dimenDecl.c_str());

							GenRamIndexLoops(m_gblPostInstr2x, dimenList);
							if (bHasAddr)
								m_gblPostInstr2x.Append("\tc_%sRdData%s%s%s = m_%s%s%s%s.read_mem();\n",
									intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, dimenIndex.c_str(),
									intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, dimenIndex.c_str());
							else
								m_gblPostInstr2x.Append("\tc_%sRdData%s%s%s = r_%s%s%s%s;\n",
									intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, dimenIndex.c_str(),
									intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, dimenIndex.c_str());
						}
					}
				}

				if (bOne1xReadIntf) {
					string elemType = FindFieldType(field);

					CRamPort &gblPort = *field.m_readerList[rdIdx].m_pRamPort;
					bool bCont = (gblPort.m_bOne1xReadIntf || gblPort.m_bFirst1xReadIntf) && gblPort.m_bSecond1xReadIntf;

					if (m_bBlockRamDoReg && field.m_ramType == eBlockRam) {
						if (bCont) {
							m_gblRegDecl.Append("\tsc_signal<%s > c_%sRdData%s%s_1x%s;\n",
								elemType.c_str(), intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, dimenDecl.c_str());

							GenRamIndexLoops(m_gblPostInstr1x, dimenList);
							m_gblPostInstr1x.Append("\tc_%sRdData%s%s_1x%s = m_%s%s%s%s.read_mem();\n",
								intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, dimenIndex.c_str(),
								intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, dimenIndex.c_str());

						} else {
							m_gblRegDecl.Append("\tsc_signal<%s > c_%sRdData%s%s_2x%s;\n",
								elemType.c_str(), intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, dimenDecl.c_str());

							GenRamIndexLoops(m_gblPostInstr2x, dimenList);
							if (bHasAddr)
								m_gblPostInstr2x.Append("\tc_%sRdData%s%s_2x%s = m_%s%s%s%s.read_mem();\n",
									intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, dimenIndex.c_str(),
									intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, dimenIndex.c_str());
							else
								m_gblPostInstr2x.Append("\tc_%sRdData%s%s_2x%s = r_%s%s%s%s;\n",
									intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, dimenIndex.c_str(),
									intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, dimenIndex.c_str());

						}
					} else {
						if (dimenList.size() == 0) {
							if (bHasAddr)
								m_gblPostInstr1x.Append("\t%s c_%sRdData%s%s = m_%s%s%s.read_mem();\n",
									elemType.c_str(),
									intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr,
									intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr);
							else
								m_gblPostInstr1x.Append("\t%s c_%sRdData%s%s = r_%s%s%s;\n",
									elemType.c_str(),
									intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr,
									intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr);
						} else {
							m_gblPostInstr1x.Append("\t%s c_%sRdData%s%s%s;\n",
								elemType.c_str(),
								intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, dimenDecl.c_str());

							GenRamIndexLoops(m_gblPostInstr1x, dimenList);
							if (bHasAddr)
								m_gblPostInstr1x.Append("\tc_%sRdData%s%s%s = m_%s%s%s%s.read_mem();\n",
									intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, dimenIndex.c_str(),
									intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, dimenIndex.c_str());
							else
								m_gblPostInstr1x.Append("\tc_%sRdData%s%s%s = r_%s%s%s%s;\n",
									intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, dimenIndex.c_str(),
									intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, dimenIndex.c_str());
						}
					}
				}

				if (field.m_readerList[rdIdx].m_pRamPort->m_bOne2xReadIntf) {
					string elemType = FindFieldType(field);

					m_gblRegDecl.Append("\t%s c_%sRdData%s%s%s;\n",
						elemType.c_str(), intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, dimenDecl.c_str());

					GenRamIndexLoops(m_gblPostInstr2x, dimenList);
					if (bHasAddr)
						m_gblPostInstr2x.Append("\tc_%sRdData%s%s%s = m_%s%s%s%s.read_mem();\n",
							intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, dimenIndex.c_str(),
							intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, dimenIndex.c_str());
					else
						m_gblPostInstr2x.Append("\tc_%sRdData%s%s%s = r_%s%s%s%s;\n",
							intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, dimenIndex.c_str(),
							intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, dimenIndex.c_str());
				}
			}
		}
	}
	m_gblPostInstr1x.NewLine();
	m_gblPostInstr2x.NewLine();

	////////////////////////////////////////////
	// ram 2x read first data register

	for (size_t ramIdx = 0; ramIdx < mod.m_intGblList.size(); ramIdx += 1) {
		CRam &intGbl = *mod.m_intGblList[ramIdx];

		for (size_t fldIdx = 0; fldIdx < intGbl.m_allFieldList.size(); fldIdx += 1) {
			CField &field = intGbl.m_allFieldList[fldIdx];

			string fieldName = field.m_name.size() > 0 ? (string("_") + field.m_name) : "";

			vector<int> dimenList;
			string		dimenDecl;
			string		dimenIndex;

			GenDimenInfo(intGbl, field, dimenList, dimenDecl, dimenIndex);

			if (m_bBlockRamDoReg && field.m_ramType == eBlockRam)
				continue;

			for (size_t rdIdx = 0; rdIdx < field.m_readerList.size(); rdIdx += 2) {

				char dupCh[3] = { '_', (char)('a' + (rdIdx/2)), '\0' };
				const char *pDupStr = field.m_readerList.size() <= 2 ? "" : dupCh;

 				bool bFirst1xReadIntf = field.m_readerList[rdIdx].m_bFirst1xReadIntf;
				bool bSecond1xReadIntf = (rdIdx & 1) == 1 && field.m_readerList[rdIdx].m_pRamPort != 0 &&
					field.m_readerList[rdIdx-1].m_pRamPort != field.m_readerList[rdIdx].m_pRamPort;

				if (bFirst1xReadIntf || bSecond1xReadIntf) {
					string elemType = FindFieldType(field);

					m_gblRegDecl.Append("\tsc_signal<%s > r_%sRdData%s%s%s;\n",
						elemType.c_str(), intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, dimenDecl.c_str());

					GenRamIndexLoops(m_gblReg2x, dimenList);
					m_gblReg2x.Append("\tr_%sRdData%s%s%s = c_%sRdData%s%s%s;\n",
						intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, dimenIndex.c_str(),
						intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, dimenIndex.c_str());
				}
			}
		}
	}

	for (size_t ramIdx = 0; ramIdx < mod.m_intGblList.size(); ramIdx += 1) {
		CRam &intGbl = *mod.m_intGblList[ramIdx];

		for (size_t ramPortIdx = 0; ramPortIdx < intGbl.m_gblPortList.size(); ramPortIdx += 1) {
			CRamPort gblPort = intGbl.m_gblPortList[ramPortIdx];

			int rdStg = mod.m_tsStg + intGbl.m_rdStg.AsInt()-1;

			string ramType;
			if (gblPort.m_pRam->m_bGlobal)
				ramType = gblPort.m_pRam->m_type;
			else
				ramType = "C" + gblPort.m_pRam->m_gblName.Uc() + "RdData";

			if (gblPort.m_bFirst1xReadIntf || gblPort.m_bSecond1xReadIntf || gblPort.m_bOne1xReadIntf || gblPort.m_bOne2xReadIntf) {
				if (gblPort.m_bIntIntf && !gblPort.m_bMifRead) {

					m_gblRegDecl.Append("\t%s %sr_t%d_%sRdData%s;\n",
						ramType.c_str(), gblPort.m_pRam->m_bPrivGbl ? "ht_noload " : "", rdStg,
						gblPort.m_pRam->m_gblName.c_str(), intGbl.m_dimenDecl.c_str());

					GenRamIndexLoops(ramRegSm, "", intGbl);
					ramRegSm.Append("\tr_t%d_%sRdData%s = c_t%d_%sRdData%s;\n",
						rdStg, intGbl.m_gblName.c_str(), intGbl.m_dimenIndex.c_str(),
						rdStg-1, intGbl.m_gblName.c_str(), intGbl.m_dimenIndex.c_str());

				} else if ((gblPort.m_bFirst1xReadIntf || gblPort.m_bOne1xReadIntf) != gblPort.m_bSecond1xReadIntf || gblPort.m_bOne2xReadIntf) {

					if (gblPort.m_bMifRead/* || gblPort.m_bMifWrite*/) {
						if ((gblPort.m_bFirst1xReadIntf || gblPort.m_bOne1xReadIntf) && !gblPort.m_bSecond1xReadIntf ||
							gblPort.m_bOne2xReadIntf && mod.m_clkRate == eClk2x) 
						{
							m_gblRegDecl.Append("\tC%sRdData r_%sMifRdData%s;\n",
								gblPort.m_pRam->m_gblName.Uc().c_str(),
								gblPort.m_pRam->m_gblName.c_str(), intGbl.m_dimenDecl.c_str());

							if (mod.m_clkRate == eClk2x) {
								GenRamIndexLoops(m_gblReg2x, "", intGbl);
								m_gblReg2x.Append("\tr_%sMifRdData%s = c_%sMifRdData%s;\n",
									intGbl.m_gblName.c_str(), intGbl.m_dimenIndex.c_str(),
									intGbl.m_gblName.c_str(), intGbl.m_dimenIndex.c_str());
							} else {
								GenRamIndexLoops(m_gblReg1x, "", intGbl);
								m_gblReg1x.Append("\tr_%sMifRdData%s = c_%sMifRdData%s;\n",
									intGbl.m_gblName.c_str(), intGbl.m_dimenIndex.c_str(),
									intGbl.m_gblName.c_str(), intGbl.m_dimenIndex.c_str());
							}
						} else {
							m_gblRegDecl.Append("\tsc_signal<C%sRdData> r_%sMifRdData_2x%s;\n",
								gblPort.m_pRam->m_gblName.Uc().c_str(),
								gblPort.m_pRam->m_gblName.c_str(), intGbl.m_dimenDecl.c_str());

							GenRamIndexLoops(m_gblReg2x, "", intGbl);
							m_gblReg2x.Append("\tr_%sMifRdData_2x%s = c_%sMifRdData%s;\n",
								intGbl.m_gblName.c_str(), intGbl.m_dimenIndex.c_str(),
								intGbl.m_gblName.c_str(), intGbl.m_dimenIndex.c_str());
						}
					} else {
						m_gblRegDecl.Append("\tC%sTo%s_%sRdData r_%sTo%s_%sRdData%s;\n",
							mod.m_modName.Uc().c_str(), gblPort.m_pModInfo->m_modName.Uc().c_str(), gblPort.m_pRam->m_gblName.c_str(),
							mod.m_modName.Lc().c_str(), gblPort.m_instName.Uc().c_str(), gblPort.m_pRam->m_gblName.c_str(), intGbl.m_dimenDecl.c_str());

						if ((gblPort.m_bFirst1xReadIntf || gblPort.m_bOne1xReadIntf) && !gblPort.m_bSecond1xReadIntf) {
							GenRamIndexLoops(m_gblReg1x, "", intGbl);
							m_gblReg1x.Append("\tr_%sTo%s_%sRdData%s = c_%sTo%s_%sRdData%s;\n",
								mod.m_modName.Lc().c_str(), gblPort.m_instName.Uc().c_str(), intGbl.m_gblName.c_str(), intGbl.m_dimenIndex.c_str(),
								mod.m_modName.Lc().c_str(), gblPort.m_instName.Uc().c_str(), intGbl.m_gblName.c_str(), intGbl.m_dimenIndex.c_str());
						} else {
							GenRamIndexLoops(m_gblReg2x, "", intGbl);
							m_gblReg2x.Append("\tr_%sTo%s_%sRdData%s = c_%sTo%s_%sRdData%s;\n",
								mod.m_modName.Lc().c_str(), gblPort.m_instName.Uc().c_str(), intGbl.m_gblName.c_str(), intGbl.m_dimenIndex.c_str(),
								mod.m_modName.Lc().c_str(), gblPort.m_instName.Uc().c_str(), intGbl.m_gblName.c_str(), intGbl.m_dimenIndex.c_str());
						}
					}
				} else {
					// register individual fields for continuous assignment

					for (size_t fldIdx = 0; fldIdx < intGbl.m_allFieldList.size(); fldIdx += 1) {
						CField &field = intGbl.m_allFieldList[fldIdx];

						string fieldName = field.m_name.size() > 0 ? (string("_") + field.m_name) : "";

						for (size_t rdIdx = 0; rdIdx < field.m_readerList.size(); rdIdx += 1) {
							CRamPort *pInstRam = field.m_readerList[rdIdx].m_pRamPort;

							if (pInstRam == 0 || pInstRam->m_instName.Lc() != gblPort.m_instName.Lc()) continue;

							char dupCh[3] = { '_', (char)('a' + (rdIdx/2)), '\0' };
							const char *pDupStr = field.m_readerList.size() <= 2 ? "" : dupCh;

							bool bOne1xReadIntf = field.m_readerList[rdIdx].m_bOne1xReadIntf;
                                                        /* bool bOne2xReadIntf = (rdIdx & 1) == 0 && rdIdx+1 < field.m_readerList.size() && field.m_readerList[rdIdx].m_pRamPort == field.m_readerList[rdIdx+1].m_pRamPort; */
							bool bFirst1xReadIntf = field.m_readerList[rdIdx].m_bFirst1xReadIntf;
							bool bSecond1xReadIntf = (rdIdx & 1) == 1 && field.m_readerList[rdIdx].m_pRamPort != 0 &&
								field.m_readerList[rdIdx-1].m_pRamPort != field.m_readerList[rdIdx].m_pRamPort;

							string elemType = FindFieldType(field);

							if (bOne1xReadIntf) {

								if (!(m_bBlockRamDoReg && field.m_ramType == eBlockRam)) {
									m_gblRegDecl.Append("\tsc_signal<%s > r_%sRdData_%s%s_1x%s;\n",
										elemType.c_str(),
										intGbl.m_gblName.c_str(), field.m_name.c_str(), pDupStr, intGbl.m_dimenDecl.c_str());

									GenRamIndexLoops(m_gblReg1x, "", intGbl);
									m_gblReg1x.Append("\tr_%sRdData%s%s_1x%s = c_%sRdData%s%s%s;\n",
										intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, intGbl.m_dimenIndex.c_str(),
										intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, intGbl.m_dimenIndex.c_str());
								}
							} else if (bFirst1xReadIntf) {

								m_gblRegDecl.Append("\tsc_signal<%s > r_%sRdData%s%s_1x%s;\n",
									elemType.c_str(),
									intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, intGbl.m_dimenDecl.c_str());

								GenRamIndexLoops(m_gblReg1x, "", intGbl);

								if (m_bBlockRamDoReg && field.m_ramType == eBlockRam)
									m_gblReg1x.Append("\tr_%sRdData%s%s_1x%s = c_%sRdData%s%s_2x%s.read();\n",
										intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, intGbl.m_dimenIndex.c_str(),
										intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, intGbl.m_dimenIndex.c_str());
								else
									m_gblReg1x.Append("\tr_%sRdData%s%s_1x%s = r_%sRdData%s%s%s.read();\n",
										intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, intGbl.m_dimenIndex.c_str(),
										intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, intGbl.m_dimenIndex.c_str());

							} else if (bSecond1xReadIntf) {

								m_gblRegDecl.Append("\tsc_signal<%s > r_%sRdData%s%s_2x%s;\n",
									elemType.c_str(),
									intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, intGbl.m_dimenDecl.c_str());

								if (m_bBlockRamDoReg && field.m_ramType == eBlockRam) {

									GenRamIndexLoops(m_gblReg2x, "", intGbl);
									m_gblReg2x.Append("\tr_%sRdData%s%s_2x%s = c_%sRdData%s%s_2x%s.read();\n",
										intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, intGbl.m_dimenIndex.c_str(),
										intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, intGbl.m_dimenIndex.c_str());

								} else {
									GenRamIndexLoops(m_gblPostInstr2x, "", intGbl);
									m_gblPostInstr2x.Append("\t%s c_%sRdData%s%s_2x%s = r_%sRdData%s%s%s.read();\n",
										elemType.c_str(),
										intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, intGbl.m_dimenIndex.c_str(),
										intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, intGbl.m_dimenIndex.c_str());

									GenRamIndexLoops(m_gblReg2x, "", intGbl);
									m_gblReg2x.Append("\tr_%sRdData%s%s_2x%s = c_%sRdData%s%s_2x%s;\n",
										intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, intGbl.m_dimenIndex.c_str(),
										intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, intGbl.m_dimenIndex.c_str());
								}
							}
						}
					}
				}
			}
		}

		m_gblReg1x.NewLine();
		m_gblReg2x.NewLine();
	}

	for (size_t ramIdx = 0; ramIdx < mod.m_intGblList.size(); ramIdx += 1) {
		CRam &intGbl = *mod.m_intGblList[ramIdx];

		for (size_t ramPortIdx = 0; ramPortIdx < intGbl.m_gblPortList.size(); ramPortIdx += 1) {
			CRamPort gblPort = intGbl.m_gblPortList[ramPortIdx];

			// Input address registers
			if (!gblPort.m_bSrcRead && !gblPort.m_bMifRead/* || gblPort.m_bSecond1xReadIntf*/) continue;

			bool bRamCont = false;
			CHtCode *pRamCode;
			if (gblPort.m_bIntIntf)
				pRamCode = (mod.m_clkRate == eClk1x && !gblPort.m_bSecond1xReadIntf) ? &m_gblPostInstr1x : &m_gblPostInstr2x;
			else if ((gblPort.m_bOne1xReadIntf || gblPort.m_bFirst1xReadIntf) && gblPort.m_bSecond1xReadIntf) {
				pRamCode = &m_gblPreCont;
				bRamCont = true;
			} else if (gblPort.m_bOne1xReadIntf || gblPort.m_bFirst1xReadIntf)
				pRamCode = &m_gblPostInstr1x;
			else
				pRamCode = &m_gblPostInstr2x;

			char rdDataStr[128];
	
			if (gblPort.m_bFirst1xReadIntf || gblPort.m_bSecond1xReadIntf || gblPort.m_bOne1xReadIntf || gblPort.m_bOne2xReadIntf) {

				if (gblPort.m_bIntIntf) {
					string ramType;
					if (gblPort.m_pRam->m_bGlobal)
						ramType = gblPort.m_pRam->m_type;
					else
						ramType = "C" + gblPort.m_pRam->m_gblName.Uc() + "RdData";

					if (gblPort.m_bMifRead) {
						sprintf(rdDataStr, "c_%sMifRdData",
							gblPort.m_pRam->m_gblName.c_str());
						pRamCode->Append("\tC%sRdData %s%s;\n",
							gblPort.m_pRam->m_gblName.Uc().c_str(),
							rdDataStr, intGbl.m_dimenDecl.c_str());
					} else {
						int rdStg = mod.m_tsStg + intGbl.m_rdStg.AsInt() - 1;

						sprintf(rdDataStr, "c_t%d_%sRdData", rdStg-1, gblPort.m_pRam->m_gblName.c_str());
						pRamCode->Append("\t%s %s%s;\n",
							ramType.c_str(),
							rdDataStr, intGbl.m_dimenDecl.c_str());
					}
				} else {
					sprintf(rdDataStr, "c_%sTo%s_%sRdData",
						mod.m_modName.Lc().c_str(), gblPort.m_instName.Uc().c_str(), gblPort.m_pRam->m_gblName.c_str());
					pRamCode->Append("\tC%sTo%s_%sRdData %s%s;\n",
						mod.m_modName.Uc().c_str(), gblPort.m_pModInfo->m_modName.Uc().c_str(), gblPort.m_pRam->m_gblName.c_str(),
						rdDataStr, intGbl.m_dimenDecl.c_str());
				}

				for (size_t fldIdx = 0; fldIdx < intGbl.m_allFieldList.size(); fldIdx += 1) {
					CField &field = intGbl.m_allFieldList[fldIdx];

					string fieldName = field.m_name.size() > 0 ? (string("_") + field.m_name) : "";
					string dotFieldName = field.m_name.size() > 0 ? (string(".m_") + field.m_name) : "";

					vector<int> dimenList;
					string		dimenDecl;
					string		fullIndex;
					string		varIndex;
					string		fldIndex;

					GenDimenInfo(intGbl, field, dimenList, dimenDecl, fullIndex, &varIndex, &fldIndex);

					for (size_t rdIdx = 0; rdIdx < field.m_readerList.size(); rdIdx += 1) {
						CRamPort *pInstRam = field.m_readerList[rdIdx].m_pRamPort;

						if (pInstRam == 0 || pInstRam->m_instName.Lc() != gblPort.m_instName.Lc()) continue;

						if (!(gblPort.m_bMifRead && pInstRam->m_bMifRead || gblPort.m_bSrcRead && pInstRam->m_bSrcRead)) continue;

						char dupCh[3] = { '_', (char)('a' + (rdIdx/2)), '\0' };
						const char *pDupStr = field.m_readerList.size() <= 2 ? "" : dupCh;

						bool bOne1xReadIntf = field.m_readerList[rdIdx].m_bOne1xReadIntf;
						bool bOne2xReadIntf = (rdIdx & 1) == 0 && rdIdx+1 < field.m_readerList.size() && field.m_readerList[rdIdx].m_pRamPort == field.m_readerList[rdIdx+1].m_pRamPort;
						bool bFirst1xReadIntf = field.m_readerList[rdIdx].m_bFirst1xReadIntf;
						bool bSecond1xReadIntf = (rdIdx & 1) == 1 && field.m_readerList[rdIdx].m_pRamPort != 0 &&
							field.m_readerList[rdIdx-1].m_pRamPort != field.m_readerList[rdIdx].m_pRamPort;

						bool bDoReg = m_bBlockRamDoReg && field.m_ramType == eBlockRam;
						char doRegCh = bDoReg ? 'c' : 'r';

						if (bOne1xReadIntf) {

							if (bRamCont) {
								GenRamIndexLoops(m_gblSenCont, dimenList);
								m_gblSenCont.Append("\t\tsensitive << %c_%sRdData%s%s_1x%s;\n",
									doRegCh, intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, fullIndex.c_str());

								GenRamIndexLoops(*pRamCode, dimenList);
								pRamCode->Append("\t%s%s%s%s = %c_%sRdData%s%s_1x%s.read();\n",
									rdDataStr, varIndex.c_str(), dotFieldName.c_str(), fldIndex.c_str(),
									doRegCh, intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, fullIndex.c_str());
							} else if (bDoReg) {
								GenRamIndexLoops(*pRamCode, dimenList);
								pRamCode->Append("\t%s%s%s%s = c_%sRdData%s%s_2x%s.read();\n",
									rdDataStr, varIndex.c_str(), dotFieldName.c_str(), fldIndex.c_str(),
									intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, fullIndex.c_str());
							} else {
								GenRamIndexLoops(*pRamCode, dimenList);
								pRamCode->Append("\t%s%s%s%s = c_%sRdData%s%s%s;\n",
									rdDataStr, varIndex.c_str(), dotFieldName.c_str(), fldIndex.c_str(),
									intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, fullIndex.c_str());
							}

						} else if (bOne2xReadIntf) {

							GenRamIndexLoops(*pRamCode, dimenList);
							pRamCode->Append("\t%s%s%s%s = c_%sRdData%s%s%s;\n",
								rdDataStr, varIndex.c_str(), dotFieldName.c_str(), fldIndex.c_str(),
								intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, fullIndex.c_str());

						} else if (bFirst1xReadIntf) {

							if (bRamCont) {
								GenRamIndexLoops(m_gblSenCont, dimenList);
								m_gblSenCont.Append("\t\tsensitive << r_%sRdData%s%s_1x%s;\n",
									intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, fullIndex.c_str());

								GenRamIndexLoops(*pRamCode, dimenList);
								pRamCode->Append("\t%s%s%s%s = r_%sRdData%s%s_1x%s.read();\n",
									rdDataStr, varIndex.c_str(), dotFieldName.c_str(), fldIndex.c_str(),
									intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, fullIndex.c_str());
							} else {
								if (bDoReg) {
									GenRamIndexLoops(*pRamCode, dimenList);
									pRamCode->Append("\t%s%s%s%s = c_%sRdData%s%s_2x%s.read();\n",
										rdDataStr, varIndex.c_str(), dotFieldName.c_str(), fldIndex.c_str(),
										intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, fullIndex.c_str());
								} else {
									GenRamIndexLoops(*pRamCode, dimenList);
									pRamCode->Append("\t%s%s%s%s = r_%sRdData%s%s%s.read();\n",
										rdDataStr, varIndex.c_str(), dotFieldName.c_str(), fldIndex.c_str(),
										intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, fullIndex.c_str());
								}
							}
						} else if (bSecond1xReadIntf) {

							if (bRamCont) {

								GenRamIndexLoops(m_gblSenCont, dimenList);
								m_gblSenCont.Append("\t\tsensitive << r_%sRdData%s%s_2x%s;\n",
									intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, fullIndex.c_str());

								GenRamIndexLoops(*pRamCode, dimenList);
								pRamCode->Append("\t%s%s%s%s = r_%sRdData%s%s_2x%s.read();\n",
									rdDataStr, varIndex.c_str(), dotFieldName.c_str(), fldIndex.c_str(),
									intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, fullIndex.c_str());
							} else
								if (bDoReg) {
									GenRamIndexLoops(*pRamCode, dimenList);
									pRamCode->Append("\t%s%s%s%s = c_%sRdData%s%s_2x%s.read();\n",
										rdDataStr, varIndex.c_str(), dotFieldName.c_str(), fldIndex.c_str(),
										intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, fullIndex.c_str());
								} else {
									GenRamIndexLoops(*pRamCode, dimenList);
									pRamCode->Append("\t%s%s%s%s = r_%sRdData%s%s%s.read();\n",
										rdDataStr, varIndex.c_str(), dotFieldName.c_str(), fldIndex.c_str(),
										intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, fullIndex.c_str());
								}
						}
					}
				}
				pRamCode->NewLine();
			}
		}
	}

	// write statements
	for (size_t ramIdx = 0; ramIdx < mod.m_intGblList.size(); ramIdx += 1) {
		CRam &intGbl = *mod.m_intGblList[ramIdx];

		bool bHasAddr = intGbl.m_addr0W.AsInt() + intGbl.m_addr1W.AsInt() + intGbl.m_addr2W.AsInt() > 0;

		string addrW;
		if (intGbl.m_addr0W.size() > 0) {
			addrW = intGbl.m_addr0W.AsStr();
			if (intGbl.m_addr1W.size() > 0)
				addrW += "+";
		}
		addrW += intGbl.m_addr1W.AsStr();
		if (intGbl.m_addr2W.size() > 0)
			addrW += " + " + intGbl.m_addr2W.AsStr();

		//m_gblRegDecl.Append("\t// Internal rams field registers\n");
		for (size_t fldIdx = 0; fldIdx < intGbl.m_allFieldList.size(); fldIdx += 1) {
			CField &field = intGbl.m_allFieldList[fldIdx];

			vector<int> fullDimenList;
			string		fullDecl;
			string		fullIndex;
			string		varIndex;
			string		fldIndex;

			GenDimenInfo(intGbl, field, fullDimenList, fullDecl, fullIndex, &varIndex, &fldIndex);

			if (field.m_bTwo1xWriteIntf) {

				string elemType = FindFieldType(field);

				if (bHasAddr)
					m_gblRegDecl.Append("\tsc_uint<%s> r_%sWrAddr_%s%s;\n", 
						addrW.c_str(),
						intGbl.m_gblName.c_str(), field.m_name.c_str(), intGbl.m_dimenDecl.c_str());

				m_gblRegDecl.Append("\t%s r_%sWrData_%s%s;\n", 
					elemType.c_str(),
					intGbl.m_gblName.c_str(), field.m_name.c_str(), fullDecl.c_str());
				m_gblRegDecl.Append("\tbool r_%sWrEn_%s%s;\n", 
					intGbl.m_gblName.c_str(), field.m_name.c_str(), fullDecl.c_str());

				if (bHasAddr) {
					GenRamIndexLoops(m_gblReg2x, "", intGbl);
					m_gblReg2x.Append("\tr_%sWrAddr_%s%s = c_%sWrAddr_%s%s;\n",
						intGbl.m_gblName.c_str(), field.m_name.c_str(), intGbl.m_dimenIndex.c_str(),
						intGbl.m_gblName.c_str(), field.m_name.c_str(), intGbl.m_dimenIndex.c_str());
				}

				GenRamIndexLoops(m_gblReg2x, fullDimenList);
				m_gblReg2x.Append("\tr_%sWrEn_%s%s = c_%sWrEn_%s%s;\n",
					intGbl.m_gblName.c_str(), field.m_name.c_str(), fullIndex.c_str(),
					intGbl.m_gblName.c_str(), field.m_name.c_str(), fullIndex.c_str());

				GenRamIndexLoops(m_gblReg2x, fullDimenList);
				m_gblReg2x.Append("\tr_%sWrData_%s%s = c_%sWrData_%s%s;\n",
					intGbl.m_gblName.c_str(), field.m_name.c_str(), fullIndex.c_str(),
					intGbl.m_gblName.c_str(), field.m_name.c_str(), fullIndex.c_str());
			}
		}
		m_gblRegDecl.NewLine();
		m_gblReg2x.NewLine();

		for (size_t ramPortIdx = 0; ramPortIdx < intGbl.m_gblPortList.size(); ramPortIdx += 1) {
			CRamPort gblPort = intGbl.m_gblPortList[ramPortIdx];

			string addrW;
			if (gblPort.m_pRam->m_addr0W.size() > 0) {
				addrW = gblPort.m_pRam->m_addr0W.AsStr();
				if (gblPort.m_pRam->m_addr1W.size() > 0)
					addrW += "+";
			}
			addrW += gblPort.m_pRam->m_addr1W.AsStr();
			if (gblPort.m_pRam->m_addr2W.size() > 0)
				addrW += " + " + gblPort.m_pRam->m_addr2W.AsStr();

			if (!gblPort.m_bIntIntf && (gblPort.m_bSrcWrite || gblPort.m_bMifWrite) && gblPort.m_bSecond1xWriteIntf) {

				if (bHasAddr)
					m_gblRegDecl.Append("\tsc_signal<sc_uint<%s> > r_%sTo%s_%sWrAddr%s;\n",
						addrW.c_str(),
						gblPort.m_instName.Lc().c_str(), mod.m_modName.Uc().c_str(), gblPort.m_pRam->m_gblName.c_str(), intGbl.m_dimenDecl.c_str());

				m_gblRegDecl.Append("\tsc_signal<C%sTo%s_%sWrEn> r_%sTo%s_%sWrEn%s;\n",
					gblPort.m_pModInfo->m_modName.Uc().c_str(), mod.m_modName.Uc().c_str(), gblPort.m_pRam->m_gblName.c_str(),
					gblPort.m_instName.Lc().c_str(), mod.m_modName.Uc().c_str(), gblPort.m_pRam->m_gblName.c_str(), intGbl.m_dimenDecl.c_str());

				m_gblRegDecl.Append("\tsc_signal<C%sTo%s_%sWrData> r_%sTo%s_%sWrData%s;\n",
					gblPort.m_pModInfo->m_modName.Uc().c_str(), mod.m_modName.Uc().c_str(), gblPort.m_pRam->m_gblName.c_str(),
					gblPort.m_instName.Lc().c_str(), mod.m_modName.Uc().c_str(), gblPort.m_pRam->m_gblName.c_str(), intGbl.m_dimenDecl.c_str());

				if (bHasAddr) {
					GenRamIndexLoops(m_gblReg1x, "", intGbl);
					m_gblReg1x.Append("\tr_%sTo%s_%sWrAddr%s = i_%sTo%s_%sWrAddr%s;\n",
						gblPort.m_instName.Lc().c_str(), mod.m_modName.Uc().c_str(), gblPort.m_pRam->m_gblName.c_str(), intGbl.m_dimenIndex.c_str(),
						gblPort.m_instName.Lc().c_str(), mod.m_modName.Uc().c_str(), gblPort.m_pRam->m_gblName.c_str(), intGbl.m_dimenIndex.c_str());
				}

				GenRamIndexLoops(m_gblReg1x, "", intGbl);
				m_gblReg1x.Append("\tr_%sTo%s_%sWrEn%s = i_%sTo%s_%sWrEn%s;\n",
					gblPort.m_instName.Lc().c_str(), mod.m_modName.Uc().c_str(), gblPort.m_pRam->m_gblName.c_str(), intGbl.m_dimenIndex.c_str(),
					gblPort.m_instName.Lc().c_str(), mod.m_modName.Uc().c_str(), gblPort.m_pRam->m_gblName.c_str(), intGbl.m_dimenIndex.c_str());

				GenRamIndexLoops(m_gblReg1x, "", intGbl);
				m_gblReg1x.Append("\tr_%sTo%s_%sWrData%s = i_%sTo%s_%sWrData%s;\n",
					gblPort.m_instName.Lc().c_str(), mod.m_modName.Uc().c_str(), gblPort.m_pRam->m_gblName.c_str(), intGbl.m_dimenIndex.c_str(),
					gblPort.m_instName.Lc().c_str(), mod.m_modName.Uc().c_str(), gblPort.m_pRam->m_gblName.c_str(), intGbl.m_dimenIndex.c_str());

			}
		}

		m_gblReg1x.NewLine();
		m_gblReg2x.NewLine();
	}

	for (size_t ramIdx = 0; ramIdx < mod.m_intGblList.size(); ramIdx += 1) {
		CRam &intGbl = *mod.m_intGblList[ramIdx];

		bool bHasAddr = intGbl.m_addr0W.AsInt() + intGbl.m_addr1W.AsInt() + intGbl.m_addr2W.AsInt() > 0;

		string addrW;
		if (intGbl.m_addr0W.size() > 0) {
			addrW = intGbl.m_addr0W.AsStr();
			if (intGbl.m_addr1W.size() > 0)
				addrW += "+";
		}
		addrW += intGbl.m_addr1W.AsStr();
		if (intGbl.m_addr2W.size() > 0)
			addrW += " + " + intGbl.m_addr2W.AsStr();
			
		for (size_t fldIdx = 0; fldIdx < intGbl.m_allFieldList.size(); fldIdx += 1) {
			CField &field = intGbl.m_allFieldList[fldIdx];

			for (size_t wrIdx = 0; wrIdx < field.m_writerList.size(); wrIdx += 2) {

				if (wrIdx+1 < field.m_writerList.size() && field.m_writerList[wrIdx+1].m_pRamPort != 0
					&& field.m_writerList[wrIdx].m_pRamPort != field.m_writerList[wrIdx+1].m_pRamPort) {

					vector<int> fullDimenList;
					string		fullDecl;
					string		fullIndex;
					string		varIndex;
					string		fldIndex;

					GenDimenInfo(intGbl, field, fullDimenList, fullDecl, fullIndex, &varIndex, &fldIndex);

					char phase0WrAddrStr[128];
					if (field.m_writerList[wrIdx].m_pRamPort->m_bIntIntf) {
						if (field.m_writerList[wrIdx].m_pRamPort->m_bMifWrite)
						    sprintf(phase0WrAddrStr, "r_m2");
					    else {
							int wrStg = mod.m_tsStg + intGbl.m_wrStg.AsInt();
                            sprintf(phase0WrAddrStr, "r_t%d", wrStg);
						}
                    } else
						sprintf(phase0WrAddrStr, "i_%sTo%s",
							field.m_writerList[wrIdx].m_pRamPort->m_instName.Lc().c_str(), mod.m_modName.Uc().c_str());

					char phase1WrAddrStr[128];
					if (field.m_writerList[wrIdx+1].m_pRamPort->m_bIntIntf)
						sprintf(phase1WrAddrStr, "r_m2");

					else
						sprintf(phase1WrAddrStr, "r_%sTo%s",
							field.m_writerList[wrIdx+1].m_pRamPort->m_instName.Lc().c_str(), mod.m_modName.Uc().c_str());

					string fieldType = FindFieldType(field);

					mod.m_phaseCnt += 1;
					if (phaseStr[0] < 'Z')
						phaseStr[0] += 1;
					else {
						phaseStr[0] = 'A';
						if (phaseStr[1] == '\0')
							phaseStr[1] = 'A';
						else
							phaseStr[1] += 1;
					}

					m_gblRegDecl.Append("\tbool r_phase%s;\n", phaseStr);
					m_gblRegDecl.Append("\tsc_signal<bool> c_phaseReset%s;\n", phaseStr);
					m_gblRegDecl.Append("\tbool r_phaseReset%s;\n", phaseStr);
					m_gblPostReg1x.Append("\tc_phaseReset%s = r_phaseReset%s;\n", phaseStr, phaseStr);
					m_gblReg2x.Append("\tht_attrib(equivalent_register_removal, r_phase%s, \"no\");\n", phaseStr);
					m_gblReg2x.Append("\tr_phase%s = c_phaseReset%s.read() || !r_phase%s;\n", phaseStr, phaseStr, phaseStr);

					if (bHasAddr) {
						m_gblPostInstr2x.Append("\tsc_uint<%s> c_%sWrAddr_%s%s;\n",
							addrW.c_str(),
							intGbl.m_gblName.c_str(), field.m_name.c_str(), intGbl.m_dimenDecl.c_str());

						GenRamIndexLoops(m_gblPostInstr2x, "", intGbl);
						m_gblPostInstr2x.Append("\tc_%sWrAddr_%s%s = r_phase%s ? %s_%sWrAddr%s.read() : %s_%sWrAddr%s.read();\n",
							intGbl.m_gblName.c_str(), field.m_name.c_str(), intGbl.m_dimenIndex.c_str(),
							phaseStr,
							phase1WrAddrStr, intGbl.m_gblName.c_str(), intGbl.m_dimenIndex.c_str(),
							phase0WrAddrStr, intGbl.m_gblName.c_str(), intGbl.m_dimenIndex.c_str());
					}

					m_gblPostInstr2x.Append("\tbool c_%sWrEn_%s%s;\n",
						intGbl.m_gblName.c_str(), field.m_name.c_str(), fullDecl.c_str());

					GenRamIndexLoops(m_gblPostInstr2x, fullDimenList);
					m_gblPostInstr2x.Append("\tc_%sWrEn_%s%s = r_phase%s ? %s_%sWrEn%s.read().m_%s%s : %s_%sWrEn%s.read().m_%s%s;\n",
						intGbl.m_gblName.c_str(), field.m_name.c_str(), fullIndex.c_str(),
						phaseStr,
						phase1WrAddrStr, intGbl.m_gblName.c_str(), varIndex.c_str(), field.m_name.c_str(), fldIndex.c_str(),
						phase0WrAddrStr, intGbl.m_gblName.c_str(), varIndex.c_str(), field.m_name.c_str(), fldIndex.c_str());

					m_gblPostInstr2x.Append("\t%s c_%sWrData_%s%s;\n", 
						fieldType.c_str(),
						intGbl.m_gblName.c_str(), field.m_name.c_str(), fullDecl.c_str());

					GenRamIndexLoops(m_gblPostInstr2x, fullDimenList);
					m_gblPostInstr2x.Append("\tc_%sWrData_%s%s = r_phase%s ? %s_%sWrData%s.read().m_%s%s : %s_%sWrData%s.read().m_%s%s;\n",
						intGbl.m_gblName.c_str(), field.m_name.c_str(), fullIndex.c_str(),
						phaseStr,
						phase1WrAddrStr, intGbl.m_gblName.c_str(), varIndex.c_str(), field.m_name.c_str(), fldIndex.c_str(),
						phase0WrAddrStr, intGbl.m_gblName.c_str(), varIndex.c_str(), field.m_name.c_str(), fldIndex.c_str());

					m_gblPostInstr2x.NewLine();
				}
			}
		}
	}

	for (size_t ramIdx = 0; ramIdx < mod.m_intGblList.size(); ramIdx += 1) {
		CRam &intGbl = *mod.m_intGblList[ramIdx];

		bool bHasAddr = intGbl.m_addr0W.AsInt() + intGbl.m_addr1W.AsInt() + intGbl.m_addr2W.AsInt() > 0;

		int wrStg = mod.m_tsStg + intGbl.m_wrStg.AsInt() - 1;

		char replStr[16] = { '\0' };
		if (intGbl.m_bReplAccess && mod.m_modInstList.size() > 1)
			sprintf(replStr, " >> %d", FindLg2(mod.m_modInstList.size()-1, false));

		for (size_t ramPortIdx = 0; ramPortIdx < intGbl.m_gblPortList.size(); ramPortIdx += 1) {
			CRamPort gblPort = intGbl.m_gblPortList[ramPortIdx];

			for (size_t fldIdx = 0; fldIdx < intGbl.m_allFieldList.size(); fldIdx += 1) {
				CField &field = intGbl.m_allFieldList[fldIdx];

				string fieldName = field.m_name.size() > 0 ? (string("_") + field.m_name) : "";
				string dotFieldName = field.m_name.size() > 0 ? (string(".m_") + field.m_name) : "";

				vector<int> dimenList;
				string		dimenDecl;
				string		fullIndex;
				string		varIndex;
				string		fldIndex;

				GenDimenInfo(intGbl, field, dimenList, dimenDecl, fullIndex, &varIndex, &fldIndex);

				for (size_t wrIdx = 0; wrIdx < field.m_writerList.size(); wrIdx += 1) {
					CRamPort *pInstRam = field.m_writerList[wrIdx].m_pRamPort;

					if (pInstRam->m_instName.Lc() != gblPort.m_instName.Lc()) continue;

					if (!(gblPort.m_bMifWrite && pInstRam->m_bMifWrite || gblPort.m_bSrcWrite && pInstRam->m_bSrcWrite)) continue;

					bool bOne1xWriteIntf = (wrIdx & 1) == 0 && (wrIdx+1 == field.m_writerList.size() || field.m_writerList[wrIdx+1].m_pRamPort == 0);
					bool bOne2xWriteIntf = (wrIdx & 1) == 0 && wrIdx+1 < field.m_writerList.size() && field.m_writerList[wrIdx].m_pRamPort == field.m_writerList[wrIdx+1].m_pRamPort;
					bool bFirst1xWriteIntf = (wrIdx & 1) == 0 && wrIdx+1 < field.m_writerList.size() && field.m_writerList[wrIdx+1].m_pRamPort != 0 &&
						field.m_writerList[wrIdx].m_pRamPort != field.m_writerList[wrIdx+1].m_pRamPort;

					char wrAddrStr[128];
					if (field.m_writerList[wrIdx].m_pRamPort->m_bIntIntf) {
						if (field.m_writerList[wrIdx].m_pRamPort->m_bMifRead || field.m_writerList[wrIdx].m_pRamPort->m_bMifWrite)
						    sprintf(wrAddrStr, "r_m2");
					    else 
                            sprintf(wrAddrStr, "r_t%d", wrStg+1);
                    } else
						sprintf(wrAddrStr, "i_%sTo%s",
							gblPort.m_instName.Lc().c_str(), mod.m_modName.Uc().c_str());

					bool bSignal = !gblPort.m_bIntIntf /*&& !gblPort.m_bMifRead && !gblPort.m_bMifWrite*/ || gblPort.m_bSecond1xWriteIntf || gblPort.m_bFirst1xWriteIntf;

					for (size_t rdIdx = 0; rdIdx < field.m_readerList.size(); rdIdx += 2) {

						char dupCh[3] = { '_', (char)('a' + (rdIdx/2)), '\0' };
						const char *pDupStr = field.m_readerList.size() <= 2 ? "" : dupCh;

						string elemType = FindFieldType(field);

						if (bOne1xWriteIntf) {

							if (bHasAddr) {
								string tabs = GenRamIndexLoops(m_gblPostInstr1x, dimenList, true);
								m_gblPostInstr1x.Append("\tm_%s%s%s%s.write_addr(%s_%sWrAddr%s%s%s);\n",
									intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, fullIndex.c_str(),
									wrAddrStr, gblPort.m_pRam->m_gblName.c_str(), varIndex.c_str(), bSignal ? ".read()" : "", replStr);

								m_gblPostInstr1x.Append("%s\tif (%s_%sWrEn%s%s%s%s)\n", tabs.c_str(),
									wrAddrStr, gblPort.m_pRam->m_gblName.c_str(), varIndex.c_str(), bSignal ? ".read()" : "", dotFieldName.c_str(), fldIndex.c_str());

								m_gblPostInstr1x.Append("%s\t\tm_%s%s%s%s.write_mem( %s_%sWrData%s%s%s%s );\n", tabs.c_str(),
									intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, fullIndex.c_str(),
									wrAddrStr, gblPort.m_pRam->m_gblName.c_str(), varIndex.c_str(), bSignal ? ".read()" : "", dotFieldName.c_str(), fldIndex.c_str());

								if (dimenList.size() > 0)
									m_gblPostInstr1x.Append("%s}\n", tabs.c_str());

							} else {
								if (dimenList.size() > 0)
									m_gblPostInstr1x.Append("\t%s c_%s%s%s%s;\n",
										elemType.c_str(),
										intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, dimenDecl.c_str());

								string tabs = GenRamIndexLoops(m_gblPostInstr1x, dimenList, true);

								if (dimenList.size() == 0)
									m_gblPostInstr1x.Append("\t%s c_%s%s%s%s%s = r_%s%s%s%s%s;\n",
										elemType.c_str(),
										intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, varIndex.c_str(), fldIndex.c_str(),
										intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, varIndex.c_str(), fldIndex.c_str());
								else
									m_gblPostInstr1x.Append("\tc_%s%s%s%s%s = r_%s%s%s%s%s;\n",
										intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, varIndex.c_str(), fldIndex.c_str(),
										intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, varIndex.c_str(), fldIndex.c_str());

								m_gblPostInstr1x.Append("%s\tif (%s_%sWrEn%s%s%s%s)\n", tabs.c_str(),
									wrAddrStr, gblPort.m_pRam->m_gblName.c_str(), varIndex.c_str(), bSignal ? ".read()" : "", dotFieldName.c_str(), fldIndex.c_str());

								m_gblPostInstr1x.Append("%s\t\tc_%s%s%s%s%s = %s_%sWrData%s%s%s%s;\n", tabs.c_str(),
									intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, varIndex.c_str(), fldIndex.c_str(),
									wrAddrStr, gblPort.m_pRam->m_gblName.c_str(), varIndex.c_str(), bSignal ? ".read()" : "", dotFieldName.c_str(), fldIndex.c_str());

								m_gblPostInstr1x.Append("%s\tif (c_reset1x)\n", tabs.c_str());

								m_gblPostInstr1x.Append("%s\t\tc_%s%s%s%s%s = 0;\n", tabs.c_str(),
									intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, varIndex.c_str(), fldIndex.c_str());

								if (dimenList.size() > 0)
									m_gblPostInstr1x.Append("%s}\n", tabs.c_str());
							}

						} else if (bOne2xWriteIntf) {

							if (bHasAddr) {
								string tabs = GenRamIndexLoops(m_gblPostInstr2x, dimenList, true);
								m_gblPostInstr2x.Append("\tm_%s%s%s%s.write_addr(%s_%sWrAddr%s%s%s);\n",
									intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, fullIndex.c_str(),
									wrAddrStr, gblPort.m_pRam->m_gblName.c_str(), varIndex.c_str(), bSignal ? ".read()" : "", replStr);

								m_gblPostInstr2x.Append("%s\tif (%s_%sWrEn%s%s%s%s)\n", tabs.c_str(),
									wrAddrStr, gblPort.m_pRam->m_gblName.c_str(), varIndex.c_str(), bSignal ? ".read()" : "", dotFieldName.c_str(), fldIndex.c_str());

								m_gblPostInstr2x.Append("%s\t\tm_%s%s%s%s.write_mem( %s_%sWrData%s%s%s%s );\n", tabs.c_str(),
									intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, fullIndex.c_str(),
									wrAddrStr, gblPort.m_pRam->m_gblName.c_str(), varIndex.c_str(), bSignal ? ".read()" : "", dotFieldName.c_str(), fldIndex.c_str());

								if (dimenList.size() > 0)
									m_gblPostInstr2x.Append("%s}\n", tabs.c_str());
							} else {
								if (dimenList.size() > 0)
									m_gblPostInstr2x.Append("\t%s c_%s%s%s%s;\n",
										elemType.c_str(),
										intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, dimenDecl.c_str());

								string tabs = GenRamIndexLoops(m_gblPostInstr2x, dimenList, true);

								if (dimenList.size() == 0)
									m_gblPostInstr2x.Append("\t%s c_%s%s%s%s%s = r_%s%s%s%s%s;\n",
										elemType.c_str(),
										intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, varIndex.c_str(), fldIndex.c_str(),
										intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, varIndex.c_str(), fldIndex.c_str());
								else
									m_gblPostInstr2x.Append("\tc_%s%s%s%s%s = r_%s%s%s%s%s;\n",
										intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, varIndex.c_str(), fldIndex.c_str(),
										intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, varIndex.c_str(), fldIndex.c_str());

								m_gblPostInstr2x.Append("%s\tif (%s_%sWrEn%s%s%s%s)\n", tabs.c_str(),
									wrAddrStr, gblPort.m_pRam->m_gblName.c_str(), varIndex.c_str(), bSignal ? ".read()" : "", dotFieldName.c_str(), fldIndex.c_str());

								m_gblPostInstr2x.Append("%s\t\tc_%s%s%s%s%s = %s_%sWrData%s%s%s%s;\n", tabs.c_str(),
									intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, varIndex.c_str(), fldIndex.c_str(),
									wrAddrStr, gblPort.m_pRam->m_gblName.c_str(), varIndex.c_str(), bSignal ? ".read()" : "", dotFieldName.c_str(), fldIndex.c_str());

								if (dimenList.size() > 0)
									m_gblPostInstr2x.Append("%s}\n", tabs.c_str());
							}

						} else if (bFirst1xWriteIntf) {

							if (bHasAddr) {
								string tabs = GenRamIndexLoops(m_gblPostInstr2x, dimenList, true);

								m_gblPostInstr2x.Append("\tm_%s%s%s%s.write_addr(r_%sWrAddr%s%s%s);\n",
									intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, fullIndex.c_str(),
									intGbl.m_gblName.c_str(), fieldName.c_str(), replStr, varIndex.c_str());

								m_gblPostInstr2x.Append("%s\tif (r_%sWrEn%s%s)\n", tabs.c_str(),
									intGbl.m_gblName.c_str(), fieldName.c_str(), fullIndex.c_str());

								m_gblPostInstr2x.Append("%s\t\tm_%s%s%s%s.write_mem( r_%sWrData%s%s );\n", tabs.c_str(),
									intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, fullIndex.c_str(),
									intGbl.m_gblName.c_str(), fieldName.c_str(), fullIndex.c_str());

								if (dimenList.size() > 0)
									m_gblPostInstr2x.Append("%s}\n", tabs.c_str());
							} else {
								if (dimenList.size() > 0)
									m_gblPostInstr2x.Append("\t%s c_%s%s%s%s;\n",
										elemType.c_str(),
										intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, dimenDecl.c_str());

								string tabs = GenRamIndexLoops(m_gblPostInstr2x, dimenList, true);

								if (dimenList.size() == 0)
									m_gblPostInstr2x.Append("\t%s c_%s%s%s%s%s = r_%s%s%s%s%s;\n",
										elemType.c_str(),
										intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, varIndex.c_str(), fldIndex.c_str(),
										intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, varIndex.c_str(), fldIndex.c_str());
								else
									m_gblPostInstr2x.Append("\tc_%s%s%s%s%s = r_%s%s%s%s%s;\n",
										intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, varIndex.c_str(), fldIndex.c_str(),
										intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, varIndex.c_str(), fldIndex.c_str());

								m_gblPostInstr2x.Append("%s\tif (r_%sWrEn%s%s)\n", tabs.c_str(),
									intGbl.m_gblName.c_str(), fieldName.c_str(), fullIndex.c_str());

								m_gblPostInstr2x.Append("%s\t\tc_%s%s%s%s%s = r_%sWrData%s%s;\n", tabs.c_str(),
									intGbl.m_gblName.c_str(), fieldName.c_str(), pDupStr, varIndex.c_str(), fldIndex.c_str(),
									intGbl.m_gblName.c_str(), fieldName.c_str(), fullIndex.c_str());

								if (dimenList.size() > 0)
									m_gblPostInstr2x.Append("%s}\n", tabs.c_str());
							}
						}
					}
				}
			}
		}
	}

	////////////////////////////////////////////
	// Ram outputs

	bComment1x = true;
	bComment2x = true;
	bool bCommentCont = true;
	for (size_t ramIdx = 0; ramIdx < mod.m_intGblList.size(); ramIdx += 1) {
		CRam &intGbl = *mod.m_intGblList[ramIdx];

		for (size_t ramPortIdx = 0; ramPortIdx < intGbl.m_gblPortList.size(); ramPortIdx += 1) {
			CRamPort gblPort = intGbl.m_gblPortList[ramPortIdx];

			// Input address registers
			if (gblPort.m_bIntIntf || gblPort.m_bMifRead || gblPort.m_bMifWrite || !gblPort.m_pRam->m_bSrcRead) continue;

			if ((gblPort.m_bOne1xReadIntf || gblPort.m_bFirst1xReadIntf) && gblPort.m_bSecond1xReadIntf) {
				if (bCommentCont) {
					m_gblOutCont.Append("\t// External ram read data output\n");
					bCommentCont = false;
				}

				GenRamIndexLoops(m_gblOutCont, "", intGbl);
				m_gblOutCont.Append("\to_%sTo%s_%sRdData%s = c_%sTo%s_%sRdData%s;\n",
					mod.m_modName.Lc().c_str(), gblPort.m_instName.Uc().c_str(), intGbl.m_gblName.c_str(), intGbl.m_dimenIndex.c_str(),
					mod.m_modName.Lc().c_str(), gblPort.m_instName.Uc().c_str(), intGbl.m_gblName.c_str(), intGbl.m_dimenIndex.c_str());

			} else if (gblPort.m_bOne1xReadIntf || gblPort.m_bFirst1xReadIntf) {
				if (bComment1x) {
					m_gblOut1x.Append("\t// External ram read data output\n");
					bComment1x = false;
				}

				GenRamIndexLoops(m_gblOut1x, "", intGbl);
				m_gblOut1x.Append("\to_%sTo%s_%sRdData%s = r_%sTo%s_%sRdData%s;\n",
					mod.m_modName.Lc().c_str(), gblPort.m_instName.Uc().c_str(), intGbl.m_gblName.c_str(), intGbl.m_dimenIndex.c_str(),
					mod.m_modName.Lc().c_str(), gblPort.m_instName.Uc().c_str(), intGbl.m_gblName.c_str(), intGbl.m_dimenIndex.c_str());

			} else {
				if (bComment2x) {
					m_gblOut2x.Append("\t// External ram read data output\n");
					bComment2x = false;
				}
				
				GenRamIndexLoops(m_gblOut2x, "", intGbl);
				m_gblOut2x.Append("\to_%sTo%s_%sRdData%s = r_%sTo%s_%sRdData%s;\n",
					mod.m_modName.Lc().c_str(), gblPort.m_instName.Uc().c_str(), intGbl.m_gblName.c_str(), intGbl.m_dimenIndex.c_str(),
					mod.m_modName.Lc().c_str(), gblPort.m_instName.Uc().c_str(), intGbl.m_gblName.c_str(), intGbl.m_dimenIndex.c_str());
			}
		}
	}
}


void CDsnInfo::GenDimenInfo(CDimenList & ramList, CDimenList & fieldList, vector<int> & dimenList,
	string & dimenDecl, string & dimenIndex, string * pVarIndex, string * pFldIndex)
{
	string		dimIdx = "1";

	for (size_t i = 0; i < ramList.m_dimenList.size(); i += 1) {
		dimenList.push_back(ramList.m_dimenList[i].AsInt());
		dimenDecl += "[" + ramList.m_dimenList[i].AsStr() + "]";
		dimenIndex += "[idx" + dimIdx + "]";
		if (pVarIndex)
			*pVarIndex += "[idx" + dimIdx + "]";
		dimIdx[0] += 1;
	}

	for (size_t i = 0; i < fieldList.m_dimenList.size(); i += 1) {
		dimenList.push_back(fieldList.m_dimenList[i].AsInt());
		dimenDecl += "[" + fieldList.m_dimenList[i].AsStr() + "]";
		dimenIndex += "[idx" + dimIdx + "]";
		if (pFldIndex)
			*pFldIndex += "[idx" + dimIdx + "]";
		dimIdx[0] += 1;
	}
}

string CDsnInfo::GenRamIndexLoops(CHtCode &ramCode, vector<int> & dimenList, bool bOpenParen)
{
	string tabs;
	for (size_t i = 0; i < dimenList.size(); i += 1) {
		tabs += "\t";
		ramCode.Append("\tfor (int idx%d = 0; idx%d < %d; idx%d += 1)%s\n%s",
			(int)i+1, (int)i+1, dimenList[i], (int)i+1, bOpenParen && i == dimenList.size()-1 ? " {" : "", tabs.c_str());
	}
	return tabs;
}

string CDsnInfo::GenRamIndexLoops(CHtCode &ramCode, const char *pTabs, CDimenList &dimenList, bool bOpenParen)
{
	string loopIdx;
	string indentTabs;
	for (size_t i = 0; i < dimenList.m_dimenList.size(); i += 1) {
		bool bIsSigned = m_defineTable.FindStringIsSigned(dimenList.m_dimenList[i].AsStr());
		ramCode.Append("%s\tfor (%s idx%d = 0; idx%d < %d; idx%d += 1)%s\n\t%s", pTabs,
			bIsSigned ? "int" : "unsigned", (int)i+1, (int)i+1, dimenList.m_dimenList[i].AsInt(), (int)i+1,
			bOpenParen && dimenList.m_dimenList.size() == i+1 ? " {" : "", indentTabs.c_str());
		indentTabs += "\t";
		loopIdx += VA("[\" + (std::string)HtStrFmt(\"%%d\", idx%d) + \"]", (int)i+1);
	}
	return loopIdx;
}

void CDsnInfo::GenRamIndexLoops(FILE *fp, CDimenList &dimenList, const char *pTabs)
{
	string indentTabs;
	for (size_t i = 0; i < dimenList.m_dimenList.size(); i += 1) {
		bool bIsSigned = m_defineTable.FindStringIsSigned(dimenList.m_dimenList[i].AsStr());
		fprintf(fp, "%sfor (%s idx%d = 0; idx%d < %s; idx%d += 1)\n\t%s",
			pTabs, bIsSigned ? "int" : "unsigned", (int)i+1, (int)i+1, dimenList.m_dimenList[i].c_str(), (int)i+1, indentTabs.c_str());
		indentTabs += "\t";
	}
}

void CDsnInfo::GenIndexLoops(CHtCode &htCode, string &tabs, vector<CHtString> & dimenList, bool bOpenParen)
{
	for (size_t i = 0; i < dimenList.size(); i += 1) {
		htCode.Append("%sfor (int idx%d = 0; idx%d < %d; idx%d += 1)%s\n",
			tabs.c_str(), (int)i + 1, (int)i + 1, dimenList[i].AsInt(), (int)i + 1,
			bOpenParen && i == dimenList.size() - 1 ? " {" : "", tabs.c_str()); 
		tabs += "\t";
	}
}
