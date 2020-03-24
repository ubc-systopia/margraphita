include ../common.mk
INCLUDES = -I../$(WT_HOME) -I../$(WT_HOME)/src/include
LD_FLAGS= ../$(WT_HOME)/.libs/libwiredtiger.a
LIBS = -lrt -ldl -lpthread
OBJECTS = graph_main

all: $(OBJECTS)

$(OBJECTS): %: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(INCLUDES) $(LD_FLAGS) $(LIBS)