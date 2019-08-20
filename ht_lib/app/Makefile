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
SRCS	+= $(PWD)/ht$(HT_PROJ_SFX)/app/UnitIntf.cpp
SRCS	+= $(wildcard $(HT_LIB)/host/*.cpp)

OBJS	=  $(addsuffix .o, $(basename $(notdir $(SRCS))))

OFILT	= $(addsuffix .o, $(basename $(notdir $(CNY_HT_LIBEXCLUDE))))
LOBJS	= $(filter-out $(OFILT), $(OBJS))
POBJS	= $(addsuffix .po, $(basename $(LOBJS)))

PLAT_OBJS = $(wildcard $(HT_LIB)/platform/$(OHT_PLAT)/host/*.o)

HT_HTIS =  $(wildcard $(STDIR)/*.hti)
HT_HTDS =  $(wildcard $(STDIR)/*.htd)

HFILES   = $(filter-out PersAeTop.h, \
           $(addsuffix .h, $(basename $(notdir \
	       $(wildcard $(PWD)/ht$(HT_PROJ_SFX)/app/*.sc)))))

vpath %.sc $(PWD)/ht$(HT_PROJ_SFX)/app

genfiles: HostIntf.h

.PHONY: app
app: ../../app$(HT_PROJ_SFX)

../../app$(HT_PROJ_SFX): $(OBJS)
	$(CXX) -o $@ $(LDFLAGS) $(OBJS) $(PLAT_OBJS) $(LDLIBS)

.PHONY: libapp
ifdef HT_LIB_PIC
libapp: ../../libhtapp$(HT_PROJ_SFX).pa
else
libapp: ../../libhtapp$(HT_PROJ_SFX).a
endif

../../libhtapp$(HT_PROJ_SFX).a: $(LOBJS) $(HT_LIB)/libht.a
	cp $(HT_LIB)/libht.a $@
	ar rcs $@ $(LOBJS) $(PLAT_OBJS) 
	@chmod 644 $@

../../libhtapp$(HT_PROJ_SFX).pa: $(POBJS) $(HT_LIB)/libht.pa
	cp $(HT_LIB)/libht.pa $@
	ar rcs $@ $(POBJS) $(PLAT_OBJS)
	@chmod 644 $@


include $(HT_DIR)/ht_lib/Makefile.template
