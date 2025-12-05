CXX = g++
CXXFLAGS = -Wall -Wextra -Wpedantic -Iinclude -I../ZeroEngine/include -std=c++17
LIBS = -lraylib -langelscript -lGL -lm -lpthread -ldl -lrt -lX11

SRC_DIR = src

SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(SRCS:.cpp=.o)

ENGINE_LIB = ../ZeroEngine/.build/debug/libZeroEngine.a

OUT = ZeroRuntime

all: $(OUT)

$(OUT): $(OBJS) $(ENGINE_LIB)
	$(CXX) -o $(OUT) $(OBJS) $(ENGINE_LIB) $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(OUT)

.PHONY: all clean
