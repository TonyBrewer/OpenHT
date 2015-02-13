/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
// Synthesize.cpp
//
//////////////////////////////////////////////////////////////////////

#include "Htv.h"
#include "HtvDesign.h"

void
CHtvDesign::GenSandbox()
{
	CSbxOpt xstOpt;
	vector<SbxOptMap_ValuePair> makeOpt;
	vector<SbxOptMap_ValuePair> prjOpt;
	vector<SbxOptMap_ValuePair> ucfOpt;
	CSbxClks sbxClks;

	// find first module
	CHtvIdentTblIter topHierIter(GetTopHier()->GetIdentTbl());
	for (topHierIter.Begin(); !topHierIter.End(); topHierIter++)
		if (topHierIter->IsModule() && topHierIter->IsInputFile())
			break;

	if (topHierIter.End()) {
		printf("GenSandbox: A module was not found\n");
		return;
	}

	CHtvIdent *pModule = topHierIter->GetThis();

	// first parse sandbox configuration file
	string sbxCfg = g_htvArgs.GetInputPathName();
	int periodPos = sbxCfg.find_last_of(".");
	sbxCfg.erase(periodPos);
	sbxCfg += ".sbx";

	FILE *sbxFp;
	if (!(sbxFp = fopen(sbxCfg.c_str(), "r"))) {
		// sandbox configuration file does not exist, write default configuration file

		GenSandboxInitialConfigFile(sbxCfg, pModule);

		// does not return

	} else
		printf("Reading sandbox configuration file...\n");

	// read sandbox configuration information
	string sbxDir;
	char line[256];
	while (fgets(line, 256, sbxFp)) {
		if (strncmp(line, "//", 2) == 0)
			continue;

		// parse line
		string cfgType, cfgName, cfgValue;
		int strCnt = ParseSbxConfigLine(line, cfgType, cfgName, cfgValue);
		if (strCnt == 0)
			continue;

		if (cfgType == "SBX") {
			if (cfgName == "dir")
				sbxDir = cfgValue;
			else if (cfgName == "clock") {
				char clkName[128];
				char clkUnits[128];
				float clkFreqInMhz;
				if (sscanf(cfgValue.c_str(), "%s%f%s", clkName, &clkFreqInMhz, clkUnits) != 3
					|| (stricmp(clkUnits, "mhz") != 0 && stricmp(clkUnits, "ns") != 0)) {
					printf("Sandbox configuation sytax error: %s", line);
					printf("  Expected SBX clock clkName clkRate clkUnits\n");
					printf("    Where clkUnits can be mhz or ns\n");
					ErrorExit();
				}
				if (strcmp(clkUnits, "ns") == 0)
					clkFreqInMhz = 1000.0f / clkFreqInMhz;
				sbxClks.AddClkInKhz(string(clkName), (int)((clkFreqInMhz*1000)+0.5f));
			} else if (cfgName == "reset") {
				sbxClks.SetResetName(cfgValue);
			} else {
				printf("Unknown sandbox configuration type/name/value: %s", line);
				ErrorExit();
			}
		} else if (cfgType == "XST")
			xstOpt.Insert(cfgName, cfgValue);
		else if (cfgType == "MAKE") {
			if (cfgValue.c_str()[0] != '=')
				cfgValue = "= " + cfgValue;
			makeOpt.push_back(SbxOptMap_ValuePair(cfgName, cfgValue));
		} else if (cfgType == "PRJ")
			prjOpt.push_back(SbxOptMap_ValuePair(cfgName, cfgValue));
		else if (cfgType == "UCF")
			ucfOpt.push_back(SbxOptMap_ValuePair(cfgName, cfgValue));
		else {
			printf("Unknown sandbox configuration type: %s", line);
			ErrorExit();
		}
	}

#ifdef _WIN32
	int err = mkdir(sbxDir.c_str());
#else
	int err = mkdir(sbxDir.c_str(), 0755);
#endif
	if (err != 0) {
		int errnum;
#ifdef _WIN32
		_get_errno(&errnum);
#else
		errnum = errno;
#endif
		if (errnum != EEXIST) {
			printf("Unable to create sandbox directory: %s", sbxDir.c_str());
			ErrorExit();
		}
	}

	string dsnName = pModule->GetName();

	// Generate sandbox files
	GenSandboxMakefile(sbxDir, makeOpt);
	GenSandboxPrjFile(sbxDir, prjOpt, dsnName);
	GenSandboxXstFile(sbxDir, xstOpt);
	GenSandboxUcfFile(sbxDir, sbxClks, ucfOpt);
	GenSandboxFpgaVFile(sbxDir, sbxClks, pModule);
	GenSandboxModuleVFile(sbxDir, sbxClks, pModule);
}

// needed for sorting clocks
bool clkCmpFunc ( CSbxClks::CSbxClk clk1, CSbxClks::CSbxClk clk2 )
{
   return clk1.m_freqInKhz < clk2.m_freqInKhz;
}

void CSbxClks::Initialize()
{
	m_bInit = true;

	if (m_sbxClks.size() == 0) {
		printf("All clocks must be specified in design.sbx file (none found)\n");
		printf("  Syntax is: SBX clock  clkName clkFreq clkUnits\n");
		printf("    Where clkUnits is either mhz or ns\n");
		ErrorExit();
	}

	if (m_sbxClks.size() > 7) {
		printf("Maximum of 7 clocks can be specified in .sbx file\n");
		ErrorExit();
	}

	// Given the list of clocks and frequencies, generate sandbox parameters

	// first sort clocks from lowest to highest frequency
	sort(m_sbxClks.begin(), m_sbxClks.end(), clkCmpFunc);

	// find largest common denominator
	int lcd[7];
	int rem[7];
	for (size_t i = 0; i < m_sbxClks.size(); i += 1) {
		lcd[i] = 1;
		rem[i] = m_sbxClks[i].m_freqInKhz;
	}

	size_t onesCnt = 0;
	int clkLcd = 1;
	for (int div = 2; div < 32 && onesCnt < m_sbxClks.size(); div += 1) {
		for (bool bFoundOne = true; bFoundOne; ) {
			bFoundOne = false;
			for (size_t i = 0; i < m_sbxClks.size(); i += 1) {
				if (rem[i] % div == 0) {
					lcd[i] *= div;
					rem[i] /= div;
					bFoundOne = true;

					if (rem[i] == 1)
						onesCnt += 1;
				}
			}
			if (bFoundOne)
				clkLcd *= div;
		}
	}

	if (onesCnt < m_sbxClks.size()) {
		printf("    A least common denomimator was not found for .sbx clocks\n");
		printf("    Clocks are rounded to the nearest Khz for LCD calculation.\n");
		ErrorExit();
	}

	if (clkLcd > 1200000) {
		printf("    A common reference clock of %5.3f Mhz was found,\n", clkLcd / 1000.0);
		printf("    which is greater than the maximum value of 1200.0 Mhz (PLL limit)\n");
		ErrorExit();
	}

	while (clkLcd < 600000)
		clkLcd *= 2;

	m_vcoMul = 8;

	// find Fvco multiplier (Fvco must be between 600 and 1200
	m_refFreqInKhz = clkLcd / m_vcoMul;
}

