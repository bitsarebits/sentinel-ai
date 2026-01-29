# --- 1. Variables ---

# Compiler to use
CC = gcc

# Paths to ONNX Runtime
# (Adjust this if your folder name is slightly different)
ONNX_BASE = lib/onnxruntime-linux-x64-1.23.2
ONNX_INC = $(ONNX_BASE)/include
ONNX_LIB = $(ONNX_BASE)/lib

# Base Compiler Flags (Common to both Debug and Release)
# -D_DEFAULT_SOURCE: Fixes the "u_int" errors in libpcap
# -Iinclude: Look for header files (.h) in the 'include' folder
# -Wall -Wextra: Enable all warnings (helps find bugs)
# -g: Add debug info (for GDB/Valgrind)
# -pthread: Enable multithreading support
BASE_CFLAGS = -Iinclude -Wall -Wextra -pthread -std=c11 -D_DEFAULT_SOURCE -I$(ONNX_INC)


# --- BUILD MODES ---

# Default CFLAGS = Debug Mode
# We include -DDEBUG by default so 'make run' and 'make train' always show logs.
# -L$(ONNX_LIB) tells linker where to find .so files at build time
# -lonnxruntime links the library
# -Wl,-rpath,$(ONNX_LIB) tells the Linux loader where to find .so files at RUN time
CFLAGS = $(BASE_CFLAGS) -g -O0 -DDEBUG

# Linker Flags:
# -lpcap: Link against the libpcap library
# -pthread: Link against the pthread library
LDFLAGS = -lpcap -pthread -L$(ONNX_LIB) -lonnxruntime -Wl,-rpath,$(ONNX_LIB)

# Release Target: Explicitly Overwrites CFLAGS
# Usage: make release
# This removes -DDEBUG, -g, and -O0, and adds -O3 for optimization.
release: CFLAGS = $(BASE_CFLAGS) -O3
release: clean all

# Debug Target: Redundant, but kept for habit.
# It ensures a clean build with default flags.
debug: clean all

# Directory Layout
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
TEST_DIR = tests

# Target Executable Name
TARGET = $(BIN_DIR)/sentinel-engine

# --- 2. File Discovery ---

# Find all .c files in src/ (e.g., src/main.c src/sniffer.c)
SRCS = $(wildcard $(SRC_DIR)/*.c)

# Generate corresponding .o file names in obj/ (e.g., obj/main.o obj/sniffer.o)
# logic: take SRCS, replace 'src/' with 'obj/' and '.c' with '.o'
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

# --- 3. Rules ---

# Default target (runs when you type 'make')
all: create_dirs $(TARGET)

# Linking Rule: How to build the final executable
# Depends on $(OBJS). If any .o changes, re-link.
$(TARGET): $(OBJS)
	@echo "Linking $(TARGET)..."
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

# Compilation Rule: How to build a .o from a .c
# $@ matches the target (the .o file)
# $< matches the first dependency (the .c file)
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Create directories if they don't exist
create_dirs:
	@mkdir -p $(OBJ_DIR) $(BIN_DIR)

# Clean Rule: Remove generated files
clean:
	@echo "Cleaning up..."
	rm -rf $(OBJ_DIR) $(BIN_DIR)

# Standard Run (Default: Sniffer Mode)
run: all
	@echo "Starting Sentinel AI (Sniffer Mode)..."
	sudo ./$(TARGET)

# Data Collection Mode
train: all
	@echo "Starting Sentinel AI (Training Mode)..."
	@echo "Writing data to training_data.csv..."
	sudo ./$(TARGET) --train

# Guard Mode (AI Inference)
guard: all
	@echo "Starting Sentinel AI (Guard Mode)..."
	sudo ./$(TARGET) --guard


# --- 4. Testing Rules ---

# Define the test executables
TEST_PARSER_BIN = $(BIN_DIR)/test_parsers
TEST_RINGBUFFER_BIN = $(BIN_DIR)/test_ringbuffer

# define dependencies for the parser test (The Test + The Implementation + The Utils)
TEST_PARSER_DEPS = $(OBJ_DIR)/parser.o $(OBJ_DIR)/utils.o

# Main test target: Compiles AND Runs all tests
test: create_dirs $(TEST_PARSER_BIN) $(TEST_RINGBUFFER_BIN)
	@echo "\n======================================"
	@echo "   RUNNING SENTINEL AI TEST SUITE"
	@echo "======================================\n"
	@echo ">>> Executing Ring Buffer Tests..."
	@./$(TEST_RINGBUFFER_BIN)
	@echo "\n>>> Executing Parser Tests..."
	@./$(TEST_PARSER_BIN)
	@echo "\n======================================"
	@echo "          ALL TESTS COMPLETED"
	@echo "======================================"

# Compile Parser Tests (LINKING FIX HERE)
$(TEST_PARSER_BIN): $(TEST_DIR)/test_parsers.c $(TEST_PARSER_DEPS)
	@echo "Compiling Parser Tests..."
	$(CC) $(CFLAGS) $(TEST_DIR)/test_parsers.c $(TEST_PARSER_DEPS) -o $@ $(LDFLAGS)

# Compile Ring Buffer Tests
# Ring buffer needs common.h but the implementation is mostly header-based or in main (for now). 
# If ringbuffer logic moves to a .c file, link it here too.
$(TEST_RINGBUFFER_BIN): $(TEST_DIR)/test_ringbuffer.c $(OBJ_DIR)/ringBuffer.o $(OBJ_DIR)/utils.o
	@echo "Compiling Ring Buffer Tests..."
	$(CC) $(CFLAGS) $(TEST_DIR)/test_ringbuffer.c $(OBJ_DIR)/ringBuffer.o $(OBJ_DIR)/utils.o -o $@ $(LDFLAGS)


# Phony targets (not real files)
.PHONY: all clean run create_dirs