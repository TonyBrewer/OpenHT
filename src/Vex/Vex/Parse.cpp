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

// name, isInput, isClock, bits
CDesign::CPrimPort CDesign::X_BUF_PrimPorts[] = {
	{ "I", true, false, 1 },
	{ "O", false, false, 1 },
	{ 0 }
};
CDesign::CPrimPort CDesign::X_INV_PrimPorts[] = {
	{ "I", true, false, 1 },
	{ "O", false, false, 1 },
	{ 0 }
};
CDesign::CPrimPort CDesign::X_IBUFDS_PrimPorts[] = {
	{ "I", true, false, 1 },
	{ "IB", true, false, 1 },
	{ "O", false, false, 1 },
	{ 0 }
};
CDesign::CPrimPort CDesign::X_FF_PrimPorts[] = {
	{ "CE", true, false, 1 },
	{ "CLK", true, true, 1 },
	{ "I", true, false, 1 },
	{ "O", false, false, 1 },
	{ "SET", true, false, 1 },
	{ "RST", true, false, 1 },
	{ 0 }
};
CDesign::CPrimPort CDesign::FD_PrimPorts[] = {
	{ "C", true, true, 1 },
	{ "D", true, false, 1 },
	{ "Q", false, false, 1 },
	{ 0 }
};
CDesign::CPrimPort CDesign::FDE_PrimPorts[] = {
	{ "C", true, true, 1 },
	{ "D", true, false, 1 },
	{ "CE", true, false, 1 },
	{ "Q", false, false, 1 },
	{ 0 }
};
CDesign::CPrimPort CDesign::FDR_PrimPorts[] = {
	{ "C", true, true, 1 },
	{ "R", true, false, 1 },
	{ "D", true, false, 1 },
	{ "Q", false, false, 1 },
	{ 0 }
};
CDesign::CPrimPort CDesign::FDRE_PrimPorts[] = {
	{ "C", true, true, 1 },
	{ "R", true, false, 1 },
	{ "CE", true, false, 1 },
	{ "D", true, false, 1 },
	{ "Q", false, false, 1 },
	{ 0 }
};
CDesign::CPrimPort CDesign::FDS_PrimPorts[] = {
	{ "C", true, true, 1 },
	{ "S", true, false, 1 },
	{ "D", true, false, 1 },
	{ "Q", false, false, 1 },
	{ 0 }
};
CDesign::CPrimPort CDesign::FDSE_PrimPorts[] = {
	{ "C", true, true, 1 },
	{ "S", true, false, 1 },
	{ "CE", true, false, 1 },
	{ "D", true, false, 1 },
	{ "Q", false, false, 1 },
	{ 0 }
};
CDesign::CPrimPort CDesign::FDRS_PrimPorts[] = {
	{ "C", true, true, 1 },
	{ "S", true, false, 1 },
	{ "R", true, false, 1 },
	{ "D", true, false, 1 },
	{ "Q", false, false, 1 },
	{ 0 }
};
CDesign::CPrimPort CDesign::FDRSE_PrimPorts[] = {
	{ "C", true, true, 1 },
	{ "S", true, false, 1 },
	{ "R", true, false, 1 },
	{ "CE", true, false, 1 },
	{ "D", true, false, 1 },
	{ "Q", false, false, 1 },
	{ 0 }
};
CDesign::CPrimPort CDesign::X_IPAD_PrimPorts[] = {
	{ "PAD", true, false, 1 },
	{ 0 }
};
CDesign::CPrimPort CDesign::X_OPAD_PrimPorts[] = {
	{ "PAD", false, false, 1 },
	{ 0 }
};
CDesign::CPrimPort CDesign::X_OBUF_PrimPorts[] = {
	{ "I", true, false, 1 },
	{ "O", false, false, 1 },
	{ 0 }
};
CDesign::CPrimPort CDesign::X_PLL_ADV_PrimPorts[] = {
	{ "CLKIN1", true, false, 1 },
	{ "CLKIN2", true, false, 1 },
	{ "CLKFBIN", true, false, 1 },
	{ "RST", true, false, 1 },
	{ "CLKINSEL", true, false, 1 },
	{ "DWE", true, false, 1 },
	{ "DEN", true, false, 1 },
	{ "DCLK", true, false, 1 },
	{ "REL", true, false, 1 },
	{ "CLKOUT0", false, true, 1 },
	{ "CLKOUT1", false, true, 1 },
	{ "CLKOUT2", false, true, 1 },
	{ "CLKOUT3", false, true, 1 },
	{ "CLKOUT4", false, true, 1 },
	{ "CLKOUT5", false, true, 1 },
	{ "CLKFBOUT", false, true, 1 },
	{ "CLKOUTDCM0", false, true, 1 },
	{ "CLKOUTDCM1", false, true, 1 },
	{ "CLKOUTDCM2", false, true, 1 },
	{ "CLKOUTDCM3", false, true, 1 },
	{ "CLKOUTDCM4", false, true, 1 },
	{ "CLKOUTDCM5", false, true, 1 },
	{ "CLKFBDCM", false, true, 1 },
	{ "LOCKED", false, true, 1 },
	{ "DRDY", false, true, 1 },
	{ "DADDR", false, true, 5 },
	{ "DI", true, true, 16 },
	{ "DO", false, true, 16 },
	{ 0 }
};
CDesign::CPrimPort CDesign::LUT6_PrimPorts[] = {
	{ "I0", true, false, 1 },
	{ "I1", true, false, 1 },
	{ "I2", true, false, 1 },
	{ "I3", true, false, 1 },
	{ "I4", true, false, 1 },
	{ "I5", true, false, 1 },
	{ "O", false, false, 1 },
	{ 0 }
};
CDesign::CPrimPort CDesign::LUT5_PrimPorts[] = {
	{ "I0", true, false, 1 },
	{ "I1", true, false, 1 },
	{ "I2", true, false, 1 },
	{ "I3", true, false, 1 },
	{ "I4", true, false, 1 },
	{ "O", false, false, 1 },
	{ 0 }
};
CDesign::CPrimPort CDesign::LUT4_PrimPorts[] = {
	{ "I0", true, false, 1 },
	{ "I1", true, false, 1 },
	{ "I2", true, false, 1 },
	{ "I3", true, false, 1 },
	{ "O", false, false, 1 },
	{ 0 }
};
CDesign::CPrimPort CDesign::LUT3_PrimPorts[] = {
	{ "I0", true, false, 1 },
	{ "I1", true, false, 1 },
	{ "I2", true, false, 1 },
	{ "O", false, false, 1 },
	{ 0 }
};
CDesign::CPrimPort CDesign::LUT2_PrimPorts[] = {
	{ "I0", true, false, 1 },
	{ "I1", true, false, 1 },
	{ "O", false, false, 1 },
	{ 0 }
};
CDesign::CPrimPort CDesign::LUT1_PrimPorts[] = {
	{ "I0", true, false, 1 },
	{ "O", false, false, 1 },
	{ 0 }
};
CDesign::CPrimPort CDesign::LUT6_L_PrimPorts[] = {
	{ "I0", true, false, 1 },
	{ "I1", true, false, 1 },
	{ "I2", true, false, 1 },
	{ "I3", true, false, 1 },
	{ "I4", true, false, 1 },
	{ "I5", true, false, 1 },
	{ "LO", false, false, 1 },
	{ 0 }
};
CDesign::CPrimPort CDesign::LUT5_L_PrimPorts[] = {
	{ "I0", true, false, 1 },
	{ "I1", true, false, 1 },
	{ "I2", true, false, 1 },
	{ "I3", true, false, 1 },
	{ "I4", true, false, 1 },
	{ "LO", false, false, 1 },
	{ 0 }
};
CDesign::CPrimPort CDesign::LUT4_L_PrimPorts[] = {
	{ "I0", true, false, 1 },
	{ "I1", true, false, 1 },
	{ "I2", true, false, 1 },
	{ "I3", true, false, 1 },
	{ "LO", false, false, 1 },
	{ 0 }
};
CDesign::CPrimPort CDesign::LUT3_L_PrimPorts[] = {
	{ "I0", true, false, 1 },
	{ "I1", true, false, 1 },
	{ "I2", true, false, 1 },
	{ "LO", false, false, 1 },
	{ 0 }
};
CDesign::CPrimPort CDesign::LUT2_L_PrimPorts[] = {
	{ "I0", true, false, 1 },
	{ "I1", true, false, 1 },
	{ "LO", false, false, 1 },
	{ 0 }
};
CDesign::CPrimPort CDesign::LUT1_L_PrimPorts[] = {
	{ "I0", true, false, 1 },
	{ "LO", false, false, 1 },
	{ 0 }
};
CDesign::CPrimPort CDesign::X_LUT5_PrimPorts[] = {
	{ "ADR0", true, false, 1 },
	{ "ADR1", true, false, 1 },
	{ "ADR2", true, false, 1 },
	{ "ADR3", true, false, 1 },
	{ "ADR4", true, false, 1 },
	{ "O", false, false, 1 },
	{ 0 }
};
CDesign::CPrimPort CDesign::X_LUT6_PrimPorts[] = {
	{ "ADR0", true, false, 1 },
	{ "ADR1", true, false, 1 },
	{ "ADR2", true, false, 1 },
	{ "ADR3", true, false, 1 },
	{ "ADR4", true, false, 1 },
	{ "ADR5", true, false, 1 },
	{ "O", false, false, 1 },
	{ 0 }
};
CDesign::CPrimPort CDesign::X_ONE_PrimPorts[] = {
	{ "O", false, false, 1 },
	{ 0 }
};
CDesign::CPrimPort CDesign::VCC_PrimPorts[] = {
	{ "P", false, false, 1 },
	{ 0 }
};
CDesign::CPrimPort CDesign::X_ZERO_PrimPorts[] = {
	{ "O", false, false, 1 },
	{ 0 }
};
CDesign::CPrimPort CDesign::GND_PrimPorts[] = {
	{ "G", false, false, 1 },
	{ 0 }
};
CDesign::CPrimPort CDesign::X_CARRY4_PrimPorts[] = {
	{ "CI", true, false, 1 },
	{ "CYINIT", true, false, 1 },
	{ "CO", false, false, 4 },
	{ "DI", true, false, 4 },
	{ "O", false, false, 4 },
	{ "S", true, false, 4 },
	{ 0 }
};
CDesign::CPrimPort CDesign::CARRY4_PrimPorts[] = {
	{ "CI", true, false, 1 },
	{ "CYINIT", true, false, 1 },
	{ "CO", false, false, 4 },
	{ "DI", true, false, 4 },
	{ "O", false, false, 4 },
	{ "S", true, false, 4 },
	{ 0 }
};
CDesign::CPrimPort CDesign::X_MUX2_PrimPorts[] = {
	{ "IA", true, false, 1 },
	{ "IB", true, false, 1 },
	{ "SEL", true, false, 1 },
	{ "O", false, false, 1 },
	{ 0 }
};
CDesign::CPrimPort CDesign::X_CKBUF_PrimPorts[] = {
	{ "I", true, false, 1 },
	{ "O", false, false, 1 },
	{ 0 }
};
CDesign::CPrimPort CDesign::SRL16_PrimPorts[] = {
	{ "CLK", true, true, 1 },
	{ "D", true, false, 1 },
	{ "A0", true, false, 1 },
	{ "A1", true, false, 1 },
	{ "A2", true, false, 1 },
	{ "A3", true, false, 1 },
	{ "Q", false, false, 1 },
	{ 0 }
};
CDesign::CPrimPort CDesign::SRL16E_PrimPorts[] = {
	{ "CLK", true, true, 1 },
	{ "CE", true, false, 1 },
	{ "D", true, false, 1 },
	{ "A0", true, false, 1 },
	{ "A1", true, false, 1 },
	{ "A2", true, false, 1 },
	{ "A3", true, false, 1 },
	{ "Q", false, false, 1 },
	{ 0 }
};
CDesign::CPrimPort CDesign::MUXF7_PrimPorts[] = {
	{ "S", true, false, 1 },
	{ "I0", true, false, 1 },
	{ "I1", true, false, 1 },
	{ "O", false, false, 1 },
	{ 0 }
};
CDesign::CPrimPort CDesign::MUXF8_PrimPorts[] = {
	{ "S", true, false, 1 },
	{ "I0", true, false, 1 },
	{ "I1", true, false, 1 },
	{ "O", false, false, 1 },
	{ 0 }
};
CDesign::CPrimPort CDesign::DUMMY_LOAD_PrimPorts[] = {
	{ "L", true, false, 1 },
	{ 0 }
};
CDesign::CPrimPort CDesign::RAM32X1D_PrimPorts[] = {
	{ "DPO", false, false, 1 },
	{ "SPO", false, false, 1 },
	{ "D", true, false, 1 },
	{ "A0", true, false, 1 },
	{ "A1", true, false, 1 },
	{ "A2", true, false, 1 },
	{ "A3", true, false, 1 },
	{ "A4", true, false, 1 },
	{ "DPRA0", true, false, 1 },
	{ "DPRA1", true, false, 1 },
	{ "DPRA2", true, false, 1 },
	{ "DPRA3", true, false, 1 },
	{ "DPRA4", true, false, 1 },
	{ "WE", true, false, 1 },
	{ "WCLK", true, true, 1 },
	{ 0 }
};

