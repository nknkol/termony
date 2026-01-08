include ../utils/Makefrag

# Debug container config
DOCKER ?= docker
IMAGE_NAME ?= userlandexec-arm64-debug
DEFAULT_PAYLOAD_PATH ?= /data/service/hnp/horpkg-base.org/horpkg-base_1.0/bin/payload.bin
DEFAULT_CONFIG_PATH=
SAMPLE_CONFIG_PATH=

TEST_DIR := elf/test
DASH_BUILD_DIR := temp/dash
DASH_CONFIGURE_HOST := $(OHOS_ARCH)-unknown-linux-musl
DASH_BUILD_TRIPLET := $(shell $(HOST_CC) -dumpmachine 2>/dev/null || echo unknown)


all: ##download/elf
	rm -rf temp build
	mkdir -p temp build/bin
	$(MAKE) dash-build
	cd temp && cp -vr ../elf elf
ifeq ($(OHOS_ARCH),aarch64)
	cd temp/elf && make -C src ARCH=aarch64 CFLAGS="-ffreestanding -DELFCLASS=ELFCLASS64" DEFAULT_CONFIG_PATH="$(DEFAULT_CONFIG_PATH)" DEFAULT_PAYLOAD_PATH="$(DEFAULT_PAYLOAD_PATH)"
else ifeq ($(OHOS_ARCH),x86_64)
	cd temp/elf && make -C src ARCH=amd64 CFLAGS="-ffreestanding -DELFCLASS=ELFCLASS64" DEFAULT_CONFIG_PATH="$(DEFAULT_CONFIG_PATH)" DEFAULT_PAYLOAD_PATH="$(DEFAULT_PAYLOAD_PATH)"
else
	echo "Unsupported OHOS_ARCH=$(OHOS_ARCH)"
	exit 1
endif
	$(MAKE) -C $(TEST_DIR)
	cp temp/elf/src/elfloader ./build/bin/
	cp temp/elf/src/payload/payload.bin ./build/bin/
	$(MAKE) sync-sysroot

# download/elf:
# 	mkdir -p download
# 	cd download && rm -rf elf && git clone https://github.com/nknkol/elf.git

.PHONY: all dash dash-build sync-sysroot container-image shell

dash: dash-build sync-sysroot

dash-build:
	mkdir -p temp build/bin
	if [ "$(OHOS_ARCH)" != "aarch64" ] && [ "$(OHOS_ARCH)" != "x86_64" ]; then \
		echo "Unsupported OHOS_ARCH=$(OHOS_ARCH)"; \
		exit 1; \
	fi
	rm -rf $(DASH_BUILD_DIR)
	cd temp && cp -vr ../dash dash
	cd $(DASH_BUILD_DIR) && ./autogen.sh
	cd $(DASH_BUILD_DIR) && CC_FOR_BUILD="$(HOST_CC)" CFLAGS_FOR_BUILD="-O2" LDFLAGS_FOR_BUILD="" LDFLAGS="-static $(LDFLAGS)" ./configure --build=$(DASH_BUILD_TRIPLET) --host=$(DASH_CONFIGURE_HOST) --prefix=/
	$(MAKE) -C $(DASH_BUILD_DIR) -j $(shell nproc)
	$(MAKE) -C $(DASH_BUILD_DIR) install DESTDIR=$(CURDIR)/build
	$(STRIP) $(CURDIR)/build/bin/dash

sync-sysroot:
	mkdir -p ../sysroot
	cp -rv ./build/. ../sysroot | tee file.lst
ifneq ($(SAMPLE_CONFIG_PATH),)
	mkdir -p $(dir $(SAMPLE_CONFIG_PATH))
	cp elf/src/config/loader.rc.sample $(SAMPLE_CONFIG_PATH)
endif

# Build an arm64 Alpine image with gdb/strace and common tools
container-image:
	$(DOCKER) build --platform=linux/arm64/v8 -t $(IMAGE_NAME) -f Dockerfile .

# Drop into the debug container with current repo mounted at /workspace
shell: container-image
	$(DOCKER) run --rm -it --platform=linux/arm64/v8 \
		--cap-add=SYS_PTRACE --security-opt seccomp=unconfined \
		-v $(PWD):/workspace -w /workspace $(IMAGE_NAME) /bin/bash
