/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "stdafx.h"
#include "Design.h"

CDesign::CSignalPortIter::CSignalPortIter(CDesign::CSignal *pSignal, int sigBit) : m_portIdx(0)
{
	if (pSignal->m_bIndexed)
		m_pSignalPorts = &(*pSignal->m_pIndexedSignalPorts)[sigBit];
	else
		m_pSignalPorts = pSignal->m_pSignalPorts;
}

void
CDesign::GenFFChainRpt(FILE *fp)
{
	if (GetParseErrorCnt() > 0)
		return;

	ModuleMap_Iter moduleIter;
	for (moduleIter = m_moduleMap.begin(); moduleIter != m_moduleMap.end(); moduleIter++) {
		CModule *pModule = &moduleIter->second;
		if (pModule->m_bPrim)
			continue;

		InstMap_Iter instIter;
		for (instIter = pModule->m_instMap.begin(); instIter != pModule->m_instMap.end(); instIter++) {
			CInst *pInst = instIter->second;

			const char *pName = pInst->m_pModule->m_name.c_str();
			if (strncmp(pName, "FD", 2) != 0)
				continue;

			// check if D input signal has other loads or if driver of signal is a FD primitive
			if (pInst->GetPortSignal("D")->GetLoadCnt() == 1 && IsDInputSignalDriverFDPrim(pInst))
				continue;

			// found first FD primitive of possible chain, count FDs in chain. Last FD can be FD, FDR or FDS
			CInst *pFdInst = pInst;
			int fdCnt = 0;
			for (;;) {
				CPortSignal *pQSignal = pFdInst->GetPortSignal("Q");
				if (pQSignal->GetLoadCnt() != 1)
					break;
				pFdInst = pQSignal->GetFirstInputInst();
				pName = pFdInst->GetModuleName().c_str();
				if (strncmp(pName, "FD", 2) != 0)
					break;
				fdCnt += 1;
				if (pName[2] != '\0')
					break;
			}

			if (fdCnt > 0)
				fprintf(fp, "Instance %s (%s) has chain of %d FD primitives\n", pFdInst->m_name.c_str(), pFdInst->GetModuleName().c_str(), fdCnt);
		}
	}
}

bool
CDesign::IsDInputSignalDriverFDPrim(CInst *pInst)
{
	CInstPortIter instPortIter = pInst->GetInstPortIter();
	for ( ; !instPortIter.end(); instPortIter++) {
		if (instPortIter->GetModulePortName() != "D")
			continue;

		assert(instPortIter->GetModulePortWidth() == 1);

		CSignalPortIter sigPortIter = instPortIter->GetPortSignal()->GetSignalPortIter();
		for ( ; !sigPortIter.end(); sigPortIter++) {
			if (sigPortIter->IsOutput()) {
				string name = sigPortIter->GetModuleName();
				return sigPortIter->IsInstPrim() && strncmp(name.c_str(), "FD", 2) == 0;
			}				
		}

		return false; // no driver on net
	}

	assert(0);	// no D input
	return false;
}

CDesign::CPortSignal *
CDesign::CInst::GetPortSignal(char *pName)
{
	CInstPortIter instPortIter = GetInstPortIter();
	for ( ; !instPortIter.end(); instPortIter++)
		if (instPortIter->GetModulePortName() == pName)
			return instPortIter->GetPortSignal();

	assert(0);
	return 0;
}

int
CDesign::CPortSignal::GetLoadCnt()
{
	int loadCnt = 0;
	CSignalPortIter sigPortIter = GetSignalPortIter();
	for ( ; !sigPortIter.end(); sigPortIter++) {
		if (sigPortIter->IsInput())
			loadCnt += 1;
	}

	return loadCnt;
}

CDesign::CInst *
CDesign::CPortSignal::GetFirstInputInst()
{
	CSignalPortIter sigPortIter = GetSignalPortIter();
	for ( ; !sigPortIter.end(); sigPortIter++)
		if (sigPortIter->IsInput())
			return sigPortIter->GetInst();

	return 0;
}

int
CDesign::CSignal::GetLoadCnt()
{
	assert(0);
	return 0;
}

