G500_URL=http://www.graph500.org/sites/default/files/files
G500_VER=2.1.4

graph500:
	wget $(G500_URL)/graph500-$(G500_VER).tar.bz2
	bunzip2 -c *.bz2 | tar -xf -
	patch -p0 -i graph500-$(G500_VER).patch
	mv graph500-$(G500_VER) $@

clean:
	rm -f graph500-$(G500_VER).tar.bz2
	rm -rf graph500
