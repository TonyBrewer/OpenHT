export CNY_PDK          ?= /work/pdk_releases
export CNY_PDK_REV      ?= latest
export CNY_PDK_PLATFORM ?= hc-2ex

export CNY_HT_AEUCNT = 16

LIB_DIR  = lib_pers
DIR_HPCC = import/hpcc
EXE_HPCC = $(DIR_HPCC)/hpcc

export AE_LIM = 1
ifneq ($(findstring hc,$(CNY_PDK_PLATFORM)),)
app:export AE_LIM = 4
endif

.PHONY: app
app:
	cd import > /dev/null && $(MAKE)
	cd $(LIB_DIR) > /dev/null && $(MAKE) pdk_config
	cd $(LIB_DIR) > /dev/null && $(MAKE) libapp
	$(MAKE) -C $(DIR_HPCC) LIB_HT=../../$(LIB_DIR)/libhtapp.a arch=OpenHT
	cp -p $(EXE_HPCC) app

sysc:
	cd import > /dev/null && $(MAKE)
	cd $(LIB_DIR) > /dev/null && $(MAKE) pdk_config
	cd $(LIB_DIR) > /dev/null && $(MAKE) libsysc
	$(MAKE) -C $(DIR_HPCC) LIB_HT=../../$(LIB_DIR)/libhtsysc.a arch=OpenHT
	cp -p $(EXE_HPCC) app_sysc

model:
	cd import > /dev/null && $(MAKE)
	cd $(LIB_DIR) > /dev/null && $(MAKE) pdk_config
	cd $(LIB_DIR) > /dev/null && $(MAKE) libmodel
	$(MAKE) -C $(DIR_HPCC) LIB_HT=../../$(LIB_DIR)/libhtmodel.a arch=OpenHT
	cp -p $(EXE_HPCC) app_model

vsim: app_vsim
	cd import > /dev/null && $(MAKE)
	cd $(LIB_DIR) > /dev/null && $(MAKE) pdk_config
	cd $(LIB_DIR) > /dev/null && $(MAKE) libvsim vfiles
	$(MAKE) -C $(DIR_HPCC) LIB_HT=../../$(LIB_DIR)/libhtvsim.a arch=OpenHT
	cp -p $(EXE_HPCC) app_vsim.exe

app_vsim:
	@echo "#!/bin/sh" > $@
	@echo ". $(LIB_DIR)/ht/config.sh" >> $@
	@echo ". $$CNY_PDK/$$CNY_PDK_REV/settings.sh" >> $@
	@echo "export CNY_PERSONALITY_PATH=$(CURDIR)/$(LIB_DIR)/personalities" >> $@
	@echo "runpdksim -ae=0 -deadman=10000000 -mc_read_short_percent=99 -run_dir=$(LIB_DIR)/ht/sim -log=$(LIB_DIR)/ht/sim/runpdksim.log ./app_vsim.exe \$$*" >> $@
	@chmod 755 $@

pers:
	cd $(LIB_DIR) > /dev/null && $(MAKE) pers

clean:
	rm -rf app app_sysc app_model app_vsim app_vsim.exe HtMonRpt.txt
	@$(MAKE) -s -C $(LIB_DIR) clean
	@$(MAKE) -s -C import clean

cleaner distclean: clean
	@$(MAKE) -s -C $(LIB_DIR) $@
	@$(MAKE) -s -C import $@