CDesign::CPrimPort CDesign::DSP48E_PrimPorts[] = {
	{ "P", false, false, 48 },
	{ "PCOUT", false, false, 48 },
	{ "CARRYOUT", false, false, 4 },
	{ "MULTSIGNOUT", false, false, 1 },
	{ "CARRYCASCOUT", false, false, 1 },
	{ "PATTERNDETECT", false, false, 1 },
	{ "PATTERNBDETECT", false, false, 1 },
	{ "OVERFLOW", false, false, 1 },
	{ "UNDERFLOW", false, false, 1 },
	{ "ACOUT", false, false, 30 },
	{ "BCOUT", false, false, 18 },
	{ "A", true, false, 30 },
	{ "B", true, false, 18 },
	{ "C", true, false, 48 },
	{ "BCIN", true, false, 18 },
	{ "ACIN", true, false, 30 },
	{ "PCIN", true, false, 48 },
	{ "MULTSIGNIN", true, false, 1 },
	{ "CARRYCASCIN", true, false, 1 },
	{ "CARRYIN", true, false, 1 },
	{ "CARRYINSEL", true, false, 3 },
	{ "OPMODE", true, false, 7 },
	{ "ALUMODE", true, false, 4 },
	{ "CEA1", true, false, 1 },
	{ "CEA2", true, false, 1 },
	{ "CEB1", true, false, 1 },
	{ "CEB2", true, false, 1 },
	{ "CEC", true, false, 1 },
	{ "CEALUMODE", true, false, 1 },
	{ "CECARRYIN", true, false, 1 },
	{ "CECTRL", true, false, 1 },
	{ "CEM", true, false, 1 },
	{ "CEMULTCARRYIN", true, false, 1 },
	{ "CEP", true, false, 1 },
	{ "RSTA", true, false, 1 },
	{ "RSTB", true, false, 1 },
	{ "RSTC", true, false, 1 },
	{ "RSTM", true, false, 1 },
	{ "RSTP", true, false, 1 },
	{ "RSTALLCARRYIN", true, false, 1 },
	{ "RSTALUMODE", true, false, 1 },
	{ "RSTCTRL", true, false, 1 },
	{ "CLK", true, true, 1 },
	{ 0 }
};

