include ../utils/Makefrag

MUTAGEN_VERSION := 0.18.1
MUTAGEN_ARCHIVE := mutagen_linux_arm64_v$(MUTAGEN_VERSION).tar.gz
MUTAGEN_URL := https://github.com/mutagen-io/mutagen/releases/download/v$(MUTAGEN_VERSION)/$(MUTAGEN_ARCHIVE)
MUTAGEN_DOWNLOAD := download/$(MUTAGEN_ARCHIVE)

INSTALL_PREFIX := $(PREFIX)/opt/mutagen

# 定义数据存储路径 (鸿蒙沙箱 files 目录，持久且支持 Socket)
# 注意：确保这个路径在所有场景下都是可写的
MUTAGEN_DATA_PATH := /data/storage/el2/base/files/.mutagen

.PHONY: all clean

all: $(MUTAGEN_DOWNLOAD)
	rm -rf build shim_config
	mkdir -p build

	# 解压
	tar -xf $(MUTAGEN_DOWNLOAD) -C build

	# 部署二进制
	mkdir -p build$(INSTALL_PREFIX)/bin
	cp build/mutagen build$(INSTALL_PREFIX)/bin/

	# 准备 sysroot
	mkdir -p ../sysroot
	cp -rfv build/$(PREFIX)/. ../sysroot/ | tee file.lst

	# ---------------------------------------------------------
	# 生成 Shim 配置文件 (核心修改)
	# ---------------------------------------------------------
	mkdir -p shim_config/mutagen
	echo '#ifndef CONFIG_H' > shim_config/mutagen/config.h
	echo '#define CONFIG_H' >> shim_config/mutagen/config.h
	
	# 1. 定义目标二进制路径
	echo '#define TARGET_COMMAND_PATH "$(INSTALL_PREFIX)/bin/mutagen"' >> shim_config/mutagen/config.h
	
	# 2. 定义环境变量 (利用 X-Macro)
	# 注意 echo 中的转义字符，确保生成的 C 代码格式正确
	echo '#define SHIM_ENV_VARS \' >> shim_config/mutagen/config.h
	echo '    X("MUTAGEN_DATA_DIRECTORY", "$(MUTAGEN_DATA_PATH)")' >> shim_config/mutagen/config.h

	echo '#endif' >> shim_config/mutagen/config.h
	# ---------------------------------------------------------

	# 编译 Shim
	$(MAKE) -C ../shim install SHIM_NAME=mutagen CONFIG_PATH=$(shell pwd)/shim_config/mutagen/config.h

$(MUTAGEN_DOWNLOAD):
	mkdir -p download
	$(WGET) -O $@ $(MUTAGEN_URL)

clean:
	rm -rf build temp download shim_config file.lst
