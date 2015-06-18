export BOOST_INSTALL = $(PWD)/local_boost
export ROSE_INSTALL  = $(PWD)/local_rose
#export BOOST_INSTALL = /nethome/jeckhardt/build/inst
#export ROSE_INSTALL  = /home/jle/convey-stuff/tot2/inst-rose

CFLAGS += -I$(BOOST_INSTALL)/include
CFLAGS += -I$(ROSE_INSTALL)/include

## Our translator
TRANSLATOR = htc1
TRANSLATOR_SOURCES = \
	main.cpp \
	EmitHTDescriptionFile.cpp \
	ht_omp_lowering.cpp \
	ht_GenerateFunc.cpp \
	HtcAttributes.cpp \
	HtDeclManager.cpp \
	HtSageUtils.cpp \
	HtdInfoAttribute.cpp \
	IsolateModules.cpp \
	LateOmpLowering.cpp \
	MyConstantFolding.cpp \
	Normalize.cpp \
	ProcessCalls.cpp \
	ProcessMemRefs.cpp \
	ProcessStencils.cpp \
	RewriteAsStateMachine.cpp \
	RewriteControlFlow.cpp

TRANSLATOR_OBJECTS = $(TRANSLATOR_SOURCES:.cpp=.o)

## Input testcode for translator
TESTCODE = sanity-test.c

#-------------------------------------------------------------
# Makefile Targets
#-------------------------------------------------------------

all: local_boost local_rose
	@. ./import/build_vars.sh && $(MAKE) $(TRANSLATOR)

local_boost local_rose:
	cd import > /dev/null && $(MAKE) $@
	ln -s import/$@ .

# Compile
.cpp.o:
	@true; $(CXX) $(CFLAGS) -MM $< > $*.d
	$(CXX) $(CFLAGS) -c -g -o $@ $<

# Link
$(TRANSLATOR): $(TRANSLATOR_OBJECTS)
	. ./import/build_vars.sh && \
	$(CXX) $(TRANSLATOR_OBJECTS) -g -o $(TRANSLATOR) \
		-L$(ROSE_INSTALL)/lib -lrose

# Test (stub for now)
check: $(TRANSLATOR)
	. ./import/build_vars.sh && \
	./$(TRANSLATOR) -c -I. -I$(ROSE_INSTALL)/include $(TESTCODE) 

clean:
	rm -rf $(TRANSLATOR) *.o rose_* *.dot *.d
	rm -rf local_boost local_rose rpm

distclean: clean
	rm -rf rpm
	$(MAKE) -C import clean

# Include auto deps (and don't error on missing .o).
-include $(TRANSLATOR_OBJECTS:.o=.d)

##########

PREFIX=rpm/prefix

$(PREFIX):
	@mkdir -p $@

prefix: $(PREFIX) all
	if [ -d /sw/Local/c5x/gcc-4.4.7/lib64 ]; then \
	    mkdir -p $(PREFIX)/lib/gcc-4.4.7/lib64; \
	    cd $(PREFIX)/lib/gcc-4.4.7/lib64 > /dev/null && \
		(cd /sw/Local/c5x/gcc-4.4.7/lib64 > /dev/null && \
		    tar -cf - libstdc++.so* libgcc_s.so*) | tar -xf -; \
	fi
	cd $(PREFIX) > /dev/null && \
	    (cd $(BOOST_INSTALL) > /dev/null && \
		tar -cf - lib) | tar -xf -
	cd $(PREFIX) > /dev/null && \
	    (cd $(ROSE_INSTALL) > /dev/null && \
		tar -cf - bin include lib) | tar -xf -
	cp -p $(TRANSLATOR) htc $(PREFIX)/bin
	find $(PREFIX) -name ".svn" | xargs rm -rf
	find $(PREFIX) -type d | xargs chmod 755
	find $(PREFIX) -type f | xargs chmod 644
	chmod 755 $(PREFIX)/bin/*

VERSION = 0.0
ifneq (,$(wildcard ../../.git))
 VCSREV = $(shell head -1 ../../.git/refs/heads/master | cut -c 1-7)
endif
REL_DIR = $(VERSION)-$(VCSREV)
REL_PATH = /usr/local/htc_releases/$(REL_DIR)

release: prefix
	rsync -av --delete rpm/prefix/ $(REL_PATH)
	cd $(dir $(REL_PATH)) > /dev/null && \
	    rm -f latest; ln -s $(REL_DIR) latest
