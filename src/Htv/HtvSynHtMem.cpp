/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "Htv.h"
#include "HtvDesign.h"

void CHtvDesign::SynHtDistQueRams(CHtvIdent *pHier)
{
	string inputFileName = g_htvArgs.GetInputFileName();

	CHtvIdentTblIter identIter(pHier->GetFlatIdentTbl());
	for (identIter.Begin(); !identIter.End(); identIter++) {
		if (!identIter->IsVariable() || !identIter->IsHtDistQue())
			continue;

		CHtvIdent *pQueueRam = identIter();
		m_bIsHtQueueRamsPresent = true;

		// Block Ram
		int addrWidth = pQueueRam->GetType()->GetHtMemoryAddrWidth1();
		addrWidth += pQueueRam->GetType()->GetHtMemoryAddrWidth2();

		string cQueName = pQueueRam->GetName();
		if (cQueName.substr(0,2) == "r_" || cQueName.substr(0,2) == "m_")
			cQueName.replace(0, 2, "c_");
		else
			cQueName.insert(0, "c_");

		string rQueName = pQueueRam->GetName();
		if (rQueName.substr(0,2) == "m_")
			rQueName.replace(0, 2, "r_");

		vector<int> refList(pQueueRam->GetDimenCnt(), 0);

		char buf[128];
		do {
			string refDimStr;
			string modDimStr;

			for (size_t i = 0; i < refList.size(); i += 1) {
				sprintf(buf, "$%d", refList[i]);
				refDimStr += buf;

				sprintf(buf, "$%d", refList[i]);
				modDimStr += buf;
			}

			m_vFile.Print("\n%s_HtQueueRam #( .DATA_WIDTH(%d), .ADDR_WIDTH(%d) ) %s%s ( ",
				inputFileName.c_str(), pQueueRam->GetWidth(), addrWidth, pQueueRam->GetName().c_str(), modDimStr.c_str());

			bool bMultiple;
			CHtvIdent * pMethod = FindUniqueAccessMethod(&pQueueRam->GetHierIdent()->GetWriterList(), bMultiple);

			if (bMultiple)
				ParseMsg(PARSE_FATAL, pQueueRam->GetLineInfo(), "ht_queue push by muliple methods");

			m_vFile.Print(".clk( %s ), ", pMethod->GetClockIdent()->GetName().c_str());
			m_vFile.Print(".we( %s_WrEn%s ), ", cQueName.c_str(), refDimStr.c_str());
			m_vFile.Print(".wa( %s_WrAddr%s[%d:0] ), ", rQueName.c_str(), refDimStr.c_str(), addrWidth-1);
			m_vFile.Print(".din( %s_WrData%s ), ", cQueName.c_str(), refDimStr.c_str());
			m_vFile.Print(".ra( %s_RdAddr%s[%d:0] ), ", rQueName.c_str(), refDimStr.c_str(), addrWidth-1);
			m_vFile.Print(".dout( %s_RdData%s ) );\n", cQueName.c_str(), refDimStr.c_str());

		} while (!pQueueRam->DimenIter(refList));
	}
}

void CHtvDesign::SynHtBlockQueRams(CHtvIdent *pHier)
{
	string inputFileName = g_htvArgs.GetInputFileName();

	CHtvIdentTblIter identIter(pHier->GetFlatIdentTbl());
	for (identIter.Begin(); !identIter.End(); identIter++) {
		if (!identIter->IsVariable() || !identIter->IsHtBlockQue())
			continue;

		CHtvIdent *pQueueRam = identIter();
		m_bIs1CkHtBlockRamsPresent = true;

		// Block Ram
		int addrWidth = pQueueRam->GetType()->GetHtMemoryAddrWidth1();
		addrWidth += pQueueRam->GetType()->GetHtMemoryAddrWidth2();

		string cQueName = pQueueRam->GetName();
		if (cQueName.substr(0,2) == "r_" || cQueName.substr(0,2) == "m_")
			cQueName.replace(0, 2, "c_");
		else
			cQueName.insert(0, "c_");

		string rQueName = pQueueRam->GetName();
		if (rQueName.substr(0,2) == "m_")
			rQueName.replace(0, 2, "r_");

		vector<int> refList(pQueueRam->GetDimenCnt(), 0);

		char buf[128];
		do {
			string refDimStr;
			string modDimStr;

			for (size_t i = 0; i < refList.size(); i += 1) {
				sprintf(buf, "$%d", refList[i]);
				refDimStr += buf;

				sprintf(buf, "$%d", refList[i]);
				modDimStr += buf;
			}

			if (pQueueRam->GetType()->IsHtBlockRamDoReg())
				m_vFile.Print("\n%s_HtBlockRam1CkDoReg #( .DATA_WIDTH(%d), .ADDR_WIDTH(%d) ) %s%s ( ",
					inputFileName.c_str(), pQueueRam->GetWidth(), addrWidth, pQueueRam->GetName().c_str(), modDimStr.c_str());
			else
				m_vFile.Print("\n%s_HtBlockRam1Ck #( .DATA_WIDTH(%d), .ADDR_WIDTH(%d) ) %s%s ( ",
					inputFileName.c_str(), pQueueRam->GetWidth(), addrWidth, pQueueRam->GetName().c_str(), modDimStr.c_str());

			bool bMultiple;
			CHtvIdent * pMethod = FindUniqueAccessMethod(&pQueueRam->GetHierIdent()->GetWriterList(), bMultiple);

			if (bMultiple)
				ParseMsg(PARSE_FATAL, pQueueRam->GetLineInfo(), "ht_queue push by muliple methods");

			m_vFile.Print(".clk( %s ), ", pMethod->GetClockIdent()->GetName().c_str());
			m_vFile.Print(".we( %s_WrEn%s ), ", cQueName.c_str(), refDimStr.c_str());
			m_vFile.Print(".wa( %s_WrAddr%s[%d:0] ), ", rQueName.c_str(), refDimStr.c_str(), addrWidth-1);
			m_vFile.Print(".din( %s_WrData%s ), ", cQueName.c_str(), refDimStr.c_str());
			m_vFile.Print(".ra( %s_RdAddr%s[%d:0] ), ", cQueName.c_str(), refDimStr.c_str(), addrWidth-1);
			m_vFile.Print(".dout( %s_RdData%s ) );\n", cQueName.c_str(), refDimStr.c_str());

		} while (!pQueueRam->DimenIter(refList));
	}
}

