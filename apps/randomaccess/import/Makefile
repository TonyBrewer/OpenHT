HPCC_URL=http://icl.cs.utk.edu/projectsfiles/hpcc/download
HPCC_VER=1.4.3

hpcc: hpcc-$(HPCC_VER).tar.gz
	tar -zxf $?
	patch -p0 -i hpcc-$(HPCC_VER).patch
	mv hpcc-$(HPCC_VER) $@

hpcc-$(HPCC_VER).tar.gz:
	wget $(HPCC_URL)/hpcc-$(HPCC_VER).tar.gz

clean cleaner:
	@if [ -d hpcc ]; then \
	     $(MAKE) -s -C hpcc clean arch=OpenHT; \
	fi

distclean:
	rm -rf hpcc hpcc-$(HPCC_VER).tar.gz
