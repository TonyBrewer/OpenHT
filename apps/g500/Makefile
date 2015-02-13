export CNY_PDK          ?= /work/pdk_releases
export CNY_PDK_REV      ?= latest
export CNY_PDK_PLATFORM ?= hc-2ex

LIB_DIR  = lib_pers
DIR_G500 = graph500-2.1.4
EXE_G500 = $(DIR_G500)/convey-csr/convey-csr

.PHONY: app
app:
	cd $(LIB_DIR) > /dev/null && $(MAKE) pdk_config
	cd $(LIB_DIR) > /dev/null && $(MAKE) libapp
	$(MAKE) -C $(DIR_G500) LIB_HT=../$(LIB_DIR)/libhtapp.a
	cp -p $(EXE_G500) app

sysc:
	cd $(LIB_DIR) > /dev/null && $(MAKE) pdk_config
	cd $(LIB_DIR) > /dev/null && $(MAKE) libsysc
	$(MAKE) -C $(DIR_G500) \
		DEFINE=-DNBFS_max=1 LIB_HT=../$(LIB_DIR)/libhtsysc.a
	cp -p $(EXE_G500) app_sysc

vsim: app_vsim
	cd $(LIB_DIR) > /dev/null && $(MAKE) pdk_config
	cd $(LIB_DIR) > /dev/null && $(MAKE) libvsim vfiles
	$(MAKE) -C $(DIR_G500) \
		DEFINE=-DNBFS_max=1 LIB_HT=../$(LIB_DIR)/libhtvsim.a
	cp -p $(EXE_G500) app_vsim.exe

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
	rm -rf app app_sysc app_vsim app_vsim.exe HtMonRpt.txt
	@$(MAKE) -s -C $(LIB_DIR) clean
	@$(MAKE) -s -C $(DIR_G500) clean

cleaner : clean
	@$(MAKE) -s -C $(LIB_DIR) $@

distclean: cleaner
	@$(MAKE) -s -C $(LIB_DIR) $@