void CHtvDesign::SynHtDistRams(CHtvIdent *pHier)
{
	string inputFileName = g_htvArgs.GetInputFileName();

	CHtvIdentTblIter identIter(pHier->GetFlatIdentTbl());
	for (identIter.Begin(); !identIter.End(); identIter++) {
		if (!identIter->IsVariable() || !identIter->IsHtMemory())
			continue;

		if (identIter->IsHtBlockRam() || identIter->IsHtMrdBlockRam() || identIter->IsHtMwrBlockRam() || identIter->IsHtQueue())
			continue;

		CHtvIdent *pDistRam = identIter();
		m_bIsHtDistRamsPresent = true;

		int addrWidth = pDistRam->GetType()->GetHtMemoryAddrWidth1();
		addrWidth += pDistRam->GetType()->GetHtMemoryAddrWidth2();

		vector<CHtDistRamWeWidth> *pWeWidth = pDistRam->GetHtDistRamWeWidth();

		for (size_t j = 0; j < pWeWidth->size(); j += 1) {
			int highBit = (*pWeWidth)[j].m_highBit;
			int lowBit = (*pWeWidth)[j].m_lowBit;

			size_t i;
			for (i = 0; i < m_htDistRamTypes.size(); i += 1) {
				if (m_htDistRamTypes[i].m_addrWidth == addrWidth &&
					m_htDistRamTypes[i].m_dataWidth == highBit-lowBit+1)
					break;
			}

			if (i == m_htDistRamTypes.size())
				m_htDistRamTypes.push_back(CHtDistRamType(addrWidth, highBit-lowBit+1));
		}

		string cQueName = pDistRam->GetName();
		if (cQueName.substr(0,2) == "r_" || cQueName.substr(0,2) == "m_")
			cQueName.replace(0, 2, "c_");
		else
			cQueName.insert(0, "c_");

		bool bMultiple;
		CHtvIdent * pMethod = FindUniqueAccessMethod(&pDistRam->GetHierIdent()->GetWriterList(), bMultiple);

		if (bMultiple)
			ParseMsg(PARSE_FATAL, pDistRam->GetLineInfo(), "ht_queue push by muliple methods");

		CHtvIdent *pClockIdent = pMethod->GetClockIdent();

		vector<int> refList(pDistRam->GetDimenCnt(), 0);

		char buf[128];
		do {
			string refDimStr;
			string modDimStr;

			for (size_t i = 0; i < refList.size(); i += 1) {
				sprintf(buf, "$%d", refList[i]);
				refDimStr += buf;

				sprintf(buf, "$%d", refList[i]);
				modDimStr += buf;
			}

			if (pWeWidth->size() == 1) {
				m_vFile.Print("\n%s_HtDistRam #( .DATA_WIDTH(%d), .ADDR_WIDTH(%d) ) %s%s ( ",
					inputFileName.c_str(), pDistRam->GetWidth(), addrWidth, pDistRam->GetName().c_str(), modDimStr.c_str());

				m_vFile.Print(".clk( %s ), ", pClockIdent->GetName().c_str());
				m_vFile.Print(".we( %s_WrEn%s ), ", cQueName.c_str(), refDimStr.c_str());
				m_vFile.Print(".wa( %s_WrAddr%s ), ", cQueName.c_str(), refDimStr.c_str());
				m_vFile.Print(".din( %s_WrData%s ), ", cQueName.c_str(), refDimStr.c_str());
				m_vFile.Print(".ra( %s_RdAddr%s ), ", cQueName.c_str(), refDimStr.c_str());
				m_vFile.Print(".dout( %s_RdData%s ) );\n", cQueName.c_str(), refDimStr.c_str());

			} else {
				for (size_t j = 0; j < pWeWidth->size(); j += 1) {
					int highBit = (*pWeWidth)[j].m_highBit;
					int lowBit = (*pWeWidth)[j].m_lowBit;

					m_vFile.Print("\n%s_HtDistRam #( .DATA_WIDTH(%d), .ADDR_WIDTH(%d) ) %s%s_%d_%d ( ",
						inputFileName.c_str(), highBit-lowBit+1, addrWidth,
						pDistRam->GetName().c_str(), modDimStr.c_str(), highBit, lowBit);

					m_vFile.Print(".clk( %s ), ", pClockIdent->GetName().c_str());
					m_vFile.Print(".we( %s_WrEn_%d_%d%s ), ", cQueName.c_str(), highBit, lowBit, refDimStr.c_str());
					m_vFile.Print(".wa( %s_WrAddr%s ), ", cQueName.c_str(), refDimStr.c_str());
					m_vFile.Print(".din( %s_WrData%s[%d:%d] ), ", cQueName.c_str(), refDimStr.c_str(), highBit, lowBit);
					m_vFile.Print(".ra( %s_RdAddr%s ), ", cQueName.c_str(), refDimStr.c_str());
					m_vFile.Print(".dout( %s_RdData%s[%d:%d] ) );\n", cQueName.c_str(), refDimStr.c_str(), highBit, lowBit);
				}
			}

		} while (!pDistRam->DimenIter(refList));
	}
}