CDesign::CPrimPort CDesign::DSP48E1_PrimPorts[] = {
	{ "A", true, false, 30 },
	{ "ACIN", true, false, 30 },
	{ "ACOUT", false, false, 30 },
	{ "ALUMODE", true, false, 4 },
	{ "B", true, false, 18 },
	{ "BCIN", true, false, 18 },
	{ "BCOUT", false, false, 18 },
	{ "C", true, false, 48 },
	{ "CARRYCASCIN", true, false, 1 },
	{ "CARRYCASCOUT", false, false, 1 },
	{ "CARRYIN", true, false, 1 },
	{ "CARRYINSEL", true, false, 3 },
	{ "CARRYOUT", false, false, 4 },
	{ "CEA1", true, false, 1 },
	{ "CEA2", true, false, 1 },
	{ "CEAD", true, false, 1 },
	{ "CEALUMODE", true, false, 1 },
	{ "CEB1", true, false, 1 },
	{ "CEB2", true, false, 1 },
	{ "CEC", true, false, 1 },
	{ "CECARRYIN", true, false, 1 },
	{ "CECTRL", true, false, 1 },
	{ "CED", true, false, 1 },
	{ "CEINMODE", true, false, 1 },
	{ "CEM", true, false, 1 },
	{ "CEP", true, false, 1 },
	{ "CLK", true, true, 1 },
	{ "D", true, false, 25 },
	{ "INMODE", true, false, 5 },
	{ "MULTSIGNIN", true, false, 1 },
	{ "MULTSIGNOUT", false, false, 1 },
	{ "OPMODE", true, false, 7 },
	{ "OVERFLOW", false, false, 1 },
	{ "P", false, false, 48 },
	{ "PATTERNBDETECT", false, false, 1 },
	{ "PATTERNDETECT", false, false, 1 },
	{ "PCIN", true, false, 48 },
	{ "PCOUT", false, false, 48 },
	{ "RSTA", true, false, 1 },
	{ "RSTALLCARRYIN", true, false, 1 },
	{ "RSTALUMODE", true, false, 1 },
	{ "RSTB", true, false, 1 },
	{ "RSTC", true, false, 1 },
	{ "RSTCTRL", true, false, 1 },
	{ "RSTD", true, false, 1 },
	{ "RSTINMODE", true, false, 1 },
	{ "RSTM", true, false, 1 },
	{ "RSTP", true, false, 1 },
	{ "UNDERFLOW", false, false, 1 },
	{ 0 }
};

