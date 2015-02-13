/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#ifndef HTC_ISOLATE_MODULES_H
#define HTC_ISOLATE_MODULES_H

#include <rose.h>
#include <string>
#include <set>


extern void IsolateModulesToFiles(SgProject *project);
extern std::set<std::string> LockFunctionsInUse;

#endif // HTC_ISOLATE_MODULES_H

