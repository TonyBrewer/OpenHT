
SHELL=/bin/sh
.SUFFIXES:
.SUFFIXES: .c .h .o

CFLAGS ?=
.PHONY: all clean debug release

sources := client_async.c client.c config.c connection.c mc-hammr.c stats.c thread.c util.c
headers := client_async.h client.h config.h connection.h mc-hammr.h protocol_binary.h stats.h thread.h types.h util.h

objects := $(sources:%.c=%.o)



release: CFLAGS+=-O3
release: mc-hammr

debug: CFLAGS+=-g -O0 -DTIMEOUT_DEBUG
debug: mc-hammr

mc-hammr: $(objects) $(HDRS)
	$(CC) $(CFLAGS) -o mc-hammr $(objects) -lrt -lpthread

$(objects): %.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@ 

clean:
	rm -f mc-hammr $(objects)
