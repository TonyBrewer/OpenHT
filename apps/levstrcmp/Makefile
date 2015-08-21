export CNY_PDK          = /work/pdk_releases
export CNY_PDK_REV      = latest
export CNY_PDK_PLATFORM = hc-2ex

#export CNY_HT_SIG       = 32050
#export CNY_HT_NICK      = fzy_lev

PLAT_hc-1_AEUCNT = 3
PLAT_hc-2_AEUCNT = $(PLAT_hc-1_AEUCNT)
PLAT_hc-1ex_AEUCNT = 8
PLAT_hc-2ex_AEUCNT = $(PLAT_hc-1ex_AEUCNT)
PLAT_wx-2000_AEUCNT = 6
PLAT_wx-690_AEUCNT = 8
pers: export CNY_HT_AEUCNT = $(PLAT_${CNY_PDK_PLATFORM}_AEUCNT)

#export CFLAGS += -DEXAMPLE

export CNY_HT_FREQ = 125

# DO NOT MODIFY ITEMS BELOW THIS LINE
export HT_DIR ?= $(CNY_PDK)/ht
include $(HT_DIR)/ht_lib/MakefileInclude.cnyht

clean: myclean
myclean:
	cd levstrcmp && $(MAKE) clean
