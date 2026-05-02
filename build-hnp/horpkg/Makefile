include ../utils/Makefrag

REAL_BINARY_NAME = horpkg

EXCLUDE_SRCS := src/core_hnp_installer.c
SRCS := $(filter-out $(EXCLUDE_SRCS),$(wildcard src/*.c))
BUILD_ROOT := build$(PREFIX)
BIN_DIR := $(BUILD_ROOT)/horpkgbin

RUNTIME_PIN ?= $(HORPKG_RUNTIME_PIN)
ifeq ($(strip $(RUNTIME_PIN)),)
RUNTIME_PIN := 314159
endif
CFLAGS += -DHORPKG_RUNTIME_PIN=\"$(RUNTIME_PIN)\"

SHIM_NAME := horpkg
SHIM_CONFIG_PATH := $(shell pwd)/shim/config.h

.PHONY: all clean prep

all: prep $(BIN_DIR)/$(REAL_BINARY_NAME)
	@mkdir -p $(BUILD_ROOT)/etc
	cp -fv src/cacert.pem $(BUILD_ROOT)/etc/
	
	@echo "==> [HORPKG] Building Shim..."
	$(MAKE) -C ../shim install SHIM_NAME=$(SHIM_NAME) CONFIG_PATH=$(SHIM_CONFIG_PATH)

	mkdir -p ../sysroot
	@if command -v rsync >/dev/null 2>&1; then \
		rsync -a --delete build/$(PREFIX)/ ../sysroot > file.lst; \
	else \
		echo "rsync not found, falling back to cp -a (without delete support)." >&2; \
		cp -afv build/$(PREFIX)/. ../sysroot | tee file.lst; \
	fi

prep:
	rm -rf $(BUILD_ROOT)

$(BIN_DIR)/$(REAL_BINARY_NAME): $(SRCS)
	mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -Wno-unused-parameter -std=c11 \
		-o $@ \
		$(SRCS) \
		-I../sysroot/include \
		-L../sysroot/lib -lcurl -lyyjson -lzip

clean:
	rm -rf build
	rm -f file.lst
	$(MAKE) -C ../shim clean
