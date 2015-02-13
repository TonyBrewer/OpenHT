ROSE_BUILD = /dev/shm/rose

local_rose: patchelf
	$(MAKE) local_boost
	rm -rf $(ROSE_BUILD); mkdir -p $(ROSE_BUILD)
	cp -rp local_boost $(ROSE_BUILD)
	cp -p edg4x-rose.patch $(ROSE_BUILD)
	export BOOST_INSTALL=$(ROSE_BUILD)/local_boost && \
	. ./build_vars.sh && \
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
	./localize_rose.sh

##########

SHM_BUILD = /dev/shm/patchelf

patchelf:
	rm -rf $(SHM_BUILD); mkdir -p $(SHM_BUILD)
	cd $(SHM_BUILD) > /dev/null && \
	    wget http://nixos.org/releases/patchelf/patchelf-0.8/patchelf-0.8.tar.gz && \
	    tar -xf *.gz
	. ./build_vars.sh && \
	    cd $(SHM_BUILD)/patchelf* && \
	    ./configure --prefix=$(SHM_BUILD)/local && \
	    nice $(MAKE)
	cp -p $(SHM_BUILD)/patchelf-0.8/src/patchelf .

##########

BOOST_VER = 1_45_0
BOOST_VER_DOT = $(subst _,.,$(BOOST_VER))
BOOST_URL = http://sourceforge.net/projects/boost/files/boost
BOOST_BUILD = /dev/shm/boost

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
	rsync -a --delete --exclude=.svn $(BOOST_BUILD)/local/ local_boost

##########

clean:
	rm -rf $(ROSE_BUILD) $(BOOST_BUILD) $(SHM_BUILD)
	rm -rf local_rose local_boost patchelf
