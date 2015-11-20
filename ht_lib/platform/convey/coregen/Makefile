# Copyright (c) 2015 Convey Computer Corporation
#
# This file is part of the OpenHT toolset located at:
#
# https://github.com/TonyBrewer/OpenHT
#
# Use and distribution licensed under the BSD 3-clause license.
# See the LICENSE file for the complete license text.

.NOTPARALLEL:

SRC_DIR = ../../src_pers
SRC_FILES = $(wildcard $(SRC_DIR)/*.xco)

XCO_FILES = $(notdir $(SRC_FILES))
NGC_FILES = $(XCO_FILES:.xco=.ngc)

all: coregen.cgp $(NGC_FILES) | $(XCO_FILES)

%.ngc: %.xco
	@coregen -p . -b $?
	@mv -f coregen.log $(basename $?).log

%.xco: $(SRC_DIR)/%.xco
ifneq (,$(findstring ex, $(CNY_PDK_PLATFORM)))
	@cat $? |\
	    sed 's/SET designentry =.*/SET designentry = Verilog/' |\
	    sed 's/SET device =.*/SET device = xc6vlx760/' |\
	    sed 's/SET devicefamily =.*/SET devicefamily = virtex6/' |\
	    sed 's/SET package =.*/SET package = ff1760/' |\
	    sed 's/SET speedgrade =.*/SET speedgrade = -1/' \
	    > $@
else
	@cat $? |\
	    sed 's/SET designentry =.*/SET designentry = Verilog/' |\
	    sed 's/SET device =.*/SET device = xc5vlx330/' |\
	    sed 's/SET devicefamily =.*/SET devicefamily = virtex5/' |\
	    sed 's/SET package =.*/SET package = ff1760/' |\
	    sed 's/SET speedgrade =.*/SET speedgrade = -2/' \
	    > $@
endif

coregen.cgp:
ifneq (,$(findstring ex, $(CNY_PDK_PLATFORM)))
	@cp -p $(HT_LIB)/platform/$(OHT_PLAT)/coregen/coregenv6.cgp $@
else
	@cp -p $(HT_LIB)/platform/$(OHT_PLAT)/coregen/coregenv5.cgp $@
endif

clean:
	rm -f $(XCO_FILES) coregen.*
	rm -rf $(addsuffix *,$(basename $(XCO_FILES))) tmp xlnx*

include $(HT_DIR)/ht_lib/Makefile.template
