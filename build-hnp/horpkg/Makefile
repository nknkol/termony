include ../utils/Makefrag

REAL_BINARY_NAME = horpkg

SHIM_NAME = horpkg

SHIM_CONFIG_PATH = $(shell pwd)/shim/config.h

# LDFLAGS += -lcurl
# --- 目标 ---
.PHONY: all clean

all:
	rm -rf build
	
	mkdir -p build$(PREFIX)/horpkgbin
	mkdir -p build$(PREFIX)/etc
	mkdir -p build$(PREFIX)/share/horpkg/resources
	cp -fv src/cacert.pem build$(PREFIX)/etc/
	cp -rfv resources/. build$(PREFIX)/share/horpkg/resources/
	$(CC) $(CFLAGS) -Wno-unused-parameter -std=c11 \
		-o build$(PREFIX)/horpkgbin/$(REAL_BINARY_NAME) \
		src/*.c \
		-I../sysroot/include \
		-L../sysroot/lib -lcurl -lyyjson -lzip

	mkdir -p ../sysroot
	cp -rfv build/$(PREFIX)/. ../sysroot | tee file.lst
	
	@if [ ! -f "$(SHIM_CONFIG_PATH)" ]; then \
		echo "ERROR: Shim config file not found at $(SHIM_CONFIG_PATH)" >&2; \
		echo "Please create it and ensure its TARGET_COMMAND_PATH points to:" >&2; \
		echo "    \"$(PREFIX)/horpkgbin/$(REAL_BINARY_NAME)\"" >&2; \
		exit 1; \
	fi
	
	$(MAKE) -C ../shim install SHIM_NAME=$(SHIM_NAME) CONFIG_PATH=$(SHIM_CONFIG_PATH) 2>&1 | tee -a file.lst


clean:
	rm -rf build
	rm -f file.lst
	$(MAKE) -C ../shim clean