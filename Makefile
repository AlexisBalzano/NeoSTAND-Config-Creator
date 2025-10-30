# Simple Makefile to build ConfigCreator
# Usage:
#   make         - builds ConfigCreator.exe (or ConfigCreator on non-windows)
#   make clean   - remove build artifacts


CXX ?= g++
# Use project root as include path so <nlohmann/json.hpp> is found
CXXFLAGS ?= -std=c++17 -O2 -Wall -I.

# Detect platform and set linker flags for std::filesystem support when needed.
UNAME_S := $(shell uname -s 2>/dev/null)
ifeq ($(UNAME_S),Linux)
	# Some older libstdc++ require linking libstdc++fs
	LDFLAGS := -pthread -lstdc++fs
else ifeq ($(UNAME_S),Darwin)
	# macOS libc++ typically provides filesystem; no extra lib required
	LDFLAGS := -pthread
else
	# Default conservative flags
	LDFLAGS := -pthread
endif

# Output filename: use .exe on Windows/MSYS/Cygwin, plain name on Unix
UNAME_S := $(shell uname -s 2>/dev/null)
OUT := ConfigCreator
ifneq ($(findstring MINGW,$(UNAME_S)),)
	OUT := ConfigCreator.exe
endif
ifneq ($(findstring CYGWIN,$(UNAME_S)),)
	OUT := ConfigCreator
endif

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
