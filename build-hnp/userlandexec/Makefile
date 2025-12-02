include ../utils/Makefrag

# Debug container config
DOCKER ?= docker
IMAGE_NAME ?= userlandexec-arm64-debug
DEFAULT_CONFIG_PATH=/data/service/hnp/horpkg-base.org/horpkg-base_1.0/etc/loader.conf
SAMPLE_CONFIG_PATH=../sysroot/etc/loader.conf

TEST_DIR := elf/test

all: ##download/elf
	rm -rf temp build
	mkdir -p temp build/bin
	cd temp && cp -vr ../elf elf
ifeq ($(OHOS_ARCH),aarch64)
	cd temp/elf && make -C src ARCH=aarch64 CFLAGS="-ffreestanding -DELFCLASS=ELFCLASS64" DEFAULT_CONFIG_PATH="$(DEFAULT_CONFIG_PATH)"
else ifeq ($(OHOS_ARCH),x86_64)
	cd temp/elf && make -C src ARCH=amd64 CFLAGS="-ffreestanding -DELFCLASS=ELFCLASS64" DEFAULT_CONFIG_PATH="$(DEFAULT_CONFIG_PATH)"
else
	echo "Unsupported OHOS_ARCH=$(OHOS_ARCH)"
	exit 1
endif
	$(MAKE) -C $(TEST_DIR)
	cp temp/elf/src/elfloader ./build/bin/
	cp temp/elf/src/payload/payload.elf ./build/bin/
	mkdir -p ../sysroot
	cp -rv ./build/. ../sysroot | tee file.lst
ifneq ($(SAMPLE_CONFIG_PATH),)
	mkdir -p $(dir $(SAMPLE_CONFIG_PATH))
	cp elf/loader.conf.sample $(SAMPLE_CONFIG_PATH)
endif

# download/elf:
# 	mkdir -p download
# 	cd download && rm -rf elf && git clone https://github.com/nknkol/elf.git

.PHONY: container-image shell

# Build an arm64 Alpine image with gdb/strace and common tools
container-image:
	$(DOCKER) build --platform=linux/arm64/v8 -t $(IMAGE_NAME) -f Dockerfile .

# Drop into the debug container with current repo mounted at /workspace
shell: container-image
	$(DOCKER) run --rm -it --platform=linux/arm64/v8 \
		--cap-add=SYS_PTRACE --security-opt seccomp=unconfined \
		-v $(PWD):/workspace -w /workspace $(IMAGE_NAME) /bin/bash
