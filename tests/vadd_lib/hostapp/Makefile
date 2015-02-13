EXE = vadd

CC     = $(CXX)
CSRCS  = main.c
CXXSRS =

OBJS  = $(CSRCS:.c=.o)
OBJS += $(CXXSRCS.cpp=.o)

CFLAGS   += -O2 -g -m64
CFLAGS   += -Wall -fno-strict-aliasing

../vsim.exe: CFLAGS += -DAE_PRESENT_CNT=1

CXXFLAGS += $(CFLAGS)

LDFLAGS = -m64
LDLIBS  = -lm -lpthread 

ifdef DRIVER_API
../vsim.exe $(EXE): LDLIBS += -L /opt/convey/lib -lwx_runtime
else
../vsim.exe $(EXE): LDLIBS += /opt/convey/lib/cny_initruntime.o
../vsim.exe $(EXE): LDLIBS += -L /opt/convey/lib -lcny_utils -lPersSupport
../vsim.exe $(EXE): LDLIBS += -ldl
endif

all: $(EXE) $(EXE)_sysc $(EXE)_model ../vsim.exe

$(EXE): $(OBJS) ../libhtapp.a
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(EXE)_sysc: $(OBJS) ../libhtsysc.a
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(EXE)_model: $(OBJS) ../libhtmodel.a
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

../vsim.exe: $(OBJS) ../libhtapp.a
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

../libhtapp.a:
	cd .. > /dev/null && $(MAKE) libapp

../libhtsysc.a:
	cd .. > /dev/null && $(MAKE) libsysc

../libhtmodel.a:
	cd .. > /dev/null && $(MAKE) libmodel

clean:
	rm -rf $(OBJS) $(EXE) $(EXE)_sysc $(EXE)_model ../vsim.exe HtMonRpt.txt
	cd .. > /dev/null && make clean
