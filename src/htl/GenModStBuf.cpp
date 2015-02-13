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

// Generate a module's stencil buffer code

void CDsnInfo::InitAndValidateModStBuf()
{
	// first initialize HtStrings
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule * pMod = m_modList[modIdx];

		if (!pMod->m_bIsUsed) continue;

		for (size_t stBufIdx = 0; stBufIdx < pMod->m_stencilBufferList.size(); stBufIdx += 1) {
			CStencilBuffer * pStBuf = pMod->m_stencilBufferList[stBufIdx];

			pStBuf->m_pipeLen.InitValue( pStBuf->m_lineInfo, false, 0 );

			if (pStBuf->m_gridSize.size() != pStBuf->m_stencilSize.size())
				ParseMsg(Error, pStBuf->m_lineInfo, "expected gridSize and stencilSize dimension to be the same");

			if (pStBuf->m_gridSize.size() != 2)
				ParseMsg(Error, pStBuf->m_lineInfo, "currently only a 2 dimensional stencil buffer is supported");

			// verify that stencil size is less than or equal to grid size
			if (pStBuf->m_gridSize.size() >= 1 && pStBuf->m_gridSize[0] < pStBuf->m_stencilSize[0])
				ParseMsg(Error, pStBuf->m_lineInfo, "grid size (%d) must be larger than stencil size (%d)",
					pStBuf->m_gridSize[0], pStBuf->m_stencilSize[0]);

			if (pStBuf->m_gridSize.size() >= 2 && pStBuf->m_gridSize[1] < pStBuf->m_stencilSize[1])
				ParseMsg(Error, pStBuf->m_lineInfo, "grid size (%d) must be larger than stencil size (%d)",
					pStBuf->m_gridSize[1], pStBuf->m_stencilSize[1]);

			if (pStBuf->m_gridSize.size() >= 3 && pStBuf->m_gridSize[2] < pStBuf->m_stencilSize[2])
				ParseMsg(Error, pStBuf->m_lineInfo, "grid size (%d) must be larger than stencil size (%d)",
					pStBuf->m_gridSize[2], pStBuf->m_stencilSize[2]);

			// Fill stencil buffer structure members
			string inStructName = "CStencilBufferIn";
			string outStructName = "CStencilBufferOut";
			if (pStBuf->m_name.size() > 0) {
				inStructName += "_" + pStBuf->m_name;
				outStructName += "_" + pStBuf->m_name;
			}

			CStruct * pInStruct = FindStruct(inStructName);
			pInStruct->AddField("bool", "m_bValid", "", "");
			pInStruct->AddField(pStBuf->m_type, "m_data", "", "");

			CStruct * pOutStruct = FindStruct(outStructName);
			pOutStruct->AddField("bool", "m_bValid", "", "");

			switch (pStBuf->m_stencilSize.size()) {
			case 1:
				pOutStruct->AddField(pStBuf->m_type, "m_data", VA("%d", pStBuf->m_stencilSize[0]), "" );
				break;
			case 2:
				pOutStruct->AddField(pStBuf->m_type, "m_data", VA("%d", pStBuf->m_stencilSize[1]), VA("%d", pStBuf->m_stencilSize[0]) );
				break;
			default:
				break;
			}

			pOutStruct->m_fieldList[1].DimenListInit(pStBuf->m_lineInfo);
			pOutStruct->m_fieldList[1].InitDimen();
		}
	}
}

