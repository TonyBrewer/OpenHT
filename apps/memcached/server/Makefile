# NOTE: requires libevent and libevent-devel
#
export CNY_PDK		?= /opt/convey/pdk2
export CNY_PDK_REV	?= latest
export CNY_PDK_PLATFORM	?= wx-690

export CNY_HT_SIG	= 65000
export CNY_HT_NICK	= mcd
export CNY_HT_FREQ	= 150

PLAT_wx-690_AEUCNT = 16
PLAT_${CNY_PDK_PLATFORM}_AEUCNT ?= 4
pers: export CNY_HT_AEUCNT = $(PLAT_${CNY_PDK_PLATFORM}_AEUCNT)

ifneq (,$(findstring wx, $(CNY_PDK_PLATFORM)))
export CNY_HT_PDK_CFG = scripts/config.pdk
endif

export CFLAGS += -I$(PWD)/src
export LDLIBS += -levent

# ONLOAD
#export ONLOAD_VER = 201405
#export ONLOAD = $(abspath $(PWD)/../openonload/$(ONLOAD_VER))
#app: export CFLAGS += -DONLOAD -I$(ONLOAD)/include
#app: export LDLIBS += $(ONLOAD)/lib64/libonload_ext.a

# DO NOT MODIFY ITEMS BELOW THIS LINE
export HT_DIR ?= $(CNY_PDK)/ht
include $(HT_DIR)/ht_lib/MakefileInclude.cnyht
