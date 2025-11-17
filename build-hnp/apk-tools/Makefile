include ../utils/Makefrag

ifeq ($(strip $(OHOS_ARCH)),)
$(error OHOS_ARCH is empty; export OHOS_ARCH=aarch64 or x86_64 before building apk-tools)
endif
ifeq ($(strip $(OHOS_SDK_HOME)),)
$(error OHOS_SDK_HOME is empty; point it to your OpenHarmony SDK root)
endif

REPO_URL := https://github.com/nknkol/apk-tools.git
REPO_COMMIT := 52c3a4f5fa2e899156ed25e2e39a87a771090772
SRC_DIR := apk-tools
BUILD_ROOT := $(shell pwd)
TEMP_DIR := temp/$(SRC_DIR)
DESTDIR := $(BUILD_ROOT)/build
SHIM_NAME := apk
SHIM_CONFIG := $(BUILD_ROOT)/shim/config.h
TOOLCHAIN_VARS = \
	CC="$(CC)" \
	AR="$(AR)" \
	LD="$(LD)" \
	STRIP="$(STRIP)" \
	RANLIB="$(RANLIB)"

BUILD_VARS = $(TOOLCHAIN_VARS) \
	DESTDIR=$(DESTDIR) \
	SBINDIR=$(PREFIX)/bin \
	LIBDIR=$(PREFIX)/lib \
	CONFDIR=$(PREFIX)/etc/apk \
	MANDIR=$(PREFIX)/share/man \
	DOCDIR=$(PREFIX)/share/doc/apk \
	INCLUDEDIR=$(PREFIX)/include \
	PKGCONFIGDIR=$(PREFIX)/lib/pkgconfig \
	LUA=no \
	ZSTD=no

all: download/$(SRC_DIR)
	rm -rf temp build
	mkdir -p temp build
	cd temp && cp -r ../download/$(SRC_DIR) .
	cd $(TEMP_DIR) && git checkout -q $(REPO_COMMIT)
	cd $(TEMP_DIR) && python -c 'from pathlib import Path; io=Path("src/io.c"); txt=io.read_text(); txt=txt.replace("pwd = fgetpwent(in);", "pwd = getpwent();"); txt=txt.replace("grp = fgetgrent(in);", "grp = getgrent();"); io.write_text(txt)'
	mkdir -p $(TEMP_DIR)/src/portability
	cp $(TEMP_DIR)/portability/qsort_r.c $(TEMP_DIR)/src/portability/
	cp $(TEMP_DIR)/portability/reallocarray.c $(TEMP_DIR)/src/portability/
	# only build libfetch + src to avoid doc/lua/python dependencies
	cd $(TEMP_DIR) && python -c 'import re; from pathlib import Path; mf=Path("Makefile"); t=mf.read_text(); t=re.sub(r"^subdirs.*$$", "subdirs := libfetch/ src/", t, flags=re.M); mf.write_text(t)'
	# fix renamed shim source
	cd $(TEMP_DIR)/src && /usr/bin/sed -i.bak 's|shim.o|apk_shim.o portability/qsort_r.o portability/reallocarray.o|g' Makefile
	cd $(TEMP_DIR) && $(MAKE) $(BUILD_VARS)
	cd $(TEMP_DIR) && $(MAKE) $(BUILD_VARS) install
	# relocate real apk into horpkgbin to avoid shim override
	mkdir -p build$(PREFIX)/horpkgbin
	if [ -f build$(PREFIX)/bin/apk ]; then mv build$(PREFIX)/bin/apk build$(PREFIX)/horpkgbin/apk; fi
	# strip elf files to shrink output
	find build -type f -exec file {} \; | grep ELF | awk '{print substr($$1, 0, length($$1)-1);}' | xargs -t $(STRIP) || true
	mkdir -p ../sysroot
	cp -rfv build$(PREFIX)/. ../sysroot | tee file.lst
	# generate shim config and install shim wrapper
	mkdir -p $(dir $(SHIM_CONFIG))
	{ \
		printf '#ifndef SHIM_CONFIG_H\n'; \
		printf '#define SHIM_CONFIG_H\n'; \
		printf '#define TARGET_COMMAND_PATH "%s/horpkgbin/apk"\n' "$(PREFIX)"; \
		printf '#define LIB_PATH_ENV_VAR "LD_LIBRARY_PATH"\n'; \
		printf '#define CUSTOM_LIB_PATH "%s/lib"\n' "$(PREFIX)"; \
		printf '#define SHIM_ENV_VARS \\\n'; \
		printf '    X("SYSROOT", "%s/") \\\n' "$(PREFIX)"; \
		printf '    X("TMUX_TMPDIR", "/data/storage/el2/horpkg-base/cache")\n'; \
		printf '#endif\n'; \
	} > $(SHIM_CONFIG)
	$(MAKE) -C ../shim install SHIM_NAME=$(SHIM_NAME) CONFIG_PATH=$(SHIM_CONFIG) 2>&1 | tee -a file.lst

download/$(SRC_DIR):
	mkdir -p download
	cd download && rm -rf $(SRC_DIR) && git clone $(REPO_URL) $(SRC_DIR)

clean:
	rm -rf temp build
	rm -f file.lst
