CXX = g++
CXXFLAGS = -std=c++17 -Wall -I./src -I/usr/include -I./src/common
LDFLAGS = -lboost_program_options

COMMON_SRC = $(wildcard src/common/*.cpp)
COMMON_OBJ = $(COMMON_SRC:.cpp=.o)

COMPILER_SRC = $(wildcard src/compiler/*.cpp)
COMPILER_OBJ = $(COMPILER_SRC:.cpp=.o)
COMPILER_BIN = build/compiler_bin

BCDUMP_SRC = $(wildcard src/bcdump/*.cpp)
BCDUMP_OBJ = $(BCDUMP_SRC:.cpp=.o)
BCDUMP_BIN = build/bcdump

#DEBUGGER_SRC = $(wildcard src/debugger/*.cpp)
#DEBUGGER_OBJ = $(DEBUGGER_SRC:.cpp=.o)
#DEBUGGER_BIN = debugger_bin

#RUNTIME_SRC = $(wildcard src/runtime/*.cpp)
#RUNTIME_OBJ = $(RUNTIME_SRC:.cpp=.o)
#RUNTIME_BIN = runtime_bin

.PHONY: all clean

all: $(COMPILER_BIN) $(BCDUMP_BIN) $(DEBUGGER_BIN) $(RUNTIME_BIN)

# Compiler
$(COMPILER_BIN): $(COMMON_OBJ) $(COMPILER_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(BCDUMP_BIN): $(COMMON_OBJ) $(BCDUMP_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Debugger
#$(DEBUGGER_BIN): $(COMMON_OBJ) $(DEBUGGER_OBJ)
#	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Runtime
#$(RUNTIME_BIN): $(COMMON_OBJ) $(RUNTIME_OBJ)
#	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(COMMON_OBJ) $(COMPILER_OBJ) $(DEBUGGER_OBJ) $(RUNTIME_OBJ) \
		$(COMPILER_BIN) $(DEBUGGER_BIN) $(RUNTIME_BIN)