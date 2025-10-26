# 包含鸿蒙交叉编译环境
include ../utils/Makefrag

# --- 配置 ---

# 1. 真实的 horpkg 二进制文件名
#    (我们将它编译为 'horpkg'，但放在 'horpkgbin' 目录下)
REAL_BINARY_NAME = horpkg

# 2. shim (包装器) 的最终名称 (也叫 'horpkg'，但放在 'bin' 目录下)
SHIM_NAME = horpkg

# 3. shim 配置文件的路径 (使用你提供的 horpkg/shim/config.h)
SHIM_CONFIG_PATH = $(shell pwd)/shim/config.h

# --- 目标 ---
.PHONY: all clean

all:
	rm -rf build
	
	# 步骤 1: 为 *真实的* 二进制文件创建目录
	# 路径: .../horpkgbin/
	mkdir -p build$(PREFIX)/horpkgbin
	mkdir -p build$(PREFIX)/etc
	
	# 步骤 2: 复制 cacert.pem
	cp -fv src/cacert.pem build$(PREFIX)/etc/
	
	# 步骤 3: 编译 *真实的* horpkg 二进制文件
	# 输出到: build$(PREFIX)/horpkgbin/horpkg
	@echo "==> [HORPKG] Compiling real binary -> build$(PREFIX)/horpkgbin/$(REAL_BINARY_NAME)"
	$(CC) $(CFLAGS) -Wno-unused-parameter -std=c11 \
		-o build$(PREFIX)/horpkgbin/$(REAL_BINARY_NAME) \
		src/*.c \
		-I../sysroot/include \
		-L../sysroot/lib -lcurl -lyyjson
	
	# 步骤 4: 安装 *真实的* 二进制文件和配置到 sysroot
	# (cp 会将 build$(PREFIX)/horpkgbin/horpkg 复制到 ../sysroot/horpkgbin/horpkg)
	@echo "==> [HORPKG] Installing real binary to ../sysroot/horpkgbin/"
	mkdir -p ../sysroot
	cp -rfv build$(PREFIX)/. ../sysroot | tee file.lst
	
	# 步骤 5: 检查 shim 配置文件是否存在
	@if [ ! -f "$(SHIM_CONFIG_PATH)" ]; then \
		echo "ERROR: Shim config file not found at $(SHIM_CONFIG_PATH)" >&2; \
		echo "Please create it and ensure its TARGET_COMMAND_PATH points to:" >&2; \
		echo "    \"$(PREFIX)/horpkgbin/$(REAL_BINARY_NAME)\"" >&2; \
		exit 1; \
	fi
	
	# 步骤 6: 编译并安装 shim (包装器)
	# - 我们调用 ../shim/Makefile 的 'install' 目标
	# - 'install' 目标会自动将编译好的 $(SHIM_NAME) 复制到 ../sysroot/bin/
	@echo "==> [HORPKG] Building and installing shim wrapper -> ../sysroot/bin/$(SHIM_NAME)"
	$(MAKE) -C ../shim install SHIM_NAME=$(SHIM_NAME) CONFIG_PATH=$(SHIM_CONFIG_PATH) 2>&1 | tee -a file.lst
	
	@echo "==> [HORPKG] Build complete."
	@echo "    Real Binary: ../sysroot/horpkgbin/$(REAL_BINARY_NAME)"
	@echo "    Wrapper:     ../sysroot/bin/$(SHIM_NAME)"


clean:
	rm -rf build
	rm -f file.lst
	# 同时清理 shim 目录的构建产物
	$(MAKE) -C ../shim clean