void CHtvDesign::SynHtBlockRams(CHtvIdent *pHier)
{
	string inputFileName = g_htvArgs.GetInputFileName();

	CHtvIdentTblIter identIter(pHier->GetFlatIdentTbl());
	for (identIter.Begin(); !identIter.End(); identIter++) {
		if (!identIter->IsVariable() || (!identIter->IsHtQueue() && !identIter->IsHtMemory()))
			continue;

		if (!identIter->IsHtBlockRam())
			continue;

		CHtvIdent *pBlockRam = identIter();

		// Block Ram
		int addrWidth = pBlockRam->GetType()->GetHtMemoryAddrWidth1();
		addrWidth += pBlockRam->GetType()->GetHtMemoryAddrWidth2();

		string cQueName = pBlockRam->GetName();
		if (cQueName.substr(0,2) == "r_" || cQueName.substr(0,2) == "m_")
			cQueName.replace(0, 2, "c_");
		else
			cQueName.insert(0, "c_");

		vector<int> refList(pBlockRam->GetDimenCnt(), 0);

		char buf[128];
		do {
			string refDimStr;
			string modDimStr;

			for (size_t i = 0; i < refList.size(); i += 1) {
				sprintf(buf, "$%d", refList[i]);
				refDimStr += buf;

				sprintf(buf, "$%d", refList[i]);
				modDimStr += buf;
			}

			bool bMultiple;
			CHtvIdent * pWriteMethod = FindUniqueAccessMethod(&pBlockRam->GetHierIdent()->GetWriterList(), bMultiple);

			if (bMultiple)
				ParseMsg(PARSE_FATAL, pBlockRam->GetLineInfo(), "ht_block_ram write by muliple methods");

			CHtvIdent * pReadMethod = FindUniqueAccessMethod(&pBlockRam->GetHierIdent()->GetReaderList(), bMultiple);

			if (bMultiple)
				ParseMsg(PARSE_FATAL, pBlockRam->GetLineInfo(), "ht_block_ram read by muliple methods");

			if (pReadMethod->GetClockIdent()->GetName() == pWriteMethod->GetClockIdent()->GetName()) {

				if (pBlockRam->GetType()->IsHtBlockRamDoReg()) {
					m_bIs1CkDoRegHtBlockRamsPresent = true;
					m_vFile.Print("\n%s_HtBlockRam1CkDoReg #( .DATA_WIDTH(%d), .ADDR_WIDTH(%d) ) %s%s ( ",
						inputFileName.c_str(), pBlockRam->GetWidth(), addrWidth, pBlockRam->GetName().c_str(), modDimStr.c_str());
				} else {
					m_bIs1CkHtBlockRamsPresent = true;
					m_vFile.Print("\n%s_HtBlockRam1Ck #( .DATA_WIDTH(%d), .ADDR_WIDTH(%d) ) %s%s ( ",
						inputFileName.c_str(), pBlockRam->GetWidth(), addrWidth, pBlockRam->GetName().c_str(), modDimStr.c_str());
				}

			} else {
				if (pBlockRam->GetType()->IsHtBlockRamDoReg()) {
					m_bIs2CkDoRegHtBlockRamsPresent = true;
					m_vFile.Print("\n%s_HtBlockRam2CkDoReg #( .DATA_WIDTH(%d), .ADDR_WIDTH(%d) ) %s%s ( ",
						inputFileName.c_str(), pBlockRam->GetWidth(), addrWidth, pBlockRam->GetName().c_str(), modDimStr.c_str());
				} else {
					m_bIs2CkHtBlockRamsPresent = true;
					m_vFile.Print("\n%s_HtBlockRam2Ck #( .DATA_WIDTH(%d), .ADDR_WIDTH(%d) ) %s%s ( ",
						inputFileName.c_str(), pBlockRam->GetWidth(), addrWidth, pBlockRam->GetName().c_str(), modDimStr.c_str());
				}
				m_vFile.Print(".rclk( %s ), ", pReadMethod->GetClockIdent()->GetName().c_str());
			}

			m_vFile.Print(".clk( %s ), ", pWriteMethod->GetClockIdent()->GetName().c_str());
			m_vFile.Print(".we( %s_WrEn%s ), ", cQueName.c_str(), refDimStr.c_str());
			m_vFile.Print(".wa( %s_WrAddr%s ), ", cQueName.c_str(), refDimStr.c_str());
			m_vFile.Print(".din( %s_WrData%s ), ", cQueName.c_str(), refDimStr.c_str());
			m_vFile.Print(".ra( %s_RdAddr%s ), ", cQueName.c_str(), refDimStr.c_str());
			m_vFile.Print(".dout( %s_RdData%s ) );\n", cQueName.c_str(), refDimStr.c_str());

		} while (!pBlockRam->DimenIter(refList));
	}
}

