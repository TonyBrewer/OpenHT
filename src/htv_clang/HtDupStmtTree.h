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

	class HtStmtIter;

	class HtDupStmtTree {
	public:
		// called to duplicate a statement tree (including expressions)
		HtStmt * dupStmtTree( HtStmtIter & iSrcStmt, void * pVoid );

		// callback called just prior to duplicating a node
		//  a return value of false causes duplicating to stop (without duplicating source node)
		//  ppDstStmt is where pointer to duplicated node is located
		//  pVoid is dupStmtTree pVoid value
		virtual HtStmt * dupCallback( HtStmtIter & iSrcStmt, void * pVoid ) { return 0; }

	private:

	};

}
