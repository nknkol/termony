include ../utils/Makefrag

# --- (新) Shim 和安装路径定义 ---
# 真实的二进制文件名
REAL_BINARY_NAME = hdc
# Shim (包装器) 的名称 (与真实名称相同)
SHIM_NAME = hdc
# 真实的二进制文件将安装到 'horpkgbin' (遵循 horpkg 模式)
REAL_BIN_INSTALL_DIR = ../sysroot/horpkgbin
# Shim (包装器) 的配置文件路径
SHIM_CONFIG_PATH = $(CURDIR)/shim/config.h
# ---

all: download/hdc.zip download/securec.zip CMakeLists.txt
	rm -rf temp build
	mkdir -p temp build
	cd temp && unzip ../download/hdc.zip && mv developtools_hdc-OpenHarmony-v5.0.1-Release hdc
	cd temp && unzip ../download/securec.zip && mv third_party_bounds_checking_function-OpenHarmony-5.0.1-Release securec
	cp CMakeLists.txt temp
	#cd temp/hdc && git init .
	#&& git add --all && git commit -m "init" # help to update 0001-ignore-usb-fail.diff with git diff
	cd temp/hdc && cat ../../0001-ignore-usb-fail.diff | patch -Np1
	
	@echo "==> [HDC] Running CMake..."
	cd build && cmake ../temp -DCMAKE_TOOLCHAIN_FILE=$(OHOS_SDK_HOME)/native/build/cmake/ohos.toolchain.cmake -DOHOS_ARCH=$(OHOS_ABI) -DCMAKE_CXX_FLAGS="-Wno-c++11-narrowing"
	
	@echo "==> [HDC] Running Make (Compiling real binary)..."
	cd build && make -j $(shell nproc)
	
	# --- (已修改) 安装步骤 ---
	
	# 步骤 1: 安装 *真实的* hdc-lite 到 ../sysroot/horpkgbin/
	@echo "==> [HDC] Installing real binary to $(REAL_BIN_INSTALL_DIR)/"
	mkdir -p $(REAL_BIN_INSTALL_DIR)
	cp build/$(REAL_BINARY_NAME) $(REAL_BIN_INSTALL_DIR)/
	
	# 步骤 2: 检查 shim 配置文件
	@if [ ! -f "$(SHIM_CONFIG_PATH)" ]; then \
		echo "ERROR: Shim config file not found at $(SHIM_CONFIG_PATH)" >&2; \
		echo "Please create it first." >&2; \
		exit 1; \
	fi
	
	# 步骤 3: 编译并安装 shim (包装器) 到 ../sysroot/bin/
	@echo "==> [HDC] Building and installing shim wrapper -> ../sysroot/bin/$(SHIM_NAME)"
	$(MAKE) -C ../shim install SHIM_NAME=$(SHIM_NAME) CONFIG_PATH=$(SHIM_CONFIG_PATH)

	@echo "==> [HDC] Build complete."
	@echo "    Real Binary: $(REAL_BIN_INSTALL_DIR)/$(REAL_BINARY_NAME)"
	@echo "    Wrapper:     ../sysroot/bin/$(SHIM_NAME)"

# (下载规则保持不变)
download/hdc.zip:
	mkdir -p download
	cd download && $(WGET) -O hdc.zip https://gitee.com/openharmony/developtools_hdc/repository/archive/OpenHarmony-v5.0.1-Release.zip

download/securec.zip:
	mkdir -p download
	cd download && $(WGET) -O securec.zip https://gitee.com/openharmony/third_party_bounds_checking_function/repository/archive/OpenHarmony-5.0.1-Release.zip

# (新) 添加 clean 目标
.PHONY: all clean

clean:
	rm -rf temp build
	rm -f download/hdc.zip download/securec.zip
	# 同时清理 shim 目录的构建产物
	$(MAKE) -C ../shim clean