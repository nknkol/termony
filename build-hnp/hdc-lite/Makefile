# 1. 包含通用编译环境 (WGET 变量在这里定义)
include ../utils/Makefrag

# --- (已修改) Shim 和安装路径定义 ---
REAL_BINARY_NAME = hdc-lite
SHIM_NAME = hdc-lite
REAL_BIN_INSTALL_DIR = ../sysroot/horpkgbin
SHIM_CONFIG_PATH = $(shell pwd)/shim/config.h
# ---

# --- (已修改) 源码和补丁定义 ---
HDC_ZIP_FILE = hdc.zip
HDC_ZIP_URL = https://gitee.com/openharmony/developtools_hdc/repository/archive/OpenHarmony-v5.0.1-Release.zip
HDC_EXTRACT_DIR = developtools_hdc-OpenHarmony-v5.0.1-Release

SECUREC_ZIP_FILE = securec.zip
SECUREC_ZIP_URL = https://gitee.com/openharmony/third_party_bounds_checking_function/repository/archive/OpenHarmony-5.0.1-Release.zip
SECUREC_EXTRACT_DIR = third_party_bounds_checking_function-OpenHarmony-5.0.1-Release

# 本地文件
PATCH_FILE = hdc-lite.patch
# 编译脚本 (我们修改过的版本)
MAKEFILE_SIMPLE = Makefile.simple
# ---

# --- (已修改) 主要目标 ---
# (依赖于 'download/hdc.zip', 'download/securec.zip', 和本地的 Makefile.simple)
all: download/$(HDC_ZIP_FILE) download/$(SECUREC_ZIP_FILE) $(PATCH_FILE) $(MAKEFILE_SIMPLE)
	@echo "==> [hdc-lite] Cleaning up old directories..."
	rm -rf temp
	mkdir -p temp

	@echo "==> [hdc-lite] Extracting source..."
	cd temp && unzip -q ../download/$(HDC_ZIP_FILE) && mv $(HDC_EXTRACT_DIR) hdc
	
	@echo "==> [hdc-lite] Extracting securec..."
	cd temp && unzip -q ../download/$(SECUREC_ZIP_FILE) && mv $(SECUREC_EXTRACT_DIR) securec

	@echo "==> [hdc-lite] Applying patch: $(PATCH_FILE)..."
	# 补丁在 'temp' 目录执行
	cd temp && patch -p1 < ../$(PATCH_FILE)
	
	@echo "==> [hdc-lite] Copying build script..."
	# 将我们修改版的 Makefile.simple 复制到 'temp/' 目录, 补丁打完后 'src' 目录会位于 'temp/src'
	cp $(MAKEFILE_SIMPLE) temp/

	@echo "==> [hdc-lite] Running Make (Compiling real binary)..."
	# 在 'temp' 目录中, 使用 'Makefile.simple' 进行编译
	# $(MAKE) 会自动将 $(CC), $(CXX), $(CFLAGS), $(LDFLAGS) 传递下去
	$(MAKE) -C temp -f $(MAKEFILE_SIMPLE) all
	
	@echo "==> [hdc-lite] Installing real binary to $(REAL_BIN_INSTALL_DIR)/"
	mkdir -p $(REAL_BIN_INSTALL_DIR)
	# (产物路径来自 Makefile.simple)
	cp build/$(REAL_BINARY_NAME) $(REAL_BIN_INSTALL_DIR)/
	
	#///////////////
	@if [ ! -f "$(SHIM_CONFIG_PATH)" ]; then \
		echo "ERROR: Shim config file not found at $(SHIM_CONFIG_PATH)" >&2; \
		echo "Please create 'shim/config.h' inside the '$(shell pwd)' directory." >&2; \
		exit 1; \
	fi
	@echo "==> [hapsigntool] Building and installing shim wrapper -> ../sysroot/bin/$(SHIM_NAME)"
	$(MAKE) -C ../shim install SHIM_NAME=$(SHIM_NAME) CONFIG_PATH=$(SHIM_CONFIG_PATH)
	#///////////////

# --- .stamp 目标 ---
.stamp: all
	touch .stamp

# --- (已修改) 下载目标 ---
download/$(HDC_ZIP_FILE):
	mkdir -p download
	cd download && $(WGET) -O $(HDC_ZIP_FILE) "$(HDC_ZIP_URL)"

download/$(SECUREC_ZIP_FILE):
	mkdir -p download
	cd download && $(WGET) -O $(SECUREC_ZIP_FILE) "$(SECUREC_ZIP_URL)"

# --- 清理目标 ---
.PHONY: clean all
clean:
	@echo "==> [hdc-lite] Cleaning..."
	rm -rf temp download .stamp