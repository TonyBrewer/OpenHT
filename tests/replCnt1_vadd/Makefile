export CNY_PDK          ?= /work/pdk_releases
export CNY_PDK_REV      ?= latest
export CNY_PDK_PLATFORM ?= wx-690

PLAT_wx-2000_AEUCNT = 8
PLAT_${CNY_PDK_PLATFORM}_AEUCNT ?= 16
pers: export CNY_HT_AEUCNT = $(PLAT_${CNY_PDK_PLATFORM}_AEUCNT)

# DO NOT MODIFY ITEMS BELOW THIS LINE
export HT_DIR ?= /usr/local/ht_releases/latest
include $(HT_DIR)/ht_lib/MakefileInclude.cnyht
