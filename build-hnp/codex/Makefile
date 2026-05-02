include ../utils/Makefrag

CODEX_VERSION := rust-v0.115.0
CODEX_ARCHIVE := codex-aarch64-unknown-linux-musl.tar.gz
CODEX_URL := https://github.com/openai/codex/releases/download/$(CODEX_VERSION)/$(CODEX_ARCHIVE)
CODEX_DOWNLOAD := download/$(CODEX_ARCHIVE)

INSTALL_PREFIX := $(PREFIX)/opt/codex

.PHONY: all clean

all: $(CODEX_DOWNLOAD)
	rm -rf build shim_config
	mkdir -p build

	mkdir -p build/temp
	tar -xf $(CODEX_DOWNLOAD) -C build/temp

	mkdir -p build$(INSTALL_PREFIX)/bin
	# Find the binary (it might be named 'codex' or 'codex-aarch64-unknown-linux-musl') and copy it as 'codex'
	find build/temp -type f -name "codex*" -exec cp {} build$(INSTALL_PREFIX)/bin/codex \;
	chmod +x build$(INSTALL_PREFIX)/bin/codex

	mkdir -p ../sysroot
	cp -rfv build/$(PREFIX)/. ../sysroot/ | tee file.lst

	mkdir -p shim_config/codex
	echo '#ifndef CONFIG_H' > shim_config/codex/config.h
	echo '#define CONFIG_H' >> shim_config/codex/config.h
	echo '#define TARGET_COMMAND_PATH "$(INSTALL_PREFIX)/bin/codex"' >> shim_config/codex/config.h
	echo '#define SHIM_ENV_VARS \' >> shim_config/codex/config.h
	echo '    X("LD_LIBRARY_PATH", "$(PREFIX)/lib") \' >> shim_config/codex/config.h
	echo '    X("CODEX_HOME", "/data/storage/el2/base/files/.codex") \' >> shim_config/codex/config.h
	echo '    X("CODEX_CACHE", "/data/storage/el2/base/cache/codex") \' >> shim_config/codex/config.h
	echo '    X("SSL_CERT_FILE", "/etc/ssl/certs/cacert.pem") \' >> shim_config/codex/config.h
	echo '    X("SSL_CERT_DIR", "/etc/ssl/certs")' >> shim_config/codex/config.h
	echo '#endif' >> shim_config/codex/config.h

	$(MAKE) -C ../shim install SHIM_NAME=codex CONFIG_PATH=$(shell pwd)/shim_config/codex/config.h

$(CODEX_DOWNLOAD):
	mkdir -p download
	$(WGET) -O $@ $(CODEX_URL)

clean:
	rm -rf build temp download shim_config file.lst
