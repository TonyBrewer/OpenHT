# Copyright (c) 2015 Convey Computer Corporation
#
# This file is part of the OpenHT toolset located at:
#
# https://github.com/TonyBrewer/OpenHT
#
# Use and distribution licensed under the BSD 3-clause license.
# See the LICENSE file for the complete license text.

HT_LIB	= $(HT_DIR)/ht_lib
HT_PROJ_SFX ?= 

SDIR	= $(PWD)/src
STDIR	= $(PWD)/src_pers

SRCS	+= $(wildcard $(SDIR)/*.cpp $(SDIR)/*.cc $(SDIR)/*.c)
SRCS	+= $(PWD)/ht$(HT_PROJ_SFX)/vsim/UnitIntf.cpp
SRCS	+= $(wildcard $(HT_LIB)/host/*.cpp)

OBJS	=  $(addsuffix .o, $(basename $(notdir $(SRCS))))

OFILT	= $(addsuffix .o, $(basename $(notdir $(CNY_HT_LIBEXCLUDE))))
LOBJS	= $(filter-out $(OFILT), $(OBJS))
POBJS	= $(addsuffix .po, $(basename $(LOBJS)))

PLAT_OBJS = $(wildcard $(HT_LIB)/platform/$(OHT_PLAT)/host/*.o)

HT_HTIS	= $(wildcard $(STDIR)/*.hti)
HT_HTDS	= $(wildcard $(STDIR)/*.htd)

HFILES   = $(filter-out PersAeTop.h, \
	   $(addsuffix .h, $(basename $(notdir \
	       $(wildcard $(PWD)/ht$(HT_PROJ_SFX)/vsim/*.sc)))))

VFILES = $(addprefix ../verilog/, $(addsuffix .v, $(basename $(notdir \
	 $(wildcard $(HT_LIB)/sysc/Per*.cpp \
	 	    $(PWD)/ht$(HT_PROJ_SFX)/vsim/Per*.cpp \
		    $(PWD)/ht$(HT_PROJ_SFX)/vsim/Pers*.sc)))))

vpath %.sc $(PWD)/ht$(HT_PROJ_SFX)/vsim

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
vsim: ../../app_vsim$(HT_PROJ_SFX)

../../app_vsim$(HT_PROJ_SFX): ../../app_vsim$(HT_PROJ_SFX).exe $(VFILES)
	@echo "#!/bin/sh" > $@
	@echo ". ht$(HT_PROJ_SFX)/config.sh" >> $@
	@echo "export LD_LIBRARY_PATH=\$$LD_LIBRARY_PATH:$(CNY_PATH)/lib" >> $@
	@echo "export CNY_PERSONALITY_PATH=$(PWD)/personalities" >> $@
	@echo -n "$(CNY_PDK)/$(CNY_PDK_REV)/bin/runpdksim" >> $@
	@echo -n " -ae=0 -deadman=10000000 -mc_read_short_percent=99" >> $@
	@echo -n " -run_dir=ht$(HT_PROJ_SFX)/sim -log=ht$(HT_PROJ_SFX)/sim/runpdksim.log" >> $@
	@echo    " ./app_vsim$(HT_PROJ_SFX).exe \$$@" >> $@
	@chmod 755 $@

../../app_vsim$(HT_PROJ_SFX).exe: $(OBJS)
	$(CXX) -o $@ $(LDFLAGS) $(OBJS) $(PLAT_OBJS) $(LDLIBS)


.PHONY: libvsim
ifdef HT_LIB_PIC
libvsim: ../../libhtvsim$(HT_PROJ_SFX).a ../../libhtvsim$(HT_PROJ_SFX).pa
else
libvsim: ../../libhtvsim$(HT_PROJ_SFX).a
endif

../../libhtvsim$(HT_PROJ_SFX).a: $(LOBJS) $(HT_LIB)/libht.a
	cp $(HT_LIB)/libht.a $@
	ar rcs $@ $(LOBJS) $(PLAT_OBJS)
	@chmod 644 $@

../../libhtvsim$(HT_PROJ_SFX).pa: $(POBJS) $(HT_LIB)/libht.pa
	cp $(HT_LIB)/libht.pa $@
	ar rcs $@ $(POBJS) $(PLAT_OBJS)
	@chmod 644 $@

include $(HT_DIR)/ht_lib/Makefile.template
-include $(wildcard $(PWD)/ht$(HT_PROJ_SFX)/vsim/*.P)
