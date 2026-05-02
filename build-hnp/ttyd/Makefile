include ../utils/Makefrag

TTYD_VERSION := 1.7.7
TTYD_ARCHIVE := ttyd.aarch64
TTYD_URL := https://github.com/tsl0922/ttyd/releases/download/$(TTYD_VERSION)/$(TTYD_ARCHIVE)
TTYD_DOWNLOAD := download/$(TTYD_ARCHIVE)

INSTALL_PREFIX := $(PREFIX)/opt/ttyd

.PHONY: all clean

all: $(TTYD_DOWNLOAD)
	rm -rf build shim_config
	mkdir -p build

	mkdir -p build$(INSTALL_PREFIX)/bin
	cp $(TTYD_DOWNLOAD) build$(INSTALL_PREFIX)/bin/ttyd
	chmod +x build$(INSTALL_PREFIX)/bin/ttyd

	mkdir -p ../sysroot
	cp -rfv build/$(PREFIX)/. ../sysroot/ | tee file.lst

	mkdir -p shim_config/ttyd
	echo '#ifndef CONFIG_H' > shim_config/ttyd/config.h
	echo '#define CONFIG_H' >> shim_config/ttyd/config.h
	echo '#define TARGET_COMMAND_PATH "$(INSTALL_PREFIX)/bin/ttyd"' >> shim_config/ttyd/config.h
	echo '#define SHIM_ENV_VARS \\' >> shim_config/ttyd/config.h
	echo '    X("LD_LIBRARY_PATH", "$(PREFIX)/lib")' >> shim_config/ttyd/config.h
	echo '#endif' >> shim_config/ttyd/config.h

	$(MAKE) -C ../shim install SHIM_NAME=ttyd CONFIG_PATH=$(shell pwd)/shim_config/ttyd/config.h

$(TTYD_DOWNLOAD):
	mkdir -p download
	$(WGET) -O $@ $(TTYD_URL)

clean:
	rm -rf build temp download shim_config file.lst
