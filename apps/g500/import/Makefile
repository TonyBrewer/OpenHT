G500_URL=http://www.graph500.org/sites/default/files/files
G500_VER=2.1.4

graph500: graph500-$(G500_VER).tar.bz2
	bunzip2 -c $? | tar -xf -
	patch -p0 -i graph500-$(G500_VER).patch
	mv graph500-$(G500_VER) $@

graph500-$(G500_VER).tar.bz2:
	wget $(G500_URL)/graph500-$(G500_VER).tar.bz2

clean cleaner:
	@if [ -d graph500 ]; then \
	     $(MAKE) -s -C graph500 clean; \
	fi

distclean:
	rm -rf graph500 graph500-$(G500_VER).tar.bz2
