CGMINER_URL=https://github.com/ckolivas/cgminer/archive
CGMINER_VER=3.4.3

cgminer: cgminer-$(CGMINER_VER) lib_pers/libhtapp.a
	cd cgminer-$(CGMINER_VER) > /dev/null && \
	    ./autogen.sh CFLAGS=-g --enable-openht --without-curses; \
	    $(MAKE)
	mv cgminer-$(CGMINER_VER)/$@ .

cgminer-$(CGMINER_VER): v$(CGMINER_VER).tar.gz import/v$(CGMINER_VER).patch
	tar -zxf v$(CGMINER_VER).tar.gz
	cd $@ > /dev/null && patch -i ../import/v$(CGMINER_VER).patch

v$(CGMINER_VER).tar.gz:
	wget $(CGMINER_URL)/v$(CGMINER_VER).tar.gz

lib_pers/libhtapp.a:
	cd lib_pers > /dev/null && $(MAKE) libapp

pers:
	cd lib_pers > /dev/null && $(MAKE) $@

clean distclean:
	cd lib_pers > /dev/null && $(MAKE) $@
	rm -rf cgminer cgminer-$(CGMINER_VER) v$(CGMINER_VER).tar.gz