void CHtvDesign::SynHtAsymBlockRams(CHtvIdent *pHier)
{
	string inputFileName = g_htvArgs.GetInputFileName();

	CHtvIdentTblIter identIter(pHier->GetFlatIdentTbl());
	for (identIter.Begin(); !identIter.End(); identIter++) {
		if (!identIter->IsVariable() || (!identIter->IsHtQueue() && !identIter->IsHtMemory()))
			continue;

		if (!identIter->IsHtMrdBlockRam() && !identIter->IsHtMwrBlockRam())
			continue;

		CHtvIdent *pBlockRam = identIter();

		// Block Ram
		int addrWidth = pBlockRam->GetType()->GetHtMemoryAddrWidth1();
		addrWidth += pBlockRam->GetType()->GetHtMemoryAddrWidth2();

		int selWidth = pBlockRam->GetType()->GetHtMemorySelWidth();

		int dataWidth = pBlockRam->GetType()->GetType()->GetWidth();

		string cQueName = pBlockRam->GetName();
		if (cQueName.substr(0,2) == "r_" || cQueName.substr(0,2) == "m_")
			cQueName.replace(0, 2, "c_");
		else
			cQueName.insert(0, "c_");

		vector<int> refList(pBlockRam->GetDimenCnt(), 0);

		char buf[128];
		do {
			string refDimStr;
			string modDimStr;

			for (size_t i = 0; i < refList.size(); i += 1) {
				sprintf(buf, "$%d", refList[i]);
				refDimStr += buf;

				sprintf(buf, "$%d", refList[i]);
				modDimStr += buf;
			}

			bool bMultiple;
			CHtvIdent * pWriteMethod = FindUniqueAccessMethod(&pBlockRam->GetHierIdent()->GetWriterList(), bMultiple);

			if (bMultiple)
				ParseMsg(PARSE_FATAL, pBlockRam->GetLineInfo(), "ht_asym_block_ram write by muliple methods");

			CHtvIdent * pReadMethod = FindUniqueAccessMethod(&pBlockRam->GetHierIdent()->GetReaderList(), bMultiple);

			if (bMultiple)
				ParseMsg(PARSE_FATAL, pBlockRam->GetLineInfo(), "ht_asym_block_ram read by muliple methods");

			if (pReadMethod->GetClockIdent()->GetName() == pWriteMethod->GetClockIdent()->GetName()) {

				if (pBlockRam->GetType()->IsHtBlockRamDoReg()) {
					if (identIter->IsHtMrdBlockRam()) {
						m_bIs1CkDoRegHtMrdBlockRamsPresent = true;
						m_vFile.Print("\n%s_HtBlockMrdRam1CkDoReg #( .DATA_W(%d), .ADDR_W(%d), .SEL_W(%d) ) %s%s ( ",
							inputFileName.c_str(), dataWidth, addrWidth, selWidth, pBlockRam->GetName().c_str(), modDimStr.c_str());
					}
					if (identIter->IsHtMwrBlockRam()) {
						m_bIs1CkDoRegHtMwrBlockRamsPresent = true;
						m_vFile.Print("\n%s_HtBlockMwrRam1CkDoReg #( .DATA_W(%d), .ADDR_W(%d), .SEL_W(%d) ) %s%s ( ",
							inputFileName.c_str(), dataWidth, addrWidth, selWidth, pBlockRam->GetName().c_str(), modDimStr.c_str());
					}
				} else {
					if (identIter->IsHtMrdBlockRam()) {
						m_bIs1CkHtMrdBlockRamsPresent = true;
						m_vFile.Print("\n%s_HtMrdBlockRam1Ck #( .DATA_W(%d), .ADDR_W(%d), .SEL_W(%d) ) %s%s ( ",
							inputFileName.c_str(), dataWidth, addrWidth, selWidth, pBlockRam->GetName().c_str(), modDimStr.c_str());
					}
					if (identIter->IsHtMwrBlockRam()) {
						m_bIs1CkHtMwrBlockRamsPresent = true;
						m_vFile.Print("\n%s_HtMwrBlockRam1Ck #( .DATA_W(%d), .ADDR_W(%d), .SEL_W(%d) ) %s%s ( ",
							inputFileName.c_str(), dataWidth, addrWidth, selWidth, pBlockRam->GetName().c_str(), modDimStr.c_str());
					}
				}

				m_vFile.Print(".ck( %s ), ", pWriteMethod->GetClockIdent()->GetName().c_str());
			} else {
				if (pBlockRam->GetType()->IsHtBlockRamDoReg()) {
					if (identIter->IsHtMrdBlockRam()) {
						m_bIs2CkDoRegHtMrdBlockRamsPresent = true;
						m_vFile.Print("\n%s_HtMrdBlockRam2CkDoReg #( .DATA_W(%d), .ADDR_W(%d), .SEL_W(%d) ) %s%s ( ",
								inputFileName.c_str(), dataWidth, addrWidth, selWidth, pBlockRam->GetName().c_str(), modDimStr.c_str());
					}
					if (identIter->IsHtMwrBlockRam()) {
						m_bIs2CkDoRegHtMwrBlockRamsPresent = true;
						m_vFile.Print("\n%s_HtMwrBlockRam2CkDoReg #( .DATA_W(%d), .ADDR_W(%d), .SEL_W(%d) ) %s%s ( ",
								inputFileName.c_str(), dataWidth, addrWidth, selWidth, pBlockRam->GetName().c_str(), modDimStr.c_str());
					}
				} else {
					if (identIter->IsHtMrdBlockRam()) {
						m_bIs2CkHtMrdBlockRamsPresent = true;
						m_vFile.Print("\n%s_HtMrdBlockRam2Ck #( .DATA_W(%d), .ADDR_W(%d), .SEL_W(%d) ) %s%s ( ",
								inputFileName.c_str(), dataWidth, addrWidth, selWidth, pBlockRam->GetName().c_str(), modDimStr.c_str());
					}
					if (identIter->IsHtMwrBlockRam()) {
						m_bIs2CkHtMwrBlockRamsPresent = true;
						m_vFile.Print("\n%s_HtMwrBlockRam2Ck #( .DATA_W(%d), .ADDR_W(%d), .SEL_W(%d) ) %s%s ( ",
								inputFileName.c_str(), dataWidth, addrWidth, selWidth, pBlockRam->GetName().c_str(), modDimStr.c_str());
					}
				}
				m_vFile.Print(".rck( %s ), ", pReadMethod->GetClockIdent()->GetName().c_str());
				m_vFile.Print(".wck( %s ), ", pWriteMethod->GetClockIdent()->GetName().c_str());
			}

			m_vFile.Print(".we( %s_WrEn%s ), ", cQueName.c_str(), refDimStr.c_str());
			if (identIter->IsHtMrdBlockRam())
				m_vFile.Print(".ws( %s_WrSel%s ), ", cQueName.c_str(), refDimStr.c_str());
			m_vFile.Print(".wa( %s_WrAddr%s ), ", cQueName.c_str(), refDimStr.c_str());
			m_vFile.Print(".wd( %s_WrData%s ), ", cQueName.c_str(), refDimStr.c_str());
			if (identIter->IsHtMwrBlockRam())
				m_vFile.Print(".rs( %s_RdSel%s ), ", cQueName.c_str(), refDimStr.c_str());
			m_vFile.Print(".ra( %s_RdAddr%s ), ", cQueName.c_str(), refDimStr.c_str());
			m_vFile.Print(".rd( %s_RdData%s ) );\n", cQueName.c_str(), refDimStr.c_str());

		} while (!pBlockRam->DimenIter(refList));
	}
}

