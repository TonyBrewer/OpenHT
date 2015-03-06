export CNY_PDK		= /opt/convey/pdk2
export CNY_PDK_REV	= latest
export CNY_PDK_PLATFORM	= wx-690

#export CNY_HT_SIG	= 32070
export CNY_HT_NICK	= jir

export CNY_HT_PDK_CFG	= scripts/config.pdk

export PLAT_wx-690_PART = 510_000078_000
export CNY_PDK_FPGA_PARTNUM = $(PLAT_${CNY_PDK_PLATFORM}_PART)

#export HTL_FLAGS += -DVEIMG_FIXTURE
#export HTV_FLAGS += -DVEIMG_FIXTURE
#export CFLAGS += -DVEIMG_FIXTURE
export HT_HOST_API = Driver
app vsim:export CFLAGS += -I../../src_pers
libmodel:export CFLAGS += -I../../../devapp

ifdef DEBUG
export CNY_HT_CFLAGS_APP= -g -Wall -O0
endif 

export CFLAGS += -std=gnu++0x -Wno-deprecated-declarations -L./ 
export LDLIBS += -ldl 

#export CNY_HT_SIM_VCD=true

#export HT_DIR ?= /work/ht_releases/latest

# DO NOT MODIFY ITEMS BELOW THIS LINE
export HT_DIR ?= $(CNY_PDK)/ht
include $(HT_DIR)/ht_lib/MakefileInclude.cnyht 
