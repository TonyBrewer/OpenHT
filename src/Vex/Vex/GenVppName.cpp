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

void
CDesign::GenVppFile(char *pName, bool bIsLedaEnabled)
{
	if (GetParseErrorCnt() > 0) {
		ParseMsg(PARSE_WARNING, "previous error prohibit generation of verilog file");
		return;
	}

	FILE *fp;

	if (!(fp = fopen(pName, "w"))) {
		printf("Could not open file '%s'\n", pName);
		return;
	}

	fprintf(fp, "`timescale 1 ns/1 ps\n");

	if (bIsLedaEnabled) {
		fprintf(fp, "// leda B_3200 off Unequal length operand\n");
		fprintf(fp, "// leda B_3208 off Unequal length operand\n");
		fprintf(fp, "// leda VER_2_10_3_5 off Specify base format\n");
		fprintf(fp, "// leda SYN9_22 off Delay values are ignored in synthesis\n");
		fprintf(fp, "// leda B_3401 off Blocking delay not allowed in non-blocking assignment\n");
		fprintf(fp, "// leda VCS_12 off Use only non-blocking assignments without delays in always blocks\n");
		fprintf(fp, "// leda W280 off Delay in non blocking assignment\n");
		fprintf(fp, "// leda W257 off Delays ignored by synthesis tools\n");
		fprintf(fp, "// leda W126 off Non integer delay\n");
		fprintf(fp, "// leda FM_2_2 off Delays are ignored by synthesis tools\n");
		fprintf(fp, "// leda VER_2_3_1_5 off\n");
		fprintf(fp, "// leda DCVER_130 off Intraassignment delays for nonblocking statements are ignored\n");
		fprintf(fp, "// leda DCVER_176 off Delay statements are ignored for synthesis\n");
		fprintf(fp, "// leda C_1C_R off No delay values are allowed in RTL code\n");
	}

	ModuleMap_Iter moduleIter;
	for (moduleIter = m_moduleMap.begin(); moduleIter != m_moduleMap.end(); moduleIter++) {
		CModule *pModule = &moduleIter->second;
		if (pModule->m_bPrim)
			continue;

		// associate each synthesis attribute to the module, a signal, or an instance
		for (int i = 0; i < (int)pModule->m_synAttrib.size(); i += 1) {
			CInst *pInst;
			if (pInst = pModule->FindInst(pModule->m_synAttrib[i].m_inst)) {

				// check for duplicate attributes
				string attrib = pModule->m_synAttrib[i].m_attrib;
				for (size_t j = 0; j < pInst->m_synAttrib.size(); j += 1) {
					if (pInst->m_synAttrib[j].m_attrib == attrib) {
						ParseMsg(PARSE_ERROR, "Multiple %s synthesis attribute found for instance %s in module %s\n",
							attrib.c_str(), pModule->m_synAttrib[i].m_inst.c_str(), pModule->m_name.c_str());
						break;
					}
				}

				pInst->m_synAttrib.push_back(pModule->m_synAttrib[i]);
			} else
				ParseMsg(PARSE_ERROR, "Could not find synthesis attribute instance '%s' in module '%s'\n",
					pModule->m_synAttrib[i].m_inst.c_str(), pModule->m_name.c_str());
		}

		fprintf(fp, "\nmodule %s (", pModule->m_name.c_str());

		char *pFirst = "";
		SignalMap_Iter sigIter;
		for (sigIter = pModule->m_signalMap.begin(); sigIter != pModule->m_signalMap.end(); sigIter++) {
			CSignal *pSignal = sigIter->second;

			if (pSignal->m_sigType == sig_input) {
				fprintf(fp, "%s %s", pFirst, pSignal->m_name.c_str());
				pFirst = ",";
			}
		}

		for (sigIter = pModule->m_signalMap.begin(); sigIter != pModule->m_signalMap.end(); sigIter++) {
			CSignal *pSignal = sigIter->second;

			if (pSignal->m_sigType == sig_output) {
				fprintf(fp, "%s %s", pFirst, pSignal->m_name.c_str());
				pFirst = ",";
			}
		}

		fprintf(fp, " );\n");

		fprintf(fp, "    // synthesis attribute keep_hierarchy %s \"true\";\n", pModule->m_name.c_str());

		for (sigIter = pModule->m_signalMap.begin(); sigIter != pModule->m_signalMap.end(); sigIter++) {
			CSignal *pSignal = sigIter->second;

			if (pSignal->m_sigType == sig_input) {
				char indexStr[32];
				if (pSignal->m_bIndexed)
					sprintf(indexStr, " [%d : %d] ", pSignal->m_upperIdx, pSignal->m_lowerIdx);
				else
					strcpy(indexStr, " ");

				fprintf(fp, "    input%s%s;\n", indexStr, pSignal->m_name.c_str());
			}
		}

		for (sigIter = pModule->m_signalMap.begin(); sigIter != pModule->m_signalMap.end(); sigIter++) {
			CSignal *pSignal = sigIter->second;

			if (pSignal->m_sigType == sig_output) {
				char indexStr[32];
				if (pSignal->m_bIndexed)
					sprintf(indexStr, " [%d : %d] ", pSignal->m_upperIdx, pSignal->m_lowerIdx);
				else
					strcpy(indexStr, " ");

				fprintf(fp, "    output%s%s;\n", indexStr, pSignal->m_name.c_str());
			}
		}

		if (pModule->m_ifdefLine.size() > 0) {
			fprintf(fp, "`ifdef %s\n", pModule->m_ifdefName.c_str());
			for (size_t i = 0; i < pModule->m_ifdefLine.size(); i += 1)
				fprintf(fp, "    %s\n", pModule->m_ifdefLine[i].c_str());
			fprintf(fp, "`else\n");
		}

		for (sigIter = pModule->m_signalMap.begin(); sigIter != pModule->m_signalMap.end(); sigIter++) {
			CSignal *pSignal = sigIter->second;

			if (pSignal->m_sigType == sig_wire) {
				char indexStr[32];
				if (pSignal->m_bIndexed)
					sprintf(indexStr, " [%d : %d] ", pSignal->m_upperIdx, pSignal->m_lowerIdx);
				else
					strcpy(indexStr, " ");

				size_t size = pSignal->m_name.size();
				string tailStr;
				if (size > 3)
					pSignal->m_name.substr(size-3, 3);

				fprintf(fp, "    wire%s%s;\n", indexStr, pSignal->m_name.c_str()/*, tailStr == "$UN" ? "" : " / * synthesis syn_keep=1 * /"*/);
			}
		}

		InstMap_Iter instIter;
		for (instIter = pModule->m_instMap.begin(); instIter != pModule->m_instMap.end(); instIter++) {
			CInst *pInst = instIter->second;

			if (pInst->m_pModule->m_bDummyLoad)
				continue;

			string instName = GetVppName(pInst->m_name);
			if (pInst->m_bDefparamInit)
				fprintf(fp, "    defparam %s.INIT = %s;\n", instName.c_str(), pInst->m_initStr.c_str());

			for (size_t i = 0; i < pInst->m_defparams.size(); i += 1) {
				if (pInst->m_defparams[i].m_bIsString)
					fprintf(fp, "    defparam %s = \"%s\";\n", pInst->m_defparams[i].m_heirName.c_str(), pInst->m_defparams[i].m_paramValue.c_str());
				else
					fprintf(fp, "    defparam %s = %s;\n", pInst->m_defparams[i].m_heirName.c_str(), pInst->m_defparams[i].m_paramValue.c_str());
			}

			for (int i = 0; i < (int)pInst->m_synAttrib.size(); i += 1)
				fprintf(fp, "    (* %s = \"%s\" *)\n", pInst->m_synAttrib[i].m_attrib.c_str(), pInst->m_synAttrib[i].m_value.c_str());

			char *pFirst = "";
			//fprintf(fp, "    (* S = \"TRUE\" *)\n");
			fprintf(fp, "    %s %s (", pInst->m_pModule->m_name.c_str(), instName.c_str());

			vector<CInstPort> &instPorts = pInst->m_instPorts;
			for (unsigned int idx = 0; idx < instPorts.size(); idx += 1) {
				// print signals on port
				CSignal *pModulePort = instPorts[idx].m_pModulePort;
				fprintf(fp, "%s .%s( ", pFirst, pModulePort->m_name.c_str());
				pFirst = ",";

				if (pModulePort->GetWidth() == 1) {
					if (instPorts[idx].m_pSingle) {
						CSignal *pSignal = instPorts[idx].m_pSingle->m_pSignal;
						if (pSignal == &m_zeroSignal)
							fprintf(fp, "1'b0");
						else if (pSignal == &m_oneSignal)
							fprintf(fp, "1'b1");
						else if (true || !pSignal->m_bUnconnected) {
							if (pSignal->m_bIndexed)
								fprintf(fp, "%s[%d]", pSignal->m_name.c_str(),
									instPorts[idx].m_pSingle->m_sigBit);
							else
								fprintf(fp, "%s", pSignal->m_name.c_str());
						} else
							fprintf(fp, "/* NC */");
					}
				} else {
					char *pFirst2 = "{ ";
					char *pLast2 = " }";
					for (unsigned int sigIdx = 0; sigIdx < instPorts[idx].m_pVector->size(); sigIdx += 1) {
						CSignal *pSignal = (*instPorts[idx].m_pVector)[sigIdx].m_pSignal;
						if (pSignal->m_bIndexed) {
							int sigCnt = 1;
							int repCnt = 1;
							int highSigBit = (*instPorts[idx].m_pVector)[sigIdx].m_sigBit;
							int lowSigBit = highSigBit;

							while (sigIdx+1 < instPorts[idx].m_pVector->size() &&
								pSignal == (*instPorts[idx].m_pVector)[sigIdx+1].m_pSignal &&
								lowSigBit-1 == (*instPorts[idx].m_pVector)[sigIdx+1].m_sigBit) {

								sigIdx += 1;
								lowSigBit -= 1;
								sigCnt += 1;
							}

							if (sigCnt == 1) {
								while (sigIdx+1 < instPorts[idx].m_pVector->size() &&
									pSignal == (*instPorts[idx].m_pVector)[sigIdx+1].m_pSignal &&
									lowSigBit == (*instPorts[idx].m_pVector)[sigIdx+1].m_sigBit) {

									sigIdx += 1;
									repCnt += 1;
								}
							}

							if (repCnt > 1) {
								fprintf(fp, "%s {%d{%s[%d]}}", pFirst2, repCnt, pSignal->m_name.c_str(), highSigBit);
							} else if (sigCnt == (int)instPorts[idx].m_pVector->size()) {
								fprintf(fp, "%s[%d:%d]", pSignal->m_name.c_str(), highSigBit, lowSigBit);
								pLast2 = 0;
							} else if (highSigBit != lowSigBit) {
								fprintf(fp, "%s %s[%d:%d]", pFirst2, pSignal->m_name.c_str(), highSigBit, lowSigBit);
							} else
								fprintf(fp, "%s %s[%d]", pFirst2, pSignal->m_name.c_str(), highSigBit);

						} else if (pSignal == &m_zeroSignal || pSignal == &m_oneSignal) {
							int width = 0;
							uint64 value = 0;
							while (sigIdx < instPorts[idx].m_pVector->size() &&
								(&m_zeroSignal == (*instPorts[idx].m_pVector)[sigIdx].m_pSignal ||
								&m_oneSignal == (*instPorts[idx].m_pVector)[sigIdx].m_pSignal)) {

								if ((*instPorts[idx].m_pVector)[sigIdx].m_pSignal == &m_zeroSignal)
									value = value << 1;
								else
									value = (value << 1) | 1;

								sigIdx += 1;
								width += 1;
							}

							sigIdx -= 1;
							fprintf(fp, "%s %d'h%llx", pFirst2, width, value);

						} else {
							int repCnt = 1;
							while (sigIdx+1 < instPorts[idx].m_pVector->size() &&
								pSignal == (*instPorts[idx].m_pVector)[sigIdx+1].m_pSignal) {

								sigIdx += 1;
								repCnt += 1;
							}

							if (repCnt > 1)
								fprintf(fp, "%s {%d{%s}}", pFirst2, repCnt, pSignal->m_name.c_str());
							else
								fprintf(fp, "%s %s", pFirst2, pSignal->m_name.c_str());
						}

						pFirst2 = ",";
					}
					if (pLast2)
						fprintf(fp, " }");
				}

				fprintf(fp, " )");
			}

			fprintf(fp, " );\n");
		}

		if (pModule->m_ifdefLine.size() > 0)
			fprintf(fp, "`endif\n");

		fprintf(fp, "endmodule\n");
	}

	if (bIsLedaEnabled) {
		fprintf(fp, "// leda B_3200 on Unequal length operand\n");
		fprintf(fp, "// leda B_3208 on Unequal length operand\n");
		fprintf(fp, "// leda VER_2_10_3_5 on Specify base format\n");
		fprintf(fp, "// leda SYN9_22 on Delay values are ignored in synthesis\n");
		fprintf(fp, "// leda B_3401 on Blocking delay not allowed in non-blocking assignment\n");
		fprintf(fp, "// leda VCS_12 on Use only non-blocking assignments without delays in always blocks\n");
		fprintf(fp, "// leda W280 on Delay in non blocking assignment\n");
		fprintf(fp, "// leda W257 on Delays ignored by synthesis tools\n");
		fprintf(fp, "// leda W126 on Non integer delay\n");
		fprintf(fp, "// leda FM_2_2 on Delays are ignored by synthesis tools\n");
		fprintf(fp, "// leda VER_2_3_1_5 on\n");
		fprintf(fp, "// leda DCVER_130 on Intraassignment delays for nonblocking statements are ignored\n");
		fprintf(fp, "// leda DCVER_176 on Delay statements are ignored for synthesis\n");
		fprintf(fp, "// leda C_1C_R on No delay values are allowed in RTL code\n");
	}
	
	fclose(fp);
}

string
CDesign::GetVppName(string &name)
{
	if (name[0] == '_' || isalpha(name[0]))
		return name;

	char buf[256];
	char *pBuf = buf;
	const char *pStr = name.c_str();
	while (*pStr != '\0') {
		if (*pStr != '_' && *pStr != '$' && !isalnum(*pStr))
			*pBuf++ = '_';
		else
			*pBuf++ = *pStr;
		pStr ++;
	}
	*pBuf = '\0';

	return string(buf);
}
