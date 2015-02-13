/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#ifndef HTC_HT_DECL_MANAGER_H
#define HTC_HT_DECL_MANAGER_H

#include <rose.h>

namespace HtDeclMgr {

void buildAllKnownHtlDecls(SgProject *project);
SgVariableDeclaration *buildHtlVarDecl(std::string name, SgScopeStatement *GS);
SgVariableDeclaration *buildHtlVarDecl_generic(std::string name, SgType *vtype, SgScopeStatement *GS);
SgFunctionDeclaration *buildHtlFuncDecl(std::string name, SgScopeStatement *GS);
SgFunctionDeclaration *buildHtlFuncDecl_generic(std::string name, char rtype,
                                     std::string atypes, SgScopeStatement *GS);
SgFunctionDeclaration *buildHtlFuncDecl_generic(std::string name,
                                              SgType *sgtype,
                                              SgFunctionParameterList *sgplist,
                                              SgScopeStatement *GS);

}

#endif //HTC_DECL_MANAGER_H