void CHtvDesign::SynHtDistRamModule(bool bQueue)
{
	if (!bQueue && !m_bIsHtDistRamsPresent ||
		bQueue && !m_bIsHtQueueRamsPresent)
		return;

	string inputFileName = g_htvArgs.GetInputFileName();

	// The following module is used to infer block rams
	//   Originally written by Mike Ruff (tpmem.v)
	
	if (g_htvArgs.IsKeepHierarchyEnabled())
        m_vFile.Print("\n(* keep_hierarchy = \"true\" *)\n");
    else
        m_vFile.Print("\n");

	m_vFile.Print("module %s_Ht%sRam #(parameter\n", inputFileName.c_str(), bQueue ? "Queue" : "Dist");
	m_vFile.IncIndentLevel();
	m_vFile.Print("DATA_WIDTH = 32,\n");
	m_vFile.Print("ADDR_WIDTH = 5\n");
	m_vFile.DecIndentLevel();
	m_vFile.Print(") (\n");
	m_vFile.IncIndentLevel();
	m_vFile.Print("input                    clk,\n");
	m_vFile.Print("input                    we,\n");
	m_vFile.Print("input  [ADDR_WIDTH-1:0]  wa,\n");
	m_vFile.Print("input  [DATA_WIDTH-1:0]  din,\n");
	m_vFile.Print("input  [ADDR_WIDTH-1:0]  ra,\n");
	m_vFile.Print("output [DATA_WIDTH-1:0]  dout\n");
	m_vFile.DecIndentLevel();
	m_vFile.Print(");\n");
	m_vFile.IncIndentLevel();

	if (g_htvArgs.IsAlteraDistRams()) {
		m_vFile.Print("wire[DATA_WIDTH - 1:0] sub_wire0;\n");
		m_vFile.Print("assign dout = sub_wire0[DATA_WIDTH - 1:0];\n");
		m_vFile.Print("\n");

		m_vFile.Print("altdpram  altdpram_component(\n");
		m_vFile.IncIndentLevel();
		m_vFile.Print(".data(din),\n");
		m_vFile.Print(".inclock(clk),\n");
		m_vFile.Print(".outclock(clk),\n");
		m_vFile.Print(".rdaddress(ra),\n");
		m_vFile.Print(".wraddress(wa),\n");
		m_vFile.Print(".wren(we),\n");
		m_vFile.Print(".q(sub_wire0),\n");
		m_vFile.Print(".aclr(1'b0),\n");
		m_vFile.Print(".byteena(1'b1),\n");
		m_vFile.Print(".inclocken(1'b1),\n");
		m_vFile.Print(".outclocken(1'b1),\n");
		m_vFile.Print(".rdaddressstall(1'b0),\n");
		m_vFile.Print(".rden(1'b1),\n");
		m_vFile.Print(".wraddressstall(1'b0));\n");
		m_vFile.Print("defparam\n");
		m_vFile.Print("altdpram_component.intended_device_family = \"Arria 10\",\n");
		m_vFile.Print("altdpram_component.lpm_type = \"altdpram\",\n");
		m_vFile.Print("altdpram_component.ram_block_type = \"MLAB\",\n");
		m_vFile.Print("altdpram_component.outdata_aclr = \"OFF\",\n");
		m_vFile.Print("altdpram_component.outdata_reg = \"UNREGISTERED\",\n");
		m_vFile.Print("altdpram_component.rdaddress_aclr = \"OFF\",\n");
		m_vFile.Print("altdpram_component.rdaddress_reg = \"UNREGISTERED\",\n");
		m_vFile.Print("altdpram_component.rdcontrol_aclr = \"OFF\",\n");
		m_vFile.Print("altdpram_component.rdcontrol_reg = \"UNREGISTERED\",\n");
		m_vFile.Print("altdpram_component.read_during_write_mode_mixed_ports = \"NEW_DATA\",\n");
		m_vFile.Print("altdpram_component.width = DATA_WIDTH,\n");
		m_vFile.Print("altdpram_component.widthad = ADDR_WIDTH,\n");
		m_vFile.Print("altdpram_component.width_byteena = 1,\n");
		m_vFile.Print("altdpram_component.indata_aclr = \"OFF\",\n");
		m_vFile.Print("altdpram_component.indata_reg = \"INCLOCK\",\n");
		m_vFile.Print("altdpram_component.wraddress_aclr = \"OFF\",\n");
		m_vFile.Print("altdpram_component.wraddress_reg = \"INCLOCK\",\n");
		m_vFile.Print("altdpram_component.wrcontrol_aclr = \"OFF\",\n");
		m_vFile.Print("altdpram_component.wrcontrol_reg = \"INCLOCK\";\n");
		m_vFile.DecIndentLevel();

	} else {

		m_vFile.Print("\n(* ram_style = \"distributed\" *)\n");
		m_vFile.Print("reg [DATA_WIDTH-1:0] mem[2**ADDR_WIDTH-1:0];\n");

		m_vFile.Print("always @(posedge clk)\n");
		m_vFile.IncIndentLevel();
		m_vFile.Print("begin\n");
		m_vFile.IncIndentLevel();
		m_vFile.Print("if (we)\n");
		m_vFile.IncIndentLevel();
		m_vFile.Print("mem[wa] <= din;\n");
		m_vFile.DecIndentLevel();
		m_vFile.DecIndentLevel();
		m_vFile.Print("end\n");
		m_vFile.DecIndentLevel();

		// Output pipe stage
		m_vFile.Print("assign dout = mem[ra];\n");
	}

	m_vFile.DecIndentLevel();
	m_vFile.Print("endmodule\n");
	m_vFile.Print(" ");
}

