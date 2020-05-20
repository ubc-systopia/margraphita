include ../common.mk
INCLUDES = -I../$(WT_HOME) -I../$(WT_HOME)/src/include
LD_FLAGS= ../$(WT_HOME)/.libs/libwiredtiger.a
LIBS = -lrt -ldl -lpthread
OBJECTS =  common.o standard_graph.o graph_main.o
SRCS := $(patsubst %.o,%.cpp,$(OBJECTS))
TARGET := graph_api

all: $(TARGET)
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^ $(LD_FLAGS) $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< $(LD_FLAGS) $(LIBS) 


#graph_main : $(OBJECTS)
#	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LD_FLAGS) $(LIBS) -o $@

#$(OBJECTS):
# 	$(CXX) $(CXXFLAGS) -o $@ $^ $(INCLUDES) $(LD_FLAGS) $(LIBS)