CDesign::CPrimPort CDesign::RAMB36E1_PrimPorts[] = {
	{ "DIADI", true, false, 32 },
	{ "DIPADIP", true, false, 4 },
	{ "DIBDI", true, false, 32 },
	{ "DIPBDIP", true, false, 4 },
	{ "ADDRARDADDR", true, false, 16 },
	{ "ADDRBWRADDR", true, false, 16 },
	{ "WEA", true, false, 4 },
	{ "WEBWE", true, false, 8 },
	{ "ENARDEN", true, false, 1 },
	{ "ENBWREN", true, false, 1 },
	{ "RSTREGARSTREG", true, false, 1 },
	{ "RSTREGB", true, false, 1 },
	{ "RSTRAMARSTRAM", true, false, 1 },
	{ "RSTRAMB", true, false, 1 },
	{ "CLKARDCLK", true, true, 1 },
	{ "CLKBWRCLK", true, true, 1 },
	{ "REGCEAREGCE", true, false, 1 },
	{ "REGCEB", true, false, 1 },
	{ "CASCADEINA", true, false, 1 },
	{ "CASCADEINB", true, false, 1 },
	{ "CASCADEOUTA", false, false, 1 },
	{ "CASCADEOUTB", false, false, 1 },
	{ "DOADO", false, false, 32 },
	{ "DOPADOP", false, false, 4 },
	{ "DOBDO", false, false, 32 },
	{ "DOPBDOP", false, false, 4 },
	{ "DBITERR", false, false, 1 },
	{ "ECCPARITY", false, false, 8 },
	{ "RDADDRECC", false, false, 9 },
	{ "SBITERR", false, false, 1 },
	{ "INJECTDBITERR", true, false, 1 },
	{ "INJECTSBITERR", true, false, 1 },
	{ 0 }
};

CDesign::CDesign(const char *pFileName) : CLex(pFileName)
{
	ModuleMapInsert("X_IPAD", X_IPAD_PrimPorts);
	ModuleMapInsert("X_OPAD", X_OPAD_PrimPorts);
	ModuleMapInsert("X_OBUF", X_OBUF_PrimPorts);
	ModuleMapInsert("X_BUF", X_BUF_PrimPorts);
	ModuleMapInsert("X_INV", X_INV_PrimPorts);
	ModuleMapInsert("X_IBUFDS", X_IBUFDS_PrimPorts);
	ModuleMapInsert("X_PLL_ADV", X_PLL_ADV_PrimPorts);
	ModuleMapInsert("X_FF", X_FF_PrimPorts);
	ModuleMapInsert("X_LUT5", X_LUT5_PrimPorts);
	ModuleMapInsert("X_LUT6", X_LUT6_PrimPorts);
	ModuleMapInsert("LUT6", LUT6_PrimPorts);
	ModuleMapInsert("LUT5", LUT5_PrimPorts);
	ModuleMapInsert("LUT4", LUT4_PrimPorts);
	ModuleMapInsert("LUT3", LUT3_PrimPorts);
	ModuleMapInsert("LUT2", LUT2_PrimPorts);
	ModuleMapInsert("LUT1", LUT1_PrimPorts);
	ModuleMapInsert("LUT6_L", LUT6_L_PrimPorts);
	ModuleMapInsert("LUT5_L", LUT5_L_PrimPorts);
	ModuleMapInsert("LUT4_L", LUT4_L_PrimPorts);
	ModuleMapInsert("LUT3_L", LUT3_L_PrimPorts);
	ModuleMapInsert("LUT2_L", LUT2_L_PrimPorts);
	ModuleMapInsert("LUT1_L", LUT1_L_PrimPorts);
	ModuleMapInsert("FD", FD_PrimPorts);
	ModuleMapInsert("FDE", FDE_PrimPorts);
	ModuleMapInsert("FDR", FDR_PrimPorts);
	ModuleMapInsert("FDRE", FDRE_PrimPorts);
	ModuleMapInsert("FDS", FDS_PrimPorts);
	ModuleMapInsert("FDSE", FDSE_PrimPorts);
	ModuleMapInsert("FDRS", FDRS_PrimPorts);
	ModuleMapInsert("FDRSE", FDRSE_PrimPorts);
	ModuleMapInsert("X_ZERO", X_ZERO_PrimPorts);
	ModuleMapInsert("GND", GND_PrimPorts);
	ModuleMapInsert("X_ONE", X_ONE_PrimPorts);
	ModuleMapInsert("VCC", VCC_PrimPorts);
	ModuleMapInsert("X_CARRY4", X_CARRY4_PrimPorts);
	ModuleMapInsert("CARRY4", CARRY4_PrimPorts);
	ModuleMapInsert("X_MUX2", X_MUX2_PrimPorts);
	ModuleMapInsert("X_CKBUF", X_CKBUF_PrimPorts);
	ModuleMapInsert("SRL16", SRL16_PrimPorts);
	ModuleMapInsert("SRL16E", SRL16E_PrimPorts);
	ModuleMapInsert("MUXF7", MUXF7_PrimPorts);
	ModuleMapInsert("MUXF8", MUXF8_PrimPorts);
	ModuleMapInsert("DUMMY_LOAD", DUMMY_LOAD_PrimPorts);
	ModuleMapInsert("RAM32X1D", RAM32X1D_PrimPorts);
	ModuleMapInsert("DSP48E", DSP48E_PrimPorts);
	ModuleMapInsert("DSP48E1", DSP48E1_PrimPorts);
	ModuleMapInsert("RAMB36E1", RAMB36E1_PrimPorts);
}

CDesign::~CDesign(void)
{
}

void
CDesign::ModuleMapInsert(string name, CPrimPort *pPorts)
{
	ModuleMapInsert(name);
	m_pModule->m_bPrim = true;
	m_pModule->m_bDummyLoad = (name == "DUMMY_LOAD");

	for (int i = 0;  ; i += 1) {
		CPrimPort *pPrimPort = &pPorts[i];
		if (pPrimPort->m_pName == 0)
			break;

		string portName = pPrimPort->m_pName;
		CSignal *pSignal = SignalMapInsert(portName, pPrimPort->m_bits-1, 0);
		pSignal->m_sigType = pPrimPort->m_bInput ? sig_input : sig_output;
		pSignal->m_bClock = pPrimPort->m_bClock;
	}
}

