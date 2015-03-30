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
	class HtDecl;

	class HtWalkStmtTree {
	public:
		// called to walk a statement tree (including expressions)
		void walkStmtTree( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid=0 );

		// callback called just prior to walking a node
		//  a return value of false causes walking stop decending
		//  isLoa - is left of assignment
		//  pVoid is walkStmtTree pVoid value
		virtual bool walkCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid ) = 0;
		virtual void walkPostCallback( HtStmtIter & iStmt, HtStmtIter & iInsert, bool isLoa, void * pVoid ) = 0;

		bool isParamTypeLoa(HtQualType qualType);
		bool isParamTypeConst(HtQualType qualType);
	};

}
