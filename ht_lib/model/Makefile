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
STDIR	= $(PWD)/src_model

SRCS	+= $(wildcard $(SDIR)/*.cpp $(SDIR)/*.cc $(SDIR)/*.c)
SRCS	+= $(wildcard $(STDIR)/*.cpp $(STDIR)/*.cc $(STDIR)/*.c)
SRCS	+= $(PWD)/ht/model/UnitIntf.cpp
SRCS	+= $(wildcard $(HT_LIB)/host/*.cpp)

OBJS	=  $(addsuffix .o, $(basename $(notdir $(SRCS))))

OFILT	= $(addsuffix .o, $(basename $(notdir $(CNY_HT_LIBEXCLUDE))))
LOBJS	= $(filter-out $(OFILT), $(OBJS))
POBJS	= $(addsuffix .po, $(basename $(LOBJS)))

HT_HTIS =  $(wildcard $(STDIR)/*.hti)
HT_HTDS =  $(wildcard $(STDIR)/*.htd)

genfiles: HostIntf.h

.PHONY: model
model: ../../app_model

../../app_model: $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@

.PHONY: libmodel
ifdef HT_LIB_PIC
libmodel: ../../libhtmodel.pa
else
libmodel: ../../libhtmodel.a
endif

../../libhtmodel.a: $(LOBJS) $(HT_LIB)/libht.a
	cp $(HT_LIB)/libht.a $@
	ar rcs $@ $(LOBJS)
	@chmod 644 $@

../../libhtmodel.pa: $(POBJS) $(HT_LIB)/libht.pa
	cp $(HT_LIB)/libht.pa $@
	ar rcs $@ $(POBJS)
	@chmod 644 $@

include $(HT_DIR)/ht_lib/Makefile.template
