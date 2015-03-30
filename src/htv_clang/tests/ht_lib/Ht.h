#pragma once

#define assert(x)

#include "HtIntTypes.h"
#include "HtMemTypes.h"
#include "HtSyscTypes.h"

#include "HostIntf.h"

#define SC_X 0
#define ht_topology_top
//#define ht_prim
#define ht_noload
#define ht_state
#define ht_local
//#define ht_clk(...)
#define ht_attrib(name,inst,value)

ht_prim ht_clk("clk") inline void ResetFDSE (bool &r_reset, const bool &i_reset) { r_reset = i_reset; }