void
CDesign::GenConnRpt(FILE *fp)
{
	if (GetParseErrorCnt() > 0) {
		ParseMsg(PARSE_WARNING, "previous error prohibit generation of connectivity report");
		return;
	}

	ModuleMap_Iter moduleIter;
	for (moduleIter = m_moduleMap.begin(); moduleIter != m_moduleMap.end(); moduleIter++) {
		CModule *pModule = &moduleIter->second;
		if (pModule->m_bPrim)
			continue;

		fprintf(fp, "Module: %s\n", pModule->m_name.c_str());

		GetTotalInstCnt(fp, pModule);

		// first dump out instances
		InstMap_Iter instIter;
		for (instIter = pModule->m_instMap.begin(); instIter != pModule->m_instMap.end(); instIter++) {
			CInst *pInst = instIter->second;
			fprintf(fp, "  %-8s   %dL  %s\n", pInst->m_pModule->m_name.c_str(), pInst->m_lvl, pInst->m_name.c_str());

			vector<CInstPort> &instPorts = pInst->m_instPorts;
			for (unsigned int idx = 0; idx < instPorts.size(); idx += 1) {
				// check signals on port
				if (instPorts[idx].m_pModulePort->GetWidth() == 1) {
					if (instPorts[idx].m_pSingle == 0)
						continue;

					CSignal *pSignal = instPorts[idx].m_pSingle->m_pSignal;
					char signalName[256];
					if (pSignal->m_bIndexed)
						sprintf(signalName, "'%s'[%d]", pSignal->m_name.c_str(),
							instPorts[idx].m_pSingle->m_sigBit);
					else
						sprintf(signalName, "'%s'", pSignal->m_name.c_str());

					fprintf(fp, "    %s  %-10s    %s\n",
						instPorts[idx].m_pModulePort->m_sigType == sig_input ? "I" : "O",
						instPorts[idx].m_pModulePort->m_name.c_str(),
						signalName);
				} else {
					for (unsigned int sigIdx = 0; sigIdx < instPorts[idx].m_pVector->size(); sigIdx += 1) {
						CSignal *pSignal = (*instPorts[idx].m_pVector)[sigIdx].m_pSignal;
						char signalName[256];
						if (pSignal->m_bIndexed)
							sprintf(signalName, "'%s'[%d]", pSignal->m_name.c_str(),
								(*instPorts[idx].m_pVector)[sigIdx].m_sigBit);
						else
							sprintf(signalName, "'%s'", pSignal->m_name.c_str());

						char portName[32];
						sprintf(portName, "%s[%d]", instPorts[idx].m_pModulePort->m_name.c_str(), sigIdx);
						fprintf(fp, "    %s  %-10s    %s\n",
							instPorts[idx].m_pModulePort->m_sigType == sig_input ? "I" : "O",
							portName,
							signalName);
					}
				}
			}
		}

		// now dump out signals
		SignalMap_Iter sigIter;
		for (sigIter = pModule->m_signalMap.begin(); sigIter != pModule->m_signalMap.end(); sigIter++) {
			CSignal *pSignal = sigIter->second;
			SignalPortVector *pSignalPorts;

			if (pSignal->m_bIndexed) {
				for (int sigBit = pSignal->m_lowerIdx; sigBit <= pSignal->m_upperIdx; sigBit += 1) {
					fprintf(fp, "  '%s'[%d]\n", pSignal->m_name.c_str(), sigBit);
					pSignalPorts = &(*pSignal->m_pIndexedSignalPorts)[sigBit];

					for (unsigned int portIdx = 0; portIdx < pSignalPorts->size(); portIdx += 1) {
						CSignalPort *pSignalPort = &(*pSignalPorts)[portIdx];

						CInstPort *pInstPort = &pSignalPort->m_pInst->m_instPorts[pSignalPort->m_portIdx];

						char portName[32];
						if (pInstPort->m_pModulePort->GetWidth() == 1)
							sprintf(portName, "%s", pInstPort->m_pModulePort->m_name.c_str());
						else
							sprintf(portName, "%s[%d]", pInstPort->m_pModulePort->m_name.c_str(), pSignalPort->m_portBit);

						fprintf(fp, "    %s  %-10s  %-8s  %s\n",
							pInstPort->m_pModulePort->m_sigType == sig_input ? "I" : "O",
							portName,
							pSignalPort->m_pInst->m_pModule->m_name.c_str(),
							pSignalPort->m_pInst->m_name.c_str());
					}
				}

			} else {
				fprintf(fp, "  '%s'\n", pSignal->m_name.c_str());
				pSignalPorts = pSignal->m_pSignalPorts;

				for (unsigned int portIdx = 0; portIdx < pSignalPorts->size(); portIdx += 1) {
					CSignalPort *pSignalPort = &(*pSignalPorts)[portIdx];

					CInstPort *pInstPort = &pSignalPort->m_pInst->m_instPorts[pSignalPort->m_portIdx];

					char portName[32];
					if (pInstPort->m_pModulePort->GetWidth() == 1)
						sprintf(portName, "%s", pInstPort->m_pModulePort->m_name.c_str());
					else
						sprintf(portName, "%s[%d]", pInstPort->m_pModulePort->m_name.c_str(), pSignalPort->m_portBit);

					fprintf(fp, "    %s  %-10s  %-8s  %s\n",
						pInstPort->m_pModulePort->m_sigType == sig_input ? "I" : "O",
						portName,
						pSignalPort->m_pInst->m_name.c_str(),
						pSignalPort->m_pInst->m_name.c_str());
				}
			}
		}
	}
}

