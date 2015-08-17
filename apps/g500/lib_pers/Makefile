export CNY_PDK          ?= /work/pdk_releases
export CNY_PDK_REV      ?= latest
export CNY_PDK_PLATFORM ?= hc-2ex

export CNY_HT_SIG ?= 65000
#export CNY_HT_NICK ?= $(CNY_HT_SIG)

export PLAT_hc-1_AEUCNT = 6
export PLAT_hc-2_AEUCNT = $(PLAT_hc-1_AEUCNT)
export PLAT_hc-1ex_AEUCNT = 16
export PLAT_hc-2ex_AEUCNT = $(PLAT_hc-1ex_AEUCNT)
export PLAT_wx-2000_AEUCNT = 8
export PLAT_wx-690_AEUCNT = 16
pers: export CNY_HT_AEUCNT = $(PLAT_${CNY_PDK_PLATFORM}_AEUCNT)

# DO NOT MODIFY ITEMS BELOW THIS LINE
export HT_DIR ?= $(CNY_PDK)/ht
include $(HT_DIR)/ht_lib/MakefileInclude.cnyht
