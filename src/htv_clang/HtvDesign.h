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

#include "HtDesign.h"

using namespace ht;

namespace htv {

	class HtvDesign : public HtDesign {
	public:

		//HtStmt * handleNewHtStmt(clang::Stmt const * pStmt) { return new HtStmt(pStmt); }
		//HtExpr * handleNewHtExpr(clang::Expr const * pExpr) { return new HtExpr(pExpr); }
		//HtType * handleNewHtType(clang::Type const * pType) { return new HtType(pType); }
		//HtDecl * handleNewHtDecl(clang::Decl const * pDecl) { return new HtDecl(pDecl); }
		//HtTemplateArgument * handleNewHtTemplateArgument( TemplateArgument const * pArg ) { return new HtTemplateArgument(pArg); }
		//HtBaseSpecifier * handleNewHtBaseSpecifier( CXXBaseSpecifier const * pBase ) { return new HtBaseSpecifier(pBase); }
	
	};
}
