
all: cnyleved levstrcmp

CC = g++
CFLAGS = -O2 -Wall -g -I. -fopenmp -Winline

LIBS = -lm -lpthread 

export VERSION = 2.3
export SVNREV = $(shell svn info | grep Revis | awk -F: '{print $$2}' | sed 's/ //g')

CFLAGS += -DVERSION="\"$(VERSION)\""
CFLAGS += -DSVNREV="\"$(SVNREV)\""

levstrcmp: bin levstrcmp.o procname.o
	$(CC) $(CFLAGS) levstrcmp.o procname.o -o $@ $(LIBS)
	cp -pu $@ bin

.cpp.o:
	$(CC) $(CFLAGS) $< -c

levstrcmp.o: levstrcmp.h
procname.o: levstrcmp.h

cnyleved: bin ../app
	cp -pu ../app cnyleved
	cp -pu cnyleved bin

../app:
	cd ..; $(MAKE) app

bin:
	mkdir -p $@

clean:
	rm -rf bin *.o levstrcmp cnyleved core.*

cleanall:
	$(MAKE) clean

allclean:
	$(MAKE) cleanall
