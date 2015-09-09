BLD_BASE = $(wildcard /dev/shm)
ifeq (,$(BLD_BASE))
BLD_BASE = /tmp
endif

ROSE_BUILD = $(BLD_BASE)/rose

local_rose: patchelf
	$(MAKE) local_boost
	rm -rf $(ROSE_BUILD); mkdir -p $(ROSE_BUILD)
	cp -rp local_boost $(ROSE_BUILD)
	cp -p edg4x-rose.patch $(ROSE_BUILD)
	export BOOST_INSTALL=$(ROSE_BUILD)/local_boost && \
	. ./build_vars.sh && \
	    cd $(ROSE_BUILD) > /dev/null && \
	    git clone https://github.com/rose-compiler/rose && \
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
	rsync -a --delete $(ROSE_BUILD)/local/ local_rose
	./localize_rose.sh
	rm -rf $(ROSE_BUILD)

##########

ELF_BUILD = $(BLD_BASE)/patchelf

patchelf:
	rm -rf $(ELF_BUILD); mkdir -p $(ELF_BUILD)
	cd $(ELF_BUILD) > /dev/null && \
	    wget http://nixos.org/releases/patchelf/patchelf-0.8/patchelf-0.8.tar.gz && \
	    tar -xf *.gz
	. ./build_vars.sh && \
	    cd $(ELF_BUILD)/patchelf* && \
	    ./configure --prefix=$(ELF_BUILD)/local && \
	    nice $(MAKE)
	cp -p $(ELF_BUILD)/patchelf-0.8/src/patchelf .
	rm -rf $(ELF_BUILD)

##########

BOOST_VER = 1_45_0
BOOST_VER_DOT = $(subst _,.,$(BOOST_VER))
BOOST_URL = http://sourceforge.net/projects/boost/files/boost
BOOST_BUILD = $(BLD_BASE)/boost

local_boost:
	rm -rf $(BOOST_BUILD); mkdir -p $(BOOST_BUILD)
	cd $(BOOST_BUILD) > /dev/null && \
	    wget $(BOOST_URL)/$(BOOST_VER_DOT)/boost_$(BOOST_VER).tar.gz && \
	    tar -xf *.gz
	. ./build_vars.sh && \
	    cd $(BOOST_BUILD)/boost_$(BOOST_VER) && \
	    nice ./bootstrap.sh --prefix=$(BOOST_BUILD)/local && \
	    ./bjam install \
		--prefix=$(BOOST_BUILD)/local \
		--without-mpi
	rsync -a --delete $(BOOST_BUILD)/local/ local_boost
	rm -rf $(BOOST_BUILD)

##########

clean:
	rm -rf $(ROSE_BUILD) $(BOOST_BUILD) $(ELF_BUILD)
	rm -rf local_rose local_boost patchelf
