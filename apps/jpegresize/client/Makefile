EXEC=jrcl

SRCS=$(shell ls *.cpp)
OBJS=$(SRCS:.cpp=.o)


CPP_FLAGS += -g -O2 -std=gnu++0x -DENABLE_DAEMON -Wno-deprecated-declarations
CPP_FLAGS += -I./ -I$(CURDIR)/../import/local/include

LDFLAGS += -L./ -L$(CURDIR)/../import/local/lib
LDLIBS += -ldl -lmpio -lmsgpack -lmsgpack-rpc -lpthread


all: import
	$(MAKE) $(EXEC)

.PHONY: import
import:
	cd ../import && $(MAKE)

%.o: %.cpp %.hpp
	g++ -m64 -c -fPIC $(CPP_FLAGS) $*.cpp -o $@	

$(EXEC): $(OBJS)
	g++ $(LDFLAGS) $(OBJS) $(LDLIBS) -o $(EXEC)


clean:
	rm -f *.o $(EXEC)

distclean: clean
	cd ../import && $(MAKE) clean
