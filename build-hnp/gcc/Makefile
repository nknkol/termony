include ../utils/Makefrag

all: download/gcc-15.1.0.tar.xz
	rm -rf temp build
	mkdir -p temp build
	cd temp && tar xvf ../download/gcc-15.1.0.tar.xz
	cd temp/gcc-15.1.0 && mkdir build
	
	# [修改1] 配置阶段：增加 C++ 支持和共享库支持
	# --enable-languages=c,c++ : 开启 C++ 语言 (libstdc++ 需要)
	# --enable-shared          : 开启编译动态库 (.so)
	# --disable-libsanitizer   : 建议禁用，musl 环境下常出问题
	cd temp/gcc-15.1.0/build && ../configure --host $(OHOS_ARCH)-unknown-linux-musl \
		--enable-host-shared \
		--enable-languages=c,c++ \
		--enable-shared \
		--disable-libsanitizer \
		--disable-multilib

	# 第一次尝试编译 (保留你的逻辑，允许失败)
	cd temp/gcc-15.1.0/build && make all-gcc -j $(shell nproc) -k || true
	
	# 打补丁
	cd temp/gcc-15.1.0/build && cat ../../../0001-fix-gcc.diff | patch -Np1
	
	# [修改2] 正式编译阶段：显式增加 C++ 标准库和 GCC 运行时库的目标
	cd temp/gcc-15.1.0/build && make all-gcc all-target-libstdc++-v3 all-target-libgcc -j $(shell nproc)
	
	# [修改3] 安装阶段：显式增加库的安装
	cd temp/gcc-15.1.0/build && make install-gcc install-target-libstdc++-v3 install-target-libgcc -j $(shell nproc) DESTDIR=$(shell pwd)/build
	
	# 后处理 (保留你的 strip 逻辑)
	$(OHOS_SDK_HOME)/native/llvm/bin/llvm-strip build/usr/local/bin/*
	rm -f build/usr/local/libexec/gcc/$(OHOS_ARCH)-unknown-linux-musl/15.1.0/*.la
	find build/usr/local/libexec/gcc/$(OHOS_ARCH)-unknown-linux-musl/15.1.0/ -maxdepth 1 -type f | xargs $(OHOS_SDK_HOME)/native/llvm/bin/llvm-strip
	
	# [修改4] Strip 动态库 (减小体积)
	find build/usr/local/lib* -name "libstdc++.so*" | xargs $(OHOS_SDK_HOME)/native/llvm/bin/llvm-strip
	find build/usr/local/lib* -name "libgcc_s.so*" | xargs $(OHOS_SDK_HOME)/native/llvm/bin/llvm-strip

	# 拷贝到 sysroot
	mkdir -p ../sysroot
	cp -rfv build/usr/local/. ../sysroot | tee file.lst

download/gcc-15.1.0.tar.xz:
	mkdir -p download
	cd download && $(WGET) https://mirrors.tuna.tsinghua.edu.cn/gnu/gcc/gcc-15.1.0/gcc-15.1.0.tar.xz
