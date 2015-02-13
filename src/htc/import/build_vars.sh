#!/bin/sh

unset JAVA_HOME
unset PERL5LIB
unset PYTHON PYTHON_DIR PYTHON_VER

BOOST_INSTALL=${BOOST_INSTALL:-local_boost}
ROSE_INSTALL=${ROSE_INSTALL:-local_rose}
echo BOOST_INSTALL = $BOOST_INSTALL
echo ROSE_INSTALL  = $ROSE_INSTALL

export LD_LIBRARY_PATH=$BOOST_INSTALL/lib
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$ROSE_INSTALL/lib
# Look for java
if [ -d /usr/lib/jvm ]; then
  _JVM=`find /usr/lib/jvm -name libjvm.so |\
        sed 's#/libjvm.so$##' | sort | tail -1`
  if [ -n "$_JVM" ]; then
    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$_JVM
  fi
fi

PATH=/sbin:/bin:/usr/sbin:/usr/bin

if [ -f /etc/redhat-release ]; then
  OS_VER=`sed 's/[^0-9]*\(.\).*/\1/' /etc/redhat-release`
  if [ $OS_VER -lt 6 ]; then
    PREFIX=/sw/Local/c5x
    PATH=$PREFIX/bin:$PREFIX/gcc-4.4.7/bin:$PATH
    LD_LIBRARY_PATH=$PREFIX/lib:$LD_LIBRARY_PATH
    LD_LIBRARY_PATH=$PREFIX/lib64:$LD_LIBRARY_PATH
  fi
elif [ -f /etc/lsb-release ]; then
  OS_VER=`grep RELEASE /etc/lsb-release | sed 's/^.*=//'`
  export CXX=g++-4.4
fi
