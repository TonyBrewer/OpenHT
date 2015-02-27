export CNY_PDK		= /work/pdk_releases
export CNY_PDK_REV	= latest
export CNY_PDK_PLATFORM	= hc-1ex

export CNY_HT_LIBEXCLUDE = Main.cpp

#export CNY_HT_SIG	= 65000
export CNY_HT_NICK	= bcm

PLAT_hc-1_AEUCNT = 1
PLAT_hc-2_AEUCNT = $(PLAT_hc-1_AEUCNT)
PLAT_hc-1ex_AEUCNT = 1
PLAT_hc-2ex_AEUCNT = $(PLAT_hc-1ex_AEUCNT)
PLAT_wx-2000_AEUCNT = 4
PLAT_wx-690_AEUCNT = 7
pers: export CNY_HT_AEUCNT = $(PLAT_${CNY_PDK_PLATFORM}_AEUCNT)

PLAT_hc-1_FREQ = 75
PLAT_hc-2_FREQ = $(PLAT_hc-1_FREQ)
PLAT_hc-1ex_FREQ = 75
PLAT_hc-2ex_FREQ = $(PLAT_hc-1ex_FREQ)
PLAT_wx-2000_FREQ = 150
PLAT_wx-690_FREQ = 150
export CNY_HT_FREQ = $(PLAT_${CNY_PDK_PLATFORM}_FREQ)

ifneq (,$(findstring wx,$(CNY_PDK_PLATFORM)))
  export CNY_HT_PDK_CFG = scripts/config.pdk
endif

export HT_DIR ?= /work/ht_releases/latest

# DO NOT MODIFY ITEMS BELOW THIS LINE
export HT_DIR ?= $(CNY_PDK)/ht
include $(HT_DIR)/ht_lib/MakefileInclude.cnyht
