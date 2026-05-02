include ../utils/Makefrag

CRUSH_VERSION := v0.50.0
CRUSH_REPO := https://github.com/charmbracelet/crush.git
CRUSH_BUILD := temp/crush

INSTALL_PREFIX := $(PREFIX)/opt/crush

.PHONY: all clean

all:
	rm -rf build temp shim_config
	mkdir -p build

	mkdir -p temp
	cd temp && git clone --depth 1 --branch $(CRUSH_VERSION) $(CRUSH_REPO)

	cd $(CRUSH_BUILD) && sed -i 's/go 1.26.1/go 1.23/' go.mod
	cd $(CRUSH_BUILD) && go mod tidy
	cd $(CRUSH_BUILD) && CGO_ENABLED=1 GOOS=linux GOARCH=arm64 CC=$(OHOS_SDK_HOME)/native/llvm/bin/aarch64-unknown-linux-ohos-clang CGO_CFLAGS="--sysroot=$(OHOS_SDK_HOME)/native/sysroot" CGO_LDFLAGS="--sysroot=$(OHOS_SDK_HOME)/native/sysroot -L$(OHOS_SDK_HOME)/native/sysroot/usr/lib/aarch64-linux-ohos -static" go build -o ../../build/crush -ldflags="-s -w -linkmode external"

	mkdir -p build$(INSTALL_PREFIX)/bin
	cp build/crush build$(INSTALL_PREFIX)/bin/crush
	chmod +x build$(INSTALL_PREFIX)/bin/crush

	mkdir -p ../sysroot
	cp -rfv build/$(PREFIX)/. ../sysroot/ | tee file.lst

	mkdir -p shim_config/crush
	echo '#ifndef CONFIG_H' > shim_config/crush/config.h
	echo '#define CONFIG_H' >> shim_config/crush/config.h
	echo '#define TARGET_COMMAND_PATH "$(INSTALL_PREFIX)/bin/crush"' >> shim_config/crush/config.h
	echo '#define SHIM_ENV_VARS \' >> shim_config/crush/config.h
	echo '    X("LD_LIBRARY_PATH", "$(PREFIX)/lib") \' >> shim_config/crush/config.h
	echo '    X("HOME", "/data/storage/el2/base/files/.crush")' >> shim_config/crush/config.h
	echo '#endif' >> shim_config/crush/config.h

	$(MAKE) -C ../shim install SHIM_NAME=crush CONFIG_PATH=$(shell pwd)/shim_config/crush/config.h

clean:
	rm -rf build temp download shim_config file.lst
