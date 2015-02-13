export CNY_PDK          ?= /opt/convey/pdk2
export CNY_PDK_REV      ?= latest
export CNY_PDK_PLATFORM ?= hc-2

export PLAT_hc-1_AEUCNT = 16
export PLAT_hc-2_AEUCNT = $(PLAT_hc-1_AEUCNT)
export PLAT_hc-1ex_AEUCNT = 16
export PLAT_hc-2ex_AEUCNT = $(PLAT_hc-1ex_AEUCNT)
export PLAT_wx-690_AEUCNT = 8
export PLAT_wx-2000_AEUCNT = $(PLAT_wx-690_AEUCNT)
pers: export CNY_HT_AEUCNT = $(PLAT_${CNY_PDK_PLATFORM}_AEUCNT)

export HT_DIR ?= /work/ht_releases/latest

# DO NOT MODIFY ITEMS BELOW THIS LINE
export HT_DIR ?= $(CNY_PDK)/ht
include $(HT_DIR)/ht_lib/MakefileInclude.cnyht
