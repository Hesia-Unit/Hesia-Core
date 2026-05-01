CC ?= cc
AR ?= ar
CFLAGS ?= -O2 -g
CPPFLAGS ?=
LDFLAGS ?=
LDLIBS ?=

INCLUDES := -Iinclude
WARNINGS := -Wall -Wextra -Wpedantic

ifeq ($(OS),Windows_NT)
	LDLIBS += -lbcrypt
	SHARED_EXT := dll
else
	SHARED_EXT := so
endif

.PHONY: all test clean

all: build/libhesia_core.a build/libhesia_core.$(SHARED_EXT)

build:
	mkdir -p build

build/hesia_core.o: src/hesia_core.c include/hesia/core.h | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARNINGS) $(INCLUDES) -c $< -o $@

build/libhesia_core.a: build/hesia_core.o
	$(AR) rcs $@ $<

build/libhesia_core.$(SHARED_EXT): src/hesia_core.c include/hesia/core.h | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARNINGS) $(INCLUDES) -shared $< -o $@ $(LDFLAGS) $(LDLIBS)

build/test_core: tests/test_core.c build/libhesia_core.a
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARNINGS) $(INCLUDES) $< build/libhesia_core.a -o $@ $(LDFLAGS) $(LDLIBS)

test: build/test_core
	./build/test_core

clean:
	rm -rf build