struct CClkInfo {
	CClkInfo(const string &name) : m_name(name) {}
	string	m_name;
	int		m_freq;
};

void CHtvDesign::GenSandboxInitialConfigFile(const string &sbxCfg, CHtvIdent *pModule)
{
	printf("  Writing initial sandbox configuration file...\n");

	FILE *sbxFp;
	if (!(sbxFp = fopen(sbxCfg.c_str(), "w"))) {
		printf("Unable to open sandbox configuration file '%s' for writing", sbxCfg.c_str());
		ErrorExit();
	}

	string inputBaseName = g_htvArgs.GetInputFileName();

	string			reset;
	vector<CClkInfo>	clks;

	// find clock and reset signals from design
	CHtfeIdentTblIter portIter(pModule->GetIdentTbl());
	for (portIter.Begin(); !portIter.End(); portIter++) {
		if (!portIter->IsInPort())
			continue;

		if (strncmp(portIter->GetName().c_str(), "i_clk", 5) == 0 || strncmp(portIter->GetName().c_str(), "clk", 3) == 0 ||
			strncmp(portIter->GetName().c_str(), "i_clock", 7) == 0 || strncmp(portIter->GetName().c_str(), "clock", 5) == 0)
				clks.push_back(CClkInfo(portIter->GetName()));

		if (strncmp(portIter->GetName().c_str(), "i_rst", 5) == 0 || strncmp(portIter->GetName().c_str(), "rst", 3) == 0 ||
			strncmp(portIter->GetName().c_str(), "i_reset", 7) == 0 || strncmp(portIter->GetName().c_str(), "reset", 5) == 0)
				reset = portIter->GetName();
	}

	// find highest clock multiplier (i.e. 4x)
	int highestMultiplier = 0;
	for (size_t i = 0; i < clks.size(); i += 1) {
		size_t len = clks[i].m_name.size();
		if (clks[i].m_name[len-1] == 'x' && isdigit(clks[i].m_name[len-2])) {
			int multiplier = clks[i].m_name[len-2] - '0';
			highestMultiplier = max(highestMultiplier, multiplier);
		}
	}

	// assign frequencies to clocks
	if (highestMultiplier > 0) {
		for (size_t i = 0; i < clks.size(); i += 1) {
			size_t len = clks[i].m_name.size();
			if (clks[i].m_name[len-1] == 'x' && isdigit(clks[i].m_name[len-2])) {
				int multiplier = clks[i].m_name[len-2] - '0';
				switch (highestMultiplier) {
					case 1: clks[i].m_freq = multiplier * 150; break;
					case 2: clks[i].m_freq = multiplier * 150; break;
					case 4: clks[i].m_freq = multiplier * 100; break;
					default: clks[i].m_freq = multiplier * 150; break;
				}
			}
		}
	} else {
		for (size_t i = 0; i < clks.size(); i += 1)
			clks[i].m_freq = 150;
	}

	string sbxDir = "sandbox_" + inputBaseName;

	fprintf(sbxFp, "// %s sandbox configuration file\n//\n", inputBaseName.c_str());
	fprintf(sbxFp, "SBX\tdir %s\n", sbxDir.c_str());
	fprintf(sbxFp, "//SBX 	reset           rstName\n");
	fprintf(sbxFp, "//SBX 	clock           clkName clkFreq clkUnits\n");
	if (reset.size() > 0)
		fprintf(sbxFp, "SBX		reset			%s\n", reset.c_str());
	for (size_t i = 0; i < clks.size(); i += 1)
		fprintf(sbxFp, "SBX		clock			%s %d Mhz\n", clks[i].m_name.c_str(), clks[i].m_freq); 
	fprintf(sbxFp, "\n");
	fprintf(sbxFp, "MAKE	XILINX_VERS     = 13.4\n");
	fprintf(sbxFp, "MAKE	XILINX          = /hw/tools/xilinx/$(XILINX_VERS)/ISE\n");
	fprintf(sbxFp, "MAKE	XILINX_BIN      = $(XILINX)/bin/lin64\n");
	fprintf(sbxFp, "MAKE	XILINX_PA_BIN   = $(XILINX)/../PlanAhead/bin\n");
	fprintf(sbxFp, "MAKE	PART			= xc6vlx130t\n");
	fprintf(sbxFp, "MAKE	SPEED_GRADE		= 1\n");
	fprintf(sbxFp, "MAKE	PACKAGE			= ff1156\n");
	fprintf(sbxFp, "MAKE	XST_FLAGS		= \n");
	fprintf(sbxFp, "MAKE	NGCBUILD_FLAGS	= -sd ../common/xilinx/12.1/core_v6\n");
	fprintf(sbxFp, "MAKE	MAP_FLAGS		= -mt 2 -pr b -detail -ignore_keep_hierarchy -logic_opt on\n");
	fprintf(sbxFp, "MAKE	PAR_FLAGS		= -ol std -mt 2\n");
	fprintf(sbxFp, "MAKE	TRCE_FLAGS		= -v 2048 -u 32\n");
	fprintf(sbxFp, "MAKE	MPPR_SEEDS		= 1 3 5 7 11 13 17 19 23 29 37 41 43 47 53 59 61 67 71 73 79 83 89 97\n");
	fprintf(sbxFp, "\n");
	fprintf(sbxFp, "PRJ		verilog work \"sandbox_fpga.v\"\n");
	fprintf(sbxFp, "PRJ		verilog work \"sandbox_%s.v\"\n", inputBaseName.c_str());
	if (g_htvArgs.GetVFileName().c_str()[0] == '/')
		fprintf(sbxFp, "PRJ		verilog work \"%s\"\n", g_htvArgs.GetVFileName().c_str());
	else
		fprintf(sbxFp, "PRJ		verilog work \"../%s\"\n", g_htvArgs.GetVFileName().c_str());
	fprintf(sbxFp, "\n");
	fprintf(sbxFp, "//XST		-lc area\n");
	fprintf(sbxFp, "//XST		-shreg_extract yes\n");
	fprintf(sbxFp, "//XST		-shreg_min_size 3\n");
	fprintf(sbxFp, "\n");
	fprintf(sbxFp, "UCF		NET \"i_clk_ref_p\" TNM_NET = \"TN_CLK\";\n");
	fprintf(sbxFp, "UCF		INST \"sb\" AREA_GROUP = \"AG_sandbox\" ;\n");
	fprintf(sbxFp, "UCF		AREA_GROUP \"AG_sandbox\" GROUP = CLOSED ;\n");
	fprintf(sbxFp, "UCF		AREA_GROUP \"AG_sandbox\" PLACE = CLOSED ;\n");
	fprintf(sbxFp, "UCF		AREA_GROUP \"AG_sandbox\" RANGE = SLICE_X2Y80:SLICE_X41Y159 ;\n");
	fprintf(sbxFp, "\n");
	fclose(sbxFp);

	printf("  Initial design.sbx file has been written\n");
	printf("    User should review sandbox defaults in sbx file and run Htv again to create sandbox files\n");

	ErrorExit();
}

