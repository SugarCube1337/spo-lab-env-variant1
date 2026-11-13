CC = gcc
CFLAGS = -std=c11 -g3 -O0 -Wall -Wextra -Wpedantic -Wshadow -Wconversion
CPPFLAGS =
LDFLAGS =
LDLIBS =

ifeq ($(OS),Windows_NT)
  PLATFORM := windows
  EXE := .exe
else
  PLATFORM := linux
  EXE :=
endif

BUILD_DIR := build/$(PLATFORM)
TARGET := $(BUILD_DIR)/spo-lab$(EXE)
SOURCES := $(wildcard src/*.c)

.DEFAULT_GOAL := all
.PHONY: all check run debug clean

all: $(TARGET)

$(TARGET): $(SOURCES) Makefile | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SOURCES) -o $@ $(LDFLAGS) $(LDLIBS)

$(BUILD_DIR):
	mkdir -p $@

check: all
	$(TARGET) 3

run: all
	$(TARGET) $(ARGS)

debug: all
	gdb --args $(TARGET) 3

clean:
	rm -rf build
