export CNY_PDK          ?= /work/pdk_releases
export CNY_PDK_REV      ?= latest
export CNY_PDK_PLATFORM ?= wx-690

export CNY_HT_AEUCNT = 16

ifneq (,$(findstring hc,$(CNY_PDK_PLATFORM)))
export CFLAGS += -DAE_LIM=4
else
export CFLAGS += -DAE_LIM=1
endif
export CFLAGS += -DCNY_HT_AEUCNT=$(CNY_HT_AEUCNT)

# DO NOT MODIFY ITEMS BELOW THIS LINE
export HT_DIR ?= $(CNY_PDK)/ht
include $(HT_DIR)/ht_lib/MakefileInclude.cnyht
