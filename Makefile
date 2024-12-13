# Compiler and flags
CC := gcc
CFLAGS := -Wall -Wextra -O2 -I./include
LDFLAGS := -lm -lSDL2 -lSDL2_ttf

# Check OS for additional flags
ifeq ($(OS),Windows_NT)
    LDFLAGS += -lmingw32
else
    CFLAGS += $(shell sdl2-config --cflags) $(shell pkg-config --cflags SDL2_ttf)
    LDFLAGS += $(shell sdl2-config --libs) $(shell pkg-config --libs SDL2_ttf)
endif

# Directories
SRC_DIR := src
INC_DIR := include
BUILD_DIR := build
BIN_DIR := bin
FONT_DIR := fonts

# Source files
SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

# Target executable
TARGET := $(BIN_DIR)/xvm

# Default target
all: $(TARGET) $(FONT_DIR)

# Create directories
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Compile source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

# Link object files
$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# Include dependencies
-include $(DEPS)

# Clean build files
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

# Additional targets
.PHONY: all clean install-deps

# Install SDL2 helper target
install-deps:
ifeq ($(OS),Windows_NT)
	@echo "Please install SDL2 and SDL2_ttf manually on Windows"
else
	sudo apt-get install libsdl2-dev libsdl2-ttf-dev
endif