# bun/Makefile  —— git 拉源 + 生成 sources + CMake 构建
include ../utils/Makefrag

# ---- 获取源码（Git）----
SUPPORT_DIR ?= /support
REPO_URL   := https://github.com/oven-sh/bun.git
REPO_REF   := bun-v1.3.0
SRC_DIR    := temp/bun
BUILD_DIR  := $(SRC_DIR)/build
STAMP      := .stamp

# ---- OHOS / 编译器 / Flags ----
TOOL_HOME        ?= /workspace/command-line-tools
SDK_SYSROOT      := $(TOOL_HOME)/sdk/default/openharmony/native/sysroot

BUN_FORCE_CFLAGS   := -fuse-ld=lld
BUN_FORCE_CXXFLAGS := -fuse-ld=lld -stdlib=libc++

CMAKE_ARGS = \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_INSTALL_PREFIX=$(PREFIX) \
	-DCMAKE_SYSTEM_NAME=Linux \
	-DCMAKE_SYSTEM_PROCESSOR=$(OHOS_ARCH) \
	-DCMAKE_C_COMPILER=$(CC) \
	-DCMAKE_CXX_COMPILER=$(CXX) \
	-DLLVM_VERSION=15.0.4 \
	-DCMAKE_C_COMPILER_TARGET=$(OHOS_ARCH)-unknown-linux-ohos \
	-DCMAKE_CXX_COMPILER_TARGET=$(OHOS_ARCH)-unknown-linux-ohos \
	-DCMAKE_SYSROOT=$(SDK_SYSROOT) \
	-DCMAKE_C_FLAGS_INIT="$(BUN_FORCE_CFLAGS)" \
	-DCMAKE_CXX_FLAGS_INIT="$(BUN_FORCE_CXXFLAGS)" \
	-DCMAKE_EXE_LINKER_FLAGS_INIT="" \
	-DCMAKE_SHARED_LINKER_FLAGS_INIT="" \
	-DCMAKE_MODULE_LINKER_FLAGS_INIT="" \
	-DBUN_EXECUTABLE="$${BUN_EXECUTABLE}" \
	-DBUN_EXECUTABLE_VERSION="$${BUN_EXECUTABLE_VERSION}" \

.PHONY: all fetch patch gensources configure build install clean distclean

all: $(STAMP)

$(STAMP): build
	@touch $(STAMP)

fetch:
	@rm -rf $(SRC_DIR)
	git clone --depth=1 --branch $(REPO_REF) $(REPO_URL) $(SRC_DIR)
	git -C $(SRC_DIR) submodule update --init --recursive

patch: fetch
	@cd $(SRC_DIR) && patch -p1 < ../../bun-fix-llvm-check.patch

# 用宿主机 bun 展开 cmake/Sources.json => cmake/sources/*.txt
gensources: patch
	$${BUN_EXECUTABLE} $(SUPPORT_DIR)/bun-gen-cmake-sources.ts $(SRC_DIR)

configure: gensources
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake .. $(CMAKE_ARGS)

build: configure
	$$(command -v nproc >/dev/null 2>&1 && echo $(MAKE) -C $(BUILD_DIR) -j$$(nproc) || echo $(MAKE) -C $(BUILD_DIR))

install: build
	@true   # 如需安装到 PREFIX，可在此添加 cmake --install

clean:
	@rm -rf $(BUILD_DIR) $(STAMP)

distclean: clean
	@rm -rf temp