CDesign::CSignal *
CDesign::SignalMapInsert(string &name, int upperBit, int lowerBit)
{
	SignalMap_InsertPair insertPair;
	insertPair = m_pModule->m_signalMap.insert(SignalMap_ValuePair(name, (CSignal *)0));

	CSignal *pSignal;
	if (insertPair.second || insertPair.first->second->m_sigType == sig_port) {
		if (!insertPair.second)
			pSignal = insertPair.first->second;
		else {
			pSignal = insertPair.first->second = new CSignal;
			pSignal->m_name = name;
		}

		if (upperBit != -1) {
			pSignal->m_bIndexed = true;
			pSignal->m_lowerIdx = lowerBit;
			pSignal->m_upperIdx = upperBit;
			pSignal->m_pIndexedSignalPorts = new vector<SignalPortVector>;
			pSignal->m_pIndexedSignalPorts->reserve(upperBit + 1);

			SignalPortVector spv;
			pSignal->m_pIndexedSignalPorts->insert(pSignal->m_pIndexedSignalPorts->begin(), upperBit + 1, spv);
		} else {
			pSignal->m_bIndexed = false;
			pSignal->m_pSignalPorts = new SignalPortVector;
		}

		pSignal->m_lineInfo = GetLineInfo();

		if (strncmp(name.c_str()+name.size()-3, "$UN", 3) == 0)
			pSignal->m_bUnconnected = true;
	} else {
		ParseMsg(PARSE_ERROR, "Signal re-declared: '%s'", name.c_str());
		pSignal = insertPair.first->second;
	}

	return pSignal;
}

CDesign::CInst *
CDesign::InstMapInsert(string &name)
{
	InstMap_InsertPair insertPair;
	insertPair = m_pModule->m_instMap.insert(InstMap_ValuePair(name, (CInst *)0));

	CInst *pInst;
	if (insertPair.second) {
		pInst = insertPair.first->second = new CInst;
		pInst->m_name = name;
		pInst->m_lineInfo = GetLineInfo();
	} else {
		ParseMsg(PARSE_ERROR, "Instance name reused: '%s'", name.c_str());
		pInst = insertPair.first->second;
	}

	return pInst;
}

bool
CDesign::ModuleMapInsert(string &name)
{
	ModuleMap_InsertPair insertPair;
	insertPair = m_moduleMap.insert(ModuleMap_ValuePair(name, CModule()));

	if (insertPair.second) {
		m_pModule = &insertPair.first->second;
		m_pModule->m_name = name;
		return true;
	} else {
		ParseMsg(PARSE_ERROR, "Module name reused: '%s'", name.c_str());
		return false;
	}
}

void
CDesign::DefparamMapInsert(string &heirName, bool bIsString, string &paramValue)
{
	DefparamMap_InsertPair insertPair;
	insertPair = m_pModule->m_defparamMap.insert(DefparamMap_ValuePair(heirName, CDefparam(heirName, bIsString, paramValue)));

	if (!insertPair.second)
		ParseMsg(PARSE_ERROR, "Defparam instance name / parameter name re-defined");
	else
		insertPair.first->second.m_lineInfo = GetLineInfo();
}

void
CDesign::Parse()
{
	GetNextToken();
	do {
		switch (GetToken()) {
		case tk_eof:
			return;
		case tk_ident:
			if (GetString() == "module") {
				ParseModule();
				continue;
			}
		default:
			ParseMsg(PARSE_ERROR, "unknown statement type");
			SkipTo(tk_semicolon);
			GetNextToken();
			break;
		} 
	} while (GetToken() != tk_eof);
}

void
CDesign::ParseModule()
{
	bool bEndOfSignals = false;

	if (GetNextToken() != tk_ident)
		ParseMsg(PARSE_ERROR, "Expected module name");

	if (GetToken() != tk_ident || GetString() == "glbl" || !ModuleMapInsert(GetString())) {
		while (GetNextToken() != tk_ident || GetString() != "endmodule");
		GetNextToken();
		return;
	}

	if (GetNextToken() != tk_lparen) {
		ParseMsg(PARSE_ERROR, "Expected '('");
		SkipTo(tk_semicolon);
	} else {
		for (int portIdx = 0; ; portIdx += 1) {
			if (GetNextToken() != tk_ident) {
				ParseMsg(PARSE_ERROR, "Expected '('");
				SkipTo(tk_semicolon);
				return;
			}

			CSignal *pSignal = SignalMapInsert(GetString(), -1, -1);
			pSignal->m_sigType = sig_port;
			pSignal->m_portIdx = portIdx;

			if (GetNextToken() != tk_comma && GetToken() != tk_rparen) {
				ParseMsg(PARSE_ERROR, "Expected ')'");
				SkipTo(tk_semicolon);
				return;
			}

			if (GetToken() == tk_rparen)
				break;
		}

		if (GetNextToken() != tk_semicolon)
			ParseMsg(PARSE_ERROR, "Expected ';'");
	}

	CModule *pInstModule;
	do {

		switch (GetNextToken()) {
		case tk_ident:
			if (GetString() == "input") {
				if (bEndOfSignals)
					ParseMsg(PARSE_ERROR, "declaration of input within statement section");

				ParseSignal(sig_input);
				continue;
			} else if (GetString() == "output") {
				if (bEndOfSignals)
					ParseMsg(PARSE_ERROR, "declaration of input within statement section");

				ParseSignal(sig_output);
				continue;
			} else if (GetString() == "wire") {
				if (bEndOfSignals)
					ParseMsg(PARSE_ERROR, "declaration of input within statement section");

				ParseSignal(sig_wire);
				continue;
			} else if (GetString() == "initial") {
				SkipTo(tk_semicolon);
				break;
			} else if (GetString() == "defparam") {
				ParseDefparam();
				break;
			} else if (GetString() == "ifdef") {
				ParseIfdef();
				break;
			} else if (GetString() == "synthesis") {
				ParseSynthesis();
				break;
			} else if (pInstModule = FindModule(GetString())) {
				if (!bEndOfSignals) {
					bEndOfSignals = true;
					CheckPortDeclarations();
				}
				ParseModuleInst(pInstModule);
				break;
			} else if (GetString() == "endmodule") {
				GetNextToken();
				m_pModule = 0;
				return;
			} else {
				ParseMsg(PARSE_ERROR, "unknown statement identifier '%s'", GetString().c_str());
				SkipTo(tk_semicolon);
			}
			break;
		default:
			ParseMsg(PARSE_ERROR, "unknown statement type");
			SkipTo(tk_semicolon);
			GetNextToken();
			break;
		} 
	} while (GetToken() != tk_eof);
}