void CHtvDesign::SynHtBlockRamModule()
{
	for (int mode = 0; mode < 4; mode += 1) {
		int clkCnt = 0;
		bool bDoReg = false;

		switch (mode) {
		case 0:
			if (!m_bIs1CkHtBlockRamsPresent) continue;
			clkCnt = 1;
			bDoReg = false;
			break;
		case 1:
			if (!m_bIs2CkHtBlockRamsPresent) continue;
			clkCnt = 2;
			bDoReg = false;
			break;
		case 2:
			if (!m_bIs1CkDoRegHtBlockRamsPresent) continue;
			clkCnt = 1;
			bDoReg = true;
			break;
		case 3:
			if (!m_bIs2CkDoRegHtBlockRamsPresent) continue;
			clkCnt = 2;
			bDoReg = true;
			break;
		}

		// The following module is used to infer block rams
		//   Originally written by Mike Ruff (tpmem.v)
	
		string inputFileName = g_htvArgs.GetInputFileName();

		if (g_htvArgs.IsKeepHierarchyEnabled())
            m_vFile.Print("\n(* keep_hierarchy = \"true\" *)\n");
        else
            m_vFile.Print("\n");

		m_vFile.Print("module %s_HtBlockRam%s%s #(parameter\n", inputFileName.c_str(),
			clkCnt == 1 ? "1Ck" : "2Ck", bDoReg ? "DoReg" : "");
		m_vFile.IncIndentLevel();
		m_vFile.Print("DATA_WIDTH = 32,\n");
		m_vFile.Print("ADDR_WIDTH = 9\n");
		m_vFile.DecIndentLevel();
		m_vFile.Print(") (\n");
		m_vFile.IncIndentLevel();
		m_vFile.Print("input                    clk,\n");
		m_vFile.Print("input                    we,\n");
		m_vFile.Print("input  [ADDR_WIDTH-1:0]  wa,\n");
		m_vFile.Print("input  [DATA_WIDTH-1:0]  din,\n");
		if (clkCnt == 2)
			m_vFile.Print("input                    rclk,\n");
		m_vFile.Print("input  [ADDR_WIDTH-1:0]  ra,\n");
		m_vFile.Print("output [DATA_WIDTH-1:0]  dout\n");
		m_vFile.DecIndentLevel();
		m_vFile.Print(");\n");
		m_vFile.IncIndentLevel();

		if (g_htvArgs.IsQuartusEnabled())
			m_vFile.Print("\n(* ramstyle = \"M20K\" *)\n");
		else
			m_vFile.Print("\n(* ram_style = \"block\" *)\n");
		m_vFile.Print("reg [DATA_WIDTH-1:0] mem[2**ADDR_WIDTH-1:0];\n");

		m_vFile.Print("always @(posedge clk)\n");
		m_vFile.IncIndentLevel();
		m_vFile.Print("begin\n");
		m_vFile.IncIndentLevel();
		m_vFile.Print("if (we)\n");
		m_vFile.IncIndentLevel();
		m_vFile.Print("mem[wa] <= din;\n");
		m_vFile.DecIndentLevel();
		m_vFile.DecIndentLevel();
		m_vFile.Print("end\n");
		m_vFile.DecIndentLevel();

		m_vFile.Print("\nreg [DATA_WIDTH-1:0] ram_dout;\n");
		if (clkCnt == 2)
			m_vFile.Print("always @(posedge rclk)\n");
		else
			m_vFile.Print("always @(posedge clk)\n");
		m_vFile.IncIndentLevel();
		m_vFile.Print("begin\n");
		m_vFile.IncIndentLevel();
		m_vFile.Print("ram_dout <=\t// synthesis translate_off\n");
		m_vFile.Print("\t\t(we && wa == ra) ? {(DATA_WIDTH+11)/12{12'hbad}} :\n");
		m_vFile.Print("\t\t// synthesis translate_on\n");
		m_vFile.Print("\t\tmem[ra];\n");
		m_vFile.DecIndentLevel();
		m_vFile.Print("end\n");
		m_vFile.DecIndentLevel();

		if (bDoReg) {
			m_vFile.Print("\nreg [DATA_WIDTH-1:0] r_dout;\n");
			if (clkCnt == 2)
				m_vFile.Print("always @(posedge rclk)\n");
			else
				m_vFile.Print("always @(posedge clk)\n");
			m_vFile.IncIndentLevel();
			m_vFile.Print("begin\n");
			m_vFile.IncIndentLevel();
			m_vFile.Print("r_dout <= ram_dout;\n");
			m_vFile.DecIndentLevel();
			m_vFile.Print("end\n");
			m_vFile.DecIndentLevel();
			m_vFile.Print("assign dout = r_dout;\n");
		} else {
			m_vFile.Print("assign dout = ram_dout;\n");
		}

		m_vFile.DecIndentLevel();

		m_vFile.Print("endmodule\n");
		m_vFile.Print(" ");
	}
}

