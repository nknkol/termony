include ../utils/Makefrag

RIPGREP_APK := ripgrep-15.1.0-r0.apk
RIPGREP_URL := https://dl-cdn.alpinelinux.org/alpine/edge/community/aarch64/$(RIPGREP_APK)
RIPGREP_DOWNLOAD := download/$(RIPGREP_APK)

PCRE2_APK := pcre2-10.47-r0.apk
PCRE2_URL := https://dl-cdn.alpinelinux.org/alpine/edge/main/aarch64/$(PCRE2_APK)
PCRE2_DOWNLOAD := download/$(PCRE2_APK)

INSTALL_PREFIX := $(PREFIX)/opt/ripgrep
SHIM_CONFIG_PATH := $(shell pwd)/shim/config.h

.PHONY: all clean

all: $(RIPGREP_DOWNLOAD) $(PCRE2_DOWNLOAD)
	rm -rf build temp
	mkdir -p build temp/ripgrep temp/pcre2

	tar -xzf $(RIPGREP_DOWNLOAD) -C temp/ripgrep
	tar -xzf $(PCRE2_DOWNLOAD) -C temp/pcre2

	mkdir -p build$(INSTALL_PREFIX)/bin
	cp -fv temp/ripgrep/usr/bin/rg build$(INSTALL_PREFIX)/bin/rg
	chmod +x build$(INSTALL_PREFIX)/bin/rg

	mkdir -p build$(PREFIX)/lib
	cp -afv temp/pcre2/usr/lib/libpcre2-8.so* build$(PREFIX)/lib/

	mkdir -p ../sysroot
	cp -rfv build/$(PREFIX)/. ../sysroot/ | tee file.lst

	$(MAKE) -C ../shim install SHIM_NAME=rg CONFIG_PATH=$(SHIM_CONFIG_PATH)
	ln -sf rg ../sysroot/bin/ripgrep

$(RIPGREP_DOWNLOAD):
	mkdir -p download
	$(WGET) -O $@ $(RIPGREP_URL)

$(PCRE2_DOWNLOAD):
	mkdir -p download
	$(WGET) -O $@ $(PCRE2_URL)

clean:
	rm -rf build temp download file.lst .stamp
