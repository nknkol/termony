include ../utils/Makefrag

SHIM_CONFIG_PATH = $(shell pwd)/shim/config.h

all: download/openssl-3.5.0.tar.gz
	rm -rf temp build
	mkdir -p temp build/bin
	cd temp && tar xvf ../download/openssl-3.5.0.tar.gz
	cd temp/openssl-3.5.0 && cat ../../0001-ohos-getauxval.diff | patch -Np1
	cd temp/openssl-3.5.0 && ./Configure linux-$(OHOS_ARCH) shared enable-legacy no-tests --prefix=$(PREFIX) --libdir=lib --openssldir=$(PREFIX)/etc/ssl -D__OHOS__ CC=$(OHOS_SDK_HOME)/native/llvm/bin/$(OHOS_ARCH)-unknown-linux-ohos-clang AR=$(OHOS_SDK_HOME)/native/llvm/bin/llvm-ar RANLIB=$(OHOS_SDK_HOME)/native/llvm/bin/llvm-ranlib STRIP=$(OHOS_SDK_HOME)/native/llvm/bin/llvm-strip
	cd temp/openssl-3.5.0 && make -j $(shell nproc)
	cd temp/openssl-3.5.0 && make install_sw -j $(shell nproc) DESTDIR=$(shell pwd)/build
	mkdir -p ../sysroot
	$(OHOS_SDK_HOME)/native/llvm/bin/llvm-strip build/$(PREFIX)/lib/*.so
	rm -f ./build/$(PREFIX)/lib/*.a
	mkdir -p ./build/$(PREFIX)/etc/ssl
	cp -fv openssl.cnf ./build/$(PREFIX)/etc/ssl/
	cp -rv ./build/$(PREFIX)/. ../sysroot | tee file.lst
	@cp -fv ./build/$(PREFIX)/bin/openssl ../sysroot/horpkgbin/ | tee file.lst
	#///////////////
	@if [ ! -f "$(SHIM_CONFIG_PATH)" ]; then \
		echo "ERROR: Shim config file not found at $(SHIM_CONFIG_PATH)" >&2; \
		echo "Please create 'shim/config.h' inside the '$(shell pwd)' directory." >&2; \
		exit 1; \
	fi
	@echo "==> [hapsigntool] Building and installing shim wrapper -> ../sysroot/bin/openssl"
	$(MAKE) -C ../shim install SHIM_NAME=openssl CONFIG_PATH=$(SHIM_CONFIG_PATH)
	#///////////////

download/openssl-3.5.0.tar.gz:
	mkdir -p download
	cd download && $(WGET) https://github.com/openssl/openssl/releases/download/openssl-3.5.0/openssl-3.5.0.tar.gz