void CHtvDesign::SynHtAsymBlockRamModule()
{
	for (int mode = 0; mode < 8; mode += 1) {
		int clkCnt = 0;
		bool bDoReg = false;
		bool bMwr = false;

		switch (mode) {
		case 0:
			if (!m_bIs1CkHtMwrBlockRamsPresent) continue;
			clkCnt = 1;
			bDoReg = false;
			bMwr = true;
			break;
		case 1:
			if (!m_bIs2CkHtMwrBlockRamsPresent) continue;
			clkCnt = 2;
			bDoReg = false;
			bMwr = true;
			break;
		case 2:
			if (!m_bIs1CkDoRegHtMwrBlockRamsPresent) continue;
			clkCnt = 1;
			bDoReg = true;
			bMwr = true;
			break;
		case 3:
			if (!m_bIs2CkDoRegHtMwrBlockRamsPresent) continue;
			clkCnt = 2;
			bDoReg = true;
			bMwr = true;
			break;
		case 4:
			if (!m_bIs1CkHtMrdBlockRamsPresent) continue;
			clkCnt = 1;
			bDoReg = false;
			bMwr = false;
			break;
		case 5:
			if (!m_bIs2CkHtMrdBlockRamsPresent) continue;
			clkCnt = 2;
			bDoReg = false;
			bMwr = false;
			break;
		case 6:
			if (!m_bIs1CkDoRegHtMrdBlockRamsPresent) continue;
			clkCnt = 1;
			bDoReg = true;
			bMwr = false;
			break;
		case 7:
			if (!m_bIs2CkDoRegHtMrdBlockRamsPresent) continue;
			clkCnt = 2;
			bDoReg = true;
			bMwr = false;
			break;
		}

		// The following module is used to infer block rams
		//   Originally written by Mike Ruff (tpmem.v)
	
		string inputFileName = g_htvArgs.GetInputFileName();

		if (g_htvArgs.IsKeepHierarchyEnabled())
            m_vFile.Print("\n(* keep_hierarchy = \"true\" *)\n");
        else
            m_vFile.Print("\n");

		m_vFile.Print("module %s_Ht%sBlockRam%s%s #(parameter\n", inputFileName.c_str(),
			bMwr ? "Mwr" : "Mrd", clkCnt == 1 ? "1Ck" : "2Ck", bDoReg ? "DoReg" : "");
		m_vFile.IncIndentLevel();
		m_vFile.Print("DATA_W = 32,\n");
		m_vFile.Print("ADDR_W = 9,\n");
		m_vFile.Print("SEL_W = 1\n");
		m_vFile.DecIndentLevel();
		m_vFile.Print(") (\n");
		m_vFile.IncIndentLevel();

		if (clkCnt == 2)
			m_vFile.Print("input wck,\n");
		else
			m_vFile.Print("input ck,\n");

		m_vFile.Print("input we,\n");

		if (!bMwr)
			m_vFile.Print("input [SEL_W-1:0] ws,\n");

		m_vFile.Print("input [ADDR_W-1:0] wa,\n");

		if (bMwr)
			m_vFile.Print("input [(2**SEL_W)*DATA_W-1:0] wd,\n");
		else
			m_vFile.Print("input [DATA_W-1:0] wd,\n");

		if (clkCnt == 2)
			m_vFile.Print("input rck,\n");

		if (bMwr)
			m_vFile.Print("input [SEL_W-1:0] rs,\n");

		m_vFile.Print("input [ADDR_W-1:0] ra,\n");

		if (!bMwr)
			m_vFile.Print("output [(2**SEL_W)*DATA_W-1:0] rd\n");
		else
			m_vFile.Print("output [DATA_W-1:0] rd\n");

		m_vFile.DecIndentLevel();
		m_vFile.Print(");\n");
		m_vFile.IncIndentLevel();

		if (g_htvArgs.IsQuartusEnabled())
			m_vFile.Print("\n(* ramstyle = \"M20K\" *)\n");
		else
			m_vFile.Print("\n(* ram_style = \"block\" *)\n");
		m_vFile.Print("reg [DATA_W-1:0] mem[2**(ADDR_W+SEL_W)-1:0];\n");

		m_vFile.Print("always @(posedge ck)\n");
		m_vFile.IncIndentLevel();
		m_vFile.Print("begin : ramwrite\n");
		m_vFile.IncIndentLevel();

		if (bMwr) {
			m_vFile.Print("integer i;\n");
			m_vFile.Print("reg [SEL_W-1:0] ws;\n");
			m_vFile.Print("for (i=0; i<2**SEL_W; i=i+1) begin\n");
			m_vFile.IncIndentLevel();
			m_vFile.Print("ws = i;\n");
			m_vFile.Print("if (we)\n");
			m_vFile.IncIndentLevel();
			m_vFile.Print("mem[{wa,ws}] <= wd[(i+1)*DATA_W-1 -: DATA_W];\n");
			m_vFile.DecIndentLevel();
			m_vFile.DecIndentLevel();
			m_vFile.Print("end\n");
		} else {
			m_vFile.Print("if (we)\n");
			m_vFile.IncIndentLevel();
			m_vFile.Print("mem[{wa,ws}] <= wd;\n");
			m_vFile.DecIndentLevel();
		}
		m_vFile.DecIndentLevel();
		m_vFile.Print("end\n");
		m_vFile.DecIndentLevel();

		if (bMwr)
			m_vFile.Print("\nreg [DATA_W-1:0] ram_dout;\n");
		else
			m_vFile.Print("\nreg [(2**SEL_W)*DATA_W-1:0] ram_dout;\n");

		if (clkCnt == 2)
			m_vFile.Print("always @(posedge rck)\n");
		else
			m_vFile.Print("always @(posedge ck)\n");
		m_vFile.IncIndentLevel();
		m_vFile.Print("begin : ramread\n");
		m_vFile.IncIndentLevel();

		if (!bMwr) {
			m_vFile.Print("integer i;\n");
			m_vFile.Print("reg [SEL_W-1:0] rs;\n");
			m_vFile.Print("for (i=0; i<2**SEL_W; i=i+1) begin\n");
			m_vFile.IncIndentLevel();
			m_vFile.Print("rs = i;\n");
			m_vFile.Print("ram_dout[(i+1)*DATA_W-1 -: DATA_W] <=\n");
		} else
			m_vFile.Print("ram_dout <=\n");

		m_vFile.IncIndentLevel();
		m_vFile.Print("// synthesis translate_off\n");
		m_vFile.Print("(we != 0 && wa == ra) ? {(DATA_W+11)/12{12'hbad}} :\n");
		m_vFile.Print("// synthesis translate_on\n");
		m_vFile.Print("mem[{ra,rs}];\n");
		m_vFile.DecIndentLevel();

		if (!bMwr)
			m_vFile.Print("end\n");

		m_vFile.DecIndentLevel();
		m_vFile.Print("end\n");
		m_vFile.DecIndentLevel();

		if (bDoReg) {
			m_vFile.Print("\nreg [DATA_W-1:0] r_rd;\n");
			if (clkCnt == 2)
				m_vFile.Print("always @(posedge rck)\n");
			else
				m_vFile.Print("always @(posedge ck)\n");
			m_vFile.IncIndentLevel();
			m_vFile.Print("begin\n");
			m_vFile.IncIndentLevel();
			m_vFile.Print("r_rd <= ram_dout;\n");
			m_vFile.DecIndentLevel();
			m_vFile.Print("end\n");
			m_vFile.DecIndentLevel();
			m_vFile.Print("assign rd = r_rd;\n");
		} else {
			m_vFile.Print("assign rd = ram_dout;\n");
		}

		m_vFile.DecIndentLevel();

		m_vFile.Print("endmodule\n");
		m_vFile.Print(" ");
	}
}

