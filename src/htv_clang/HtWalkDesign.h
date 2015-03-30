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

	class HtDesign;

	class HtWalkDesign {
	public:

		void walkDesign(HtDesign * pHtDesign);
		void walkDecl(HtDecl ** ppHtDecl);
		void walkStmt(HtStmt ** ppHtStmt);
		void walkExpr(HtExpr ** ppHtExpr);

		virtual void handleSwitchStmt(HtStmt ** ppStmt) {}
		virtual void handleTopDecl(HtDecl ** ppDecl) {}
		virtual void handleCXXRecordDecl(HtDecl ** ppDecl) {}
		virtual void handleCXXMethodDecl(HtDecl ** ppDecl) {}
		virtual void handleFunctionDecl(HtDecl ** ppDecl) {}
		virtual void handleDeclStmt(HtStmt ** ppStmt) {}
		virtual void handleStmt(HtStmt ** ppStmt) {}

	private:
	};
}
