
CXX = g++
CXXFLAGS = -Wall -Wextra -Wpedantic -Iinclude -I../ZeroEngine/include
LDFLAGS = -lraylib

SRC_DIR = src

SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(SRCS:.cpp=.o)

ENGINE_LIB = ../ZeroEngine/.build/debug/libZeroEngine.a

OUT = ZeroRuntime

all: $(OUT)

$(OUT): $(OBJS) $(ENGINE_LIB)
	$(CXX) $(OBJS) $(ENGINE_LIB) $(LDFLAGS) -o $(OUT)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(OUT)