void CHtvDesign::SynHtDistRamWe(CHtvIdent *pHier, CHtvObject * pObj, CHtvObject * pRtnObj, CHtvOperand *pExpr, bool bIsAlwaysAt)
{
	// Synthesize a write enable assignment statement
	//   syntax is: variable = value
	//   write variable as single bit wide value per dist ram section

	if (!bIsAlwaysAt)
		m_vFile.Print("assign ");

	vector<CHtfeOperand *> &weIndexList = pExpr->GetOperand1()->GetIndexList();

	CHtvIdent *pWeIdent = pExpr->GetOperand1()->GetMember();
	vector<CHtDistRamWeWidth> *pWeWidth = pWeIdent->GetHtDistRamWeWidth();

	CHtvOperand *pOp2 = pExpr->GetOperand2();
	Assert(pOp2->IsConstValue());

	if ( (*pWeWidth).size() == 1) {

		bool bFoundSubExpr;
		bool bWriteIndex;
		FindSubExpr(pObj, pRtnObj, pExpr, bFoundSubExpr, bWriteIndex);

		if (!bWriteIndex) {
			PrintSubExpr(pObj, pRtnObj, pExpr);
			m_vFile.Print(";\n");
		}

		if (bFoundSubExpr) {
			m_vFile.DecIndentLevel();
			m_vFile.Print("end\n");
		}

	} else {

		int exprHighBit, exprLowBit;
		pExpr->GetOperand1()->GetDistRamWeWidth(exprHighBit, exprLowBit);

		for (size_t i = 0; i < (*pWeWidth).size(); i += 1) {
			int highBit = (*pWeWidth)[i].m_highBit;
			int lowBit = (*pWeWidth)[i].m_lowBit;

			if (!(exprLowBit < 0) && !(exprLowBit <= lowBit && highBit <= exprHighBit))
				continue;

			m_vFile.Print("%s_%d_%d", pWeIdent->GetName().c_str(), highBit, lowBit);

			for (size_t i = 0; i < weIndexList.size(); i += 1) {
				if (!weIndexList[i]->IsConstValue())
					ParseMsg(PARSE_FATAL, weIndexList[i]->GetLineInfo(), "Non-constant index not supported");
				m_vFile.Print("$%d", weIndexList[i]->GetConstValue().GetSint32());
			}

			m_vFile.Print(" = ");

			if (pOp2->IsScX())
				m_vFile.Print("1'bx;\n");
			else if (pOp2->GetConstValue().GetUint64() == 0)
				m_vFile.Print("1'b0;\n");
			else
				m_vFile.Print("1'b1;\n");
		}
	}
}
