/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#ifndef HTC_NORMALIZE_H
#define HTC_NORMALIZE_H

#include <rose.h>


extern void NormalizeSyntax(SgProject *project);
extern void ExpandConditionalExpressions(SgProject *project);
extern void ExpandSgArrowExpressions(SgProject *project);
extern void ExpandCompoundAssignOperators(SgProject *project);
extern void FlattenAssignChains(SgProject *project);
extern void AddCastsForRhsOfAssignments(SgProject *project);
extern void FlattenArrayIndexExpressions(SgProject *project);
extern void FlattenSideEffectsInStatements(SgProject *project);
extern void FlattenFunctionCalls(SgProject *project);
extern void FlattenCommaExpressions(SgProject *project);
extern void LiftDeclarations(SgProject *project);
extern void StripPreprocessingInfo(SgProject *project);

extern void RemoveEmptyElseBlocks(SgProject *project);
extern void PreservePreprocInfoForBlocks(SgProject *project);

#endif //HTC_NORMALIZE_H

