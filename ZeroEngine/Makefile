# ZeroEngine/Makefile
CXX = g++
CXXFLAGS = -Wall -Wextra -Wpedantic -O2 -Iinclude -fPIC
AR = ar
ARFLAGS = rcs

SRC_DIR = src
BUILD_DIR = .build/debug
OBJ_DIR = $(BUILD_DIR)/obj

# Find all cpp files recursively
SRCS = $(shell find $(SRC_DIR) -name '*.cpp')
OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))

TARGET = $(BUILD_DIR)/libZeroEngine.a

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(BUILD_DIR)
	$(AR) $(ARFLAGS) $@ $^
	@echo "Built $@"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
