CC ?= cc
AR ?= ar
CFLAGS ?= -O2 -g
CPPFLAGS ?=
LDFLAGS ?=
LDLIBS ?=
WITH_OPENSSL ?= 0
WITH_LIBOQS ?= 0
DEPS_ROOT ?= third_party/install/linux-$$(uname -m)
OPENSSL_PREFIX ?= $(DEPS_ROOT)/openssl
LIBOQS_PREFIX ?= $(DEPS_ROOT)/liboqs

INCLUDES := -Iinclude
WARNINGS := -Wall -Wextra -Wpedantic
PROVIDER_CPPFLAGS :=
PROVIDER_LDLIBS :=

ifeq ($(WITH_OPENSSL),1)
	PROVIDER_CPPFLAGS += -DHESIA_CORE_WITH_OPENSSL -I$(OPENSSL_PREFIX)/include
	PROVIDER_LDLIBS += $(OPENSSL_PREFIX)/lib/libcrypto.a -ldl -pthread
endif

ifeq ($(WITH_LIBOQS),1)
	PROVIDER_CPPFLAGS += -DHESIA_CORE_WITH_LIBOQS -I$(LIBOQS_PREFIX)/include
	PROVIDER_LDLIBS += $(LIBOQS_PREFIX)/lib/liboqs.a
endif

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
	$(CC) $(CPPFLAGS) $(PROVIDER_CPPFLAGS) $(CFLAGS) $(WARNINGS) $(INCLUDES) -c $< -o $@

build/ascon.o: src/algorithms/ascon.c src/algorithms/ascon.h | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARNINGS) -c $< -o $@

build/libhesia_core.a: build/hesia_core.o build/ascon.o
	$(AR) rcs $@ $^

build/libhesia_core.$(SHARED_EXT): src/hesia_core.c src/algorithms/ascon.c include/hesia/core.h | build
	$(CC) $(CPPFLAGS) $(PROVIDER_CPPFLAGS) $(CFLAGS) $(WARNINGS) $(INCLUDES) -shared $(filter %.c,$^) -o $@ $(LDFLAGS) $(PROVIDER_LDLIBS) $(LDLIBS)

build/test_core: tests/test_core.c build/libhesia_core.a
	$(CC) $(CPPFLAGS) $(PROVIDER_CPPFLAGS) $(CFLAGS) $(WARNINGS) $(INCLUDES) $< build/libhesia_core.a -o $@ $(LDFLAGS) $(PROVIDER_LDLIBS) $(LDLIBS)

build/test_providers: tests/test_providers.c build/libhesia_core.a
	$(CC) $(CPPFLAGS) $(PROVIDER_CPPFLAGS) $(CFLAGS) $(WARNINGS) $(INCLUDES) $< build/libhesia_core.a -o $@ $(LDFLAGS) $(PROVIDER_LDLIBS) $(LDLIBS)

build/test_ascon_kat: tests/test_ascon_kat.c src/algorithms/ascon.c | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARNINGS) -Isrc $^ -o $@

test: build/test_core build/test_ascon_kat
	./build/test_core
	./build/test_ascon_kat tests/kat/ascon128v12/LWC_AEAD_KAT_128_128.txt

test-providers: build/test_providers
	./build/test_providers

clean:
	rm -rf build
