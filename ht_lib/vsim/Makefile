# Copyright (c) 2015 Convey Computer Corporation
#
# This file is part of the OpenHT toolset located at:
#
# https://github.com/TonyBrewer/OpenHT
#
# Use and distribution licensed under the BSD 3-clause license.
# See the LICENSE file for the complete license text.

HT_LIB	= $(HT_DIR)/ht_lib

SDIR	= $(PWD)/src
STDIR	= $(PWD)/src_pers

SRCS	+= $(wildcard $(SDIR)/*.cpp $(SDIR)/*.cc $(SDIR)/*.c)
SRCS	+= $(PWD)/ht/vsim/UnitIntf.cpp
SRCS	+= $(wildcard $(HT_LIB)/host/*.cpp)

OBJS	=  $(addsuffix .o, $(basename $(notdir $(SRCS))))

OFILT	= $(addsuffix .o, $(basename $(notdir $(CNY_HT_LIBEXCLUDE))))
LOBJS	= $(filter-out $(OFILT), $(OBJS))
POBJS	= $(addsuffix .po, $(basename $(LOBJS)))

HT_HTIS	= $(wildcard $(STDIR)/*.hti)
HT_HTDS	= $(wildcard $(STDIR)/*.htd)

HFILES   = $(filter-out PersAeTop.h, \
	   $(addsuffix .h, $(basename $(notdir \
	       $(wildcard $(PWD)/ht/vsim/*.sc)))))

VFILES = $(addprefix ../verilog/, $(addsuffix .v, $(basename $(notdir \
	 $(wildcard $(HT_LIB)/sysc/Per*.cpp \
	 	    $(PWD)/ht/vsim/Per*.cpp \
		    $(PWD)/ht/vsim/Pers*.sc)))))

vpath %.sc $(PWD)/ht/vsim

genfiles: HostIntf.h
genhfiles: hfiles

vsim libvsim: ENABLE=1
ifeq ($(ENABLE), 1)
$(OBJS): | hfiles
endif

.PHONY: vfiles
vfiles: $(VFILES)

$(VFILES): | hfiles

.PHONY: vsim
vsim: ../../app_vsim

../../app_vsim: ../../app_vsim.exe $(VFILES)
	@echo "#!/bin/sh" > $@
	@echo ". ht/config.sh" >> $@
	@echo "export LD_LIBRARY_PATH=\$$LD_LIBRARY_PATH:$(CNY_PATH)/lib" >> $@
	@echo "export CNY_PERSONALITY_PATH=$(PWD)/personalities" >> $@
	@echo -n "$(CNY_PDK)/$(CNY_PDK_REV)/bin/runpdksim" >> $@
	@echo -n " -ae=0 -deadman=10000000 -mc_read_short_percent=99" >> $@
	@echo -n " -run_dir=ht/sim -log=ht/sim/runpdksim.log" >> $@
	@echo    " ./app_vsim.exe \$$@" >> $@
	@chmod 755 $@

../../app_vsim.exe: $(OBJS)
	$(CXX) -o $@ $(LDFLAGS) $(OBJS) $(LDLIBS)


.PHONY: libvsim
ifdef HT_LIB_PIC
libvsim: ../../libhtvsim.a ../../libhtvsim.pa
else
libvsim: ../../libhtvsim.a
endif

../../libhtvsim.a: $(LOBJS) $(HT_LIB)/libht.a
	cp $(HT_LIB)/libht.a $@
	ar rcs $@ $(LOBJS)
	@chmod 644 $@

../../libhtvsim.pa: $(POBJS) $(HT_LIB)/libht.pa
	cp $(HT_LIB)/libht.pa $@
	ar rcs $@ $(POBJS)
	@chmod 644 $@

include $(HT_DIR)/ht_lib/Makefile.template
-include $(wildcard $(PWD)/ht/vsim/*.P)
