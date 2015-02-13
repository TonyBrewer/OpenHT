#!/bin/sh

unset JAVA_HOME
unset PERL5LIB
unset PYTHON PYTHON_DIR PYTHON_VER

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
