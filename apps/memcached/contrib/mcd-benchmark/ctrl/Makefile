SHELL=/bin/sh
.SUFFIXES:
.SUFFIXES: .c .h .o

CFLAGS=-g -O2
.PHONY: all clean

execs := mcast_msg mcast_wait

all: $(execs)

$(execs): %: %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(execs)
