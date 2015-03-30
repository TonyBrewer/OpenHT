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

	class HtPrintDesign
	{
	public:
		HtPrintDesign(HtDesign * pHtDesign, ASTContext &Context) 
			: m_pHtDesign(pHtDesign), m_context(Context), m_diags(Context.getDiagnostics())
		{
		}

		void printDesign();
		void printDecl(string indent, HtDecl * pHtDecl);
		void printStmt(string indent, HtStmtIter & iStmt);
		void printStmt(string indent, HtStmtList & stmtList);
		void printExpr(HtStmtIter & iExpr, bool needParanIfNotLValue=false);
		void printExprList(HtStmtList & stmt, bool needParanIfNotLValue=false);
		void printQualType(HtQualType & qualType, string identStr="");
		void printQualType(HtType::HtPrintType printFlag, HtQualType & qualType);
		void printTemplateArg(HtTemplateArgument * pHtTemplateArg);
		void printInitDecl(string indent, HtDecl * pHtDecl);

	private:
		HtDesign * m_pHtDesign;
		ASTContext & m_context;
		DiagnosticsEngine & m_diags;
	};
};