void
CDesign::ParseSignal(ESigType sigType)
{
	int upperBit = -1;
	int lowerBit = -1;
	if (GetNextToken() == tk_lbrack) {	// multi-bit signal
		if (GetNextToken() != tk_num) {
			ParseMsg(PARSE_ERROR, "expected bit index");
			SkipTo(tk_semicolon);
			return;
		}
		int width;
		upperBit = GetInteger(width);
		if (GetNextToken() != tk_colon) {
			ParseMsg(PARSE_ERROR, "expected colon");
			SkipTo(tk_semicolon);
			return;
		}
		if (GetNextToken() != tk_num) {
			ParseMsg(PARSE_ERROR, "expected bit index");
			SkipTo(tk_semicolon);
			return;
		}
		lowerBit = GetInteger(width);
		if (GetNextToken() != tk_rbrack) {
			ParseMsg(PARSE_ERROR, "expected ']'");
			SkipTo(tk_semicolon);
			return;
		}
		GetNextToken();
	}

	if (GetToken() != tk_ident) {
		ParseMsg(PARSE_ERROR, "expected signal name");
		SkipTo(tk_semicolon);
		return;
	}

	CSignal *pSignal = SignalMapInsert(GetString(), upperBit, lowerBit);
	pSignal->m_sigType = sigType;
	pSignal->m_bDeclared = true;

	if (GetNextToken() != tk_semicolon) {
		ParseMsg(PARSE_ERROR, "expected ';'");
		SkipTo(tk_semicolon);
	}
}

void
CDesign::ParseModuleInst(CModule *pModule)
{
	if (GetNextToken() != tk_ident) {
		ParseMsg(PARSE_ERROR, "expected module instance name");
		SkipTo(tk_semicolon);
		return;
	}

	CInst *pInst = InstMapInsert(GetString());
	pInst->m_pModule = pModule;

	ClearPortUsedFlags(pModule);

	if (GetNextToken() != tk_lparen) {
		ParseMsg(PARSE_ERROR, "Expected '('");
		SkipTo(tk_semicolon);
		return;
	}
	
	for (;;) {
		if (GetNextToken() != tk_period) {
			ParseMsg(PARSE_ERROR, "Expected '.'");
			SkipTo(tk_semicolon);
			return;
		}

		if (GetNextToken() != tk_ident) {
			ParseMsg(PARSE_ERROR, "Expected port name");
			SkipTo(tk_semicolon);
			return;
		}

		CSignal *pModulePort;
		CInstPort *pInstPort = 0;
		int portIdx = -1;
		if (!(pModulePort = pModule->FindPort(GetString())))
			ParseMsg(PARSE_ERROR, "Unknown instance port name: %s %s port %s",
				pModule->m_name.c_str(), pInst->m_name.c_str(), GetString().c_str());
		else if (pModulePort->m_bPortUsed) {
			ParseMsg(PARSE_ERROR, "duplicate instance port name: %s %s port %s",
				pModule->m_name.c_str(), pInst->m_name.c_str(), GetString().c_str());
		} else {
			// check for duplicate port names
			pModulePort->m_bPortUsed = true;
			portIdx = (int)pInst->m_instPorts.size();
			int idx;
			for (idx = 0; idx < portIdx; idx += 1) {
				if (pInst->m_instPorts[idx].m_pModulePort == pModulePort) {
					ParseMsg(PARSE_ERROR, "duplicate instance port names");
					break;
				}
			}

			if (idx == portIdx) {
				pInst->m_instPorts.push_back(CInstPort(pModulePort));
				pInstPort = &pInst->m_instPorts[portIdx];

				if (pModulePort->GetWidth() > 1) {
					pInstPort->m_pVector = new vector<CPortSignal>;
					pInstPort->m_pVector->reserve(pModulePort->GetWidth());
					pInstPort->m_pVector->insert(pInstPort->m_pVector->begin(), pModulePort->GetWidth(), CPortSignal());
				}
			} else
				portIdx = -1;
		}

		if (GetNextToken() != tk_lparen) {
			ParseMsg(PARSE_ERROR, "Expected '('");
			SkipTo(tk_semicolon);
			return;
		}

		unsigned int portBit = 0;
		if (!ParsePortSignalList(pInst, portIdx, portBit))
			return;

		if (pInstPort && (int)portBit != pInstPort->m_pModulePort->GetWidth())
			ParseMsg(PARSE_ERROR, "port width (%d), signal width (%d) mismatch", pInstPort->m_pModulePort->GetWidth(), portBit);

		if (GetNextToken() != tk_comma && GetToken() != tk_rparen) {
			ParseMsg(PARSE_ERROR, "Expected ')'");
			SkipTo(tk_semicolon);
			return;
		}

		if (GetToken() == tk_rparen)
			break;
	}

	CheckPortUsedFlags(pModule);

	if (GetNextToken() != tk_semicolon) {
		ParseMsg(PARSE_ERROR, "expected ';'");
		SkipTo(tk_semicolon);
	}
}