void CDsnInfo::GenModStBufStatements(CModule * pMod)
{
	if (pMod->m_stencilBufferList.size() == 0)
		return;

	CHtCode & stBufPreInstr = pMod->m_clkRate == eClk2x ? m_stBufPreInstr2x : m_stBufPreInstr1x;
	CHtCode & stBufReg = pMod->m_clkRate == eClk2x ? m_stBufReg2x : m_stBufReg1x;

	string vcdModName = VA("Pers%s", pMod->m_modName.Uc().c_str());

	g_appArgs.GetDsnRpt().AddLevel("Stencil Buffer\n");

	for (size_t stBufIdx = 0; stBufIdx < pMod->m_stencilBufferList.size(); stBufIdx += 1) {
		CStencilBuffer * pStBuf = pMod->m_stencilBufferList[stBufIdx];

		string stBufName = pStBuf->m_name.size() == 0 ? "" : "_" + pStBuf->m_name;

		GenModDecl(eVcdAll, m_stBufRegDecl, vcdModName, VA("ht_uint%d", FindLg2(pStBuf->m_gridSize[0])),
			VA("r_stBuf%s_xDimIdx", stBufName.c_str()));
		m_stBufRegDecl.Append("\tht_uint%d c_stBuf%s_xDimIdx;\n", FindLg2(pStBuf->m_gridSize[0]), stBufName.c_str());
		stBufPreInstr.Append("\tc_stBuf%s_xDimIdx = r_stBuf%s_xDimIdx;\n",
			stBufName.c_str(), stBufName.c_str());
		stBufReg.Append("\tr_stBuf%s_xDimIdx = c_stBuf%s_xDimIdx;\n",
			stBufName.c_str(), stBufName.c_str());

		if (pStBuf->m_gridSize.size() >= 2) {
			GenModDecl(eVcdAll, m_stBufRegDecl, vcdModName, VA("ht_uint%d", FindLg2(pStBuf->m_gridSize[1])),
				VA("r_stBuf%s_yDimIdx", stBufName.c_str()));
			m_stBufRegDecl.Append("\tht_uint%d c_stBuf%s_yDimIdx;\n", FindLg2(pStBuf->m_gridSize[1]), stBufName.c_str());
			stBufPreInstr.Append("\tc_stBuf%s_yDimIdx = r_stBuf%s_yDimIdx;\n",
				stBufName.c_str(), stBufName.c_str());
			stBufReg.Append("\tr_stBuf%s_yDimIdx = c_stBuf%s_yDimIdx;\n",
				stBufName.c_str(), stBufName.c_str());
		}

		GenModDecl(eVcdAll, m_stBufRegDecl, vcdModName, VA("ht_uint%d", FindLg2(pStBuf->m_gridSize[0])),
			VA("r_stBuf%s_xDimSize", stBufName.c_str()));
		m_stBufRegDecl.Append("\tht_uint%d c_stBuf%s_xDimSize;\n", FindLg2(pStBuf->m_gridSize[0]), stBufName.c_str());
		stBufPreInstr.Append("\tc_stBuf%s_xDimSize = r_stBuf%s_xDimSize;\n",
			stBufName.c_str(), stBufName.c_str());
		stBufReg.Append("\tr_stBuf%s_xDimSize = c_stBuf%s_xDimSize;\n",
			stBufName.c_str(), stBufName.c_str());

		if (pStBuf->m_gridSize.size() >= 2) {
			GenModDecl(eVcdAll, m_stBufRegDecl, vcdModName, VA("ht_uint%d", FindLg2(pStBuf->m_gridSize[1])),
				VA("r_stBuf%s_yDimSize", stBufName.c_str()));
			m_stBufRegDecl.Append("\tht_uint%d c_stBuf%s_yDimSize;\n", FindLg2(pStBuf->m_gridSize[1]), stBufName.c_str());
			stBufPreInstr.Append("\tc_stBuf%s_yDimSize = r_stBuf%s_yDimSize;\n",
				stBufName.c_str(), stBufName.c_str());
			stBufReg.Append("\tr_stBuf%s_yDimSize = c_stBuf%s_yDimSize;\n",
				stBufName.c_str(), stBufName.c_str());
		}

		for (int i = 2; i < pStBuf->m_pipeLen.AsInt()+2; i += 1) {
			m_stBufRegDecl.Append("\tCStencilBufferOut%s c_t%d_stBuf%s_stOut;\n", stBufName.c_str(), i-1, stBufName.c_str());
			GenModDecl(eVcdAll, m_stBufRegDecl, vcdModName, VA("CStencilBufferOut%s", stBufName.c_str()),
				VA("r_t%d_stBuf%s_stOut", i, stBufName.c_str()));
			stBufPreInstr.Append("\tc_t%d_stBuf%s_stOut = r_t%d_stBuf%s_stOut;\n",
				i-1, stBufName.c_str(), i==2?2:i-1, stBufName.c_str());
			stBufReg.Append("\tr_t%d_stBuf%s_stOut = c_t%d_stBuf%s_stOut;\n",
				i, stBufName.c_str(), i-1, stBufName.c_str());
		}

		if (pStBuf->m_stencilSize.size() >= 2 && pStBuf->m_stencilSize[1] >= 2)
			m_stBufRegDecl.Append("\tht_block_que<%s, %d> m_stBuf%s_que[%d];\n", 
				pStBuf->m_type.c_str(), FindLg2(pStBuf->m_gridSize[0]-1),
				stBufName.c_str(), pStBuf->m_stencilSize[1]-1);

		for (int i = 0; i < pStBuf->m_stencilSize[1]-1; i += 1)
			stBufReg.Append("\tm_stBuf%s_que[%d].clock(c_reset1x);\n", stBufName.c_str(), i);

		m_stBufRegDecl.Append("\n");
		stBufPreInstr.Append("\n");
		stBufReg.Append("\n");

		// Init function
		g_appArgs.GetDsnRpt().AddItem("void StencilBufferInit%s(", stBufName.c_str());
		m_stBufFuncDecl.Append("\tvoid StencilBufferInit%s(", stBufName.c_str());
		m_stBufFuncDef.Append("void CPers%s::StencilBufferInit%s(",
			pMod->m_modName.Uc().c_str(), stBufName.c_str());

		int xDimSizeW = FindLg2(pStBuf->m_gridSize[0]);
		g_appArgs.GetDsnRpt().AddText("ht_uint%d const &xDimSize", xDimSizeW);
		m_stBufFuncDecl.Append("ht_uint%d const &xDimSize", xDimSizeW);
		m_stBufFuncDef.Append("ht_uint%d const &xDimSize", xDimSizeW);

		if (pStBuf->m_gridSize.size() >= 2) {
			int yDimSizeW = FindLg2(pStBuf->m_gridSize[1]);
			g_appArgs.GetDsnRpt().AddText(", ht_uint%d const &yDimSize", yDimSizeW);
			m_stBufFuncDecl.Append(", ht_uint%d const &yDimSize", yDimSizeW);
			m_stBufFuncDef.Append(", ht_uint%d const &yDimSize", yDimSizeW);
		}

		if (pStBuf->m_gridSize.size() >= 3) {
			int zDimSizeW = FindLg2(pStBuf->m_gridSize[2]);
			g_appArgs.GetDsnRpt().AddText(", ht_uint%d const &zDimSize", zDimSizeW);
			m_stBufFuncDecl.Append(", ht_uint%d const &zDimSize", zDimSizeW);
			m_stBufFuncDef.Append(", ht_uint%d const &zDimSize", zDimSizeW);
		}

		g_appArgs.GetDsnRpt().AddText("t)\n");
		m_stBufFuncDecl.Append(");\n");
		m_stBufFuncDef.Append(")\n");

		m_stBufFuncDef.Append("{\n");
		m_stBufFuncDef.Append("\tc_stBuf%s_xDimIdx = 0;\n", stBufName.c_str());
		m_stBufFuncDef.Append("\tc_stBuf%s_xDimSize = xDimSize;\n", stBufName.c_str());

		if (pStBuf->m_gridSize.size() >= 2) {
			m_stBufFuncDef.Append("\tc_stBuf%s_yDimIdx = 0;\n", stBufName.c_str());
			m_stBufFuncDef.Append("\tc_stBuf%s_yDimSize = yDimSize;\n", stBufName.c_str());
		}

		if (pStBuf->m_gridSize.size() >= 3) {
			m_stBufFuncDef.Append("\tc_stBuf%s_zDimIdx = 0;\n", stBufName.c_str());
			m_stBufFuncDef.Append("\tc_stBuf%s_zDimSize = zDimSize;\n", stBufName.c_str());
		}

		m_stBufFuncDef.Append("}\n");
		m_stBufFuncDef.Append("\n");

		// stencil buffer function
		g_appArgs.GetDsnRpt().AddItem("void StencilBuffer%s(CStencilBufferIn%s const & stIn, CStencilBufferOut%s & stOut)\n",
			stBufName.c_str(), stBufName.c_str(), stBufName.c_str());
		m_stBufFuncDecl.Append("\tvoid StencilBuffer%s(CStencilBufferIn%s const & stIn, CStencilBufferOut%s & stOut);\n",
			stBufName.c_str(), stBufName.c_str(), stBufName.c_str());
		m_stBufFuncDef.Append("void CPers%s::StencilBuffer%s(CStencilBufferIn%s const & stIn, CStencilBufferOut%s & stOut)\n",
			pMod->m_modName.Uc().c_str(), stBufName.c_str(), stBufName.c_str(), stBufName.c_str());

		m_stBufFuncDef.Append("{\n");
		m_stBufFuncDef.Append("\tif (stIn.m_bValid) {\n");

		if (pStBuf->m_stencilSize.size() >= 2 && pStBuf->m_stencilSize[1] >= 2) {
			m_stBufFuncDef.Append("\t\tif (r_stBuf%s_yDimIdx == 0) {\n", stBufName.c_str());
			m_stBufFuncDef.Append("\t\t\tm_stBuf%s_que[%d].push(stIn.m_data);\n", stBufName.c_str(), pStBuf->m_stencilSize[1]-2);

			for (int i = pStBuf->m_stencilSize[1]-3; i >= 0; i -= 1)
				m_stBufFuncDef.Append("\t\t\tm_stBuf%s_que[%d].push(0);\n", stBufName.c_str(), i);

			m_stBufFuncDef.Append("\t\t} else if (r_stBuf%s_yDimIdx == r_stBuf%s_yDimSize + %d - 1) {\n",
				stBufName.c_str(), stBufName.c_str(), pStBuf->m_stencilSize[1]-1);

			for (int i = pStBuf->m_stencilSize[1]-2; i >= 0; i -= 1)
				m_stBufFuncDef.Append("\t\t\tm_stBuf%s_que[%d].pop();\n", stBufName.c_str(), i);

			m_stBufFuncDef.Append("\t\t} else {\n");

			m_stBufFuncDef.Append("\t\t\tm_stBuf%s_que[%d].push(stIn.m_data);\n", stBufName.c_str(), pStBuf->m_stencilSize[1]-2);

			for (int i = pStBuf->m_stencilSize[1]-3; i >= 0; i -= 1)
				m_stBufFuncDef.Append("\t\t\tm_stBuf%s_que[%d].push(m_stBuf%s_que[%d].front());\n", stBufName.c_str(), i, stBufName.c_str(), i+1);

			m_stBufFuncDef.Append("\n");

			for (int i = pStBuf->m_stencilSize[1]-2; i >= 0; i -= 1)
				m_stBufFuncDef.Append("\t\t\tm_stBuf%s_que[%d].pop();\n", stBufName.c_str(), i);

			m_stBufFuncDef.Append("\t\t}\n");
			m_stBufFuncDef.Append("\n");
		}
		m_stBufFuncDef.Append("\t\tc_stBuf%s_xDimIdx += 1;\n", stBufName.c_str());
		m_stBufFuncDef.Append("\t\tif (c_stBuf%s_xDimIdx == r_stBuf%s_xDimSize + %d) {\n", 
			stBufName.c_str(), stBufName.c_str(), pStBuf->m_stencilSize[0]-1);
		m_stBufFuncDef.Append("\t\t\tc_stBuf%s_xDimIdx = 0;\n", stBufName.c_str());
		m_stBufFuncDef.Append("\t\t\tc_stBuf%s_yDimIdx += 1;\n", stBufName.c_str());
		m_stBufFuncDef.Append("\t\t}\n");
		m_stBufFuncDef.Append("\n");

		m_stBufFuncDef.Append("\t\tbool bStValid = r_stBuf%s_yDimIdx >= %d && r_stBuf%s_xDimIdx >= %d;\n",
			stBufName.c_str(), pStBuf->m_stencilSize[1]-1, stBufName.c_str(), pStBuf->m_stencilSize[0]-1);
		m_stBufFuncDef.Append("\n");

		m_stBufFuncDef.Append("\t\t%s stData[%d];\n", pStBuf->m_type.c_str(), pStBuf->m_stencilSize[1]);
		for (int i = 0; i < pStBuf->m_stencilSize[1]-1; i += 1)
			m_stBufFuncDef.Append("\t\tstData[%d] = m_stBuf%s_que[%d].front();\n", i, stBufName.c_str(), i);

		m_stBufFuncDef.Append("\t\tstData[%d] = stIn.m_data;\n", pStBuf->m_stencilSize[1]-1);
		m_stBufFuncDef.Append("\n");

		m_stBufFuncDef.Append("\t\tfor (int x = 0; x < %d; x += 1) {\n", pStBuf->m_stencilSize[0]);
		m_stBufFuncDef.Append("\t\t\tfor (int y = 0; y < %d; y += 1) {\n", pStBuf->m_stencilSize[1]);
		m_stBufFuncDef.Append("\t\t\t\tif (x == %d)\n", pStBuf->m_stencilSize[0]-1);
		m_stBufFuncDef.Append("\t\t\t\t\tc_t1_stBuf%s_stOut.m_data[y][x] = stData[y];\n", stBufName.c_str());
		m_stBufFuncDef.Append("\t\t\t\telse\n");
		m_stBufFuncDef.Append("\t\t\t\t\tc_t1_stBuf%s_stOut.m_data[y][x] = r_t2_stBuf%s_stOut.m_data[y][x+1];\n",
			stBufName.c_str(), stBufName.c_str());
		m_stBufFuncDef.Append("\t\t\t}\n");
		m_stBufFuncDef.Append("\t\t}\n");
		m_stBufFuncDef.Append("\n");

		m_stBufFuncDef.Append("\t\tc_t1_stBuf%s_stOut.m_bValid = bStValid;\n", stBufName.c_str());
		m_stBufFuncDef.Append("\t} else\n");
		m_stBufFuncDef.Append("\t\tc_t1_stBuf%s_stOut.m_bValid = false;\n", stBufName.c_str());
		m_stBufFuncDef.Append("\n");

		m_stBufFuncDef.Append("\n\tstOut = r_t%d_stBuf%s_stOut;\n", pStBuf->m_pipeLen.AsInt()+1, stBufName.c_str());
		m_stBufFuncDef.Append("}\n");
		m_stBufFuncDef.Append("\n");
	}
}
