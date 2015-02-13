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
SRCS	+= $(PWD)/ht/app/UnitIntf.cpp
SRCS	+= $(wildcard $(HT_LIB)/host/*.cpp)

OBJS	=  $(addsuffix .o, $(basename $(notdir $(SRCS))))

OFILT	= $(addsuffix .o, $(basename $(notdir $(CNY_HT_LIBEXCLUDE))))
LOBJS	= $(filter-out $(OFILT), $(OBJS))
POBJS	= $(addsuffix .po, $(basename $(LOBJS)))

HT_HTIS =  $(wildcard $(STDIR)/*.hti)
HT_HTDS =  $(wildcard $(STDIR)/*.htd)

HFILES   = $(filter-out PersAeTop.h, \
           $(addsuffix .h, $(basename $(notdir \
	       $(wildcard $(PWD)/ht/app/*.sc)))))

vpath %.sc $(PWD)/ht/app

genfiles: HostIntf.h

.PHONY: app
app: ../../app

../../app: $(OBJS)
	$(CXX) -o $@ $(LDFLAGS) $(OBJS) $(LDLIBS)

.PHONY: libapp
ifdef HT_LIB_PIC
libapp: ../../libhtapp.pa
else
libapp: ../../libhtapp.a
endif

../../libhtapp.a: $(LOBJS) $(HT_LIB)/libht.a
	cp $(HT_LIB)/libht.a $@
	ar rcs $@ $(LOBJS)
	@chmod 644 $@

../../libhtapp.pa: $(POBJS) $(HT_LIB)/libht.pa
	cp $(HT_LIB)/libht.pa $@
	ar rcs $@ $(POBJS)
	@chmod 644 $@


include $(HT_DIR)/ht_lib/Makefile.template
