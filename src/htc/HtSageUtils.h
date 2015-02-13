/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#ifndef HTC_HT_SAGE_UTILS_H
#define HTC_HT_SAGE_UTILS_H

// macro to control whether TraversInputFiles traverses strictly 
// the main input file or uses the old Rose traversal that also
// includes some code in header files
#define STRICT_INPUT_FILE_TRAVERSAL false

#include <rose.h>

namespace HtSageUtils {

extern bool isHtlReservedIdent(std::string name);

extern SgLabelStatement *makeLabelStatement(SgName baseName, int num,
                                            SgScopeStatement *scope);
extern SgLabelStatement *makeLabelStatement(SgName baseName,
                                            SgScopeStatement *scope);
extern SgStatement *findSafeInsertPoint(SgNode *node);

extern std::pair<bool, std::string> getFileName(SgNode *n);

extern void ScreenForHtcRestrictions(SgProject *project);

extern void DeclareFunctionInScope(std::string name, 
                                   SgType *type,
                                   SgScopeStatement *scope);

extern SgExpression *SkipCasting(SgExpression *exp);

}

#endif //HTC_HT_SAGE_UTILS_H

