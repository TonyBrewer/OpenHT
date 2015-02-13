#!/bin/sh

for l in `find local_rose -type l`; do
    ld=`dirname $l`
    lf=`echo $l | sed 's/^.*\///'`
    sf=`ls -l $l | awk '{print $11}'`
    if [ "${sf:0:1}" = "/" ] ; then
	echo "Importing $sf"
	(cd $ld > /dev/null; rm $lf; cp -p $sf .)
    fi
done

./patchelf --set-rpath "" local_rose/lib/librose.so
