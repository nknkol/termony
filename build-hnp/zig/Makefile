include ../utils/Makefrag

ZIG_VERSION := 0.15.2
ZIG_ARCHIVE := zig-aarch64-linux-$(ZIG_VERSION).tar.xz
ZIG_URL := https://ziglang.org/download/$(ZIG_VERSION)/$(ZIG_ARCHIVE)
ZIG_TOP_DIR := zig-aarch64-linux-$(ZIG_VERSION)
ZIG_DOWNLOAD := download/$(ZIG_ARCHIVE)

SHIM_NAME := zig
SHIM_CONFIG_PATH := $(shell pwd)/shim/config.h

.PHONY: all clean

all: $(ZIG_DOWNLOAD)
	rm -rf build
	mkdir -p build
	tar -xf $(ZIG_DOWNLOAD) -C build
	if [ ! -d "build/$(ZIG_TOP_DIR)" ]; then \
		echo "Unexpected Zig archive layout. Expected $(ZIG_TOP_DIR)."; \
		exit 1; \
	fi
	mv build/$(ZIG_TOP_DIR) build/zig-root
	mkdir -p build$(PREFIX)/opt
	cp -rf build/zig-root build$(PREFIX)/opt/zig

	mkdir -p ../sysroot
	cp -rfv build/$(PREFIX)/. ../sysroot > file.lst

	@if [ ! -f "$(SHIM_CONFIG_PATH)" ]; then \
		echo "ERROR: Shim config file not found at $(SHIM_CONFIG_PATH)" >&2; \
		exit 1; \
	fi
	$(MAKE) -C ../shim install SHIM_NAME=$(SHIM_NAME) CONFIG_PATH=$(SHIM_CONFIG_PATH)

$(ZIG_DOWNLOAD):
	mkdir -p download
	$(WGET) -O $@ $(ZIG_URL)

clean:
	rm -rf build temp download $(ZIG_DOWNLOAD) file.lst
