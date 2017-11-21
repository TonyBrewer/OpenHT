XILINX_THREADS = 2
CNY_PDK_PERS_DIR = --exclude ht.released ../..

####################################################################
# Include Convey Makefile Template
####################################################################
include ../config.pdk

SILENT ?= .SILENT   # Allow Override of Silent

$(SILENT):

ifneq (,$(findstring wx2-vu7p, $(CNY_PDK_PLATFORM)))
    PLACE_FILES += $(wildcard compvu7p.xdc)
else ifneq (,$(findstring wx-690, $(CNY_PDK_PLATFORM)))
    PLACE_FILES += $(wildcard comp690.xdc)
else ifneq (,$(findstring wx-2000, $(CNY_PDK_PLATFORM)))
    PLACE_FILES += $(wildcard comp2000.xdc)
else ifneq (,$(findstring ex, $(CNY_PDK_PLATFORM)))
    UCF_FILES += $(wildcard comp760.ucf)
else
    XST_TEMPLATE=xst.template
    UCF_FILES += $(wildcard comp330.ucf)
endif

XILINX_MAP_OPTIONS += -logic_opt on
