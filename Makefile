# Simple Makefile to build ConfigCreator
# Usage:
#   make         - builds ConfigCreator.exe (or ConfigCreator on non-windows)
#   make clean   - remove build artifacts

CXX ?= g++
# Use project root as include path so <nlohmann/json.hpp> is found
CXXFLAGS ?= -std=c++17 -O2 -Wall -I.
# Older libstdc++ versions require linking stdc++fs for <filesystem>
LDFLAGS ?= -pthread -lstdc++fs

OUT := ConfigCreator.exe
SRCS := $(wildcard *.cpp)

.PHONY: all clean run

all: $(OUT)

$(OUT): $(SRCS)
	@echo Building $(OUT) with $(CXX)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(OUT) $(LDFLAGS)

clean:
	rm -f $(OUT) *.o

run: $(OUT)
	./$(OUT)
