
all: cnyleved levstrcmp

CC = g++
CFLAGS = -O2 -Wall -g -I. -fopenmp -Winline

LIBS = -lm -lpthread 

export VERSION = 2.3
export SVNREV = $(shell svn info | grep Revis | awk -F: '{print $$2}' | sed 's/ //g')

CFLAGS += -DVERSION="\"$(VERSION)\""
CFLAGS += -DSVNREV="\"$(SVNREV)\""

levstrcmp: levstrcmp.o procname.o
	$(CC) $(CFLAGS) levstrcmp.o procname.o -o $@ $(LIBS)
	cp -pu $@ bin

.cpp.o:
	$(CC) $(CFLAGS) $< -c

levstrcmp.o: levstrcmp.h
procname.o: levstrcmp.h

cnyleved: ../app
	cp -pu ../app cnyleved
	cp -pu cnyleved bin

../app::
	cd ..; $(MAKE) app

clean:
	$(RM) levstrcmp.o procname.o levstrcmp cnyleved core.* bin/cnyleved bin/levstrcmp

cleanall:
	$(MAKE) clean

allclean:
	$(MAKE) cleanall
