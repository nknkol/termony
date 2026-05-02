include ../utils/Makefrag

SOURCE_URL = https://gitee.com/openharmony/third_party_bzip2/repository/archive/OpenHarmony-5.0.1-Release.zip
SOURCE_FILE = bzip2-OpenHarmony-5.0.1-Release.zip

# === 核心逻辑 ===
# 1. 解压外层 zip
# 2. 找到内层 tar.gz 并解压 (关键步骤)
# 3. 进入解压出来的源码目录 (bzip2-1.0.8) 编译

all: download/$(SOURCE_FILE)
	@echo "==> Cleaning previous build..."
	rm -rf temp
	mkdir -p temp

	@echo "==> Unpacking Zip (Layer 1)..."
	cd temp && unzip -q ../download/$(SOURCE_FILE)
	
	@echo "==> Unpacking Tarball (Layer 2)..."
	# 显式查找并解压那个 tar.gz 文件
	# 使用 wildcard 通配符进入外层目录，然后解压 tar.gz 到 temp 根目录
	cd temp/* && tar xf bzip2-1.0.8.tar.gz -C ..
	
	@echo "==> Compiling libbz2.a..."
	# 此时源码一定在 temp/bzip2-1.0.8 下 (tar 包解压出来的默认名字)
	cd temp/bzip2-1.0.8 && \
		$(MAKE) libbz2.a \
		CC="$(CC)" \
		AR="$(AR)" \
		RANLIB="$(RANLIB)" \
		CFLAGS="$(CFLAGS) -D_FILE_OFFSET_BITS=64 -O2 -Wno-unused-variable"

	@echo "==> Installing to sysroot..."
	mkdir -p ../sysroot/lib
	mkdir -p ../sysroot/include
	
	# 复制文件
	cp -fv temp/bzip2-1.0.8/libbz2.a ../sysroot/lib/
	cp -fv temp/bzip2-1.0.8/bzlib.h ../sysroot/include/
	
	# 更新记录
	echo "../sysroot/lib/libbz2.a" > file.lst
	echo "../sysroot/include/bzlib.h" >> file.lst

# 下载规则
download/$(SOURCE_FILE):
	mkdir -p download
	cd download && $(WGET) -O $(SOURCE_FILE) $(SOURCE_URL)

.PHONY: all
