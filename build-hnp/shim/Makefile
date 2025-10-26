#
# Makefile for compiling C Shims (带 build 和 install 目标)
# 路径: build-hnp/shim/Makefile
#
# -------------------------------------------------------------------
# 变量 (必须由调用者传入):
#   SHIM_NAME   : 最终 shim 可执行文件的名字 (例如: "horpkg")
#   CONFIG_PATH : 用于此次编译的 'config.h' 文件的路径
#
# 目标:
#   all:     (默认) 编译 shim 到 ./build/$(SHIM_NAME)
#   install: 编译 shim 并将其安装到 ../sysroot/bin/$(SHIM_NAME)
#   clean:   清理本地的 build 目录和临时文件
# -------------------------------------------------------------------

# 1. 包含主构建环境配置
# [cite_start]这会从 ../utils/Makefrag 导入所有交叉编译变量 [cite: 1, 2, 3, 4]
# (例如: $(CC)[cite_start], $(CFLAGS), $(LDFLAGS), $(STRIP) [cite: 1, 2, 3])
include ../utils/Makefrag

# --- 变量 ---
SRC = shim.c

# 'shim.c' 期望 #include 的临时配置文件名
BUILD_CONFIG = config.h

# 本地构建目录
BUILD_DIR = build

# 最终安装目标目录
INSTALL_DIR = ../sysroot/bin

# 最终编译产物的完整路径
SHIM_BINARY = $(BUILD_DIR)/$(SHIM_NAME)

.PHONY: all clean install

# 默认目标 'all': 仅编译，不安装
# 编译规则，产物是 $(SHIM_BINARY)
all: $(SHIM_BINARY)

# 编译的依赖和命令
$(SHIM_BINARY): $(SRC)
	# 1. 检查必需的变量
	@if [ -z "$(SHIM_NAME)" ]; then echo "ERROR: SHIM_NAME is not set." >&2; exit 1; fi
	@if [ -z "$(CONFIG_PATH)" ]; then echo "ERROR: CONFIG_PATH is not set." >&2; exit 1; fi
	@if [ ! -f "$(CONFIG_PATH)" ]; then echo "ERROR: Config file not found: $(CONFIG_PATH)" >&2; exit 1; fi

	# 2. 创建本地 build 目录
	mkdir -p $(BUILD_DIR)

	# 3. 准备配置文件
	@echo "==> [SHIM] Using config: $(CONFIG_PATH)"
	cp -f "$(CONFIG_PATH)" "$(BUILD_CONFIG)"

	# [cite_start]4. 编译和链接 Shim (使用 Makefrag 中的 $(CC) 和 $(CFLAGS)) [cite: 1]
	@echo "==> [SHIM] Compiling $(SRC) -> $(SHIM_BINARY)"
	$(CC) $(CFLAGS) -o "$(SHIM_BINARY)" $(SRC) $(LDFLAGS)

	# [cite_start]5. Strip (精简) 二进制文件 (使用 Makefrag 中的 $(STRIP)) [cite: 1, 3]
	@echo "==> [SHIM] Stripping $(SHIM_BINARY)"
	$(STRIP) "$(SHIM_BINARY)"
	@echo "==> [SHIM] Build complete: $(SHIM_BINARY)"


# 安装目标: install
# 这会将 'all' 编译的产物复制到 sysroot
install: all
	@echo "==> [SHIM] Installing $(SHIM_BINARY) to $(INSTALL_DIR)"
	# 1. 确保安装目录存在
	mkdir -p $(INSTALL_DIR)
	
	# 2. 复制文件
	cp -f "$(SHIM_BINARY)" "$(INSTALL_DIR)/"
	@echo "==> [SHIM] Install complete."


# 清理
clean:
	@echo "==> [SHIM] Cleaning local build directory..."
	rm -rf $(BUILD_DIR)
	rm -f $(BUILD_CONFIG)