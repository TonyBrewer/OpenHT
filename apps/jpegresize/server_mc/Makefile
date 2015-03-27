EXEC=jrd
HT_TGT?=app

SRCS=$(shell ls *.cpp)
OBJS=$(SRCS:.cpp=.o)


CPP_FLAGS += -g -O2 -std=gnu++0x -DENABLE_DAEMON
CPP_FLAGS += -Wno-deprecated-declarations
CPP_FLAGS += -I../import/local/include -I/usr/include/ImageMagick
CPP_FLAGS += -DHT_$(shell echo $(HT_TGT) | tr '[:lower:]' '[:upper:]')
CPP_FLAGS += -I/opt/convey/pdk2/ht/ht_lib
CPP_FLAGS += -I../lib_pers/src -I../lib_pers/ht/$(HT_TGT)

LDFLAGS += -L./ -L../import/local/lib
LDLIBS += -ldl -lmsgpack -lmsgpack-rpc -lmpio -lMagick++ -lMagickCore

ifneq ($(findstring sysc,$(HT_TGT)),)
CPP_FLAGS += -I/opt/convey/pdk2/ht/systemc-2.3.1/include/
endif
ifneq ($(findstring vsim,$(HT_TGT)),)
export CPP_FLAGS += -I/opt/convey/include
LDFLAGS += -L/opt/convey/lib
LDLIBS += -lwx_sim_runtime -lcpSimLib2_wx
endif
ifneq ($(findstring app,$(HT_TGT)),)
export CPP_FLAGS += -I/opt/convey/include
LDFLAGS += -L/opt/convey/lib
LDLIBS += -lwx_runtime -ldl
endif
LDLIBS += ../lib_pers/libht$(HT_TGT).a -lpthread


all: import
	$(MAKE) $(EXEC)

.PHONY: import
import:
	cd ../import && $(MAKE)

%.o: %.cpp 
	g++ -m64 -c -fPIC $(CPP_FLAGS) $*.cpp -o $@	

$(OBJS): ../lib_pers/libht$(HT_TGT).a

$(EXEC): $(OBJS)
	g++ $(LDFLAGS) $(OBJS) $(LDLIBS) -o $(EXEC)

../lib_pers/libht$(HT_TGT).a:
	cd ../lib_pers && $(MAKE) lib$(HT_TGT)

clean:
	cd ../lib_pers && $(MAKE) $@
	rm -f *.o $(EXEC)

distclean: clean
	cd ../import && $(MAKE) clean