void
CDesign::GetTotalInstCnt(FILE *fp, CModule *pModule)
{
	ModuleMap_Iter moduleIter;
	for (moduleIter = m_moduleMap.begin(); moduleIter != m_moduleMap.end(); moduleIter++)
		moduleIter->second.m_useCnt = 0;

	GetTotalInstCnt(pModule);

	fprintf(fp, "  Total primitive count:\n");
	for (moduleIter = m_moduleMap.begin(); moduleIter != m_moduleMap.end(); moduleIter++)
		if (moduleIter->second.m_useCnt > 0)
			fprintf(fp, "    %8s  %d\n", moduleIter->second.m_name.c_str(), moduleIter->second.m_useCnt);
	fprintf(fp, "\n");
}

void
CDesign::GetTotalInstCnt(CModule *pModule)
{
	InstMap_Iter instIter;
	for (instIter = pModule->m_instMap.begin(); instIter != pModule->m_instMap.end(); instIter++) {
		if (instIter->second->m_pModule->m_bPrim)
			instIter->second->m_pModule->m_useCnt += 1;
		else
			GetTotalInstCnt(instIter->second->m_pModule);
	}
}

void
CDesign::SetInstLvl(FILE *fp, char *pInstNamePrefix)
{
	int levelZeroFFCnt = 0;
	size_t prefixLen = strlen(pInstNamePrefix);

	// set all instance levels to -1
	InstMap_Iter instIter;
	for (instIter = m_pModule->m_instMap.begin(); instIter != m_pModule->m_instMap.end(); instIter++)
		instIter->second->m_lvl = -1;

	// set all signal levels to -1
	SignalMap_Iter sigIter;
	for (sigIter = m_pModule->m_signalMap.begin(); sigIter != m_pModule->m_signalMap.end(); sigIter++)
		sigIter->second->m_lvl = -1;

	// check all FFs for inputs that don't match prefix, mark them as level 0
	for (instIter = m_pModule->m_instMap.begin(); instIter != m_pModule->m_instMap.end(); instIter++) {
		//if (instIter->second->m_name.c_str() != "X_FF")
		if (instIter->second->m_name.compare("X_FF") != 0)
			continue;

		if (strncmp(pInstNamePrefix, instIter->second->m_name.c_str(), prefixLen) != 0)
			continue;

		// recursively check non-clock inputs for logic cones with all inputs either input signals or non-prefix FFs
		bool bIsPreLevelZero = true;
		vector<CInstPort> &instPorts = instIter->second->m_instPorts;
		for (unsigned int idx = 0; idx < instPorts.size(); idx += 1) {
			if (instPorts[idx].m_pModulePort->m_bClock || !instPorts[idx].m_pModulePort->m_sigType == sig_input)
				continue;

			// check signals on port
			if (instPorts[idx].m_pModulePort->GetWidth() == 1) {
				if (!IsInputPreLevelZero(pInstNamePrefix,
							instPorts[idx].m_pSingle->m_pSignal,
							instPorts[idx].m_pSingle->m_sigBit)) {
					bIsPreLevelZero = false;
					break;
				}
			} else {
				for (unsigned int sigIdx = 0; sigIdx < instPorts[idx].m_pVector->size(); sigIdx += 1) {
					if (!IsInputPreLevelZero(pInstNamePrefix,
								(*instPorts[idx].m_pVector)[sigIdx].m_pSignal,
								(*instPorts[idx].m_pVector)[sigIdx].m_sigBit)) {

						bIsPreLevelZero = false;
						break;
					}
				}
			}
		}
		if (bIsPreLevelZero) {
			levelZeroFFCnt += 1;
			instIter->second->m_lvl = 0;
			//printf("%s\n", instIter->second->m_name.c_str());
		}
	}

	// recursively walk the logic forward from the level 0 FFs, setting the level
	for (instIter = m_pModule->m_instMap.begin(); instIter != m_pModule->m_instMap.end(); instIter++) {
		//if (instIter->second->m_name.c_str() != "X_FF" || instIter->second->m_lvl != 0)
		if (instIter->second->m_name.compare("X_FF") != 0 || instIter->second->m_lvl != 0)
			continue;

		SetInstLvl(instIter->second, 0);
	}

	// report number of instances at each level
	for (int lvl = 0; lvl < 10; lvl += 1) {
		int instCnt = 0;

		ModuleMap_Iter moduleIter;
		for (moduleIter = m_moduleMap.begin(); moduleIter != m_moduleMap.end(); moduleIter++)
			moduleIter->second.m_useCnt = 0;

		InstMap_Iter instIter;
		for (instIter = m_pModule->m_instMap.begin(); instIter != m_pModule->m_instMap.end(); instIter++)
			if (instIter->second->m_lvl == lvl) {
				instIter->second->m_pModule->m_useCnt += 1;
				instCnt += 1;
			}

		fprintf(fp, "\nLvl %d:\n", lvl);
		for (moduleIter = m_moduleMap.begin(); moduleIter != m_moduleMap.end(); moduleIter++)
			if (moduleIter->second.m_useCnt > 0)
				fprintf(fp, "  %8s  %d\n", moduleIter->second.m_name.c_str(), moduleIter->second.m_useCnt);

		//if (instCnt == 0)
		//	break;
	}
}

