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

all:
	@. ./import/build_vars.sh && $(MAKE) $(TRANSLATOR)

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

# Include auto deps (and don't error on missing .o).
-include $(TRANSLATOR_OBJECTS:.o=.d)

cleaner: clean
	rm -rf rpm patchelf

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
SVN_REV = $(shell svn info | grep Revis | awk -F: '{print $$2}' | sed 's/ //g')
REL_DIR = $(VERSION)-$(SVN_REV)
REL_PATH = /work/htc_releases/$(REL_DIR)

release: prefix
	rsync -av --delete rpm/prefix/ $(REL_PATH)
	cd $(dir $(REL_PATH)) > /dev/null && \
	    rm -f latest; ln -s $(REL_DIR) latest

##########

BOOST_VER = 1_45_0
BOOST_VER_DOT = $(subst _,.,$(BOOST_VER))
BOOST_URL = http://sourceforge.net/projects/boost/files/boost
BOOST_BUILD = /dev/shm/boost

update_boost:
	rm -rf $(BOOST_BUILD); mkdir -p $(BOOST_BUILD)
	cd $(BOOST_BUILD) > /dev/null && \
	    wget $(BOOST_URL)/$(BOOST_VER_DOT)/boost_$(BOOST_VER).tar.gz && \
	    tar -xf *.gz
	. ./import/build_vars.sh && \
	    cd $(BOOST_BUILD)/boost_$(BOOST_VER) && \
	    nice ./bootstrap.sh --prefix=$(BOOST_BUILD)/local && \
	    ./bjam install \
		--prefix=$(BOOST_BUILD)/local \
		--without-mpi
	rsync -a --delete --exclude=.svn $(BOOST_BUILD)/local/ local_boost

##########

ROSE_BUILD = /dev/shm/rose

update_rose: patchelf
	rm -rf $(ROSE_BUILD); mkdir -p $(ROSE_BUILD)
	rsync -a --exclude=.svn $(BOOST_INSTALL) $(ROSE_BUILD)
	cp -p import/edg4x-rose.patch $(ROSE_BUILD)
	. ./import/build_vars.sh && \
	    cd $(ROSE_BUILD) > /dev/null && \
	    git clone https://github.com/rose-compiler/edg4x-rose rose && \
	    cd rose > /dev/null && \
	    git reset --hard 18f3b98052c0c57f684b65693c58c4a48b314389 && \
	    patch -p1 -i ../edg4x-rose.patch && \
	    ./build && cd .. > /dev/null && \
	    mkdir local && mkdir build && cd build && \
	    ../rose/configure \
		--prefix=$(ROSE_BUILD)/local \
		--with-boost=$(ROSE_BUILD)/local_boost \
		--disable-java --disable-fortran \
		--with-C_DEBUG=-g \
		--with-CXX_OPTIMIZE=-O0 \
		--with-C_OPTIMIZE=-O0 && \
	    nice $(MAKE) -j16 install-core
	rsync -a --delete --exclude=.svn $(ROSE_BUILD)/local/ local_rose
	import/localize_rose.sh

##########

SHM_BUILD = /dev/shm/build

patchelf:
	rm -rf $(SHM_BUILD); mkdir -p $(SHM_BUILD)
	cd $(SHM_BUILD) > /dev/null && \
	    wget http://nixos.org/releases/patchelf/patchelf-0.8/patchelf-0.8.tar.gz && \
	    tar -xf *.gz
	. ./import/build_vars.sh && \
	    cd $(SHM_BUILD)/patchelf* && \
	    ./configure --prefix=$(SHM_BUILD)/local && \
	    nice $(MAKE)
	cp -p $(SHM_BUILD)/patchelf-0.8/src/patchelf .
	rm -rf $(SHM_BUILD)
