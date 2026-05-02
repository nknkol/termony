include ../utils/Makefrag

MC_VERSION := 4.8.33-r3
MC_ARCHIVE := mc-$(MC_VERSION).apk
MC_URL := https://dl-cdn.alpinelinux.org/alpine/edge/main/aarch64/$(MC_ARCHIVE)
MC_DOWNLOAD := download/$(MC_ARCHIVE)

INSTALL_PREFIX := $(PREFIX)/opt/mc

.PHONY: all clean

all: $(MC_DOWNLOAD)
	rm -rf build shim_config
	mkdir -p build

	mkdir -p build/temp
	tar -xf $(MC_DOWNLOAD) -C build/temp

	mkdir -p build$(INSTALL_PREFIX)/bin
	cp build/temp/usr/bin/mc build$(INSTALL_PREFIX)/bin/

	mkdir -p build$(INSTALL_PREFIX)/share
	cp -r build/temp/usr/share/mc build$(INSTALL_PREFIX)/share/

	if [ -d build/temp/etc ]; then \
		mkdir -p build$(INSTALL_PREFIX)/etc; \
		cp -r build/temp/etc/* build$(INSTALL_PREFIX)/etc/; \
	fi

	mkdir -p ../sysroot
	cp -rfv build/$(PREFIX)/. ../sysroot/ | tee file.lst

	mkdir -p shim_config/mc
	echo '#ifndef CONFIG_H' > shim_config/mc/config.h
	echo '#define CONFIG_H' >> shim_config/mc/config.h
	echo '#define TARGET_COMMAND_PATH "$(INSTALL_PREFIX)/bin/mc"' >> shim_config/mc/config.h
	echo '#define SHIM_ENV_VARS \' >> shim_config/mc/config.h
	echo '    X("LD_LIBRARY_PATH", "$(PREFIX)/lib:$(PREFIX)/usr/lib") \' >> shim_config/mc/config.h
	echo '    X("MC_DATADIR", "$(INSTALL_PREFIX)/share/mc") \' >> shim_config/mc/config.h
	echo '    X("MC_HOME", "/data/storage/el2/base/files/.mc") \' >> shim_config/mc/config.h
	echo '    X("TERM", "xterm-256color")' >> shim_config/mc/config.h
	echo '#endif' >> shim_config/mc/config.h

	$(MAKE) -C ../shim install SHIM_NAME=mc CONFIG_PATH=$(shell pwd)/shim_config/mc/config.h

$(MC_DOWNLOAD):
	mkdir -p download
	$(WGET) -O $@ $(MC_URL)

clean:
	rm -rf build temp download shim_config file.lst
