CGMINER_URL=https://github.com/ckolivas/cgminer/archive
CGMINER_VER=3.4.3

cgminer-$(CGMINER_VER)/cgminer: cgminer-$(CGMINER_VER) ../lib_pers/libhtapp.a
	cd cgminer-$(CGMINER_VER) > /dev/null && $(MAKE)

cgminer-$(CGMINER_VER): v$(CGMINER_VER).tar.gz v$(CGMINER_VER).patch
	tar -zxf v$(CGMINER_VER).tar.gz
	cd $@ > /dev/null && patch -i ../v$(CGMINER_VER).patch
	-cd $@ > /dev/null && \
	    ./autogen.sh CFLAGS=-g --enable-openht --without-curses
	ln -s cgminer-$(CGMINER_VER) cgminer

v$(CGMINER_VER).tar.gz:
	wget $(CGMINER_URL)/v$(CGMINER_VER).tar.gz

../lib_pers/libhtapp.a:
	cd ../lib_pers > /dev/null && $(MAKE) libapp

clean:
	@if [ -d cgminer-$(CGMINER_VER) ]; then \
	    cd cgminer-$(CGMINER_VER) > /dev/null; \
	    $(MAKE) -s clean; \
	fi

distclean:
	rm -rf cgminer cgminer-$(CGMINER_VER) v$(CGMINER_VER).tar.gz