bool
CDesign::ParsePortSignalList(CInst *pInst, int portIdx, unsigned int &portBit)
{
	vector<CPortSignalInfo> signalList;

	if (GetNextToken() == tk_lbrace) {
		if (!ParseConcatSignal(signalList))
			return false;
		GetNextToken();
	} else if (GetToken() == tk_ident) {

		CSignal *pSignal;
		int upperBit = -1;
		int lowerBit = -1;

		if (ParsePortSignal(pSignal, upperBit, lowerBit))
			return true;

		signalList.push_back(CPortSignalInfo(pSignal, upperBit, lowerBit));

	} else if (GetToken() == tk_num) {

		int width;
		uint64 value = GetInteger(width);
		int upperBit = width > 0 ? width-1 : -1;
		int lowerBit = width > 0 ? 0 : -1;

		signalList.push_back(CPortSignalInfo(value, upperBit, lowerBit));

		GetNextToken();

	} else if (GetToken() == tk_rparen) {
		if (pInst->GetModuleName() != "CARRY4" || pInst->m_instPorts[portIdx].GetModulePortName() != "CI") {
			ParseMsg(PARSE_ERROR, "Expected signal name");
			SkipTo(tk_semicolon);
			return false;
		}
		portBit = 1;
		return true;

	} else {
		ParseMsg(PARSE_ERROR, "Expected signal name");
		SkipTo(tk_semicolon);
		return false;
	}

	for (uint32 i = 0; i < signalList.size(); i += 1) {
		CSignal *pSignal = 0;
		uint64 value = 0;
		bool bIsConst = signalList[i].m_bConst;
		if (bIsConst)
			value = signalList[i].m_value;
		else
			pSignal = signalList[i].m_pSignal;
		int upperBit = signalList[i].m_upperBit;
		int lowerBit = signalList[i].m_lowerBit;

		for (int sigBit = upperBit; sigBit >= lowerBit; sigBit -= 1, portBit += 1) {
			if (bIsConst) {
				if (portIdx < 0)
					return true;

				CInstPort *pInstPort = &pInst->m_instPorts[portIdx];
				uint32 portWidth = (uint32)(pInstPort->m_pModulePort->GetWidth() > 1 ? pInstPort->m_pVector->size() : 1);

				if (portWidth <= portBit) {
					ParseMsg(PARSE_ERROR, "too many signals for port");
					continue;
				}

				if (sigBit == -1) {
					if (i+1 != signalList.size()) {
						ParseMsg(PARSE_ERROR, "Unable to assign port signals for constant with undefined width");
						return true;
					}
					sigBit = portWidth - portBit - 1;
					lowerBit = 0;
				}
				uint32 bitValue = (uint32)((value >> sigBit) & 1);
				if (bitValue == 0)
					pSignal = &m_zeroSignal;
				else
					pSignal = &m_oneSignal;

				if (pInstPort->m_pModulePort->GetWidth() > 1)
					(*pInstPort->m_pVector)[portBit] = CPortSignal(pSignal, sigBit);
				else
					pInstPort->m_pSingle = new CPortSignal(pSignal, sigBit);

				continue;
			}

			SignalPortVector *pSignalPorts = 0;
			if ((upperBit != -1) && !pSignal->m_bIndexed) {
				ParseMsg(PARSE_ERROR, "Indexing of non-indexed signal: '%s'", pSignal->m_name.c_str());
				continue;
			} else if (upperBit == -1 && pSignal->m_bIndexed) {
				ParseMsg(PARSE_ERROR, "Indexed signal without bit index");
				continue;
			} else if (pSignal->m_bIndexed && upperBit > pSignal->m_upperIdx) {
				pSignal->m_pIndexedSignalPorts->reserve(upperBit + 1);
				SignalPortVector spv;
				pSignal->m_pIndexedSignalPorts->insert(pSignal->m_pIndexedSignalPorts->begin() + pSignal->m_upperIdx + 1, upperBit - pSignal->m_upperIdx, spv);
				pSignal->m_upperIdx = upperBit;
			} else if (pSignal->m_bIndexed && lowerBit < pSignal->m_lowerIdx) {
				if (pSignal->m_bDeclared) {
					ParseMsg(PARSE_ERROR, "Index out of range: [ %d : %d ]", pSignal->m_upperIdx, pSignal->m_lowerIdx);
					continue;
				} else
					pSignal->m_lowerIdx = lowerBit;
			}

			if (pSignal->m_bIndexed)
				pSignalPorts = &(*pSignal->m_pIndexedSignalPorts)[sigBit];
			else
				pSignalPorts = pSignal->m_pSignalPorts;

			if (portIdx < 0)
				return true;

			CInstPort *pInstPort = &pInst->m_instPorts[portIdx];
			size_t portWidth = pInstPort->m_pModulePort->GetWidth() > 1 ? pInstPort->m_pVector->size() : 1;
			if (portWidth <= portBit) {
				ParseMsg(PARSE_ERROR, "too many signals for port");
				continue;
			}

			CSignalPort signalPort;
			signalPort.m_pInst = pInst;
			signalPort.m_portIdx = portIdx;
			signalPort.m_portBit = portBit;

			pSignalPorts->push_back(signalPort);

			if (pInstPort->m_pModulePort->GetWidth() > 1)
				(*pInstPort->m_pVector)[portBit] = CPortSignal(pSignal, sigBit);
			else
				pInstPort->m_pSingle = new CPortSignal(pSignal, sigBit);
		}
	}

	return true;
}

bool
CDesign::ParsePortSignal(CSignal *&pSignal, int &upperBit, int &lowerBit)
{
	string signalName = GetString();
	pSignal = SignalMapFind(signalName);

	if (GetNextToken() == tk_lbrack) {	// multi-bit signal
		if (GetNextToken() != tk_num) {
			ParseMsg(PARSE_ERROR, "expected bit index");
			SkipTo(tk_semicolon);
			return true;
		}
		int width;
		upperBit = lowerBit = GetInteger(width);
		if (GetNextToken() == tk_colon) {
			if (GetNextToken() != tk_num) {
				ParseMsg(PARSE_ERROR, "expected bit index");
				SkipTo(tk_semicolon);
				return true;
			}
			lowerBit = GetInteger(width);
			GetNextToken();
		}
		if (GetToken() != tk_rbrack) {
			ParseMsg(PARSE_ERROR, "expected ']'");
			SkipTo(tk_semicolon);
			return true;
		}
		GetNextToken();
	}

	if (!pSignal) {
		pSignal = SignalMapInsert(signalName, upperBit, lowerBit);
		pSignal->m_sigType = sig_wire;
		pSignal->m_bDeclared = false;
	}

	if (upperBit == -1 && pSignal->m_bIndexed) {
		upperBit = pSignal->m_upperIdx;
		lowerBit = pSignal->m_lowerIdx;
	}

	return false;
}

