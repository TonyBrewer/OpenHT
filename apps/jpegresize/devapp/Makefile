EXEC=devapp
HT_TGT?=app

SRCS=$(shell ls *.cpp)
OBJS=$(SRCS:.cpp=.o)

CPP_FLAGS += -DHT_$(shell echo $(HT_TGT) | tr '[:lower:]' '[:upper:]')
CPP_FLAGS += -I/opt/convey/pdk2/ht/ht_lib
CPP_FLAGS += -I. -I../lib_pers/src -I../lib_pers/src_pers
CPP_FLAGS += -I../lib_pers/src_$(HT_TGT) -I../lib_pers/ht/$(HT_TGT)

ifneq ($(findstring model,$(HT_TGT)),)
CFLAGS += -I$(CURDIR)
endif
ifneq ($(findstring sysc,$(HT_TGT)),)
CPP_FLAGS += -I/opt/convey/pdk2/ht/systemc-2.3.1/include/
endif
ifneq ($(findstring vsim,$(HT_TGT)),)
CPP_FLAGS += -I/opt/convey/include
LDFLAGS += -L/opt/convey/lib
LDLIBS += -lwx_sim_runtime -lcpSimLib2_wx
endif
ifneq ($(findstring app,$(HT_TGT)),)
CPP_FLAGS += -I/opt/convey/include
LDFLAGS += -L/opt/convey/lib
LDLIBS += -lwx_runtime -ldl
endif
LDLIBS += ../lib_pers/libht$(HT_TGT).a -lpthread


ifdef ENABLE_DAEMON
RPCLIBDIR ?= /opt/convey/cnyjpegresize/lib
CPP_FLAGS += -DENABLE_DAEMON -I$(RPCLIBDIR)/include  -fopenmp
LD_FLAGS += -L$(RPCLIBDIR)/lib
LDLIBS+=-lmsgpack -lmsgpack-rpc -lmpio 
endif

ifdef MAGICK_FALLBACK
CPP_FLAGS += -DMAGICK_FALLBACK
LDLIBS += -lMagick++ -lMagickCore
endif

all: $(EXEC)

main.o: %.o : %.cpp 
	g++ -m64 -c -fPIC $(CPP_FLAGS) $*.cpp -o $@

%.o: %.cpp 
	g++ -m64 -c -fPIC $(CPP_FLAGS) $*.cpp -o $@	

$(OBJS): ../lib_pers/libht$(HT_TGT).a

$(EXEC): $(OBJS)
	g++ $(LDFLAGS) $(OBJS) $(LDLIBS) -o $(EXEC)

../lib_pers/libht$(HT_TGT).a:
	cd ../lib_pers && $(MAKE) lib$(HT_TGT)

distclean clean:
	$(MAKE) -C ../lib_pers $@
	rm -f *.o $(EXEC)
