export CNY_PDK          ?= /opt/convey/pdk2
export CNY_PDK_REV      ?= latest
export CNY_PDK_PLATFORM ?= hc-2

PLAT_wx-2000_AEUCNT = 8
PLAT_${CNY_PDK_PLATFORM}_AEUCNT ?= 16
pers: export CNY_HT_AEUCNT = $(PLAT_${CNY_PDK_PLATFORM}_AEUCNT)

# DO NOT MODIFY ITEMS BELOW THIS LINE
export HT_DIR ?= $(CNY_PDK)/ht
include $(HT_DIR)/ht_lib/MakefileInclude.cnyht
