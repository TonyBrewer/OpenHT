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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <string>
#include <queue>
#include <list>
#include <map>
#include <errno.h>

#ifdef WIN32
#include <crtdbg.h>
#endif

#if defined(HT_SYSC) || defined(_HTV) || defined(HT_LIB_SYSC)
	#include <systemc.h>
# if !defined(_HTV)
	#include "sysc/HtStrFmt.h"
# endif
#endif

#if !defined(HT_LIB_HIF) && !defined(HT_LIB_SYSC)
#include "HostIntf.h"
#endif

#include "host/HtDefines.h"
#include "host/HtPlatform.h"

#if defined(HT_SYSC) || defined(_HTV)
	#include "sysc/Params.h"
#endif

#if !defined(_HTV)
	using namespace std;
	#include "host/HtCtrlMsg.h"
	#include "host/HtHif.h"
	#include "host/HtModel.h"
	#include "sysc/mtrand.h"
#endif

#if defined(HT_SYSC) || defined(_HTV) || defined(HT_LIB_SYSC)
	#include "sysc/HtInt.h"
# if !defined(_HTV)
	#include "sysc/MemMon.h"
# endif
	#include "sysc/HtMemTypes.h"
	#include "sysc/MemRdWrIntf.h"
	#include "sysc/PersXbarStub.h"
	#include "sysc/PersMiStub.h"
	#include "sysc/PersMoStub.h"
	#include "sysc/PersUnitCnt.h"
	#include "sysc/SyscClock.h"
	#include "sysc/SyscDisp.h"
	#include "sysc/SyscMem.h"
#endif

#if !defined(HT_LIB_HIF) && !defined(HT_LIB_SYSC)
#include "UnitIntf.h"
#endif