bool
CDesign::IsInputPreLevelZero(char *pInstNamePrefix, CSignal *pSignal, int sigBit)
{
	if (pSignal->m_sigType == sig_input)
		return true;

	size_t prefixLen = strlen(pInstNamePrefix);

	// find driver of signal
	SignalPortVector *pSignalPorts;
	if (pSignal->m_bIndexed)
		pSignalPorts = &(*pSignal->m_pIndexedSignalPorts)[sigBit];
	else
		pSignalPorts = pSignal->m_pSignalPorts;

	int outputCnt = 0;
	for (unsigned int portIdx = 0; portIdx < pSignalPorts->size(); portIdx += 1) {
		CSignalPort *pSignalPort = &(*pSignalPorts)[portIdx];

		CInstPort *pInstPort = &pSignalPort->m_pInst->m_instPorts[pSignalPort->m_portIdx];

		if (pInstPort->m_pModulePort->m_sigType == sig_input)
			continue;

		outputCnt += 1;
		if (strncmp(pInstNamePrefix, pSignalPort->m_pInst->m_name.c_str(), prefixLen) == 0)
			return false;
	}

	assert(outputCnt == 1);

	return true;
}

void
CDesign::SetInstLvl(CSignal *pSignal, int sigBit, int lvl)
{
	//if (pSignal->m_lvl != -1 && pSignal->m_lvl != lvl) {
	//	printf("Unexpected signal level (%d/%d): %s\n", pSignal->m_lvl, lvl, pSignal->m_name.c_str());
	//	if (pSignal->m_lvl > lvl)
	//		return;
	//} else if (pSignal->m_lvl == lvl)
	//	return;

	pSignal->m_lvl = lvl;

	SignalPortVector *pSignalPorts;
	if (pSignal->m_bIndexed)
		pSignalPorts = &(*pSignal->m_pIndexedSignalPorts)[sigBit];
	else
		pSignalPorts = pSignal->m_pSignalPorts;

	for (unsigned int portIdx = 0; portIdx < pSignalPorts->size(); portIdx += 1) {
		CSignalPort *pSignalPort = &(*pSignalPorts)[portIdx];

		CInstPort *pInstPort = &pSignalPort->m_pInst->m_instPorts[pSignalPort->m_portIdx];

		if (!pInstPort->m_pModulePort->m_sigType == sig_input)
			continue;

		SetInstLvl(pSignalPort->m_pInst, lvl);
	}
}

