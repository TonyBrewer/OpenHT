TOPDIR = $(shell pwd)

HTL=./src/htl
HTL_SRC = $(wildcard $(HTL)/*.cpp)
HTL_OBJ = $(HTL_SRC:.cpp=.o)
htl: DEPS = $(HTL_SRC:.cpp=.d)

HTV_SRC = $(wildcard src/Htfe/*.cpp src/Htv/*.cpp)
HTV_OBJ = $(HTV_SRC:.cpp=.o)
htv: CPPFLAGS += -I./src/Htfe
htv: DEPS = $(HTV_SRC:.cpp=.d)

VEX=./src/Vex/Vex
VEX_SRC = $(wildcard $(VEX)/*.cpp)
VEX_OBJ = $(VEX_SRC:.cpp=.o)
vex: DEPS = $(VEX_SRC:.cpp=.d)

HTLIB_SRC = $(wildcard ht_lib/*/*Lib.cpp)
HTLIB_OBJ = $(HTLIB_SRC:.cpp=.o)
HTLIB_POBJ = $(HTLIB_SRC:.cpp=.po)
$(HTLIB_OBJ) $(HTLIB_POBJ): CXXFLAGS += -Iht_lib -I/opt/convey/include
$(HTLIB_OBJ) $(HTLIB_POBJ): CXXFLAGS += -I./local_systemc/include

BLDDTE = $(shell date)
ifneq (,$(wildcard .svn))
 VCSREV = $(shell svn info | grep Revis | awk -F: '{print $$2}' | sed 's/ //g')
else ifneq (,$(wildcard .git))
 VCSREV = $(shell head -1 .git/refs/heads/master | cut -c 1-7)
endif
VERSION = 2.1.1
REL_DIR = $(VERSION)-$(VCSREV)

export OPT_LVL = -g -O2

CXXFLAGS += $(OPT_LVL)
CXXFLAGS += -m64
CXXFLAGS += -Wall
CXXFLAGS += -Wno-parentheses -Wno-write-strings
CXXFLAGS += -DBLDDTE="\"$(BLDDTE)\""
#CXXFLAGS += -DSVNURL="\"$(SVNURL)\""
CXXFLAGS += -DVCSREV="\"$(VCSREV)\""
CXXFLAGS += -DVERSION="\"$(VERSION)\""
CFLAGS = $(CXXFLAGS)

SPEC = $(TOPDIR)/rpm/convey_ht_tools.spec
PREFIX ?= $(TOPDIR)/rpm/prefix

.PHONY: all prefix rpm clean

all: local_systemc
	$(MAKE) htl htv ht_lib/libht.a ht_lib/libht.pa

local_systemc:
	cd import > /dev/null; $(MAKE)
	mv import/$@ .

htl: $(HTL_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $(HTL_OBJ) $(LIC_LIB)

htv: $(HTV_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $(HTV_OBJ) $(LIC_LIB)

vex: $(VEX_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $(VEX_OBJ)

ht_lib/libht.a: $(HTLIB_OBJ)
	ar rcs $@ $(HTLIB_OBJ)

%.po : %.cpp
	$(CXX) $(CXXFLAGS) -fPIC -c -o $@ $<

ht_lib/libht.pa: $(HTLIB_POBJ)
	ar rcs $@ $(HTLIB_POBJ)

prefix install: all
	rm -rf $(PREFIX)
	mkdir -p $(PREFIX)/bin
	cp -p htl htv $(PREFIX)/bin
	cp -rp ht_lib $(PREFIX)
	sed 's/<VERSION>/$(REL_DIR)/' ht_lib/MakefileInclude.cnyht \
		> $(PREFIX)/ht_lib/MakefileInclude.cnyht
	rm -rf $(PREFIX)/ht_lib/*/*Lib.* 
	rm -rf $(PREFIX)/ht_lib/platform/convey/verilog/*.vpp
	rm -rf $(PREFIX)/ht_lib/qt
	cp -rp html $(PREFIX)
	mkdir -p $(PREFIX)/doc/man1
	cp -p doc/man1/*.1 $(PREFIX)/doc/man1
	mkdir -p $(PREFIX)/examples
	cp -p tests/examples.txt $(PREFIX)/examples; \
	(for ex in $(wildcard tests/ex_*); do \
	  e=$${ex#*/ex_}; \
	  cp -rp $$ex $(PREFIX)/examples/$$e; \
	  rm -rf $(PREFIX)/examples/$$e/msvs*; \
	done)
	find $(PREFIX)/examples -name "*.htl" -exec rm {} \;
	cp -rp local_systemc $(PREFIX)
ifdef STRIP
	strip $(PREFIX)/bin/*
	strip -g -S -d -x -X --strip-unneeded $(PREFIX)/ht_lib/libht.*
endif
	find $(PREFIX) -name ".svn" | xargs rm -rf
	find $(PREFIX) -type d | xargs chmod 755
	find $(PREFIX) -type f | xargs chmod 644
	chmod 755 $(PREFIX)/bin/* $(PREFIX)/local_systemc/lib*/*.a

REL_RPM  = convey-ht-tools-$(VERSION)-$(VCSREV).x86_64.rpm
REL_BASE = /usr/local/ht_releases
REL_PATH = $(REL_BASE)/$(REL_DIR)

release: prefix
	find $(PREFIX) -type d | xargs chmod 775
	if [ ! -d "$(REL_BASE)" ]; then     \
		mkdir -p $(REL_BASE);       \
	fi
	rsync -av --delete rpm/prefix/ $(REL_PATH)
	cd $(dir $(REL_PATH)); rm -f latest; ln -s $(REL_DIR) latest
ifeq ($(shell uname), Linux)
#	$(MAKE) rpm
#	mv -f RPMS/x86_64/$(REL_RPM) $(dir $(REL_PATH))
#	cd $(dir $(REL_PATH)); rm -f rpm_latest; ln -s $(REL_RPM) rpm_latest
endif

rpm:
	/bin/echo "%_topdir $(PREFIX)" > $(HOME)/.rpmmacros
	rpmbuild --define '_topdir $(TOPDIR)' \
		 --define 'version $(VERSION)' \
		 --define 'release $(VCSREV)' \
		 -bb $(SPEC)

clean:
	$(MAKE) -C import clean
	rm -f ./*.o ./htl ./htv ./vex
	rm -f $(HTL)/*.[od] src/Ht*/*.[od] $(VEX)/*.[od]
	rm -f $(HTLIB_OBJ) $(HTLIB_POBJ) ht_lib/libht.*
	rm -f $(HTL)/*.d.* src/Ht*/*.d.* $(VEX)/*.d.*
	rm -rf $(PREFIX) RPMS/*

cleaner: clean
	$(MAKE) -s -C tests distclean

distclean: cleaner
	rm -rf local_systemc

#
# Automatic Prerequisites
#
%.d: %.cpp
	@set -e; rm -f $@;\
	$(CC) -M $(CPPFLAGS) $< > $@.$$$$;\
	sed 's,\($(notdir $*\))\.o[ :]*,$(dir $<)\1.o $@ : ,g' < $@.$$$$ > $@;\
	rm -f $@.$$$$

-include $(DEPS)