int CHtvDesign::ParseSbxConfigLine(char *pLine, string &cfgType, string &cfgName, string &cfgValue)
{
	// cfgType is first string
	while (isspace(*pLine)) pLine += 1;
	char *pEnd = pLine;
	while (!isspace(*pEnd) && *pEnd != '\0') pEnd += 1;
	char endCh = *pEnd;
	*pEnd = '\0';

	cfgType = pLine;
	*pEnd = endCh;
	pLine = pEnd;

	// cfgName is next string
	while (isspace(*pLine)) pLine += 1;
	pEnd = pLine;
	while (!isspace(*pEnd) && *pEnd != '\0') pEnd += 1;
	endCh = *pEnd;
	*pEnd = '\0';

	cfgName = pLine;
	*pEnd = endCh;
	pLine = pEnd;

	// cfgValue is last string
	while (isspace(*pLine)) pLine += 1;
	int len = strlen(pLine);
	if (len > 0) {
		pEnd = pLine+len-2;
		while (isspace(*pEnd) && pEnd > pLine) pEnd -= 1;
		pEnd[1] = '\0';
	}

	cfgValue = pLine;

	return (cfgType.size() > 0 ? 1 : 0) + (cfgName.size() > 0 ? 1 : 0)+ (cfgValue.size() > 0 ? 1 : 0);
}

void CHtvDesign::GenSandboxMakefile(string &sbxDir, vector<SbxOptMap_ValuePair> &makeOpt)
{
	printf("  Writing sandbox Makefile...\n");

	string makeFile = sbxDir + "/Makefile";

	FILE *mkFp;
	if (!(mkFp = fopen(makeFile.c_str(), "w"))) {
		printf("Unable to open '%s' for writing", makeFile.c_str());
		ErrorExit();
	}

	for (size_t i = 0; i < makeOpt.size(); i += 1)
		fprintf(mkFp, "%s %s\n", makeOpt[i].first.c_str(), makeOpt[i].second.c_str());

	fprintf(mkFp, "\n");
	fprintf(mkFp, "DESIGN = sandbox_fpga\n");
	fprintf(mkFp, "\n");
	fprintf(mkFp, "NICE\t\t\t= /bin/nice\n");
	fprintf(mkFp, "\n");
	fprintf(mkFp, "# generate timing report\n");
	fprintf(mkFp, "timing: $(DESIGN)_routed.twr\n");
	fprintf(mkFp, "\n");
	fprintf(mkFp, "$(DESIGN)_routed.twr : $(DESIGN)_routed.ncd $(DESIGN).pcf\n");
	fprintf(mkFp, "\t$(NICE) $(XILINX_BIN)/trce $(TRCE_FLAGS) -intstyle xflow \\\n");
	fprintf(mkFp, "\t\t$(DESIGN)_routed.ncd $(DESIGN).pcf\n");
	fprintf(mkFp, "\n");
	fprintf(mkFp, "# run place and route\n");
	fprintf(mkFp, "route : $(DESIGN)_routed.ncd\n");
	fprintf(mkFp, "\n");
	fprintf(mkFp, "$(DESIGN)_routed.ncd: $(DESIGN).ncd $(DESIGN).pcf\n");
	fprintf(mkFp, "\t$(NICE) $(XILINX_BIN)/par $(PAR_FLAGS) -intstyle xflow \\\n");
	fprintf(mkFp, "\t\t-w $(DESIGN).ncd $(DESIGN)_routed.ncd $(DESIGN).pcf\n");
	fprintf(mkFp, "\n");
	fprintf(mkFp, "# run map\n");
	fprintf(mkFp, "map : $(DESIGN).ncd\n");
	fprintf(mkFp, "\n");
	fprintf(mkFp, "$(DESIGN).ncd: $(DESIGN).ngd\n");
	fprintf(mkFp, "\t$(NICE) $(XILINX_BIN)/map $(MAP_FLAGS) -intstyle xflow -o $(DESIGN).ncd $(DESIGN).ngd\n");
	fprintf(mkFp, "\n");
	fprintf(mkFp, "# run ngdbuild\n");
	fprintf(mkFp, "ngdbuild: $(DESIGN).ngd\n");
	fprintf(mkFp, "\n");
	fprintf(mkFp, "$(DESIGN).ngd: $(DESIGN).ngc\n");
	fprintf(mkFp, "\t$(NICE) $(XILINX_BIN)/ngdbuild $(NGCBUILD_FLAGS) \\\n");
	fprintf(mkFp, "\t\t-intstyle xflow -dd _ngo -uc $(DESIGN).ucf $(DESIGN) $(DESIGN).ngd\n");
	fprintf(mkFp, "\n");
	fprintf(mkFp, "# run xst\n");
	fprintf(mkFp, "xst: $(DESIGN).ngc\n");
	fprintf(mkFp, "\n");
	fprintf(mkFp, "$(DESIGN).ngc: $(DESIGN).v $(DESIGN).xst $(DESIGN).prj\n");
	fprintf(mkFp, "\t@ if [ ! -d xst ]; then mkdir xst; mkdir xst/projnav.tmp; fi\n");
	fprintf(mkFp, "\t$(NICE) $(XILINX_BIN)/xst $(XST_FLAGS) -ifn $(DESIGN).xst -intstyle xflow\n");
	fprintf(mkFp, "\n");

	fprintf(mkFp, "mppr : $(DESIGN).ngd\n");
	fprintf(mkFp, "\t@ if [ ! -d results.dir ]; then mkdir results.dir; fi\n");
	fprintf(mkFp, "\t$ (for seed in $(MPPR_SEEDS); do \\\n");

	fprintf(mkFp, "\t$(NICE) $(XILINX_BIN)/map $(MAP_FLAGS) -intstyle xflow -t $$seed -o results_$$seed.ncd $(DESIGN).ngd ;\\\n");

	fprintf(mkFp, "\t$(NICE) $(XILINX_BIN)/par $(PAR_FLAGS) -intstyle xflow -w results_$$seed.ncd \\\n");
	fprintf(mkFp, "\t\t\tresults_$${seed}_routed.ncd\\\n\t\t\tresults_$${seed}.pcf ; \\\n");

	fprintf(mkFp, "\t$(NICE) $(XILINX_BIN)/trce $(TRCE_FLAGS) \\\n");
	fprintf(mkFp, "\t\t\tresults_$${seed}_routed.ncd results_$${seed}.pcf ; \\\n");
	fprintf(mkFp, "\tsync; mv results_$${seed}_* results.dir; mv results_$${seed}.* results.dir; done)\n");
	fprintf(mkFp, "\n");

	fprintf(mkFp, "# bring up the planAhead\n");
	fprintf(mkFp, ".PHONY: planin.jou\n");
	fprintf(mkFp, "COREGEN_DIRS += .\n");
	fprintf(mkFp, "planin.jou:\n");
	fprintf(mkFp, "\t@echo -n \"create_project planin ./planin\" > $@\n");
	fprintf(mkFp, "\t@echo \" -part $(PART)$(PACKAGE)-$(SPEED_GRADE)\" >> $@\n");
	fprintf(mkFp, "\t@echo \"set_property design_mode GateLvl [current_fileset]\" >> $@\n");
	fprintf(mkFp, "\t@echo \"set_property edif_top_file $(DESIGN).ngc [current_fileset]\" >> $@\n");
	fprintf(mkFp, "\t@echo -n \"set_property EDIF_EXTRA_SEARCH_PATHS\" >> $@\n");
	fprintf(mkFp, "\t@echo \" {$(COREGEN_DIRS)} [current_fileset]\" >> $@\n");
	fprintf(mkFp, "\t@echo \"if [file exists $(DESIGN).ucf] {\" >> $@\n");
	fprintf(mkFp, "\t@echo \"read_ucf $(DESIGN).ucf\" >> $@\n");
	fprintf(mkFp, "\t@echo \"}\" >> $@\n");
	fprintf(mkFp, "\t@echo \"if [file exists $(DESIGN)_routed.ncd] {\" >> $@\n");
	fprintf(mkFp, "\t@echo \"set cny_twx \\\"\\\"\" >> $@\n");
	fprintf(mkFp, "\t@echo \"if [file exists $(DESIGN)_routed.twx] {\" >> $@\n");
	fprintf(mkFp, "\t@echo \"set cny_twx \\\"-twx $(DESIGN)_routed.twx\\\"\" >> $@\n");
	fprintf(mkFp, "\t@echo \"}\" >> $@\n");
	fprintf(mkFp, "\t@echo -n \"eval import_as_run -run impl_1 \\$$cny_twx\" >> $@\n");
	fprintf(mkFp, "\t@echo \" $(DESIGN)_routed.ncd\" >> $@\n");
	fprintf(mkFp, "\t@echo \"open_impl_design -quiet\" >> $@\n");
	fprintf(mkFp, "\t@echo \"} elseif [file exists $(DESIGN).ncd] {\" >> $@\n");
	fprintf(mkFp, "\t@echo \"eval import_as_run -run impl_1 $(DESIGN).ncd\" >> $@\n");
	fprintf(mkFp, "\t@echo \"open_impl_design -quiet\" >> $@\n");
	fprintf(mkFp, "\t@echo \"} else {\" >> $@\n");
	fprintf(mkFp, "\t@echo \"link_design -name netlist_1\" >> $@\n");
	fprintf(mkFp, "\t@echo \"}\" >> $@\n");
	fprintf(mkFp, "\n");
	fprintf(mkFp, "netlist: $(DESIGN)_routed.v\n");
	fprintf(mkFp, "$(DESIGN)_routed.v: $(DESIGN)_routed.ncd\n");
	fprintf(mkFp, "\trm -rf $(DESIGN)_routed.v\n");
	fprintf(mkFp, "\t$(NICE) $(XILINX_BIN)/netgen -ofmt verilog -sim $(DESIGN)_routed.ncd\n");
	fprintf(mkFp, "\n");
	fprintf(mkFp, ".PHONY: planin\n");
	fprintf(mkFp, "planin:\n");
	fprintf(mkFp, "\t@rm -rf $@; mkdir $@\n");
	fprintf(mkFp, "\t@make planin.jou\n");
	fprintf(mkFp, "\t$(XILINX_PA_BIN)/planAhead -source planin.jou &\n");
	fprintf(mkFp, "\n");
	fprintf(mkFp, "clean:\n");
	fprintf(mkFp, "\t@ rm -rf _xmsgs xst _ngo xlnx_auto_0_xdb\n");
	fprintf(mkFp, "\t@ rm -f $(DESIGN).ngc $(DESIGN).ngr $(DESIGN).lso $(DESIGN).srp\n");
	fprintf(mkFp, "\t@ rm -f $(DESIGN).bld $(DESIGN).map $(DESIGN)_map.rpt $(DESIGN).mrp\n");
	fprintf(mkFp, "\t@ rm -f $(DESIGN).ncd $(DESIGN).ngd $(DESIGN)_ngdbuild.xrpt\n");
	fprintf(mkFp, "\t@ rm -f $(DESIGN).ngm $(DESIGN)_par.xrpt $(DESIGN).pcf\n");
	fprintf(mkFp, "\t@ rm -f $(DESIGN)_routed* $(DESIGN)_map.xrpt\n");
	fprintf(mkFp, "\t@ rm -f $(DESIGN)_summary.xml $(DESIGN)_usage.xml $(DESIGN)_xst.xrpt\n");

	fclose(mkFp);
}