bool
CDesign::ParseConcatSignal(vector<CPortSignalInfo> &signalList)
{
	for (;;) {

		if (GetNextToken() == tk_lbrace) {
			if (!ParseConcatSignal(signalList))
				return false;
			GetNextToken();
		} else if (GetToken() == tk_ident) {

			CSignal *pSignal;
			int upperBit = -1;
			int lowerBit = -1;

			if (ParsePortSignal(pSignal, upperBit, lowerBit))
				return true;

			signalList.push_back(CPortSignalInfo(pSignal, upperBit, lowerBit));

		} else if (GetToken() == tk_num) {

			vector<CPortSignalInfo> signalList2;
			int width;
			int value = GetInteger(width);

			if (GetNextToken() == tk_lbrace) {
				int repCnt = value;

				if (!ParseConcatSignal(signalList2))
					return false;

				GetNextToken();

				for (int i = 0; i < repCnt; i += 1) {
					for (uint32 j = 0; j < signalList2.size(); j += 1)
						signalList.push_back(signalList2[j]);
				}
			} else {
				int upperBit = width > 0 ? width-1 : -1;
				int lowerBit = width > 0 ? 0 : -1;

				signalList.push_back(CPortSignalInfo(value, upperBit, lowerBit));
			}

			//if (GetNextToken() != tk_lbrace) {
			//	ParseMsg(PARSE_ERROR, "Expected a '{' after integer replication count");
			//	SkipTo(tk_semicolon);
			//	return false;
			//}

		} else {
			ParseMsg(PARSE_ERROR, "Expected signal name");
			SkipTo(tk_semicolon);
			return false;
		}

		if (GetToken() != tk_comma && GetToken() != tk_rbrace) {
			ParseMsg(PARSE_ERROR, "Expected '}'");
			SkipTo(tk_semicolon);
			return false;
		}

		if (GetToken() == tk_rbrace)
			break;
	}

	//for (;;) {
	//	if (!ParsePortSignalList(pInst, portIdx, portBit))
	//		return false;

	//	if (GetToken() != tk_comma && GetToken() != tk_rbrace) {
	//		ParseMsg(PARSE_ERROR, "Expected '}'");
	//		SkipTo(tk_semicolon);
	//		return false;
	//	}

	//	if (GetToken() == tk_rbrace)
	//		break;
	//}

	return true;
}

void
CDesign::ParseSynthesis()
{
	if (GetNextToken() != tk_ident || GetString() != "attribute") {
		ParseMsg(PARSE_ERROR, "expected 'attribute' keyword");
		SkipTo(tk_semicolon);
		GetNextToken();
		return;
	}

	if (GetNextToken() != tk_ident) {
		ParseMsg(PARSE_ERROR, "expected synthesis attribute name");
		SkipTo(tk_semicolon);
		GetNextToken();
		return;
	}
	string attribName = GetString();

	if (GetNextToken() != tk_ident) {
		ParseMsg(PARSE_ERROR, "expected synthesis instance name");
		SkipTo(tk_semicolon);
		GetNextToken();
		return;
	}
	string instName = GetString();

	if (GetNextToken() != tk_string) {
		ParseMsg(PARSE_ERROR, "expected synthesis instance value");
		SkipTo(tk_semicolon);
		GetNextToken();
		return;
	}
	string valueName = GetString();

	m_pModule->m_synAttrib.push_back(CSynAttrib(attribName, instName, valueName));

	if (GetNextToken() != tk_semicolon)
		ParseMsg(PARSE_ERROR, "expected a ';'");
}

void
CDesign::ParseIfdef()
{
	if (GetNextToken() != tk_ident) {
		ParseMsg(PARSE_ERROR, "expected ifdef identifier");
		GetLineBuf();
		GetNextToken();
		return;
	}

	string ifdefName = GetString();
	string ifdefLine = GetLineBuf();

	if (m_pModule->m_ifdefName.size() > 0) {
		if (m_pModule->m_ifdefName != ifdefName) {
			ParseMsg(PARSE_ERROR, "inconsistent ifdef names within module");
			GetNextToken();
			return;
		}
	} else
		m_pModule->m_ifdefName = ifdefName;

	m_pModule->m_ifdefLine.push_back(ifdefLine);

	// set next statement file starting position
	AdvanceStatementFilePos();
}

void
CDesign::ParseDefparam()
{
	string heirName;
	for (;;) {
		if (GetNextToken() != tk_ident) {
			ParseMsg(PARSE_ERROR, "expected a module instance name");
			SkipTo(tk_semicolon);
			return;
		}

		heirName += GetString();

		if (GetNextToken() == tk_equal)
			break;

		if (GetToken() != tk_period) {
			ParseMsg(PARSE_ERROR, "expected a '=' or '.'");
			SkipTo(tk_semicolon);
			return;
		}

		heirName += ".";
	}

	if (GetNextToken() != tk_string && GetToken() != tk_num) {
		ParseMsg(PARSE_ERROR, "expected a defparam parameter value");
		SkipTo(tk_semicolon);
		return;
	}

	DefparamMapInsert(heirName, GetToken() == tk_string, GetString());

	if (GetNextToken() != tk_semicolon)
		ParseMsg(PARSE_ERROR, "expected a ';'");
}

void
CDesign::CheckPortDeclarations()
{
	// verify all module ports were declared
	SignalMap_Iter sigIter;
	for (sigIter = m_pModule->m_signalMap.begin(); sigIter != m_pModule->m_signalMap.end(); sigIter++) {
		if (sigIter->second->m_sigType == sig_port)
			ParseMsg(PARSE_ERROR, sigIter->second->m_lineInfo,
				"Port signal '%s' was not declared", sigIter->second->m_name.c_str());
	}
}

void
CDesign::ClearPortUsedFlags(CModule *pModule) {
	for (SignalMap_Iter iter = pModule->m_signalMap.begin(); iter != pModule->m_signalMap.end(); iter++)
		iter->second->m_bPortUsed = false;
}

void
CDesign::CheckPortUsedFlags(CModule *pModule) {
	for (SignalMap_Iter iter = pModule->m_signalMap.begin(); iter != pModule->m_signalMap.end(); iter++) {
		if ((iter->second->m_sigType == sig_input || iter->second->m_sigType == sig_output) &&
			!iter->second->m_bPortUsed)
			ParseMsg(PARSE_ERROR, "unassigned instance port: %s", iter->second->m_name.c_str());
	}
}
