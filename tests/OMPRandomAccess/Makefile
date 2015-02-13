export CNY_PDK          ?= /opt/convey/pdk2
export CNY_PDK_REV      ?= latest
#export CNY_PDK_PLATFORM ?= hc-2ex
export CNY_PDK_PLATFORM ?= wx-690

export PLAT_hc-1_AEUCNT = 16
export PLAT_hc-2_AEUCNT = $(PLAT_hc-1_AEUCNT)
export PLAT_hc-1ex_AEUCNT = 1
export PLAT_hc-2ex_AEUCNT = $(PLAT_hc-1ex_AEUCNT)
export PLAT_wx-690_AEUCNT = 16
export PLAT_wx-2000_AEUCNT = $(PLAT_wx-690_AEUCNT)

export CNY_HT_AEUCNT = $(PLAT_${CNY_PDK_PLATFORM}_AEUCNT)

export CFLAGS += "-fopenmp"
export LDFLAGS += "-fopenmp"



# DO NOT MODIFY ITEMS BELOW THIS LINE
export HT_DIR ?= $(CNY_PDK)/ht
include $(HT_DIR)/ht_lib/MakefileInclude.cnyht

