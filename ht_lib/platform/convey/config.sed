#
# Copyright 2013-2014 Convey Computer Corp. 
#

# Platforms:
#	hc-1, hc-1ex, hc-2, hc-2ex, wx-690, wx-2000
CNY_PDK_PLATFORM = CNY_PDK_PLATFORM

# Personality frequency:
#	SYNC_CLK, SYNC_CLK_HALF, 50, 75, ..., 250
CLK_PERS_FREQ = CLK_PERS_FREQ

# Extra verilog directories
USER_VERILOG_DIRS += HT_LIB/platform/OHT_PLAT/verilog ../../src_pers

####################################################################
# Include Convey Makefile Template
####################################################################
include $(CNY_PDK)/$(CNY_PDK_REV)/$(CNY_PDK_PLATFORM)/lib/MakefileInclude.cnypdk