void CHtvDesign::GenSandboxPrjFile(string &sbxDir, vector<SbxOptMap_ValuePair> &prjOpt, string &dsnName)
{
	printf("  Writing sandbox prj file...\n");

	string prjFile = sbxDir + "/sandbox_fpga.prj";

	FILE *prjFp;
	if (!(prjFp = fopen(prjFile.c_str(), "w"))) {
		printf("Unable to open '%s' for writing", prjFile.c_str());
		ErrorExit();
	}

	for (size_t i = 0; i < prjOpt.size(); i += 1)
		fprintf(prjFp, "%s %s\n", prjOpt[i].first.c_str(), prjOpt[i].second.c_str());

	fclose(prjFp);
}

struct CSbxOptList {
	const char *m_pName;
	const char *m_pValue;
};

CSbxOptList xstOptList[] = {
	{ "set -tmpdir", "\"./xst/projnav.tmp\"" },
	{ "set -xsthdpdir", "\"./xst\"" },
	{ "run", "" },
	{ "-ifn", "sandbox_fpga.prj" },
	{ "-ifmt", "mixed" },
	{ "-ofn", "sandbox_fpga" },
	{ "-ofmt", "NGC" },
	{ "-p", "xc6vlx130t-1-ff1156" },
	{ "-top", "sandbox_fpga" },
	{ "-opt_mode", "Speed" },
	{ "-opt_level", "2" },
	{ "-power", "NO" },
	{ "-iuc", "NO" },
	{ "-keep_hierarchy", "NO" },
	{ "-netlist_hierarchy", "As_Optimized" },
	{ "-rtlview", "Yes" },
	{ "-glob_opt", "AllClockNets" },
	{ "-read_cores", "YES" },
	{ "-write_timing_constraints", "NO" },
	{ "-cross_clock_analysis", "NO" },
	{ "-hierarchy_separator", "/" },
	{ "-bus_delimiter", "[]" },
	{ "-case", "Maintain" },
	{ "-slice_utilization_ratio", "100" },
	{ "-bram_utilization_ratio", "100" },
	{ "-dsp_utilization_ratio", "100" },
	{ "-lc", "OFF" },
	{ "-reduce_control_sets", "OFF" },
	{ "-fsm_extract", "YES" },
	{ "-fsm_encoding", "Auto" },
	{ "-safe_implementation", "No" },
	{ "-fsm_style", "LUT" },
	{ "-ram_extract", "Yes" },
	{ "-ram_style", "Auto" },
	{ "-rom_extract", "Yes" },
	{ "-shreg_extract", "YES" },
	{ "-rom_style", "Auto" },
	{ "-auto_bram_packing", "NO" },
	{ "-resource_sharing", "YES" },
	{ "-async_to_sync", "NO" },
	{ "-shreg_min_size", "3" },
	{ "-use_dsp48", "Auto" },
	{ "-iobuf", "YES" },
	{ "-max_fanout", "100000" },
	{ "-bufg", "0" },
	{ "-register_duplication", "YES" },
	{ "-register_balancing", "No" },
	{ "-optimize_primitives", "NO" },
	{ "-use_clock_enable", "Auto" },
	{ "-use_sync_set", "Auto" },
	{ "-use_sync_reset", "Auto" },
	{ "-iob", "Auto" },
	{ "-equivalent_register_removal", "YES" },
	{ "-slice_utilization_ratio_maxmargin", "5" },
	{ 0 }
};

