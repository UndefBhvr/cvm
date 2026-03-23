# Compiler and flags
CXX := g++
CXXFLAGS := -Wall -Wextra -std=c++23 -O2
LDFLAGS :=

# Directories
SRC_DIR := src
BUILD_DIR := build

# Target executable
TARGET := $(BUILD_DIR)/cvm

# Find all source files
SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(SRCS:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

# Default target
all: $(TARGET)

# Link object files to create executable
$(TARGET): $(OBJS) | $(BUILD_DIR)
	$(CXX) $(LDFLAGS) -o $@ $^

# Compile source files to object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Create directories if they don't exist
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)


# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)


.PHONY: all clean