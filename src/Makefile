include ../common.mk
INCLUDES = -I. -I../$(WT_HOME) -I../$(WT_HOME)/src/include
LD_FLAGS= ../$(WT_HOME)/.libs/libwiredtiger.a
#LIBS = -lrt -ldl -lpthread
# OBJECTS =  common.o standard_graph.o graph_main.o
# SRCS := $(patsubst %.o,%.cpp,$(OBJECTS))

OBJECTS = $(wildcard *.cpp)
SRCS = $(src:.cpp=.o)
TARGET := graph_api

all: $(TARGET)


%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< $(LD_FLAGS) $(LIBS) 

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^ $(LD_FLAGS) $(LIBS)