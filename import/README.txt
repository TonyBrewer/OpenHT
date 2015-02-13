# Retrieve source
http://www.accellera.org/downloads/standards/systemc
https://github.com/yTakatsukasa/misc/tree/master/systemc-2.3_vcd_hier


# sanitize environment
unset JAVA_HOME
unset PERL5LIB
unset PYTHON PYTHON_DIR PYTHON_VER
unset LD_LIBRARY_PATH
PATH=/sbin:/bin:/usr/sbin:/usr/bin

# build lib (cwd == ./downloads)

BDIR=/dev/shm/systemc
rm -rf $BDIR; mkdir $BDIR

VER=2.3.1

rm -rf systemc-$VER
tar -zxf systemc-$VER.tgz
(cd systemc-$VER > /dev/null; \
	for p in `cat ../patches-$VER/order.txt`; do \
	    patch -p0 -i ../patches-$VER/$p; \
	done)

#rsync -rav --delete --filter='-rs_*/.svn*' systemc-$VER/src ../systemc-$VER

cp -rp systemc\-$VER $BDIR
(cd $BDIR/systemc\-$VER > /dev/null; \
	./configure; \
	make install -j OPT_CXXFLAGS="-g -O2")

rsync -rav --delete --filter='-rs_*/.svn*' $BDIR/systemc-$VER/include ../systemc-$VER
rsync -rav --delete --filter='-rs_*/.svn*' $BDIR/systemc-$VER/lib-linux64 ../systemc-$VER
#rsync -rav --delete --filter='-rs_*/.svn*' $BDIR/systemc-$VER/lib-macosx64 ../systemc-$VER

rm -rf systemc-$VER $BDIR