void
CDesign::SetInstLvl(CInst *pInst, int lvl)
{
	if (pInst->m_lvl != -1 && pInst->m_lvl != lvl) {
		printf("Unexpected instance level (%d/%d): %s\n", pInst->m_lvl, lvl, pInst->m_name.c_str());
		if (pInst->m_lvl > lvl)
			return;
	} else if (pInst->m_lvl == lvl && lvl > 0)
		return;

	pInst->m_lvl = lvl;
	//printf("lvl %d %-8s  %s \n", pInst->m_lvl, pInst->m_name.c_str(), pInst->m_name.c_str());

	//if (pInst->m_name.c_str() == "X_FF")
	if (pInst->m_name.compare("X_FF") == 0)
		lvl += 1;

	// now find output port
	vector<CInstPort> &instPorts = pInst->m_instPorts;
	unsigned int portIdx;
	for (portIdx = 0; portIdx < instPorts.size(); portIdx += 1) {
		if (instPorts[portIdx].m_pModulePort->m_sigType == sig_input)
			continue;

		// check signal on output port
		if (instPorts[portIdx].m_pModulePort->GetWidth() == 1)
			SetInstLvl(instPorts[portIdx].m_pSingle->m_pSignal, instPorts[portIdx].m_pSingle->m_sigBit, lvl);
		else
			for (unsigned int sigIdx = 0; sigIdx < instPorts[portIdx].m_pVector->size(); sigIdx += 1)
				SetInstLvl((*instPorts[portIdx].m_pVector)[sigIdx].m_pSignal,
						(*instPorts[portIdx].m_pVector)[sigIdx].m_sigBit, lvl);
	}
}

void
CDesign::PerformModuleChecks()
{
	ModuleMap_Iter moduleIter;
	for (moduleIter = m_moduleMap.begin(); moduleIter != m_moduleMap.end(); moduleIter++) {
		CModule *pModule = &moduleIter->second;
		if (pModule->m_bPrim)
			continue;

		// verify all module ports were declared
		SignalMap_Iter sigIter;
		for (sigIter = pModule->m_signalMap.begin(); sigIter != pModule->m_signalMap.end(); sigIter++) {
			if (sigIter->second->m_sigType == sig_port)
				ParseMsg(PARSE_ERROR, sigIter->second->m_lineInfo,
					"Port signal '%s' was not declared", sigIter->second->m_name.c_str());
		}

		// verify all signals have one output and at least one input
		for (sigIter = pModule->m_signalMap.begin(); sigIter != pModule->m_signalMap.end(); sigIter++) {
			CSignal *pSignal = sigIter->second;
			SignalPortVector *pSignalPorts;

			// count input and output instance ports for signal
			if (pSignal->m_bIndexed) {
				for (int sigBit = pSignal->m_lowerIdx; sigBit <= pSignal->m_upperIdx; sigBit += 1) {
					pSignalPorts = &(*pSignal->m_pIndexedSignalPorts)[sigBit];

					int inputCnt = 0;
					int outputCnt = 0;

					for (unsigned int portIdx = 0; portIdx < pSignalPorts->size(); portIdx += 1) {
						CSignalPort *pSignalPort = &(*pSignalPorts)[portIdx];

						CInstPort *pInstPort = &pSignalPort->m_pInst->m_instPorts[pSignalPort->m_portIdx];

						switch (pInstPort->m_pModulePort->m_sigType) {
							case sig_input: inputCnt += 1; break;
							case sig_output: outputCnt += 1; break;
							default: assert(0);
						}
					}

					if (pSignal->m_sigType == sig_output)
						inputCnt += 1;
					if (inputCnt == 0 && !pSignal->m_bUnconnected)
						ParseMsg(PARSE_WARNING, pSignal->m_lineInfo, "Signal '%s'[%d] has no load", pSignal->m_name.c_str(), sigBit);

					if (pSignal->m_sigType == sig_input)
						outputCnt += 1;
					if (outputCnt == 0)
						ParseMsg(PARSE_WARNING, pSignal->m_lineInfo, "Signal '%s'[%d] has no driver", pSignal->m_name.c_str(), sigBit);
					else if (outputCnt > 1)
						ParseMsg(PARSE_WARNING, pSignal->m_lineInfo, "Signal '%s'[%d] has multiple drivers", pSignal->m_name.c_str(), sigBit);
				}

			} else {
				pSignalPorts = pSignal->m_pSignalPorts;

				int inputCnt = 0;
				int outputCnt = 0;

				for (unsigned int portIdx = 0; portIdx < pSignalPorts->size(); portIdx += 1) {
					CSignalPort *pSignalPort = &(*pSignalPorts)[portIdx];

					CInstPort *pInstPort = &pSignalPort->m_pInst->m_instPorts[pSignalPort->m_portIdx];

					switch (pInstPort->m_pModulePort->m_sigType) {
						case sig_input: inputCnt += 1; break;
						case sig_output: outputCnt += 1; break;
						default: assert(0);
					}
				}

				if (pSignal->m_sigType == sig_output)
					inputCnt += 1;
				if (inputCnt == 0 && !pSignal->m_bUnconnected)
					ParseMsg(PARSE_WARNING, pSignal->m_lineInfo, "Signal '%s' has no loads", pSignal->m_name.c_str());

				if (pSignal->m_sigType == sig_input)
					outputCnt += 1;
				if (outputCnt == 0)
					ParseMsg(PARSE_WARNING, pSignal->m_lineInfo, "Signal '%s' has no driver", pSignal->m_name.c_str());
				else if (outputCnt > 1)
					ParseMsg(PARSE_WARNING, pSignal->m_lineInfo, "Signal '%s' has multiple drivers", pSignal->m_name.c_str());
			}
		}

		// verify defparam path names are valid
		static const basic_string <char>::size_type npos = -1;

		DefparamMap_Iter defparamIter;
		for (defparamIter = pModule->m_defparamMap.begin(); defparamIter != pModule->m_defparamMap.end(); defparamIter++) {
			CDefparam *pDefparam = &defparamIter->second;
			string &heirName = pDefparam->m_heirName;

			CModule *pHeirModule = pModule;

			CInst *pInst = 0;
			string instName;
			size_t pos = 0;
			size_t nextPos;
			for (; pos != heirName.size(); pos = nextPos+1) {
				nextPos = heirName.find_first_of('.', pos);
				instName = heirName.substr(pos, nextPos - pos);

				if (nextPos == npos)
					break;

				pInst = pHeirModule->FindInst(instName);
				if (!pInst) {
					ParseMsg(PARSE_WARNING, pDefparam->m_lineInfo, "invalid defparam heirarchical name, '%s'", heirName.c_str());
					break;
				}
				pHeirModule = pInst->m_pModule;
			}

			if (pInst) {
				if (heirName.substr(heirName.size()-5, 5) == ".INIT" && strncmp(pHeirModule->m_name.c_str(), "LUT", 3) == 0) {
					pInst->m_bDefparamInit = true;
					uint64 initValue = GenDefparamInit(pDefparam, pHeirModule->m_name[3]);
					char initStr[32];
					sprintf(initStr, "%d'h%llx", 1 << (pHeirModule->m_name[3] - '0'), initValue);
					pInst->m_initStr = initStr;
				} else
					pInst->m_defparams.push_back(*pDefparam);
			}
		}

		// verify all LUT instances have a defparam INIT
		InstMap_Iter instIter;
		for (instIter = pModule->m_instMap.begin(); instIter != pModule->m_instMap.end(); instIter++) {
			CInst *pInst = instIter->second;
			if (strncmp(pInst->m_pModule->m_name.c_str(), "LUT", 3) == 0 && !pInst->m_bDefparamInit)
				ParseMsg(PARSE_WARNING, pInst->m_lineInfo, "LUT primitive does not have INIT defparam, instance %s", pInst->m_name.c_str());
		}
	}
}

