/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#ifndef HTC_PROCESS_STENCILS_H
#define HTC_PROCESS_STENCILS_H

#include <rose.h>

//
// Process rhomp_stencil intrinsic calls.
//

extern void ProcessStencils(SgProject *);

#define StencilPrefix       "__STENCIL_OUT__"
#define StencilStreamPrefix "__STENCIL_OUT__STB"


#endif //HTC_PROCESS_STENCILS_H

