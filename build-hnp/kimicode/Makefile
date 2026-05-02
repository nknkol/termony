include ../utils/Makefrag

KIMICODE_VERSION := 1.12.0
KIMICODE_ARCHIVE := kimi-$(KIMICODE_VERSION)-aarch64-unknown-linux-gnu.tar.gz
KIMICODE_URL := https://github.com/MoonshotAI/kimi-cli/releases/download/$(KIMICODE_VERSION)/$(KIMICODE_ARCHIVE)
KIMICODE_DOWNLOAD := download/$(KIMICODE_ARCHIVE)

INSTALL_PREFIX := $(PREFIX)/opt/kimicode

.PHONY: all clean

all: $(KIMICODE_DOWNLOAD)
	rm -rf build shim_config
	mkdir -p build

	mkdir -p build/temp
	tar -xf $(KIMICODE_DOWNLOAD) -C build/temp

	mkdir -p build$(INSTALL_PREFIX)/bin
	# Find the 'kimi' or 'kimicode' binary and copy it as 'kimicode'
	find build/temp -type f \( -name "kimi" -o -name "kimicode" \) -exec cp {} build$(INSTALL_PREFIX)/bin/kimicode \;
	chmod +x build$(INSTALL_PREFIX)/bin/kimicode

	mkdir -p build$(PREFIX)/lib
	
	# 1. Create a symlink for ld-linux-aarch64.so.1 to point to musl
	# This satisfies the "needed" dependency if we can't fully remove it, or if it's re-added.
	ln -sf /lib/ld-musl-aarch64.so.1 build$(PREFIX)/lib/ld-linux-aarch64.so.1

	# 2. Create a shim library to provide missing Glibc symbols (like __realpath_chk)
	echo '#include <stdlib.h>' > build/shim_libc.c
	echo '#include <limits.h>' >> build/shim_libc.c
	echo 'char *__realpath_chk(const char *path, char *resolved_path, size_t resolved_len) { return realpath(path, resolved_path); }' >> build/shim_libc.c
	# Compile the shim library using the project's CC (which points to clang)
	$(CC) $(CFLAGS) -shared -fPIC -o build$(PREFIX)/lib/libshim_libc.so build/shim_libc.c

	# Patch the binary:
	# - Set interpreter to system Musl
	# - Add dependency on our shim library
	# - Set RPATH so it finds ld-linux symlink and libshim_libc.so in our package lib dir
	patchelf --set-interpreter /lib/ld-musl-aarch64.so.1 build$(INSTALL_PREFIX)/bin/kimicode
	patchelf --add-needed libshim_libc.so build$(INSTALL_PREFIX)/bin/kimicode
	patchelf --set-rpath $(PREFIX)/lib build$(INSTALL_PREFIX)/bin/kimicode
	
	# Try to remove ld-linux dependency again, but if it fails, the symlink in RPATH should save us.
	patchelf --remove-needed ld-linux-aarch64.so.1 build$(INSTALL_PREFIX)/bin/kimicode || true

	mkdir -p ../sysroot
	cp -rfv build/$(PREFIX)/. ../sysroot/ | tee file.lst

	mkdir -p shim_config/kimicode
	echo '#ifndef CONFIG_H' > shim_config/kimicode/config.h
	echo '#define CONFIG_H' >> shim_config/kimicode/config.h
	echo '#define TARGET_COMMAND_PATH "$(INSTALL_PREFIX)/bin/kimicode"' >> shim_config/kimicode/config.h
	echo '#define SHIM_ENV_VARS \' >> shim_config/kimicode/config.h
	echo '    X("LD_LIBRARY_PATH", "$(PREFIX)/lib") \' >> shim_config/kimicode/config.h
	echo '    X("KIMICODE_CACHE", "/data/storage/el2/base/cache/kimicode")' >> shim_config/kimicode/config.h
	echo '#endif' >> shim_config/kimicode/config.h

	$(MAKE) -C ../shim install SHIM_NAME=kimicode CONFIG_PATH=$(shell pwd)/shim_config/kimicode/config.h

$(KIMICODE_DOWNLOAD):
	mkdir -p download
	$(WGET) -O $@ $(KIMICODE_URL)

clean:
	rm -rf build temp download shim_config file.lst