void CHtvDesign::GenSandboxXstFile(string &sbxDir, CSbxOpt &xstOpt)
{
	printf("  Writing sandbox xst file...\n");

	string xstFile = sbxDir + "/sandbox_fpga.xst";

	FILE *xstFp;
	if (!(xstFp = fopen(xstFile.c_str(), "w"))) {
		printf("Unable to open '%s' for writing", xstFile.c_str());
		ErrorExit();
	}

	for (int i = 0; xstOptList[i].m_pName; i += 1)
		xstOpt.Insert(xstOptList[i].m_pName, xstOptList[i].m_pValue);

	CSbxOptIter optIter(xstOpt);
	for (optIter.Begin(); !optIter.End(); optIter++) {
		if (optIter->substr(0,3) == "set")
			fprintf(xstFp, "%s\n", optIter->c_str());
	}

	for (optIter.Begin(); !optIter.End(); optIter++) {
		if (optIter->substr(0,3) == "run")
			fprintf(xstFp, "%s\n", optIter->c_str());
	}

	for (optIter.Begin(); !optIter.End(); optIter++) {
		if (optIter->substr(0,3) != "set" && optIter->substr(0,3) != "run")
			fprintf(xstFp, "%s\n", optIter->c_str());
	}

	fclose(xstFp);
}

void CHtvDesign::GenSandboxUcfFile(string &sbxDir, CSbxClks &sbxClks, vector<SbxOptMap_ValuePair> &ucfOpt)
{
	printf("  Writing sandbox ucf file...\n");

	string ucfFile = sbxDir + "/sandbox_fpga.ucf";

	FILE *ucfFp;
	if (!(ucfFp = fopen(ucfFile.c_str(), "w"))) {
		printf("Unable to open '%s' for writing", ucfFile.c_str());
		ErrorExit();
	}

	bool bTimeSpec = false;
	for (size_t i = 0; i < ucfOpt.size(); i += 1) {
		fprintf(ucfFp, "%s %s\n", ucfOpt[i].first.c_str(), ucfOpt[i].second.c_str());
		bTimeSpec |= stricmp(ucfOpt[i].first.c_str(), "TIMESPEC") == 0;
	}

	if (!bTimeSpec)
		fprintf(ucfFp, "TIMESPEC \"TS_CLK\" = PERIOD \"TN_CLK\" %f ns HIGH 50 %%;\n", sbxClks.GetRefPeriodInNs());

	fclose(ucfFp);
}

