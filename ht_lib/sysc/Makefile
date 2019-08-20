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
SRCS	+= $(filter-out $(wildcard $(STDIR)/Pers*_src.cpp), \
	      $(wildcard $(STDIR)/*.cpp $(STDIR)/*.cc $(STDIR)/*.c))
SRCS	+= $(wildcard $(HT_LIB)/host/*.cpp $(HT_LIB)/sysc/*.cpp)
SRCS	+= $(wildcard $(PWD)/ht$(HT_PROJ_SFX)/sysc/*.cpp)

OBJS	= $(addsuffix .o, $(basename $(notdir $(SRCS))))

OFILT	= $(addsuffix .o, $(basename $(notdir $(CNY_HT_LIBEXCLUDE))))
LOBJS	= $(filter-out $(OFILT), $(OBJS))
POBJS	= $(addsuffix .po, $(basename $(LOBJS)))

HT_HTIS = $(wildcard $(STDIR)/*.hti)
HT_HTDS = $(wildcard $(STDIR)/*.htd)

HFILES	= $(filter-out PersAeTop.h, \
          $(addsuffix .h, $(basename $(notdir \
	      $(wildcard $(PWD)/ht$(HT_PROJ_SFX)/sysc/*.sc)))))

vpath %.sc $(PWD)/ht$(HT_PROJ_SFX)/sysc

genfiles: HostIntf.h
genhfiles: hfiles 

.PHONY: sysc
sysc: ../../app_sysc$(HT_PROJ_SFX)

../../app_sysc$(HT_PROJ_SFX): $(OBJS)
	$(CXX) -o $@ $(CXXFLAGS) $(LDFLAGS) $(OBJS) $(LDLIBS)

.PHONY: libsysc
ifdef HT_LIB_PIC
libsysc: ../../libhtsysc$(HT_PROJ_SFX).pa
else
libsysc: ../../libhtsysc$(HT_PROJ_SFX).a
endif

../../libhtsysc$(HT_PROJ_SFX).a: $(LOBJS) $(HT_LIB)/libht.a
	cp -p $(HT_LIB)/libht.a $@
	ar rcs $@ $(LOBJS)
	@(TMPDIR=`mktemp -p . -d`; cd $$TMPDIR; \
	    ar x $(SC_LIB); \
	    ar rcs ../$@ *; \
	    cd ..; \
	    rm -rf $$TMPDIR)
	@chmod 644 $@

../../libhtsysc$(HT_PROJ_SFX).pa: $(POBJS) $(HT_LIB)/libht.pa
	cp -p $(HT_LIB)/libht.pa $@
	ar rcs $@ $(POBJS)
	@(TMPDIR=`mktemp -p . -d`; cd $$TMPDIR; \
	    ar x $(SC_LIB); \
	    ar rcs ../$@ *; \
	    cd ..; \
	    rm -rf $$TMPDIR)
	@chmod 644 $@

include $(HT_DIR)/ht_lib/Makefile.template
