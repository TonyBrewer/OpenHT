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

	class HtStmt;
	class HtExpr;
	class HtDecl;

	class HtNode : public HtLineInfo {
	public:
		HtNode(HtLineInfo & lineInfo) : HtLineInfo(lineInfo) {
			Init();
		}

		HtNode( HtNode const * pNode ) : HtLineInfo(*(HtLineInfo*)pNode) {
			Init();
		}

		HtNode( SourceRange & range ) : HtLineInfo( range ) {
			Init();
		}

#define STOP_ID 0x14c
		void Init() {
			m_id = m_nextId++;
			m_nodes[m_id].m_pNode = this;
			if (m_id == STOP_ID)
				bool stop = true;
		}

		int getNodeId() { return m_id; }

		HtLineInfo & getLineInfo() { return *(HtLineInfo*)this; }

	private:
		int m_id;

	private:
		union NodePtr {
			HtStmt * m_pStmt;
			HtExpr * m_pExpr;
			HtDecl * m_pDecl;
			HtNode * m_pNode;
		};

		static int m_nextId;
	public:
		static NodePtr m_nodes[50000];
	};
}