void CHtvDesign::GenSandboxFpgaVFile(string &sbxDir, CSbxClks &sbxClks, CHtvIdent *pModule)
{
	printf("  Writing sandbox_fpga.v file...\n");

	string vFile = sbxDir + "/sandbox_fpga.v";

	FILE *vFp;
	if (!(vFp = fopen(vFile.c_str(), "w"))) {
		printf("Unable to open '%s' for writing", vFile.c_str());
		ErrorExit();
	}

	// Find reset, clocks and count input signals
	int insTotalWidth = 0;

	CHtfeIdentTblIter portIter(pModule->GetIdentTbl());
	for (portIter.Begin(); !portIter.End(); portIter++) {
		if (!portIter->IsInPort())
			continue;

		int clkIdx;
		for (clkIdx = 0; clkIdx < sbxClks.GetClkCnt(); clkIdx += 1) {
			if (portIter->GetName() == sbxClks.GetClkName(clkIdx) ||
				portIter->GetName().substr(0,2) == "i_" && portIter->GetName().substr(2) == sbxClks.GetClkName(clkIdx))
				break;
		}

		if (clkIdx < sbxClks.GetClkCnt() || portIter->GetName() == sbxClks.GetResetName() ||
			portIter->GetName().substr(0,2) == "i_" && portIter->GetName().substr(2) == sbxClks.GetResetName())
			continue;

		int width = portIter->GetType()->GetWidth();

		vector<int> &dimenList = portIter->GetDimenList();
		for (size_t i = 0; i < dimenList.size(); i += 1)
			width *= dimenList[i];

		insTotalWidth += width;
	}

	int outsTotalWidth = 0;
	for (portIter.Begin(); !portIter.End(); portIter++) {
		if (!portIter->IsOutPort())
			continue;

		int width = portIter->GetType()->GetWidth();

		vector<int> &dimenList = portIter->GetDimenList();
		for (size_t i = 0; i < dimenList.size(); i += 1)
			width *= dimenList[i];

		outsTotalWidth += width;
	}

	fprintf(vFp, "`timescale 1 ns / 1 ps\n");
	fprintf(vFp, "\n");
	fprintf(vFp, "module sandbox_fpga (/*AUTOARG*/\n");
	fprintf(vFp, "  // Outputs\n");
	fprintf(vFp, "    outs,\n");
	fprintf(vFp, "  // Inputs\n");
	fprintf(vFp, "    i_clk_ref_p, i_clk_ref_n,\n");
	if (sbxClks.HasReset())
		fprintf(vFp, "    i_reset, dcm_reset_f,\n");
	fprintf(vFp, "    ins\n");
	fprintf(vFp, "  );\n");
	fprintf(vFp, "input         i_clk_ref_p, i_clk_ref_n;\n");
	if (sbxClks.HasReset()) {
		fprintf(vFp, "input         i_reset;\n");
		fprintf(vFp, "input         dcm_reset_f;\n");
	}
	fprintf(vFp, "input  [63:0] ins;\n");
	fprintf(vFp, "output [63:0] outs;\n");
	fprintf(vFp, "\n");
	fprintf(vFp, "  //\n");
	fprintf(vFp, "  // Clocks & Reset\n");
	fprintf(vFp, "  //\n");
	fprintf(vFp, "  IBUFGDS clk_ref_ibufg (.O(clk_ref), .I(i_clk_ref_p), .IB(i_clk_ref_n));\n");
	fprintf(vFp, "  BUFG pll_bufg	(.O(pll_clkfbin), .I(pll_clkfbout));\n");
	for (int i = 0; i < sbxClks.GetClkCnt(); i += 1)
		fprintf(vFp, "  BUFG %s_bufg	(.O(%s),	.I(pll_%s));\n",
			sbxClks.GetClkName(i).c_str(), sbxClks.GetClkName(i).c_str(), sbxClks.GetClkName(i).c_str());
	fprintf(vFp, "  PLL_ADV #(\n");
	fprintf(vFp, "    .CLKIN1_PERIOD(%f),	//  %5.3f MHz\n", sbxClks.GetRefPeriodInNs(), sbxClks.GetRefFreqInKhz() / 1000.0);
	fprintf(vFp, "    .CLKIN2_PERIOD(10.0),\n");
	fprintf(vFp, "    .DIVCLK_DIVIDE(1),\n");
	fprintf(vFp, "    .CLKFBOUT_MULT(%d),	//  %5.3f MHz (Fvco)\n", sbxClks.GetVcoMul(), sbxClks.GetVcoFreqInKhz() / 1000.0);
	for (int i = 0; i < sbxClks.GetClkCnt(); i += 1)
		fprintf(vFp, "    .CLKOUT%d_DIVIDE(%d)%s	//  %5.3f MHz\n",
		i, sbxClks.GetClkDiv(i), i == sbxClks.GetClkCnt()-1 ? "" : ",", sbxClks.GetClkFreqInKhz(i) / 1000.0);
	fprintf(vFp, "  ) pll (\n");
	fprintf(vFp, "    .CLKFBIN(pll_clkfbin),\n");
	fprintf(vFp, "    .CLKINSEL(1'b1),\n");
	fprintf(vFp, "    .CLKIN1(clk_ref),\n");
	fprintf(vFp, "    .CLKIN2(1'b0), .DADDR(5'b0), .DCLK(1'b0), .DEN(1'b0),\n");
	fprintf(vFp, "    .DI(16'b0), .DWE(1'b0), .REL(1'b0),\n");
	fprintf(vFp, "    .RST(!dcm_reset_f),\n");
	fprintf(vFp, "    .CLKFBDCM(),\n");
	fprintf(vFp, "    .CLKFBOUT(pll_clkfbout),\n");
	fprintf(vFp, "    .CLKOUTDCM0(), .CLKOUTDCM1(), .CLKOUTDCM2(),\n");
	fprintf(vFp, "    .CLKOUTDCM3(), .CLKOUTDCM4(), .CLKOUTDCM5(),\n");
	for (int i = 0; i < sbxClks.GetClkCnt(); i += 1)
		fprintf(vFp, "    .CLKOUT%d(pll_%s),\n", i, sbxClks.GetClkName(i).c_str());
	fprintf(vFp, "    .DO(), .DRDY(), .LOCKED()\n");
	fprintf(vFp, "  );\n");
	fprintf(vFp, "\n");
	if (sbxClks.HasReset()) {
		fprintf(vFp, "  // Reset\n");
		fprintf(vFp, "  (* keep = \"true\" *) reg r_reset_0, r_reset_bufg;\n");
		fprintf(vFp, "  always @(posedge %s) begin\n", sbxClks.GetSlowClkName().c_str());
		fprintf(vFp, "    r_reset_0 <= i_reset;\n");
		fprintf(vFp, "    r_reset_bufg <= r_reset_0;\n");
		fprintf(vFp, "  end\n");
		fprintf(vFp, "  BUFG bufg_reset(.O(reset), .I(r_reset_bufg));\n");
		fprintf(vFp, "\n");
	}
	fprintf(vFp, "  //\n");
	fprintf(vFp, "  // pipe stages into and out of the sandbox\n");
	fprintf(vFp, "  //\n");
	fprintf(vFp, "  (* equivalent_register_removal = \"no\" *)\n");
	fprintf(vFp, "  (* shreg_extract = \"no\" *)\n");
	fprintf(vFp, "  (* keep = \"true\" *)\n");
	fprintf(vFp, "  reg [%d:0]		r_in_0;\n", insTotalWidth > 63 ? 63 : insTotalWidth-1);
	fprintf(vFp, "  //\n");
	fprintf(vFp, "  (* equivalent_register_removal = \"no\" *)\n");
	fprintf(vFp, "  (* shreg_extract = \"no\" *)\n");
	fprintf(vFp, "  (* keep = \"true\" *)\n");
	fprintf(vFp, "  reg [%d:0]		r_in_1;\n", insTotalWidth > 255 ? 255 : insTotalWidth-1);
	fprintf(vFp, "  //\n");
	fprintf(vFp, "  (* equivalent_register_removal = \"no\" *)\n");
	fprintf(vFp, "  (* shreg_extract = \"no\" *)\n");
	fprintf(vFp, "  (* keep = \"true\" *)\n");
	fprintf(vFp, "  reg [%d:0]		r_in_2;\n", insTotalWidth > 1023 ? 1023 : insTotalWidth-1);
	fprintf(vFp, "  //\n");
	fprintf(vFp, "  (* equivalent_register_removal = \"no\" *)\n");
	fprintf(vFp, "  (* shreg_extract = \"no\" *)\n");
	fprintf(vFp, "  (* keep = \"true\" *)\n");
	fprintf(vFp, "  reg [%d:0]		r_in_3;\n", insTotalWidth > 4095 ? 4095 : insTotalWidth-1);
	fprintf(vFp, "  //\n");
	fprintf(vFp, "  (* equivalent_register_removal = \"no\" *)\n");
	fprintf(vFp, "  (* shreg_extract = \"no\" *)\n");
	fprintf(vFp, "  (* keep = \"true\" *)\n");
	fprintf(vFp, "  reg [%d:0]		r_in_4;\n", insTotalWidth-1);
	fprintf(vFp, "  //\n");
	fprintf(vFp, "  (* equivalent_register_removal = \"no\" *)\n");
	fprintf(vFp, "  (* shreg_extract = \"no\" *)\n");
	fprintf(vFp, "  (* keep = \"true\" *)\n");
	fprintf(vFp, "  reg [%d:0]		r_in_5;\n", insTotalWidth-1);
	fprintf(vFp, "  //\n");
	fprintf(vFp, "  always @(posedge %s) begin\n", sbxClks.GetSlowClkName().c_str());
	fprintf(vFp, "    r_in_0 <= ins;\n");

	if (insTotalWidth <= 64)
		fprintf(vFp, "    r_in_1 <= r_in_0;\n");
	else if (insTotalWidth < 256)
		fprintf(vFp, "    r_in_1 <= {r_in_0[%d:0],{%d{r_in_0}}};\n", (insTotalWidth-1)%64, (insTotalWidth-1)/64);
	else
		fprintf(vFp, "    r_in_1 <= {4{r_in_0}};\n");

	if (insTotalWidth <= 256)
		fprintf(vFp, "    r_in_2 <= r_in_1;\n");
	else if (insTotalWidth < 1024)
		fprintf(vFp, "    r_in_2 <= {r_in_1[%d:0],{%d{r_in_1}}};\n", (insTotalWidth-1)%256, (insTotalWidth-1)/256);
	else
		fprintf(vFp, "    r_in_2 <= {4{r_in_1}};\n");

	if (insTotalWidth <= 1024)
		fprintf(vFp, "    r_in_3 <= r_in_2;\n");
	else if (insTotalWidth < 4096)
		fprintf(vFp, "    r_in_3 <= {r_in_2[%d:0],{%d{r_in_2}}};\n", (insTotalWidth-1)%1024, (insTotalWidth-1)/1024);
	else
		fprintf(vFp, "    r_in_3 <= {4{r_in_2}};\n");

	if (insTotalWidth <= 4096)
		fprintf(vFp, "    r_in_4 <= r_in_3;\n");
	else if (insTotalWidth < 16384)
		fprintf(vFp, "    r_in_4 <= {r_in_3[%d:0],{%d{r_in_3}}};\n", (insTotalWidth-1)%4096, (insTotalWidth-1)/4096);
	else
		fprintf(vFp, "    r_in_4 <= {4{r_in_3}};\n");

	if (insTotalWidth <= 4096)
		fprintf(vFp, "    r_in_5 <= r_in_4;\n");
	else if (insTotalWidth < 16384)
		fprintf(vFp, "    r_in_5 <= {r_in_4[%d:0],{%d{r_in_4}}};\n", (insTotalWidth-1)%4096, (insTotalWidth-1)/4096);
	else
		fprintf(vFp, "    r_in_5 <= {4{r_in_4}};\n");

	fprintf(vFp, "  end\n");
	fprintf(vFp, "\n");
	fprintf(vFp, "  (* equivalent_register_removal = \"no\" *)\n");
	fprintf(vFp, "  (* shreg_extract = \"no\" *)\n");
	fprintf(vFp, "  (* keep = \"true\" *)\n");
	fprintf(vFp, "  reg [%d:0]		r_out_5;\n", outsTotalWidth > 63 ? 63 : outsTotalWidth-1);
	fprintf(vFp, "\n");
	fprintf(vFp, "  (* equivalent_register_removal = \"no\" *)\n");
	fprintf(vFp, "  (* shreg_extract = \"no\" *)\n");
	fprintf(vFp, "  (* keep = \"true\" *)\n");
	fprintf(vFp, "  reg [%d:0]		r_out_4;\n", outsTotalWidth > 63 ? 63 : outsTotalWidth-1);
	fprintf(vFp, "\n");
	fprintf(vFp, "  (* equivalent_register_removal = \"no\" *)\n");
	fprintf(vFp, "  (* shreg_extract = \"no\" *)\n");
	fprintf(vFp, "  (* keep = \"true\" *)\n");
	fprintf(vFp, "  reg [%d:0]		r_out_3;\n", outsTotalWidth > 63 ? 63 : outsTotalWidth-1);
	fprintf(vFp, "  //\n");
	fprintf(vFp, "  (* equivalent_register_removal = \"no\" *)\n");
	fprintf(vFp, "  (* shreg_extract = \"no\" *)\n");
	fprintf(vFp, "  (* keep = \"true\" *)\n");
	fprintf(vFp, "  reg [%d:0]		r_out_2;\n", outsTotalWidth > 255 ? 255 : outsTotalWidth-1);
	fprintf(vFp, "  //\n");
	fprintf(vFp, "  (* equivalent_register_removal = \"no\" *)\n");
	fprintf(vFp, "  (* shreg_extract = \"no\" *)\n");
	fprintf(vFp, "  (* keep = \"true\" *)\n");
	fprintf(vFp, "  reg [%d:0]		r_out_1;\n", outsTotalWidth > 1023 ? 1023 : outsTotalWidth-1);
	fprintf(vFp, "  //\n");
	fprintf(vFp, "  (* equivalent_register_removal = \"no\" *)\n");
	fprintf(vFp, "  (* shreg_extract = \"no\" *)\n");
	fprintf(vFp, "  (* keep = \"true\" *)\n");
	fprintf(vFp, "  reg [%d:0]		r_out_0;\n", outsTotalWidth > 4095 ? 4095 : outsTotalWidth-1);
	fprintf(vFp, "  //\n");
	fprintf(vFp, "\n");
	fprintf(vFp, "  wire [%d:0]	sb_outs;\n", outsTotalWidth > 4095 ? 4095 : outsTotalWidth-1);
	fprintf(vFp, "  always @(posedge %s) begin\n", sbxClks.GetSlowClkName().c_str());
	fprintf(vFp, "    r_out_0 <= sb_outs;\n");

	if (outsTotalWidth <= 1024)
		fprintf(vFp, "    r_out_1 <= r_out_0;\n");
	else {
		fprintf(vFp, "    r_out_1 <= r_out_0[1023:0]");
		for (int i = 1024; i < 4096 && i < outsTotalWidth; i += 1024)
			fprintf(vFp, " | r_out_0[%d:%d]", outsTotalWidth < i+1024 ? outsTotalWidth-1 : i+1024, i);
		fprintf(vFp, ";\n");
	}

	if (outsTotalWidth <= 256)
		fprintf(vFp, "    r_out_2 <= r_out_1;\n");
	else {
		fprintf(vFp, "    r_out_2 <= r_out_1[255:0]");
		for (int i = 256; i < 1024 && i < outsTotalWidth; i += 256)
			fprintf(vFp, " | r_out_1[%d:%d]", outsTotalWidth < i+256 ? outsTotalWidth-1 : i+255, i);
		fprintf(vFp, ";\n");
	}

	if (outsTotalWidth <= 64)
		fprintf(vFp, "    r_out_3 <= r_out_2;\n");
	else {
		fprintf(vFp, "    r_out_3 <= r_out_2[63:0]");
		for (int i = 64; i < 256 && i < outsTotalWidth; i += 64)
			fprintf(vFp, " | r_out_2[%d:%d]", outsTotalWidth < i+64 ? outsTotalWidth-1 : i+63, i);
		fprintf(vFp, ";\n");
	}

	fprintf(vFp, "    r_out_4 <= r_out_3;\n");
	fprintf(vFp, "    r_out_5 <= r_out_4;\n");

	fprintf(vFp, "  end\n");
	fprintf(vFp, "  assign outs = r_out_5;\n");
	fprintf(vFp, "\n");
	fprintf(vFp, "  //\n");
	fprintf(vFp, "  // sandbox\n");
	fprintf(vFp, "  //\n");
	fprintf(vFp, "  sandbox sb (\n");

	for (int i = 0; i < sbxClks.GetClkCnt(); i += 1)
		fprintf(vFp, "    .%s(%s),\n", sbxClks.GetClkName(i).c_str(), sbxClks.GetClkName(i).c_str());

	if (sbxClks.HasReset())
		fprintf(vFp, "    .reset(reset),\n");

	fprintf(vFp, "    .ins(r_in_5),\n");
	fprintf(vFp, "    .outs(sb_outs)\n");
	fprintf(vFp, "  );\n");
	fprintf(vFp, "\n");
	fprintf(vFp, "endmodule\n");

	fclose(vFp);
}

