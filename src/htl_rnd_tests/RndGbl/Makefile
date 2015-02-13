EXE = rndtest

CSRC = $(wildcard *.cpp)
OBJS = $(CSRC:.cpp=.o)
DEPS = $(CSRC:.cpp=.d)

CXXFLAGS += -O2 -m64
CXXFLAGS += -Wall -Wno-parentheses -Wno-write-strings
CFLAGS = $(CXXFLAGS)

all: $(EXE)

$(EXE): $(OBJS)
	$(CXX) -o $@ $(OBJS)

clean:
	rm -f $(EXE) $(OBJS) $(DEPS)

distclean:
	rm -f $(EXE) $(OBJS) $(DEPS)
	rm -rf Issues
	$(MAKE) -s -C TestDir reap

#
# Automatic Prerequisites
#
%.d: %.cpp
	@set -e; rm -f $@;\
	    $(CC) -M $(CPPFLAGS) $< > $@.$$$$;\
	    sed 's,\($(notdir $*\))\.o[ :]*,$(dir $<)\1.o $@ : ,g' < $@.$$$$ > $@;\
	    rm -f $@.$$$$

-include $(DEPS)
