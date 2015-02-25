
OPT_FLAGS	?= -O2

CXX		= g++
LINK		= g++
CXXFLAGS	= $(OPT_FLAGS) -Wall
LFLAGS		=  -pthread


TARGET	= test_client

SOURCES = Client.cpp Thread.cpp Ascii.cpp Binary.cpp mtrand.cpp
OBJECTS = $(SOURCES:.cpp=.o)
DEP     = $(SOURCES:.cpp=.d)

all:	$(TARGET)

debug:
	$(MAKE) OPT_FLAGS=-ggdb

clean:
	rm -f *.o *.d $(TARGET)

-include $(DEP)

.SUFFIXES: .o .cpp

.cpp.o:
	$(CXX) -MM -MT $@ $(CXXFLAGS)  $*.cpp > $*.d
	$(CXX) -c $(CXXFLAGS) $< -o $@

$(TARGET):	$(OBJECTS)
	$(LINK) $(LFLAGS) $(OBJECTS) $(LIBS) -o $(TARGET)