void CHtvDesign::GenSandboxModuleVFile(string &sbxDir, CSbxClks &sbxClks, CHtvIdent *pModule)
{
	string inputBaseName = g_htvArgs.GetInputFileName();

	string sbxName = "sandbox_" + inputBaseName;

	printf("  Writing sandbox_%s.v file...\n", inputBaseName.c_str());

	string vFile = sbxDir + "/sandbox_" + inputBaseName + ".v";

	FILE *vFp;
	if (!(vFp = fopen(vFile.c_str(), "w"))) {
		printf("Unable to open '%s' for writing", vFile.c_str());
		ErrorExit();
	}

	// Find reset, clocks and count input signals
	int insTotalWidth = 0;

	CHtfeIdentTblIter portIter(pModule->GetIdentTbl());
	for (portIter.Begin(); !portIter.End(); portIter++) {
		if (!portIter->IsInPort())
			continue;

		int clkIdx;
		for (clkIdx = 0; clkIdx < sbxClks.GetClkCnt(); clkIdx += 1) {
			if (portIter->GetName() == sbxClks.GetClkName(clkIdx) ||
				portIter->GetName().substr(0,2) == "i_" && portIter->GetName().substr(2) == sbxClks.GetClkName(clkIdx))
				break;
		}

		if (clkIdx < sbxClks.GetClkCnt() || portIter->GetName() == sbxClks.GetResetName() ||
			portIter->GetName().substr(0,2) == "i_" && portIter->GetName().substr(2) == sbxClks.GetResetName())
			continue;

		int width = portIter->GetType()->GetWidth();

		vector<int> &dimenList = portIter->GetDimenList();
		for (size_t i = 0; i < dimenList.size(); i += 1)
			width *= dimenList[i];

		insTotalWidth += width;
	}

	int outsTotalWidth = 0;
	for (portIter.Begin(); !portIter.End(); portIter++) {
		if (!portIter->IsOutPort())
			continue;

		int width = portIter->GetType()->GetWidth();

		vector<int> &dimenList = portIter->GetDimenList();
		for (size_t i = 0; i < dimenList.size(); i += 1)
			width *= dimenList[i];

		outsTotalWidth += width;
	}

	fprintf(vFp, "(* keep_hierarchy = \"true\" *)\n");
	fprintf(vFp, "module sandbox (\n");

	for (int clkIdx = 0; clkIdx < sbxClks.GetClkCnt(); clkIdx += 1)
		fprintf(vFp, "  input %s,\n", sbxClks.GetClkName(clkIdx).c_str());

	if (sbxClks.HasReset())
		fprintf(vFp, "  input reset,\n");
	fprintf(vFp, "  input [%d:0] ins,\n", insTotalWidth-1);
	fprintf(vFp, "  output [%d:0] outs\n", outsTotalWidth-1);
	fprintf(vFp, ");\n");
	fprintf(vFp, "\n");
	fprintf(vFp, "  //\n");
	if (sbxClks.HasReset()) {
		fprintf(vFp, "  (* keep = \"true\" *) reg r_reset;\n");
		fprintf(vFp, "  always @(posedge %s) r_reset <= reset;\n", sbxClks.GetSlowClkName().c_str());
	}

	fprintf(vFp, "  %s dut (\n", pModule->GetName().c_str());

	int insIdx = 0;
	char *preStr = "";
	for (portIter.Begin(); !portIter.End(); portIter++) {
		if (!portIter->IsInPort())
			continue;

		int clkIdx;
		for (clkIdx = 0; clkIdx < sbxClks.GetClkCnt(); clkIdx += 1) {
			if (portIter->GetName() == sbxClks.GetClkName(clkIdx) ||
				portIter->GetName().substr(0,2) == "i_" && portIter->GetName().substr(2) == sbxClks.GetClkName(clkIdx))
				break;
		}

		if (clkIdx < sbxClks.GetClkCnt())
			fprintf(vFp, "%s    .%s(%s)", preStr, portIter->GetName().c_str(), portIter->GetName().c_str());
		else if (portIter->GetName() == sbxClks.GetResetName() ||
			portIter->GetName().substr(0,2) == "i_" && portIter->GetName().substr(2) == sbxClks.GetResetName()) {
			fprintf(vFp, "%s    .%s(r_reset)", preStr, portIter->GetName().c_str());
		} else {

			vector<int> refList(portIter->GetDimenCnt(), 0);

			do {
				string identName = GenVerilogIdentName(portIter->GetName(), refList);

				int width = portIter->GetType()->GetWidth();
				if (width == 1)
					fprintf(vFp, "%s    .%s(ins[%d])", preStr, identName.c_str(), insIdx);
				else
					fprintf(vFp, "%s    .%s(ins[%d:%d])", preStr, identName.c_str(), insIdx+width-1, insIdx);
				insIdx += width;
			} while (!portIter->DimenIter(refList));
		}
		preStr = ",\n";
	}

	int outsIdx = 0;
	for (portIter.Begin(); !portIter.End(); portIter++) {
		if (!portIter->IsOutPort())
			continue;

		vector<int> refList(portIter->GetDimenCnt(), 0);

		do {
			string identName = GenVerilogIdentName(portIter->GetName(), refList);

			int width = portIter->GetType()->GetWidth();
			if (width == 1)
				fprintf(vFp, "%s    .%s(outs[%d])", preStr, identName.c_str(), outsIdx);
			else
				fprintf(vFp, "%s    .%s(outs[%d:%d])", preStr, identName.c_str(), outsIdx+width-1, outsIdx);
			outsIdx += width;
		} while (!portIter->DimenIter(refList));
	}

	fprintf(vFp, "\n  );\n");

	fprintf(vFp, "endmodule\n");
	fprintf(vFp, "\n");

	fprintf(vFp, "module HtResetFlop (\n");
	fprintf(vFp, "  input clk,\n  input i_reset,\n  output r_reset\n");
	fprintf(vFp, ");\n");
	fprintf(vFp, "  reg r_reset;");
	fprintf(vFp, "  always @(posedge clk) r_reset <= i_reset;\n");
	fprintf(vFp, "endmodule\n");

	fclose(vFp);
}
