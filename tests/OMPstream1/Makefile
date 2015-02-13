export CNY_PDK          ?= /opt/convey/pdk2
export CNY_PDK_REV      ?= latest
export CNY_PDK_PLATFORM ?= wx-690

PLAT_${CNY_PDK_PLATFORM}_AEUCNT ?= 8
export CNY_HT_AEUCNT = $(PLAT_${CNY_PDK_PLATFORM}_AEUCNT)

export CFLAGS += "-fopenmp"
export LDFLAGS += "-fopenmp"

# DO NOT MODIFY ITEMS BELOW THIS LINE
export HT_DIR ?= $(CNY_PDK)/ht
include $(HT_DIR)/ht_lib/MakefileInclude.cnyht