void
CDesign::WritePrpFile(const string &vexName)
{
	string prpName = vexName;
	size_t periodPos = prpName.find_last_of(".");
	if (periodPos > 0)
		prpName.erase(periodPos+1);
	prpName += "prp";

	FILE *prpFp;
	if (!(prpFp = fopen(prpName.c_str(), "w"))) {
		printf("Could not open file '%s' for writing\n", prpName.c_str());
		exit(1);
	}

	ModuleMap_Iter moduleIter;
	for (moduleIter = m_moduleMap.begin(); moduleIter != m_moduleMap.end(); moduleIter++) {
		CModule *pModule = &moduleIter->second;
		if (pModule->m_bPrim)
			continue;

		InstMap_Iter instIter;
		for (instIter = pModule->m_instMap.begin(); instIter != pModule->m_instMap.end(); instIter++) {
			CInst *pInst = instIter->second;

			string instName = GetVppName(pInst->m_name);

			for (int i = 0; i < (int)pInst->m_synAttrib.size(); i += 1)
				fprintf(prpFp, "%s %s %s %s\n", pModule->m_name.c_str(), instName.c_str(), 
					pInst->m_synAttrib[i].m_attrib.c_str(), pInst->m_synAttrib[i].m_value.c_str());

		}
	}

	fclose(prpFp);
}
