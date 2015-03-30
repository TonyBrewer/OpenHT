/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#pragma once

namespace ht {

	class HtPathDst {
		HtStmtIter m_iStmt;
		int m_listIdx;
	public:
		HtPathDst() : m_listIdx(-1) {}
		HtPathDst(HtStmtIter & iStmt, int listIdx) : m_iStmt(iStmt), m_listIdx(listIdx) {}

		HtStmtIter & getStmtIter() { return m_iStmt; }
		int getListIdx() { return m_listIdx; }
	};

	class HtPath : public HtNode {
		vector<HtStmtIter>	m_srcList;
		vector<HtPathDst>	m_dstList;

	public:
		HtPath(HtLineInfo & lineInfo) : HtNode(lineInfo) {}

		void addSrc(HtStmtIter & iStmt) { m_srcList.push_back(iStmt); }
		void addDst(HtPathDst & pathDst) { m_dstList.push_back(pathDst); }
	};

}
