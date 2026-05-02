include ../utils/Makefrag

OPENCODE_VERSION := 1.1.49
OPENCODE_ARCHIVE := opencode-linux-arm64-musl.tar.gz
OPENCODE_URL := https://github.com/anomalyco/opencode/releases/download/v$(OPENCODE_VERSION)/$(OPENCODE_ARCHIVE)
OPENCODE_DOWNLOAD := download/$(OPENCODE_ARCHIVE)

INSTALL_PREFIX := $(PREFIX)/opt/opencode

.PHONY: all clean

all: $(OPENCODE_DOWNLOAD)
	rm -rf build shim_config
	mkdir -p build

	tar -xf $(OPENCODE_DOWNLOAD) -C build

	mkdir -p build$(INSTALL_PREFIX)/bin
	cp build/opencode build$(INSTALL_PREFIX)/bin/

	mkdir -p ../sysroot
	cp -rfv build/$(PREFIX)/. ../sysroot/ | tee file.lst

	mkdir -p shim_config/opencode
	echo '#ifndef CONFIG_H' > shim_config/opencode/config.h
	echo '#define CONFIG_H' >> shim_config/opencode/config.h
	echo '#define TARGET_COMMAND_PATH "$(INSTALL_PREFIX)/bin/opencode"' >> shim_config/opencode/config.h
	echo '#define SHIM_ENV_VARS \' >> shim_config/opencode/config.h
	echo '    X("LD_LIBRARY_PATH", "$(PREFIX)/lib") \' >> shim_config/opencode/config.h
	echo '    X("OPENCODE_CACHE", "/data/storage/el2/base/cache/opencode")' >> shim_config/opencode/config.h
	echo '#endif' >> shim_config/opencode/config.h

	$(MAKE) -C ../shim install SHIM_NAME=opencode CONFIG_PATH=$(shell pwd)/shim_config/opencode/config.h

$(OPENCODE_DOWNLOAD):
	mkdir -p download
	$(WGET) -O $@ $(OPENCODE_URL)

clean:
	rm -rf build temp download shim_config file.lst
